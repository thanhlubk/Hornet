#include <gl/glew.h>
#include <HornetView/GLViewWindow.h>
#include <QOpenGLShaderProgram>
#include <QSurfaceFormat>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDir>
#include <QtMath>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>
#include <utility>
#include <QPainter>
#include <array>
#include <string>
#include <QVector2D>
#include <QWidget>
#include "HornetBase/HINode.h"
#include "HornetBase/HIElement.h"
#include "HornetBase/HILbcForce.h"
#include "HornetBase/HILbcConstraint.h"

inline bool isRayIntersectTriangle(const QVector3D &orig, const QVector3D &dir, const QVector3D &v0, const QVector3D &v1, const QVector3D &v2, float &tOut, QVector3D &hitPosOut)
{
    const float EPS = 1e-7f;

    const QVector3D e1 = v1 - v0;
    const QVector3D e2 = v2 - v0;

    const QVector3D p = QVector3D::crossProduct(dir, e2);
    const float det = QVector3D::dotProduct(e1, p);
    if (std::abs(det) < EPS)
        return false; // parallel (double-sided test)

    const float invDet = 1.0f / det;
    const QVector3D tvec = orig - v0;

    const float u = QVector3D::dotProduct(tvec, p) * invDet;
    if (u < 0.0f || u > 1.0f)
        return false;

    const QVector3D q = QVector3D::crossProduct(tvec, e1);
    const float v = QVector3D::dotProduct(dir, q) * invDet;
    if (v < 0.0f || (u + v) > 1.0f)
        return false;

    const float t = QVector3D::dotProduct(e2, q) * invDet;
    if (t < EPS)
        return false; // behind origin / too close

    tOut = t;
    hitPosOut = orig + t * dir; // world-space intersection point
    return true;
}

GLViewWindow::GLViewWindow(QWindow *parent)
    : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate, parent)
    , m_pDb(nullptr)
{
    // QWidget-specific calls like setFocusPolicy / setMouseTracking
    // are NOT available here. Focus/mouse handling will be handled
    // by the QWidget container.
    // setFocusPolicy(Qt::StrongFocus);
    // setMouseTracking(true);

    // Request MSAA for smoother edges
    QSurfaceFormat surfaceFormat = format();
    if (surfaceFormat.samples() < 4)
    {
        surfaceFormat.setSamples(4);
        setFormat(surfaceFormat);
    }

    // Initialize members with defaults
    m_bPickMode = false;
    m_bRMBDown = false;
    m_bMMBDown = false;
    m_bBlockMouseLook = false;
    m_bDragSelect = false;
    m_iSelectRadius = 8;
    m_fMarqueeBorderWidth = 1.0f;
    m_colorMarquee = QColor(0, 120, 215, 48);
    m_colorMarqueeBorder = QColor(0, 120, 215, 200);
    m_ePrevSelectType = SelectType::None;

    // Same QObject ownership as before: these still take 'this' as parent
    m_pLighting = new HViewLighting(this);
    m_pCamera = new HViewCamera(this);
    m_pSelection = new HViewSelectionManager(this);

    m_pRenderCoordinate = new HRenderCoordinateGizmo(this);
    m_pRenderForce = new HRenderForce(this);
    m_pRenderModel = new HRenderModel(this);
    m_pRenderConstraint = new HRenderConstraint(this);
}

GLViewWindow::~GLViewWindow()
{
    destroyGLObjects(); // delete VAO/VBO/programs, gizmo, etc.
}

QWidget *GLViewWindow::createContainer(QWidget *parent)
{
    // The container will own this QOpenGLWindow (see docs).
    QWidget *container = QWidget::createWindowContainer(this, parent);
    // If you want strong focus / mouse tracking, set it on the container:
    container->setFocusPolicy(Qt::StrongFocus);
    container->setMouseTracking(true);
    return container;
}

void GLViewWindow::setDatabase(DatabaseSession *pDb)
{
    m_pDb = pDb;
}

void GLViewWindow::setNotifyDispatcher(NotifyDispatcher &disp)
{
    m_observer = disp.attach(this, &GLViewWindow::onNotify);
}

void GLViewWindow::onNotify(MessageType mess, MessageParam a, MessageParam b)
{
    if (mess == MessageType::DataModified || mess == MessageType::DataEmplaced)
    {
        rebuildFromDatabase();
    }
}

void GLViewWindow::rebuildFromDatabase()
{
    if (!m_pDb)
        return;

    // 1) Collect nodes from DB
    std::vector<QVector3D> positions;
    std::vector<QVector4D> nodeColors;
    std::vector<int> nodeIds;
    std::unordered_map<int, int> nodeIdToIndex;

    m_mapNodeIdPos.clear();

    auto pPoolNode = m_pDb->getPoolUnique(CategoryType::CatNode);
    if (pPoolNode)
    {
        for (auto any : pPoolNode->range())
        {
            auto crNode = std::launder(reinterpret_cast<HCursor *>(any));
            auto pNode = crNode->item<HINode>();
            if (!pNode)
                continue;

            int id = static_cast<int>(crNode->id());
            HVector3d pos = pNode->position();
            HColor col = pNode->color();

            int idx = static_cast<int>(positions.size());
            QVector3D qpos(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z));
            positions.push_back(qpos);
            nodeColors.emplace_back(col.r() / 255.0f, col.g() / 255.0f, col.b() / 255.0f, col.a() / 255.0f);
            nodeIds.push_back(id);
            nodeIdToIndex[id] = idx;

            m_mapNodeIdPos[id] = qpos;
        }
    }

    // 2) Collect elements from DB
    std::vector<RenderElementData> elements;
    m_elements.clear();

    auto pPoolElement = m_pDb->getPoolMix(CategoryType::CatElement);
    if (pPoolElement)
    {
        for (auto any : pPoolElement->range())
        {
            auto crElement = std::launder(reinterpret_cast<HCursor *>(any));
            auto pElement = crElement->item<HIElement>();
            if (!pElement)
                continue;

            RenderElementData ed;
            ed.id = static_cast<int>(crElement->id());
            ed.type = pElement->elementType();

            HColor ecol = pElement->color();
            ed.r = ecol.r() / 255.0f;
            ed.g = ecol.g() / 255.0f;
            ed.b = ecol.b() / 255.0f;
            ed.a = ecol.a() / 255.0f;

            auto nodeSpan = pElement->nodes();
            int nCount = std::min(static_cast<int>(nodeSpan.size()), 20);
            for (int i = 0; i < nCount; ++i)
            {
                if (nodeSpan[i])
                    ed.v[i] = static_cast<int>(nodeSpan[i]->id());
                else
                    ed.v[i] = 0;
            }

            elements.push_back(ed);
        }
    }

    m_elements = elements;

    // 3) Collect forces and constraints from DB
    std::vector<RenderForceData> renderForces;
    std::vector<RenderConstraintData> renderConstraints;
    auto pPoolLbc = m_pDb->getPoolMix(CategoryType::CatLbc);
    if (pPoolLbc)
    {
        for (auto any : pPoolLbc->range())
        {
            auto crLbc = std::launder(reinterpret_cast<HCursor *>(any));
            int id = static_cast<int>(crLbc->id());

            if (auto pForce = crLbc->item<HILbcForce>())
            {
                HVector3d fdir = pForce->force();
                QVector3D qdir(static_cast<float>(fdir.x), static_cast<float>(fdir.y), static_cast<float>(fdir.z));

                for (auto crNode : pForce->targets())
                {
                    if (!crNode) continue;
                    auto pNode = crNode->item<HINode>();
                    if (!pNode) continue;
                    
                    HVector3d pos = pNode->position();
                    QVector3D qpos(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z));
                    
                    RenderForceData fd;
                    fd.id = id;
                    fd.position = qpos;
                    fd.direction = qdir;
                    renderForces.push_back(fd);
                }
            }
            else if (auto pConstraint = crLbc->item<HILbcConstraint>())
            {
                for (auto crNode : pConstraint->targets())
                {
                    if (!crNode) continue;
                    auto pNode = crNode->item<HINode>();
                    if (!pNode) continue;
                    
                    HVector3d pos = pNode->position();
                    QVector3D qpos(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z));
                    
                    auto addCone = [&](LbcConstraintDof dofFlag, QVector3D dir) {
                        if (pConstraint->hasDof(dofFlag)) {
                            RenderConstraintData cd;
                            cd.id = id;
                            cd.position = qpos;
                            cd.direction = dir;
                            renderConstraints.push_back(cd);
                        }
                    };

                    addCone(LbcConstraintDof::LbcConstraintDofTx, QVector3D(1, 0, 0));
                    addCone(LbcConstraintDof::LbcConstraintDofTy, QVector3D(0, 1, 0));
                    addCone(LbcConstraintDof::LbcConstraintDofTz, QVector3D(0, 0, 1));
                    addCone(LbcConstraintDof::LbcConstraintDofRx, QVector3D(-1, 0, 0));
                    addCone(LbcConstraintDof::LbcConstraintDofRy, QVector3D(0, -1, 0));
                    addCone(LbcConstraintDof::LbcConstraintDofRz, QVector3D(0, 0, -1));
                }
            }
        }
    }

    // 4) Feed to renderer
    m_pRenderModel->setMesh(positions, nodeColors, nodeIds, nodeIdToIndex, elements);
    if (m_pRenderForce)
        m_pRenderForce->setForces(renderForces);
    if (m_pRenderConstraint)
        m_pRenderConstraint->setConstraints(renderConstraints);

    fitView();
    m_pCamera->setFocus(m_pRenderModel->center());

    update();
}
void GLViewWindow::fitView()
{
    m_pCamera->frameBounds(m_pRenderModel->center(), m_pRenderModel->frameRadius(), width(), height());
    update();
}

void GLViewWindow::setAlwaysMouseLook(bool on) 
{ 
    m_bBlockMouseLook = on; 
}

void GLViewWindow::setPickRadius(int pixel)
{
    m_iSelectRadius = std::max(1, pixel);
}

void GLViewWindow::setMarqueeFillColor(const QColor &color)
{
    m_colorMarquee = color;
    update();
}

void GLViewWindow::setMarqueeStrokeColor(const QColor &color)
{
    m_colorMarqueeBorder = color;
    update();
}

void GLViewWindow::setMarqueeStrokeWidth(float pixel)
{
    m_fMarqueeBorderWidth = std::max(0.0f, pixel);
    update();
}

QColor GLViewWindow::marqueeFillColor() const 
{
    return m_colorMarquee; 
}

QColor GLViewWindow::marqueeStrokeColor() const 
{ 
    return m_colorMarqueeBorder; 
}

float GLViewWindow::marqueeStrokeWidth() const 
{ 
    return m_fMarqueeBorderWidth; 
}

HViewCamera *GLViewWindow::camera() 
{ 
    return m_pCamera; 
}

HViewLighting *GLViewWindow::lighting() 
{
    return m_pLighting; 
}

HViewSelectionManager *GLViewWindow::selectionManager() 
{ 
    return m_pSelection; 
}

HRenderCoordinateGizmo *GLViewWindow::coordinateGizmo()
{ 
    return m_pRenderCoordinate; 
}

HRenderModel *GLViewWindow::modelRenderer() 
{ 
    return m_pRenderModel; 
}

HRenderForce *GLViewWindow::forceRenderer() 
{ 
    return m_pRenderForce; 
}

HRenderConstraint *GLViewWindow::constraintRenderer() 
{ 
    return m_pRenderConstraint; 
}

void GLViewWindow::initializeGL()
{
    GLenum err = glewInit();
    if (err != GLEW_OK)
        return;

    initializeOpenGLFunctions();
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    if (auto *ctx = context()) {
        connect(ctx, &QOpenGLContext::aboutToBeDestroyed,
                this, [this]() { destroyGLObjects(); },
                Qt::DirectConnection);
    }

    // New: init renderers + gizmo
    m_pRenderModel->initialize();
    m_pRenderCoordinate->initialize();
    m_pRenderForce->initialize();
    m_pRenderConstraint->initialize();

    connect(m_pLighting, &HViewLighting::lightingChanged, this, [this]() { update(); });
    connect(m_pCamera, &HViewCamera::viewChanged, this, [this]() { update(); });
    connect(m_pSelection, &HViewSelectionManager::selectionChanged, this, [this]() { update(); });
    connect(m_pRenderCoordinate, &HRenderCoordinateGizmo::dataChanged, this, [this]() { update(); });
    connect(m_pRenderForce, &HRenderForce::dataChanged, this, [this]() { update(); });
    connect(m_pRenderConstraint, &HRenderConstraint::dataChanged, this, [this]() { update(); });
    connect(m_pRenderModel, &HRenderModel::settingChanged, this, [this]() { update(); });

    // QObject::connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, [this] { destroyGLObjects(); }, Qt::DirectConnection);
}

void GLViewWindow::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void GLViewWindow::paintGL()
{
    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get matrices from the new Camera
    const QMatrix4x4 P = m_pCamera->projectionMatrix(width(), height());
    const QMatrix4x4 V = m_pCamera->viewMatrix();

    // Main mesh
    m_pRenderModel->draw(P, V, m_pLighting);

    // Your existing overlays (forces/highlights) still use m_prog.
    // If you keep them, leave drawForces() and drawHighlights() calls here:
    m_pRenderForce->draw(P, V, *m_pLighting, width(), height());
    m_pRenderConstraint->draw(P, V, *m_pLighting, width(), height());
    // Bottom-left mini-axes (drawn last, in its own mini-viewport)
    m_pRenderCoordinate->draw(m_pCamera->rotation(), width(), height());

    // === 2D UI overlay: marquee rectangle ===
    if (m_bDragSelect)
    {
        // Build rect in Qt widget pixels
        const QRectF rect(
            QPointF(std::min(m_pointDragStartPos.x(), m_pointDragCurrentPos.x()), 
                    std::min(m_pointDragStartPos.y(), m_pointDragCurrentPos.y())), 
            QPointF(std::max(m_pointDragStartPos.x(), m_pointDragCurrentPos.x()), 
                    std::max(m_pointDragStartPos.y(), m_pointDragCurrentPos.y())));
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true); // crisp edges
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        QPen pen(m_colorMarqueeBorder);
        pen.setWidthF(m_fMarqueeBorderWidth);
        painter.setPen(pen);
        painter.setBrush(QBrush(m_colorMarquee));
        painter.drawRect(rect);
    }
}

void GLViewWindow::keyPressEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat())
    {
        if (event->key() == Qt::Key_Tab)
        {
            // Switch control mode
            m_pCamera->setControlMode(m_pCamera->controlMode() == ControlMode::FPS ? ControlMode::OrbitPan : ControlMode::FPS);
        }
        else if (event->key() == Qt::Key_P)
        {
            // Quick toggle projection for testing
            m_pCamera->setProjection(m_pCamera->projection() == Projection::Perspective ? Projection::Orthographic : Projection::Perspective);
        }
        else if (event->key() == Qt::Key_M)
        {
            // toggle focus-pick mode
            if (!m_bPickMode)
            {
                // entering pick mode: temporarily disable selection and any marquee
                m_ePrevSelectType = m_pSelection->type();
                m_pSelection->setType(SelectType::None); // turn off Node/Element/Force selection
                m_bDragSelect = false;         // cancel any drag selection in progress

                m_bPickMode = true;
                setCursor(Qt::CrossCursor); // (or QApplication::setOverrideCursor if you use that)
            }
            else
            {
                // leaving pick mode without selecting a point: restore selection mode
                m_bPickMode = false;
                unsetCursor(); // (or QApplication::restoreOverrideCursor)
                m_pSelection->setType(m_ePrevSelectType);
            }
            update();
        }
        else if (event->key() == Qt::Key_W)
            m_pCamera->move(HViewCamera::Forward);
        else if (event->key() == Qt::Key_S)
            m_pCamera->move(HViewCamera::Backward);
        else if (event->key() == Qt::Key_A)
            m_pCamera->move(HViewCamera::Left);
        else if (event->key() == Qt::Key_D)
            m_pCamera->move(HViewCamera::Right);
        else if (event->key() == Qt::Key_Q)
            m_pCamera->move(HViewCamera::Up);
        else if (event->key() == Qt::Key_E)
            m_pCamera->move(HViewCamera::Down);
        }
        event->accept();
}

void GLViewWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_W)
        m_pCamera->stop(HViewCamera::Forward);
    else if (event->key() == Qt::Key_S)
        m_pCamera->stop(HViewCamera::Backward);
    else if (event->key() == Qt::Key_A)
        m_pCamera->stop(HViewCamera::Left);
    else if (event->key() == Qt::Key_D)
        m_pCamera->stop(HViewCamera::Right);
    else if (event->key() == Qt::Key_Q)
        m_pCamera->stop(HViewCamera::Up);
    else if (event->key() == Qt::Key_E)
        m_pCamera->stop(HViewCamera::Down);
    event->accept();
}

void GLViewWindow::mousePressEvent(QMouseEvent *event)
{
    if (m_bPickMode && event->button() == Qt::LeftButton)
    {
        QVector3D vHitPos;
        int iElemId = -1;
        if (getHitPosition(event->position(), vHitPos, iElemId))
        {
            m_pCamera->setFocus(vHitPos);
            m_pCamera->recenterToFocus();
            // leave pick mode and restore whichever selection mode was active before
            m_bPickMode = false;
            unsetCursor();
            m_pSelection->setType(m_ePrevSelectType);
            update();
        }
        // consume the click (even if no hit, stay in pick mode so the user can try again)
        event->accept();
        return;
    }
    
    // Begin marquee or click selection when selection mode is active
    if (!m_bPickMode && event->button() == Qt::LeftButton && m_pSelection->type() != SelectType::None)
    {
        m_bDragSelect = true;
        m_pointDragStartPos = event->position();
        m_pointDragCurrentPos = m_pointDragStartPos;
        update(); // so a marquee (if you draw one) appears immediately
        event->accept();
        return;
    }
    
    if (event->button() == Qt::RightButton)
    {
        m_bRMBDown = true;
        m_pointLastMousePos = event->position();
        if (m_pCamera->controlMode() == ControlMode::FPS)
            setCursor(Qt::BlankCursor);
    }
    else if (event->button() == Qt::MiddleButton)
    {
        m_bMMBDown = true;
        m_pointLastMousePos = event->position();
    }

    if (m_bBlockMouseLook && m_pCamera->controlMode() == ControlMode::FPS)
        m_pointLastMousePos = event->position();

    event->accept();
}

void GLViewWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_bDragSelect && event->button() == Qt::LeftButton)
    {
        const QRectF rectMarquee = QRectF(
            QPointF(std::min(m_pointDragStartPos.x(), event->position().x()), 
                    std::min(m_pointDragStartPos.y(), event->position().y())), 
            QPointF(std::max(m_pointDragStartPos.x(), event->position().x()), 
                    std::max(m_pointDragStartPos.y(), event->position().y())));

        if (rectMarquee.width() * rectMarquee.height() < 9)
            selectAtPoint(event->position());
        else
            selectInRect(rectMarquee);
            
        m_bDragSelect = false;
        update();
        event->accept();
        return;
    }

    if (event->button() == Qt::RightButton)
    {
        m_bRMBDown = false;
        if (m_pCamera->controlMode() == ControlMode::FPS && !m_bPickMode)
            unsetCursor();
    }
    else if (event->button() == Qt::MiddleButton)
    {
        m_bMMBDown = false;
    }
    event->accept();
}

void GLViewWindow::mouseMoveEvent(QMouseEvent *event)
{
    const QPointF pointPrevPos = m_pointLastMousePos;
    const QPointF pointCurrentPos = event->position();
    const QPointF pointDistance = pointCurrentPos - pointPrevPos;
    m_pointLastMousePos = pointCurrentPos;

    if (m_pCamera->controlMode() == ControlMode::FPS)
    {
        // RMB look (or always-look) + WASDQE handled in onFrameTick
        if (m_bRMBDown || m_bBlockMouseLook)
        {
            m_pCamera->fpsLook(float(pointDistance.x()), float(pointDistance.y()));
            update();
        }
        event->accept();
        return;
    }

    // While dragging a marquee, just update the rectangle and repaint
    if (m_bDragSelect)
    {
        m_pointDragCurrentPos = event->position();
        update();
        event->accept();
        return;
    }

    // Orbit / pan
    if (m_bMMBDown)
    {
        m_pCamera->orbitArcball(pointPrevPos, pointCurrentPos, width(), height());
        update();
        event->accept();
        return;
    }
    if (m_bRMBDown)
    {
        m_pCamera->orbitPan(float(pointDistance.x()), float(pointDistance.y()), height());
        update();
        event->accept();
        return;
    }
    event->ignore();
}

void GLViewWindow::wheelEvent(QWheelEvent *event)
{
    const float fSteps = event->angleDelta().y() / 120.f;
    if (m_pCamera->projection() == Projection::Orthographic)
    {
        float fScale = std::pow(1.f - m_pCamera->zoomStep(), fSteps);

        m_pCamera->setOrthoHeight(std::clamp(m_pCamera->orthoHeight() * (1.f / fScale), 1e-4f, 1e8f));
    }
    else
    {
        m_pCamera->orbitZoomSteps(fSteps); // Camera uses its own zoomStep internally
    }
    update();
    event->accept();
}

void GLViewWindow::destroyGLObjects()
{
    QOpenGLContext *pContext = context();
    if (!pContext)
        return; // context already gone — nothing we can safely delete

    // Only take/release the context if it's not already current on this thread
    const bool bMake = (QOpenGLContext::currentContext() != pContext);
    if (bMake)
        makeCurrent();

    // ---- delete your GL stuff here ----
    m_pRenderConstraint->destroy();
    m_pRenderForce->destroy();
    m_pRenderModel->destroy();
    m_pRenderCoordinate->destroy();

    if (bMake)
        doneCurrent();
}

bool GLViewWindow::getHitPosition(const QPointF &point, QVector3D &hit, int &elemId) const
{
    if (width() <= 0 || height() <= 0)
        return false;

    QVector3D vRayOrigin;
    if (!convertScreenToWorld(m_pCamera, QVector3D(point.x(), point.y(), -1.0f), vRayOrigin))
        return false;

    QVector3D vRayTarget;
    if (!convertScreenToWorld(m_pCamera, QVector3D(point.x(), point.y(), 1.1f), vRayTarget))
        return false;

    QVector3D vRayDirection = vRayTarget - vRayOrigin;
    if (!vRayDirection.isNull())
        vRayDirection.normalize();

    float fDistance = std::numeric_limits<float>::max();
    bool bHit = false;

    for (const auto &elem : m_elements)
    {
        std::vector<std::tuple<QVector3D, QVector3D, QVector3D>> vecTriangles;
        switch (elem.type)
        {
        case ElementType::ElementTypeTri3:
        case ElementType::ElementTypeTri6:
            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[0]), m_mapNodeIdPos.at(elem.v[1]), m_mapNodeIdPos.at(elem.v[2]));
            break;
        case ElementType::ElementTypeQuad4:
        case ElementType::ElementTypeQuad8:
            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[0]), m_mapNodeIdPos.at(elem.v[1]), m_mapNodeIdPos.at(elem.v[2]));
            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[0]), m_mapNodeIdPos.at(elem.v[2]), m_mapNodeIdPos.at(elem.v[3]));
            break;
        case ElementType::ElementTypeTet4:
        case ElementType::ElementTypeTet10:
            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[0]), m_mapNodeIdPos.at(elem.v[1]), m_mapNodeIdPos.at(elem.v[2]));
            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[0]), m_mapNodeIdPos.at(elem.v[3]), m_mapNodeIdPos.at(elem.v[1]));
            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[1]), m_mapNodeIdPos.at(elem.v[3]), m_mapNodeIdPos.at(elem.v[2]));
            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[2]), m_mapNodeIdPos.at(elem.v[3]), m_mapNodeIdPos.at(elem.v[0]));
            break;
        case ElementType::ElementTypeHex8:
        case ElementType::ElementTypeHex20:
            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[0]), m_mapNodeIdPos.at(elem.v[1]), m_mapNodeIdPos.at(elem.v[2]));
            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[0]), m_mapNodeIdPos.at(elem.v[2]), m_mapNodeIdPos.at(elem.v[3]));

            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[4]), m_mapNodeIdPos.at(elem.v[5]), m_mapNodeIdPos.at(elem.v[6]));
            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[4]), m_mapNodeIdPos.at(elem.v[6]), m_mapNodeIdPos.at(elem.v[7]));

            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[0]), m_mapNodeIdPos.at(elem.v[1]), m_mapNodeIdPos.at(elem.v[5]));
            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[0]), m_mapNodeIdPos.at(elem.v[5]), m_mapNodeIdPos.at(elem.v[4]));

            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[1]), m_mapNodeIdPos.at(elem.v[2]), m_mapNodeIdPos.at(elem.v[6]));
            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[1]), m_mapNodeIdPos.at(elem.v[6]), m_mapNodeIdPos.at(elem.v[5]));

            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[2]), m_mapNodeIdPos.at(elem.v[3]), m_mapNodeIdPos.at(elem.v[7]));
            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[2]), m_mapNodeIdPos.at(elem.v[7]), m_mapNodeIdPos.at(elem.v[6]));

            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[3]), m_mapNodeIdPos.at(elem.v[0]), m_mapNodeIdPos.at(elem.v[4]));
            vecTriangles.emplace_back(m_mapNodeIdPos.at(elem.v[3]), m_mapNodeIdPos.at(elem.v[4]), m_mapNodeIdPos.at(elem.v[7]));
            break;
        default:
            break;
        }

        for (const auto &[posNode1, posNode2, posNode3] : vecTriangles)
        {
            float fCurrentDistance;
            QVector3D vCurrentHitPos;
            if (isRayIntersectTriangle(vRayOrigin, vRayDirection, posNode1, posNode2, posNode3, fCurrentDistance, vCurrentHitPos))
            {
                if (fCurrentDistance < fDistance)
                {
                    fDistance = fCurrentDistance;
                    hit = vCurrentHitPos;
                    bHit = true;
                    elemId = elem.id;
                }
            }
        }
    }

    return bHit;
}

void GLViewWindow::selectAtPoint(const QPointF &point)
{
    std::vector<int> pickedNodes, pickedElems, pickedForces, pickedConstraints;
    if (m_pSelection->type() == SelectType::None)
        return;

    if (m_pSelection->type() == SelectType::Node)
    {
        int iFindId = -1;
        float fDistance = std::numeric_limits<float>::max();
        QVector3D vMousePosition = QVector3D(point.x(), point.y(), 0);
        for (auto itr = m_mapNodeIdPos.begin(); itr != m_mapNodeIdPos.end(); itr++)
        {
            QVector3D vScreenPos;
            if (convertWorldToScreen(m_pCamera, itr->second, vScreenPos))
            {
                vScreenPos.setZ(0);
                float fCurrentDistance = std::abs((vScreenPos - vMousePosition).length());
                if (fCurrentDistance < m_iSelectRadius && fCurrentDistance < fDistance)
                {
                    fDistance = fCurrentDistance;
                    iFindId = itr->first;
                }
            }
        }

        if (iFindId >= 0)
            pickedNodes.push_back(iFindId);
    }
    else if (m_pSelection->type() == SelectType::Element)
    {
        int iFindId = -1;
        QVector3D vHitPosition;
        getHitPosition(point, vHitPosition, iFindId);
        if (iFindId >= 0)
            pickedElems.push_back(iFindId);
    }
    else if (m_pSelection->type() == SelectType::Force)
    {
        int best = -1;
        float bestD2 = 1e30f;
        float tol2 = float(m_iSelectRadius * m_iSelectRadius);
        for (const auto &f : m_pRenderForce->forces())
        {
            QVector3D sp;
            if (!convertWorldToScreen(m_pCamera, f.position, sp))
                continue;
            const float dx = float(point.x()) - sp.x(), dy = float(point.y()) - sp.y();
            const float d2 = dx * dx + dy * dy;
            if (d2 <= tol2 && d2 < bestD2)
            {
                bestD2 = d2;
                best = f.id;
            }
        }
        if (best >= 0)
            pickedForces.push_back(best);
    }
    else if (m_pSelection->type() == SelectType::Constraint)
    {
        int best = -1;
        float bestD2 = 1e30f;
        float tol2 = float(m_iSelectRadius * m_iSelectRadius);
        for (const auto &c : m_pRenderConstraint->constraints())
        {
            QVector3D sp;
            if (!convertWorldToScreen(m_pCamera, c.position, sp))
                continue;
            const float dx = float(point.x()) - sp.x(), dy = float(point.y()) - sp.y();
            const float d2 = dx * dx + dy * dy;
            if (d2 <= tol2 && d2 < bestD2)
            {
                bestD2 = d2;
                best = c.id;
            }
        }
        if (best >= 0)
            pickedConstraints.push_back(best);
    }

    if (!pickedNodes.empty())
    {
        m_pSelection->setType(SelectType::Node);
        m_pSelection->setSelectedNodeIds(pickedNodes);
    }
    else if (!pickedElems.empty())
    {
        m_pSelection->setType(SelectType::Element);
        m_pSelection->setSelectedElementIds(pickedElems);
    }
    else if (!pickedForces.empty())
    {
        m_pSelection->setType(SelectType::Force);
        m_pSelection->setSelectedForceIds(pickedForces);
    }
    else if (!pickedConstraints.empty())
    {
        m_pSelection->setType(SelectType::Constraint);
        m_pSelection->setSelectedConstraintIds(pickedConstraints);
    }
    m_pSelection->applyTo(m_pRenderModel, m_pRenderForce, m_pRenderConstraint);
}

void GLViewWindow::selectInRect(const QRectF &rectMarquee)
{
    std::vector<int> pickedNodes, pickedElems, pickedForces, pickedConstraints;
    const auto checkContains = [&](const QPointF &q)
    { return rectMarquee.contains(q.toPoint()); };

    if (m_pSelection->type() == SelectType::Node)
    {
        for (auto itr = m_mapNodeIdPos.begin(); itr != m_mapNodeIdPos.end(); itr++)
        {
            QVector3D vScreenPos;
            if (convertWorldToScreen(m_pCamera, itr->second, vScreenPos) && checkContains(QPointF(vScreenPos.x(), vScreenPos.y())))
            {
                pickedNodes.push_back(itr->first);
            }
        }
    }
    else if (m_pSelection->type() == SelectType::Element)
    {
        // auto triRect = [&](int ia, int ib, int ic, int elemId)
        // {
        //     int A = m_nodeIndexById.value(ia, -1), B = m_nodeIndexById.value(ib, -1), C = m_nodeIndexById.value(ic, -1);
        //     if (A < 0 || B < 0 || C < 0)
        //         return;
        //     QPointF a, b, c;
        //     if (!worldToScreen(m_positions[A], a, nullptr))
        //         return;
        //     if (!worldToScreen(m_positions[B], b, nullptr))
        //         return;
        //     if (!worldToScreen(m_positions[C], c, nullptr))
        //         return;
        //     if (r.contains(a.toPoint()) || r.contains(b.toPoint()) || r.contains(c.toPoint()) ||
        //         pointInTri2D(r.center(), a, b, c))
        //         pickedElems.push_back(elemId);
        // };

        // for (const auto &e : m_elements)
        // {
        //     switch (e.type)
        //     {
        //     case Element::Type::Tri3:
        //         triRect(e.v[0], e.v[1], e.v[2], e.id);
        //         break;
        //     case Element::Type::Tri6:
        //         triRect(e.v[0], e.v[1], e.v[2], e.id);
        //         break;
        //     case Element::Type::Quad4:
        //     case Element::Type::Quad8:
        //         triRect(e.v[0], e.v[1], e.v[2], e.id);
        //         triRect(e.v[0], e.v[2], e.v[3], e.id);
        //         break;

        //     case Element::Type::Tet4:
        //     case Element::Type::Tet10:
        //     case Element::Type::Hex8:
        //     case Element::Type::Hex20:
        //         forEachSolidSurface(e,
        //                             /*tri*/ [&](int a, int b, int c)
        //                             { triRect(a, b, c, e.id); },
        //                             /*edge*/ [](int, int) {});
        //         break;

        //     default:
        //         break;
        //     }
        // }
        // std::sort(pickedElems.begin(), pickedElems.end());
        // pickedElems.erase(std::unique(pickedElems.begin(), pickedElems.end()), pickedElems.end());
    }
    else if (m_pSelection->type() == SelectType::Force)
    {
        for (const auto &f : m_pRenderForce->forces())
        {
            QVector3D sp;
            if (convertWorldToScreen(m_pCamera, f.position, sp) && checkContains(QPointF(sp.x(), sp.y())))
            {
                pickedForces.push_back(f.id);
            }
        }
    }
    else if (m_pSelection->type() == SelectType::Constraint)
    {
        for (const auto &c : m_pRenderConstraint->constraints())
        {
            QVector3D sp;
            if (convertWorldToScreen(m_pCamera, c.position, sp) && checkContains(QPointF(sp.x(), sp.y())))
            {
                pickedConstraints.push_back(c.id);
            }
        }
    }

    if (!pickedNodes.empty())
    {
        m_pSelection->setType(SelectType::Node);
        m_pSelection->setSelectedNodeIds(pickedNodes);
    }
    else if (!pickedElems.empty())
    {
        m_pSelection->setType(SelectType::Element);
        m_pSelection->setSelectedElementIds(pickedElems); 
    }
    else if (!pickedForces.empty())
    {
        m_pSelection->setType(SelectType::Force);
        m_pSelection->setSelectedForceIds(pickedForces);
    }
    else if (!pickedConstraints.empty())
    {
        m_pSelection->setType(SelectType::Constraint);
        m_pSelection->setSelectedConstraintIds(pickedConstraints);
    }
    m_pSelection->applyTo(m_pRenderModel, m_pRenderForce, m_pRenderConstraint);
}

bool GLViewWindow::convertWorldToScreen(const HViewCamera *pCamera, const QVector3D &vWorldPosition, QVector3D &vScreenPosition) const
{
    if (width() <= 0 || height() <= 0)
        return false;

    // use the SAME matrices you render with
    const QMatrix4x4 matView = pCamera->viewMatrix();
    const QMatrix4x4 matProjection = pCamera->projectionMatrix(width(), height());

    // world -> clip
    const QVector4D matClip = matProjection * matView * QVector4D(vWorldPosition, 1.0f);
    const float w = matClip.w();
    if (std::abs(w) < 1e-6f)
        return false; // avoid divide-by-zero

    // clip -> NDC
    const float fNDCX = matClip.x() / w;
    const float fNDCY = matClip.y() / w;
    const float fNDCZ = matClip.z() / w; // [-1..+1] in OpenGL

    // NDC -> screen
    const float fScreenX = (fNDCX * 0.5f + 0.5f) * width();
    const float fScreenY = (1.0f - (fNDCY * 0.5f + 0.5f)) * height(); // flip Y for Qt
    vScreenPosition.setX(fScreenX);
    vScreenPosition.setY(fScreenY);
    vScreenPosition.setZ(fNDCZ); // keep NDC z (visible if -1..+1)

    // “visible-ish” if inside canonical clip depth
    return (fNDCZ >= -1.0f && fNDCZ <= +1.0f);
}

bool GLViewWindow::convertScreenToWorld(const HViewCamera *pCamera, const QVector3D &vScreenPosition, QVector3D &vWorldPosition) const
{
    // Guard: widget size must be valid
    if (width() <= 0 || height() <= 0)
        return false;

    // --- 1) Qt widget pixels (origin top-left) -> NDC [-1,+1] ---
    // Note: DPR cancels out here; this is exactly the inverse of the mapping
    // you use when projecting to Qt pixels.
    const float fNDCX = 2.0f * (float(vScreenPosition.x()) / float(width())) - 1.0f;
    const float fNDCY = 1.0f - 2.0f * (float(vScreenPosition.y()) / float(height()));
    const float fNDCZ = vScreenPosition.z(); // caller supplies NDC z (−1 near .. +1 far)

    // --- 2) Unproject NDC -> world using the SAME matrices you draw with ---
    // If your projectionMatrix(...) uses framebuffer size (width()*dpr, height()*dpr),
    // do the same here for consistency.
    const QMatrix4x4 matProjection = pCamera->projectionMatrix(width(), height());
    const QMatrix4x4 matView = pCamera->viewMatrix();

    bool bRet = false;
    const QMatrix4x4 matInv = (matProjection * matView).inverted(&bRet);
    if (!bRet)
        return false;

    const QVector4D vNDC(fNDCX, fNDCY, fNDCZ, 1.0f);
    QVector4D vWorld = matInv * vNDC;
    if (std::abs(vWorld.w()) < 1e-6f)
        return false; // avoid divide-by-zero

    vWorld /= vWorld.w();
    vWorldPosition = vWorld.toVector3D();
    return true;
}
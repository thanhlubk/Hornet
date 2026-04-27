#include "HornetWindow.h"
#include <HornetView/GLViewWindow.h>
#include "ui_HornetWindow.h"
#include <HornetView/HViewDef.h>
#include <HornetBase/HINode.h>
#include <HornetBase/HIElement.h>
#include <HornetBase/AppBase.h>
#include <HornetBase/DocumentManager.h>
#include "DocumentModel.h"
#include <QBoxLayout>
#include <HornetBase/HILbcForce.h>
#include <HornetBase/HILbcConstraint.h>

namespace
{ 
// ---- tiny helpers ----
}

HornetWindow::HornetWindow(AppBase* app, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::HornetWindow), m_app(app)
{
    m_pViewWidget = nullptr;
    // Init window
    initWindow();
}

HornetWindow::~HornetWindow()
{
    ui->centralwidget->layout()->removeWidget(m_pViewWidget);
    m_pViewWidget->setParent(nullptr);
    m_pViewWidget = nullptr;
    delete ui;
}

void HornetWindow::initWindow()
{
    // Remove native title bar
    // setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    // setAttribute(Qt::WA_TranslucentBackground, false);

    // Setup UI
    ui->setupUi(this);

    connect(ui->pushButton, &QPushButton::clicked, this, &HornetWindow::onImportModel);

    createDocumentModel();
}

void HornetWindow::onImportModel()
{
    auto pDoc = dynamic_cast<DocumentModel*>(m_app->docs()->activeDocument());
    if (pDoc)
    {
        auto pDb = pDoc->database();
        if (pDb)
        {
            pDb->beginTransaction();
            pDb->emplace<HINode>(1);
            pDb->emplace<HINode>(2);
            pDb->emplace<HINode>(3);
            pDb->emplace<HINode>(4);

            pDb->emplace<HIElementTri3>(1);
            pDb->emplace<HIElement>(2, ElementType::ElementTypeTri3);

            auto pNode1 = pDb->checkOut<HINode>(1);
            pNode1->setPosition({0, 0, 0});

            auto pNode2 = pDb->checkOut<HINode>(2);
            pNode2->setPosition({1, 0, 0});

            auto pNode3 = pDb->checkOut<HINode>(3);
            pNode3->setPosition({0, 1, 0});

            auto pNode4 = pDb->checkOut<HINode>(4);
            pNode4->setPosition({1, 1, 0});

            auto pElem1 = pDb->checkOut<HIElementTri3>(1);
            std::vector<HCursor*> myCursors1 = {pNode1->getCursor(), pNode2->getCursor(), pNode3->getCursor()};
            pElem1->setNodes(myCursors1);

            auto pElem2 = pDb->checkOut<HIElementTri3>(2);
            std::vector<HCursor*> myCursors2 = {pNode2->getCursor(), pNode4->getCursor(), pNode3->getCursor()};
            pElem2->setNodes(myCursors2);

            pDb->emplace<HILbcForce>(1);
            pDb->emplace<HILbcForce>(2);
            pDb->emplace<HILbcConstraint>(3);
            pDb->emplace<HILbcConstraint>(4);

            auto pLbcForce1 = pDb->checkOut<HILbcForce>(1);
            auto pLbcForce2 = pDb->checkOut<HILbcForce>(2);
            auto pLbcConstraint1 = pDb->checkOut<HILbcConstraint>(3);
            auto pLbcConstraint2 = pDb->checkOut<HILbcConstraint>(4);

            pLbcForce1->addTarget(pNode1->getCursor());
            pLbcForce1->addTarget(pNode2->getCursor());
            pLbcForce1->setForce({1, 0, 0});

            pLbcForce2->addTarget(pNode3->getCursor());
            pLbcForce2->addTarget(pNode4->getCursor());
            pLbcForce2->setForce({0, 1, 0});

            pLbcConstraint1->addTarget(pNode2->getCursor());
            pLbcConstraint2->addTarget(pNode1->getCursor());

            pLbcConstraint1->addDof(LbcConstraintDof::LbcConstraintDofAll);
            pLbcConstraint2->addDof(LbcConstraintDof::LbcConstraintDofAll);

            pDb->commitTransaction();
        }
    }
}

void HornetWindow::createDocumentModel()
{
    // Setup document
    auto pDoc = m_app->docs()->createDocument<DocumentModel>();

    m_app->docs()->setActiveDocument(pDoc);
    auto pView = new GLViewWindow(nullptr);
    pDoc->setView(pView);

    pView->modelRenderer()->setShowFaces(true);
    pView->modelRenderer()->setShowEdges(true);
    pView->modelRenderer()->setShowNodes(true); // interior points off for clarity

    pView->fitView();

    pView->camera()->setFocus(QVector3D(0, 0, 0));

    pView->modelRenderer()->setElementEdgeWidth(1.0f);
    pView->modelRenderer()->setNodeSize(5.0f);                           // default point size
    pView->modelRenderer()->setDefaultElementEdgeColor(QColor("white")); // dark gray/black lines

    pView->modelRenderer()->enablePerNodeColor(true);
    pView->modelRenderer()->enablePerElementColor(true);
    pView->modelRenderer()->setDefaultElementFaceColor(QColor(190, 190, 195)); // light gray faces
    pView->modelRenderer()->setDefaultNodeColor(QColor(240, 90, 60));

    pView->coordinateGizmo()->setSize(100);          // smaller/larger corner widget
    pView->coordinateGizmo()->setMargin(0);          // distance from bottom-left
    pView->coordinateGizmo()->setScale(0.5);         // arrow thickness/length within the viewport

    // pView->setControlMode(GLViewWindow::ControlMode::FPS);
    pView->camera()->setControlMode(ControlMode::OrbitPan);

    pView->camera()->setProjection(Projection::Orthographic); // or Orthographic
    pView->camera()->setFovDegrees(10.0f);                                 // perspective FOV
    pView->camera()->setOrthoHeight(10.0f);

    pView->camera()->setOrbitRotateSpeed(30.0f);
    pView->camera()->setOrbitPanSpeed(30.0f);

    pView->lighting()->enableAmbient(true);
    pView->lighting()->enableDiffuse(true);
    pView->lighting()->enableSpecular(true);

    pView->lighting()->setAmbientIntensity(0.25f);
    pView->lighting()->setDiffuseIntensity(1.0f);
    pView->lighting()->setSpecularIntensity(0.5f);
    pView->lighting()->setShininess(48.0f);

    // Optional: set light colors (defaults are white)
    pView->lighting()->setAmbientColor(QColor(255, 255, 255));
    pView->lighting()->setDiffuseColor(QColor(255, 255, 255));
    pView->lighting()->setSpecularColor(QColor(255, 255, 255));

    // pView->clearForces();

#if 0
    std::vector<ForceArrow> vecforces;
    vecforces.reserve(100);
    for (int i = 0; i < 100; ++i)
    {
        QColor defaultColor(255, 80, 40);
        ForceArrow force;
        force.id = i + 1;
        force.position = QVector3D(i * 0.05f, 0, 0);
        force.direction = QVector3D(0, 1, 0);
        force.size = 1.0f;
        force.lengthScale = 1.0f;
        force.color = QVector4D(defaultColor.redF(), defaultColor.greenF(), defaultColor.blueF(), defaultColor.alphaF());
        force.style = ForceStyle::Tail;
        force.lightingEnabled = (i % 2 == 0);

        vecforces.emplace_back(force);
        // pView->addForceArrow(QVector3D(i * 0.05f, 0, 0),        // position
        //                                 QVector3D(0, 1, 0),                // direction
        //                                 1.0f,                              // size (screen-scale)
        //                                 1.0f,                              // lengthScale (shaft only)
        //                                 QColor(255, 80, 40),               // color
        //                                 GLViewWindow::ForceStyle::Tail,    // style
        //                                 /*lightingEnabled=*/(i % 2 == 0)); // mix lit/unlit
    }
    pView->forceRenderer()->setForces(vecforces);
#endif

    // Choose mode
    pView->selectionManager()->setType(SelectType::Element);

    // Optional tuning
    pView->setPickRadius(8);
    pView->selectionManager()->setNodeHighlightColor(QColor("#ffd700"));
    // pView->setEdgeHighlightColor(Qt::red);
    pView->selectionManager()->setElemHighlightColor(QColor(64, 160, 255));
    pView->selectionManager()->setNodeScale(1.6f);
    // pView->setHighlightEdgeScale(1.5f);

    // Alpha for highlights (0..1)
    // pView->setNodeHighlightAlpha(0.9f);
    // pView->setEdgeHighlightAlpha(0.8f);
    pView->selectionManager()->setElemAlpha(0.4f); // softer face overlay

    // Marquee colors (RGBA via QColor)
    pView->setMarqueeFillColor(QColor(64,160,255,48));     // fill
    pView->setMarqueeStrokeColor(QColor(64,160,255,160));  // outline
    pView->setMarqueeStrokeWidth(1.5f);
    
    // // Retrieve selections after user clicks/box-selects
    // const auto &selNodes = pView->selectionManager()->selectedNodeIds();
    // const auto &selElems = pView->selectionManager()->selectedElementIds();
    // const auto &selForces = pView->selectionManager()->selectedForceIds();

    m_pViewWidget = pDoc->viewWidget();
    m_pViewWidget->setParent(ui->centralwidget); // QWidget parenting for ownership/layout
    m_pViewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto pLayout = qobject_cast<QBoxLayout*>(ui->centralwidget->layout());
    if (pLayout)
        pLayout->insertWidget(0, m_pViewWidget);

    // Ensure always draw
    m_pViewWidget->show();
}

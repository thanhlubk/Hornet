#include <HornetView/HRenderCoordinateGizmo.h>
#include <QtMath>

HRenderCoordinateGizmo::HRenderCoordinateGizmo(QObject *parent)
    : QObject(parent)
{
    m_vao = 0;
    m_vbo = 0;
    m_ebo = 0;
    m_iTriCount = 0;

    m_iSize = 96;
    m_iMargin = 12;
    m_fScale = 0.9f;
}

void HRenderCoordinateGizmo::initialize()
{
    initializeOpenGLFunctions();

    if (!m_shaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/vertex/coordinate"))
        qWarning("Failed compiling shader: %s", m_shaderProgram.log());

    if (!m_shaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/fragment/coordinate"))
        qWarning("Failed compiling shader: %s", m_shaderProgram.log());

    if (!m_shaderProgram.link())
        qWarning() << m_shaderProgram.log();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
    buildArrowCoordinate();
}

void HRenderCoordinateGizmo::destroy()
{
    if (m_ebo)
    {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    if (m_vbo)
    {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao)
    {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    m_shaderProgram.removeAllShaders();
}

void HRenderCoordinateGizmo::setSize(int pixel)
{
    m_iSize = std::max(32, pixel);
    emit dataChanged();
}
void HRenderCoordinateGizmo::setMargin(int pixel)
{
    m_iMargin = std::max(0, pixel);
    emit dataChanged();
}
void HRenderCoordinateGizmo::setScale(float scale)
{
    m_fScale = std::clamp(scale, 0.2f, 2.f);
    emit dataChanged();
}

void HRenderCoordinateGizmo::buildArrowCoordinate()
{
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    const int S = 24;
    const float shaftLen = 0.75f, shaftRad = 0.045f;
    const float coneLen = 0.25f, coneRad = 0.085f;
    const float z0 = 0.f, z1 = shaftLen, z2 = shaftLen + coneLen;
    const float TWO_PI = M_PI * 2;

    std::vector<QVector3D> V;
    std::vector<uint32_t> I;
    auto ring = [&](float z, float r)
    { for(int i=0;i<S;++i){ float a=TWO_PI*i/S; V.emplace_back(std::cos(a)*r,std::sin(a)*r,z);} };
    ring(z0, shaftRad);
    ring(z1, shaftRad);
    int base0 = 0, base1 = S;
    for (int i = 0; i < S; ++i)
    {
        int i0 = base0 + i, i1 = base0 + ((i + 1) % S);
        int j0 = base1 + i, j1 = base1 + ((i + 1) % S);
        I.insert(I.end(), {(uint32_t)i0, (uint32_t)i1, (uint32_t)j0, (uint32_t)i1, (uint32_t)j1, (uint32_t)j0});
    }
    int baseCone = (int)V.size();
    ring(z1, coneRad);
    int tip = (int)V.size();
    V.emplace_back(0, 0, z2);
    for (int i = 0; i < S; ++i)
    {
        int a = baseCone + i, b = baseCone + ((i + 1) % S);
        I.insert(I.end(), {(uint32_t)a, (uint32_t)b, (uint32_t)tip});
    }

    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(V.size() * sizeof(QVector3D)), V.data(), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(I.size() * sizeof(uint32_t)), I.data(), GL_STATIC_DRAW);
    m_iTriCount = (GLsizei)I.size();

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void *)0);
    glBindVertexArray(0);
}

void HRenderCoordinateGizmo::draw(const QQuaternion &cameraQ, int w, int h)
{
    // Save viewport/scissor
    GLint oldVP[4];
    glGetIntegerv(GL_VIEWPORT, oldVP);
    GLboolean scissorWas = glIsEnabled(GL_SCISSOR_TEST);

    const float fDpr = 1.0f; // widget dpr can be injected if needed
    const int sizePx = int(std::round(m_iSize * fDpr));
    const int margin = int(std::round(m_iMargin * fDpr));

    glViewport(margin, margin, sizePx, sizePx);
    glEnable(GL_SCISSOR_TEST);
    glScissor(margin, margin, sizePx, sizePx);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    QMatrix4x4 P;
    P.perspective(35.f, 1.f, 0.01f, 10.f);
    QMatrix4x4 V;
    V.translate(0, 0, -2.2f);

    auto drawAxis = [&](const QVector3D &color, const QQuaternion &axisRot)
    {
        QMatrix4x4 M;
        // The gizmo should show world axes from the camera's current view.
        // Use the inverse camera orientation; using cameraQ directly makes the mini-axis
        // appear mirrored/backward relative to the main view.
        M.rotate(cameraQ.conjugated());
        M.rotate(axisRot);
        M.scale(m_fScale);
        QMatrix4x4 MVP = P * V * M;
        m_shaderProgram.bind();
        m_shaderProgram.setUniformValue("uMVP", MVP);
        m_shaderProgram.setUniformValue("uColor", color);
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, m_iTriCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        m_shaderProgram.release();
    };
    drawAxis(QVector3D(1.0f, 0.25f, 0.25f), QQuaternion::fromAxisAndAngle(0, 1, 0, 90)); // X
    drawAxis(QVector3D(0.25f, 1.0f, 0.25f), QQuaternion::fromAxisAndAngle(1, 0, 0, -90));  // Y
    drawAxis(QVector3D(0.25f, 0.5f, 1.0f), QQuaternion::fromAxisAndAngle(0, 0, 1, 90)); // Z

    if (!scissorWas)
        glDisable(GL_SCISSOR_TEST);
    glViewport(oldVP[0], oldVP[1], oldVP[2], oldVP[3]);
    (void)w;
    (void)h;
}

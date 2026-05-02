#include <HornetView/HRenderCustomDraw.h>
#include <QDebug>
#include <algorithm>

HRenderCustomDraw::HRenderCustomDraw(QObject *parent)
    : QObject(parent)
{
    m_pointVao = 0;
    m_pointVboPosition = 0;
    m_pointVboColor = 0;

    m_lineVao = 0;
    m_lineVboPosition = 0;
    m_lineVboColor = 0;
    m_lineEbo = 0;

    m_iPointCount = 0;
    m_iLineVertexCount = 0;
    m_iLineIndexCount = 0;

    m_bInitialized = false;
    m_bDirtyPoints = true;
    m_bDirtyLines = true;
    m_bShowPoints = true;
    m_bShowLines = true;

    m_fPointSize = 6.0f;
    m_fLineWidth = 1.0f;
    m_vDefaultPointColor = QVector4D(0.95f, 0.35f, 0.25f, 1.0f);
    m_vDefaultLineColor = QVector4D(0.10f, 0.10f, 0.10f, 1.0f);
}

void HRenderCustomDraw::initialize()
{
    initializeOpenGLFunctions();

    m_pointShaderProgram.removeAllShaders();
    if (!m_pointShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/vertex/node"))
        qWarning("Failed compiling custom point vertex shader: %s", qPrintable(m_pointShaderProgram.log()));
    if (!m_pointShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/fragment/node"))
        qWarning("Failed compiling custom point fragment shader: %s", qPrintable(m_pointShaderProgram.log()));
    if (!m_pointShaderProgram.link())
        qWarning() << m_pointShaderProgram.log();

    m_lineShaderProgram.removeAllShaders();
    if (!m_lineShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/vertex/element"))
        qWarning("Failed compiling custom line vertex shader: %s", qPrintable(m_lineShaderProgram.log()));
    if (!m_lineShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/fragment/element"))
        qWarning("Failed compiling custom line fragment shader: %s", qPrintable(m_lineShaderProgram.log()));
    if (!m_lineShaderProgram.link())
        qWarning() << m_lineShaderProgram.log();

    glGenVertexArrays(1, &m_pointVao);
    glGenBuffers(1, &m_pointVboPosition);
    glGenBuffers(1, &m_pointVboColor);

    glGenVertexArrays(1, &m_lineVao);
    glGenBuffers(1, &m_lineVboPosition);
    glGenBuffers(1, &m_lineVboColor);
    glGenBuffers(1, &m_lineEbo);

    m_bInitialized = true;
    m_bDirtyPoints = true;
    m_bDirtyLines = true;

    uploadPoints();
    uploadLines();
}

void HRenderCustomDraw::destroy()
{
    if (m_lineEbo)
    {
        glDeleteBuffers(1, &m_lineEbo);
        m_lineEbo = 0;
    }
    if (m_lineVboColor)
    {
        glDeleteBuffers(1, &m_lineVboColor);
        m_lineVboColor = 0;
    }
    if (m_lineVboPosition)
    {
        glDeleteBuffers(1, &m_lineVboPosition);
        m_lineVboPosition = 0;
    }
    if (m_lineVao)
    {
        glDeleteVertexArrays(1, &m_lineVao);
        m_lineVao = 0;
    }

    if (m_pointVboColor)
    {
        glDeleteBuffers(1, &m_pointVboColor);
        m_pointVboColor = 0;
    }
    if (m_pointVboPosition)
    {
        glDeleteBuffers(1, &m_pointVboPosition);
        m_pointVboPosition = 0;
    }
    if (m_pointVao)
    {
        glDeleteVertexArrays(1, &m_pointVao);
        m_pointVao = 0;
    }

    m_pointShaderProgram.removeAllShaders();
    m_lineShaderProgram.removeAllShaders();

    m_iPointCount = 0;
    m_iLineVertexCount = 0;
    m_iLineIndexCount = 0;
    m_bInitialized = false;
    m_bDirtyPoints = true;
    m_bDirtyLines = true;
}

void HRenderCustomDraw::draw(const QMatrix4x4 &P, const QMatrix4x4 &V)
{
    if (!m_bInitialized)
        return;

    if (m_bDirtyPoints)
        uploadPoints();
    if (m_bDirtyLines)
        uploadLines();

    if (m_bShowLines)
        drawLines(P, V);
    if (m_bShowPoints)
        drawPoints(P, V);
}

void HRenderCustomDraw::drawCustomPoint3D(const QVector3D &position)
{
    drawCustomPoint3D(position, defaultPointColor());
}

void HRenderCustomDraw::drawCustomPoint3D(const QVector3D &position, const QColor &color)
{
    m_vecPointPositions.push_back(position);
    m_vecPointColors.push_back(toVec4(color));
    m_iPointCount = GLsizei(m_vecPointPositions.size());
    m_bDirtyPoints = true;
    emit dataChanged();
}

void HRenderCustomDraw::drawCustomLine3D(const QVector3D &p0, const QVector3D &p1)
{
    drawCustomLine3D(p0, p1, defaultLineColor());
}

void HRenderCustomDraw::drawCustomLine3D(const QVector3D &p0, const QVector3D &p1, const QColor &color)
{
    const uint32_t base = static_cast<uint32_t>(m_vecLinePositions.size());
    const QVector4D c = toVec4(color);

    m_vecLinePositions.push_back(p0);
    m_vecLinePositions.push_back(p1);
    m_vecLineColors.push_back(c);
    m_vecLineColors.push_back(c);
    m_vecLineIndices.push_back(base);
    m_vecLineIndices.push_back(base + 1u);

    m_iLineVertexCount = GLsizei(m_vecLinePositions.size());
    m_iLineIndexCount = GLsizei(m_vecLineIndices.size());
    m_bDirtyLines = true;
    emit dataChanged();
}

void HRenderCustomDraw::clear()
{
    clearCustomPoints();
    clearCustomLines();
}

void HRenderCustomDraw::clearCustomPoints()
{
    m_vecPointPositions.clear();
    m_vecPointColors.clear();
    m_iPointCount = 0;
    m_bDirtyPoints = true;
    emit dataChanged();
}

void HRenderCustomDraw::clearCustomLines()
{
    m_vecLinePositions.clear();
    m_vecLineColors.clear();
    m_vecLineIndices.clear();
    m_iLineVertexCount = 0;
    m_iLineIndexCount = 0;
    m_bDirtyLines = true;
    emit dataChanged();
}

void HRenderCustomDraw::setPointSize(float pixel)
{
    const float clamped = std::max(1.0f, pixel);
    if (m_fPointSize == clamped)
        return;

    m_fPointSize = clamped;
    emit settingChanged();
}

float HRenderCustomDraw::pointSize() const
{
    return m_fPointSize;
}

void HRenderCustomDraw::setLineWidth(float pixel)
{
    const float clamped = std::max(1.0f, pixel);
    if (m_fLineWidth == clamped)
        return;

    m_fLineWidth = clamped;
    emit settingChanged();
}

float HRenderCustomDraw::lineWidth() const
{
    return m_fLineWidth;
}

void HRenderCustomDraw::setDefaultPointColor(const QColor &color)
{
    m_vDefaultPointColor = toVec4(color);
    emit settingChanged();
}

QColor HRenderCustomDraw::defaultPointColor() const
{
    return QColor::fromRgbF(m_vDefaultPointColor.x(),
                            m_vDefaultPointColor.y(),
                            m_vDefaultPointColor.z(),
                            m_vDefaultPointColor.w());
}

void HRenderCustomDraw::setDefaultLineColor(const QColor &color)
{
    m_vDefaultLineColor = toVec4(color);
    emit settingChanged();
}

QColor HRenderCustomDraw::defaultLineColor() const
{
    return QColor::fromRgbF(m_vDefaultLineColor.x(),
                            m_vDefaultLineColor.y(),
                            m_vDefaultLineColor.z(),
                            m_vDefaultLineColor.w());
}

void HRenderCustomDraw::setShowPoints(bool show)
{
    if (m_bShowPoints == show)
        return;

    m_bShowPoints = show;
    emit settingChanged();
}

bool HRenderCustomDraw::showPoints() const
{
    return m_bShowPoints;
}

void HRenderCustomDraw::setShowLines(bool show)
{
    if (m_bShowLines == show)
        return;

    m_bShowLines = show;
    emit settingChanged();
}

bool HRenderCustomDraw::showLines() const
{
    return m_bShowLines;
}

int HRenderCustomDraw::pointCount() const
{
    return static_cast<int>(m_vecPointPositions.size());
}

int HRenderCustomDraw::lineCount() const
{
    return static_cast<int>(m_vecLineIndices.size() / 2u);
}

QVector4D HRenderCustomDraw::toVec4(const QColor &color)
{
    return QVector4D(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

QVector3D HRenderCustomDraw::toVec3(const QVector4D &color)
{
    return QVector3D(color.x(), color.y(), color.z());
}

void HRenderCustomDraw::uploadPoints()
{
    if (!m_bInitialized)
        return;

    glBindVertexArray(m_pointVao);

    glBindBuffer(GL_ARRAY_BUFFER, m_pointVboPosition);
    glBufferData(GL_ARRAY_BUFFER,
                 GLsizeiptr(m_vecPointPositions.size() * sizeof(QVector3D)),
                 m_vecPointPositions.data(),
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), reinterpret_cast<void *>(0));

    glBindBuffer(GL_ARRAY_BUFFER, m_pointVboColor);
    glBufferData(GL_ARRAY_BUFFER,
                 GLsizeiptr(m_vecPointColors.size() * sizeof(QVector4D)),
                 m_vecPointColors.data(),
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(QVector4D), reinterpret_cast<void *>(0));

    glBindVertexArray(0);

    m_iPointCount = GLsizei(m_vecPointPositions.size());
    m_bDirtyPoints = false;
}

void HRenderCustomDraw::uploadLines()
{
    if (!m_bInitialized)
        return;

    glBindVertexArray(m_lineVao);

    glBindBuffer(GL_ARRAY_BUFFER, m_lineVboPosition);
    glBufferData(GL_ARRAY_BUFFER,
                 GLsizeiptr(m_vecLinePositions.size() * sizeof(QVector3D)),
                 m_vecLinePositions.data(),
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), reinterpret_cast<void *>(0));

    glBindBuffer(GL_ARRAY_BUFFER, m_lineVboColor);
    glBufferData(GL_ARRAY_BUFFER,
                 GLsizeiptr(m_vecLineColors.size() * sizeof(QVector4D)),
                 m_vecLineColors.data(),
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(QVector4D), reinterpret_cast<void *>(0));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_lineEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 GLsizeiptr(m_vecLineIndices.size() * sizeof(uint32_t)),
                 m_vecLineIndices.data(),
                 GL_DYNAMIC_DRAW);

    glBindVertexArray(0);

    m_iLineVertexCount = GLsizei(m_vecLinePositions.size());
    m_iLineIndexCount = GLsizei(m_vecLineIndices.size());
    m_bDirtyLines = false;
}

void HRenderCustomDraw::drawPoints(const QMatrix4x4 &P, const QMatrix4x4 &V)
{
    if (m_iPointCount == 0)
        return;

    const QMatrix4x4 MVP = P * V;

    glEnable(GL_DEPTH_TEST);

    GLint oldDepthFunc = GL_LESS;
    glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
    glDepthFunc(GL_LEQUAL);

    const GLboolean hadBlend = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_POLYGON_OFFSET_POINT);
    glPolygonOffset(-1.0f, -1.0f);

    m_pointShaderProgram.bind();
    m_pointShaderProgram.setUniformValue("uMVP", MVP);
    m_pointShaderProgram.setUniformValue("uPointSize", m_fPointSize);
    m_pointShaderProgram.setUniformValue("uUseVertexColor", 1);
    m_pointShaderProgram.setUniformValue("uColor", toVec3(m_vDefaultPointColor));

    glBindVertexArray(m_pointVao);
    glDrawArrays(GL_POINTS, 0, m_iPointCount);
    glBindVertexArray(0);

    m_pointShaderProgram.release();

    glDisable(GL_POLYGON_OFFSET_POINT);
    if (!hadBlend)
        glDisable(GL_BLEND);
    glDepthFunc(oldDepthFunc);
}

void HRenderCustomDraw::drawLines(const QMatrix4x4 &P, const QMatrix4x4 &V)
{
    if (m_iLineVertexCount == 0 || m_iLineIndexCount == 0)
        return;

    const QMatrix4x4 MVP = P * V;

    glEnable(GL_DEPTH_TEST);

    GLboolean oldDepthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &oldDepthMask);
    glDepthMask(GL_FALSE);

    GLint oldDepthFunc = GL_LESS;
    glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
    glDepthFunc(GL_LEQUAL);

    GLfloat oldLineWidth = 1.0f;
    glGetFloatv(GL_LINE_WIDTH, &oldLineWidth);

    const GLboolean hadBlend = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const GLboolean hadLineSmooth = glIsEnabled(GL_LINE_SMOOTH);
    glEnable(GL_LINE_SMOOTH);

    m_lineShaderProgram.bind();
    m_lineShaderProgram.setUniformValue("uMVP", MVP);
    m_lineShaderProgram.setUniformValue("uView", V);
    m_lineShaderProgram.setUniformValue("uLit", 0);
    m_lineShaderProgram.setUniformValue("uUseVertexColor", 1);
    m_lineShaderProgram.setUniformValue("uColor", toVec3(m_vDefaultLineColor));

    glBindVertexArray(m_lineVao);
    glLineWidth(m_fLineWidth);
    glDrawElements(GL_LINES, m_iLineIndexCount, GL_UNSIGNED_INT, reinterpret_cast<void *>(0));
    glBindVertexArray(0);

    m_lineShaderProgram.release();

    if (!hadLineSmooth)
        glDisable(GL_LINE_SMOOTH);
    if (!hadBlend)
        glDisable(GL_BLEND);
    glLineWidth(oldLineWidth);
    glDepthMask(oldDepthMask);
    glDepthFunc(oldDepthFunc);
}

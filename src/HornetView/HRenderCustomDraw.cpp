#include <HornetView/HRenderCustomDraw.h>
#include <QDebug>
#include <algorithm>

HRenderCustomDraw::HRenderCustomDraw(QObject *parent)
    : QObject(parent)
{
    m_pointVao = 0;
    m_pointVboPosition = 0;
    m_pointVboColor = 0;
    m_pointVboSize = 0;

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

    m_iNextPrimitiveId = 1;

    m_fPointSize = 6.0f;
    m_fLineWidth = 1.0f;
    m_vDefaultPointColor = QVector4D(0.95f, 0.35f, 0.25f, 1.0f);
    m_vDefaultLineColor = QVector4D(0.10f, 0.10f, 0.10f, 1.0f);
}

void HRenderCustomDraw::initialize()
{
    initializeOpenGLFunctions();

    m_pointShaderProgram.removeAllShaders();
    if (!m_pointShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/vertex/customdraw"))
        qWarning("Failed compiling custom draw point vertex shader: %s", qPrintable(m_pointShaderProgram.log()));
    if (!m_pointShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/fragment/customdraw"))
        qWarning("Failed compiling custom draw point fragment shader: %s", qPrintable(m_pointShaderProgram.log()));
    if (!m_pointShaderProgram.link())
        qWarning() << m_pointShaderProgram.log();

    m_lineShaderProgram.removeAllShaders();
    if (!m_lineShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/vertex/customdraw"))
        qWarning("Failed compiling custom draw line vertex shader: %s", qPrintable(m_lineShaderProgram.log()));
    if (!m_lineShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/fragment/customdraw"))
        qWarning("Failed compiling custom draw line fragment shader: %s", qPrintable(m_lineShaderProgram.log()));
    if (!m_lineShaderProgram.link())
        qWarning() << m_lineShaderProgram.log();

    glGenVertexArrays(1, &m_pointVao);
    glGenBuffers(1, &m_pointVboPosition);
    glGenBuffers(1, &m_pointVboColor);
    glGenBuffers(1, &m_pointVboSize);

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

    if (m_pointVboSize)
    {
        glDeleteBuffers(1, &m_pointVboSize);
        m_pointVboSize = 0;
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

int HRenderCustomDraw::drawCustomPoint3D(const QVector3D &position)
{
    return drawCustomPoint3D(position, defaultPointColor(), m_fPointSize);
}

int HRenderCustomDraw::drawCustomPoint3D(const QVector3D &position, const QColor &color)
{
    return drawCustomPoint3D(position, color, m_fPointSize);
}

int HRenderCustomDraw::drawCustomPoint3D(const QVector3D &position, const QColor &color, float pointSize)
{
    const int id = nextPrimitiveId();

    CustomPoint p;
    p.id = id;
    p.position = position;
    p.color = toVec4(color);
    p.size = clampPixel(pointSize);

    m_vecPoints.push_back(p);
    m_iPointCount = GLsizei(m_vecPoints.size());
    m_bDirtyPoints = true;
    emit dataChanged();
    return id;
}

int HRenderCustomDraw::drawCustomLine3D(const QVector3D &p0, const QVector3D &p1)
{
    return drawCustomLine3D(p0, p1, defaultLineColor(), m_fLineWidth);
}

int HRenderCustomDraw::drawCustomLine3D(const QVector3D &p0, const QVector3D &p1, const QColor &color)
{
    return drawCustomLine3D(p0, p1, color, m_fLineWidth);
}

int HRenderCustomDraw::drawCustomLine3D(const QVector3D &p0, const QVector3D &p1, const QColor &color, float lineWidth)
{
    const int id = nextPrimitiveId();

    CustomLine l;
    l.id = id;
    l.p0 = p0;
    l.p1 = p1;
    l.color = toVec4(color);
    l.width = clampPixel(lineWidth);

    m_vecLines.push_back(l);
    m_iLineVertexCount = GLsizei(m_vecLines.size() * 2u);
    m_iLineIndexCount = GLsizei(m_vecLines.size() * 2u);
    m_bDirtyLines = true;
    emit dataChanged();
    return id;
}

void HRenderCustomDraw::clear()
{
    const bool hadAny = !m_vecPoints.empty() || !m_vecLines.empty();
    m_vecPoints.clear();
    m_vecLines.clear();

    m_iPointCount = 0;
    m_iLineVertexCount = 0;
    m_iLineIndexCount = 0;
    m_bDirtyPoints = true;
    m_bDirtyLines = true;

    if (hadAny)
        emit dataChanged();
}

void HRenderCustomDraw::clearCustomPoints()
{
    if (m_vecPoints.empty())
        return;

    m_vecPoints.clear();
    m_iPointCount = 0;
    m_bDirtyPoints = true;
    emit dataChanged();
}

void HRenderCustomDraw::clearCustomLines()
{
    if (m_vecLines.empty())
        return;

    m_vecLines.clear();
    m_iLineVertexCount = 0;
    m_iLineIndexCount = 0;
    m_bDirtyLines = true;
    emit dataChanged();
}

bool HRenderCustomDraw::clearCustomPoint(int id)
{
    const auto oldSize = m_vecPoints.size();
    m_vecPoints.erase(std::remove_if(m_vecPoints.begin(), m_vecPoints.end(),
                                     [id](const CustomPoint &p) { return p.id == id; }),
                      m_vecPoints.end());

    if (m_vecPoints.size() == oldSize)
        return false;

    m_iPointCount = GLsizei(m_vecPoints.size());
    m_bDirtyPoints = true;
    emit dataChanged();
    return true;
}

bool HRenderCustomDraw::clearCustomLine(int id)
{
    const auto oldSize = m_vecLines.size();
    m_vecLines.erase(std::remove_if(m_vecLines.begin(), m_vecLines.end(),
                                    [id](const CustomLine &l) { return l.id == id; }),
                     m_vecLines.end());

    if (m_vecLines.size() == oldSize)
        return false;

    m_iLineVertexCount = GLsizei(m_vecLines.size() * 2u);
    m_iLineIndexCount = GLsizei(m_vecLines.size() * 2u);
    m_bDirtyLines = true;
    emit dataChanged();
    return true;
}

bool HRenderCustomDraw::clearCustomPrimitive(int id)
{
    const bool removedPoint = clearCustomPoint(id);
    const bool removedLine = clearCustomLine(id);
    return removedPoint || removedLine;
}

void HRenderCustomDraw::setPointSize(float pixel)
{
    const float clamped = clampPixel(pixel);
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
    const float clamped = clampPixel(pixel);
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
    return static_cast<int>(m_vecPoints.size());
}

int HRenderCustomDraw::lineCount() const
{
    return static_cast<int>(m_vecLines.size());
}

QVector4D HRenderCustomDraw::toVec4(const QColor &color)
{
    return QVector4D(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

QVector3D HRenderCustomDraw::toVec3(const QVector4D &color)
{
    return QVector3D(color.x(), color.y(), color.z());
}

float HRenderCustomDraw::clampPixel(float pixel)
{
    return std::max(1.0f, pixel);
}

int HRenderCustomDraw::nextPrimitiveId()
{
    return m_iNextPrimitiveId++;
}

void HRenderCustomDraw::uploadPoints()
{
    if (!m_bInitialized)
        return;

    m_vecPointPositions.clear();
    m_vecPointColors.clear();
    m_vecPointSizes.clear();

    m_vecPointPositions.reserve(m_vecPoints.size());
    m_vecPointColors.reserve(m_vecPoints.size());
    m_vecPointSizes.reserve(m_vecPoints.size());

    for (const CustomPoint &p : m_vecPoints)
    {
        m_vecPointPositions.push_back(p.position);
        m_vecPointColors.push_back(p.color);
        m_vecPointSizes.push_back(p.size);
    }

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

    glBindBuffer(GL_ARRAY_BUFFER, m_pointVboSize);
    glBufferData(GL_ARRAY_BUFFER,
                 GLsizeiptr(m_vecPointSizes.size() * sizeof(float)),
                 m_vecPointSizes.data(),
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), reinterpret_cast<void *>(0));

    glBindVertexArray(0);

    m_iPointCount = GLsizei(m_vecPointPositions.size());
    m_bDirtyPoints = false;
}

void HRenderCustomDraw::uploadLines()
{
    if (!m_bInitialized)
        return;

    m_vecLinePositions.clear();
    m_vecLineColors.clear();
    m_vecLineIndices.clear();
    m_vecLineWidths.clear();

    m_vecLinePositions.reserve(m_vecLines.size() * 2u);
    m_vecLineColors.reserve(m_vecLines.size() * 2u);
    m_vecLineIndices.reserve(m_vecLines.size() * 2u);
    m_vecLineWidths.reserve(m_vecLines.size());

    for (const CustomLine &line : m_vecLines)
    {
        const uint32_t base = static_cast<uint32_t>(m_vecLinePositions.size());

        m_vecLinePositions.push_back(line.p0);
        m_vecLinePositions.push_back(line.p1);
        m_vecLineColors.push_back(line.color);
        m_vecLineColors.push_back(line.color);
        m_vecLineIndices.push_back(base);
        m_vecLineIndices.push_back(base + 1u);
        m_vecLineWidths.push_back(line.width);
    }

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

    // Lines do not need the point-size attribute. Set a safe constant value for attribute 2.
    glDisableVertexAttribArray(2);
    glVertexAttrib1f(2, 1.0f);

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
    m_pointShaderProgram.setUniformValue("uUseVertexSize", 1);
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
    m_lineShaderProgram.setUniformValue("uPointSize", 1.0f);
    m_lineShaderProgram.setUniformValue("uUseVertexSize", 0);
    m_lineShaderProgram.setUniformValue("uUseVertexColor", 1);
    m_lineShaderProgram.setUniformValue("uColor", toVec3(m_vDefaultLineColor));

    glBindVertexArray(m_lineVao);

    for (int i = 0; i < static_cast<int>(m_vecLineWidths.size()); ++i)
    {
        glLineWidth(m_vecLineWidths[static_cast<size_t>(i)]);
        const void *off = reinterpret_cast<const void *>(sizeof(uint32_t) * 2u * static_cast<uint32_t>(i));
        glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, off);
    }

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

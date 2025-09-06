#include <HornetView/HRenderNode.h>
#include <QFile>

HRenderNode::HRenderNode()
{
    m_vao = 0;
    m_vboPosition = 0;
    m_vboColor = 0;
    m_iNumNode = 0;
    m_bInitialize = false;

    m_fNodeSize = 6.f;
    m_bEnablePerNodeColor = true;
    m_vDefaultColor = QVector3D(0.95f, 0.35f, 0.25f);

    m_mapIdIndex.clear();
    m_eboSelection = 0;
    m_iSelectionCount = 0;
    m_vSelectionColor = QVector3D(1.f, 0.84f, 0.f);
    m_fSelectionSizeScale = 1.5f;
    m_bIsSelected = false;
}

void HRenderNode::initialize()
{
    initializeOpenGLFunctions();

    if (!m_shaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/vertex/node"))
        qWarning("Failed compiling shader: %s", m_shaderProgram.log());

    if (!m_shaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/fragment/node"))
        qWarning("Failed compiling shader: %s", m_shaderProgram.log());

    if (!m_shaderProgram.link())
        qWarning() << m_shaderProgram.log();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vboPosition);
    glGenBuffers(1, &m_vboColor);

    m_bInitialize = true;

    upload({}, {}, {});
}

void HRenderNode::destroy()
{
    if (m_vboColor)
    {
        glDeleteBuffers(1, &m_vboColor);
        m_vboColor = 0;
    }
    if (m_vboPosition)
    {
        glDeleteBuffers(1, &m_vboPosition);
        m_vboPosition = 0;
    }
    if (m_vao)
    {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    m_shaderProgram.removeAllShaders();
}

void HRenderNode::setNodes(const std::vector<Node>& nodes)
{
    // Pack positions, colors, ids then reuse existing upload()
    std::vector<QVector3D> pos; 
    std::vector<QVector4D> col; 
    std::vector<int> ids;

    pos.reserve(nodes.size());
    col.reserve(nodes.size());
    ids.reserve(nodes.size());
    for (const auto& n : nodes)
    {
        pos.emplace_back(n.x, n.y, n.z);
        col.emplace_back(n.r, n.g, n.b, n.a);
        ids.emplace_back(n.id);
    }

    if (m_bInitialize)
        upload(pos, col, ids);
}

void HRenderNode::upload(const std::vector<QVector3D> &pos, const std::vector<QVector4D> &col, const std::vector<int> &nodeIds)
{
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboPosition);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(pos.size() * sizeof(QVector3D)), pos.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void *)0);
    m_iNumNode = GLsizei(pos.size());

    glBindBuffer(GL_ARRAY_BUFFER, m_vboColor);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(col.size() * sizeof(QVector4D)), col.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(QVector4D), (void *)0);
    glBindVertexArray(0);

    // Build ID -> index map
    m_mapIdIndex.clear();
    m_mapIdIndex.reserve(nodeIds.size());
    for (int i = 0; i < (int)nodeIds.size(); ++i)
        m_mapIdIndex[nodeIds[i]] = i;

    // (re)create selection EBO
    if (!m_eboSelection)
        glGenBuffers(1, &m_eboSelection);
    m_iSelectionCount = 0;
    m_bIsSelected = false;
}

void HRenderNode::draw(const QMatrix4x4 &P, const QMatrix4x4 &V)
{
    if (m_iNumNode == 0)
        return;

    const QMatrix4x4 MVP = P * V;

    glEnable(GL_DEPTH_TEST);
    // --- ensure nodes "win" over edges at equal depth ---
    GLint _oldDepthFunc = GL_LESS;
    glGetIntegerv(GL_DEPTH_FUNC, &_oldDepthFunc);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_POLYGON_OFFSET_POINT);
    glPolygonOffset(-1.0f, -1.0f);

    m_shaderProgram.bind();
    m_shaderProgram.setUniformValue("uMVP", MVP);
    m_shaderProgram.setUniformValue("uPointSize", m_fNodeSize);
    m_shaderProgram.setUniformValue("uUseVertexColor", m_bEnablePerNodeColor ? 1 : 0);
    m_shaderProgram.setUniformValue("uColor", m_vDefaultColor);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_POINTS, 0, m_iNumNode);
    glBindVertexArray(0);
    m_shaderProgram.release();

    // highlight overlay (second pass)
    if (m_bIsSelected && m_iSelectionCount > 0)
    {
        m_shaderProgram.bind();
        QMatrix4x4 MVP = P * V;
        m_shaderProgram.setUniformValue("uMVP", MVP);
        m_shaderProgram.setUniformValue("uView", V);
        m_shaderProgram.setUniformValue("uColor", m_vSelectionColor);
        m_shaderProgram.setUniformValue("uLit", 0);
        m_shaderProgram.setUniformValue("uUseVertexColor", 0);
        m_shaderProgram.setUniformValue("uPointSize", m_fNodeSize * m_fSelectionSizeScale);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboSelection);
        glDrawElements(GL_POINTS, m_iSelectionCount, GL_UNSIGNED_INT, (void *)0);

        glBindVertexArray(0);
        m_shaderProgram.release();
    }
}

void HRenderNode::setNodeSize(float pixel)
{
    m_fNodeSize = std::max(1.f, pixel);
}

float HRenderNode::nodeSize() const 
{
    return m_fNodeSize; 
}

void HRenderNode::enablePerNodeColor(bool enable)
{
    m_bEnablePerNodeColor = enable;
}

void HRenderNode::setDefaultColor(const QColor &color)
{ 
    m_vDefaultColor = QVector3D(color.redF(), color.greenF(), color.blueF()); 
}

QColor HRenderNode::defaultColor() const
{ 
    return QColor::fromRgbF(m_vDefaultColor.x(), m_vDefaultColor.y(), m_vDefaultColor.z()); 
}

void HRenderNode::setSelection(const std::vector<int> &nodeIds, const QColor &c, float sScale)
{
    // Map IDs → indices
    std::vector<uint32_t> idx;
    idx.reserve(nodeIds.size());
    for (int id : nodeIds)
    {
        auto it = m_mapIdIndex.find(id);
        if (it != m_mapIdIndex.end())
            idx.push_back(uint32_t(it->second));
    }
    m_iSelectionCount = GLsizei(idx.size());
    m_vSelectionColor = QVector3D(c.redF(), c.greenF(), c.blueF());
    m_fSelectionSizeScale = std::max(0.1f, sScale);
    m_bIsSelected = (m_iSelectionCount > 0);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboSelection);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(idx.size() * sizeof(uint32_t)), idx.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
}

void HRenderNode::clearSelection()
{
    m_iSelectionCount = 0;
    m_bIsSelected = false;
}

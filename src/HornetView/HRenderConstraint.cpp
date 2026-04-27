#include <HornetView/HRenderConstraint.h>
#include <HornetView/HViewLighting.h>
#include <HornetView/HViewCamera.h>
#include <cmath>

static inline void addTri(std::vector<uint32_t> &idx, uint32_t a, uint32_t b, uint32_t c)
{
    idx.push_back(a);
    idx.push_back(b);
    idx.push_back(c);
}

HRenderConstraint::HRenderConstraint(QObject *parent) 
    : QObject(parent)
{
    m_vao = 0;
    m_vboPosition = 0;
    m_vboNormal = 0;
    m_vboColor = 0;
    m_ebo = 0;
    m_bInitialize = false;

    m_vecConstraints.clear();
    m_bConstantScreenSize = true;
    m_bRebuild = true;
    m_fSize = 80.0f;

    m_vSelectionColor = QVector3D({1.f, 0.5f, 0.f});
    m_fSelectionAlpha = 0.45f;
    m_bIsSelected = false;

    m_mapRangesLighting.clear();
    m_mapRangesUnlighting.clear();
    m_iLightingCount = 0;
    m_iUnlightingCount = 0;
}

void HRenderConstraint::initialize()
{
    initializeOpenGLFunctions();
    // We can reuse the force shaders since they just do basic lit/unlit vertex coloring
    if (!m_shaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/vertex/force"))
        qWarning("Failed compiling shader: %s", m_shaderProgram.log());

    if (!m_shaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/fragment/force"))
        qWarning("Failed compiling shader: %s", m_shaderProgram.log());

    if (!m_shaderProgram.link())
        qWarning() << m_shaderProgram.log();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vboPosition);
    glGenBuffers(1, &m_vboNormal);
    glGenBuffers(1, &m_vboColor);
    glGenBuffers(1, &m_ebo);
    m_bInitialize = true;
}

void HRenderConstraint::destroy()
{
    if (!m_bInitialize)
        return;
    if (m_ebo)
        glDeleteBuffers(1, &m_ebo);
    if (m_vboColor)
        glDeleteBuffers(1, &m_vboColor);
    if (m_vboNormal)
        glDeleteBuffers(1, &m_vboNormal);
    if (m_vboPosition)
        glDeleteBuffers(1, &m_vboPosition);
    if (m_vao)
        glDeleteVertexArrays(1, &m_vao);
    m_ebo = m_vboColor = m_vboNormal = m_vboPosition = m_vao = 0;
    m_shaderProgram.removeAllShaders();
    m_bInitialize = false;
}

void HRenderConstraint::setConstraints(const std::vector<RenderConstraintData> &constraints)
{
    m_vecConstraints = constraints;
    m_bRebuild = true;
    emit dataChanged();
}

void HRenderConstraint::setConstantScreenSize(bool on)
{
    m_bConstantScreenSize = on;
    m_bRebuild = true;
    emit dataChanged();
}

void HRenderConstraint::setBasePixelSize(float px)
{
    m_fSize = std::max(1.0f, px);
    m_bRebuild = true;
    emit dataChanged();
}

bool HRenderConstraint::constantScreenSize() const 
{ 
    return m_bConstantScreenSize; 
}

float HRenderConstraint::basePixelSize() const 
{ 
    return m_fSize; 
}

void HRenderConstraint::ensureGL()
{
    if (!m_bInitialize)
        initialize();
}

float HRenderConstraint::worldPerPixelAt(const QMatrix4x4 &VP, const QMatrix4x4 &invVP, int viewportH, const QVector3D &world) const
{
    QVector4D clip = VP * QVector4D(world, 1.0f);
    if (std::fabs(clip.w()) < 1e-12f)
        return 0.0f;
    float ndcX = clip.x() / clip.w();
    float ndcY = clip.y() / clip.w();
    float ndcZ = clip.z() / clip.w();
    const float dNdcY = 2.0f / std::max(1, viewportH);
    auto unproj = [&](float x, float y, float z) -> QVector3D
    {
        QVector4D p = invVP * QVector4D(x, y, z, 1.0f);
        if (std::fabs(p.w()) < 1e-12f)
            return QVector3D();
        return (p / p.w()).toVector3D();
    };
    QVector3D w0 = unproj(ndcX, ndcY, ndcZ);
    QVector3D w1 = unproj(ndcX, ndcY + dNdcY, ndcZ);
    return (w1 - w0).length();
}

void HRenderConstraint::rebuild(const QMatrix4x4 &P, const QMatrix4x4 &V, int w, int h)
{
    const int segs = 20;
    const float headR = 0.08f;
    const float headL = 0.25f;

    const QMatrix4x4 VP = P * V;
    QMatrix4x4 invVP = VP.inverted();

    std::vector<QVector3D> m_pos, m_nrm;
    std::vector<QVector4D> m_col;
    std::vector<uint32_t> m_idx;

    m_pos.clear();
    m_nrm.clear();
    m_col.clear();
    m_idx.clear();
    std::vector<uint32_t> idxLit, idxUnlit;
    m_mapRangesLighting.clear();
    m_mapRangesUnlighting.clear();

    auto buildOne = [&](const RenderConstraintData& data, std::vector<uint32_t>& outIdx, bool isLit)
    {
        const ConstraintCone& A = data.style;
        QVector3D z = data.direction.lengthSquared() > 1e-12f ? data.direction.normalized() : QVector3D(0, 0, 1);
        QVector3D up0(0, 1, 0);
        if (std::fabs(QVector3D::dotProduct(z, up0)) > 0.99f)
            up0 = QVector3D(1, 0, 0);
        QVector3D x = QVector3D::crossProduct(up0, z).normalized();
        QVector3D y = QVector3D::crossProduct(z, x);

        const float px = m_fSize * std::max(0.0001f, A.size);
        const float wpp = std::max(1e-9f, worldPerPixelAt(VP, invVP, h, data.position));
        const float totalLen = (m_bConstantScreenSize ? px * wpp : px); // if not constant, px acts as world-units scale

        const float headLen = headL * totalLen;
        const float rHead = headR * totalLen;

        // Constraint cone points TO the target node.
        // So apex is at the node position, base is headLen backwards along z.
        QVector3D headPos = data.position;
        QVector3D tailPos = data.position - z * headLen;

        auto toW = [&](const QVector3D &pL)
        { return tailPos + x * pL.x() + y * pL.y() + z * pL.z(); };

        const uint32_t baseV = (uint32_t)m_pos.size();
        const GLsizei baseI = (GLsizei)outIdx.size();

        // Head (cone as triangle fan, per-tri normal)
        if (headLen > 1e-6f && rHead > 1e-7f)
        {
            QVector3D apexW = toW({0, 0, headLen}); // matches headPos
            for (int i = 0; i < segs; i++)
            {
                float th = float(i) / float(segs) * 6.28318530718f;
                float thn = float(i + 1) / float(segs) * 6.28318530718f;
                QVector3D rim0 = toW({rHead * std::cos(th), rHead * std::sin(th), 0});
                QVector3D rim1 = toW({rHead * std::cos(thn), rHead * std::sin(thn), 0});
                QVector3D N = QVector3D::normal(rim0 - apexW, rim1 - apexW).normalized();
                
                m_pos.push_back(apexW);
                m_nrm.push_back(N);
                m_col.push_back(A.color);
                
                m_pos.push_back(rim0);
                m_nrm.push_back(N);
                m_col.push_back(A.color);
                
                m_pos.push_back(rim1);
                m_nrm.push_back(N);
                m_col.push_back(A.color);
                
                uint32_t t0 = baseV + 3 * i + 0;
                uint32_t t1 = baseV + 3 * i + 1;
                uint32_t t2 = baseV + 3 * i + 2;
                addTri(outIdx, t0, t1, t2);
            }
        }

        // record range for this constraint
        GLsizei cnt = (GLsizei)outIdx.size() - baseI;
        if (cnt > 0)
        {
            auto &mapRef = isLit ? m_mapRangesLighting : m_mapRangesUnlighting;
            mapRef[data.id].push_back({baseI, cnt});
        }
    };

    for (const auto &data : m_vecConstraints)
    {
        if (data.style.lightingEnabled) 
            buildOne(data, idxLit, true);
        else 
            buildOne(data, idxUnlit, false);
    }

    m_idx.clear();
    m_iLightingCount = (GLsizei)idxLit.size();
    m_idx.insert(m_idx.end(), idxLit.begin(), idxLit.end());
    m_iUnlightingCount = (GLsizei)idxUnlit.size();
    m_idx.insert(m_idx.end(), idxUnlit.begin(), idxUnlit.end());

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vboPosition);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(m_pos.size() * sizeof(QVector3D)), m_pos.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void *)0);

    glBindBuffer(GL_ARRAY_BUFFER, m_vboNormal);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(m_nrm.size() * sizeof(QVector3D)), m_nrm.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void *)0);

    glBindBuffer(GL_ARRAY_BUFFER, m_vboColor);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(m_col.size() * sizeof(QVector4D)), m_col.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(QVector4D), (void *)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(m_idx.size() * sizeof(uint32_t)), m_idx.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(0);
    m_bRebuild = false;
}

void HRenderConstraint::draw(const QMatrix4x4 &P, const QMatrix4x4 &V, const HViewLighting &lighting, int viewportW, int viewportH)
{
    ensureGL();
    if (m_vecConstraints.empty())
        return;

    if (m_bConstantScreenSize || m_bRebuild)
        rebuild(P, V, viewportW, viewportH);

    const QMatrix4x4 VP = P * V;

    glBindVertexArray(m_vao);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    m_shaderProgram.bind();
    m_shaderProgram.setUniformValue("uMVP", VP);
    m_shaderProgram.setUniformValue("uView", V);
    m_shaderProgram.setUniformValue("uUseVertexColor", 1);
    m_shaderProgram.setUniformValue("uAlpha", 1.0f);

    lighting.applyTo(m_shaderProgram);

    // Lit
    if (m_iLightingCount > 0)
    {
        m_shaderProgram.setUniformValue("uLit", 1);
        const void *off = reinterpret_cast<const void *>(sizeof(uint32_t) * 0);
        glDrawElements(GL_TRIANGLES, m_iLightingCount, GL_UNSIGNED_INT, off);
    }
    // Unlit
    if (m_iUnlightingCount > 0)
    {
        m_shaderProgram.setUniformValue("uLit", 0);
        const void *off = reinterpret_cast<const void *>(sizeof(uint32_t) * m_iLightingCount);
        glDrawElements(GL_TRIANGLES, m_iUnlightingCount, GL_UNSIGNED_INT, off);
    }

    // --- Selection overlay (re-draw selected constraints with color/alpha) ---
    if (m_bIsSelected && !m_vecSelectionIds.empty())
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.f, -1.f); // pull forward slightly

        m_shaderProgram.setUniformValue("uLit", 0); // flat tint overlay
        m_shaderProgram.setUniformValue("uColor", m_vSelectionColor);
        m_shaderProgram.setUniformValue("uAlpha", m_fSelectionAlpha);

        // draw lit/unlit sub-ranges for each selected id
        for (int cid : m_vecSelectionIds)
        {
            auto itL = m_mapRangesLighting.find(cid);
            if (itL != m_mapRangesLighting.end())
            {
                for (auto &r : itL->second)
                {
                    const void *off = reinterpret_cast<const void *>(sizeof(uint32_t) * (0 + r.first));
                    glDrawElements(GL_TRIANGLES, r.second, GL_UNSIGNED_INT, off);
                }
            }
            auto itU = m_mapRangesUnlighting.find(cid);
            if (itU != m_mapRangesUnlighting.end())
            {
                for (auto &r : itU->second)
                {
                    const void *off = reinterpret_cast<const void *>(sizeof(uint32_t) * (m_iLightingCount + r.first));
                    glDrawElements(GL_TRIANGLES, r.second, GL_UNSIGNED_INT, off);
                }
            }
        }
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDepthMask(GL_TRUE);
    }

    m_shaderProgram.release();
    glBindVertexArray(0);
}

void HRenderConstraint::setSelection(const std::vector<int> &constraintIds, const QColor &color, float alpha)
{
    m_vecSelectionIds = constraintIds;
    m_vSelectionColor = QVector3D(color.redF(), color.greenF(), color.blueF());
    m_fSelectionAlpha = std::clamp(alpha, 0.f, 1.f);
    m_bIsSelected = !m_vecSelectionIds.empty();
}

void HRenderConstraint::clearSelection()
{
    m_vecSelectionIds.clear();
    m_bIsSelected = false;
}

#include <HornetView/HRenderElement.h>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <array>

// helper: compute vertex normals from triangles
static std::vector<QVector3D> computeNormals(const std::vector<QVector3D> &pos, const std::vector<uint32_t> &triIdx)
{
    std::vector<QVector3D> nrm(pos.size(), QVector3D(0, 0, 0));
    for (size_t i = 0; i + 2 < triIdx.size(); i += 3)
    {
        uint32_t ia = triIdx[i + 0], ib = triIdx[i + 1], ic = triIdx[i + 2];
        if (ia >= pos.size() || ib >= pos.size() || ic >= pos.size())
            continue;
        const QVector3D &A = pos[ia];
        const QVector3D &B = pos[ib];
        const QVector3D &C = pos[ic];
        QVector3D n = QVector3D::crossProduct(B - A, C - A);
        if (!n.isNull())
            n.normalize();
        nrm[ia] += n;
        nrm[ib] += n;
        nrm[ic] += n;
    }
    for (auto &n : nrm)
    {
        if (!n.isNull())
            n.normalize();
        else
            n = QVector3D(0, 0, 1);
    }
    return nrm;
}

HRenderElement::HRenderElement()
{
    m_vao = 0;
    m_vboPosition = 0;
    m_vboNormal = 0;
    m_eboTri = 0;
    m_eboEdge = 0;
    m_bInitialize = false;

    m_iVertexCount = 0;
    m_iTriCount = 0;
    m_iEdgeCount = 0;
    m_vecElementBatches.clear();
    m_vecEdgeBatches.clear();

    m_bEnablePerElementColor = true;

    m_vDefaultEdgeColor = QVector3D(0.1f, 0.1f, 0.1f);
    m_vDefaultElementColor = QVector3D(0.75f, 0.75f, 0.78f);
    m_fEdgeWidth = 1.0f;

    m_mapElementTriRanges.clear();
    m_vecSelectionElementIds.clear();
    m_vSelectionElementColor = QVector3D(0.25f, 0.6f, 1.f);
    m_fSelectionElementAlpha = 0.35f;
    m_bIsElementSelected = false;
}

void HRenderElement::initialize()
{
    initializeOpenGLFunctions();

    if (!m_shaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/vertex/element"))
        qWarning("Failed compiling shader: %s", m_shaderProgram.log());

    if (!m_shaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/fragment/element"))
        qWarning("Failed compiling shader: %s", m_shaderProgram.log());

    if (!m_shaderProgram.link())
        qWarning() << m_shaderProgram.log();
        
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vboPosition);
    glGenBuffers(1, &m_vboNormal);
    glGenBuffers(1, &m_eboTri);
    glGenBuffers(1, &m_eboEdge);

    m_bInitialize = true;

    upload({}, {}, {}, {}, {}, {});
}

void HRenderElement::destroy()
{
    if (m_eboEdge)
    {
        glDeleteBuffers(1, &m_eboEdge);
        m_eboEdge = 0;
    }
    if (m_eboTri)
    {
        glDeleteBuffers(1, &m_eboTri);
        m_eboTri = 0;
    }
    if (m_vboNormal)
    {
        glDeleteBuffers(1, &m_vboNormal);
        m_vboNormal = 0;
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

void HRenderElement::setElements(const std::vector<QVector3D> &pos, const std::unordered_map<int, int> &idToIdx, const std::vector<RenderElementData> &elements)
{
    // Build everything we need, then call upload(...)
    auto IDX = [&](int nodeId) -> int
    {
        auto it = idToIdx.find(nodeId);
        return (it == idToIdx.end()) ? -1 : it->second;
    };
    auto P = [&](int i) -> const QVector3D &
    { return pos[i]; };

    // 2) local index buffers + batches
    std::vector<uint32_t> triIdx;
    triIdx.reserve(elements.size() * 6);
    std::vector<uint32_t> lineIdx;
    lineIdx.reserve(elements.size() * 6);
    std::vector<FaceBatch> faceBatches;
    std::vector<LineBatch> lineBatches; // (we batch edges by a single default color)

    auto emitTri3 = [&](int a, int b, int c)
    {
        if (a < 0 || b < 0 || c < 0)
            return;
        triIdx.push_back(uint32_t(a));
        triIdx.push_back(uint32_t(b));
        triIdx.push_back(uint32_t(c));
    };
    auto emitTri6 = [&](int a, int b, int c, int ab, int bc, int ca)
    {
        emitTri3(a, ab, ca);
        emitTri3(ab, b, bc);
        emitTri3(ca, bc, c);
        emitTri3(ca, ab, bc);
    };
    auto emitQuad4 = [&](int a, int b, int c, int d)
    {
        emitTri3(a, b, c);
        emitTri3(a, c, d);
    };
    auto emitQuad8 = [&](int a, int b, int c, int d, int ab, int bc, int cd, int da)
    {
        if (a < 0 || b < 0 || c < 0 || d < 0 || ab < 0 || bc < 0 || cd < 0 || da < 0)
            return;
        emitTri3(a, ab, da);
        emitTri3(b, bc, ab);
        emitTri3(c, cd, bc);
        emitTri3(d, da, cd);
        emitTri3(ab, bc, cd);
        emitTri3(ab, cd, da);
    };
    auto edgeKey = [](uint32_t u, uint32_t v) -> uint64_t
    {
        if (u > v)
            std::swap(u, v);
        return (uint64_t(u) << 32) | uint64_t(v);
    };
    std::unordered_set<uint64_t> edgeSet;
    auto addEdge = [&](int i, int j)
    {
        if (i < 0 || j < 0)
            return;
        uint32_t u = uint32_t(i), v = uint32_t(j);
        uint64_t k = edgeKey(u, v);
        if (edgeSet.insert(k).second)
        {
            lineIdx.push_back(u);
            lineIdx.push_back(v);
        }
    };
    std::vector<std::pair<uint32_t, uint32_t>> extraLines;
    auto emitLine = [&](int i, int j)
    {
        if (i < 0 || j < 0)
            return;
        extraLines.emplace_back(uint32_t(i), uint32_t(j));
    };
    struct TriFace
    {
        int a, b, c;
        int ab = -1, bc = -1, ca = -1;
        QVector4D col;
        QVector3D elemC;
        int ownerId = -1;
        std::array<int, 3> key() const
        {
            auto k = std::array<int, 3>{a, b, c};
            std::sort(k.begin(), k.end());
            return k;
        }
    };
    struct QuadFace
    {
        int a, b, c, d;
        int ab = -1, bc = -1, cd = -1, da = -1;
        QVector4D col;
        QVector3D elemC;
        int ownerId = -1;
        std::array<int, 4> key() const
        {
            auto k = std::array<int, 4>{a, b, c, d};
            std::sort(k.begin(), k.end());
            return k;
        }
    };
    std::vector<TriFace> triFaces;
    triFaces.reserve(elements.size() * 4);
    std::vector<QuadFace> quadFaces;
    quadFaces.reserve(elements.size() * 6);

    // walk elements
    auto addQuad = [&](const RenderElementData &e, int a, int b, int c, int d)
    {
        // also edges
        addEdge(a, b);
        addEdge(b, c);
        addEdge(c, d);
        addEdge(d, a);
    };
    auto addTri = [&](const RenderElementData &e, int a, int b, int c)
    {
        addEdge(a, b);
        addEdge(b, c);
        addEdge(c, a);
    };
    for (const auto &e : elements)
    {
        QVector4D ecol(e.r, e.g, e.b, e.a);
        switch (e.type)
        {
        case ElementType::ElementTypeTri3:
        {
            int a = IDX(e.v[0]), b = IDX(e.v[1]), c = IDX(e.v[2]);
            GLsizei first = (GLsizei)triIdx.size();
            emitTri3(a, b, c);
            GLsizei count = (GLsizei)triIdx.size() - first;
            if (count > 0)
                faceBatches.push_back({first, count, ecol, e.id});
            addTri(e, a, b, c);
        }
        break;
        case ElementType::ElementTypeTri6:
        {
            int a = IDX(e.v[0]), b = IDX(e.v[1]), c = IDX(e.v[2]);
            int ab = IDX(e.v[3]), bc = IDX(e.v[4]), ca = IDX(e.v[5]);
            GLsizei first = (GLsizei)triIdx.size();
            emitTri6(a, b, c, ab, bc, ca);
            GLsizei count = (GLsizei)triIdx.size() - first;
            if (count > 0)
                faceBatches.push_back({first, count, ecol, e.id});
            addEdge(a, ab);
            addEdge(ab, b);
            addEdge(b, bc);
            addEdge(bc, c);
            addEdge(c, ca);
            addEdge(ca, a);
        }
        break;
        case ElementType::ElementTypeQuad4:
        {
            int a = IDX(e.v[0]), b = IDX(e.v[1]), c = IDX(e.v[2]), d = IDX(e.v[3]);
            GLsizei first = (GLsizei)triIdx.size();
            emitQuad4(a, b, c, d);
            GLsizei count = (GLsizei)triIdx.size() - first;
            if (count > 0)
                faceBatches.push_back({first, count, ecol, e.id});
            addQuad(e, a, b, c, d);
        }
        break;
        case ElementType::ElementTypeQuad8:
        {
            int a = IDX(e.v[0]), b = IDX(e.v[1]), c = IDX(e.v[2]), d = IDX(e.v[3]);
            int ab = IDX(e.v[4]), bc = IDX(e.v[5]), cd = IDX(e.v[6]), da = IDX(e.v[7]);
            GLsizei first = (GLsizei)triIdx.size();
            emitQuad8(a, b, c, d, ab, bc, cd, da);
            GLsizei count = (GLsizei)triIdx.size() - first;
            if (count > 0)
                faceBatches.push_back({first, count, ecol, e.id});
            addEdge(a, ab);
            addEdge(ab, b);
            addEdge(b, bc);
            addEdge(bc, c);
            addEdge(c, cd);
            addEdge(cd, d);
            addEdge(d, da);
            addEdge(da, a);
        }
        break;
        case ElementType::ElementTypeLine2:
        {
            emitLine(IDX(e.v[0]), IDX(e.v[1]));
        }
        break;
        case ElementType::ElementTypeLine3:
        {
            int i0 = IDX(e.v[0]), im = IDX(e.v[1]), i1 = IDX(e.v[2]);
            emitLine(i0, im);
            emitLine(im, i1);
        }
        break;
        case ElementType::ElementTypeTet4:
        {
            int n0 = IDX(e.v[0]), n1 = IDX(e.v[1]), n2 = IDX(e.v[2]), n3 = IDX(e.v[3]);
            QVector3D C = (P(n0) + P(n1) + P(n2) + P(n3)) * 0.25f;
            triFaces.push_back({n0, n1, n2, -1, -1, -1, ecol, C, e.id});
            triFaces.push_back({n0, n3, n1, -1, -1, -1, ecol, C, e.id});
            triFaces.push_back({n1, n3, n2, -1, -1, -1, ecol, C, e.id});
            triFaces.push_back({n0, n2, n3, -1, -1, -1, ecol, C, e.id});
        }
        break;
        case ElementType::ElementTypeTet10:
        {
            int n0 = IDX(e.v[0]), n1 = IDX(e.v[1]), n2 = IDX(e.v[2]), n3 = IDX(e.v[3]);
            int n01 = IDX(e.v[4]), n12 = IDX(e.v[5]), n20 = IDX(e.v[6]),
                n03 = IDX(e.v[7]), n13 = IDX(e.v[8]), n23 = IDX(e.v[9]);
            QVector3D C = (P(n0) + P(n1) + P(n2) + P(n3)) * 0.25f;
            triFaces.push_back({n0, n1, n2, n01, n12, n20, ecol, C, e.id});
            triFaces.push_back({n0, n3, n1, n03, n13, n01, ecol, C, e.id});
            triFaces.push_back({n1, n3, n2, n13, n23, n12, ecol, C, e.id});
            triFaces.push_back({n0, n2, n3, n20, n23, n03, ecol, C, e.id});
        }
        break;
        case ElementType::ElementTypeHex8:
        {
            int n0 = IDX(e.v[0]), n1 = IDX(e.v[1]), n2 = IDX(e.v[2]), n3 = IDX(e.v[3]);
            int n4 = IDX(e.v[4]), n5 = IDX(e.v[5]), n6 = IDX(e.v[6]), n7 = IDX(e.v[7]);
            QVector3D C = (P(n0) + P(n1) + P(n2) + P(n3) + P(n4) + P(n5) + P(n6) + P(n7)) * (1.0f / 8.0f);
            quadFaces.push_back({n0, n1, n2, n3, -1, -1, -1, -1, ecol, C, e.id});
            quadFaces.push_back({n4, n5, n6, n7, -1, -1, -1, -1, ecol, C, e.id});
            quadFaces.push_back({n0, n1, n5, n4, -1, -1, -1, -1, ecol, C, e.id});
            quadFaces.push_back({n1, n2, n6, n5, -1, -1, -1, -1, ecol, C, e.id});
            quadFaces.push_back({n2, n3, n7, n6, -1, -1, -1, -1, ecol, C, e.id});
            quadFaces.push_back({n3, n0, n4, n7, -1, -1, -1, -1, ecol, C, e.id});
        }
        break;
        case ElementType::ElementTypeHex20:
        {
            int c0 = IDX(e.v[0]), c1 = IDX(e.v[1]), c2 = IDX(e.v[2]), c3 = IDX(e.v[3]);
            int c4 = IDX(e.v[4]), c5 = IDX(e.v[5]), c6 = IDX(e.v[6]), c7 = IDX(e.v[7]);
            int m01 = IDX(e.v[8]), m12 = IDX(e.v[9]), m23 = IDX(e.v[10]), m30 = IDX(e.v[11]),
                m45 = IDX(e.v[12]), m56 = IDX(e.v[13]), m67 = IDX(e.v[14]), m74 = IDX(e.v[15]),
                m04 = IDX(e.v[16]), m15 = IDX(e.v[17]), m26 = IDX(e.v[18]), m37 = IDX(e.v[19]);
            QVector3D C = (P(c0) + P(c1) + P(c2) + P(c3) + P(c4) + P(c5) + P(c6) + P(c7)) * (1.0f / 8.0f);
            quadFaces.push_back({c0, c1, c2, c3, m01, m12, m23, m30, ecol, C, e.id});
            quadFaces.push_back({c4, c5, c6, c7, m45, m56, m67, m74, ecol, C, e.id});
            quadFaces.push_back({c0, c1, c5, c4, m01, m15, m45, m04, ecol, C, e.id});
            quadFaces.push_back({c1, c2, c6, c5, m12, m26, m56, m15, ecol, C, e.id});
            quadFaces.push_back({c2, c3, c7, c6, m23, m37, m67, m26, ecol, C, e.id});
            quadFaces.push_back({c3, c0, c4, c7, m30, m04, m74, m37, ecol, C, e.id});
        }
        break;
        }
    }
    auto triKeyStr = [&](const std::array<int, 3> &k)
    {
        return std::to_string(k[0]) + "_" + std::to_string(k[1]) + "_" + std::to_string(k[2]);
    };
    auto quadKeyStr = [&](const std::array<int, 4> &k)
    {
        return std::to_string(k[0]) + "_" + std::to_string(k[1]) + "_" + std::to_string(k[2]) + "_" + std::to_string(k[3]);
    };
    std::unordered_map<std::string, int> triSeen, quadSeen;
    for (auto &f : triFaces)
        ++triSeen[triKeyStr(f.key())];
    for (auto &f : quadFaces)
        ++quadSeen[quadKeyStr(f.key())];
    auto fixTriOutward = [&](TriFace &f)
    {
        const QVector3D &A = P(f.a), &B = P(f.b), &C = P(f.c);
        QVector3D n = QVector3D::crossProduct(B - A, C - A);
        QVector3D fc = (A + B + C) / 3.0f;
        if (QVector3D::dotProduct(n, fc - f.elemC) < 0.0f)
        {
            std::swap(f.b, f.c);
            if (f.ab >= 0 || f.bc >= 0 || f.ca >= 0)
            {
                int ab = f.ab, bc = f.bc, ca = f.ca;
                f.ab = ca;
                f.bc = bc;
                f.ca = ab;
            }
        }
    };
    auto fixQuadOutward = [&](QuadFace &f)
    {
        const QVector3D &A = P(f.a), &B = P(f.b), &C = P(f.c), &D = P(f.d);
        QVector3D n = QVector3D::crossProduct(B - A, C - A);
        QVector3D fc = (A + B + C + D) * 0.25f;
        if (QVector3D::dotProduct(n, fc - f.elemC) < 0.0f)
        {
            std::swap(f.b, f.d);
            int ab = f.ab, bc = f.bc, cd = f.cd, da = f.da;
            f.ab = da;
            f.bc = cd;
            f.cd = bc;
            f.da = ab;
        }
    };
    for (auto &f : triFaces)
    {
        if (triSeen[triKeyStr(f.key())] != 1)
            continue;
        fixTriOutward(f);
        GLsizei first = (GLsizei)triIdx.size();
        if (f.ab < 0 || f.bc < 0 || f.ca < 0)
            emitTri3(f.a, f.b, f.c);
        else
            emitTri6(f.a, f.b, f.c, f.ab, f.bc, f.ca);
        GLsizei count = (GLsizei)triIdx.size() - first;
        if (count > 0)
            faceBatches.push_back({first, count, f.col, f.ownerId});
        if (f.ab < 0)
        {
            addEdge(f.a, f.b);
            addEdge(f.b, f.c);
            addEdge(f.c, f.a);
        }
        else
        {
            addEdge(f.a, f.ab);
            addEdge(f.ab, f.b);
            addEdge(f.b, f.bc);
            addEdge(f.bc, f.c);
            addEdge(f.c, f.ca);
            addEdge(f.ca, f.a);
        }
    }
    for (auto &f : quadFaces)
    {
        if (quadSeen[quadKeyStr(f.key())] != 1)
            continue;
        fixQuadOutward(f);
        GLsizei first = (GLsizei)triIdx.size();
        if (f.ab < 0 || f.bc < 0 || f.cd < 0 || f.da < 0)
            emitQuad4(f.a, f.b, f.c, f.d);
        else
            emitQuad8(f.a, f.b, f.c, f.d, f.ab, f.bc, f.cd, f.da);
        GLsizei count = (GLsizei)triIdx.size() - first;
        if (count > 0)
            faceBatches.push_back({first, count, f.col, f.ownerId});
        if (f.ab < 0)
        {
            addEdge(f.a, f.b);
            addEdge(f.b, f.c);
            addEdge(f.c, f.d);
            addEdge(f.d, f.a);
        }
        else
        {
            addEdge(f.a, f.ab);
            addEdge(f.ab, f.b);
            addEdge(f.b, f.bc);
            addEdge(f.bc, f.c);
            addEdge(f.c, f.cd);
            addEdge(f.cd, f.d);
            addEdge(f.d, f.da);
            addEdge(f.da, f.a);
        }
    }
    for (auto &p : extraLines)
    {
        uint64_t k = ((uint64_t)p.first << 32) | (uint64_t)p.second;
        if (edgeSet.insert(k).second)
        {
            lineIdx.push_back(p.first);
            lineIdx.push_back(p.second);
        }
    }
    if (faceBatches.empty() && !triIdx.empty())
        faceBatches.push_back({0, (GLsizei)triIdx.size(), QVector4D(m_vDefaultElementColor.x(), m_vDefaultElementColor.y(), m_vDefaultElementColor.z(), 1.0f), -1});
    if (lineBatches.empty() && !lineIdx.empty())
        lineBatches.push_back({0, (GLsizei)lineIdx.size(), QVector4D(m_vDefaultEdgeColor.x(), m_vDefaultEdgeColor.y(), m_vDefaultEdgeColor.z(), 1.0f)});

    // 3) normals
    auto nrm = computeNormals(pos, triIdx);

    // 4) upload to GPU + build element selection ranges
    if (m_bInitialize)
        upload(pos, nrm, triIdx, lineIdx, faceBatches, lineBatches);
    // 4) upload to GPU + build element selection ranges
    // upload(pos, nrm, triIdx, lineIdx, faceBatches, lineBatches);
}

void HRenderElement::upload(const std::vector<QVector3D> &pos, const std::vector<QVector3D> &nrm, const std::vector<uint32_t> &triIdx, const std::vector<uint32_t> &lineIdx, const std::vector<FaceBatch> &faceBatches, const std::vector<LineBatch> &lineBatches)
{
    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vboPosition);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(pos.size() * sizeof(QVector3D)), pos.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void *)0);
    m_iVertexCount = GLsizei(pos.size());

    glBindBuffer(GL_ARRAY_BUFFER, m_vboNormal);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(nrm.size() * sizeof(QVector3D)), nrm.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void *)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboTri);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(triIdx.size() * sizeof(uint32_t)), triIdx.data(), GL_STATIC_DRAW);
    m_iTriCount = GLsizei(triIdx.size());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboEdge);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(lineIdx.size() * sizeof(uint32_t)), lineIdx.data(), GL_STATIC_DRAW);
    m_iEdgeCount = GLsizei(lineIdx.size());

    glBindVertexArray(0);

    // Build element-id → tri ranges map
    m_mapElementTriRanges.clear();
    for (const auto &b : faceBatches)
    {
        if (b.elementId >= 0 && b.count > 0)
        {
            m_mapElementTriRanges[b.elementId].push_back({b.first, b.count});
        }
    }

    m_vecElementBatches = faceBatches;
    m_vecEdgeBatches = lineBatches;
}

void HRenderElement::setDefaultLineColor(const QColor &color)
{
    m_vDefaultEdgeColor = QVector3D(color.redF(), color.greenF(), color.blueF());
}
void HRenderElement::setDefaultFaceColor(const QColor &color)
{
    m_vDefaultElementColor = QVector3D(color.redF(), color.greenF(), color.blueF());
}

QColor HRenderElement::defaultEdgeColor() const 
{ 
    return QColor::fromRgbF(m_vDefaultEdgeColor.x(), m_vDefaultEdgeColor.y(), m_vDefaultEdgeColor.z()); 
}

QColor HRenderElement::defaultFaceColor() const 
{ 
    return QColor::fromRgbF(m_vDefaultElementColor.x(), m_vDefaultElementColor.y(), m_vDefaultElementColor.z()); 
}

void HRenderElement::setEdgeWidth(float pixel) 
{
    m_fEdgeWidth = std::max(1.f, pixel);
}

float HRenderElement::edgeWidth() const
{
    return m_fEdgeWidth;
}

void HRenderElement::drawElement(const QMatrix4x4 &P, const QMatrix4x4 &V, const HViewLighting &lighting)
{
    if (m_iVertexCount == 0)
        return;

    const QMatrix4x4 MVP = P * V;

    // Faces
    if (m_iTriCount > 0)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        // two-sided to avoid disappearing faces when backfacing
        GLboolean hadCull = glIsEnabled(GL_CULL_FACE);
        if (hadCull)
            glDisable(GL_CULL_FACE);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.f, 2.f);

        m_shaderProgram.bind();
        m_shaderProgram.setUniformValue("uMVP", MVP);
        m_shaderProgram.setUniformValue("uView", V);
        m_shaderProgram.setUniformValue("uLit", 1);

        // NEW: feed all lighting uniforms in one call
        lighting.applyTo(m_shaderProgram);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboTri);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        for (const auto &b : m_vecElementBatches)
        {
            QVector3D col = m_bEnablePerElementColor ? QVector3D(b.color.x(), b.color.y(), b.color.z()) : m_vDefaultElementColor;
            m_shaderProgram.setUniformValue("uColor", col);
            const void *off = reinterpret_cast<const void *>(sizeof(uint32_t) * b.first);
            glDrawElements(GL_TRIANGLES, b.count, GL_UNSIGNED_INT, off);
        }
        glBindVertexArray(0);
        m_shaderProgram.release();

        glDisable(GL_POLYGON_OFFSET_FILL);
        if (hadCull)
            glEnable(GL_CULL_FACE);
    }

    // === element selection overlay (faces only; optional: add edges too) ===
    if (m_bIsElementSelected)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); // overlay without messing depth
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.f, -1.f); // pull slightly toward camera

        m_shaderProgram.bind();
        QMatrix4x4 MVP = P * V;
        m_shaderProgram.setUniformValue("uMVP", MVP);
        m_shaderProgram.setUniformValue("uView", V);
        m_shaderProgram.setUniformValue("uLit", 0);
        m_shaderProgram.setUniformValue("uColor", m_vSelectionElementColor);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboTri);

        // manual alpha in shader: add uAlpha or premultiply here by drawing twice thinner;
        // quick path: modulate via glBlendColor is not portable with current shader; do per-color
        // If you want uAlpha: add uniform and multiply in FS; for now we fake with color only.
        for (int eid : m_vecSelectionElementIds)
        {
            auto it = m_mapElementTriRanges.find(eid);
            if (it == m_mapElementTriRanges.end())
                continue;
            for (auto &r : it->second)
            {
                const void *off = reinterpret_cast<const void *>(sizeof(uint32_t) * r.first);
                glDrawElements(GL_TRIANGLES, r.second, GL_UNSIGNED_INT, off);
            }
        }
        glBindVertexArray(0);
        m_shaderProgram.release();

        glDisable(GL_POLYGON_OFFSET_FILL);
        glDepthMask(GL_TRUE);
    }
}

void HRenderElement::drawEdges(const QMatrix4x4 &P, const QMatrix4x4 &V)
{
    if (m_iVertexCount == 0)
        return;

    const QMatrix4x4 MVP = P * V;

    // Edges
    if (m_iEdgeCount > 0)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        GLint oldDepthFunc;
        glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
        glDepthFunc(GL_LEQUAL);

        m_shaderProgram.bind();
        m_shaderProgram.setUniformValue("uMVP", MVP);
        m_shaderProgram.setUniformValue("uView", V);
        m_shaderProgram.setUniformValue("uLit", 0); // unlit wire
        // We can still apply lighting (uniforms exist), but uLit=0 skips lighting in shader.

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboEdge);
        glLineWidth(m_fEdgeWidth);
        glEnable(GL_LINE_SMOOTH);

        for (const auto &b : m_vecEdgeBatches)
        {
            m_shaderProgram.setUniformValue("uColor", m_vDefaultEdgeColor);
            const void *off = reinterpret_cast<const void *>(sizeof(uint32_t) * b.first);
            glDrawElements(GL_LINES, b.count, GL_UNSIGNED_INT, off);
        }
        glBindVertexArray(0);
        m_shaderProgram.release();

        glDepthMask(GL_TRUE);
        glDepthFunc(oldDepthFunc);
    }
}

void HRenderElement::setSelection(const std::vector<int> &elementIds, const QColor &color, float alpha)
{
    m_vecSelectionElementIds = elementIds;
    m_vSelectionElementColor = QVector3D(color.redF(), color.greenF(), color.blueF());
    m_fSelectionElementAlpha = std::clamp(alpha, 0.f, 1.f);
    m_bIsElementSelected = !m_vecSelectionElementIds.empty();
}

void HRenderElement::clearSelection()
{
    m_vecSelectionElementIds.clear();
    m_bIsElementSelected = false;
}

void HRenderElement::enablePerElementColor(bool enable)
{
    m_bEnablePerElementColor = enable;
}
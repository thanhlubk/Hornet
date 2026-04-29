// RenderElement.h
#pragma once
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector4D>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include "HViewLighting.h"
#include "HornetBase/HIElement.h"

// Lightweight rendering-only element descriptor (no DB dependency)
struct RenderElementData
{
    int id = -1;
    ElementType type = ElementType::ElementTypeUnkown;
    int v[20] = {0};           // node IDs for connectivity
    float r = 0.65f, g = 0.75f, b = 0.90f, a = 1.0f; // fallback element color
};

struct FaceBatch
{
    GLsizei first, count;
    QVector4D color;
    int elementId = -1;
};
struct LineBatch
{
    GLsizei first, count;
    QVector4D color;
};

class HRenderElement : protected QOpenGLExtraFunctions
{
public:
    explicit HRenderElement();
    void initialize();
    void destroy();

    void setElements(const std::vector<QVector3D> &positions,
                     const std::vector<QVector4D> &nodeColors,
                     const std::unordered_map<int, int> &nodeIdToIndex,
                     const std::vector<RenderElementData> &elements);

    void upload(const std::vector<QVector3D> &positions,
                const std::vector<QVector3D> &normals,
                const std::vector<QVector4D> &colors,
                const std::vector<uint32_t> &triIdx,
                const std::vector<uint32_t> &lineIdx,
                const std::vector<FaceBatch> &faceBatches,
                const std::vector<LineBatch> &lineBatches);

    void setDefaultLineColor(const QColor &color);
    void setDefaultFaceColor(const QColor &color);

    QColor defaultEdgeColor() const;
    QColor defaultFaceColor() const;

    void setEdgeWidth(float pixel);
    float edgeWidth() const;

    void drawElement(const QMatrix4x4 &P, const QMatrix4x4 &V, const HViewLighting &lighting);
    void drawEdges(const QMatrix4x4 &P, const QMatrix4x4 &V);

    void setSelection(const std::vector<int> &elementIds, const QColor &color, float alpha);
    void clearSelection();
    void enablePerElementColor(bool enable);

    // When true, faces use the per-node color attribute, interpolated over triangles.
    void enablePerVertexColor(bool enable);

private:
    QOpenGLShaderProgram m_shaderProgram;
    GLuint m_vao, m_vboPosition, m_vboNormal, m_vboColor, m_eboTri, m_eboEdge;
    bool m_bInitialize;

    GLsizei m_iVertexCount, m_iTriCount, m_iEdgeCount;
    std::vector<FaceBatch> m_vecElementBatches;
    std::vector<LineBatch> m_vecEdgeBatches;

    bool m_bEnablePerElementColor;
    bool m_bEnablePerVertexColor;

    QVector3D m_vDefaultElementColor, m_vDefaultEdgeColor;
    float m_fEdgeWidth;

    // map: elementId -> list of (first,count) triangle ranges
    std::unordered_map<int, std::vector<std::pair<GLsizei, GLsizei>>> m_mapElementTriRanges;
    // selection state
    std::vector<int> m_vecSelectionElementIds;
    QVector3D m_vSelectionElementColor;
    float m_fSelectionElementAlpha;
    bool m_bIsElementSelected;
};

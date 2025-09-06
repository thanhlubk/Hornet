// RenderElement.h
#pragma once
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector4D>
#include <vector>
#include <cstdint>
#include "HViewLighting.h"
#include "HViewDef.h"

struct FaceBatch
{
    GLsizei first, count;
    QVector4D color;
    int elementId = -1; // NEW
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

    void setElements(const std::vector<Node> &nodes, const std::vector<Element> &elements);
    void upload(const std::vector<QVector3D> &positions, const std::vector<QVector3D> &normals, const std::vector<uint32_t> &triIdx, const std::vector<uint32_t> &lineIdx, const std::vector<FaceBatch> &faceBatches, const std::vector<LineBatch> &lineBatches);

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

private:
    QOpenGLShaderProgram m_shaderProgram;
    GLuint m_vao, m_vboPosition, m_vboNormal, m_eboTri, m_eboEdge;
    bool m_bInitialize;

    GLsizei m_iVertexCount, m_iTriCount, m_iEdgeCount;
    std::vector<FaceBatch> m_vecElementBatches;
    std::vector<LineBatch> m_vecEdgeBatches;

    bool m_bEnablePerElementColor;

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

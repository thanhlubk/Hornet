// RenderNode.h
#pragma once
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector4D>
#include <unordered_map>
#include <vector>
#include <QColor>


class HRenderNode : protected QOpenGLExtraFunctions
{
public:
    explicit HRenderNode();

    void initialize();
    void destroy();

    void upload(const std::vector<QVector3D> &positions, const std::vector<QVector4D> &colors, const std::vector<int> &nodeIds);
    void draw(const QMatrix4x4 &P, const QMatrix4x4 &V);

    void setNodeSize(float pixel);
    float nodeSize() const;
    void enablePerNodeColor(bool enable);
    void setDefaultColor(const QColor &color);
    QColor defaultColor() const;
    
    void setSelection(const std::vector<int> &nodeIds, const QColor &color, float sizeScale);
    void clearSelection();

private:
    QOpenGLShaderProgram m_shaderProgram;
    GLuint m_vao, m_vboPosition, m_vboColor;
    GLsizei m_iNumNode;
    bool m_bInitialize;

    float m_fNodeSize;
    bool m_bEnablePerNodeColor;
    QVector3D m_vDefaultColor;

    std::unordered_map<int, int> m_mapIdIndex; // nodeId -> vertex index
    GLuint m_eboSelection; // indices of selected vertices
    GLsizei m_iSelectionCount;
    QVector3D m_vSelectionColor;
    float m_fSelectionSizeScale;
    bool m_bIsSelected;
};

#pragma once
#include "HornetViewExport.h"
#include "HRenderNode.h"
#include "HRenderElement.h"
#include <QObject>
#include <unordered_map>

class HORNETVIEW_EXPORT HRenderModel : public QObject
{
    Q_OBJECT
public:
    HRenderModel(QObject *parent = nullptr);

    void initialize();
    void destroy();

    void draw(const QMatrix4x4 &P, const QMatrix4x4 &V, const HViewLighting &lighting);

    void setShowFaces(bool on);
    void setShowEdges(bool on);
    void setShowNodes(bool on);

    void setDefaultElementFaceColor(const QColor &color);
    void setDefaultElementEdgeColor(const QColor &color);
    void setDefaultNodeColor(const QColor &color);
    void setNodeSize(float px);
    void setElementEdgeWidth(float px);

    void enablePerNodeColor(bool enable);
    void enablePerElementColor(bool enable);
    void enablePerElementVertexColor(bool enable);

    float frameRadius() const;
    QVector3D center() const;

    void setMesh(const std::vector<QVector3D> &positions,
                 const std::vector<QVector4D> &nodeColors,
                 const std::vector<int> &nodeIds,
                 const std::unordered_map<int, int> &nodeIdToIndex,
                 const std::vector<RenderElementData> &elements);

    void setNodeSelection(const std::vector<int> &nodeIds, const QColor &color, float sizeScale);
    void clearNodeSelection();

    void setElementSelection(const std::vector<int> &elementIds, const QColor &color, float alpha);
    void clearElementSelection();

signals: 
    void settingChanged();
    void dataChanged();

private:
    HRenderNode m_renderNode;
    HRenderElement m_renderElement;

    bool m_bShowNode;
    bool m_bShowFace;
    bool m_bShowEdge;

    QVector3D m_vCenter;
    float m_fFrameRadius;
};
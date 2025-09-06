#pragma once
#include <vector>
#include <QColor>
#include "HRenderModel.h"
#include "HRenderForce.h"
#include "HViewDef.h"
#include "HornetViewExport.h"

class HORNETVIEW_EXPORT HViewSelectionManager : public QObject
{
    Q_OBJECT
public:
    HViewSelectionManager(QObject *parent = nullptr);

    void setType(SelectType t);
    SelectType type() const;

    // update selections (ids are external IDs: node IDs or element IDs)
    void setSelectedNodeIds(const std::vector<int> &ids);
    void setSelectedElementIds(const std::vector<int> &ids);
    void setSelectedForceIds(const std::vector<int> &ids);
    const std::vector<int> &selectedNodeIds() const;
    const std::vector<int> &selectedElementIds() const;
    const std::vector<int> &selectedForceIds() const;
    void clear(HRenderModel *model, HRenderForce *force);

    // highlight style
    void setNodeHighlightColor(const QColor &c);
    void setNodeScale(float s);
    void setElemHighlightColor(const QColor &c);
    void setElemAlpha(float a);
    void setForceHighlightColor(const QColor &c);
    void setForceAlpha(float a);

    // push into renderers
    void applyNode(HRenderModel *renderModel);
    void applyElement(HRenderModel *renderModel);
    void applyForce(HRenderForce *renderForce);
    void applyTo(HRenderModel* model, HRenderForce* force);

signals:
    void selectionChanged();

private:
    SelectType m_eSelectionType = SelectType::None;

    std::vector<int> m_vecSelectionNodeId;
    std::vector<int> m_vecSelectionElementId;
    std::vector<int> m_vecSelectionForceId;

    QColor m_colorNode = QColor(255, 215, 0); // gold
    float m_fNodeScale = 1.5f;
    QColor m_colorElement = QColor(64, 160, 255);
    float m_fElementAlpha = 0.35f;

    QColor m_colorForce = QColor(255, 128, 0); // orange
    float m_fForceAlpha = 0.45f;
};

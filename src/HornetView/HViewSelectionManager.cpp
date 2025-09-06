#include <HornetView/HViewSelectionManager.h>

HViewSelectionManager::HViewSelectionManager(QObject *parent)
    : QObject(parent)
{
    m_eSelectionType = SelectType::None;
    m_vecSelectionNodeId.clear();
    m_vecSelectionElementId.clear();
    m_vecSelectionForceId.clear();
}

void HViewSelectionManager::setType(SelectType type)
{
    m_eSelectionType = type;
}

SelectType HViewSelectionManager::type() const 
{ 
    return m_eSelectionType; 
}

// update selections (ids are external IDs: node IDs or element IDs)
void HViewSelectionManager::setSelectedNodeIds(const std::vector<int> &ids) 
{ 
    m_vecSelectionNodeId = ids; 
}

void HViewSelectionManager::setSelectedElementIds(const std::vector<int> &ids) 
{ 
    m_vecSelectionElementId = ids; 
}

void HViewSelectionManager::setSelectedForceIds(const std::vector<int> &ids) 
{ 
    m_vecSelectionForceId = ids; 
}

const std::vector<int>& HViewSelectionManager::selectedNodeIds() const
{ 
    return m_vecSelectionNodeId; 
}

const std::vector<int>& HViewSelectionManager::selectedElementIds() const
{ 
    return m_vecSelectionElementId; 
}

const std::vector<int>& HViewSelectionManager::selectedForceIds() const
{ 
    return m_vecSelectionForceId; 
}

void HViewSelectionManager::clear(HRenderModel *model, HRenderForce *force)
{
    m_vecSelectionNodeId.clear();
    m_vecSelectionElementId.clear();
    m_vecSelectionForceId.clear();

    applyTo(model, force);
}

// highlight style
void HViewSelectionManager::setNodeHighlightColor(const QColor &color)
{
    m_colorNode = color;
    emit selectionChanged();
}

void HViewSelectionManager::setNodeScale(float scale)
{
    m_fNodeScale = scale;
    emit selectionChanged();
} // multiplier on point size

void HViewSelectionManager::setElemHighlightColor(const QColor &color)
{
    m_colorElement = color;
    emit selectionChanged();
}

void HViewSelectionManager::setElemAlpha(float alpha)
{
    m_fElementAlpha = std::clamp(alpha, 0.f, 1.f);
    emit selectionChanged();
}

void HViewSelectionManager::setForceHighlightColor(const QColor &color)
{
    m_colorForce = color;
    emit selectionChanged();
}

void HViewSelectionManager::setForceAlpha(float alpha)
{
    m_fForceAlpha = std::clamp(alpha, 0.f, 1.f);
    emit selectionChanged();
}

// push into renderers
void HViewSelectionManager::applyNode(HRenderModel *renderModel)
{
    if (!renderModel)
        return;

    if (!m_vecSelectionNodeId.empty())
        renderModel->setNodeSelection(m_vecSelectionNodeId, m_colorNode, m_fNodeScale);
    else
        renderModel->clearNodeSelection();

    emit selectionChanged();
}

void HViewSelectionManager::applyElement(HRenderModel *renderModel)
{
    if (!renderModel)
        return;

    if (!m_vecSelectionElementId.empty())
        renderModel->setElementSelection(m_vecSelectionElementId, m_colorElement, m_fElementAlpha);
    else
        renderModel->clearElementSelection();

    emit selectionChanged();
}

void HViewSelectionManager::applyForce(HRenderForce *renderForce)
{
    if (!renderForce)
        return;

    if (!m_vecSelectionForceId.empty())
        renderForce->setSelection(m_vecSelectionForceId, m_colorForce, m_fForceAlpha);
    else
        renderForce->clearSelection();

    emit selectionChanged();
}

void HViewSelectionManager::applyTo(HRenderModel *model, HRenderForce *force)
{
    blockSignals(true);
    // clear existing selections in all
    applyNode(model);
    applyElement(model);
    applyForce(force);
    blockSignals(false);

    emit selectionChanged();
}
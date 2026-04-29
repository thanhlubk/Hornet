#include <HornetView/HRenderModel.h>

HRenderModel::HRenderModel(QObject *parent)
    : QObject(parent)
{
    m_bShowNode = true;
    m_bShowFace = true;
    m_bShowEdge = true;

    m_vCenter = QVector3D(0, 0, 0);
    m_fFrameRadius = 1.0f;
}

void HRenderModel::initialize()
{
    m_renderNode.initialize();
    m_renderElement.initialize();
}

void HRenderModel::destroy()
{
    m_renderNode.destroy();
    m_renderElement.destroy();
}

void HRenderModel::draw(const QMatrix4x4 &P, const QMatrix4x4 &V, const HViewLighting &lighting)
{
    if (m_bShowFace)
        m_renderElement.drawElement(P, V, lighting);

    if (m_bShowEdge)
        m_renderElement.drawEdges(P, V);

    if (m_bShowNode)
        m_renderNode.draw(P, V);
}

void HRenderModel::setShowFaces(bool on)
{
    m_bShowFace = on;
    emit settingChanged();
}

void HRenderModel::setShowEdges(bool on)
{
    m_bShowEdge = on;
    emit settingChanged();
}

void HRenderModel::setShowNodes(bool on)
{
    m_bShowNode = on;
    emit settingChanged();
}

void HRenderModel::setDefaultElementFaceColor(const QColor &color)
{
    m_renderElement.setDefaultFaceColor(color);
    emit settingChanged();
}

void HRenderModel::setDefaultElementEdgeColor(const QColor &color)
{
    m_renderElement.setDefaultLineColor(color);
    emit settingChanged();
}

void HRenderModel::setDefaultNodeColor(const QColor &color)
{
    m_renderNode.setDefaultColor(color);
    emit settingChanged();
}

void HRenderModel::setNodeSize(float px)
{
    m_renderNode.setNodeSize(px);
    emit settingChanged();
}

void HRenderModel::setElementEdgeWidth(float px)
{
    m_renderElement.setEdgeWidth(px);
    emit settingChanged();
}

void HRenderModel::enablePerNodeColor(bool enable)
{
    m_renderNode.enablePerNodeColor(enable);
    emit settingChanged();
}

void HRenderModel::enablePerElementColor(bool enable)
{
    m_renderElement.enablePerElementColor(enable);
    emit settingChanged();
}

void HRenderModel::enablePerElementVertexColor(bool enable)
{
    m_renderElement.enablePerVertexColor(enable);
    emit settingChanged();
}

float HRenderModel::frameRadius() const 
{ 
    return m_fFrameRadius; 
}

QVector3D HRenderModel::center() const 
{ 
    return m_vCenter; 
}

void HRenderModel::setMesh(const std::vector<QVector3D> &positions,
                           const std::vector<QVector4D> &nodeColors,
                           const std::vector<int> &nodeIds,
                           const std::unordered_map<int, int> &nodeIdToIndex,
                           const std::vector<RenderElementData> &elements)
{
    m_renderNode.upload(positions, nodeColors, nodeIds);
    m_renderElement.setElements(positions, nodeColors, nodeIdToIndex, elements);

    if (!positions.empty())
    {
        QVector3D mn = positions.front();
        QVector3D mx = mn;
        for (const auto &p : positions)
        {
            mn.setX(std::min(mn.x(), p.x()));
            mn.setY(std::min(mn.y(), p.y()));
            mn.setZ(std::min(mn.z(), p.z()));
            mx.setX(std::max(mx.x(), p.x()));
            mx.setY(std::max(mx.y(), p.y()));
            mx.setZ(std::max(mx.z(), p.z()));
        }

        m_vCenter = 0.5f * (mn + mx);
        m_fFrameRadius = 0.5f * (mx - mn).length();
        if (m_fFrameRadius < 1e-6f)
            m_fFrameRadius = 1.0f;
    }
    else
    {
        m_vCenter = {0, 0, 0};
        m_fFrameRadius = 1.0f;
    }

    emit dataChanged();
}

void HRenderModel::setNodeSelection(const std::vector<int> &nodeIds, const QColor &color, float sizeScale)
{
    m_renderNode.setSelection(nodeIds, color, sizeScale);
    emit settingChanged();
}

void HRenderModel::clearNodeSelection()
{
    m_renderNode.clearSelection();
    emit settingChanged();
}

void HRenderModel::setElementSelection(const std::vector<int> &elementIds, const QColor &color, float alpha)
{
    m_renderElement.setSelection(elementIds, color, alpha);
    emit settingChanged();
}

void HRenderModel::clearElementSelection()
{
    m_renderElement.clearSelection();
    emit settingChanged();
}
#pragma once
#include "HornetViewExport.h"
#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector4D>
#include <QQuaternion>
#include <QColor>
#include <QTimer>
#include <QElapsedTimer>
#include <QSet>
#include <vector>
#include <cstdint>
#include <QHash>
#include <algorithm>
#include "HViewCamera.h"
#include "HRenderModel.h"
#include "HRenderCoordinateGizmo.h"
#include "HViewLighting.h"
#include "HRenderForce.h"
#include "HViewSelectionManager.h"
#include "HViewDef.h"
#include "HornetBase/NotifyDispatcher.h"

class HORNETVIEW_EXPORT GLViewWindow : public QOpenGLWidget, protected QOpenGLExtraFunctions
{
    Q_OBJECT
public:
    explicit GLViewWindow(QWidget *parent = nullptr);
    ~GLViewWindow() override;

    void setNotifyDispatcher(NotifyDispatcher &disp);
    void onNotify(MessageType mess, MessageParam a, MessageParam b);

    void setMesh(const std::vector<Node> &nodes, const std::vector<Element> &elements);

    void fitView();

    // Mouse-look behavior for FPS (optional)
    void setAlwaysMouseLook(bool on);

    // Controls
    void setPickRadius(int pixel); // click tolerance

    // ---- Marquee style (RGBA) ----
    void setMarqueeFillColor(const QColor &color);
    void setMarqueeStrokeColor(const QColor &color);
    void setMarqueeStrokeWidth(float pixel);
    QColor marqueeFillColor()const;
    QColor marqueeStrokeColor()const;
    float marqueeStrokeWidth()const;

    HViewCamera *camera();
    HViewLighting *lighting();
    HViewSelectionManager *selectionManager();
    HRenderCoordinateGizmo* coordinateGizmo();
    HRenderModel* modelRenderer();
    HRenderForce* forceRenderer();

protected:
    // QOpenGLWidget
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    // Input
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;

private:
    NotifyDispatcher::Observer m_observer;

    void destroyGLObjects();
    bool getHitPosition(const QPointF &point, QVector3D &hit, int &elemId) const;
    void selectAtPoint(const QPointF &point);
    void selectInRect(const QRectF &rectMarquee);

    // Render
    HRenderModel *m_pRenderModel;
    HRenderCoordinateGizmo* m_pRenderCoordinate;
    HRenderForce* m_pRenderForce;

    // View control
    HViewSelectionManager* m_pSelection;
    HViewCamera* m_pCamera;
    HViewLighting* m_pLighting;

    // Control
    bool m_bRMBDown;
    bool m_bMMBDown;
    bool m_bBlockMouseLook; // FPS-only
    QPointF m_pointLastMousePos;

    // Picking mode
    bool m_bPickMode;
    SelectType m_ePrevSelectType;

    // Selection
    bool m_bDragSelect;
    QPointF m_pointDragStartPos;
    QPointF m_pointDragCurrentPos; // updated while dragging
    int m_iSelectRadius; // pixels

    // Marquee appearance
    QColor m_colorMarquee; // semi-transparent
    QColor m_colorMarqueeBorder; // opaque border
    float m_fMarqueeBorderWidth; // px

    // Mesh cache for picking & framing
    std::vector<Element> m_elements; // original elements (ids, topology, colors)
    std::unordered_map<int, QVector3D> m_mapNodeIdPos;

public:
    bool convertWorldToScreen(const HViewCamera *pCamera, const QVector3D &vWorldPosition, QVector3D &vScreenPosition) const;
    bool convertScreenToWorld(const HViewCamera *pCamera, const QVector3D &vScreenPosition, QVector3D &vWorldPosition) const;
};

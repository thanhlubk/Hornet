#pragma once
#include "HornetViewExport.h"
#include <QOpenGLWindow>
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
#include "HRenderConstraint.h"
#include "HViewSelectionManager.h"
#include "HViewDef.h"
#include "HornetBase/NotifyDispatcher.h"
#include "HornetBase/DatabaseSession.h"

class HORNETVIEW_EXPORT GLViewWindow : public QOpenGLWindow, protected QOpenGLExtraFunctions
{
    Q_OBJECT
public:
    explicit GLViewWindow(QWindow *parent = nullptr);
    ~GLViewWindow() override;

    // Helper to get a QWidget wrapper that you can put into layouts/FViewHost
    QWidget *createContainer(QWidget *parent = nullptr);

    void setDatabase(DatabaseSession *pDb);

    // Result display controls.
    // step == 0 means show the undeformed model with no result contour.
    void setStep(int step);
    int step() const;

    // LBC visibility control. This does not rebuild or reload LBC data; it only toggles drawing.
    void setShowLbc(bool show);
    bool showLbc() const;

    // Coordinate-gizmo visibility control. This only toggles drawing.
    void setShowCoordinate(bool show);
    bool showCoordinate() const;

    // Mesh visibility controls. These do not rebuild the database mesh;
    // they only toggle the renderer passes.
    void setShowMeshLine(bool show);
    bool showMeshLine() const;

    void setShowNode(bool show);
    bool showNode() const;

    void setShowDeformation(bool show);
    bool showDeformation() const;

    void setShowResultComponent(bool show);
    bool showResultComponent() const;

    // Multiplier for deformation visualization. Default is 1000.0.
    void setScale(double scale);
    double scale() const;

    void setResultComponent(int component);
    int resultComponent() const;

    void setNotifyDispatcher(NotifyDispatcher &disp);
    void onNotify(MessageType mess, MessageParam a, MessageParam b);


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
    HRenderConstraint* constraintRenderer();

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
    DatabaseSession* m_pDb;

    int m_iResultStep;
    bool m_bShowDeformation;
    bool m_bShowResultComponent;
    bool m_bShowLbc;
    bool m_bShowCoordinate;
    bool m_bAutoScale;
    double m_dDeformationScale;
    int m_iResultComponent;
    bool m_bHasResultRange;
    double m_dResultMin;
    double m_dResultMax;
    std::string m_strResultComponentName;

    void destroyGLObjects();
    void rebuildFromDatabase();
    void drawResultComponentLegend();
    bool getHitPosition(const QPointF &point, QVector3D &hit, int &elemId) const;
    void selectAtPoint(const QPointF &point);
    void selectInRect(const QRectF &rectMarquee);

    // Render
    HRenderModel *m_pRenderModel;
    HRenderCoordinateGizmo* m_pRenderCoordinate;
    HRenderForce* m_pRenderForce;
    HRenderConstraint* m_pRenderConstraint;

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
    std::vector<RenderElementData> m_elements;
    std::unordered_map<int, QVector3D> m_mapNodeIdPos;

public:
    bool convertWorldToScreen(const HViewCamera *pCamera, const QVector3D &vWorldPosition, QVector3D &vScreenPosition) const;
    bool convertScreenToWorld(const HViewCamera *pCamera, const QVector3D &vScreenPosition, QVector3D &vWorldPosition) const;
};

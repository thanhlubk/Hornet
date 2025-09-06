#pragma once
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>
#include <QTimer>
#include "HViewDef.h"
#include "HornetViewExport.h"

class HORNETVIEW_EXPORT HViewCamera : public QObject
{
    Q_OBJECT
public:
    HViewCamera(QObject *parent = nullptr);

    enum MovementDir
    {
        None = 0,
        Forward = 0x000001,
        Backward = 0x000002,
        Right = 0x000004,
        Left = 0x000008,
        Up = 0x000010,
        Down = 0x000020,
    };
    Q_ENUM(MovementDir)

    void setProjection(Projection p);
    Projection projection() const;

    // Perspective & Ortho params (kept identical to your GLViewWindow fields)
    void setFovDegrees(float d);
    float fovDegrees() const;

    void setOrthoHeight(float h);
    float orthoHeight() const;

    void setNearFarPlane(float zn, float zf);
    float nearPlane() const;
    float farPlane() const;

    void setControlMode(ControlMode m);
    ControlMode controlMode() const;

    // Orientation / position / focus
    void setFocus(const QVector3D &p);
    QVector3D focus() const;

    void setPose(const QVector3D &pos, const QQuaternion &q);

    void recenterToFocus();
    const QVector3D &position() const;
    const QQuaternion &rotation() const;

    // Speeds
    void setOrbitRotateSpeed(float s);
    void setOrbitPanSpeed(float s);
    void setFPSMoveSpeed(float s);
    void setFPSRotateSpeed(float s);
    void setZoomStep(float s);
    float zoomStep() const;

    // Matrices
    QMatrix4x4 viewMatrix() const;
    QMatrix4x4 projectionMatrix(int w, int h) const;

    // Frame model bounds (same idea as your fitCamera)
    void frameBounds(const QVector3D &center, float radius, int w, int h);
    
    // Arcball orbit (MMB drag)
    void orbitArcball(const QPointF &prevPx, const QPointF &currPx, int w, int h);

    // Pan in camera plane (RMB drag)
    void orbitPan(float dx, float dy, int viewportH);

    // Wheel zoom
    void orbitZoomSteps(float steps);

    // FPS look (RMB or always-look)
    void fpsLook(float dx, float dy);
    void fpsMove();

    void move(MovementDir dir);
    void stop(MovementDir dir);

signals:
    void viewChanged();

private:
    Projection m_eProjection;
    ControlMode m_eControl;

    // pose
    QVector3D m_vCameraPosition;
    QQuaternion m_quaternion;

    // frustum
    float m_fFOVDegree;
    float m_fOrthoHeight;
    float m_fNearPlane, m_fFarPlane;

    // orbit target
    QVector3D m_vFocusPosition;
    float m_fFrameDistance;

    // speeds
    float m_fOrbitRotateSpeed;
    float m_fOrbitPanSpeed;
    float m_fFPSMoveSpeed;
    float m_fFPSRotateSpeed;
    float m_fZoomStep;

    // movement state
    ushort m_eMoveDirection;
    QTimer m_timer;
};

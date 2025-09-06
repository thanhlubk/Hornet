#include <HornetView/HViewCamera.h>

HViewCamera::HViewCamera(QObject *parent)
    : QObject(parent)
{
    m_eProjection = Projection::Orthographic;
    m_eControl = ControlMode::OrbitPan;

    m_vCameraPosition = QVector3D(0, 0, 5);

    m_fFOVDegree = 45.f;
    m_fOrthoHeight = 4.f;
    m_fNearPlane = 0.001f;
    m_fFarPlane = 1e6f;

    m_vFocusPosition = QVector3D(0, 0, 0);
    m_fFrameDistance = 3.f;

    m_fOrbitRotateSpeed = 1.f;
    m_fOrbitPanSpeed = 1.f;
    m_fFPSMoveSpeed = 2.f;
    m_fFPSRotateSpeed = 0.15f;
    m_fZoomStep = 0.2f;

    m_eMoveDirection = MovementDir::None;

    m_timer.setInterval(16); // ~60 FPS
    QObject::connect(&m_timer, &QTimer::timeout, [this]() { fpsMove(); });
}

void HViewCamera::setProjection(Projection p)
{
    m_eProjection = p;
    emit viewChanged();
}

Projection HViewCamera::projection() const
{ 
    return m_eProjection; 
}

void HViewCamera::setFovDegrees(float d)
{
    m_fFOVDegree = std::clamp(d, 10.f, 140.f);
    emit viewChanged();
}

float HViewCamera::fovDegrees() const
{ 
    return m_fFOVDegree; 
}

void HViewCamera::setOrthoHeight(float h)
{
    m_fOrthoHeight = std::max(1e-4f, h);
    emit viewChanged();
}

float HViewCamera::orthoHeight() const
{ 
    return m_fOrthoHeight; 
}

void HViewCamera::setNearFarPlane(float zn, float zf)
{
    m_fNearPlane = std::max(1e-6f, zn);
    m_fFarPlane = std::max(m_fNearPlane * 2.f, zf);
    emit viewChanged();
}

float HViewCamera::nearPlane() const
{ 
    return m_fNearPlane; 
}

float HViewCamera::farPlane() const
{ 
    return m_fFarPlane; 
}

void HViewCamera::setControlMode(ControlMode m)
{
    m_eControl = m;
    emit viewChanged();
}

ControlMode HViewCamera::controlMode() const
{ 
    return m_eControl; 
}

// Orientation / position / focus
void HViewCamera::setFocus(const QVector3D &p)
{
    m_vFocusPosition = p;
    emit viewChanged();
}

QVector3D HViewCamera::focus() const
{ 
    return m_vFocusPosition; 
}

void HViewCamera::setPose(const QVector3D &pos, const QQuaternion &q)
{
    m_vCameraPosition = pos;
    m_quaternion = q;
}

void HViewCamera::recenterToFocus()
{
    float dist = (m_vCameraPosition - m_vFocusPosition).length();
    if (dist <= 1e-6f)
        dist = std::max(1.f, m_fFrameDistance);
    const QVector3D fwd = m_quaternion.rotatedVector(QVector3D(0, 0, -1));
    m_vCameraPosition = m_vFocusPosition - fwd * dist;
    emit viewChanged();
}

const QVector3D &HViewCamera::position() const
{ 
    return m_vCameraPosition; 
}

const QQuaternion &HViewCamera::rotation() const
{ 
    return m_quaternion; 
}

void HViewCamera::setOrbitRotateSpeed(float s)
{
    m_fOrbitRotateSpeed = std::max(0.01f, s);
}

void HViewCamera::setOrbitPanSpeed(float s)
{ 
    m_fOrbitPanSpeed = std::max(0.01f, s); 
}

void HViewCamera::setFPSMoveSpeed(float s)
{ 
    m_fFPSMoveSpeed = std::max(0.01f, s); 
}

void HViewCamera::setFPSRotateSpeed(float s)
{ 
    m_fFPSRotateSpeed = std::max(0.001f, s); 
}

void HViewCamera::setZoomStep(float s)
{ 
    m_fZoomStep = std::clamp(s, 0.01f, 0.9f); 
}

float HViewCamera::zoomStep() const
{ 
    return m_fZoomStep; 
}

QMatrix4x4 HViewCamera::viewMatrix() const
{
    const QVector3D fwd = m_quaternion.rotatedVector(QVector3D(0, 0, -1));
    const QVector3D up = m_quaternion.rotatedVector(QVector3D(0, 1, 0));
    QMatrix4x4 V;
    V.lookAt(m_vCameraPosition, m_vCameraPosition + fwd, up);
    return V;
}

QMatrix4x4 HViewCamera::projectionMatrix(int w, int h) const
{
    QMatrix4x4 P;
    const float aspect = (h > 0) ? float(w) / float(h) : 1.f;
    if (m_eProjection == Projection::Perspective)
        P.perspective(m_fFOVDegree, aspect, m_fNearPlane, m_fFarPlane);
    else
    {
        float hh = 0.5f * m_fOrthoHeight, hw = hh * aspect;
        P.ortho(-hw, hw, -hh, hh, m_fNearPlane, m_fFarPlane);
    }
    return P;
}

void HViewCamera::frameBounds(const QVector3D &center, float radius, int w, int h)
{
    const float fovRad = qDegreesToRadians(m_fFOVDegree);
    m_fFrameDistance = radius / std::tan(0.5f * fovRad) * 1.2f;
    m_fNearPlane = std::max(0.001f, m_fFrameDistance - 4.0f * radius);
    m_fFarPlane = std::max(10.0f, m_fFrameDistance + 4.0f * radius);

    // reset pose: on +Z looking at center
    m_quaternion = QQuaternion();
    m_vCameraPosition = QVector3D(center.x(), center.y(), center.z() + m_fFrameDistance);
    m_vFocusPosition = center;

    m_fOrthoHeight = 2.4f * radius; // same heuristic
    (void)w;
    (void)h; // aspect handled in projectionMatrix()
}

void HViewCamera::orbitArcball(const QPointF &prevPx, const QPointF &currPx, int w, int h)
{
    auto proj = [](const QPointF &p, int W, int H) -> QVector3D
    {
        float x = (2.f * p.x() - W) / float(W);
        float y = (H - 2.f * p.y()) / float(H);
        float z2 = 1.f - x * x - y * y;
        float z = z2 > 0.f ? std::sqrt(z2) : 0.f;
        QVector3D v(x, y, z);
        if (!v.isNull())
            v.normalize();
        return v;
    };
    const QVector3D v0 = proj(prevPx, w, h);
    const QVector3D v1 = proj(currPx, w, h);
    const QVector3D axis = QVector3D::crossProduct(v0, v1);
    const float dot = std::clamp(QVector3D::dotProduct(v0, v1), -1.f, 1.f);
    const float angleDeg = qRadiansToDegrees(std::acos(dot)) * m_fOrbitRotateSpeed;
    if (axis.lengthSquared() < 1e-12f || angleDeg <= 1e-4f)
        return;
    QQuaternion dq = QQuaternion::fromAxisAndAngle(axis.normalized(), angleDeg);
    m_quaternion = dq * m_quaternion;
    m_quaternion.normalize();

    // keep distance to focus
    float dist = (m_vCameraPosition - m_vFocusPosition).length();
    if (dist <= 1e-6f)
        dist = std::max(1.f, m_fFrameDistance);
    const QVector3D fwd = m_quaternion.rotatedVector(QVector3D(0, 0, -1));
    m_vCameraPosition = m_vFocusPosition - fwd * dist;
}

void HViewCamera::orbitPan(float dx, float dy, int viewportH)
{
    float dist = (m_vCameraPosition - m_vFocusPosition).length();
    if (dist <= 1e-6f)
        dist = std::max(1.f, m_fFrameDistance);
    float hWorld = (m_eProjection == Projection::Perspective)
                       ? 2.f * dist * std::tan(qDegreesToRadians(0.5f * m_fFOVDegree))
                       : m_fOrthoHeight;
    const float worldPerPixel = (hWorld / std::max(1, viewportH)) * m_fOrbitPanSpeed;

    const QVector3D right = m_quaternion.rotatedVector(QVector3D(1, 0, 0));
    const QVector3D up = m_quaternion.rotatedVector(QVector3D(0, 1, 0));
    const QVector3D pan = (-dx * worldPerPixel) * right + (dy * worldPerPixel) * up;
    m_vCameraPosition += pan;
    m_vFocusPosition += pan;
}

void HViewCamera::orbitZoomSteps(float steps)
{
    float dist = (m_vCameraPosition - m_vFocusPosition).length();
    if (dist <= 1e-6f)
        dist = std::max(1.f, m_fFrameDistance);
    float scale = std::pow(1.f - std::clamp(m_fZoomStep, 0.01f, 0.9f), steps);
    float newDist = std::clamp(dist * (1.f / scale), 0.05f, 1e6f);
    const QVector3D fwd = m_quaternion.rotatedVector(QVector3D(0, 0, -1));
    m_vCameraPosition = m_vFocusPosition - fwd * newDist;
}

void HViewCamera::fpsLook(float dx, float dy)
{
    const QVector3D up = m_quaternion.rotatedVector(QVector3D(0, 1, 0));
    const QVector3D right = m_quaternion.rotatedVector(QVector3D(1, 0, 0));
    QQuaternion yawQ = QQuaternion::fromAxisAndAngle(up, dx * m_fFPSRotateSpeed);
    QQuaternion pitchQ = QQuaternion::fromAxisAndAngle(right, -dy * m_fFPSRotateSpeed);
    m_quaternion = (yawQ * pitchQ) * m_quaternion;
    m_quaternion.normalize();
}

void HViewCamera::fpsMove()
{
    float dt = 1;
    dt = std::min(dt, 0.05f); // avoid large jumps
    if (m_eMoveDirection == MovementDir::None || m_eControl != ControlMode::FPS)
    {
        m_timer.stop();
        return;
    }
    QVector3D fwd = m_quaternion.rotatedVector(QVector3D(0, 0, -1));
    QVector3D right = m_quaternion.rotatedVector(QVector3D(1, 0, 0));
    QVector3D up = m_quaternion.rotatedVector(QVector3D(0, 1, 0));
    QVector3D move(0, 0, 0);
    if (m_eMoveDirection & MovementDir::Forward)
        move += fwd;
    if (m_eMoveDirection & MovementDir::Backward)
        move -= fwd;
    if (m_eMoveDirection & MovementDir::Right)
        move += right;
    if (m_eMoveDirection & MovementDir::Left)
        move -= right;
    if (m_eMoveDirection & MovementDir::Up)
        move += up;
    if (m_eMoveDirection & MovementDir::Down)
        move -= up;
    if (!move.isNull())
        m_vCameraPosition += move.normalized() * (m_fFPSMoveSpeed * dt * std::max(0.5f, m_fFrameDistance));
}

void HViewCamera::move(MovementDir dir)
{
    if (m_eControl != ControlMode::FPS)
        return;
    m_eMoveDirection |= dir;
    if (m_timer.isActive() == false && m_eMoveDirection != MovementDir::None)
        m_timer.start();
}

void HViewCamera::stop(MovementDir dir)
{
    if (m_eControl != ControlMode::FPS)
        return;
    m_eMoveDirection &= ~dir;
    if (m_timer.isActive() == true && m_eMoveDirection == MovementDir::None)
        m_timer.stop();
}

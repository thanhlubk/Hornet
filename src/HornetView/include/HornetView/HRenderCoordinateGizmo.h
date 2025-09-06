// CoordinateGizmo.h
#pragma once
#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>
#include "HornetViewExport.h"

class HORNETVIEW_EXPORT HRenderCoordinateGizmo : public QObject, protected QOpenGLExtraFunctions
{
    Q_OBJECT
public:
    HRenderCoordinateGizmo(QObject *parent = nullptr);
    void initialize();
    void destroy();

    // bottom-left mini-viewport; size & margin in logical px
    void setSize(int pixel);
    void setMargin(int pixel);
    void setScale(float scale);
    void draw(const QQuaternion &cameraQ, int fbWidth, int fbHeight);

signals:
    void dataChanged(); // if you add properties that affect rendering, emit this when they change 

private:
    void buildArrowCoordinate();

    QOpenGLShaderProgram m_shaderProgram;
    GLuint m_vao, m_vbo, m_ebo;
    GLsizei m_iTriCount;

    int m_iSize;
    int m_iMargin;
    float m_fScale;
};

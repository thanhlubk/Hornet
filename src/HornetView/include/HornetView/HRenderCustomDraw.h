#pragma once

#include "HornetViewExport.h"
#include <QColor>
#include <QMatrix4x4>
#include <QObject>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QVector3D>
#include <QVector4D>
#include <cstdint>
#include <vector>

class HORNETVIEW_EXPORT HRenderCustomDraw : public QObject, protected QOpenGLExtraFunctions
{
    Q_OBJECT
public:
    explicit HRenderCustomDraw(QObject *parent = nullptr);

    void initialize();
    void destroy();

    // Draw all custom primitives currently stored in this renderer.
    // Call this from GLViewWindow::paintGL() after the model pass.
    void draw(const QMatrix4x4 &P, const QMatrix4x4 &V);

    // Adds one custom point and returns a stable primitive id.
    // The primitive stays until clearCustomPoint(id), clearCustomPoints(), or clear() is called.
    int drawCustomPoint3D(const QVector3D &position);
    int drawCustomPoint3D(const QVector3D &position, const QColor &color);
    int drawCustomPoint3D(const QVector3D &position, const QColor &color, float pointSize);

    // Adds one custom line segment and returns a stable primitive id.
    // The primitive stays until clearCustomLine(id), clearCustomLines(), or clear() is called.
    int drawCustomLine3D(const QVector3D &p0, const QVector3D &p1);
    int drawCustomLine3D(const QVector3D &p0, const QVector3D &p1, const QColor &color);
    int drawCustomLine3D(const QVector3D &p0, const QVector3D &p1, const QColor &color, float lineWidth);

    void clear();
    void clearCustomPoints();
    void clearCustomLines();

    // Remove one primitive by id. Return true if something was removed.
    bool clearCustomPoint(int id);
    bool clearCustomLine(int id);
    bool clearCustomPrimitive(int id);

    // Defaults are still used by overloads that do not pass pointSize/lineWidth.
    void setPointSize(float pixel);
    float pointSize() const;

    void setLineWidth(float pixel);
    float lineWidth() const;

    void setDefaultPointColor(const QColor &color);
    QColor defaultPointColor() const;

    void setDefaultLineColor(const QColor &color);
    QColor defaultLineColor() const;

    void setShowPoints(bool show);
    bool showPoints() const;

    void setShowLines(bool show);
    bool showLines() const;

    int pointCount() const;
    int lineCount() const;

signals:
    void settingChanged();
    void dataChanged();

private:
    struct CustomPoint
    {
        int id = -1;
        QVector3D position;
        QVector4D color;
        float size = 1.0f;
    };

    struct CustomLine
    {
        int id = -1;
        QVector3D p0;
        QVector3D p1;
        QVector4D color;
        float width = 1.0f;
    };

    static QVector4D toVec4(const QColor &color);
    static QVector3D toVec3(const QVector4D &color);
    static float clampPixel(float pixel);

    int nextPrimitiveId();

    void uploadPoints();
    void uploadLines();
    void drawPoints(const QMatrix4x4 &P, const QMatrix4x4 &V);
    void drawLines(const QMatrix4x4 &P, const QMatrix4x4 &V);

private:
    QOpenGLShaderProgram m_pointShaderProgram;
    QOpenGLShaderProgram m_lineShaderProgram;

    GLuint m_pointVao;
    GLuint m_pointVboPosition;
    GLuint m_pointVboColor;
    GLuint m_pointVboSize;

    GLuint m_lineVao;
    GLuint m_lineVboPosition;
    GLuint m_lineVboColor;
    GLuint m_lineEbo;

    GLsizei m_iPointCount;
    GLsizei m_iLineVertexCount;
    GLsizei m_iLineIndexCount;

    bool m_bInitialized;
    bool m_bDirtyPoints;
    bool m_bDirtyLines;
    bool m_bShowPoints;
    bool m_bShowLines;

    int m_iNextPrimitiveId;

    float m_fPointSize;
    float m_fLineWidth;
    QVector4D m_vDefaultPointColor;
    QVector4D m_vDefaultLineColor;

    std::vector<CustomPoint> m_vecPoints;
    std::vector<CustomLine> m_vecLines;

    std::vector<QVector3D> m_vecPointPositions;
    std::vector<QVector4D> m_vecPointColors;
    std::vector<float> m_vecPointSizes;

    std::vector<QVector3D> m_vecLinePositions;
    std::vector<QVector4D> m_vecLineColors;
    std::vector<uint32_t> m_vecLineIndices;
    std::vector<float> m_vecLineWidths;
};

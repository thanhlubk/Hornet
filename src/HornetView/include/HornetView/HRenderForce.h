#pragma once
#include <vector>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>
#include <unordered_map>
#include "HViewDef.h"
#include "HornetViewExport.h"
#include "HViewLighting.h"
#include "HViewCamera.h"

class HORNETVIEW_EXPORT HRenderForce : public QObject, protected QOpenGLExtraFunctions
{
    Q_OBJECT

public:
    explicit HRenderForce(QObject *parent = nullptr);
    void initialize();
    void destroy();

    void setForces(const std::vector<ForceArrow> &forces);
    // convenience for GLViewWindow’s ForceArrow without changing its public API
    template <class ForceArrowContainer>
    void setForcesCompat(const ForceArrowContainer &exts);

    const std::vector<ForceArrow> &forces() const { return m_vecForces; }

    void setConstantScreenSize(bool on);
    void setBasePixelSize(float px);

    bool constantScreenSize() const;
    float basePixelSize() const;

    // Draw. If constant-screen-size is on, we rebuild every frame to match the camera.
    void draw(const QMatrix4x4 &P, const QMatrix4x4 &V, const HViewLighting &lighting, int viewportW, int viewportH);

    // --- Selection overlay (highlight chosen force IDs) ---
    void setSelection(const std::vector<int> &forceIds, const QColor &color, float alpha);
    void clearSelection();

signals:
    void dataChanged();

private:
    // helpers
    void ensureGL();
    void rebuild(const QMatrix4x4 &P, const QMatrix4x4 &V, int viewportW, int viewportH);
    float worldPerPixelAt(const QMatrix4x4 &VP, const QMatrix4x4 &invVP, int viewportH, const QVector3D &world) const;

    // GPU
    QOpenGLShaderProgram m_shaderProgram;
    GLuint m_vao = 0, m_vboPosition = 0, m_vboNormal = 0, m_vboColor = 0, m_ebo = 0;
    bool m_bInitialize = false;

    // data
    std::vector<ForceArrow> m_vecForces;
    bool m_bConstantScreenSize = true;
    bool m_bRebuild = true;
    float m_fSize = 80.0f;

    // selection state
    std::vector<int> m_vecSelectionIds;
    QVector3D m_vSelectionColor {1.f, 0.5f, 0.f};
    float m_fSelectionAlpha = 0.45f;
    bool m_bIsSelected = false;

    // per-force index ranges gathered during rebuild()
    // stored relative to the *local* lit/unlit buffers; adjusted at draw time
    std::unordered_map<int, std::vector<std::pair<GLsizei,GLsizei>>> m_mapRangesLighting;
    std::unordered_map<int, std::vector<std::pair<GLsizei,GLsizei>>> m_mapRangesUnlighting;
    GLsizei m_iLightingCount = 0, m_iUnlightingCount = 0;
};

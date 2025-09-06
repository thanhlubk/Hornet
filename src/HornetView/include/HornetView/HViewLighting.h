#pragma once
#include <QColor>
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include "HornetViewExport.h"

class HORNETVIEW_EXPORT HViewLighting : public QObject
{
    Q_OBJECT
public:
    HViewLighting(QObject *parent = nullptr);

    // toggles
    void enableAmbient(bool enable);
    void enableDiffuse(bool enable);
    void enableSpecular(bool enable);

    bool ambientEnabled() const;
    bool diffuseEnabled() const;
    bool specularEnabled() const;

    // colors
    void setAmbientColor(const QColor &color);
    void setDiffuseColor(const QColor &color);
    void setSpecularColor(const QColor &color);

    QColor ambientColor() const;
    QColor diffuseColor() const;
    QColor specularColor() const;

    // intensities
    void setAmbientIntensity(float value);
    void setDiffuseIntensity(float value);
    void setSpecularIntensity(float value);
    void setShininess(float value);

    float ambientIntensity() const;
    float diffuseIntensity() const;
    float specularIntensity() const;
    float shininess() const;

    // Bind all uniforms expected by our shaders
    void applyTo(QOpenGLShaderProgram &shaderProgram) const;

signals:
    void lightingChanged();

private:
    bool m_bEnableAmbient, m_bEnableDiffuse, m_bEnableSpecular;
    QColor m_colorAmbient;
    QColor m_colorDiffuse;
    QColor m_colorSpecular;
    float m_fAmbientIntensity, m_fDiffuseIntensity, m_fSpecularIntensity, m_fShininess;
};

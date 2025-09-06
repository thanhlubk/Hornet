#include <HornetView/HViewLighting.h>
#include <QtMath>

// tiny helper
static QVector3D rgb(const QColor &c) 
{ 
    return {c.redF(), c.greenF(), c.blueF()}; 
}

HViewLighting::HViewLighting(QObject *parent) 
    : QObject(parent) 
{
    m_bEnableAmbient = true;
    m_bEnableDiffuse = true;
    m_bEnableSpecular = true;

    m_colorAmbient = QColor::fromRgbF(1, 1, 1);
    m_colorDiffuse = QColor::fromRgbF(1, 1, 1);
    m_colorSpecular = QColor::fromRgbF(1, 1, 1);

    m_fAmbientIntensity = 0.20f;
    m_fDiffuseIntensity = 1.0f;
    m_fSpecularIntensity = 0.40f;
    m_fShininess = 32.0f;
}

void HViewLighting::enableAmbient(bool enable)
{
    m_bEnableAmbient = enable;
    emit lightingChanged();
}

void HViewLighting::enableDiffuse(bool enable)
{
    m_bEnableDiffuse = enable;
    emit lightingChanged();
}

void HViewLighting::enableSpecular(bool enable)
{
    m_bEnableSpecular = enable;
    emit lightingChanged();
}

bool HViewLighting::ambientEnabled() const 
{ 
    return m_bEnableAmbient; 
}

bool HViewLighting::diffuseEnabled() const 
{ 
    return m_bEnableDiffuse; 
}

bool HViewLighting::specularEnabled() const 
{ 
    return m_bEnableSpecular; 
}

void HViewLighting::setAmbientColor(const QColor &color)
{
    m_colorAmbient = color;
    emit lightingChanged();
}

void HViewLighting::setDiffuseColor(const QColor &color)
{
    m_colorDiffuse = color;
    emit lightingChanged();
}

void HViewLighting::setSpecularColor(const QColor &color)
{
    m_colorSpecular = color;
    emit lightingChanged();
}

QColor HViewLighting::ambientColor() const 
{ 
    return m_colorAmbient; 
}

QColor HViewLighting::diffuseColor() const 
{ 
    return m_colorDiffuse;
}

QColor HViewLighting::specularColor() const 
{ 
    return m_colorSpecular; 
}

void HViewLighting::setAmbientIntensity(float value)
{
    m_fAmbientIntensity = value;
    emit lightingChanged();
}
void HViewLighting::setDiffuseIntensity(float value)
{
    m_fDiffuseIntensity = value;
    emit lightingChanged();
}

void HViewLighting::setSpecularIntensity(float value)
{
    m_fSpecularIntensity = value;
    emit lightingChanged();
}

void HViewLighting::setShininess(float value)
{
    m_fShininess = value;
    emit lightingChanged();
}

float HViewLighting::ambientIntensity() const 
{ 
    return m_fAmbientIntensity; 
}

float HViewLighting::diffuseIntensity() const 
{ 
    return m_fDiffuseIntensity; 
}

float HViewLighting::specularIntensity() const 
{
    return m_fSpecularIntensity; 
}

float HViewLighting::shininess() const 
{ 
    return m_fShininess; 
}

// Bind all uniforms expected by our shaders
void HViewLighting::applyTo(QOpenGLShaderProgram &shaderProgram) const
{
    shaderProgram.setUniformValue("uAmbOn", m_bEnableAmbient);
    shaderProgram.setUniformValue("uDifOn", m_bEnableDiffuse);
    shaderProgram.setUniformValue("uSpeOn", m_bEnableSpecular);
    shaderProgram.setUniformValue("uAmbColor", rgb(m_colorAmbient));
    shaderProgram.setUniformValue("uDifColor", rgb(m_colorDiffuse));
    shaderProgram.setUniformValue("uSpeColor", rgb(m_colorSpecular));
    shaderProgram.setUniformValue("uAmbI", m_fAmbientIntensity);
    shaderProgram.setUniformValue("uDifI", m_fDiffuseIntensity);
    shaderProgram.setUniformValue("uSpeI", m_fSpecularIntensity);
    shaderProgram.setUniformValue("uShininess", m_fShininess);
}
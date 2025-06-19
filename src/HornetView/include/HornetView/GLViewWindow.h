#pragma once

#include "HornetViewExport.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class HORNETVIEW_EXPORT GLViewWindow : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit GLViewWindow(QWidget* parent = nullptr);
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void loadShaders();
};

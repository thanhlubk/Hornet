#include <gl/glew.h>
#include "HornetView/GLViewWindow.h"
#include <QOpenGLShaderProgram>
#include <QFile>
#include <glm/glm.hpp>

GLViewWindow::GLViewWindow(QWidget* parent) : QOpenGLWidget(parent) {}

void GLViewWindow::initializeGL() {
    GLenum err = glewInit();
	if (err != GLEW_OK) {
		// Show error
		return;
	}
    initializeOpenGLFunctions();
    loadShaders();
}

void GLViewWindow::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void GLViewWindow::paintGL() {
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLViewWindow::loadShaders() {
    // Placeholder: later we'll load shaders from files
}

#ifndef QDMOPENGLVIEWPORT_H
#define QDMOPENGLVIEWPORT_H

#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <memory>

class QDMOpenGLViewport : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

    std::unique_ptr<QOpenGLShaderProgram> m_program;
    std::unique_ptr<QOpenGLVertexArrayObject> m_vao;

public:
    explicit QDMOpenGLViewport(QWidget *parent = nullptr);

    virtual QSize sizeHint() const override;
    virtual void initializeGL() override;
    virtual void resizeGL(int nx, int ny) override;
    virtual void paintGL() override;
};

#endif // QDMOPENGLVIEWPORT_H

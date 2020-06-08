# Qt Examples

- qmdkplay(QMDKWindow class): the simplest example, but should be good enough for most cases. Change QOpenGLWindow to QOpenGLWidget if needed.
- qmlrhi: qt5.15 and qt6 RHI example. Currently supports OpenGL, D3D11 and Metal
- qml: render to opengl framebuffer object

To display log in qtcreator, call `setLogHandler()` to redirect log message to `qDebug()`
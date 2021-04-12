# Qt Examples

- qmdkplay(QMDKWindow class): the simplest example, but should be good enough for most cases. Change QOpenGLWindow to QOpenGLWidget if needed.
- **qmlrhi**: **Recommended for QtQuick**. support qt>=5.14/qt6 RHI, and qt<5.14 via direct OpenGL. Currently supports Vulkan, OpenGL, D3D11 and Metal
- qml: render to opengl framebuffer object

To display log in qtcreator, call `setLogHandler()` to redirect log message to `qDebug()`
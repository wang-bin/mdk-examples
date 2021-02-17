media player examples based on mdk sdk. runs on all platforms. [Download the latest sdk](https://sourceforge.net/projects/mdk-sdk/files/nightly/)

About libmdk sdk: https://github.com/wang-bin/mdk-sdk

## GLFW
### glfwplay
- has the most features
- use glfw context as foreign context mode
- use glfw window to create render loop and gl/d3d11/metal/vulkan context internally

### multiplayers
1 player per window

### multiwindows
many windows as render targets sharing the same player

### PlayListAsOne

Gapless play/seek multiple videos like a single video

### simple
minimal glfw example

## macOS
- use `CAOpenGLLayer` as foreign context
- use NSView to create render loop and gl context internally

## iOS
Legacy Obj-C OpenGLES2 example

## Apple
- HelloTriangle: Using mdk sdk to play video in Apple's official example https://developer.apple.com/documentation/metal/using_a_render_pipeline_to_render_primitives?language=objc
- OCPlayer: macOS only. via cocoapods
- SwiftMdkExample: supports macOS, iOS, macCatalyst. via submodule [swiftMDK](https://github.com/wang-bin/swiftMDK). requires mdk.xcframeworks
- SwiftPlayer: macOS only. via cocoapods

## Native
examples without platform specific code
- mdkplay: a mininal player with internally created platform dependent window/surface and opengl context, also create render loop and gl context
- DecodeFps: a tool to test decoder performance. `./DecodeFps [-c:v DecoderName] file"
- offscreen: offscreen rendering guide

## Qt
- Integrate with QOpenGLWindow and QOpenGLWidget. Use gl context provided by Qt.
- QML example via QOpenGLFrameBuffer
- RHI example: vulkan, d3d11, metal, opengl

## SDL
Use gl context provided by SDL

## SFML
- Use gl context provided by SFML
- Render to SFML texture

## WindowsStore
UWP example, supports D3D11, and ANGLE GLES2

### CoreWindowCx
- use `Windows::UI::Core::CoreWindow` as platform dependent surface to create render loop and gl context internally
- use App provided gl context

### XamlCx
- Use `Windows::UI::Xaml::Controls::SwapChainPanel` as platform dependent surface to create render loop and gl context internally
- Switch between 2 render targets

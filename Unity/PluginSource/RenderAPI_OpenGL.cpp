/*
 * Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
#include "MDKUnityPlugin.h"

#if defined(_WIN32)
#include <windows.h>
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#include <OpenGLES/ES3/gl.h>
#else
#include <OpenGL/gl3.h>
#endif
#elif defined(__ANDROID__)
#include <GLES3/gl3.h>
#elif defined(__linux__)
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#if defined(_WIN32) || defined(__APPLE__) || defined(__ANDROID__) || defined(__linux__)

namespace {

constexpr GLenum kFramebuffer = 0x8D40;
constexpr GLenum kFramebufferBinding = 0x8CA6;
constexpr GLenum kColorAttachment0 = 0x8CE0;
constexpr GLenum kFramebufferComplete = 0x8CD5;
constexpr GLenum kDrawFramebuffer = 0x8CA9;
constexpr GLenum kReadFramebuffer = 0x8CA8;
constexpr GLenum kReadFramebufferBinding = 0x8CAA;

#if defined(_WIN32)
struct StateFunctions {
    void (APIENTRY* UseProgram)(GLuint) = nullptr;
    void (APIENTRY* BindVertexArray)(GLuint) = nullptr;
    void (APIENTRY* BindBuffer)(GLenum, GLuint) = nullptr;
    void (APIENTRY* ActiveTexture)(GLenum) = nullptr;
    void (APIENTRY* BlendFuncSeparate)(GLenum, GLenum, GLenum, GLenum) = nullptr;
    void (APIENTRY* BlendEquationSeparate)(GLenum, GLenum) = nullptr;
};
StateFunctions g_state;
#define GL_STATE_FUNCTION(name) g_state.name
#else
#define GL_STATE_FUNCTION(name) gl##name
#endif

#if defined(_WIN32)
using GenFramebuffersProc = void (APIENTRY*)(GLsizei, GLuint*);
using BindFramebufferProc = void (APIENTRY*)(GLenum, GLuint);
using FramebufferTexture2DProc = void (APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint);
using CheckFramebufferStatusProc = GLenum (APIENTRY*)(GLenum);
using DeleteFramebuffersProc = void (APIENTRY*)(GLsizei, const GLuint*);

struct FramebufferFunctions {
    GenFramebuffersProc gen = nullptr;
    BindFramebufferProc bind = nullptr;
    FramebufferTexture2DProc attach = nullptr;
    CheckFramebufferStatusProc check = nullptr;
    DeleteFramebuffersProc del = nullptr;
};

FramebufferFunctions g_framebuffer;

template<typename Function>
Function ResolveGLFunction(const char* name)
{
    const auto proc = wglGetProcAddress(name);
    if (proc && proc != reinterpret_cast<PROC>(1) && proc != reinterpret_cast<PROC>(2)
        && proc != reinterpret_cast<PROC>(3) && proc != reinterpret_cast<PROC>(-1))
        return reinterpret_cast<Function>(proc);
    return nullptr;
}

bool LoadFramebufferFunctions()
{
    if (g_framebuffer.gen)
        return true;
    g_framebuffer.gen = ResolveGLFunction<GenFramebuffersProc>("glGenFramebuffers");
    g_framebuffer.bind = ResolveGLFunction<BindFramebufferProc>("glBindFramebuffer");
    g_framebuffer.attach = ResolveGLFunction<FramebufferTexture2DProc>("glFramebufferTexture2D");
    g_framebuffer.check = ResolveGLFunction<CheckFramebufferStatusProc>("glCheckFramebufferStatus");
    g_framebuffer.del = ResolveGLFunction<DeleteFramebuffersProc>("glDeleteFramebuffers");
    g_state.UseProgram = ResolveGLFunction<decltype(g_state.UseProgram)>("glUseProgram");
    g_state.BindVertexArray = ResolveGLFunction<decltype(g_state.BindVertexArray)>("glBindVertexArray");
    g_state.BindBuffer = ResolveGLFunction<decltype(g_state.BindBuffer)>("glBindBuffer");
    g_state.ActiveTexture = ResolveGLFunction<decltype(g_state.ActiveTexture)>("glActiveTexture");
    g_state.BlendFuncSeparate = ResolveGLFunction<decltype(g_state.BlendFuncSeparate)>("glBlendFuncSeparate");
    g_state.BlendEquationSeparate = ResolveGLFunction<decltype(g_state.BlendEquationSeparate)>("glBlendEquationSeparate");
    const bool ready = g_framebuffer.gen && g_framebuffer.bind && g_framebuffer.attach && g_framebuffer.check && g_framebuffer.del
        && g_state.UseProgram && g_state.BindVertexArray && g_state.BindBuffer && g_state.ActiveTexture
        && g_state.BlendFuncSeparate && g_state.BlendEquationSeparate;
    if (!ready)
        g_framebuffer = {};
    return ready;
}
#else
bool LoadFramebufferFunctions() { return true; }
#endif

void GenFramebuffers(GLsizei count, GLuint* ids)
{
#if defined(_WIN32)
    g_framebuffer.gen(count, ids);
#else
    glGenFramebuffers(count, ids);
#endif
}

void BindFramebuffer(GLenum target, GLuint framebuffer)
{
#if defined(_WIN32)
    g_framebuffer.bind(target, framebuffer);
#else
    glBindFramebuffer(target, framebuffer);
#endif
}

void AttachFramebufferTexture(GLenum target, GLenum attachment, GLenum textureTarget, GLuint texture, GLint level)
{
#if defined(_WIN32)
    g_framebuffer.attach(target, attachment, textureTarget, texture, level);
#else
    glFramebufferTexture2D(target, attachment, textureTarget, texture, level);
#endif
}

GLenum CheckFramebufferStatus(GLenum target)
{
#if defined(_WIN32)
    return g_framebuffer.check(target);
#else
    return glCheckFramebufferStatus(target);
#endif
}

void DeleteFramebuffers(GLsizei count, const GLuint* ids)
{
#if defined(_WIN32)
    g_framebuffer.del(count, ids);
#else
    glDeleteFramebuffers(count, ids);
#endif
}

GLuint TextureId(const UnityRenderTarget& target)
{
    return static_cast<GLuint>(reinterpret_cast<uintptr_t>(target.nativeTexture));
}

bool IsOpenGL(UnityGfxRenderer renderer)
{
    return renderer == kUnityGfxRendererOpenGL || renderer == kUnityGfxRendererOpenGLCore
        || renderer == kUnityGfxRendererOpenGLES20 || renderer == kUnityGfxRendererOpenGLES30;
}

class GLState final {
public:
    GLState()
    {
        glGetIntegerv(0x8B8D, &program_); // GL_CURRENT_PROGRAM
        glGetIntegerv(0x85B5, &vao_); // GL_VERTEX_ARRAY_BINDING
        glGetIntegerv(0x8894, &arrayBuffer_);
        glGetIntegerv(0x8895, &elementBuffer_);
        glGetIntegerv(0x88EF, &unpackBuffer_);
        glGetIntegerv(0x88ED, &packBuffer_);
        glGetIntegerv(kFramebufferBinding, &drawFbo_);
        glGetIntegerv(kReadFramebufferBinding, &readFbo_);
        glGetIntegerv(GL_VIEWPORT, viewport_);
        glGetIntegerv(GL_SCISSOR_BOX, scissor_);
        glGetIntegerv(0x84E0, &activeTexture_);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &activeBinding_);
        for (int i = 0; i < 4; ++i) {
            GL_STATE_FUNCTION(ActiveTexture)(0x84C0 + i);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &textures_[i]);
        }
        GL_STATE_FUNCTION(ActiveTexture)(activeTexture_);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment_);
        glGetIntegerv(0x0CF2, &unpackRowLength_);
        glGetIntegerv(0x80C9, &blend_[0]); // GL_BLEND_SRC_RGB
        glGetIntegerv(0x80C8, &blend_[1]); // GL_BLEND_DST_RGB
        glGetIntegerv(0x80CB, &blend_[2]); // GL_BLEND_SRC_ALPHA
        glGetIntegerv(0x80CA, &blend_[3]); // GL_BLEND_DST_ALPHA
        glGetIntegerv(0x8009, &blendEquation_[0]);
        glGetIntegerv(0x883D, &blendEquation_[1]);
        glGetBooleanv(GL_COLOR_WRITEMASK, colorMask_);
        for (int i = 0; i < 5; ++i)
            enabled_[i] = glIsEnabled(capabilities_[i]);
// Draw the video independently of camera depth, stencil and clipping state.
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    ~GLState()
    {
        GL_STATE_FUNCTION(UseProgram)(program_);
        GL_STATE_FUNCTION(BindVertexArray)(vao_);
        GL_STATE_FUNCTION(BindBuffer)(0x8892, arrayBuffer_);
        if (vao_)
            GL_STATE_FUNCTION(BindBuffer)(0x8893, elementBuffer_);
        GL_STATE_FUNCTION(BindBuffer)(0x88EC, unpackBuffer_);
        GL_STATE_FUNCTION(BindBuffer)(0x88EB, packBuffer_);
        GL_STATE_FUNCTION(ActiveTexture)(activeTexture_);
        glBindTexture(GL_TEXTURE_2D, activeBinding_);
        for (int i = 0; i < 4; ++i) {
            GL_STATE_FUNCTION(ActiveTexture)(0x84C0 + i);
            glBindTexture(GL_TEXTURE_2D, textures_[i]);
        }
        GL_STATE_FUNCTION(ActiveTexture)(activeTexture_);
        glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment_);
        glPixelStorei(0x0CF2, unpackRowLength_);
        GL_STATE_FUNCTION(BlendFuncSeparate)(blend_[0], blend_[1], blend_[2], blend_[3]);
        GL_STATE_FUNCTION(BlendEquationSeparate)(blendEquation_[0], blendEquation_[1]);
        glColorMask(colorMask_[0], colorMask_[1], colorMask_[2], colorMask_[3]);
        for (int i = 0; i < 5; ++i) {
            if (enabled_[i]) glEnable(capabilities_[i]);
            else glDisable(capabilities_[i]);
        }
        glViewport(viewport_[0], viewport_[1], viewport_[2], viewport_[3]);
        glScissor(scissor_[0], scissor_[1], scissor_[2], scissor_[3]);
        BindFramebuffer(kDrawFramebuffer, drawFbo_);
        BindFramebuffer(kReadFramebuffer, readFbo_);
    }

private:
    GLint program_ = 0, vao_ = 0, arrayBuffer_ = 0, elementBuffer_ = 0, unpackBuffer_ = 0, packBuffer_ = 0;
    GLint drawFbo_ = 0, readFbo_ = 0, activeTexture_ = 0, activeBinding_ = 0;
    GLint viewport_[4]{}, scissor_[4]{}, textures_[4]{}, blend_[4]{}, blendEquation_[2]{};
    GLint unpackAlignment_ = 4, unpackRowLength_ = 0;
    GLboolean colorMask_[4]{}, enabled_[5]{};
    static constexpr GLenum capabilities_[5] = {GL_DEPTH_TEST, GL_STENCIL_TEST, GL_CULL_FACE, GL_SCISSOR_TEST, GL_BLEND};
};

} // namespace

MDKUnityOpenGLStateGuard::MDKUnityOpenGLStateGuard(UnityGfxRenderer renderer)
{
    if (IsOpenGL(renderer) && glGetString(GL_VERSION) && LoadFramebufferFunctions())
        state_ = new GLState;
}

MDKUnityOpenGLStateGuard::~MDKUnityOpenGLStateGuard()
{
    delete static_cast<GLState*>(state_);
}

bool MDKUnityOpenGLInitialize(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    if (!IsOpenGL(renderer))
        return false;
    if (!LoadFramebufferFunctions())
        return false;
    UnityRenderTarget target;
    {
        const std::lock_guard lock(player.targetMutex);
        target = player.target;
    }
    const auto texture = TextureId(target);
    if (!texture)
        return false;

    GLuint fbo = 0;
    GenFramebuffers(1, &fbo);
    GLint previous = 0, previousRead = 0;
    glGetIntegerv(kFramebufferBinding, &previous);
    glGetIntegerv(kReadFramebufferBinding, &previousRead);
    BindFramebuffer(kFramebuffer, fbo);
    AttachFramebufferTexture(kFramebuffer, kColorAttachment0, GL_TEXTURE_2D, texture, 0);
    const auto complete = CheckFramebufferStatus(kFramebuffer) == kFramebufferComplete;
    BindFramebuffer(kDrawFramebuffer, static_cast<GLuint>(previous));
    BindFramebuffer(kReadFramebuffer, static_cast<GLuint>(previousRead));
    if (!complete) {
        DeleteFramebuffers(1, &fbo);
        return false;
    }

    auto api = std::make_unique<mdk::GLRenderAPI>();
    api->fbo = static_cast<int>(fbo);
    player.renderApi = std::move(api);
    player.backendState = reinterpret_cast<void*>(static_cast<uintptr_t>(fbo));
    player.player.setRenderAPI(player.renderApi.get(), &player);
    return true;
}

bool MDKUnityOpenGLRender(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    if (!IsOpenGL(renderer) || !player.renderApi)
        return false;
    return player.player.renderVideo(&player) >= 0;
}

void MDKUnityOpenGLRelease(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    if (!IsOpenGL(renderer))
        return;
    const auto fbo = static_cast<GLuint>(reinterpret_cast<uintptr_t>(player.backendState));
    if (fbo)
        DeleteFramebuffers(1, &fbo);
    player.backendState = nullptr;
}

void MDKUnityOpenGLDeviceEvent([[maybe_unused]] UnityGfxDeviceEventType eventType,
                               IUnityInterfaces*, UnityGfxRenderer)
{
#if defined(_WIN32)
    if (eventType == kUnityGfxDeviceEventShutdown || eventType == kUnityGfxDeviceEventBeforeReset) {
        g_framebuffer = {};
        g_state = {};
    }
#endif
}

#else

MDKUnityOpenGLStateGuard::MDKUnityOpenGLStateGuard(UnityGfxRenderer) {}
MDKUnityOpenGLStateGuard::~MDKUnityOpenGLStateGuard() = default;

bool MDKUnityOpenGLInitialize(MDKUnityPlayer&, UnityGfxRenderer) { return false; }
bool MDKUnityOpenGLRender(MDKUnityPlayer&, UnityGfxRenderer) { return false; }
void MDKUnityOpenGLRelease(MDKUnityPlayer&, UnityGfxRenderer) {}
void MDKUnityOpenGLDeviceEvent(UnityGfxDeviceEventType, IUnityInterfaces*, UnityGfxRenderer) {}

#endif

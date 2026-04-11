/*
 * Copyright (c) 2024 WangBin <wbsecg1 at gmail.com>
 * MDK SDK OHOS (OpenHarmony) example
 *
 * This file implements the native N-API module for MDK player on OHOS.
 * It handles XComponent surface lifecycle and exposes player controls.
 */
#include <napi/native_api.h>
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <hilog/log.h>
#include "mdk/Player.h"
#include <map>
#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <cstring>

using namespace MDK_NS;

// Global player storage keyed by XComponent ID
static std::mutex gMutex;
static std::map<std::string, std::unique_ptr<Player>> gPlayers;
static std::string gCurrentId;

static Player* GetPlayer(const std::string& id) {
    std::lock_guard<std::mutex> lock(gMutex);
    auto it = gPlayers.find(id);
    return (it != gPlayers.end()) ? it->second.get() : nullptr;
}

static Player* GetDefaultPlayer() {
    std::lock_guard<std::mutex> lock(gMutex);
    if (gCurrentId.empty()) return nullptr;
    auto it = gPlayers.find(gCurrentId);
    return (it != gPlayers.end()) ? it->second.get() : nullptr;
}

// XComponent surface callbacks
static void OnSurfaceCreated(OH_NativeXComponent* component, void* window) {
    char id[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
    uint64_t idLen = OH_XCOMPONENT_ID_LEN_MAX + 1;
    OH_NativeXComponent_GetXComponentId(component, id, &idLen);

    uint64_t w = 0, h = 0;
    OH_NativeXComponent_GetXComponentSize(component, window, &w, &h);

    auto* player = GetPlayer(std::string(id));
    if (player)
        player->updateNativeSurface(window, (int)w, (int)h);
}

static void OnSurfaceChanged(OH_NativeXComponent* component, void* window) {
    char id[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
    uint64_t idLen = OH_XCOMPONENT_ID_LEN_MAX + 1;
    OH_NativeXComponent_GetXComponentId(component, id, &idLen);

    uint64_t w = 0, h = 0;
    OH_NativeXComponent_GetXComponentSize(component, window, &w, &h);

    auto* player = GetPlayer(std::string(id));
    if (player)
        player->setVideoSurfaceSize((int)w, (int)h);
}

static void OnSurfaceDestroyed(OH_NativeXComponent* component, void* window) {
    char id[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
    uint64_t idLen = OH_XCOMPONENT_ID_LEN_MAX + 1;
    OH_NativeXComponent_GetXComponentId(component, id, &idLen);

    auto* player = GetPlayer(std::string(id));
    if (player)
        player->updateNativeSurface(nullptr, 0, 0);
}

// N-API helper: get string argument
static std::string GetStringArg(napi_env env, napi_value val) {
    size_t len = 0;
    napi_get_value_string_utf8(env, val, nullptr, 0, &len);
    std::vector<char> buf(len + 1);
    napi_get_value_string_utf8(env, val, buf.data(), len + 1, &len);
    return std::string(buf.data(), len);
}

// N-API exported functions

static napi_value SetMedia(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    std::string url = GetStringArg(env, args[0]);
    auto* player = GetDefaultPlayer();
    if (player) {
        // "fd://N" — use setMedia("fd:") + setProperty("avio", "<N>")
        if (url.rfind("fd://", 0) == 0) {
            std::string fdStr = url.substr(5);
            player->setProperty("avio", fdStr.c_str());
            player->setMedia("fd:");
        } else {
            player->setMedia(url.c_str());
        }
    }

    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value Play(napi_env env, napi_callback_info info) {
    auto* player = GetDefaultPlayer();
    if (player)
        player->set(State::Playing);
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value Pause(napi_env env, napi_callback_info info) {
    auto* player = GetDefaultPlayer();
    if (player)
        player->set(State::Paused);
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value Stop(napi_env env, napi_callback_info info) {
    auto* player = GetDefaultPlayer();
    if (player)
        player->set(State::Stopped);
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value Seek(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    int64_t ms = 0;
    napi_get_value_int64(env, args[0], &ms);

    auto* player = GetDefaultPlayer();
    if (player)
        player->seek(ms);

    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value SetPlaybackRate(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    double rate = 1.0;
    napi_get_value_double(env, args[0], &rate);

    auto* player = GetDefaultPlayer();
    if (player)
        player->setPlaybackRate((float)rate);

    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value SetVolume(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    double vol = 1.0;
    napi_get_value_double(env, args[0], &vol);

    auto* player = GetDefaultPlayer();
    if (player)
        player->setVolume((float)vol);

    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value GetPosition(napi_env env, napi_callback_info info) {
    int64_t pos = 0;
    auto* player = GetDefaultPlayer();
    if (player)
        pos = player->position();
    napi_value result;
    napi_create_int64(env, pos, &result);
    return result;
}

static napi_value GetDuration(napi_env env, napi_callback_info info) {
    int64_t dur = 0;
    auto* player = GetDefaultPlayer();
    if (player)
        dur = player->mediaInfo().duration;
    napi_value result;
    napi_create_int64(env, dur, &result);
    return result;
}

static napi_value IsPlaying(napi_env env, napi_callback_info info) {
    bool playing = false;
    auto* player = GetDefaultPlayer();
    if (player)
        playing = (player->state() == State::Playing);
    napi_value result;
    napi_get_boolean(env, playing, &result);
    return result;
}

// Module init: called when loaded via XComponent libraryname or regular import
static napi_value Init(napi_env env, napi_value exports) {
    // Redirect MDK log output to the OHOS HiLog system with tag "mdk"
    setLogHandler([](MDK_NS::LogLevel level, const char* msg) {
        static const ::LogLevel ohLevel[] = {
            LOG_INFO,    // MDK LogLevel::Off   (0)
            LOG_ERROR,   // MDK LogLevel::Error (1)
            LOG_WARN,    // MDK LogLevel::Warning (2)
            LOG_INFO,    // MDK LogLevel::Info  (3)
            LOG_DEBUG,   // MDK LogLevel::Debug (4)
            LOG_DEBUG,   // MDK LogLevel::All   (5)
        };
        const int idx = (int)level < 6 ? (int)level : 0;
        OH_LOG_Print(LOG_APP, ohLevel[idx], 0xFF00, "mdk", "%{public}s", msg);
    });

    // When loaded via XComponent libraryname, OH_NATIVE_XCOMPONENT_OBJ is set
    napi_value xcompInstance = nullptr;
    napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &xcompInstance);

    if (xcompInstance) {
        napi_valuetype valType = napi_undefined;
        napi_typeof(env, xcompInstance, &valType);

        if (valType != napi_undefined && valType != napi_null) {
            OH_NativeXComponent* nativeXComponent = nullptr;
            napi_unwrap(env, xcompInstance, (void**)&nativeXComponent);

            if (nativeXComponent) {
                char id[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
                uint64_t idLen = OH_XCOMPONENT_ID_LEN_MAX + 1;
                OH_NativeXComponent_GetXComponentId(nativeXComponent, id, &idLen);

                std::string xcompId(id);
                {
                    std::lock_guard<std::mutex> lock(gMutex);
                    if (gPlayers.find(xcompId) == gPlayers.end())
                        gPlayers[xcompId] = std::make_unique<Player>();
                    gCurrentId = xcompId;
                }

                static OH_NativeXComponent_Callback callbacks{};
                callbacks.OnSurfaceCreated = OnSurfaceCreated;
                callbacks.OnSurfaceChanged = OnSurfaceChanged;
                callbacks.OnSurfaceDestroyed = OnSurfaceDestroyed;
                callbacks.DispatchTouchEvent = nullptr;
                OH_NativeXComponent_RegisterCallback(nativeXComponent, &callbacks);
            }
        }
    }

    // Register N-API functions
    napi_property_descriptor desc[] = {
        {"setMedia",         nullptr, SetMedia,         nullptr, nullptr, nullptr, napi_default, nullptr},
        {"play",             nullptr, Play,             nullptr, nullptr, nullptr, napi_default, nullptr},
        {"pause",            nullptr, Pause,            nullptr, nullptr, nullptr, napi_default, nullptr},
        {"stop",             nullptr, Stop,             nullptr, nullptr, nullptr, napi_default, nullptr},
        {"seek",             nullptr, Seek,             nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setPlaybackRate",  nullptr, SetPlaybackRate,  nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setVolume",        nullptr, SetVolume,        nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getPosition",      nullptr, GetPosition,      nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getDuration",      nullptr, GetDuration,      nullptr, nullptr, nullptr, napi_default, nullptr},
        {"isPlaying",        nullptr, IsPlaying,        nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);

    return exports;
}

static napi_module playerModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "player_napi",
    .nm_priv = nullptr,
    .reserved = {nullptr},
};

extern "C" __attribute__((constructor)) void RegisterPlayerNapiModule(void) {
    napi_module_register(&playerModule);
}

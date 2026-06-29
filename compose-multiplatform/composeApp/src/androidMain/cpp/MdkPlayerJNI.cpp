/*
 * Copyright (c) 2024 WangBin <wbsecg1 at gmail.com>
 * MDK SDK Compose Multiplatform example – Android JNI wrapper
 *
 * Wraps the MDK C++ Player API for use from Kotlin via JNI.
 *
 * To build, place the Android MDK SDK at:
 *   src/androidMain/cpp/mdk-sdk/
 * or set the MDK_SDK_DIR cmake variable:
 *   cmake -DMDK_SDK_DIR=/path/to/mdk-sdk ...
 *
 * Download: https://sourceforge.net/projects/mdk-sdk/files/nightly/
 */
#include <jni.h>
#include <android/native_window_jni.h>
#include "mdk/Player.h"

using namespace MDK_NS;

extern "C" {

JNIEXPORT jlong JNICALL
Java_org_mdk_example_MdkPlayer_nativeCreate(JNIEnv*, jobject) {
    return reinterpret_cast<jlong>(new Player());
}

JNIEXPORT void JNICALL
Java_org_mdk_example_MdkPlayer_nativeRelease(JNIEnv*, jobject, jlong handle) {
    delete reinterpret_cast<Player*>(handle);
}

JNIEXPORT void JNICALL
Java_org_mdk_example_MdkPlayer_nativeSetMedia(JNIEnv* env, jobject,
                                               jlong handle, jstring url) {
    const char* c = env->GetStringUTFChars(url, nullptr);
    reinterpret_cast<Player*>(handle)->setMedia(c);
    env->ReleaseStringUTFChars(url, c);
}

JNIEXPORT void JNICALL
Java_org_mdk_example_MdkPlayer_nativePlay(JNIEnv*, jobject, jlong handle) {
    reinterpret_cast<Player*>(handle)->set(State::Playing);
}

JNIEXPORT void JNICALL
Java_org_mdk_example_MdkPlayer_nativePause(JNIEnv*, jobject, jlong handle) {
    reinterpret_cast<Player*>(handle)->set(State::Paused);
}

JNIEXPORT void JNICALL
Java_org_mdk_example_MdkPlayer_nativeStop(JNIEnv*, jobject, jlong handle) {
    reinterpret_cast<Player*>(handle)->set(State::Stopped);
}

JNIEXPORT void JNICALL
Java_org_mdk_example_MdkPlayer_nativeUpdateSurface(JNIEnv* env, jobject,
                                                    jlong handle, jobject surface,
                                                    jint width, jint height) {
    auto* player = reinterpret_cast<Player*>(handle);
    if (surface) {
        ANativeWindow* win = ANativeWindow_fromSurface(env, surface);
        player->updateNativeSurface(win, static_cast<int>(width),
                                    static_cast<int>(height));
        ANativeWindow_release(win);
    } else {
        // Detach surface: pass null with -1 x -1 to signal teardown
        player->updateNativeSurface(nullptr, -1, -1);
    }
}

JNIEXPORT void JNICALL
Java_org_mdk_example_MdkPlayer_nativeSetSurfaceSize(JNIEnv*, jobject,
                                                     jlong handle,
                                                     jint width, jint height) {
    reinterpret_cast<Player*>(handle)->setVideoSurfaceSize(
        static_cast<int>(width), static_cast<int>(height));
}

} // extern "C"

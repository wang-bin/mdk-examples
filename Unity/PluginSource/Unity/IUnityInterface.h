/*
 * Copyright (c) 2015 Unity Technologies ApS.
 * AI participated
 * Licensed under the Unity Companion License for Unity-dependent software.
 * See https://unity3d.com/legal/licenses/Unity_Companion_License.
 */
#pragma once

// Unity native plugin API
// Compatible with C99. The interface definitions are distributed under the
// Unity Companion License. Keep this header in sync with the Unity version
// used to build the native plugin.

#if defined(_WIN32) || defined(_WIN64) || defined(WINAPI_FAMILY)
#define UNITY_INTERFACE_API __stdcall
#define UNITY_INTERFACE_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define UNITY_INTERFACE_API
#define UNITY_INTERFACE_EXPORT __attribute__((visibility("default")))
#else
#define UNITY_INTERFACE_API
#define UNITY_INTERFACE_EXPORT
#endif

struct UnityInterfaceGUID {
#ifdef __cplusplus
    UnityInterfaceGUID(unsigned long long high, unsigned long long low)
        : m_GUIDHigh(high), m_GUIDLow(low) {}
    bool Equals(const UnityInterfaceGUID& other) const {
        return m_GUIDHigh == other.m_GUIDHigh && m_GUIDLow == other.m_GUIDLow;
    }
#endif
    unsigned long long m_GUIDHigh;
    unsigned long long m_GUIDLow;
};

#ifdef __cplusplus
inline bool operator==(const UnityInterfaceGUID& a, const UnityInterfaceGUID& b) { return a.Equals(b); }
inline bool operator!=(const UnityInterfaceGUID& a, const UnityInterfaceGUID& b) { return !(a == b); }
#endif

#define UNITY_GET_INTERFACE_GUID(TYPE) TYPE##_GUID
#define UNITY_GET_INTERFACE(INTERFACES, TYPE) (TYPE*)INTERFACES->GetInterface(UNITY_GET_INTERFACE_GUID(TYPE))

#ifdef __cplusplus
#define UNITY_DECLARE_INTERFACE(NAME) struct NAME : IUnityInterface
template<typename TYPE> inline const UnityInterfaceGUID GetUnityInterfaceGUID();
#define UNITY_REGISTER_INTERFACE_GUID(HASHH, HASHL, TYPE) \
    const UnityInterfaceGUID TYPE##_GUID(HASHH, HASHL); \
    template<> inline const UnityInterfaceGUID GetUnityInterfaceGUID<TYPE>() { return TYPE##_GUID; }
#else
#define UNITY_DECLARE_INTERFACE(NAME) typedef struct NAME NAME; struct NAME
#define UNITY_REGISTER_INTERFACE_GUID(HASHH, HASHL, TYPE) const UnityInterfaceGUID TYPE##_GUID = {HASHH, HASHL}
#endif

#ifdef __cplusplus
struct IUnityInterface {};
#else
typedef void IUnityInterface;
#endif

typedef struct IUnityInterfaces {
    IUnityInterface* (UNITY_INTERFACE_API *GetInterface)(UnityInterfaceGUID guid);
    void (UNITY_INTERFACE_API *RegisterInterface)(UnityInterfaceGUID guid, IUnityInterface* ptr);
    IUnityInterface* (UNITY_INTERFACE_API *GetInterfaceSplit)(unsigned long long guidHigh, unsigned long long guidLow);
    void (UNITY_INTERFACE_API *RegisterInterfaceSplit)(unsigned long long guidHigh, unsigned long long guidLow, IUnityInterface* ptr);
#ifdef __cplusplus
    template<typename INTERFACE>
    INTERFACE* Get() { return static_cast<INTERFACE*>(GetInterface(GetUnityInterfaceGUID<INTERFACE>())); }
    template<typename INTERFACE>
    void Register(IUnityInterface* ptr) { RegisterInterface(GetUnityInterfaceGUID<INTERFACE>(), ptr); }
#endif
} IUnityInterfaces;

typedef void (UNITY_INTERFACE_API *UnityRenderingEvent)(int eventId);
typedef void (UNITY_INTERFACE_API *UnityRenderingEventAndData)(int eventId, void* data);

#ifdef __cplusplus
extern "C" {
#endif
void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces);
void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload();
#ifdef __cplusplus
}
#endif

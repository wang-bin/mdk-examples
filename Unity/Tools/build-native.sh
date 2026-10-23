#!/usr/bin/env sh
set -eu

platform=${1:-Linux}
configuration=${CONFIGURATION:-Release}
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_root=$(CDPATH= cd -- "$script_dir/.." && pwd)
plugin_source="$project_root/PluginSource"
build_root="$project_root/build/native-$platform"
if [ "$platform" = "Android" ]; then
  abi=${ANDROID_ABI:-arm64-v8a}
elif [ "$platform" = "Windows" ]; then
  abi=x86_64
else
  abi=native
fi
unity_platform=$platform
if [ "$platform" = "MacOS" ]; then
  unity_platform=macOS
fi
install_root="$project_root/Assets/Plugins/$unity_platform/$abi"
mkdir -p "$install_root"

set -- -S "$plugin_source" -B "$build_root" -G Ninja \
  "-DCMAKE_BUILD_TYPE=$configuration" \
  "-DCMAKE_INSTALL_PREFIX=$install_root" \
  "-DMDK_UNITY_INSTALL_SUBDIR=."
if [ -n "${MDK_SDK_ROOT:-}" ]; then
  set -- "$@" "-DMDK_SDK_ROOT=$MDK_SDK_ROOT"
fi
if [ -n "${MDK_RUNTIME_DIR:-}" ]; then
  set -- "$@" "-DMDK_UNITY_RUNTIME_DIR=$MDK_RUNTIME_DIR"
fi
if [ -n "${CMAKE_TOOLCHAIN_FILE:-}" ]; then
  set -- "$@" "-DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE"
fi
if [ "$platform" = "Android" ]; then
  set -- "$@" "-DANDROID_ABI=${ANDROID_ABI:-arm64-v8a}" "-DANDROID_PLATFORM=${ANDROID_PLATFORM:-android-21}"
fi

cmake "$@"
cmake --build "$build_root" --target install --config "$configuration"

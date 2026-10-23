#!/usr/bin/env sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_root=$(CDPATH= cd -- "$script_dir/.." && pwd)
build_root="$project_root/build/native-iOS"
install_root="$project_root/Assets/Plugins/iOS/arm64"
mkdir -p "$install_root"

set -- -S "$project_root/PluginSource" -B "$build_root" -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_INSTALL_PREFIX="$install_root" \
  -DMDK_UNITY_INSTALL_SUBDIR=.
if [ -n "${MDK_SDK_ROOT:-}" ]; then
  set -- "$@" "-DMDK_SDK_ROOT=$MDK_SDK_ROOT"
fi
if [ -n "${MDK_RUNTIME_DIR:-}" ]; then
  set -- "$@" "-DMDK_UNITY_RUNTIME_DIR=$MDK_RUNTIME_DIR"
fi

cmake "$@"
cmake --build "$build_root" --target install --config "${CONFIGURATION:-Release}"

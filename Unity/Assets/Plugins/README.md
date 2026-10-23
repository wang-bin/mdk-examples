# Native plugin output

The CMake install step copies the platform-specific `MDKUnityPlugin` binary and
the MDK/FFmpeg runtime libraries into this directory.  The repository ignores
those generated binaries; keep this file so the destination exists in a fresh
checkout.

param(
    [ValidateSet("Windows", "Linux", "MacOS", "Android", "iOS")]
    [string] $Platform = "Windows",
    [string] $MdkSdkRoot = $env:MDK_SDK_ROOT,
    [string] $MdkRuntimeDir = "",
    [string] $ToolchainFile = "",
    [string] $AndroidAbi = "arm64-v8a",
    [string] $AndroidPlatform = "android-21",
    [string] $Configuration = "Release"
)

$pluginSource = (Resolve-Path (Join-Path $PSScriptRoot "..\PluginSource")).Path
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildRoot = Join-Path $projectRoot ("build\native-" + $Platform)
$abi = if ($Platform -eq "Android") { $AndroidAbi } elseif ($Platform -eq "Windows") { "x86_64" } else { "native" }
$unityPlatform = if ($Platform -eq "MacOS") { "macOS" } else { $Platform }
$installRoot = Join-Path $projectRoot ("Assets\Plugins\" + $unityPlatform + "\" + $abi)
New-Item -ItemType Directory -Force -Path $installRoot | Out-Null
$cmakeArgs = @(
    "-S", $pluginSource,
    "-B", $buildRoot,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=$Configuration",
    "-DCMAKE_INSTALL_PREFIX=$installRoot",
    "-DMDK_UNITY_INSTALL_SUBDIR=."
)
if ($MdkSdkRoot) { $cmakeArgs += "-DMDK_SDK_ROOT=$MdkSdkRoot" }
if ($MdkRuntimeDir) { $cmakeArgs += "-DMDK_UNITY_RUNTIME_DIR=$MdkRuntimeDir" }
if ($ToolchainFile) { $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$ToolchainFile" }
if ($Platform -eq "Android") {
    $cmakeArgs += "-DANDROID_ABI=$AndroidAbi"
    $cmakeArgs += "-DANDROID_PLATFORM=$AndroidPlatform"
}

cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build $buildRoot --target install --config $Configuration
exit $LASTEXITCODE

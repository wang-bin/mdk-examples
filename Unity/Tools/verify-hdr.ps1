param(
    [Parameter(Mandatory=$true)][string]$Editor,
    [ValidateSet('d3d11', 'd3d12')][string]$Backend = 'd3d11',
    [ValidateSet('FFmpeg', 'D3D11:copy=0')][string]$Decoder = 'FFmpeg',
    [string]$Project,
    [switch]$Interactive
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../../..").Path
$output = "$repo/build/cl64/unity-hdr"
if (!$Project) { $Project = "$repo/examples/Unity" }
New-Item -ItemType Directory -Force $output | Out-Null
$name = "$Backend-" + $Decoder.Replace(':','-').Replace('=','-')
$name += '-' + (Get-Date -Format 'yyyyMMdd-HHmmss')
$variables = @('MDK_LOG','MDK_CLOG','MDK_UNITY_VERIFY_EXIT','MDK_UNITY_VERIFY_HDR','MDK_UNITY_VERIFY_MEDIA','MDK_UNITY_VIDEO_DECODER','MDK_UNITY_VERIFY_RESIZE_MEDIA')
$saved = @{}
foreach ($key in $variables) { $saved[$key] = [Environment]::GetEnvironmentVariable($key) }
try {
    $env:MDK_LOG='1'; $env:MDK_CLOG='1'; $env:MDK_UNITY_VERIFY_EXIT='1'; $env:MDK_UNITY_VERIFY_HDR='1'
    $env:MDK_UNITY_VERIFY_MEDIA="$output/hdr10-patches.mkv"
    $env:MDK_UNITY_VERIFY_RESIZE_MEDIA="$output/hdr10-patches-small.mkv"
    $env:MDK_UNITY_VIDEO_DECODER=$Decoder
    if (!(Test-Path -LiteralPath $env:MDK_UNITY_VERIFY_MEDIA)) { throw 'Generate HDR fixture first.' }
    $arguments = @("-force-$Backend", '-job-worker-count', '2', '-projectPath', $Project, '-executeMethod', 'MDK.Unity.Editor.MDKPlaybackVerification.Run', '-logFile', "$output/$name.log")
    if (!$Interactive) { $arguments += '-batchmode' }
    $process = Start-Process -FilePath $Editor -ArgumentList $arguments -RedirectStandardError "$output/$name.stderr.log" -RedirectStandardOutput "$output/$name.stdout.log" -WindowStyle Hidden -PassThru
    if (!$process.WaitForExit(180000)) {
        Stop-Process -Id $process.Id -Force
        throw "Editor startup/verification timed out: $output/$name.log"
    }
    if ($process.ExitCode -ne 0) { throw "HDR verification failed: $output/$name.log" }
} finally {
    foreach ($key in $variables) { [Environment]::SetEnvironmentVariable($key, $saved[$key]) }
}

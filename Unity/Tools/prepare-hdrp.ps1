param([string]$Destination = "$PSScriptRoot/../Validation/HDRPProject", [string]$UnityEditorRoot)
$ErrorActionPreference = 'Stop'
$source = (Resolve-Path "$PSScriptRoot/..").Path
$destinationPath = [IO.Path]::GetFullPath($Destination)
New-Item -ItemType Directory -Force "$destinationPath/Assets" | Out-Null
# Copy only canonical shared sources and their GUIDs; never edit the generated copy.
foreach ($name in @('MDKUnity', 'Plugins')) {
    $generated = [IO.Path]::GetFullPath((Join-Path $destinationPath "Assets/$name"))
    $assetsRoot = [IO.Path]::GetFullPath((Join-Path $destinationPath 'Assets')) + [IO.Path]::DirectorySeparatorChar
    if (!$generated.StartsWith($assetsRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe generated asset path' }
    if (Test-Path -LiteralPath $generated) { Remove-Item -LiteralPath $generated -Recurse -Force }
}
Copy-Item -LiteralPath "$source/Assets/MDKUnity" -Destination "$destinationPath/Assets" -Recurse -Force
Copy-Item -LiteralPath "$source/Assets/MDKUnity.meta" -Destination "$destinationPath/Assets" -Force
Copy-Item -LiteralPath "$source/Assets/Plugins" -Destination "$destinationPath/Assets" -Recurse -Force
Copy-Item -LiteralPath "$source/Assets/Plugins.meta" -Destination "$destinationPath/Assets" -Force
if ($UnityEditorRoot) {
    $packages = "$UnityEditorRoot/Data/Resources/PackageManager/BuiltInPackages"
    foreach ($name in @('com.unity.render-pipelines.core', 'com.unity.render-pipelines.high-definition', 'com.unity.render-pipelines.high-definition-config', 'com.unity.shadergraph', 'com.unity.visualeffectgraph')) {
        $package = Get-Content -LiteralPath "$packages/$name/package.json" -Raw | ConvertFrom-Json
        if ($package.version -ne '14.0.12') { throw "$name is $($package.version), expected 14.0.12" }
        Copy-Item -LiteralPath "$packages/$name" -Destination "$destinationPath/Packages" -Recurse -Force
    }
}
Write-Output "Prepared $destinationPath. Open with Unity 2022.3 -force-d3d12, then MDK/HDRP/Create validation scene."

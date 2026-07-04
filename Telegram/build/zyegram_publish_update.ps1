param(
    [string]$TDesktopRoot = (Resolve-Path "$PSScriptRoot\..\..").Path,
    [string]$Configuration = "Release",
    [string]$Platform = "win64",
    [string]$UpdateRoot = "",
    [switch]$SkipBuildPacker
)

$ErrorActionPreference = "Stop"

function Read-VersionValue([string]$Path, [string]$Name) {
    $line = Get-Content -Path $Path -Encoding UTF8 |
        Where-Object { $_ -match "^\s*$Name\s+" } |
        Select-Object -First 1
    if (-not $line) {
        throw "Version value '$Name' not found in $Path"
    }
    return ($line -split "\s+")[-1]
}

$outDir = Join-Path $TDesktopRoot "out"
$releaseDir = Join-Path $outDir $Configuration
$versionFile = Join-Path $TDesktopRoot "Telegram\build\version"
$version = Read-VersionValue $versionFile "AppVersion"

if (-not $UpdateRoot) {
    $UpdateRoot = Join-Path $releaseDir "zyegram_update_source"
}

$packerExe = Join-Path $releaseDir "Packer.exe"
$appExe = Join-Path $releaseDir "ZyeGram.exe"
$updaterExe = Join-Path $releaseDir "Updater.exe"
$d3dCompiler = Join-Path $releaseDir "modules\x64\d3d\d3dcompiler_47.dll"

if (-not $SkipBuildPacker) {
    cmake --build $outDir --config $Configuration --target Packer
    if ($LASTEXITCODE -ne 0) {
        throw "Packer build failed. Reconfigure once with: Telegram\configure.bat -DDESKTOP_APP_DISABLE_AUTOUPDATE=OFF"
    }
}

foreach ($required in @($packerExe, $appExe, $updaterExe, $d3dCompiler)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Missing required file: $required"
    }
}

New-Item -ItemType Directory -Path $UpdateRoot -Force | Out-Null

Push-Location $releaseDir
try {
    $package = "tx64upd$version"
    if (Test-Path -LiteralPath $package) {
        Remove-Item -LiteralPath $package -Force
    }

    & $packerExe -version $version -path "ZyeGram.exe" -path "Updater.exe" -path "modules\x64\d3d\d3dcompiler_47.dll" -target $Platform
    if ($LASTEXITCODE -ne 0) {
        throw "Packer failed."
    }

    Copy-Item -LiteralPath (Join-Path $releaseDir $package) -Destination (Join-Path $UpdateRoot $package) -Force

    $manifest = [ordered]@{
        win64 = [ordered]@{
            stable = [ordered]@{
                released = [int64]$version
                link = "/$package"
            }
        }
    }
    $manifestPath = Join-Path $UpdateRoot "current4"
    $json = $manifest | ConvertTo-Json -Depth 8
    [System.IO.File]::WriteAllText(
        $manifestPath,
        $json,
        (New-Object System.Text.UTF8Encoding($false)))

    Write-Host "ZyeGram update source written:"
    Write-Host "  $UpdateRoot"
    Write-Host "Upload these files to:"
    Write-Host "  https://raw.githubusercontent.com/Jiang-86/ZyeGramDesktop/main/updates/"
}
finally {
    Pop-Location
}

param(
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "build-qt"

function Find-QtRoot {
    if ($env:QTDIR -and (Test-Path (Join-Path $env:QTDIR "bin\windeployqt.exe"))) {
        return $env:QTDIR
    }

    if (-not (Test-Path "C:\Qt")) {
        throw "Qt was not found. Install Qt 6.5+ or set QTDIR to the compiler kit directory."
    }

    $kits = Get-ChildItem "C:\Qt" -Directory |
        ForEach-Object {
            Get-ChildItem $_.FullName -Directory -Filter "mingw_64" -ErrorAction SilentlyContinue
        } |
        Where-Object { Test-Path (Join-Path $_.FullName "bin\windeployqt.exe") } |
        Sort-Object { [version]$_.Parent.Name } -Descending

    $kit = $kits | Select-Object -First 1
    if (-not $kit) {
        throw "No Qt MinGW kit was found. Install Qt 6.5+ or set QTDIR."
    }
    return $kit.FullName
}

function Assert-LastCommand([string]$message) {
    if ($LASTEXITCODE -ne 0) {
        throw "$message (exit code $LASTEXITCODE)."
    }
}

$qtRoot = Find-QtRoot
$qtBin = Join-Path $qtRoot "bin"
$deployTool = Join-Path $qtBin "windeployqt.exe"

if (-not $NoBuild) {
    $mingwRoot = Get-ChildItem "C:\Qt\Tools" -Directory -Filter "mingw*_64" |
        Sort-Object Name -Descending |
        Select-Object -First 1
    $ninja = "C:\Qt\Tools\Ninja\ninja.exe"

    if (-not $mingwRoot -or -not (Test-Path $ninja)) {
        throw "The Qt MinGW compiler or Ninja was not found under C:\Qt\Tools."
    }

    $compiler = Join-Path $mingwRoot.FullName "bin\g++.exe"
    Write-Host "Configuring immich..."
    & cmake -S $projectRoot -B $buildDirectory -G Ninja `
        "-DCMAKE_PREFIX_PATH=$qtRoot" `
        "-DCMAKE_CXX_COMPILER=$compiler" `
        "-DCMAKE_MAKE_PROGRAM=$ninja"
    Assert-LastCommand "CMake configuration failed"

    Write-Host "Building and deploying Qt runtime..."
    & cmake --build $buildDirectory
    Assert-LastCommand "Build failed"
}

$executable = Join-Path $buildDirectory "immich.exe"
if (-not (Test-Path $executable)) {
    throw "immich.exe was not found. Run this script without -NoBuild first."
}

# Keep -NoBuild useful for older build directories that have not run the
# CMake post-build deployment command yet.
$widgetsRuntime = Join-Path $buildDirectory "Qt6Widgets.dll"
if (-not (Test-Path $widgetsRuntime)) {
    Write-Host "Deploying missing Qt runtime libraries..."
    & $deployTool --no-translations --compiler-runtime $executable
    Assert-LastCommand "Qt runtime deployment failed"
}

Write-Host "Starting immich..."
Start-Process -FilePath $executable -WorkingDirectory $buildDirectory

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

function Get-CacheValue([string]$cacheFile, [string]$name) {
    if (-not (Test-Path $cacheFile)) {
        return $null
    }
    $line = Get-Content $cacheFile | Where-Object { $_ -like "${name}:*" } | Select-Object -First 1
    if (-not $line) {
        return $null
    }
    return ($line -split "=", 2)[1]
}

function PathsEqual([string]$left, [string]$right) {
    if ([string]::IsNullOrWhiteSpace($left) -or [string]::IsNullOrWhiteSpace($right)) {
        return $false
    }
    try {
        return ([IO.Path]::GetFullPath($left)).TrimEnd('\') -eq
               ([IO.Path]::GetFullPath($right)).TrimEnd('\')
    } catch {
        return $false
    }
}

$qtRoot = Find-QtRoot
$qtBin = Join-Path $qtRoot "bin"
$qtCMakeDir = Join-Path $qtRoot "lib\cmake\Qt6"
$deployTool = Join-Path $qtBin "windeployqt.exe"

if (-not (Test-Path $qtCMakeDir)) {
    throw "Qt CMake package was not found at $qtCMakeDir."
}

# Survive CMake's internal cache reset when the toolchain changes.
$env:CMAKE_PREFIX_PATH = $qtRoot

if (-not $NoBuild) {
    $mingwRoot = Get-ChildItem "C:\Qt\Tools" -Directory -Filter "mingw*_64" |
        Sort-Object Name -Descending |
        Select-Object -First 1
    $ninja = "C:\Qt\Tools\Ninja\ninja.exe"

    if (-not $mingwRoot -or -not (Test-Path $ninja)) {
        throw "The Qt MinGW compiler or Ninja was not found under C:\Qt\Tools."
    }

    $compiler = (Join-Path $mingwRoot.FullName "bin\g++.exe") -replace '\\', '/'
    $rcCompiler = (Join-Path $mingwRoot.FullName "bin\windres.exe") -replace '\\', '/'
    $qtRootCmake = $qtRoot -replace '\\', '/'
    $qtCMakeDirFwd = $qtCMakeDir -replace '\\', '/'
    $cacheFile = Join-Path $buildDirectory "CMakeCache.txt"

    if (Test-Path $cacheFile) {
        $cachedCompiler = Get-CacheValue $cacheFile "CMAKE_CXX_COMPILER"
        $cachedPrefix = Get-CacheValue $cacheFile "CMAKE_PREFIX_PATH"
        $cachedQtDir = Get-CacheValue $cacheFile "Qt6_DIR"
        $needsReset = $false

        if ($cachedCompiler -and -not (PathsEqual $cachedCompiler ($compiler -replace '/', '\'))) {
            $needsReset = $true
        }
        if (-not $cachedPrefix -and -not $cachedQtDir) {
            $needsReset = $true
        } elseif ($cachedPrefix -and ($cachedPrefix -notlike "*$qtRoot*") -and
                  ($cachedPrefix -notlike ("*" + ($qtRoot -replace '\\', '/') + "*"))) {
            $needsReset = $true
        }

        if ($needsReset) {
            Write-Host "Toolchain or Qt path changed; resetting CMake cache..."
            Remove-Item -Force $cacheFile
            $cmakeFiles = Join-Path $buildDirectory "CMakeFiles"
            if (Test-Path $cmakeFiles) {
                Remove-Item -Recurse -Force $cmakeFiles
            }
        }
    }

    Write-Host "Configuring immich..."
    Write-Host "Using Qt: $qtRoot"
    & cmake -S $projectRoot -B $buildDirectory -G Ninja `
        "-DCMAKE_PREFIX_PATH=$qtRootCmake" `
        "-DQt6_DIR=$qtCMakeDirFwd" `
        "-DCMAKE_CXX_COMPILER=$compiler" `
        "-DCMAKE_RC_COMPILER=$rcCompiler" `
        "-DCMAKE_MAKE_PROGRAM=$($ninja -replace '\\', '/')"
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

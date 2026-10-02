# Publish a new immich-desktop version to GitHub Releases + Snap Store.
#
# Prerequisites:
#   - GitHub secret SNAPCRAFT_STORE_CREDENTIALS (already set on this repo)
#   - Push access to hdmain/immich-desktop
#
# What this does:
#   1. Bumps version in CMakeLists.txt + changelog.json (optional -ManualVersion)
#   2. Commits, tags vX.Y.Z, pushes
#   3. GitHub Actions builds Windows/Linux packages AND publishes snaps to stable
#
# Usage:
#   .\tools\publish-release.ps1 -Version 0.1.9 -Notes "Bug fixes and polish"
#   .\tools\publish-release.ps1 -Version 0.1.9 -Notes "..." -DryRun

param(
    [Parameter(Mandatory = $true)]
    [string]$Version,

    [Parameter(Mandatory = $true)]
    [string]$Notes,

    [switch]$DryRun,
    [switch]$SkipCommit
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if ($Version -notmatch '^\d+\.\d+\.\d+$') {
    throw "Version must look like 0.1.9"
}

$tag = "v$Version"
$cmake = Get-Content "CMakeLists.txt" -Raw
if ($cmake -notmatch 'project\(\s*immich\s*VERSION\s*[\d.]+') {
    throw "Could not find project(immich VERSION ...) in CMakeLists.txt"
}
$cmake = [regex]::Replace($cmake, '(project\(\s*immich\s*VERSION\s*)[\d.]+', "`${1}$Version", 1)
$changelog = @{
    version = $Version
    what_changed = $Notes
} | ConvertTo-Json -Depth 3

Write-Host "Preparing release $tag"
Write-Host "  Snap Store: https://snapcraft.io/immich-desktop"
Write-Host "  Channel:    stable (amd64 + arm64 via CI)"

if ($DryRun) {
    Write-Host "[DryRun] Would update CMakeLists.txt + changelog.json, commit, tag $tag, push"
    Write-Host $changelog
    exit 0
}

Set-Content -Path "CMakeLists.txt" -Value $cmake -NoNewline
# Preserve UTF-8 for changelog
[System.IO.File]::WriteAllText((Join-Path $root "changelog.json"), $changelog + "`n")

if (-not $SkipCommit) {
    git add CMakeLists.txt changelog.json
    git status --short
    $msg = "Release $tag"
    git commit -m $msg
    git tag -a $tag -m $msg
    git push origin HEAD
    git push origin $tag
    Write-Host ""
    Write-Host "Pushed $tag. Watch: gh run watch"
    Write-Host "Snap publish happens in the 'Snap package' job when the tag workflow finishes."
} else {
    Write-Host "Version files updated. Commit/tag/push yourself, then CI publishes to Snap Store."
}

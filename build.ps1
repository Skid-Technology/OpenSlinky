<#
  Builds Slinky.exe + SlinkyNative.dll (Release, x64) and copies both into .\dist
  Usage:   powershell -ExecutionPolicy Bypass -File .\build.ps1
           powershell -ExecutionPolicy Bypass -File .\build.ps1 -Clean     (throw away old build folder first)
  Needs:   Visual Studio (or Build Tools) with "Desktop development with C++", and CMake 3.15+
  Note:    the Java payload jars are already prebuilt in SlinkyNative\payload - no Java/Gradle needed.
#>
param([switch]$Clean)
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

function Fail($msg) { Write-Host "`n[X] $msg" -ForegroundColor Red; exit 1 }

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Fail "CMake not found. Install from https://cmake.org/download/ (tick 'Add CMake to PATH'), then open a NEW PowerShell window."
}
foreach ($j in 'named','srg','official') {
    if (-not (Test-Path "SlinkyNative\payload\slinky-$j.jar")) { Fail "Missing SlinkyNative\payload\slinky-$j.jar" }
}
if ($Clean -and (Test-Path build)) { Remove-Item -Recurse -Force build }

Write-Host "[1/3] Configuring..." -ForegroundColor Cyan
cmake -S . -B build -A x64
if ($LASTEXITCODE -ne 0) { Fail "Configure failed. Install the 'Desktop development with C++' workload, then re-run with -Clean." }

Write-Host "[2/3] Building (Release)..." -ForegroundColor Cyan
cmake --build build --config Release
if ($LASTEXITCODE -ne 0) { Fail "Build failed. Scroll up to the first line containing 'error'." }

$bin = Join-Path $PSScriptRoot 'build\bin\Release'
foreach ($f in 'Slinky.exe','SlinkyNative.dll') {
    if (-not (Test-Path (Join-Path $bin $f))) { Fail "$f was not produced in $bin" }
}
New-Item -ItemType Directory -Force dist | Out-Null
Copy-Item (Join-Path $bin 'Slinky.exe'), (Join-Path $bin 'SlinkyNative.dll') dist -Force
Write-Host "[3/3] Done -> dist\Slinky.exe  (keep SlinkyNative.dll in the same folder)" -ForegroundColor Green

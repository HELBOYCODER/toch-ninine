# Configures and builds the portable core. Fails on any compiler warning.
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

$log = if ($env:BUILD_LOG) { $env:BUILD_LOG } else { 'build_windows.log' }

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw "configure failed with exit code $LASTEXITCODE" }

cmake --build build --config Release 2>&1 | Tee-Object -FilePath $log
if ($LASTEXITCODE -ne 0) { throw "build failed with exit code $LASTEXITCODE" }

if (Select-String -Path $log -Pattern 'warning C\d+|warning:' -Quiet) {
    throw "the build produced compiler warnings (see $log)"
}

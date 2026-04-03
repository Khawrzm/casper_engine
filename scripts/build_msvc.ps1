<#
.SYNOPSIS
    Build NIYAH-CORE and the Casper trainer using MSVC (cl.exe).
    Supports x64 and ARM64 targets.

.USAGE
    .\scripts\build_msvc.ps1              # auto-detect arch
    .\scripts\build_msvc.ps1 -Arch arm64  # force ARM64
    .\scripts\build_msvc.ps1 -Arch x64    # force x64
    .\scripts\build_msvc.ps1 -Config Release
    .\scripts\build_msvc.ps1 -Config Debug

.OUTPUTS
    Core_CPP\niyah.exe   — inference smoke-test binary
    Core_CPP\trainer.exe — training simulation binary
    niyah_train.exe      — standalone C trainer
#>
[CmdletBinding()]
param(
    [ValidateSet("x64","arm64","auto")]
    [string]$Arch = "auto",

    [ValidateSet("Debug","Release")]
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")
$Root = (Get-Location).Path

# ── detect architecture ──────────────────────────────────────────────────────
if ($Arch -eq "auto") {
    $Arch = if ([System.Runtime.InteropServices.RuntimeInformation]::ProcessArchitecture -eq "Arm64") { "arm64" } else { "x64" }
}
Write-Host "[build_msvc] Target arch : $Arch"
Write-Host "[build_msvc] Config      : $Config"

# ── locate cl.exe via vswhere ─────────────────────────────────────────────────
function Find-VsDevCmd {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        $vswhere = "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
    }
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
        if ($vsPath) {
            $candidate = Join-Path $vsPath "Common7\Tools\VsDevCmd.bat"
            if (Test-Path $candidate) { return $candidate }
        }
    }
    # Fallback — well-known locations
    $fallbacks = @(
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
    )
    foreach ($fb in $fallbacks) { if (Test-Path $fb) { return $fb } }
    return $null
}

# ── compiler flags ────────────────────────────────────────────────────────────
# x64 Release: probe whether /arch:AVX2 is supported before using it.
# ARM64: NEON always available; __ARM_NEON auto-defined by MSVC ARM64 target.
function Test-ArchFlag([string]$flag) {
    $tmp = [System.IO.Path]::GetTempFileName() + ".c"
    "int main(void){return 0;}" | Set-Content $tmp
    $out = cmd /c "cl.exe /nologo $flag $tmp /Fe:nul 2>&1"
    Remove-Item $tmp -ErrorAction SilentlyContinue
    return ($LASTEXITCODE -eq 0 -and ($out -notmatch "D9002"))
}
# MSVC ARM64 generally does not require an explicit /arch flag.
$archFlag = if ($Arch -eq "arm64") { "" } `
            elseif ($Config -eq "Release" -and (Test-ArchFlag "/arch:AVX2")) { "/arch:AVX2" } `
            else { "" }
if ($archFlag) { Write-Host "[build_msvc] SIMD flag   : $archFlag" } `
else           { Write-Host "[build_msvc] SIMD flag   : (none — scalar fallback)" }

$commonFlags = @(
    "/nologo", "/W4", "/WX",
    "/wd4996",    # suppress 'deprecated' POSIX names (fopen etc)
    "/EHsc",
    $archFlag
) | Where-Object { $_ -ne "" }

$configFlags = if ($Config -eq "Release") {
    @("/O2", "/GL", "/DNDEBUG")
} else {
    # Keep debug flags broadly compatible across MSVC targets.
    @("/Od", "/Zi", "/RTC1")
}

# ── build function ────────────────────────────────────────────────────────────
function Invoke-ClBuild {
    param([string[]]$Sources, [string]$Out, [string[]]$ExtraFlags = @())

    $allFlags  = $commonFlags + $configFlags + $ExtraFlags
    $flagStr   = $allFlags -join " "
    $srcStr    = ($Sources | ForEach-Object { "`"$_`"" }) -join " "
    $cmd       = "cl.exe $flagStr $srcStr /Fe:`"$Out`""

    Write-Host "`n[build_msvc] Compiling: $Out"
    Write-Host "  cmd> $cmd"

    $result = cmd /c "$cmd 2>&1"
    $result | ForEach-Object { Write-Host "  $_" }

    if ($LASTEXITCODE -ne 0) {
        Write-Error "[build_msvc] FAILED (exit $LASTEXITCODE): $Out"
    }
    if (Test-Path $Out) {
        $sz = (Get-Item $Out).Length
        Write-Host "[build_msvc] OK  $Out  ($([math]::Round($sz/1KB,1)) KB)"
    }
}

# ── try cl.exe already in PATH ────────────────────────────────────────────────
$clInPath = Get-Command cl.exe -ErrorAction SilentlyContinue
if (-not $clInPath) {
    $vsDevCmd = Find-VsDevCmd
    if (-not $vsDevCmd) {
        Write-Error @"
[build_msvc] cl.exe not found and VsDevCmd.bat not located.
Install Visual Studio Build Tools (C++ workload):
  winget install -e --id Microsoft.VisualStudio.2022.BuildTools
"@
    }
    Write-Host "[build_msvc] Bootstrapping MSVC environment from:`n  $vsDevCmd"

    # Re-invoke this script inside the MSVC dev shell
    $scriptPath = $MyInvocation.MyCommand.Path
    $archArg    = "-Arch $Arch"
    $cfgArg     = "-Config $Config"
    $inner      = "powershell -NoProfile -ExecutionPolicy Bypass -File `"$scriptPath`" $archArg $cfgArg"
    cmd /c "`"$vsDevCmd`" -arch=$Arch -no_logo && $inner"
    exit $LASTEXITCODE
}

# ── build targets ─────────────────────────────────────────────────────────────
$niyahSrc      = @("$Root\Core_CPP\niyah_core.c", "$Root\Core_CPP\niyah_main.c")
$trainerSrc    = @("$Root\Core_CPP\trainer.cpp")
$niyahTrainSrc = @("$Root\Core_CPP\niyah_train.c", "$Root\Core_CPP\niyah_core.c", "$Root\tokenizer.c")

Invoke-ClBuild -Sources $niyahSrc      -Out "$Root\Core_CPP\niyah.exe"    -ExtraFlags @("/std:c17")
Invoke-ClBuild -Sources $trainerSrc    -Out "$Root\Core_CPP\trainer.exe"  -ExtraFlags @("/std:c++17")
Invoke-ClBuild -Sources $niyahTrainSrc -Out "$Root\niyah_train.exe"       -ExtraFlags @("/std:c17")

# ── checksums ─────────────────────────────────────────────────────────────────
Write-Host "`n[build_msvc] Artifact checksums (SHA256):"
foreach ($artifact in @("$Root\Core_CPP\niyah.exe", "$Root\Core_CPP\trainer.exe", "$Root\niyah_train.exe")) {
    if (Test-Path $artifact) {
        $hash = (Get-FileHash $artifact -Algorithm SHA256).Hash
        $sz   = [math]::Round((Get-Item $artifact).Length / 1KB, 1)
        Write-Host "  $hash  $($artifact | Split-Path -Leaf)  (${sz} KB)"
    }
}

Write-Host "`n[build_msvc] Build complete."

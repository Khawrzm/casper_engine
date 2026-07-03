# build_library_msvc.ps1 v5
# Final build script for casper.dll on MSVC.

$cl_check = Get-Command cl.exe -ErrorAction SilentlyContinue
if (-not $cl_check) {
    Write-Error "Compiler 'cl.exe' not found. Please run this script from the 'Developer PowerShell for VS 2022'."
    exit 1
}

$projectRoot = "C:\Users\Admin\casper_engine"
if ($PWD.Path -ne $projectRoot) { cd $projectRoot }

$buildDir = ".\build"
if (-not (Test-Path $buildDir)) { New-Item -ItemType Directory -Path $buildDir | Out-Null }

$sourceFiles = @(
    "Core_CPP\casper_ffi.c",
    "Core_CPP\niyah_core.c",
    "Core_CPP\hybrid_reasoner.c",
    "Core_CPP\constraint_solver.c",
    "Core_CPP\rule_parser.c",
    "Core_CPP\proof_generator.c",
    "Core_CPP\khz_q_svd.c",
    "tokenizer.c",
    "Core_CPP\casper_core.cpp"
)

# /I adds include paths. We now add BOTH 'include' and 'Core_CPP'.
# /FI forces the inclusion of win_time.h, polyfilling POSIX functions.
$compilerFlags = @(
    "/nologo", "/O2", "/DNDEBUG", "/LD",
    "/Iinclude",
    "/ICore_CPP",
    "/D_WIN32", "/D_CRT_SECURE_NO_WARNINGS", "/EHsc",
    "/FIinclude\win_time.h",
    "/DBUILDING_CASPER_DLL",
    "/Febuild\casper.dll"
)

$linkerFlags = @("/link", "/OUT:build\casper.dll")

$command = "cl.exe " + ($sourceFiles -join " ") + " " + ($compilerFlags -join " ") + " " + ($linkerFlags -join " ")

Write-Host "Executing final build command:" -ForegroundColor Yellow
Write-Host $command

Invoke-Expression $command

if ($LASTEXITCODE -eq 0) {
    Write-Host "SUCCESS: Created shared library at build\casper.dll" -ForegroundColor Green
} else {
    Write-Error "Build failed."
}

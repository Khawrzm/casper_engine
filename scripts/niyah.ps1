param(
    [Parameter(Position = 0)]
    [ValidateSet("build","corpus","train","smoke","bench","save","run","all")]
    [string]$Action = "all",

    [Parameter(Position = 1)]
    [string]$DataPath = "Data_Training/sovereign_knowledge.txt",

    [Parameter(Position = 2)]
    [int]$Epochs = 3,

    [Parameter(Position = 3)]
    [double]$Lr = 0.001,
    [double]$MinLr = 0.0001,

    [string]$Prompt = "بِسْمِ اللَّهِ",
    [int]$Tokens = 64,
    [string]$Model = "niyah_tiny.bin",
    [string]$Size = "tiny",
    [int]$Steps = 200,
    [double]$Temp = 0.8,
    [double]$TopP = 0.9,
    [int]$Seed = 42
)

$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

function Invoke-Build {
    Write-Host "[niyah] build..."
    powershell -ExecutionPolicy Bypass -File ".\scripts\build_msvc.ps1" -Config Release
}

function Invoke-Corpus {
    Write-Host "[niyah] corpus..."
    powershell -ExecutionPolicy Bypass -File ".\scripts\build_corpus.ps1"
}

function Invoke-Train {
    Write-Host "[niyah] train..."
    if (-not (Test-Path ".\niyah_train.exe")) {
        Invoke-Build
    }
    & ".\niyah_train.exe" $DataPath $Epochs $Lr $MinLr
}

function Invoke-Smoke {
    Write-Host "[niyah] smoke..."
    & ".\Core_CPP\niyah.exe" --mode smoke
}

function Invoke-Bench {
    Write-Host "[niyah] bench..."
    & ".\scripts\niyah.exe" --mode bench --steps $Steps --size $Size
}

function Invoke-Save {
    Write-Host "[niyah] save..."
    & ".\scripts\niyah.exe" --mode save --model $Model --size $Size
}

function Invoke-Run {
    Write-Host "[niyah] run..."
    & ".\scripts\niyah.exe" --mode run --model $Model --prompt $Prompt --tokens $Tokens --temp $Temp --topp $TopP --seed $Seed
}

switch ($Action) {
    "build"  { Invoke-Build }
    "corpus" { Invoke-Corpus }
    "train"  { Invoke-Train }
    "smoke"  { Invoke-Smoke }
    "bench"  { Invoke-Bench }
    "save"   { Invoke-Save }
    "run"    { Invoke-Run }
    "all" {
        Invoke-Build
        if (Test-Path ".\Data_Training\sources") {
            try { Invoke-Corpus } catch { Write-Host "[niyah] corpus skipped: $($_.Exception.Message)" }
        }
        Invoke-Train
        Invoke-Smoke
    }
}


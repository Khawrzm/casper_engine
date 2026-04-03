# Casper Engine

Local-first C/C++ training and inference workspace for NIYAH experiments.

## What works now

- `Core_CPP/niyah.exe`: smoke + benchmark binary for NIYAH core.
- `niyah_train.exe`: standalone trainer built from `Core_CPP/niyah_train.c`.
- `Core_CPP/trainer.exe`: C++ training simulation pipeline.
- `scripts/build_msvc.ps1`: MSVC build script for core binaries.
- `scripts/build_gcc.sh`: GCC/Clang build script with optional lint/smoke.

## Quick start (Windows, PowerShell)

```powershell
cd C:\Users\Iqd20\Casper_Engine

# one command for the full pipeline: build + train + smoke
.\scripts\niyah.ps1 all
```

### Single actions (unified entrypoint)

```powershell
.\scripts\niyah.ps1 build
.\scripts\niyah.ps1 corpus
.\scripts\niyah.ps1 train Data_Training/sovereign_knowledge.txt 3 0.001
.\scripts\niyah.ps1 smoke
.\scripts\niyah.ps1 bench -Steps 300 -Size tiny
.\scripts\niyah.ps1 save  -Model niyah_tiny.bin -Size tiny
.\scripts\niyah.ps1 run   -Model niyah_tiny.bin -Prompt "بِسْمِ اللَّهِ" -Tokens 64
```

## Notes

- `scripts/niyah.exe` and `Core_CPP/niyah.exe` are different binaries with different `--mode` support.
- `scripts/niyah.ps1` is the unified wrapper to avoid mode/path confusion.
- Training data is expected in `Data_Training/sovereign_knowledge.txt`.
- `niyah_train.exe` supports arguments:
  - `niyah_train.exe [data_path] [epochs] [lr] [min_lr]`
- Prefer relative paths; avoid hardcoded absolute user paths in new code.

## Project layout

- `Core_CPP/`: NIYAH core, smoke driver, training sources.
- `Data_Training/`: datasets.
- `Math_ASM/`: assembly experiments.
- `UI_CSharp/`: optional manager app.
- `scripts/`: build and run automation.

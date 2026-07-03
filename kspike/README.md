# KSpike Sovereign Defense System

KSpike is a dual-mode, active-response kernel defense framework, powered by the **Casper Engine** (a zero-dependency, C11 hybrid neuro-symbolic reasoning brain).

## Architecture
- **Casper Engine (Core):** Neural/Symbolic evaluator integrated via FFI (`casper.dll`).
- **KSpike Daemon:** Long-running defense process acting as the system's shield (`kspiked.exe`), communicating securely over TCP Loopback (`127.0.0.1:9999`).
- **KSpike TUI:** Interactive console for operators to monitor and interact with the defense layer (`kspike-tui.exe`).

## Quick Start (Windows)
1. Download the latest `kspike-win-x64.zip` from the **Releases** tab.
2. Extract the archive to your desired directory.
3. Open a terminal and start the Daemon:
   ```cmd
   .\kspiked.exe
1. Open a second terminal and start the Interactive Console (TUI):
.\kspike-tui.exe
Maintainer
Developed and maintained by the Khawrzm Sovereign Infrastructure Initiative.

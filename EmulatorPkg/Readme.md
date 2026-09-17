# EmulatorPkg Readme

## Overview

EmulatorPkg provides an environment where a UEFI environment can be emulated under an environment where a full UEFI
compatible environment is not possible. (For example, running under an OS where an OS process hosts the UEFI emulation
environment.)

<https://github.com/tianocore/tianocore.github.io/wiki/EmulatorPkg>

## Status

- Builds and runs under
  - a posix-like environment with X windows
    - Linux
    - OS X
  - Windows environment
    - Win10 (verified)
    - Win8 (not verified)

## How to Build & Run

**You can use the following command to build.**

- 32bit emulator in Windows:

  `build -p EmulatorPkg\EmulatorPkg.dsc -t VS2026 -a IA32`

- 64bit emulator in Windows:

  `build -p EmulatorPkg\EmulatorPkg.dsc -t VS2026 -a X64`

- 32bit emulator in Linux:

  `build -p EmulatorPkg\EmulatorPkg.dsc -t GCC -a IA32`

- 64bit emulator in Linux:

  `build -p EmulatorPkg\EmulatorPkg.dsc -t GCC -a X64`

**You can start/run the emulator using the following command:**

- 32bit emulator in Windows:

  `cd Build\EmulatorIA32\DEBUG_VS2026\IA32\ && WinHost.exe`

- 64bit emulator in Windows:

  `cd Build\EmulatorX64\DEBUG_VS2026\X64\ && WinHost.exe`

- 32bit emulator in Linux:

  `cd Build/EmulatorIA32/DEBUG_GCC/IA32/ && ./Host`

- 64bit emulator in Linux:

  `cd Build/EmulatorX64/DEBUG_GCC/X64/ && ./Host`

**On posix-like environment with the bash shell you can use EmulatorPkg/build.sh to simplify building and running
emulator.**

For example, to build + run:

`$ EmulatorPkg/build.sh`

`$ EmulatorPkg/build.sh run`

The build architecture will match your host machine's architecture.

On X64 host machines, you can build + run IA32 mode as well:

`$ EmulatorPkg/build.sh -a IA32`

`$ EmulatorPkg/build.sh -a IA32 run`

## Fork perso : mode "no GOP / console série"

Cette branche (`feature/emulatorpkg-no-gop-console`) désactive la fenêtre graphique (GOP) et route l'entrée/sortie
UEFI sur un terminal série VT100, pour un usage headless / scriptable (CI, shift-left testing).

### Modifications

- [EmulatorPkg.dsc](EmulatorPkg.dsc) : `PcdEmuGop` vidée (`L""`) et forcée en `<PcdsFixedAtBuild>` sur `Host.inf`,
  console élargie à `240x56`, timeout de boot réduit à 3s
- [Library/PlatformBmLib/PlatformBmData.c](Library/PlatformBmLib/PlatformBmData.c) : ajout d'un device path console
  série (VT100, 115200 8N1) dans `gPlatformConsole`
- [Unix/Host/PosixFileSystem.c](Unix/Host/PosixFileSystem.c) : correction du calcul d'année (`tm_year` compte depuis
  1900, il manquait le `+ 1900`)
- [Unix/host.sh](Unix/host.sh) : script de lancement qui désactive `icrnl` sur le terminal hôte (sinon la touche
  Entrée casse la saisie dans l'EFI Shell, la console série VT100 gérant elle-même le CR) et restaure le terminal
  à la sortie (`trap ... EXIT INT TERM`), même en cas de crash ou de Ctrl+C

### Build avec la toolchain Clang/LLVM

```bash
# depuis la racine du workspace edk2 (édition initialisée : `source edksetup.sh`)
build -p EmulatorPkg/EmulatorPkg.dsc -a X64 -t CLANGDWARF
```

### Lancer l'émulateur (console)

```bash
cp EmulatorPkg/Unix/host.sh Build/EmulatorX64/DEBUG_CLANGDWARF/X64/
cd Build/EmulatorX64/DEBUG_CLANGDWARF/X64/
./host.sh
```

Le script étant volontairement simple (cf. décision "priorité à la simplicité"), il doit être recopié manuellement
dans le dossier `Build/.../X64/` après chaque build tant qu'aucune automatisation n'est en place.

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

### Pourquoi il n'y a aucun fichier de log par défaut

Comme pour `build` (voir tableau ci-dessus), rien n'est jamais persisté sur disque par défaut : tout part sur les
flux du process `Host`. En creusant le code source, deux canaux bien distincts existent :

- **Console UEFI Shell (interactive)** : `stdin`/`stdout` réels du process (`gEmuThunk->ConfigStdIn` /
  `WriteStdOut` dans [Library/DxeEmuSerialPortLib/DxeEmuSerialPortLib.c](Library/DxeEmuSerialPortLib/DxeEmuSerialPortLib.c)),
  reliés à votre nouveau device path console VT100.
- **Boot log PEI/DXE (`DEBUG()`)** : redirigé vers `stderr` du process, indépendamment de la console UEFI
  (`gEmuThunk->WriteStdErr` → `write(STDERR_FILENO, ...)` dans
  [Unix/Host/EmuThunk.c](Unix/Host/EmuThunk.c), voir aussi le mapping `SerialPortLib|...DxeEmuStdErrSerialPortLib...`
  dans [EmulatorPkg.dsc](EmulatorPkg.dsc) lignes 401/414).

Ces deux flux étant séparés (stdout vs stderr), `host.sh` peut rediriger uniquement `stderr` vers un fichier
**sans jamais toucher `stdin`/`stdout`** : le Shell reste 100% interactif, seul le boot log est capturé.
C'est ce que fait le script par défaut (`debug_boot.log` à côté du binaire `Host`) :

```bash
./host.sh                       # boot log -> ./debug_boot.log, Shell interactif inchangé
LOGFILE=/tmp/run1.log ./host.sh # nom de fichier personnalisé
```

### Logger la sortie des commandes du Shell UEFI (ex: `acpiview`)

Le contenu du Shell UEFI (ce que produit `acpiview`, `dh`, etc.) transite par `stdout`, pas par `stderr` : il ne
suffit pas de rediriger `stdout` vers un fichier, sinon plus rien ne s'affiche à l'écran. `host.sh` duplique donc
`stdout` vers le terminal **et** vers un fichier via `tee` (process substitution bash `> >(tee ...)`, qui préserve
le code de sortie réel de `Host`, contrairement à un simple `| tee`) :

```bash
./host.sh                              # sortie Shell -> ./shell_console.log (+ affichée normalement)
SHELL_LOGFILE=/tmp/acpiview.log ./host.sh
```

Le fichier contient les séquences d'échappement VT100 (couleurs, positionnement curseur) puisque c'est une vraie
émulation de terminal. Pour le relire proprement une fois la session terminée :

```bash
sed -r 's/\x1B\[[0-9;]*[a-zA-Z]//g' shell_console.log | less
```

Le script étant volontairement simple (cf. décision "priorité à la simplicité"), il doit être recopié manuellement
dans le dossier `Build/.../X64/` après chaque build tant qu'aucune automatisation n'est en place.

### Journal de mise au point du build CLANGDWARF (erreurs, causes, remèdes)

Premier build après clone : plusieurs erreurs successives, résolues une à une. Dans l'ordre rencontré :

| # | Erreur | Cause | Remède |
|---|---|---|---|
| 1 | `File/directory not found in workspace ... mipisyst/library/include` (puis `mbedtls/include`, `libspdm/include`) | Les submodules git ne sont pas initialisés après clone. Le validateur de méta-données edk2 (`build.py`) **scanne tous les `.dec` du workspace au démarrage**, pas seulement ceux réellement utilisés par `EmulatorPkg.dsc` : impossible de cibler finement, il faut tous les initialiser | `git submodule update --init --depth 1` (sur tous les submodules ; `--depth 1` limite fortement la conso de data vs un clone complet de l'historique) |
| 2 | `Command 'build' not found` | `edksetup.sh` ajoute au PATH un wrapper Python (`BaseTools/BinWrappers/PosixLike/build`), mais les outils C de BaseTools (dont dépend ce wrapper) ne sont pas encore compilés | `make -C BaseTools` (nécessite `make`, absent par défaut de l'image WSL minimale) |
| 3 | `Command 'make' not found` | Paquet `build-essential` non installé (WSL minimal, pas de outils de compilation C de base) | `sudo apt install -y build-essential uuid-dev nasm` |
| 4 | `llvm-ar: not found` (`make tbuild` échoue avec `Error 127`) | La toolchain `CLANGDWARF` attend des binaires **non versionnés** (`llvm-ar`, `llvm-objcopy`) sur le `PATH`. Nos paquets Ubuntu installent des binaires versionnés (`llvm-ar-22`, `llvm-objcopy-22`) | `sudo update-alternatives --install /usr/bin/llvm-ar llvm-ar /usr/bin/llvm-ar-22 100` (idem pour `llvm-objcopy`) |
| 5 | `X11GraphicsWindow.c:18:10: fatal error: 'X11/Xlib.h' file not found` | Le PCD `PcdEmuGop|L""` désactive le GOP **à l'exécution**, mais `EmulatorPkg/Unix/Host/Host.inf` compile toujours `X11GraphicsWindow.c` (le fichier source n'est pas retiré de la liste `[Sources]`). Il faut donc les headers X11 même en mode console | `sudo apt install -y libx11-dev libxext-dev` (`libxext-dev` manquait réellement ; `libx11-dev` était déjà présent) |
| 6 | *(anticipée, pas rencontrée sur ce build précis)* — build ACPI nécessitant `iasl` | Le paquet Ubuntu ne s'appelle **pas** `iasl` mais `acpica-tools` (qui fournit le binaire `iasl`) | `sudo apt install -y acpica-tools` (installé par anticipation ; non invoqué par ce build EmulatorPkg X64 précis, mais nécessaire dès qu'un module génère des tables ACPI) |

Résultat final : `- Done -`, `Host` généré dans `Build/EmulatorX64/DEBUG_CLANGDWARF/X64/Host`.

**Piste d'amélioration "carrée" pour plus tard** : si le mode no-GOP doit vraiment retirer la dépendance X11 (et pas
seulement la désactiver au runtime), il faudra rendre `X11GraphicsWindow.c`/`WinGopScreen.c` conditionnels dans
`[Sources]` de `Host.inf` (ex: via une macro `!ifdef HEADLESS_BUILD`), ce qui évitera d'installer `libx11-dev` du
tout sur les environnements CI headless.

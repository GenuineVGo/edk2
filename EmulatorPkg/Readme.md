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
- [Unix/Host/EmuThunk.c](Unix/Host/EmuThunk.c) : `SecReadStdIn` remape DEL (`0x7F`, ce qu'envoient la plupart des
  terminaux Linux pour la touche Retour arrière) vers BS (`0x08`, ASCII), seul octet reconnu par l'éditeur de
  ligne du Shell UEFI (VT100). Sans ce correctif, l'effacement visuel fonctionne mais **la commande réellement
  exécutée reste fausse silencieusement** (découvert via [Unix/tests/validate_console_poc.py](Unix/tests/validate_console_poc.py))
- [Unix/host.sh](Unix/host.sh) : script de lancement qui désactive `icrnl` sur le terminal hôte (sinon la touche
  Entrée casse la saisie dans l'EFI Shell, la console série VT100 gérant elle-même le CR), restaure le terminal
  à la sortie (`trap ... EXIT INT TERM`) et duplique stdout + stderr vers le terminal et `debug.log`

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

### Journal d'exécution

`host.sh` regroupe volontairement les deux flux du process `Host` dans un seul fichier `debug.log`, tout en les
laissant visibles dans le terminal grâce à `tee` :

- **Console UEFI Shell (interactive)** : `stdin`/`stdout` réels du process (`gEmuThunk->ConfigStdIn` /
  `WriteStdOut` dans [Library/DxeEmuSerialPortLib/DxeEmuSerialPortLib.c](Library/DxeEmuSerialPortLib/DxeEmuSerialPortLib.c)),
  reliés à votre nouveau device path console VT100.
- **Boot log PEI/DXE (`DEBUG()`)** : écrit sur `stderr` du process
  (`gEmuThunk->WriteStdErr` → `write(STDERR_FILENO, ...)` dans
  [Unix/Host/EmuThunk.c](Unix/Host/EmuThunk.c), voir aussi le mapping `SerialPortLib|...DxeEmuStdErrSerialPortLib...`
  dans [EmulatorPkg.dsc](EmulatorPkg.dsc) lignes 401/414).

Les deux flux sont fusionnés par `2>&1` avant le `tee`. `stdin` reste attaché au terminal : le Shell reste donc
interactif, tandis que stdout et stderr sont enregistrés ensemble dans `debug.log`, à côté du binaire `Host` :

```bash
./host.sh                       # sortie terminal + ./debug.log
cat debug.log                   # relire la session complète
```

### Logger la sortie des commandes du Shell UEFI (ex: `acpiview`)

Le contenu du Shell UEFI (ce que produit `acpiview`, `dh`, etc.) transite par `stdout`. `host.sh` le duplique vers
le terminal et vers `debug.log` via `tee`, avec le boot log `stderr` dans le même fichier :

```bash
./host.sh                              # sortie Shell + boot log -> ./debug.log
```

Le fichier contient les séquences d'échappement VT100 (couleurs, positionnement curseur) puisque c'est une vraie
émulation de terminal. Pour le relire proprement une fois la session terminée :

```bash
sed -r 's/\x1B\[[0-9;]*[a-zA-Z]//g' debug.log | less
```

### Validation automatisée du PoC console (Entrée, Backspace, `reset -s`, stabilité)

[Unix/tests/validate_console_poc.py](Unix/tests/validate_console_poc.py) automatise, via `pexpect`, la plus petite
validation reproductible du switch GOP -> console : boot jusqu'au prompt `Shell>`, frappe d'une commande erronée
corrigée au Backspace (octet DEL `0x7F`, comme un vrai clavier) puis validée par Entrée (CR `\r`, pas LF), sortie
via `reset -s`, le tout répété 3 fois pour prouver la stabilité et la bonne restauration du TTY.

```bash
sudo apt install -y python3-pexpect   # une fois
cd EmulatorPkg/Unix/tests && python3 validate_console_poc.py
```

Deux pièges découverts en écrivant ce script, à connaître pour tout futur pilotage du Shell (pytest, CI...) :
- la console UEFI n'accepte que **CR** (`\r`) pour Entrée, pas LF (`\n`, envoyé par défaut par la plupart des
  outils d'automatisation type `pexpect.sendline`) ;
- le flux affiché contient des séquences VT100 **entre chaque caractère** (repositionnement curseur), donc un
  simple `pattern.search("Shell>")` échoue — il faut un motif tolérant aux échappements intercalés (voir la
  fonction `fuzzy()` du script).

Le script étant volontairement simple (cf. décision "priorité à la simplicité"), il doit être recopié manuellement
dans le dossier `Build/.../X64/` après chaque build tant qu'aucune automatisation n'est en place.

### Activer `acpiview`

`smbiosview` est déjà fourni par `UefiShellDebug1CommandsLib`. Pour ajouter `acpiview` au Shell EmulatorPkg,
ajouter dans le bloc `ShellPkg/Application/Shell/Shell.inf` du `EmulatorPkg.dsc` :

```ini
NULL|ShellPkg/Library/UefiShellAcpiViewCommandLib/UefiShellAcpiViewCommandLib.inf
```

Le mapping doit être `NULL|` : `Shell.inf` ne déclare pas la classe `AcpiViewCommandLib`, mais le constructeur de
la bibliothèque enregistre la commande au démarrage du Shell. Aucun ajout FDF n'est nécessaire, car la bibliothèque
est liée dans l'application Shell déjà embarquée.

Après rebuild, `acpiview` est reconnu. Dans l'EmulatorPkg vanilla actuel, son exécution peut ensuite répondre
`Failed to find ACPI Table Guid in System Configuration Table.` : cela signifie que la commande fonctionne mais
qu'aucune table ACPI n'est encore publiée par le modèle EmulatorPkg, et non que l'intégration de la commande a
échoué.

Pour obtenir un dump binaire ACPI dans le répertoire host exposé par l'Emulator, sélectionner d'abord le volume
virtuel `FS0:` dans le Shell :

```text
Shell> FS0:
FS0:\> acpiview -s PPTT -d
FS0:\> ls *.bin
PPTT0000.bin
```

`acpiview -d` écrit dans le répertoire courant du Shell. Avec `FS0:` sélectionné, ce répertoire correspond au
filesystem host configuré par `PcdEmuFileSystem|L"."`, donc au dossier depuis lequel `host.sh` a lancé `Host`
(normalement `Build/EmulatorX64/DEBUG_CLANGDWARF/X64/`). Le fichier `PPTT0000.bin` est ainsi directement
récupérable par les outils host, sans parser le texte VT100 de la sortie console.

Si le Shell démarre sur un autre volume ou dans un répertoire non writable, `acpiview -d` affiche :
`Unable to write to the current directory, check if media is writable.` Dans ce cas, utiliser `FS0:` puis vérifier
le répertoire courant avec `pwd` avant de relancer la commande.

### Première table ACPI : PPTT constante Rhea1

Le driver [PpttDxe/PpttDxe.c](PpttDxe/PpttDxe.c) publie une PPTT entièrement déterministe via
`EFI_ACPI_TABLE_PROTOCOL`, sans `PlatformInfo`, HOB, PCD dynamique ni mise à jour de la topologie à l'exécution.
`AcpiTableDxe` fournit le protocole ACPI générique et est embarqué avec `PpttDxe` dans `EmulatorPkg.dsc` et
`EmulatorPkg.fdf`.

La constante décrit :

```text
2 sockets
128 cores par socket
1 SLC/L3 partagé par socket
1 L2 privé par core
1 L1D privé par core
1 L1I privé par core
```

La hiérarchie est `Board -> socket -> core`; aucun nœud cluster intermédiaire n'est ajouté. Les IDs processeur suivent
`ACPI_CPU_ID_ENCODE(socket, core)`. Les paramètres cache sont ceux de la référence Ampere/PPTT fournie, avec le
SLC Rhea1 fixé à 80 MiB par socket :

```text
L1I/L1D : 64 KiB, 256 sets, associativité 4, ligne 64 octets, IDs 0x11/0x10
L2      : 1 MiB, 2048 sets, associativité 8, ligne 64 octets, ID 0x20
SLC     : 80 MiB par socket, ID 0x30
```

Tous les flags de propriété cache sont valides. Chaque `CoreRecord` regroupe localement L1I, L1D, L2 et le nœud
processor correspondant. Chaque core possède deux ressources privées (L1I et L1D) ;
L1I/L1D pointent vers le L2 par `NextLevelOfCache`, et le L2 pointe vers le SLC de son socket.

Après rebuild, `acpiview -s PPTT -d` produit un dump binaire de 29 836 octets, décodable par `iasl -d`. La table
contient donc maintenant le header, le nœud `Board`, 770 caches, 2 nœuds socket et 256 nœuds core. La cohérence avec la MADT sera
validée séparément : la PPTT décrit la capacité/topologie constante, tandis que la MADT reste la source de découverte
et d'activation des processeurs.

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

#!/bin/bash
# Tester-side launcher for EmulatorPkg Host.
cd "$(dirname "$0")" || exit 1

if [ -t 0 ]; then
    SAVED_TTY=$(stty -g)
    trap 'stty "$SAVED_TTY"' EXIT INT TERM
    stty -icrnl
fi

# stdin stays attached to the terminal (interactive UEFI Shell input, e.g. acpiview).
# stdout is duplicated to the terminal AND to SHELL_LOGFILE via `tee` (process substitution,
# hence bash and not POSIX sh: keeps Host's own exit code, unlike a plain `| tee` pipe).
# stderr (PEI/DXE DEBUG() boot log) is captured separately, see EmulatorPkg/Readme.md.
LOGFILE="${LOGFILE:-debug_boot.log}"
SHELL_LOGFILE="${SHELL_LOGFILE:-shell_console.log}"
./Host "$@" > >(tee "$SHELL_LOGFILE") 2>"$LOGFILE"
exit $?

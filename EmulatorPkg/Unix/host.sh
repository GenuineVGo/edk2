#!/bin/bash
# Tester-side launcher for EmulatorPkg Host.
cd "$(dirname "$0")" || exit 1

if [ -t 0 ]; then
    SAVED_TTY=$(stty -g)
    trap 'stty "$SAVED_TTY"' EXIT INT TERM
    stty -icrnl
fi

# stdin stays attached to the terminal (interactive UEFI Shell input, e.g. acpiview).
# stdout (shell) and stderr (PEI/DXE DEBUG() boot log) are duplicated to the terminal
# AND to debug.log via `tee` (process substitution, hence bash and not POSIX sh: keeps Host's
# own exit code, unlike a plain `| tee` pipe).
LOGFILE="${LOGFILE:-debug.log}"
./Host "$@" 2>&1 | tee debug.log
exit $?

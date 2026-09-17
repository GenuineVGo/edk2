#!/bin/sh
# Tester-side launcher for EmulatorPkg Host.
cd "$(dirname "$0")" || exit 1

if [ -t 0 ]; then
    SAVED_TTY=$(stty -g)
    trap 'stty "$SAVED_TTY"' EXIT INT TERM
    stty -icrnl
fi

# stdin/stdout stay attached to the terminal (interactive UEFI Shell console).
# Only stderr (PEI/DXE DEBUG() boot log) is captured to a file, see EmulatorPkg/Readme.md.
LOGFILE="${LOGFILE:-debug_boot.log}"
./Host "$@" 2>"$LOGFILE"
exit $?

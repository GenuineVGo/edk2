#!/bin/sh
# Tester-side launcher for EmulatorPkg Host.
cd "$(dirname "$0")" || exit 1

if [ -t 0 ]; then
    SAVED_TTY=$(stty -g)
    trap 'stty "$SAVED_TTY"' EXIT INT TERM
    stty -icrnl
fi

./Host "$@"
exit $?

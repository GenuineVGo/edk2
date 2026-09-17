#!/usr/bin/env python3
"""Smallest reproducible validation of the GOP -> console PoC (verrou #1).

Checks, in a single automated run repeated 3x:
- Shell prompt appears (console visible)
- Backspace correction is honored (Enter + Backspace interpreted correctly)
- reset -s terminates cleanly and the host TTY settings are restored

Disposable script, not part of the tracked repo (lives under Build/, gitignored).
"""
import re
import subprocess
import sys
from pathlib import Path

import pexpect

HOST_DIR = Path(__file__).parent / "../../../Build/EmulatorX64/DEBUG_CLANGDWARF/X64"
HOST_DIR = HOST_DIR.resolve()
RUNS = 3
ANSI_RE = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")


def fuzzy(literal):
    """Build a regex matching `literal` even with ANSI escapes between each char.

    The emulated VT100 console redraws the cursor position before every single
    character, so a plain literal never appears contiguous in the raw stream.
    """
    return "".join(re.escape(c) + r"(?:\x1b\[[0-9;]*[A-Za-z])*" for c in literal)


def strip_ansi(text):
    return ANSI_RE.sub("", text)


def get_tty_settings():
    return subprocess.run(["stty", "-g"], capture_output=True, text=True, check=True).stdout.strip()


def run_once(run_index):
    print(f"--- run {run_index} ---")
    child = pexpect.spawn(str(HOST_DIR / "host.sh"), cwd=str(HOST_DIR), timeout=60, encoding="utf-8")
    child.linesep = "\r"  # UEFI Shell (raw terminal) expects CR for Enter, not LF; see EmulatorPkg/Readme.md
    child.expect(fuzzy("Shell>"))

    # Type a wrong command, correct it with Backspace, validate with Enter.
    # Sends the *real* DEL (0x7F), as most Linux terminal emulators actually do for the
    # Backspace key. EmuThunk.c's SecReadStdIn remaps 0x7F -> 0x08 (BS), which the UEFI
    # Shell's VT100 console line editor recognizes; 0x08 alone reached the line editor
    # correctly even before that fix, but 0x7F did not (see EmulatorPkg/Readme.md).
    child.send("echoo")
    child.send("\x7f")  # 1x Backspace (DEL, as a real keyboard sends it): "echoo" -> "echo"
    child.sendline(" backspace-ok")
    child.expect(fuzzy("Shell>"))
    output_before_prompt = strip_ansi(child.before)
    # NOTE: the console visually redraws "echoo" as typed (rendering artifact of this
    # simplistic per-character cursor-positioned text console) regardless of Backspace,
    # so the on-screen text is NOT a reliable signal. What matters is what the Shell's
    # command-line parser actually received: if Backspace correctly turned "echoo" into
    # "echo" (a valid command), running it produces no error and echoes "backspace-ok".
    assert "is not recognized" not in output_before_prompt, (
        f"run {run_index}: Shell rejected the corrected command -> Backspace not interpreted "
        f"correctly (buffer still contained 'echoo' instead of 'echo'):\n{output_before_prompt!r}"
    )
    assert "backspace-ok" in output_before_prompt, (
        f"run {run_index}: 'echo backspace-ok' did not produce the expected output:\n{output_before_prompt!r}"
    )
    print(f"run {run_index}: Enter + Backspace OK")

    child.sendline("reset -s")
    child.expect(pexpect.EOF, timeout=30)
    child.close()
    assert child.exitstatus == 0, f"run {run_index}: Host did not exit cleanly (exitstatus={child.exitstatus})"
    print(f"run {run_index}: reset -s clean exit OK")


def main():
    tty_before = get_tty_settings()
    for i in range(1, RUNS + 1):
        run_once(i)
    tty_after = get_tty_settings()
    assert tty_before == tty_after, "host.sh did not restore TTY settings (stty -g differs before/after)"
    print(f"TTY settings preserved across {RUNS} runs: OK")
    print("PoC GOP -> console: Enter, Backspace, reset -s, stability (x3) all validated.")


if __name__ == "__main__":
    try:
        main()
    except AssertionError as exc:
        print(f"VALIDATION FAILED: {exc}", file=sys.stderr)
        sys.exit(1)

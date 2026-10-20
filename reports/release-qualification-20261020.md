# Bounded-scanner release qualification — 2026-10-20

Release: `0.1.0-bounded.1` (prerelease). This receipt qualifies synthetic
bounds/input/output behavior only. It does not qualify C166/C167 semantics,
firmware authenticity, tuning, flashing, GPU/ECU control, or hardware safety.

## Provenance and preserved variants

Active-code commit: `23ff48b528a2e1a1ec212546299f670bbd5f7383`.
The release adds only receipt/ignore/manifest updates after that code commit.
Integration includes PR #1 head `e4500d7ce41eeec960a07f5f0fe9ce6e036242ad`
and corrective PR #2 head `3aac4da0143b8d63b6165497f5aa3c8da9d2b506`
as exact ancestors. The separate patch-1 source is preserved without merging it.
See [the source manifest](../PRESERVED_SOURCES.md). Only public repository
inputs and newly written synthetic tests/docs/build files were used.

## Per-script review ledger

| Path | Inspected | Changed | Freshly tested | Remaining scope |
| --- | --- | --- | --- | --- |
| `decompiler.c` | All source, CLI, file lifecycle, stepping/output errors | Bounds/FIFO fixes from PRs; help/version; detect listing write error; LF normalization; remove unused quit helper | Real compiler; 18 tests; 1,040 direct section/guard-page checks | Table semantics unverified; concurrent file mutation excluded |
| `test_static_bounds.py` | All fixture construction, compiler arguments, subprocess/temp cleanup | PR2 repairs; missing compiler diagnostic; help/version, external UTF-8/space path, output error regressions | Default/strict/sanitized runs plus clean archive | POSIX build required; no native Windows test |
| `test_static_bounds.c` | All cases and guard-page mapping/cleanup | Child PR implementation retained | All 1,040 checks in each suite | Expectations derive from existing size table, not ISA manual |
| `Makefile` | Every build/test/install command and configurable path | Added prefix/DESTDIR install and CC/CFLAGS support | Clean archive build/install, prefix with spaces and UTF-8 | No registry packaging or native Windows installer |
| `patch-1:decompiler.c` | Provenance/blob equality to original main checked | Excluded historical source, preserved by ref | Not separately qualified | Original unsafe scanner remains historical |
| `patch-1:decompile46 4 years later` | One-byte newline placeholder checked | Excluded historical placeholder, preserved by ref | Not executable | No program to qualify |

There are no other active scripts/modules in the reviewed Git tree. Historical
receipt/docs are evidence, not runtime entrypoints. No private inputs, firmware,
customer data, credentials, local configs, or generated scan output were tracked.
No AGENTS.md was present. Repository ignore rules, local excludes, tracked files,
staged diff and source-archive membership were inspected. No global ignore rule
was changed and nothing was force-added.

## Fresh execution

Environment: Ubuntu 24.04 under WSL2, Linux
`6.6.87.2-microsoft-standard-WSL2`, x86_64; GCC 13.3.0
(`Ubuntu 13.3.0-6ubuntu2~24.04.1`), Python 3.12.3, GNU Make 4.3.
The environment initially lacked `cc`; standard Ubuntu build-essential was
installed to enable actual-source tests. No hardware or firmware was accessed.

```sh
python3 -m unittest -v test_static_bounds.py
CFLAGS='-O2 -pedantic -Wconversion -Wsign-conversion' python3 -m unittest -v test_static_bounds.py
CFLAGS='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie' ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 python3 -m unittest -v test_static_bounds.py
```

All three runs passed **18 tests** with **1,040 direct scanner checks** each;
no compiler or address/undefined-behavior diagnostics were observed. Leak
detection was disabled; no successful leak check is claimed.

A fresh `git archive` of the candidate was extracted into a temporary path with
spaces. The suite passed from that clean source; `make` and
`make install PREFIX='<temporary>/installed prefix é日本'` passed. The installed
executable ran outside its source checkout on a synthetic input named
`synthetic image é日本.bin`. Its final line was `[0fffe][cc 00] nop 00`, its
exit was 0, and the input SHA-256 was unchanged. Missing input returned 2;
help/version worked. `CC=decompile46-no-such-compiler` failed with the intended
missing-compiler diagnostic. No files other than owned temporary/build/install
products were written.

The PR2 baseline returned **0** when stdout was `/dev/full`, falsely reporting
success for an unwritten listing. The new regression requires exit 2 and the
`cannot write complete listing` diagnostic. Boundaries, unsupported opcodes,
argument errors, size limits, FIFO/directory refusal and read-only hashes are
covered by the full suite.

## Exact clean-source SHA-256

| File | SHA-256 |
| --- | --- |
| `decompiler.c` | `caf81e0252f0fdd499bcc1da05b9f59d2272cfb68a62027a8e9b757a3c781c6c` |
| `test_static_bounds.py` | `0f10612bda88b20ec2475f697954735cc077520fd2cbecdc255a028f7659fb44` |
| `test_static_bounds.c` | `70873df3d1b1992dc5bfbe012bd026840b1d72f24709895615d833f8069048ab` |
| `Makefile` | `804b989786a66744794f89ec6c2ca7a2ad21ced8f524b77b11d9f4a321f3e957` |

Installed Linux CLI build SHA-256:
`2e4772828a7d15909ace3adfcb4369ce517df79e817b7f88530de9895a72b9af`
(`make`, default `-O2 -std=c11 -Wall -Wextra -Werror`). This build hash is
environment-specific; no native Windows executable is supplied.
Release assets carry their own checksum manifest and exact tagged source commit.

## Boundaries of qualification

Linux/WSL is freshly qualified. Native Windows, macOS, other architectures and
toolchains are untested. The scanner opens input read-only but mmap is not a
snapshot: keep inputs stable while scanning. Partial output may precede a HOLD.
Check exit status. Shell redirection to the input alias truncates the input
before execution and is outside the program's control; use a new output path.
No claim of complete CPU semantic correctness or high-level decompilation is
made. These limits remain explicit in the prerelease notes.

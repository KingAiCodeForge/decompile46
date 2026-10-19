# Synthetic bounds test receipt — 2026-10-19

## Provenance and scope

- Repository: `KingAiCodeForge/decompile46`.
- Starting point: draft [PR #1](https://github.com/KingAiCodeForge/decompile46/pull/1),
  branch `review/ms42-c166-safe-bounded-disassembly-20261019`.
- Exact starting commit: `e4500d7ce41eeec960a07f5f0fe9ce6e036242ad`, rechecked before publication.
- Original C blob: `0ba8b625d758481825663ddd46817df50c73ffee`.
- Original Python test blob: `f8068a7109683a74e2079c4c7bc86f652e0abbac`.
- Those exact blobs were retrieved and their Git object IDs matched locally.
- Execution: isolated cloud scratch directory; Linux 6.18.55 x86_64,
  GCC/cc 14.2.0 (Debian 14.2.0-19), Python 3.12.14.
- Only synthetic byte fixtures were processed. No real firmware was loaded or
  executed, no hardware was accessed, and no GitHub Actions jobs were requested.
- Review changes only; no merge or default-branch update.

## Reproduced failures

The original real C file compiled with `-std=c11 -Wall -Wextra -Werror`.
Running the exact original Python suite produced **4 tests, 3 failures**:

- `test_exact_first_64k_safe_input`: the purported 65,536-byte fixture was
  262,144 bytes. It started with bytes `5c 78 43 43 5c 78 30 30` (literal
  backslash-x text), and no expected final `cc 00` line appeared.
- `test_length_four_instruction_at_end_refused`: exit 0, expected 2. The
  replacement was textual bytes outside the scanned first 64 KiB.
- `test_unsupported_opcode_holds_without_unknown_length_guess`: exit 0,
  expected 2. The first byte was `5c`, not `3b`.
- The short-file refusal passed despite the wrong fixture bytes.

Changing only the escaped literals to real bytes made those original four
regressions pass against the unchanged original C implementation.

An additional synthetic FIFO with no writer reproduced a separate input guard
problem: the original CLI blocked in `open` and exceeded a one-second subprocess
timeout before it could reject a non-regular file. The production fix adds
`O_NONBLOCK` to the read-only open; the subsequent `fstat` rejects the FIFO.

## Changes tested

- Replace fragile escaped fixture literals with `bytes.fromhex` and explicit
  byte/length assertions.
- Compile the real CLI and include its actual static scanner in a C harness.
- Expand to 15 Python test methods, including 1,040 direct C checks.
- Cover 16 explicit null, empty, reversed, outside-image, converted-negative,
  nonzero-start, and truncated-section cases.
- Cover every byte value as an opcode at each of 1, 2, 3, and 4 available bytes
  (1,024 checks). Each image ends immediately before a `PROT_NONE` guard page.
- Verify exact first-64-KiB success, exact four-byte completion, HOLD at the
  file/section boundary even when extra file bytes exist, unsupported first and
  last opcodes, ignored trailing section bytes, size limits, missing/directory/
  FIFO paths, argument count, and fixture preservation hashes.
- Fix only the reproduced FIFO open hang in production C. The opcode table and
  section-stepping algorithm are unchanged.

## Commands and observed results

```sh
python3 -m unittest -v test_static_bounds.py
```

**PASS:** 15 tests in 0.439 seconds. This compiles both C executables with
`-std=c11 -Wall -Wextra -Werror`; the direct harness reported
`PASS: 1040 direct section checks`.

```sh
CFLAGS='-O2 -pedantic -Wconversion -Wsign-conversion' python3 -m unittest -v test_static_bounds.py
```

**PASS:** 15 tests in 0.529 seconds; no compiler diagnostics.

```sh
CFLAGS='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie' ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 python3 -m unittest -v test_static_bounds.py
```

**PASS:** 15 tests in 0.949 seconds with AddressSanitizer and
UndefinedBehaviorSanitizer; no sanitizer findings. Leak detection is disabled.
An initial `detect_leaks=1` attempt could not validate tests because
LeakSanitizer exited with `LeakSanitizer does not work under ptrace` in this
sandbox. That attempt is **not** a passing leak check.

```sh
python3 -m py_compile test_static_bounds.py
cc -std=c11 -Wall -Wextra -Werror decompiler.c -o decompile46
cc -std=c11 -Wall -Wextra -Werror test_static_bounds.c -o section_bounds_test
./section_bounds_test
```

**PASS:** Python compile, both C builds, and standalone harness exit 0.
Harness summary: `PASS: 1040 direct section checks`.

## Exact tested source SHA-256

| File | SHA-256 |
| --- | --- |
| `decompiler.c` | `b401792c51f288164723474a1aad5e53a5337e4e304a462bf273489269815406` |
| `test_static_bounds.py` | `eed88921953987c8e0dfcf4cc0c021aecfc123930e75c04e82bcd53a250828af` |
| `test_static_bounds.c` | `70873df3d1b1992dc5bfbe012bd026840b1d72f24709895615d833f8069048ab` |

The separately built unsanitized executables had SHA-256:

- CLI: `4641704d6ab99e91af94ac1e1c920e3daed8256c260d0958def9f8a10a011efa`.
- Harness: `8862139d21f3f99bd14d3e9593e82763bdb947d15340bed6ff9f927ac179ad13`.

Binaries are not committed; compiler/platform changes may change their hashes.

## Limits

These are synthetic bounds/input-validation checks, not proof of C166/C167
instruction semantics, a true decompiler, ROM authenticity, map consumers,
control flow, DPP state, branch targets, or ECU safety. The matrix expectations
come from the current legacy size table; they do not independently validate it.
Only the first `[0, 0x10000)` section is scanned by the CLI.

The harness requires POSIX mapping/protection facilities. LeakSanitizer did not
run successfully. No native Windows or hardware tests were performed. The input
must remain unchanged by other processes during the read-only mmap scan;
concurrent truncation/mutation is not covered and a mapping is not a snapshot.

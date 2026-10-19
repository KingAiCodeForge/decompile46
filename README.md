# decompile46 — legacy C166/C167 byte-stepper (review)

This is an old 256-entry **opcode size and mnemonic sketch**. It is **not** an
actual high-level decompiler or a proven source of ECU map consumers, DPP
addressing, branch targets, function boundaries or patch safety.

The review change makes the existing first-64-KiB scan read-only and
bounded. Unknown/unsupported opcodes and a 2-/4-byte instruction that
crosses the selected file/section end return a nonzero status instead of
reading beyond the requested region or guessing an instruction size.

Build:

```sh
cc -std=c11 -Wall -Wextra -Werror decompiler.c -o decompile46
python3 -m unittest -v test_static_bounds.py
./decompile46 /path/to/hash-checked-owned-firmware.bin > disassembly.txt
```

Only source files at least 64 KiB and no larger than 16 MiB are accepted.
The tool scans **[0x00000,0x10000)**, not the full MS42 512 KiB image. No
OSID or SHA256 identification is performed by the C utility itself, so the
caller must authenticate a firmware source independently. The old mnemonic/
instruction-size table has not been independently verified against a C166/C167
ISA manual; an apparently successful listing is NOT a verified instruction
decode.

**No ECU hardware, flash, checksum correction or tune output is supported.**
Preserve original files and prefer the fully specified C167 reverse-
engineering pipeline for target-specific proof.

## Synthetic bounds regressions

The Python suite compiles the actual `decompiler.c` plus a C harness that includes
that same source to exercise its static section scanner. Fixtures use explicit
hex bytes and assert their lengths. It covers CLI size limits, unsupported
opcodes, exact and crossing section ends, read-only input, invalid arguments,
missing files, directories, and FIFO refusal without waiting for a writer.

The harness performs 1,040 direct checks: 16 null/empty/invalid/section cases plus
all 256 opcode bytes with 1–4 available bytes immediately before an inaccessible
guard page. Converted negative `size_t` arguments must be rejected. These checks
validate bounds behavior against the existing table, **not** the table's ISA
accuracy. POSIX `mmap`, `mprotect`, and `/dev/zero` are needed for the harness.

Optional stricter and sanitizer runs (with a supporting GCC/Clang toolchain):

```sh
CFLAGS='-O2 -pedantic -Wconversion -Wsign-conversion' python3 -m unittest -v test_static_bounds.py
CFLAGS='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie' ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 python3 -m unittest -v test_static_bounds.py
```

The sanitizer command disables leak detection because the validation sandbox
cannot run LeakSanitizer under ptrace; it still runs address and undefined-
behavior instrumentation. See [the test receipt](reports/bounds-test-receipt-20261019.md)
for the exact source hashes, baseline failures, commands, and observed results.

Keep the input file stable while scanning. The read-only mapping is not a
snapshot: another process truncating or rewriting it during the scan is outside
this bounded-input test guarantee.

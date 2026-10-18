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

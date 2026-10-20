#!/usr/bin/env python3
"""Compile the real C source and test synthetic bounds, not C167 semantics."""
import hashlib
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "decompiler.c"
SECTION_SIZE = 0x10000
MAX_IMAGE_SIZE = 16 * 1024 * 1024
# fromhex makes fixture bytes explicit and avoids double-escaped byte literals.
TWO_BYTE = bytes.fromhex("cc 00")
FOUR_BYTE = bytes.fromhex("02 00 00 00")
UNSUPPORTED = bytes.fromhex("3b")
SAFE_IMAGE = TWO_BYTE * (SECTION_SIZE // len(TWO_BYTE))


class Safety(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        cls.binary = Path(cls.temp.name) / "decompile46_test"
        cls.harness = Path(cls.temp.name) / "section_bounds_test"
        compiler = shlex.split(os.environ.get("CC", "cc"))
        if not compiler or shutil.which(compiler[0]) is None:
            raise RuntimeError("C compiler not found; install a C11 POSIX compiler or set CC")
        flags = ["-std=c11", "-Wall", "-Wextra", "-Werror"]
        flags += shlex.split(os.environ.get("CFLAGS", ""))
        for source, binary in ((SOURCE, cls.binary),
                               (ROOT / "test_static_bounds.c", cls.harness)):
            run = subprocess.run(compiler + flags + [str(source), "-o", str(binary)],
                                 capture_output=True, text=True, timeout=30)
            if run.returncode:
                raise RuntimeError("Compiler failed: " + run.stderr)

    def run_cli(self, *args):
        return subprocess.run([str(self.binary), *map(str, args)],
                              capture_output=True, text=True, timeout=10)

    def run_image(self, data):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "fixture.bin"
            p.write_bytes(data)
            before = hashlib.sha256(p.read_bytes()).hexdigest()
            r = self.run_cli(p)
            self.assertEqual(hashlib.sha256(p.read_bytes()).hexdigest(), before)
            return r

    def assert_hold(self, run, diagnostic):
        self.assertEqual(run.returncode, 2, run.stderr[-2000:])
        self.assertIn("HOLD", run.stderr)
        self.assertIn(diagnostic, run.stderr)

    def test_fixture_bytes_and_lengths(self):
        self.assertEqual(TWO_BYTE, bytes((0xcc, 0x00)))
        self.assertEqual(FOUR_BYTE, bytes((0x02, 0, 0, 0)))
        self.assertEqual(UNSUPPORTED, bytes((0x3b,)))
        self.assertEqual(len(SAFE_IMAGE), SECTION_SIZE)

    def test_short_input_refused(self):
        for data in (b"", TWO_BYTE, SAFE_IMAGE[:-1]):
            with self.subTest(length=len(data)):
                r = self.run_image(data)
                self.assert_hold(r, "regular file")
                self.assertEqual(r.stdout, "")

    def test_unsupported_opcode_holds_without_unknown_length_guess(self):
        data = UNSUPPORTED + bytes(SECTION_SIZE - 1)
        self.assertEqual(len(data), SECTION_SIZE)
        r = self.run_image(data)
        self.assert_hold(r, "opcode 0x3b")
        self.assertIn("0x00000", r.stderr)
        self.assertEqual(len(r.stdout.splitlines()), 1)  # image header only

    def test_unsupported_opcode_at_last_step(self):
        r = self.run_image(SAFE_IMAGE[:-2] + UNSUPPORTED + bytes(1))
        self.assert_hold(r, "opcode 0x3b")
        self.assertIn("0x0fffe", r.stderr)
        self.assertEqual(len(r.stdout.splitlines()), 32768)

    def test_length_four_instruction_at_end_refused(self):
        data = SAFE_IMAGE[:-2] + FOUR_BYTE[:2]
        self.assertEqual(len(data), SECTION_SIZE)
        r = self.run_image(data)
        self.assert_hold(r, "boundary")
        self.assertIn("0x0fffe", r.stderr)
        self.assertNotIn("[0fffe]", r.stdout)

    def test_larger_file_cannot_supply_bytes_beyond_section_end(self):
        r = self.run_image(SAFE_IMAGE[:-2] + FOUR_BYTE)
        self.assert_hold(r, "boundary")
        self.assertIn("0x0fffe", r.stderr)
        self.assertNotIn("[0fffe]", r.stdout)

    def test_exact_first_64k_safe_input(self):
        r = self.run_image(SAFE_IMAGE)
        self.assertEqual(r.returncode, 0, r.stderr[-2000:])
        self.assertEqual(r.stderr, "")
        lines = r.stdout.splitlines()
        self.assertEqual(len(lines), 32769)
        self.assertIn("bytes=65536", lines[0])
        self.assertEqual(lines[1], "[00000][cc 00] nop 00")
        self.assertEqual(lines[-1], "[0fffe][cc 00] nop 00")

    def test_four_byte_instruction_fits_exactly(self):
        r = self.run_image(SAFE_IMAGE[:-4] + FOUR_BYTE)
        self.assertEqual(r.returncode, 0, r.stderr[-2000:])
        self.assertEqual(r.stderr, "")
        self.assertEqual(r.stdout.splitlines()[-1],
                         "[0fffc][02 00 00 00] add 00 00 00")

    def test_bytes_after_section_are_not_decoded(self):
        r = self.run_image(SAFE_IMAGE + UNSUPPORTED)
        self.assertEqual(r.returncode, 0, r.stderr[-2000:])
        self.assertEqual(r.stderr, "")
        self.assertEqual(len(r.stdout.splitlines()), 32769)
        self.assertNotIn("[10000]", r.stdout)

    def test_image_size_limit(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "sparse.bin"
            with path.open("wb") as f:
                f.write(SAFE_IMAGE)
                f.truncate(MAX_IMAGE_SIZE)
            r = self.run_cli(path)
            self.assertEqual(r.returncode, 0, r.stderr[-2000:])
            self.assertIn("bytes=16777216", r.stdout.splitlines()[0])
            with path.open("r+b") as f:
                f.truncate(MAX_IMAGE_SIZE + 1)
            r = self.run_cli(path)
            self.assert_hold(r, "<=16MiB")
            self.assertEqual(r.stdout, "")

    def test_missing_path_refused(self):
        r = self.run_cli(Path(self.temp.name) / "does-not-exist.bin")
        self.assert_hold(r, "cannot open")

    def test_directory_refused(self):
        self.assert_hold(self.run_cli(self.temp.name), "regular file")

    @unittest.skipUnless(hasattr(os, "mkfifo"), "POSIX FIFO support required")
    def test_fifo_refused_without_waiting_for_writer(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "input.fifo"
            os.mkfifo(path)
            r = self.run_cli(path)
            self.assert_hold(r, "regular file")
            self.assertEqual(r.stdout, "")

    def test_argument_count(self):
        for args in ((), ("unused.bin", "extra")):
            with self.subTest(args=args):
                r = self.run_cli(*args)
                self.assertEqual(r.returncode, 2)
                self.assertIn("Usage:", r.stderr)

    def test_help_and_version_without_input(self):
        for option, expected in (("--help", "Usage:"),
                                 ("--version", "0.1.0-bounded.1")):
            with self.subTest(option=option):
                run = self.run_cli(option)
                self.assertEqual(run.returncode, 0, run.stderr)
                self.assertIn(expected, run.stdout)

    def test_outside_checkout_with_spaces_and_unicode_path(self):
        with tempfile.TemporaryDirectory(prefix="scanner space ") as directory:
            cwd = Path(directory)
            image = cwd / "synthetic \u00e9\u65e5\u672c.bin"
            image.write_bytes(SAFE_IMAGE)
            before = hashlib.sha256(image.read_bytes()).hexdigest()
            run = subprocess.run([str(self.binary), image.name], cwd=cwd,
                                 capture_output=True, text=True, timeout=10)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertIn("[0fffe][cc 00]", run.stdout)
            self.assertEqual(hashlib.sha256(image.read_bytes()).hexdigest(), before)
            self.assertEqual(list(cwd.iterdir()), [image])

    @unittest.skipUnless(Path("/dev/full").exists(), "Linux /dev/full required")
    def test_output_failure_is_not_success(self):
        with tempfile.TemporaryDirectory() as directory:
            image = Path(directory) / "fixture.bin"
            image.write_bytes(SAFE_IMAGE)
            with open("/dev/full", "wb") as destination:
                run = subprocess.run([str(self.binary), str(image)],
                                     stdout=destination, stderr=subprocess.PIPE,
                                     text=True, timeout=10)
            self.assert_hold(run, "cannot write complete listing")
            self.assertEqual(image.read_bytes(), SAFE_IMAGE)

    def test_direct_section_bounds_and_guard_page_matrix(self):
        r = subprocess.run([str(self.harness)], capture_output=True,
                           text=True, timeout=10)
        self.assertEqual(r.returncode, 0, r.stderr[-2000:])
        self.assertIn("PASS: 1040 direct section checks", r.stdout)


if __name__ == "__main__":
    unittest.main()

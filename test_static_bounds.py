#!/usr/bin/env python3
"""Read-only synthetic guards; not native C167 semantic or ROM proof."""
import hashlib
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "decompiler.c"

class Safety(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with tempfile.TemporaryDirectory() as d:
            pass
        cls.temp = tempfile.TemporaryDirectory()
        cls.binary = Path(cls.temp.name) / "decompile46_test"
        run = subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                              str(SOURCE), "-o", str(cls.binary)],
                             capture_output=True, text=True)
        if run.returncode:
            raise RuntimeError("Compiler failed: " + run.stderr)
    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()
    def run_image(self, data):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "fixture.bin"
            p.write_bytes(data)
            before = hashlib.sha256(p.read_bytes()).hexdigest()
            r = subprocess.run([str(self.binary), str(p)],
                               capture_output=True, text=True, timeout=10)
            self.assertEqual(hashlib.sha256(p.read_bytes()).hexdigest(), before)
            return r
    def test_short_input_refused(self):
        p = self.run_image(b"\\xCC\\x00")
        self.assertNotEqual(p.returncode, 0)
        self.assertIn("HOLD", p.stderr)
    def test_unsupported_opcode_holds_without_unknown_length_guess(self):
        r=self.run_image(b"\\x3b" + b"\\x00" * 65535)
        self.assertEqual(r.returncode, 2)
        self.assertIn("unsupported", r.stderr)
    def test_length_four_instruction_at_end_refused(self):
        b=bytearray(b"\\xCC\\x00" * 32768)
        b[-2:] = b"\\x02\\x00"
        r=self.run_image(b)
        self.assertEqual(r.returncode, 2)
        self.assertIn("boundary",r.stderr)
    def test_exact_first_64k_safe_input(self):
        r=self.run_image(b"\\xCC\\x00" * 32768)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("[0fffe][cc 00]",r.stdout.lower())

if __name__ == "__main__":
    unittest.main()

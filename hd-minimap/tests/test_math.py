"""Compile and run the host tests of the HD minimap's rules (needs a C compiler on PATH)."""
import os
import subprocess
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))


class MathTest(unittest.TestCase):
    def test_rules(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = os.path.join(tmp, 'test_math')
            subprocess.run(['cc', '-Wall', '-I', os.path.join(HERE, '..', 'src'), os.path.join(HERE, 'test_math.c'), '-o', exe, '-lm'], check=True)
            out = subprocess.run([exe], check=True, capture_output=True, text=True).stdout
            self.assertIn('hd-minimap math: ok', out)


if __name__ == '__main__':
    unittest.main()

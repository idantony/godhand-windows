"""Check quoting and a real >32K Zig compile/link in spaced Unicode paths."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("compiler_response", ROOT / "tools/compiler_response.py")
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)


class ResponseTests(unittest.TestCase):
    def setUp(self):
        base = Path("G:/middle_thing/godhand_windows/tests/compiler-response")
        base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="response-", dir=str(base))
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def test_quotes_spaces_and_normalizes_windows_paths(self):
        path = self.root / "flags.rsp"
        fact = tool.write_response_file(path, [r"C:\a b\test.o", "-Wl,--export-all-symbols", ""])
        self.assertEqual(path.read_text(), '"C:/a b/test.o"\n"-Wl,--export-all-symbols"\n""\n')
        self.assertEqual(fact["size"], path.stat().st_size)
        self.assertEqual(len(fact["sha256"]), 64)

    def test_invalid_tokens_rejected_before_output_creation(self):
        for token in ('bad"quote', "bad\nline", "bad\x00nul", "bad\tspace"):
            path = self.root / "invalid.rsp"
            with self.subTest(token=token), self.assertRaises(ValueError): tool.write_response_file(path, [token])
            self.assertFalse(path.exists())

    def test_actual_zig_long_compile_link_and_execution(self):
        # This uses the real installed compiler, not a fake subprocess result.
        folder = self.root / "spaced Unicode \u6d4b\u8bd5"
        folder.mkdir()
        helper, main = folder / "helper file.c", folder / "main file.c"
        helper.write_text("int helper(void){return 7;}\n", encoding="utf-8")
        main.write_text("#ifndef RESPONSE_PAD_2499\n#error missing_response_argument\n#endif\n"
                        "int helper(void);int main(void){return helper()!=7||RESPONSE_PAD_2499!=1;}\n", encoding="utf-8")
        executable = folder / "response program.exe"
        args = ["-target", "x86_64-windows-gnu", "-std=c11", "-O2"]
        args += ["-DRESPONSE_PAD_%04d=1" % i for i in range(2500)]
        args += [helper, main, "-o", executable]
        response = self.root / "large.rsp"
        fact = tool.write_response_file(response, args)
        self.assertGreater(fact["size"], 32767)
        env = os.environ.copy()
        env.update(TEMP=str(self.root), TMP=str(self.root),
                   ZIG_GLOBAL_CACHE_DIR="G:/middle_thing/godhand_windows/cache/zig-global",
                   ZIG_LOCAL_CACHE_DIR=str(self.root / "cache"))
        command = [str(ROOT / ".tools/zig-x86_64-windows-0.17.1/zig.exe"), "cc", "@" + str(response)]
        result = subprocess.run(command, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace")
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertTrue(executable.is_file())
        self.assertEqual(subprocess.run([str(executable)]).returncode, 0)
        print(json.dumps({"real_zig_response_bytes": fact["size"], "spaced_unicode_compile_link_run": "PASS"}))


if __name__ == "__main__":
    unittest.main()

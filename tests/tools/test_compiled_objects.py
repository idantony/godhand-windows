from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from compiled_objects import ObjectCache, c_driver, compilation_key, project_inputs, toolchain_key


class CompiledObjectsTests(unittest.TestCase):
    def setUp(self):
        self.scratch = Path(r"G:\middle_thing\godhand_windows\tests\tools")
        self.scratch.mkdir(parents=True, exist_ok=True)

    def test_compile_keys_change_only_with_reachable_inputs(self):
        hashes = {"a.c": "a", "b.h": "b", "c.h": "c", "other.c": "other"}
        includes = {"a.c": ["b.h"], "b.h": ["c.h"], "c.h": ["b.h"]}
        flags = ["cc", "-target", "x86_64-windows-gnu", "-std=c11"]

        def key(recipe=flags, compiler="compiler", volatile=()):
            return compilation_key(compiler, recipe, "a.c", hashes, includes, volatile)

        initial = key()
        hashes["other.c"] = "modified"
        self.assertEqual(key(), initial)
        self.assertNotEqual(key(recipe=[*flags, "-O0"]), initial)
        self.assertNotEqual(key(compiler="other compiler"), initial)
        hashes["c.h"] = "modified"
        self.assertNotEqual(key(), initial)
        self.assertIsNone(key(volatile={"c.h"}))

    def test_object_restore_checks_content_before_replacing_output(self):
        with tempfile.TemporaryDirectory(dir=self.scratch) as directory:
            root = Path(directory)
            cache = ObjectCache(root / "cache")
            key = "a" * 64
            built = root / "original.o"
            output = root / "restored.o"
            built.write_bytes(b"object bytes")
            cache.store(key, built)
            self.assertTrue(cache.restore(key, output))
            self.assertEqual(output.read_bytes(), built.read_bytes())
            cache.entry(key).write_bytes(cache.entry(key).read_bytes() + b"corruption")
            self.assertFalse(cache.restore(key, output))
            self.assertEqual(output.read_bytes(), built.read_bytes())

    def test_object_recipe_shares_across_source_and_output_locations(self):
        with tempfile.TemporaryDirectory(dir=self.scratch) as directory:
            root = Path(directory)
            compiler = root / "zig.exe"
            cache = ObjectCache(root / "cache", "fixed-toolchain")

            def prepare(project, extra_flags=(), tracked="fixture"):
                (project / "include").mkdir(parents=True, exist_ok=True)
                (project / "entry.c").write_text('#include "include/first.h"\n', encoding="utf-8")
                (project / "include/first.h").write_text('#include "../shared.h"\n', encoding="utf-8")
                (project / "shared.h").write_text("#define VALUE 1\n", encoding="utf-8")
                (project / "test.c").write_text(tracked, encoding="utf-8")
                inputs = project_inputs(project, ["entry.c"], ["test.c"])
                command = c_driver(compiler) + [*extra_flags, "-c", "entry.c", "-o", project / "entry.o"]
                key, actual = cache.prepare(project, command, "entry.c", *inputs)
                self.assertIn(f"-ffile-prefix-map={project}=.", actual)
                self.assertEqual(set(inputs[0]), {"entry.c", "include/first.h", "shared.h", "test.c"})
                return key, inputs[0]

            windows_key, windows_hashes = prepare(root / "windows")
            native_key, native_hashes = prepare(root / "native", tracked="other test input")
            self.assertEqual(windows_key, native_key)
            self.assertNotEqual(windows_hashes["test.c"], native_hashes["test.c"])
            recording_key, _ = prepare(root / "native", ("-Dvoice=recording_voice",))
            self.assertNotEqual(recording_key, native_key)

    def test_toolchain_layout_and_override_affect_reuse(self):
        with tempfile.TemporaryDirectory(dir=self.scratch) as directory:
            root = Path(directory)
            compiler = root / "zig.exe"
            compiler.write_bytes(b"compiler")
            library = root / "lib"
            library.mkdir()
            first = toolchain_key(compiler, {})
            self.assertIsNotNone(first)
            self.assertIsNone(toolchain_key(compiler, {"CPATH": "external include directory"}))
            header = library / "header.h"
            header.write_text("int first;", encoding="utf-8")
            second = toolchain_key(compiler, {})
            self.assertNotEqual(second, first)
            header.write_text("int replacement;", encoding="utf-8")
            self.assertNotEqual(toolchain_key(compiler, {}), second)


if __name__ == "__main__":
    unittest.main()

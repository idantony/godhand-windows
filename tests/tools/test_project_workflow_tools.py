"""Workflow regression tests; all generated repositories remain under G:."""
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import unittest
import uuid
from unittest.mock import patch

PROJECT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(PROJECT / "tools"))
from test_contracts import check_contracts, literal_argc, manifest_cases, TestContractError

spec = importlib.util.spec_from_file_location("staged_eof", PROJECT / "tools/normalize-staged-eof.py")
eof = importlib.util.module_from_spec(spec)
spec.loader.exec_module(eof)
spec = importlib.util.spec_from_file_location("stage_snapshot", PROJECT / "tools/stage-verified-snapshot.py")
snapshot_stage = importlib.util.module_from_spec(spec)
spec.loader.exec_module(snapshot_stage)
spec = importlib.util.spec_from_file_location("freeze_source", PROJECT / "tools/freeze-source.py")
snapshot_freeze = importlib.util.module_from_spec(spec)
spec.loader.exec_module(snapshot_freeze)
SCRATCH = Path(os.environ.get("GH_TEST_SCRATCH", r"G:/middle_thing/godhand_windows/tool-tests"))


class WorkflowTools(unittest.TestCase):
    def setUp(self):
        self.root = (SCRATCH / ("workflow-" + uuid.uuid4().hex)).resolve()
        if SCRATCH.resolve() not in self.root.parents:
            raise RuntimeError("Test repository escaped its dedicated scratch")
        self.root.mkdir(parents=True)
        self.git("init", "--quiet")
        self.git("config", "core.autocrlf", "false")

    def git(self, *args):
        return subprocess.check_output(["git", *args], cwd=self.root, stderr=subprocess.DEVNULL)

    def stage(self, name, data):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        self.git("add", "--", name)
        return path

    def test_worker_hash_pairs_keep_both_file_versions(self):
        name = "src/model.c"
        old, new = "A" * 64, "B" * 64
        hash_rows = [{"path": name, "base": old, "overlay": new}]
        file_map = {"files": {name: {"base_sha256": old, "overlay_sha256": new}}}
        for receipt in (hash_rows, file_map):
            normalized, predecessors = snapshot_freeze.overlay_receipt(receipt)
            self.assertEqual(normalized["files"], [{"path": name, "sha256": new.lower()}])
            self.assertEqual(predecessors, {name: old.lower()})
        with self.assertRaisesRegex(ValueError, "duplicate"):
            snapshot_freeze.overlay_receipt(hash_rows * 2)
        normalized, predecessors = snapshot_freeze.overlay_receipt([{"path": name, "base": None, "overlay": new}])
        self.assertEqual(normalized["files"], [{"path": name, "sha256": new.lower()}])
        self.assertEqual(predecessors, {name: None})
        with self.assertRaisesRegex(ValueError, "SHA-256"):
            snapshot_freeze.overlay_receipt([{"path": name, "base": "", "overlay": new}])
        with self.assertRaisesRegex(ValueError, "hash pair"):
            snapshot_freeze.overlay_receipt([{"path": name, "overlay": new}])

    def test_index_only_preserves_worker_and_snapshot(self):
        path = self.stage("src/demo.c", b"int value;\n\n\n")
        snapshot = self.root / "immutable-copy.c"
        snapshot.write_bytes(path.read_bytes())
        path.write_bytes(b"new worker edit\n\n")
        index = (self.root / ".git/index").read_bytes()
        plan = eof.normalize(self.root)
        self.assertEqual(len(plan["files"]), 1)
        self.assertEqual((self.root / ".git/index").read_bytes(), index)
        result = eof.normalize(self.root, apply=True)
        self.assertEqual(result["files"][0]["terminal_newlines_removed"], 2)
        self.assertEqual(self.git("show", ":src/demo.c"), b"int value;\n")
        self.assertEqual(path.read_bytes(), b"new worker edit\n\n")
        self.assertEqual(snapshot.read_bytes(), b"int value;\n\n\n")
        self.assertEqual(eof.normalize(self.root, apply=True)["files"], [])

    def test_rejects_other_whitespace_atomically(self):
        path = self.stage("src/demo.c", b"trailing space \n\n")
        before = (self.root / ".git/index").read_bytes()
        with self.assertRaisesRegex(ValueError, "non-EOF"):
            eof.normalize(self.root, apply=True)
        self.assertEqual((self.root / ".git/index").read_bytes(), before)
        self.assertEqual(path.read_bytes(), b"trailing space \n\n")

    def test_snapshot_batch_filters_immutable_bytes_at_the_original_path(self):
        attrs = b"*.c text eol=lf\n*.py -text\n"
        self.stage(".gitattributes", attrs)
        snapshot = self.root / "snapshot"
        (snapshot / "src").mkdir(parents=True)
        (snapshot / ".gitattributes").write_bytes(attrs)
        files = {"src/测试 name.c": b"int frozen;\r\n", "src/raw.py": b"pass\r\n"}
        for name, data in files.items():
            (snapshot / name).write_bytes(data)
            live = self.stage(name, b"worker edit\n")
            live.write_bytes(b"new worker edit\n")
        index = (self.root / ".git/index").read_bytes()
        with patch.object(snapshot_stage, "git", wraps=snapshot_stage.git) as git:
            blobs = snapshot_stage.filtered_blobs(self.root, snapshot, files)
            self.assertTrue(any("--stdin-paths" in call.args[1] for call in git.call_args_list))
        self.assertEqual([self.git("cat-file", "blob", blob) for blob in blobs],
                         [b"int frozen;\n", b"pass\r\n"])
        self.assertEqual((self.root / ".git/index").read_bytes(), index)
        self.assertEqual((self.root / "src/测试 name.c").read_bytes(), b"new worker edit\n")
        self.assertEqual((snapshot / "src/测试 name.c").read_bytes(), b"int frozen;\r\n")

        # A snapshot with stale attributes still uses the live repository rule.
        (snapshot / ".gitattributes").write_bytes(b"*.c -text\n*.py -text\n")
        with patch.object(snapshot_stage, "git", wraps=snapshot_stage.git) as git:
            self.assertEqual(snapshot_stage.filtered_blobs(self.root, snapshot, files), blobs)
            self.assertFalse(any("--stdin-paths" in call.args[1] for call in git.call_args_list))

    def test_unicode_spaces_and_executable_mode(self):
        name = "src/测试 名称.py"
        self.stage(name, b"print(1)\n\n")
        self.git("update-index", "--chmod=+x", "--", name)
        eof.normalize(self.root, apply=True)
        self.assertTrue(self.git("ls-files", "--stage", "--", name).startswith(b"100755 "))
        self.assertEqual(self.git("show", ":" + name), b"print(1)\n")

    def test_existing_report_is_not_overwritten(self):
        self.stage("src/demo.c", b"x\n\n")
        report = self.root / "existing.json"
        report.write_text("preserve", encoding="utf-8")
        before = (self.root / ".git/index").read_bytes()
        result = subprocess.run([sys.executable, "-B", str(PROJECT / "tools/normalize-staged-eof.py"),
            "--repository", str(self.root), "--apply", "--report", str(report)], capture_output=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(report.read_text(), "preserve")
        self.assertEqual((self.root / ".git/index").read_bytes(), before)

    def test_concurrent_index_and_foreign_lock_are_preserved(self):
        self.stage("src/demo.c", b"x\n\n")
        index = self.root / ".git/index"
        before = index.read_bytes()
        self.stage("src/new.c", b"new staging\n")
        current = index.read_bytes()
        with self.assertRaisesRegex(RuntimeError, "Index changed"):
            eof.replace_index(index, before, b"must not replace")
        self.assertEqual(index.read_bytes(), current)
        lock = self.root / ".git/index.lock"
        self.assertFalse(lock.exists())
        lock.write_bytes(b"another process owns this")
        with self.assertRaises(FileExistsError):
            eof.replace_index(index, current, b"must not replace")
        self.assertEqual(lock.read_bytes(), b"another process owns this")
        self.assertEqual(index.read_bytes(), current)

    def test_contract_ignores_comments_and_strings(self):
        source = '/* C(argc==3) */ int main(int argc,char**argv){puts("C(argc==9)");C(argc==2&&argv);}'
        self.assertEqual(literal_argc(source), 2)
        self.assertIsNone(literal_argc("int main(void){return 0;}"))
        self.assertIsNone(literal_argc("int main(int argc,char**argv){C(argc==2);C(argc==3);}"))
        self.assertIsNone(literal_argc("int main(int argc,char**argv){C(argc==2||argc==3);}"))
        self.assertIsNone(literal_argc("int main(int argc,char**argv){if(argc==2)C(argc==2);}"))

    def test_contract_catches_wrong_file_count(self):
        source = self.root / "test.c"
        source.write_text("int main(int argc,char**argv){C(argc==2);}")
        with self.assertRaisesRegex(TestContractError, "mode both supplies argc=3"):
            check_contracts(self.root, [("door", "test.c", "both")])
        result = check_contracts(self.root, [("door", "test.c", "iso")])
        self.assertEqual(result["known"], 1)

    def test_manifest_is_read_as_data(self):
        manifest = self.root / "build.py"
        manifest.write_text("raise RuntimeError('must never execute')\nextra_tests=[('a','test.c','iso')]\n")
        self.assertEqual(manifest_cases(manifest), [("a", "test.c", "iso")])

    def test_media_modes_and_unknown_mode(self):
        (self.root / "test.c").write_text("int main(int argc,char**argv){C(argc==3);}")
        result = check_contracts(self.root, [("movie", "test.c", "movie"), ("adx", "test.c", "adx-reference")])
        self.assertEqual(result["known"], 2)
        with self.assertRaisesRegex(TestContractError, "Unknown test argument mode"):
            check_contracts(self.root, [("bad", "test.c", "unexpected")])


if __name__ == "__main__":
    unittest.main()

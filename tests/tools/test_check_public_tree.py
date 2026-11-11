import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


TOOL = Path(__file__).resolve().parents[2] / 'tools' / 'check-public-tree.py'


class PublicTreeTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.repo = Path(temporary.name)
        self.git('init', '-q')
        self.git('config', 'core.autocrlf', 'false')

    def git(self, *arguments):
        return subprocess.check_output(['git', *arguments], cwd=self.repo)

    def stage(self, name, content):
        path = self.repo / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
        self.git('add', '--', name)
        return path

    def check(self):
        run = subprocess.run([sys.executable, str(TOOL)], cwd=self.repo, capture_output=True)
        self.assertEqual(run.stderr, b'')
        return run, json.loads(run.stdout)

    def test_reads_unicode_and_space_paths_from_index_only(self):
        staged = b'staged code\n'
        first = self.stage('src/caf\u00e9 \u795e.c', staged)
        self.stage('tests/same blob.c', staged)
        first.write_bytes(b'worktree changed\0')
        run, report = self.check()
        self.assertEqual(run.returncode, 0)
        self.assertEqual(report, {'status': 'passed', 'files': 2,
                                  'bytes': 2 * len(staged), 'failures': []})

    def test_checks_paths_binary_size_and_secrets_without_disclosing_value(self):
        secret = b'ghp_' + b'Z' * 24
        binary = b'prefix\0' + secret
        oversized = b'x' * (1024 * 1024 + 1)
        self.stage('src/binary.c', binary)
        self.stage('src/oversized.c', oversized)
        self.stage('src/artifact.PNG', b'data')
        self.stage('tests/sample_evidence.json', b'{}')
        self.stage('tests/validation.json', b'{}')
        self.stage('private.txt', b'data')
        run, report = self.check()
        self.assertEqual(run.returncode, 1)
        self.assertNotIn(secret, run.stdout)
        self.assertEqual(report['status'], 'failed')
        self.assertEqual(report['files'], 2)
        self.assertEqual(report['bytes'], len(binary) + len(oversized))
        self.assertEqual(report['failures'], [
            {'file': 'private.txt', 'reason': 'outside published source allowlist'},
            {'file': 'src/artifact.PNG', 'reason': 'original/intermediate/private evidence file'},
            {'file': 'src/binary.c', 'reason': 'binary or oversized content'},
            {'file': 'src/binary.c', 'reason': 'credential-like content; value deliberately withheld'},
            {'file': 'src/oversized.c', 'reason': 'binary or oversized content'},
            {'file': 'tests/sample_evidence.json', 'reason': 'original/intermediate/private evidence file'},
            {'file': 'tests/validation.json', 'reason': 'original/intermediate/private evidence file'},
        ])


if __name__ == '__main__':
    unittest.main()

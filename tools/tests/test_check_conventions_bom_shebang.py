import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class ToolExecutableTests(unittest.TestCase):
    def test_tracked_python_tools_with_shebangs_are_executable(self):
        root = Path(__file__).resolve().parents[2]
        result = subprocess.run(["git", "ls-files", "--stage", "-z", "--", "tools/*.py"], cwd=root,
                                capture_output=True, text=True, encoding="utf-8", check=True)
        scripts = []
        for entry in result.stdout.split("\0"):
            if not entry:
                continue
            metadata, name = entry.split("\t", 1)
            path = root / name
            if path.parent == root / "tools" and path.read_bytes().startswith(b"#!"):
                scripts.append((metadata.split()[0], path))
        self.assertTrue(scripts)
        for mode, path in scripts:
            with self.subTest(script=path.name):
                self.assertEqual(mode, "100755")

    @unittest.skipUnless(os.name == "posix", "requires POSIX executable file modes")
    def test_stubgen_can_run_directly(self):
        root = Path(__file__).resolve().parents[2]
        result = subprocess.run([str(root / "tools/stubgen.py"), "--help"], cwd=root,
                                capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("usage:", result.stdout)


class BomShebangTests(unittest.TestCase):
    def check(self, header):
        checker = Path(__file__).resolve().parents[1] / "check_conventions.py"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)

            def git(*args):
                return subprocess.run(
                    ["git", "-c", "user.name=Conventions Test", "-c", "user.email=test@example.com",
                     "-c", "commit.gpgsign=false", *args], cwd=root,
                    capture_output=True, text=True, check=True)

            git("init", "-q")
            (root / "README.md").write_text("Example\n", encoding="utf-8")
            git("add", ".")
            git("commit", "-qm", "docs: add readme")
            script = root / "tools/example.py"
            script.parent.mkdir()
            script.write_text("\ufeff" + header + '\nprint("ok")\n', encoding="utf-8")
            executed = subprocess.run([sys.executable, str(script)], capture_output=True, text=True)
            self.assertEqual((executed.returncode, executed.stdout.strip()), (0, "ok"), executed.stderr)
            git("add", ".")
            git("commit", "-qm", "fix: add Python tool")
            environment = {key: value for key, value in os.environ.items()
                           if key not in ("GITHUB_EVENT_PATH", "GITHUB_STEP_SUMMARY", "GITHUB_ACTIONS")}
            return subprocess.run([sys.executable, str(checker), "--base", "HEAD~1"],
                                  cwd=root, capture_output=True, text=True, env=environment)

    def test_initial_bom_shebang_is_accepted(self):
        result = self.check("#!/usr/bin/env python3")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_initial_bom_does_not_exempt_an_ordinary_comment(self):
        result = self.check("# ordinary comment")
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("[comment]", result.stdout)


if __name__ == "__main__":
    unittest.main()

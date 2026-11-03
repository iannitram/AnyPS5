import re
import unittest
from pathlib import Path

WORKFLOWS = Path(__file__).resolve().parents[2] / ".github/workflows"
FILES = ("conflict-label.yml", "pr-overlap.yml")


def trigger_block(name):
    source = (WORKFLOWS / name).read_text(encoding="utf-8")
    match = re.search(r"^on:\n(.*?)(?=^\S|\Z)", source, re.S | re.M)
    if not match:
        raise AssertionError(f"{name} has no on: block")
    return match.group(1)


def trigger(name, key):
    match = re.search(rf"^  {key}:(.*?)(?=^  \S|\Z)", trigger_block(name), re.S | re.M)
    return None if match is None else match.group(1)


class ConflictOverlapTriggerTests(unittest.TestCase):
    def test_neither_workflow_has_a_schedule(self):
        for name in FILES:
            self.assertNotIn("cron:", trigger_block(name), name)
            self.assertIsNone(trigger(name, "schedule"), name)

    def test_both_run_on_pull_request_target(self):
        for name in FILES:
            self.assertIsNotNone(trigger(name, "pull_request_target"), name)

    def test_conflict_label_runs_when_a_pull_request_is_pushed_to(self):
        types = re.search(r"types:\s*\[(.*?)\]", trigger("conflict-label.yml", "pull_request_target") or "")
        self.assertIsNotNone(types)
        self.assertIn("synchronize", [t.strip() for t in types.group(1).split(",")])

    def test_pr_overlap_runs_when_a_pull_request_opens_reopens_or_is_pushed_to(self):
        types = re.search(r"types:\s*\[(.*?)\]", trigger("pr-overlap.yml", "pull_request_target") or "")
        self.assertIsNotNone(types)
        found = [t.strip() for t in types.group(1).split(",")]
        for wanted in ("opened", "reopened", "synchronize"):
            self.assertIn(wanted, found)

    def test_both_keep_push_to_main_and_manual_runs(self):
        for name in FILES:
            self.assertRegex(trigger(name, "push") or "", r"branches:\s*\[main\]", name)
            self.assertIsNotNone(trigger(name, "workflow_dispatch"), name)

    def test_neither_checks_out_pull_request_code(self):
        for name in FILES:
            source = (WORKFLOWS / name).read_text(encoding="utf-8")
            self.assertNotIn("github.event.pull_request.head", source, name)
            self.assertNotIn("refs/pull/", source, name)


if __name__ == "__main__":
    unittest.main()

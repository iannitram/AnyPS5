import re
import unittest
from pathlib import Path

WORKFLOWS = Path(__file__).resolve().parents[2] / ".github/workflows"
FILES = ("area-labels.yml", "fix-label.yml")


def trigger_block(name):
    source = (WORKFLOWS / name).read_text(encoding="utf-8")
    match = re.search(r"^on:\n(.*?)(?=^\S|\Z)", source, re.S | re.M)
    if not match:
        raise AssertionError(f"{name} has no on: block")
    return match.group(1)


def trigger(name, key):
    match = re.search(rf"^  {key}:(.*?)(?=^  \S|\Z)", trigger_block(name), re.S | re.M)
    return None if match is None else match.group(1)


def types(name):
    found = re.search(r"types:\s*\[(.*?)\]", trigger(name, "pull_request_target") or "")
    return None if found is None else [t.strip() for t in found.group(1).split(",")]


def daily(cron):
    fields = cron.split()
    return len(fields) == 5 and fields[0].isdigit() and fields[1].isdigit()


class AreaFixTriggerTests(unittest.TestCase):
    def test_each_workflow_has_exactly_one_cron_and_it_runs_at_most_once_a_day(self):
        for name in FILES:
            crons = re.findall(r"cron:\s*['\"]([^'\"]+)['\"]", trigger_block(name))
            self.assertEqual(len(crons), 1, name)
            self.assertTrue(daily(crons[0]), f"{name}: {crons[0]}")

    def test_area_labels_runs_when_a_pull_request_opens_reopens_or_is_pushed_to(self):
        found = types("area-labels.yml")
        self.assertIsNotNone(found)
        for wanted in ("opened", "reopened", "synchronize"):
            self.assertIn(wanted, found)

    def test_fix_label_runs_when_a_pull_request_opens_reopens_or_is_edited(self):
        found = types("fix-label.yml")
        self.assertIsNotNone(found)
        for wanted in ("opened", "edited", "reopened"):
            self.assertIn(wanted, found)

    def test_both_keep_manual_runs(self):
        for name in FILES:
            self.assertIsNotNone(trigger(name, "workflow_dispatch"), name)

    def test_neither_checks_out_pull_request_code(self):
        for name in FILES:
            source = (WORKFLOWS / name).read_text(encoding="utf-8")
            self.assertNotIn("github.event.pull_request.head", source, name)
            self.assertNotIn("refs/pull/", source, name)


if __name__ == "__main__":
    unittest.main()

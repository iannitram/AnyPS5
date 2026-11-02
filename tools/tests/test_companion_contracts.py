import subprocess
import sys
import unittest
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools.companion.contracts import ContractError, export_record, new_id, validate


def installation():
    return {
        "schema_version": 1,
        "record_type": "installation",
        "id": new_id("installation"),
        "source_path": "/private/game/source",
        "generated_path": "/private/game/output",
        "title_id": "PPSA00001",
        "game_version": "unknown",
        "relinker_revision": "r1",
        "library_revision": "l1",
        "patch_config_fingerprint": "sha256:" + "a" * 64,
        "created_at": "2026-11-02T10:00:00Z",
    }


def session():
    return {
        "schema_version": 1,
        "record_type": "session",
        "id": new_id("session"),
        "installation_id": new_id("installation"),
        "process_id": "unknown",
        "started_at": "2026-11-02T10:00:00Z",
        "relinker_revision": "r1",
        "library_revision": "l1",
        "patch_config_fingerprint": "unknown",
        "cpu": "unknown",
        "gpu": "unknown",
        "driver": "unknown",
        "os": "Linux",
        "cache_mode": "disabled",
        "instrumentation": "unknown",
        "capture": {"state": "partial", "provenance": "synthetic", "details": "some events unavailable"},
    }


class ContractTests(unittest.TestCase):
    def test_ids_are_stable_shaped_and_type_scoped(self):
        value = new_id("session")
        self.assertEqual(value, f"session:{uuid.UUID(value.split(':', 1)[1])}")
        with self.assertRaisesRegex(ContractError, "unsupported record type"):
            new_id("route")

    def test_validates_installation_and_keeps_unknown_values(self):
        record = installation()
        self.assertEqual(validate(record)["game_version"], "unknown")
        record["source_path"] = "relative/game"
        with self.assertRaisesRegex(ContractError, "absolute local path"):
            validate(record)

    def test_rejects_unknown_versions_and_extra_fields(self):
        record = installation()
        record["schema_version"] = 2
        with self.assertRaisesRegex(ContractError, "unsupported contract version"):
            validate(record)
        record = installation()
        record["arbitrary_environment"] = {"TOKEN": "secret"}
        with self.assertRaisesRegex(ContractError, "unsupported fields"):
            validate(record)

    def test_requires_capture_state_and_fingerprint_shape(self):
        record = session()
        record["capture"]["state"] = "successful-run"
        with self.assertRaisesRegex(ContractError, "complete, partial, or dropped"):
            validate(record)
        record = session()
        record["patch_config_fingerprint"] = "sha256:short"
        with self.assertRaisesRegex(ContractError, "sha256 fingerprint"):
            validate(record)

    def test_validates_milestone_and_rejects_malformed_records(self):
        record = {
            "schema_version": 1,
            "record_type": "milestone",
            "id": new_id("milestone"),
            "session_id": new_id("session"),
            "name": "boot",
            "result": "unknown",
            "monotonic_seconds": 12.5,
            "evidence": {"source": "log", "method": "observed", "timestamp": "unknown"},
            "capture": {"state": "dropped", "provenance": "log"},
        }
        self.assertEqual(validate(record), record)
        self.assertEqual(export_record(record)["result"], "unknown")
        for value, pattern in ((-1, "nonnegative number"), (True, "nonnegative number"), (float("nan"), "nonnegative number")):
            broken = dict(record, monotonic_seconds=value)
            with self.assertRaisesRegex(ContractError, pattern):
                validate(broken)
        with self.assertRaisesRegex(ContractError, "must be one of"):
            validate(dict(record, evidence={"source": "guess", "method": "observed", "timestamp": "unknown"}))
        with self.assertRaisesRegex(ContractError, "missing fields: evidence"):
            validate({key: value for key, value in record.items() if key != "evidence"})
        with self.assertRaisesRegex(ContractError, "invalid stable ID"):
            validate(dict(record, session_id=record["id"]))
        with self.assertRaisesRegex(ContractError, "must be an object"):
            validate(["not", "a", "record"])

    def test_export_omits_local_paths_and_rejects_unlisted_data(self):
        local = installation()
        exported = export_record(local)
        self.assertNotIn("source_path", exported)
        self.assertNotIn("generated_path", exported)
        self.assertNotIn("/private/game", repr(exported))

    def test_export_carries_partial_or_dropped_capture_state(self):
        record = session()
        self.assertEqual(export_record(record)["capture"]["state"], "partial")
        record["capture"]["state"] = "dropped"
        self.assertEqual(export_record(record)["capture"]["state"], "dropped")

    def test_export_drops_free_text_and_local_paths(self):
        record = {
            "schema_version": 1,
            "record_type": "failure",
            "id": new_id("failure"),
            "session_id": new_id("session"),
            "stage": "launch",
            "category": "unknown",
            "message": "private user data and /home/user/game",
            "evidence": {"source": "trace", "method": "captured", "timestamp": "unknown"},
            "capture": {"state": "complete", "provenance": "unknown", "details": "private path /home/user"},
        }
        exported = export_record(record)
        self.assertNotIn("message", exported)
        self.assertNotIn("details", exported["capture"])
        self.assertNotIn("/home/user", repr(exported))

    def test_cli_help_and_explicit_unsupported_operation(self):
        help_run = subprocess.run([sys.executable, "-m", "tools.companion", "--help"], cwd=ROOT, capture_output=True, text=True)
        self.assertEqual(help_run.returncode, 0, help_run.stderr)
        self.assertIn("Local companion contract scaffold", help_run.stdout)
        operation_run = subprocess.run([sys.executable, "-m", "tools.companion", "install"], cwd=ROOT, capture_output=True, text=True)
        self.assertEqual(operation_run.returncode, 2)
        self.assertIn("not implemented", operation_run.stderr)


if __name__ == "__main__":
    unittest.main()

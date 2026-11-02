import copy
import math
import ntpath
import re
import uuid
from datetime import datetime


VERSION = 1
RECORD_TYPES = frozenset({"installation", "session", "milestone", "failure"})
CAPTURE_STATES = frozenset({"complete", "partial", "dropped"})
UNKNOWN = "unknown"


class ContractError(ValueError):
    pass


def new_id(kind):
    if not isinstance(kind, str) or kind not in RECORD_TYPES:
        raise ContractError(f"unsupported record type: {kind!r}")
    return f"{kind}:{uuid.uuid4()}"


def _object(value, label, required, optional=()):
    if not isinstance(value, dict):
        raise ContractError(f"{label} must be an object")
    missing = set(required) - value.keys()
    extra = value.keys() - set(required) - set(optional)
    if missing:
        raise ContractError(f"{label} is missing fields: {', '.join(sorted(missing))}")
    if extra:
        raise ContractError(f"{label} has unsupported fields: {', '.join(sorted(extra))}")
    return value


def _text(value, label, *, unknown=False):
    if not isinstance(value, str) or not value.strip() or (not unknown and value == UNKNOWN):
        raise ContractError(f"{label} must be a nonempty string" + (" or 'unknown'" if unknown else ""))
    if any(ord(character) < 32 for character in value):
        raise ContractError(f"{label} contains a control character")
    return value


def _identifier(value, label, kind=None):
    value = _text(value, label)
    pattern = rf"{kind}:[0-9a-f]{{8}}-[0-9a-f]{{4}}-[1-8][0-9a-f]{{3}}-[89ab][0-9a-f]{{3}}-[0-9a-f]{{12}}" if kind else r"[a-z][a-z0-9_-]{0,63}"
    if not re.fullmatch(pattern, value):
        raise ContractError(f"{label} has an invalid stable ID")
    return value


def _fingerprint(value, label):
    value = _text(value, label, unknown=True)
    if value != UNKNOWN and not re.fullmatch(r"sha256:[0-9a-f]{64}", value):
        raise ContractError(f"{label} must be 'unknown' or a sha256 fingerprint")
    return value


def _timestamp(value, label):
    value = _text(value, label, unknown=True)
    if value != UNKNOWN:
        try:
            if not re.fullmatch(r"\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?Z", value):
                raise ValueError
            datetime.fromisoformat(value[:-1] + "+00:00")
        except ValueError:
            raise ContractError(f"{label} must be an ISO 8601 UTC timestamp or 'unknown'") from None
    return value


def _local_path(value, label):
    value = _text(value, label)
    if not (value.startswith("/") or ntpath.isabs(value)):
        raise ContractError(f"{label} must be an absolute local path")
    return value


def _evidence(value, label):
    _object(value, label, ("source", "method", "timestamp"))
    sources = {"trace", "log", "manual", "instrumentation", "unknown"}
    methods = {"observed", "asserted", "captured", "unknown"}
    if not isinstance(value["source"], str) or value["source"] not in sources:
        raise ContractError(f"{label}.source must be one of {', '.join(sorted(sources))}")
    if not isinstance(value["method"], str) or value["method"] not in methods:
        raise ContractError(f"{label}.method must be one of {', '.join(sorted(methods))}")
    _timestamp(value["timestamp"], f"{label}.timestamp")
    return value


def _capture(value, label):
    _object(value, label, ("state", "provenance"), ("details",))
    if not isinstance(value["state"], str) or value["state"] not in CAPTURE_STATES:
        raise ContractError(f"{label}.state must be complete, partial, or dropped")
    provenance = {"trace", "log", "manual", "instrumentation", "synthetic", "unknown"}
    if not isinstance(value["provenance"], str) or value["provenance"] not in provenance:
        raise ContractError(f"{label}.provenance must be one of {', '.join(sorted(provenance))}")
    if "details" in value:
        _text(value["details"], f"{label}.details", unknown=True)
    return value


def validate(record):
    if not isinstance(record, dict):
        raise ContractError("record must be an object")
    if not {"schema_version", "record_type", "id"}.issubset(record):
        raise ContractError("record is missing schema_version, record_type, or id")
    if isinstance(record["schema_version"], bool) or not isinstance(record["schema_version"], int) or record["schema_version"] != VERSION:
        raise ContractError(f"unsupported contract version {record['schema_version']!r}; supported version is {VERSION}")
    kind = record["record_type"]
    if not isinstance(kind, str) or kind not in RECORD_TYPES:
        raise ContractError(f"unsupported record type: {kind!r}")
    _identifier(record["id"], "id", kind)
    if kind == "installation":
        _object(record, "installation", ("schema_version", "record_type", "id", "source_path", "generated_path", "title_id", "game_version", "relinker_revision", "library_revision", "patch_config_fingerprint", "created_at"))
        for field in ("source_path", "generated_path"):
            _local_path(record[field], field)
        _timestamp(record["created_at"], "created_at")
        if not isinstance(record["title_id"], str) or not re.fullmatch(r"[A-Z]{4}[0-9]{5}", record["title_id"]):
            raise ContractError("title_id must be a PS title ID such as PPSA00001")
        for field in ("game_version", "relinker_revision", "library_revision"):
            _text(record[field], field, unknown=True)
        _fingerprint(record["patch_config_fingerprint"], "patch_config_fingerprint")
    elif kind == "session":
        _object(record, "session", ("schema_version", "record_type", "id", "installation_id", "process_id", "started_at", "relinker_revision", "library_revision", "patch_config_fingerprint", "cpu", "gpu", "driver", "os", "cache_mode", "instrumentation", "capture"))
        _identifier(record["installation_id"], "installation_id", "installation")
        for field in ("process_id", "relinker_revision", "library_revision", "cpu", "gpu", "driver", "os", "cache_mode", "instrumentation"):
            _text(record[field], field, unknown=True)
        _timestamp(record["started_at"], "started_at")
        _fingerprint(record["patch_config_fingerprint"], "patch_config_fingerprint")
        _capture(record["capture"], "capture")
    elif kind == "milestone":
        _object(record, "milestone", ("schema_version", "record_type", "id", "session_id", "name", "result", "monotonic_seconds", "evidence", "capture"))
        _identifier(record["session_id"], "session_id", "session")
        if not isinstance(record["name"], str) or record["name"] not in {"boot", "menu", "gameplay", "save", "load", "other"}:
            raise ContractError("milestone.name must be boot, menu, gameplay, save, load, or other")
        if not isinstance(record["result"], str) or record["result"] not in {"observed", "failed", "unknown"}:
            raise ContractError("milestone.result must be observed, failed, or unknown")
        seconds = record["monotonic_seconds"]
        if isinstance(seconds, bool) or not isinstance(seconds, (int, float)) or not math.isfinite(seconds) or seconds < 0:
            raise ContractError("milestone.monotonic_seconds must be a nonnegative number")
        _evidence(record["evidence"], "milestone.evidence")
        _capture(record["capture"], "capture")
    else:
        _object(record, "failure", ("schema_version", "record_type", "id", "session_id", "stage", "category", "message", "evidence", "capture"))
        _identifier(record["session_id"], "session_id", "session")
        stages = {"install", "conversion", "launch", "runtime", "shutdown", "unknown"}
        categories = {"unsupported", "missing-import", "crash", "timeout", "validation", "unknown"}
        if not isinstance(record["stage"], str) or record["stage"] not in stages:
            raise ContractError(f"failure.stage must be one of {', '.join(sorted(stages))}")
        if not isinstance(record["category"], str) or record["category"] not in categories:
            raise ContractError(f"failure.category must be one of {', '.join(sorted(categories))}")
        _text(record["message"], "message", unknown=True)
        _evidence(record["evidence"], "failure.evidence")
        _capture(record["capture"], "capture")
    return copy.deepcopy(record)


def export_record(record):
    record = validate(record)
    kind = record["record_type"]
    allowed = {
        "installation": ("schema_version", "record_type", "id", "title_id", "game_version", "relinker_revision", "library_revision", "patch_config_fingerprint", "created_at"),
        "session": ("schema_version", "record_type", "id", "installation_id", "started_at", "relinker_revision", "library_revision", "patch_config_fingerprint", "cpu", "gpu", "driver", "os", "cache_mode", "instrumentation", "capture"),
        "milestone": ("schema_version", "record_type", "id", "session_id", "name", "result", "monotonic_seconds", "evidence", "capture"),
        "failure": ("schema_version", "record_type", "id", "session_id", "stage", "category", "evidence", "capture"),
    }[kind]
    result = {field: copy.deepcopy(record[field]) for field in allowed}
    if "capture" in result and "details" in result["capture"]:
        result["capture"].pop("details")
    return result


def unsupported(operation):
    raise NotImplementedError(f"companion operation {operation!r} is not implemented")

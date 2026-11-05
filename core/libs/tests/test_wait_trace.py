import os
import subprocess
import sys


def main():
    executable = sys.argv[1]
    environment = os.environ.copy()
    environment.pop("APS5_TIME_SCALE", None)
    environment["APS5_TRACE_WAITS"] = "1"
    result = subprocess.run([executable], capture_output=True, check=False, env=environment, text=True, timeout=30)
    if result.returncode != 0:
        raise SystemExit(result.stderr or f"wait trace test exited {result.returncode}")

    if "nan" in result.stderr.lower():
        raise SystemExit(f"wait trace printed nan:\n{result.stderr}")
    reports = [line for line in result.stderr.splitlines() if line.startswith("[waits]")]
    if len(reports) != 2:
        raise SystemExit(f"expected 2 wait trace reports, got {len(reports)}:\n{result.stderr}")
    if reports[0].count(" | ") != 8:
        raise SystemExit(f"first window should list its eight sites:\n{reports[0]}")
    if reports[1].count(" | ") != 2:
        raise SystemExit(f"second window should list only its two active sites:\n{reports[1]}")
    for report in reports:
        if " x0 " in report:
            raise SystemExit(f"wait trace listed a site with no calls in its window:\n{report}")
    if " x1 0.0s (avg 0.0 ms, max 0 ms, 0 timeouts" not in reports[1]:
        raise SystemExit(f"active zero-duration wait is missing from the second window:\n{reports[1]}")


if __name__ == "__main__":
    main()

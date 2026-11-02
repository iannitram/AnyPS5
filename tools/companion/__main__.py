import argparse

from .contracts import unsupported


def main(argv=None):
    parser = argparse.ArgumentParser(prog="python -m tools.companion", description="Local companion contract scaffold")
    parser.add_argument("operation", nargs="?", choices=("install", "diagnose", "test", "compatibility"))
    args = parser.parse_args(argv)
    if args.operation is None:
        parser.print_help()
        return 0
    try:
        unsupported(args.operation)
    except NotImplementedError as error:
        parser.exit(2, f"error: {error}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

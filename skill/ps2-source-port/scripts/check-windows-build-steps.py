"""Check exact arguments for selected steps in a Windows build report."""

import argparse
import json
from pathlib import Path


def requirement(value):
    step, separator, argument = value.partition("=")
    if not separator or not step.strip() or not argument.strip():
        raise argparse.ArgumentTypeError("expected STEP=ARG")
    return step, argument


def check(report, requirements):
    if not isinstance(report, dict) or report.get("status") != "built":
        raise ValueError("expected a built Windows report")
    steps = report.get("steps")
    if not isinstance(steps, list) or any(
        not isinstance(step, dict) or not isinstance(step.get("name"), str)
        or not step["name"].strip()
        for step in steps
    ):
        raise ValueError("malformed build step list")

    for name, argument in requirements:
        matches = [step for step in steps if step["name"] == name]
        if len(matches) != 1:
            issue = "missing" if not matches else "ambiguous"
            raise ValueError(f"{issue} build step {name!r}")
        step = matches[0]
        returncode = step.get("returncode")
        command = step.get("command")
        if type(returncode) is not int or returncode != 0:
            raise ValueError(f"build step {name!r} did not succeed")
        if not isinstance(command, list) or not command or any(
            not isinstance(token, str) or not token for token in command
        ):
            raise ValueError(f"build step {name!r} has no valid command")
        if argument not in command:
            raise ValueError(f"build step {name!r} lacks exact argument {argument!r}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    parser.add_argument("--require", type=requirement, action="append", required=True,
                        metavar="STEP=ARG", help="require one exact argv element")
    args = parser.parse_args()
    try:
        check(json.loads(args.report.read_text(encoding="utf-8-sig")), args.require)
    except (OSError, ValueError) as error:
        parser.exit(1, f"{parser.prog}: {error}\n")
    print(f"Verified {len(args.require)} build step argument requirement(s)")


if __name__ == "__main__":
    main()

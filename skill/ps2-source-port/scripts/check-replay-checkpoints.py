"""Check emitted tick checkpoints in an archived or live native replay."""

import argparse
from dataclasses import dataclass
from pathlib import Path
import shutil
import subprocess
import sys

LIVE_CHECKPOINT_FAILURE = 65


@dataclass(frozen=True)
class Checkpoint:
    event: bytes
    fields: tuple
    tick: int

    def observed_tick(self, line):
        tokens = line.split()
        if not tokens or tokens[0] != self.event:
            return None
        fields = dict(part.split(b"=", 1) for part in tokens[1:] if b"=" in part)
        if any(fields.get(name) != value for name, value in self.fields):
            return None
        tick = fields.get(b"tick", b"")
        if not tick.isdigit():
            raise ValueError(f"{self.event.decode()} emitted an invalid tick")
        return int(tick)


def checkpoint(argument):
    selector, separator, tick = argument.rpartition("@")
    tokens = selector.encode().split()
    if not separator or not tokens or not tick.isascii() or not tick.isdecimal():
        raise argparse.ArgumentTypeError("use 'EVENT key=value @TICK'")
    fields = tuple(tuple(token.split(b"=", 1)) for token in tokens[1:])
    if any(len(field) != 2 or not all(field) or field[0] == b"tick" for field in fields):
        raise argparse.ArgumentTypeError("checkpoint selectors must be key=value fields other than tick")
    return Checkpoint(tokens[0], fields, int(tick))


def check(stream, checkpoints, copy=None):
    pending = iter(checkpoints)
    expected = next(pending, None)
    for line in stream:
        if copy is not None:
            copy.write(line)
        actual = expected.observed_tick(line)
        if actual is None:
            continue
        event = expected.event.decode()
        if actual != expected.tick:
            raise ValueError(f"{event} expected at tick {expected.tick}, observed at {actual}")
        print(f"{event} matched tick {actual}", file=sys.stderr)
        expected = next(pending, None)
        if expected is None:
            return
    raise ValueError(f"missing {expected.event.decode()} at tick {expected.tick}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--expect", type=checkpoint, action="append", required=True,
                        help="ordered first occurrence, e.g. 'QA_SAVE_MAP room=0020 phase=21 @553'")
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--log", type=Path, help="check an existing native replay stderr log")
    source.add_argument("--stderr", type=Path,
                        help="capture live stderr in a fresh file; checkpoint failure exits 65")
    parser.add_argument("--stdout", type=Path, help="capture live stdout; the path must be fresh")
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if args.log:
        if args.stdout or command:
            parser.error("--log does not take a child command or --stdout")
        try:
            with args.log.open("rb") as stream:
                check(stream, args.expect)
        except (OSError, ValueError) as error:
            print(f"Replay checkpoint failed: {error}", file=sys.stderr)
            return 1
        return 0
    if not args.stdout or not command:
        parser.error("live checks need --stdout and a child command after --")
    try:
        with args.stdout.open("xb") as stdout, args.stderr.open("xb") as stderr:
            with subprocess.Popen(command, stdout=stdout, stderr=subprocess.PIPE) as child:
                try:
                    check(child.stderr, args.expect, copy=stderr)
                    shutil.copyfileobj(child.stderr, stderr)
                except (OSError, ValueError, KeyboardInterrupt):
                    child.kill()
                    raise
                return child.wait()
    except ValueError as error:
        print(f"Replay checkpoint failed: {error}", file=sys.stderr)
        return LIVE_CHECKPOINT_FAILURE
    except OSError as error:
        print(f"Could not run replay: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())

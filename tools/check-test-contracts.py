"""Check C test argument modes without compiling or running the tests."""
import argparse
import json
from pathlib import Path
from test_contracts import check_contracts, manifest_cases, TestContractError

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
parser.add_argument("--manifest", type=Path)
args = parser.parse_args()
manifest = args.manifest or args.source_root / "tools/build-native.py"
try:
    result = check_contracts(args.source_root, manifest_cases(manifest))
except TestContractError as error:
    parser.exit(1, str(error) + "\n")
print(json.dumps({"status": "passed", "known": result["known"], "unknown": result["unknown"]}))

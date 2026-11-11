"""Conservative preflight for literal argc assertions in C test entry points.

This catches known argument-count contradictions. Unrecognized or conditional
contracts stay unknown; execution remains the authoritative test.
"""
import ast
from pathlib import Path
import re

MODE_ARGC = {"none": 1, "iso": 2, "elf": 2, "both": 3, "movie": 3, "adx-reference": 3,
             "event-adx-reference": 3, "scratch": 2, "scratch-iso": 3}
_NON_CODE = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
_MAIN = re.compile(r"\bint\s+main\s*\(\s*int\s+argc\b[^)]*\)\s*\{")
_ASSERT = re.compile(r"\b(?:C|CHECK|REQUIRE|ASSERT|assert)\s*\(\s*argc\s*==\s*([0-9]+)\b")


class TestContractError(ValueError):
    pass


def manifest_cases(path):
    tree = ast.parse(Path(path).read_text(encoding="utf-8-sig"))
    matches = [n for n in ast.walk(tree) if isinstance(n, ast.Assign)
               and any(isinstance(t, ast.Name) and t.id == "extra_tests" for t in n.targets)]
    if len(matches) != 1:
        raise TestContractError("Expected one literal extra_tests manifest")
    return ast.literal_eval(matches[0].value)


def literal_argc(source):
    code = _NON_CODE.sub(lambda m: "\n" * m.group().count("\n"), source)
    main = _MAIN.search(code)
    if not main:
        return None
    values = set()
    for match in _ASSERT.finditer(code, main.end()):
        # A branch/loop assertion or an OR/conditional expression does not
        # establish a mandatory entry-point argument count.
        if re.search(r"\b(?:if|switch|for|while)\s*\(", code[main.end():match.start()]):
            continue
        opening = code.index("(", match.start())
        depth, end = 1, opening + 1
        while end < len(code) and depth:
            depth += (code[end] == "(") - (code[end] == ")")
            end += 1
        expression = code[opening + 1:end - 1]
        if depth or "||" in expression or "?" in expression:
            continue
        values.add(int(match.group(1)))
    return next(iter(values)) if len(values) == 1 else None


def check_contracts(root, cases):
    rows, errors = [], []
    for label, source, mode in cases:
        if mode not in MODE_ARGC:
            raise TestContractError("Unknown test argument mode: " + mode)
        path = (Path(root) / source).resolve()
        if Path(root).resolve() not in path.parents:
            raise TestContractError("Test source escaped its project: " + source)
        required = literal_argc(path.read_text(encoding="utf-8-sig"))
        expected = MODE_ARGC[mode]
        rows.append({"test": label, "source": source, "mode": mode,
                     "invoked_argc": expected, "literal_asserted_argc": required})
        if required is not None and required != expected:
            errors.append(f"{label}: mode {mode} supplies argc={expected}, {source} asserts argc={required}")
    if errors:
        raise TestContractError("Test argument mismatch before compilation:\n" + "\n".join(errors))
    return {"known": sum(r["literal_asserted_argc"] is not None for r in rows),
            "unknown": sum(r["literal_asserted_argc"] is None for r in rows), "tests": rows}

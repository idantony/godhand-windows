"""Write Zig/LLVM response files for the project's fixed linker arguments.

Paths use forward slashes and every token is quoted. Embedded quotes/control
characters are rejected rather than misencoded; current linker flags need none.
This is compiler input, never shell command text.
"""
import hashlib
from pathlib import Path


def write_response_file(path, arguments):
    tokens = []
    for argument in arguments:
        token = str(argument).replace("\\", "/")
        if '"' in token or any(ord(c) < 32 for c in token):
            raise ValueError("Unsupported quote/control character in compiler response token")
        tokens.append('"' + token + '"')
    data = ("\n".join(tokens) + "\n").encode("utf-8")
    path = Path(path)
    path.write_bytes(data)
    return {"path": str(path), "sha256": hashlib.sha256(data).hexdigest(), "size": len(data)}

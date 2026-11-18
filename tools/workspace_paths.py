"""Choose the active build and verification workspace for the project."""

import os
from pathlib import Path
from tempfile import NamedTemporaryFile
from time import sleep


def work_root(project: Path) -> Path:
    override = os.environ.get("GODHAND_WORK_ROOT")
    if override:
        root = Path(override).resolve()
        local = (project / ".work").resolve()
        if overlaps_project(root, project) and root != local and local not in root.parents:
            raise ValueError("The active work root inside the project must be under .work")
        return root
    # Frozen source borrows the canonical project's tools through a junction.
    # Resolve it so snapshots use the same caches as the worktree.
    return ((project / ".tools").resolve().parent / ".work").resolve()


def overlaps_project(path: Path, project: Path) -> bool:
    return path == project or project in path.parents


def overlaps_authored_tree(path: Path, project: Path) -> bool:
    local = (project / ".work").resolve()
    return overlaps_project(path, project) and path != local and local not in path.parents


def windows_build_report(scratch: Path, target: str) -> Path:
    return scratch / "build" / ("windows-" + target) / "build-report.json"


def native_build_report(scratch: Path) -> Path:
    return scratch / "logs/native/native-verification.json"


def retry_windows_file_operation(operation):
    """Retry briefly when Windows has a sharing lock on a path."""
    for attempt in range(5):
        try:
            return operation()
        except PermissionError as error:
            if getattr(error, "winerror", None) not in (5, 32, 33) or attempt == 4:
                raise
            sleep(0.05 * (attempt + 1))


def write_build_report(path: Path, data: str) -> None:
    """Publish a complete report, allowing brief Windows file sharing locks."""
    path.parent.mkdir(exist_ok=True)
    temporary = None
    try:
        with NamedTemporaryFile(dir=path.parent, encoding="utf-8", mode="w", delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(data)
        retry_windows_file_operation(lambda: temporary.replace(path))
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)

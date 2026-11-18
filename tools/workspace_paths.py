"""Choose the active build and verification workspace for the project."""

import os
from pathlib import Path


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

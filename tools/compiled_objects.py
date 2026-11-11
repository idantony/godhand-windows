"""Content-keyed Windows C/C++ objects stored in project scratch."""

import hashlib
import json
import os
from pathlib import Path
import re
import tempfile


ENV_OVERRIDES = (
    "CPATH", "C_INCLUDE_PATH", "CPLUS_INCLUDE_PATH", "INCLUDE", "LIB",
    "ZIG_LIB_DIR", "ZIG_LIBC", "NIX_CFLAGS_COMPILE", "NIX_CFLAGS_LINK",
    "NIX_LDFLAGS", "SOURCE_DATE_EPOCH",
)
OBJECT_CACHE_DIRECTORY = "cache/windows-objects"
WINDOWS_COMPILER_FLAGS = (
    "-target", "x86_64-windows-gnu", "-O2", "-Wall", "-Wextra", "-Werror",
    "-ffp-contract=off", "-fno-fast-math", "-D_CRT_SECURE_NO_WARNINGS",
)
LOCAL_INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"\r\n]+)"', re.MULTILINE)


def c_driver(compiler):
    return [compiler, "cc", "-std=c11", *WINDOWS_COMPILER_FLAGS]


def source_hashes(project, names):
    return {name: hashlib.sha256((project / name).read_bytes()).hexdigest() for name in sorted(names)}


def project_inputs(project, sources, tracked=()):
    """Capture object dependencies and any additional source-verification inputs."""
    pending = list(set(sources))
    hashes, includes, time_sensitive = {}, {}, set()
    while pending:
        name = pending.pop()
        if name in hashes:
            continue
        source = project / name
        data = source.read_bytes()
        hashes[name] = hashlib.sha256(data).hexdigest()
        if source.suffix not in (".c", ".cpp", ".h"):
            continue
        if any(macro in data for macro in (b"__DATE__", b"__TIME__", b"__TIMESTAMP__")):
            time_sensitive.add(name)
        dependencies = includes.setdefault(name, [])
        for include in LOCAL_INCLUDE.findall(data.decode("utf-8")):
            header = (source.parent / include).resolve()
            if not header.is_file():
                raise SystemExit("Unresolved project include: " + name + " -> " + include)
            try:
                dependency = header.relative_to(project).as_posix()
            except ValueError:
                raise SystemExit("Project include escaped source tree: " + name + " -> " + include)
            dependencies.append(dependency)
            pending.append(dependency)
    hashes.update(source_hashes(project, set(tracked).difference(hashes)))
    return dict(sorted(hashes.items())), includes, time_sensitive


def toolchain_key(compiler, environment):
    """Identify the compiler bytes and the installed bundled-header layout."""
    if any(environment.get(name) for name in ENV_OVERRIDES):
        return None
    library = compiler.parent / "lib"
    if not library.is_dir():
        return None
    digest = hashlib.sha256()
    with compiler.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    digest.update(os.path.normcase(str(compiler)).encode("utf-8"))
    directories = [(library, "")]
    while directories:
        directory, prefix = directories.pop()
        with os.scandir(directory) as entries:
            for entry in sorted(entries, key=lambda item: item.name):
                if entry.is_symlink():
                    return None
                name = prefix + entry.name
                if entry.is_dir():
                    directories.append((entry.path, name + "/"))
                    continue
                if not entry.is_file():
                    return None
                metadata = entry.stat()
                digest.update(f"{name}\0{metadata.st_size}\0{metadata.st_mtime_ns}\n".encode("utf-8"))
    return digest.hexdigest()


def compilation_key(toolchain, recipe, source, hashes, includes, time_sensitive):
    """Hash the exact source and its recursively included project headers."""
    pending = [source]
    dependencies = set()
    while pending:
        name = pending.pop()
        if name in dependencies:
            continue
        if name in time_sensitive:
            return None
        dependencies.add(name)
        pending.extend(includes.get(name, ()))
    fingerprints = [(name, hashes[name]) for name in sorted(dependencies)]
    encoded = json.dumps([1, toolchain, recipe, fingerprints], separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


class ObjectCache:
    def __init__(self, root, toolchain=None):
        self.root = root
        self.toolchain = toolchain

    @classmethod
    def for_build(cls, root, compiler, environment):
        try:
            toolchain = toolchain_key(compiler, environment)
        except OSError:
            return None
        return cls(root, toolchain) if toolchain else None

    def prepare(self, project, command, source, hashes, includes, time_sensitive):
        # The recipe excludes the compiler path and the final -o destination.
        recipe = [str(argument) for argument in command[1:-2]]
        recipe.append("-ffile-prefix-map=PROJECT=.")
        key = compilation_key(self.toolchain, recipe, source, hashes, includes, time_sensitive)
        if not key:
            return None, command
        return key, [*command[:-2], f"-ffile-prefix-map={project}=.", *command[-2:]]

    def entry(self, key):
        return self.root / key[:2] / key

    def restore(self, key, output):
        try:
            encoded = self.entry(key).read_bytes()
            checksum, payload = encoded[:32], encoded[32:]
            if not payload or checksum != hashlib.sha256(payload).digest():
                return False
            output.write_bytes(payload)
        except OSError:
            return False
        return True

    def store(self, key, output):
        entry = self.entry(key)
        temporary = None
        try:
            entry.parent.mkdir(parents=True, exist_ok=True)
            data = output.read_bytes()
            with tempfile.NamedTemporaryFile(dir=entry.parent, delete=False) as stream:
                temporary = Path(stream.name)
                stream.write(hashlib.sha256(data).digest())
                stream.write(data)
            # Concurrent writers publish complete, independently verified objects.
            temporary.replace(entry)
        except OSError:
            # Cache availability never controls compilation or linking.
            pass
        finally:
            if temporary is not None:
                try:
                    temporary.unlink(missing_ok=True)
                except OSError:
                    pass

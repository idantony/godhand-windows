"""Bounded read-only AFS/FST/SNR2 inspection; no game execution or bulk unpack."""
import argparse
import collections
import hashlib
import json
from pathlib import Path, PurePosixPath
import struct
from inspect_disc import Iso9660, write_json, hashes_file


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bounds(offset, size, total):
    if offset < 0 or size < 0 or offset > total or size > total - offset:
        raise ValueError(f"Out of bounds: {offset:#x}+{size:#x} > {total:#x}")


def cstr(data, offset):
    bounds(offset, 1, len(data))
    end = data.find(b'\0', offset, min(len(data), offset + 4096))
    if end < 0:
        raise ValueError('Unterminated string')
    return data[offset:end].decode('ascii', 'strict')


def parse_fst(data):
    bounds(0, 16, len(data))
    magic, version, strings, entries = struct.unpack_from('<4sIII', data)
    if magic != b'FST\0' or version != 0x3f800000 or not 16 <= strings < entries <= len(data):
        raise ValueError('Unsupported FST header')
    result, visited, paths = [], set(), set()

    def walk(start, prefix, depth):
        if depth > 16 or start in visited or start % 12:
            raise ValueError('FST cycle, depth or alignment violation')
        visited.add(start)
        pos = start
        while True:
            bounds(entries + pos, 12, len(data))
            kind, flags, hint, name_off, value = struct.unpack_from('<BBHII', data, entries + pos)
            if kind == 3:
                return
            if kind not in (1, 2):
                raise ValueError('Unknown FST entry kind')
            bounds(name_off, 1, entries - strings)
            name = cstr(data[strings:entries], name_off)
            if not name or name in ('.', '..') or any(c in name for c in '/\\:'):
                raise ValueError('Unsafe FST component')
            path = '/'.join(prefix + (name,))
            if kind == 1:
                walk(value, prefix + (name,), depth + 1)
            else:
                if path.casefold() in paths:
                    raise ValueError('Duplicate FST path')
                paths.add(path.casefold())
                result.append(dict(path=path, afs_index=value, flags=flags, size_hint=hint,
                                   fst_record_offset=entries + pos))
            pos += 12
    walk(0, (), 0)
    return dict(version='1.0', strings_offset=strings, entries_offset=entries), result


def parse_snr2(data):
    bounds(0, 60, len(data))
    h = struct.unpack_from('<15I', data)
    if data[:4] != b'SNR2' or h[11] != len(data):
        raise ValueError('SNR2 magic/declared size mismatch')
    bounds(h[1], h[2] * 12, len(data))
    bounds(h[3], h[4] * 12, len(data))
    funcs = []
    for index in range(h[4]):
        pos = h[3] + index * 12
        name, code, unk, kind, unk_b = struct.unpack_from('<IIHBB', data, pos)
        # Type is retained: not every symbol is necessarily executable code.
        funcs.append(dict(index=index, record_offset=pos, name_offset=name,
                          name=cstr(data, name) if name else '', image_offset=code,
                          undefined=(code == 0), type=kind, unknown_8=unk, unknown_b=unk_b))
    types = collections.Counter()
    for index in range(h[2]):
        pos = h[1] + index * 12
        patch, kind = struct.unpack_from('<IB', data, pos)
        bounds(patch, 4, len(data))
        types[kind] += 1
    return dict(magic='SNR2', header_words=list(h), declared_size=h[11],
                original_image_name=cstr(data, h[5]) if h[5] else '',
                relocation_table=dict(offset=h[1], count=h[2], stride=12,
                    type_counts=dict(sorted(types.items())),
                    interpretation='Raw 12-byte records; function-index/addend packing unresolved, not applied'),
                function_table=dict(offset=h[3], count=h[4], stride=12),
                exports=dict(offset=h[8], count=h[9], interpretation='Header fields only; export record schema not asserted'),
                undefined_symbols=sum(f['undefined'] for f in funcs), symbols=funcs,
                section_note='No fabricated text/data/BSS boundaries; header does not explicitly encode section lengths')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('iso', type=Path)
    parser.add_argument('--project', type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    project, path = args.project.resolve(), args.iso.resolve()
    before = path.stat()
    baseline = json.loads((project / 'analysis/disc-baseline.json').read_text(encoding='utf-8-sig'))
    # Rehash source, never trust an old path/mtime to establish provenance.
    source = dict(path=str(path), size=before.st_size, **hashes_file(path))
    if source['sha256'] != baseline['source_iso']['sha256']:
        raise ValueError('Source ISO differs from baseline; preserve baseline and inspect version explicitly')
    iso = Iso9660(path)
    try:
        files = {e['path']: e for e in iso.inventory() if not e['directory']}
        fst = iso.contents(files['/GODHAND.FST'], 4 * 1024 * 1024)
        fst_header, named = parse_fst(fst)
        afs = files['/_DATA.AFS']
        base, total = afs['lba'] * 2048, afs['size']
        fixed = iso.read(base, 8)
        magic, count = struct.unpack('<4sI', fixed)
        if magic != b'AFS\0' or not 0 < count <= 100000:
            raise ValueError('Unsupported AFS header/count')
        bounds(0, 8 + count * 8, total)
        table = iso.read(base, 8 + count * 8)
        slots = []
        for i in range(count):
            offset, size = struct.unpack_from('<II', table, 8 + i * 8)
            bounds(offset, size, total)
            if size and offset < len(table):
                raise ValueError('AFS payload overlaps directory')
            slots.append(dict(afs_index=i, afs_offset=offset, iso_offset=base + offset, size=size))
        ranges = sorted((s['afs_offset'], s['afs_offset'] + s['size']) for s in slots if s['size'])
        if any(a[1] > b[0] for a, b in zip(ranges, ranges[1:])):
            raise ValueError('AFS payloads overlap')
        assets, overlays = [], []
        for f in sorted(named, key=lambda f: f['path']):
            if f['afs_index'] >= count:
                raise ValueError('FST index outside AFS')
            entry = {**f, **slots[f['afs_index']]}
            # Hash streaming payload, without unpacking to any filesystem.
            digest = hashlib.sha256()
            for offset in range(0, entry['size'], 4 * 1024 * 1024):
                digest.update(iso.read(entry['iso_offset'] + offset, min(4 * 1024 * 1024, entry['size'] - offset)))
            entry['sha256'] = digest.hexdigest()
            if f['path'].lower().endswith('.rel'):
                if entry['size'] > 16 * 1024 * 1024:
                    raise ValueError('Overlay exceeds bounded inspection size')
                raw = iso.read(entry['iso_offset'], entry['size'])
                overlays.append({**entry, **parse_snr2(raw)})
            assets.append(entry)
        reference = project / 'reference'
        reference.mkdir(parents=True, exist_ok=True)
        (reference / 'GODHAND.FST').write_bytes(fst)
        (reference / 'AFS-directory.bin').write_bytes(table)
        chosen = next((e for e in overlays if PurePosixPath(e['path']).name == 'r207.rel'), overlays[0])
        sample = iso.read(chosen['iso_offset'], chosen['size'])
        target = reference / 'overlays' / PurePosixPath(chosen['path']).name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(sample)
        if sha(target.read_bytes()) != chosen['sha256']:
            raise ValueError('Extracted overlay verification failed')
        if (before.st_size, before.st_mtime_ns) != (path.stat().st_size, path.stat().st_mtime_ns):
            raise ValueError('ISO changed during inspection')
        provenance = dict(source_iso=source, fst_sha256=sha(fst), afs_directory_sha256=sha(table))
        manifest_path = project / 'vendor/god-hand-decomp/disc_extract/rel/manifest.json'
        comparison = dict(note='Compared with supplied upstream manifest, not an independently authenticated retail ISO')
        if manifest_path.exists():
            manifest_raw = manifest_path.read_bytes()
            expected = {e['name']: e for e in json.loads(manifest_raw)}
            rows = []
            for overlay in overlays:
                name = PurePosixPath(overlay['path']).name
                ref = expected.get(name)
                rows.append(dict(name=name, upstream=ref,
                    matches=ref is not None and all((overlay[k] == ref[r]) for k, r in
                        [('afs_index', 'idx'), ('size', 'size'), ('sha256', 'sha256')])))
            comparison.update(manifest_sha256=sha(manifest_raw), checked=len(rows),
                              matching=sum(r['matches'] for r in rows),
                              differences=[r for r in rows if not r['matches']],
                              missing_from_input=sorted(set(expected) - {PurePosixPath(o['path']).name for o in overlays}))
        write_json(project / 'analysis/asset-index.json', dict(schema_version=1, **provenance, entries=assets,
                    unnamed_slots=[s for s in slots if s['afs_index'] not in {f['afs_index'] for f in named}]))
        write_json(project / 'analysis/overlay-index.json', dict(schema_version=1, **provenance, overlays=overlays))
        layout = dict(schema_version=1, **provenance, fst=fst_header, afs=dict(iso_offset=base, size=total,
            slot_count=count, named_file_count=len(assets), overlay_count=len(overlays), directory_size=len(table)),
            evidence=[dict(path='vendor/god-hand-decomp/scripts/afs.py', sha256=sha((project / 'vendor/god-hand-decomp/scripts/afs.py').read_bytes())),
                      dict(path='vendor/god-hand-decomp/scripts/recon/rel_inventory.py', sha256=sha((project / 'vendor/god-hand-decomp/scripts/recon/rel_inventory.py').read_bytes()))],
            limitations=['Input is named Uncensor Patch v1.0; no retail asset equivalence asserted',
                          'FST size_hint is retained but never used as byte size',
                          'SNR2 relocation bytes are inventoried, not executed or applied'],
            upstream_overlay_comparison=comparison,
            verified_read=dict(path=chosen['path'], afs_index=chosen['afs_index'], iso_offset=chosen['iso_offset'],
                size=chosen['size'], sha256=sha(sample), local_path=str(target), magic=sample[:4].decode('ascii'),
                original_image_name=chosen['original_image_name'], first_symbol=chosen['symbols'][0] if chosen['symbols'] else None))
        write_json(project / 'analysis/asset-layout.json', layout)
        print(json.dumps(dict(files=len(assets), overlays=len(overlays), sample=layout['verified_read']), ensure_ascii=False))
    finally:
        iso.close()


if __name__ == '__main__':
    main()

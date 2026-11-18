"""Exercise the real Windows QA pad tape parser without game media."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


PROJECT = Path(__file__).resolve().parents[2]
HEADER = "tick,move,buttons,left_x,left_y,right_x,right_y,connected"
SAMPLE = "100,32768,-32768,32767,4,-9,1"
DRIVER = r'''
#include "src/platform/startup_qa_r100_pad_tape.h"
#include <filesystem>

int wmain(int argc, wchar_t **argv)
{
    if (argc != 7) return 1;
    QAR100PadTape legacy, extended;
    if (!legacy.load(std::filesystem::path(argv[1])) ||
        !extended.load(std::filesystem::path(argv[2]))) return 2;
    for (int index = 3; index < argc; ++index) {
        QAR100PadTape malformed;
        if (malformed.load(std::filesystem::path(argv[index]))) return 3;
    }

    gh_startup_snapshot state = {};
    state.current_room = 0x100;
    state.game_state = 4;
    state.event_number = 1;
    state.event_state = 1;
    state.player_updates_completed = 100;
    legacy.sample(state);
    extended.sample(state);
    state.event_state = 0;

    const auto old_pad = legacy.sample(state);
    if (old_pad.buttons != 32768 || old_pad.left_x != -32768 || old_pad.left_y != 32767 ||
        old_pad.right_x != 4 || old_pad.right_y != -9 || old_pad.connected != 1 ||
        old_pad.left_trigger || old_pad.right_trigger) return 4;
    const unsigned left[] = {31, 0, 0, 255};
    const unsigned right[] = {255, 0, 0, 64};
    for (unsigned index = 0; index < 4; ++index) {
        const auto pad = extended.sample(state);
        if (pad.left_trigger != left[index] || pad.right_trigger != right[index]) return 5;
    }
    return 0;
}
'''


class PadTapeTests(unittest.TestCase):
    @unittest.skipUnless(os.name == "nt", "requires the native Windows QA parser build")
    def test_trigger_tape_width_range_and_same_move_sampling(self):
        work = (PROJECT / ".tools").resolve().parent / ".work"
        scratch = work / "tests/qa-pad-tape"
        scratch.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch, prefix="pad-tape-") as directory:
            folder = Path(directory).resolve()
            self.assertIn(scratch.resolve(), folder.parents)
            source = folder / "driver.cpp"
            source.write_text(DRIVER, encoding="utf-8")
            exe = folder / "driver.exe"
            compiler = PROJECT / ".tools/zig-x86_64-windows-0.17.1/zig.exe"
            built = subprocess.run([str(compiler), "c++", "-target", "x86_64-windows-gnu",
                "-std=c++17", "-Wall", "-Wextra", "-Werror", "-municode",
                "-I", str(PROJECT), str(source), "-o", str(exe)],
                cwd=folder, capture_output=True, text=True, timeout=60, env={
                    **os.environ, "ZIG_GLOBAL_CACHE_DIR": str(work / "cache/zig-global"),
                    "ZIG_LOCAL_CACHE_DIR": str(folder / "zig-local"),
                })
            self.assertEqual(built.returncode, 0, built.stderr)

            extended = HEADER + ",left_trigger,right_trigger"
            extended_rows = [f"{tick},{SAMPLE},{left},{right}"
                             for tick, left, right in ((10, 31, 255), (11, 0, 0), (12, 0, 0), (13, 255, 64))]
            row = f"10,{SAMPLE}"
            rows = [
                ("legacy.csv", HEADER, [row]),
                ("扳机.csv", extended, extended_rows),
                ("overflow.csv", extended, [row + ",256,0"]),
                ("negative.csv", extended, [row + ",0,-1"]),
                ("missing.csv", extended, [row + ",1"]),
                ("extra.csv", HEADER, [row + ",1,2"]),
            ]
            inputs = []
            for name, header, records in rows:
                path = folder / name
                path.write_text("\n".join([header, *records, ""]), encoding="utf-8")
                inputs.append(str(path))
            result = subprocess.run([str(exe), *inputs], capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()

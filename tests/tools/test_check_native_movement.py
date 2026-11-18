import importlib.util
import json
from pathlib import Path
import struct
import unittest


SCRIPT = Path(__file__).resolve().parents[2] / "skill/ps2-source-port/scripts/check-native-movement.py"
SPEC = importlib.util.spec_from_file_location("native_movement", SCRIPT)
MOVEMENT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOVEMENT)


def record(tick, *, position=(0, 0, 0), updates=1, presents=1, hp=100, axes=(0, 127, 0, 0),
           room=0x100, game_state=4):
    return MOVEMENT.PREFIX + json.dumps({
        "tick": tick, "room": room, "next_room": room, "game_state": game_state, "frontier": "00000000",
        "scene_presents": presents,
        "player": {"ready": 1, "hp": hp, "updates": updates,
                   "position_bits": struct.unpack("<3I", struct.pack("<3f", *position))},
        "input": {"analog_valid": 1, "focused": 1, "axes": axes},
    }) + "\n"


class NativeMovementTests(unittest.TestCase):
    def test_requires_real_input_and_measures_endpoints(self):
        lines = [record(5, axes=(0, 0, 0, 0)),
                 record(6, position=(0, 0, 1), updates=2, presents=2),
                 record(7, position=(0, 0, 2), updates=3, presents=3)]
        proof = MOVEMENT.verify(lines, 5, 7, 0x100, 1, lx=0, ly=127)
        self.assertEqual(proof["accepted_ticks"], 2)
        self.assertEqual(proof["horizontal_distance"], 2)
        self.assertEqual(proof["last"]["presents"], 3)

    def test_rejects_missing_duplicate_and_truncated_ticks(self):
        for rows, message in (([record(5), record(7)], "expected tick 6"),
                              ([record(5), record(5)], "expected tick 6"),
                              ([record(5)], "does not cover tick 6")):
            with self.subTest(message=message), self.assertRaisesRegex(ValueError, message):
                MOVEMENT.verify(rows, 5, 7, 0x100, 0.1)

    def test_rejects_counters_input_or_position_that_do_not_prove_movement(self):
        for later, message in (
            (record(6, position=(0, 0, 1), updates=2), "No new player updates or real presents"),
            (record(6, position=(0, 0, 1), presents=2), "No new player updates or real presents"),
            (record(6, position=(0, 1, 0), updates=2, presents=2), "Horizontal movement"),
            (record(6, axes=(0, 0, 127, 0), updates=2, presents=2), "accepted left-stick"),
            (record(6, axes=(0, -127, 0, 0), updates=2, presents=2), "accepted left-stick"),
            (record(6, position=(0, float("nan"), 1), updates=2, presents=2), "nonfinite position"),
        ):
            with self.subTest(message=message), self.assertRaisesRegex(ValueError, message):
                MOVEMENT.verify([record(5), later], 5, 6, 0x100, 0.1, ly=127)

    def test_rejects_other_room_transition_and_changed_hp(self):
        for later, message in ((record(6, room=0x102), "same room"),
                               (record(6, game_state=5), "same room"),
                               (record(6, hp=0), "living player"),
                               (record(6, hp=90), "HP changed")):
            with self.subTest(message=message), self.assertRaisesRegex(ValueError, message):
                MOVEMENT.verify([record(5), later], 5, 6, 0x100, 0.1)


if __name__ == "__main__":
    unittest.main()

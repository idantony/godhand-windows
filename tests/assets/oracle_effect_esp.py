"""Independent binary32 golden for actual31D098/31D108 and type2 scale.
No game data or temporary paths; emits metadata only. Used by FX05 fixture.
"""
import json
import struct

def f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]

def bits(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]

def next_seed(seed):
    return (seed * 0x19660D + 0x3C6EF35F) & 0xFFFFFFFF

def signed(seed):
    value = struct.unpack("<f", struct.pack("<I", (seed & 0x7FFFFF) | 0x3F800000))[0]
    return f32(f32(f32(value + value) - 2.0) - 1.0)

def golden():
    emitted = next_seed(0x12345678)
    xseed = next_seed(emitted)
    yseed = next_seed(xseed)
    # All operations are binary32, rounded separately in original order.
    x = f32(1.0 + f32(0.25 * signed(xseed)))
    y = f32(1.0 + f32(0.5 * signed(yseed)))
    final_seed = yseed
    for _ in range(16):  # R100 angles6+position3+zero-direction4+colour3.
        final_seed = next_seed(final_seed)
    result = dict(emitter_seed=emitted, xseed=xseed, scale_seed=yseed, esp_seed=final_seed, x_bits=bits(x), y_bits=bits(y))
    assert result == dict(emitter_seed=0x75432777, xseed=0xCD305E6A, scale_seed=0x25DBFAC1, esp_seed=0x995E6351, x_bits=0x3F705E6A, y_bits=0x3F9BFAC1)
    return {name: f"{value:08X}" for name, value in result.items()}

if __name__ == "__main__":
    print(json.dumps(golden(), indent=2))

import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import unittest


SCRIPT = Path(__file__).resolve().parents[2] / "skill/ps2-source-port/scripts/win32_keyboard.py"
SPEC = importlib.util.spec_from_file_location("win32_keyboard", SCRIPT)
tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(tool)

WINDOW_FIXTURE = r'''
import ctypes as c
from ctypes import wintypes as w
import json

user = c.WinDLL("user32", use_last_error=True)
kernel = c.WinDLL("kernel32", use_last_error=True)
kernel.GetModuleHandleW.argtypes = [w.LPCWSTR]
kernel.GetModuleHandleW.restype = w.HINSTANCE
window_proc = c.WINFUNCTYPE(c.c_ssize_t, w.HWND, w.UINT, w.WPARAM, w.LPARAM)
user.DefWindowProcW.argtypes = [w.HWND, w.UINT, w.WPARAM, w.LPARAM]
user.DefWindowProcW.restype = c.c_ssize_t
events = []

@window_proc
def receive(hwnd, message, key, flags):
    if message in (7, 256, 257):
        events.append([message, key, flags & 0xFFFFFFFF])
        if message == 257:
            user.PostQuitMessage(0)
        return 0
    return user.DefWindowProcW(hwnd, message, key, flags)

class WindowClass(c.Structure):
    _fields_ = [
        ("style", w.UINT), ("proc", window_proc), ("class_extra", c.c_int),
        ("window_extra", c.c_int), ("instance", w.HINSTANCE), ("icon", w.HICON),
        ("cursor", w.HANDLE), ("background", w.HBRUSH), ("menu", w.LPCWSTR),
        ("name", w.LPCWSTR),
    ]

entry = WindowClass(proc=receive, instance=kernel.GetModuleHandleW(None), name="PortKeyboardFixture")
user.RegisterClassW.argtypes = [c.POINTER(WindowClass)]
user.RegisterClassW.restype = w.ATOM
if not user.RegisterClassW(c.byref(entry)):
    raise c.WinError(c.get_last_error())
user.CreateWindowExW.argtypes = [
    w.DWORD, w.LPCWSTR, w.LPCWSTR, w.DWORD, c.c_int, c.c_int,
    c.c_int, c.c_int, w.HWND, w.HMENU, w.HINSTANCE, c.c_void_p,
]
user.CreateWindowExW.restype = w.HWND
if not user.CreateWindowExW(0, entry.name, "Hidden test fixture", 0,
                            0, 0, 64, 64, None, None, entry.instance, None):
    raise c.WinError(c.get_last_error())
message = w.MSG()
status = user.GetMessageW(c.byref(message), None, 0, 0)
while status > 0:
    user.DispatchMessageW(c.byref(message))
    status = user.GetMessageW(c.byref(message), None, 0, 0)
if status < 0:
    raise c.WinError(c.get_last_error())
print(json.dumps(events))
'''


def event(seconds, message, key):
    return {"seconds": seconds, "message": message, "key": key}


class KeyboardTimeline(unittest.TestCase):
    def test_canonical_receipt_preserves_focus_and_key_order(self):
        events = [event(0, 7, 0), event(0.2, 256, 90), event(0.3, 257, 90)]
        self.assertEqual(tool.checked_timeline(events), [(0, 7, 0), (0.2, 256, 90), (0.3, 257, 90)])

    def test_arbitrary_messages_and_invalid_keys_are_rejected(self):
        for row in (event(0, 0x0010, 0), event(0, 7, 7), event(0, 256, 0), event(0, 257, 255)):
            with self.subTest(row=row), self.assertRaisesRegex(ValueError, "Only focus and virtual-key"):
                tool.checked_timeline([row])

    def test_invalid_or_out_of_order_times_are_rejected(self):
        for seconds in (float("nan"), float("inf"), -1, 1, "4", True, 10 ** 400):
            with self.subTest(seconds=seconds), self.assertRaises(ValueError):
                tool.checked_timeline([event(2, 7, 0), event(seconds, 256, 90)])

    @unittest.skipUnless(os.name == "nt", "requires an actual Win32 window")
    def test_real_hidden_window_receives_only_owned_extended_key(self):
        with subprocess.Popen([sys.executable, "-B", "-c", WINDOW_FIXTURE],
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                              creationflags=subprocess.CREATE_NO_WINDOW) as child:
            try:
                with self.assertRaisesRegex(RuntimeError, "No matching hidden window"):
                    tool.HiddenWindowKeyboard(child, "OtherWindowClass", timeout=0.15)
                keyboard = tool.HiddenWindowKeyboard(child, "PortKeyboardFixture", timeout=4)
                keyboard.post(7)
                keyboard.replay([event(0.05, 256, 39), event(0.1, 257, 39)], time.monotonic())
                stdout, stderr = child.communicate(timeout=4)
            finally:
                if child.poll() is None:
                    child.terminate()
                    child.wait(timeout=4)
        self.assertEqual(child.returncode, 0, stderr)
        received = json.loads(stdout)
        self.assertEqual([row[:2] for row in received], [[7, 0], [256, 39], [257, 39]])
        self.assertEqual(received[1][2] & 0xC1000000, 0x01000000)
        self.assertEqual(received[2][2] & 0xC1000000, 0xC1000000)


if __name__ == "__main__":
    unittest.main()

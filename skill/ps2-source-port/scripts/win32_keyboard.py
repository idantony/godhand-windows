"""Replay keyboard messages to a hidden window owned by a test subprocess."""

import ctypes
from ctypes import wintypes
from math import isfinite
import subprocess
import time


WM_SETFOCUS = 0x0007
WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
# Some layouts return keypad scan codes for the separate navigation keys.
EXTENDED_KEYS = frozenset((*range(0x21, 0x29), 0x2C, 0x2D, 0x2E, 0x5B, 0x5C, 0x5D, 0x6F, 0xA3, 0xA5))


def check_message(message, key):
    if type(message) is not int or type(key) is not int:
        raise ValueError("Keyboard messages and keys require integers")
    if not ((message == WM_SETFOCUS and key == 0)
            or (message in (WM_KEYDOWN, WM_KEYUP) and 1 <= key <= 254)):
        raise ValueError("Only focus and virtual-key messages are supported")


def checked_timeline(events):
    timeline = []
    previous = 0
    for row in events:
        seconds, message, key = row["seconds"], row["message"], row["key"]
        if type(seconds) not in (int, float):
            raise ValueError("Keyboard event times must be numeric")
        try:
            seconds = float(seconds)
        except OverflowError as error:
            raise ValueError("Keyboard event time exceeds the available numeric range") from error
        if not isfinite(seconds) or seconds < previous:
            raise ValueError("Keyboard event times must be finite and nondecreasing")
        check_message(message, key)
        timeline.append((seconds, message, key))
        previous = seconds
    return timeline


class HiddenWindowKeyboard:
    def __init__(self, process, window_class, timeout=12):
        self.process = process
        self.window_class = window_class
        self.user = ctypes.WinDLL("user32", use_last_error=True)
        callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        self.user.EnumWindows.argtypes = [callback, wintypes.LPARAM]
        self.user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
        self.user.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        self.user.IsWindowVisible.argtypes = [wintypes.HWND]
        self.user.MapVirtualKeyW.argtypes = [wintypes.UINT, wintypes.UINT]
        self.user.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
        self.user.PostMessageW.restype = wintypes.BOOL

        matches = []

        @callback
        def find(hwnd, _):
            if self._matches(hwnd):
                matches.append(hwnd)
            return True

        deadline = time.monotonic() + timeout
        while process.poll() is None and time.monotonic() < deadline:
            matches.clear()
            if not self.user.EnumWindows(find, 0):
                raise ctypes.WinError(ctypes.get_last_error())
            if len(matches) > 1:
                raise RuntimeError("Multiple matching windows in the test process")
            if matches:
                self.hwnd = matches[0]
                return
            time.sleep(0.025)
        raise RuntimeError("No matching hidden window in the test process")

    def _matches(self, hwnd):
        pid = wintypes.DWORD()
        self.user.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
        if pid.value != self.process.pid or self.user.IsWindowVisible(hwnd):
            return False
        name = ctypes.create_unicode_buffer(256)
        return bool(self.user.GetClassNameW(hwnd, name, len(name))) and name.value == self.window_class

    def post(self, message, key=0):
        check_message(message, key)
        if self.process.poll() is not None or not self._matches(self.hwnd):
            raise RuntimeError("The owned hidden window vanished before the keyboard event")

        flags = 0
        if key:
            scan = self.user.MapVirtualKeyW(key, 4)
            flags = 1 | ((scan & 0xFF) << 16)
            if scan & 0xFF00 or key in EXTENDED_KEYS:
                flags |= 0x01000000
            if message == WM_KEYUP:
                flags |= 0xC0000000
        if not self.user.PostMessageW(self.hwnd, message, key, flags):
            raise ctypes.WinError(ctypes.get_last_error())

    def replay(self, events, started_at):
        for seconds, message, key in checked_timeline(events):
            remaining = started_at + seconds - time.monotonic()
            if remaining > 0:
                try:
                    self.process.wait(timeout=remaining)
                except subprocess.TimeoutExpired:
                    pass
            self.post(message, key)

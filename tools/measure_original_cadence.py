"""Read the original game's diagnostic title; never inject input or modify memory.

Enable `hidden test`, Y (XP) or `hidden<Tab>test`, Y (Full Tilt) first.
Use real key presses. Each diagnostic value
describes 300 accepted game-loop updates, not monitor refreshes. Repeated polls
are not independent measurements. Standard-library Python, Windows only.
"""
import argparse
import ctypes
import json
import re
import time
from ctypes import wintypes
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--window', type=int, required=True)
    parser.add_argument('--seconds', type=float, default=45)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if not 1 <= args.seconds <= 300:
        parser.error('seconds must be between 1 and 300')
    user = ctypes.WinDLL('user32', use_last_error=True)
    user.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
    user.GetForegroundWindow.restype = wintypes.HWND
    user.IsWindow.argtypes = [wintypes.HWND]
    samples = []
    start = time.perf_counter()
    while time.perf_counter() - start < args.seconds:
        if not user.IsWindow(args.window):
            raise RuntimeError('Target window closed before measurement ended')
        title = ctypes.create_unicode_buffer(256)
        user.GetWindowTextW(args.window, title, len(title))
        match = re.fullmatch(r'Frames/sec =\s*(\d+[.,]\d+)', title.value)
        samples.append(dict(elapsed_s=time.perf_counter() - start,
                            foreground=user.GetForegroundWindow() == args.window,
                            diagnostic_rate=float(match[1].replace(',', '.')) if match else None))
        time.sleep(0.1)
    elapsed = time.perf_counter() - start
    result = dict(method='native diagnostic title, polled every ~100 ms',
                  duration_s=elapsed, samples=samples)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    values = [s['diagnostic_rate'] for s in samples if s['diagnostic_rate'] is not None]
    print(json.dumps(dict(duration_s=elapsed, polls=len(samples),
                          missing=sum(s['diagnostic_rate'] is None for s in samples),
                          background_polls=sum(not s['foreground'] for s in samples),
                          min=min(values) if values else None,
                          max=max(values) if values else None,
                          poll_mean=sum(values)/len(values) if values else None)))


if __name__ == '__main__':
    main()

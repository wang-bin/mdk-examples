#!/usr/bin/env python3
"""Generate a lossless, limited-range HDR10 calibration clip (no external Python packages)."""
# Copyright (c) 2026 WangBin. AI participated.
import argparse
from array import array
import os
from pathlib import Path
import subprocess
import sys


def pq(nits):
    p = (nits / 10000.0) ** (2610 / 16384)
    return ((3424 / 4096 + 2413 / 128 * p) / (1 + 2392 / 128 * p)) ** (2523 / 32)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ffmpeg', default=os.environ.get('FFMPEG_EXE', 'ffmpeg'))
    parser.add_argument('--width', type=int, default=1280)
    parser.add_argument('--height', type=int, default=720)
    parser.add_argument('--output', type=Path,
                        default=Path(__file__).resolve().parents[3] / 'build/cl64/unity-hdr/hdr10-patches.mkv')
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    width, height = args.width, args.height
    if width < 100 or width % 10 or height < 100 or height % 2:
        parser.error('width must be a multiple of 10 >=100; height must be even >=100')
    values = [round(64 + 876 * pq(n)) for n in (0, 80, 100, 203, 1000)]
    row = array('H', (value for value in values for _ in range(width // 5)))
    y = row * height
    uv = array('H', [512]) * (width * height // 2)
    if sys.byteorder != 'little':
        y.byteswap()
        uv.byteswap()
    command = [args.ffmpeg, '-y', '-f', 'rawvideo', '-pixel_format', 'yuv420p10le',
               '-video_size', f'{width}x{height}', '-framerate', '30', '-i', 'pipe:0',
               '-an', '-c:v', 'libx265', '-preset', 'fast', '-x265-params',
               'lossless=1:repeat-headers=1:hdr10=1:colorprim=9:transfer=16:colormatrix=9:range=limited:master-display=G(13250,34500)B(7500,3000)R(34000,16000)WP(15635,16450)L(10000000,1):max-cll=1000,203',
               '-color_primaries', 'bt2020', '-color_trc', 'smpte2084',
               '-colorspace', 'bt2020nc', '-color_range', 'tv', str(args.output)]
    with subprocess.Popen(command, stdin=subprocess.PIPE) as process:
        for frame in range(240):
            plane = array('H', y)
            # Motion is confined to the bottom band; calibration centers remain constant.
            x = frame * 7 % (width - 64)
            for line in range(height * 5 // 6, min(height, height * 5 // 6 + 32)):
                plane[line * width + x:line * width + x + 64] = array('H', [940]) * 64
            process.stdin.write(plane.tobytes())
            process.stdin.write(uv.tobytes())
        process.stdin.close()
        if process.wait() != 0:
            raise SystemExit('FFmpeg fixture encoding failed')
    print(f'{args.output}\n10-bit limited PQ patch codes: {values}')


if __name__ == '__main__':
    main()

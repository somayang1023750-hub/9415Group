"""Generate original 64x64 voxel skins; standard library only, no external art.
The front head face uses the project's existing classic biped UV rectangle.
Committed PNGs are ready to use; this script is only for regenerating them.
"""
import random
import struct
import zlib
from pathlib import Path

root = Path(__file__).resolve().parents[1] / "assets" / "textures"

def write_skin(name, stone):
    rng = random.Random(9415 if stone else 9416)
    pixels = []
    for y in range(64):
        row = []
        for x in range(64):
            n = rng.randrange(-12, 13)
            crack = (y % 8 == 0 or (x + (y // 8 % 2) * 4) % 11 == 0)
            if stone:
                base = (68, 65, 61) if crack else (133, 126, 114)
            else:
                base = (160, 85, 190) if (x + 2*y) % 13 < 2 else (90, 47, 108)
            row.append(tuple(max(0, min(255, c+n)) for c in base))
        pixels.append(row)
    # Head front: +Z, classic atlas [8,16) x [8,16).
    for y in (10, 11):
        for x in ((9,10,13,14) if stone else (8,9,11,12,14,15)):
            pixels[y][x] = (255, 182, 57) if stone else (239, 60, 108)
    if not stone:
        # Dorsal markings keep the squat model readable from the follow camera.
        for y in (3, 4):
            for x in (10, 13):
                pixels[y][x] = (245, 96, 155)
    for x in range(10, 14):
        pixels[14][x] = (44, 39, 36) if stone else (199, 181, 210)
    def chunk(kind, data):
        return struct.pack('!I', len(data)) + kind + data + struct.pack('!I', zlib.crc32(kind+data))
    raw = b''.join(b'\0' + bytes(c for rgb in row for c in rgb) for row in pixels)
    png = b'\x89PNG\r\n\x1a\n'
    png += chunk(b'IHDR', struct.pack('!2I5B',64,64,8,2,0,0,0))
    png += chunk(b'IDAT',zlib.compress(raw)) + chunk(b'IEND',b'')
    (root/name).write_bytes(png)

if __name__ == '__main__':
    write_skin('golem.png', True)
    write_skin('spider.png', False)

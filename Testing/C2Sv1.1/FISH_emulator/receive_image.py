"""
receive_image.py

PC side of FISH_emulator.ino: asks the Arduino to fetch images from the C2S
and saves each one as a JPEG.

    pip install pyserial
    python receive_image.py /dev/cu.usbserial-XXXX            # fetch one image
    python receive_image.py /dev/cu.usbserial-XXXX -n 0       # fetch until C2S has none left
    python receive_image.py /dev/cu.usbserial-XXXX -o captures

Close the Arduino Serial Monitor first; only one program can hold the port.
"""

import argparse
import datetime
import pathlib
import sys
import time

import serial

BAUD = 115200            # must match SERIAL_BAUD in FISH_emulator.ino
LINE_TIMEOUT_S = 10.0    # no output for this long during a transfer = give up


def wait_ready(port):
    # Opening the port resets the Nano; wait for its banner.
    deadline = time.time() + 5.0
    while time.time() < deadline:
        line = port.readline().decode(errors="replace").strip()
        if line.startswith("READY"):
            return
    print("warning: no READY banner, continuing anyway", file=sys.stderr)


def fetch_image(port):
    """Returns (bytes) for a complete image, None if the C2S has none, raises on failure."""
    port.reset_input_buffer()
    port.write(b"i")

    data = bytearray()
    last = time.time()
    while True:
        raw = port.readline()
        if not raw:
            if time.time() - last > LINE_TIMEOUT_S:
                raise RuntimeError(f"timed out after {len(data)} bytes")
            continue
        last = time.time()
        line = raw.decode(errors="replace").strip()

        if line.startswith("D "):
            data += bytes.fromhex(line[2:])
        elif line == "IMG BEGIN":
            data.clear()
        elif line == "IMG NONE":
            return None
        elif line.startswith("IMG ABORT"):
            raise RuntimeError(f"Arduino aborted after {len(data)} bytes (no response from C2S)")
        elif line.startswith("IMG END"):
            _, _, total, checksum = line.split()
            if len(data) != int(total):
                raise RuntimeError(f"length mismatch: got {len(data)}, Arduino sent {total}")
            if sum(data) & 0xFFFF != int(checksum, 16):
                raise RuntimeError("checksum mismatch (serial corruption)")
            return bytes(data)
        elif line.startswith(("HB", "#")):
            print(line)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("port", help="Arduino serial port, e.g. /dev/cu.usbserial-1410")
    ap.add_argument("-n", "--count", type=int, default=1, help="images to fetch (0 = until none left)")
    ap.add_argument("-o", "--out", default="images", help="output directory")
    args = ap.parse_args()

    out = pathlib.Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    with serial.Serial(args.port, BAUD, timeout=1) as port:
        wait_ready(port)
        port.write(b"p")  # stop periodic heartbeats cluttering the stream
        time.sleep(0.1)

        fetched = 0
        while args.count == 0 or fetched < args.count:
            img = fetch_image(port)
            if img is None:
                print("C2S has no new image")
                break

            if not img.startswith(b"\xff\xd8"):
                print("warning: data does not start with a JPEG SOI marker", file=sys.stderr)

            name = datetime.datetime.now().strftime("c2s_%Y%m%d_%H%M%S_%f.jpg")
            path = out / name
            path.write_bytes(img)
            fetched += 1
            print(f"saved {path} ({len(img)} bytes)")


if __name__ == "__main__":
    main()

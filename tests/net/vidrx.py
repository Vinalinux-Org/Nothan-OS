#!/usr/bin/env python3
"""
tests/net/vidrx.py - subscribe to the board's video stream and check every pixel.

The far end of net/video_tx.c.  Sends VSUB, reassembles the datagrams into
frames, and verifies them against the formula the test pattern is generated
from, so the answer is "every pixel of these frames was correct" rather than "a
picture appeared".

  ./vidrx.py                    ten seconds
  ./vidrx.py --secs 30
  ./vidrx.py --save frame.raw   keep one frame, to look at

The pattern is pixel(x, y, seq) = (x + y + seq) & 0xFFFF, stored little endian,
which is what drivers/video/vsource-test.c writes.  Checking it end to end is
the only way to know reassembly put the bytes where they belong: a frame
assembled with two chunks transposed still looks like a plausible picture.

Nothing is verified while the socket is live.  Building an expected frame in
python takes longer than the board takes to send one, so checking inline would
back the receive queue up and report the resulting drops as the board's loss.
Frames are kept and compared after the stream is stopped.
"""

import argparse
import socket
import struct
import sys
import time

PORT = 5004
HDR = 16
MAGIC = 0x4E56
F_LAST = 0x01


def expected_frame(w, h, seq):
    """The pattern the board should have sent, as bytes."""
    out = bytearray()
    for y in range(h):
        base = (y + seq) & 0xFFFF
        out += struct.pack(f"<{w}H", *[(base + x) & 0xFFFF for x in range(w)])
    return bytes(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--board", default="10.42.0.2")
    ap.add_argument("--secs", type=float, default=10.0)
    ap.add_argument("--save", default=None)
    ap.add_argument("--verify", type=int, default=10,
                    help="how many complete frames to keep and check after")
    args = ap.parse_args()

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 8 << 20)
    s.bind(("", 0))
    s.settimeout(2.0)
    s.sendto(b"VSUB", (args.board, PORT))
    print(f"subscribed to {args.board}:{PORT} from port {s.getsockname()[1]}")

    cur_seq = None          # frame currently being reassembled
    buf = None
    have = 0                # bytes of it actually received
    geom = None

    frames = 0              # frames finished, whole or holed
    complete = 0            # frames that arrived with nothing missing
    chunks = 0
    wire = 0
    holes = 0               # bytes missing across all finished frames
    kept = []               # (seq, w, h, bytes) to verify once the link is idle

    first = None
    t0 = time.monotonic()

    def finish():
        nonlocal frames, complete, holes
        if buf is None or geom is None:
            return
        w, h = geom
        want = w * h * 2
        frames += 1
        if have == want:
            complete += 1
            if len(kept) < args.verify:
                kept.append((cur_seq, w, h, bytes(buf)))
        else:
            holes += want - have

    while time.monotonic() - t0 < args.secs:
        try:
            d, _ = s.recvfrom(2048)
        except socket.timeout:
            print("nothing arrived for 2s — is CONFIG_VIDEO_STREAM on?",
                  file=sys.stderr)
            break

        if len(d) < HDR:
            continue
        magic, ver, flags, seq, off, w, h = struct.unpack(">HBBIIHH", d[:HDR])
        if magic != MAGIC or ver != 1:
            continue

        if first is None:
            first = time.monotonic()
            print(f"stream is {w}x{h}, first frame seq {seq}")

        chunks += 1
        wire += len(d) + 66
        payload = d[HDR:]

        if seq != cur_seq:
            finish()
            cur_seq, geom = seq, (w, h)
            buf, have = bytearray(w * h * 2), 0

        end = off + len(payload)
        if end <= len(buf):
            buf[off:end] = payload
            have += len(payload)

        if flags & F_LAST:
            finish()
            cur_seq, buf, geom = None, None, None

    s.sendto(b"VSTP", (args.board, PORT))
    elapsed = (time.monotonic() - first) if first else 0.0

    if not frames or elapsed <= 0:
        print("no frames arrived")
        return 1

    px = sum(w * h * 2 for _, w, h, _ in kept) or 1
    print(f"\n{frames} frames in {elapsed:.2f}s = {frames / elapsed:.1f} fps")
    print(f"{complete} complete, {frames - complete} with holes "
          f"({holes} bytes missing)")
    print(f"{chunks} datagrams, {8 * wire / elapsed / 1e6:.1f} Mbit/s on the wire")

    if args.save and kept:
        seq, w, h, data = kept[0]
        with open(args.save, "wb") as f:
            f.write(data)
        print(f"saved {w}x{h} RGB565 frame {seq} to {args.save}")

    bad = 0
    for seq, w, h, data in kept:
        if data != expected_frame(w, h, seq):
            bad += 1
    if kept:
        print(f"pixel check: {len(kept) - bad}/{len(kept)} frames "
              f"byte-for-byte correct")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())

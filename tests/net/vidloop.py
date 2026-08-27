#!/usr/bin/env python3
"""
tests/net/vidloop.py - send the board's own camera back to its own screen.

  ./vidloop.py            30 seconds, then stop
  ./vidloop.py --secs 0   until interrupted

The laptop subscribes to the camera stream on 5004, and forwards every frame
straight back to the display on 5005.  Nothing is decoded, resized or
converted — the RGB565 bytes that left the sensor arrive at the panel — so the
picture on the HDMI monitor is the camera's, having crossed the wire twice.

Written because the laptop's own camera cannot be opened by OpenCV, which left
the receiving half with nothing real to show and made every test look like a
broken display.  It also happens to be a better demonstration than a webcam
would be: what appears on the panel has been through capture, USB isochronous
transfer, YUY2 conversion, framing, two Ethernet crossings and reassembly, so
one glance at a recognisable moving picture exercises the whole path at once.
"""

import argparse, socket, struct, sys, time

PORT_TX, PORT_RX = 5004, 5005
HDR, MAGIC = 16, 0x4E56


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--board", default="10.42.0.2")
    ap.add_argument("--secs", type=float, default=30.0)
    args = ap.parse_args()

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 8 << 20)
    s.bind(("", 0))
    s.settimeout(2.0)
    s.sendto(b"VSUB", (args.board, PORT_TX))
    print(f"camera {args.board} -> màn hình {args.board}, qua laptop")

    n = frames = 0
    t0 = last = time.monotonic()
    try:
        while args.secs == 0 or time.monotonic() - t0 < args.secs:
            try:
                d, _ = s.recvfrom(2048)
            except socket.timeout:
                print("không có gì từ board", file=sys.stderr)
                break
            if len(d) <= HDR or struct.unpack(">H", d[:2])[0] != MAGIC:
                continue
            s.sendto(d, (args.board, PORT_RX))      # nguyên xi, không đụng vào
            n += 1
            if d[3] & 0x01:                          # cờ frame cuối
                frames += 1
            now = time.monotonic()
            if now - last >= 2.0:
                el = now - t0
                print(f"{frames} frame ({frames/el:.1f}/s), {n} datagram")
                last = now
    except KeyboardInterrupt:
        pass
    finally:
        s.sendto(b"VSTP", (args.board, PORT_TX))
    print(f"xong: {frames} frame chuyển tiếp")
    return 0


if __name__ == "__main__":
    sys.exit(main())

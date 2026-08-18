#!/usr/bin/env python3
"""
tests/net/vidcall.py - the laptop's end of a call with the board.

Both directions at once:

  laptop webcam ──► 400x240 RGB565 ──► board:5005 ──► HDMI panel, doubled
  board camera  ──► 400x240 RGB565 ──► this window

  ./vidcall.py                    webcam both ways
  ./vidcall.py --no-send          only watch what the board sends
  ./vidcall.py --pattern          send a generated pattern instead of the webcam
  ./vidcall.py --board 10.42.0.2

Sending and receiving run on separate threads because they are paced by
different clocks — the webcam's and the board's — and tying them together
would make each wait for the other.

The wire format is nothan/video_net.h: a 16 byte header per datagram carrying
the frame number and the byte offset, so a lost datagram costs a band of the
picture and nothing else.  Nothing is retransmitted; by the time a replacement
arrived the frame would be two frames old.
"""

import argparse
import socket
import struct
import sys
import threading
import time

import cv2
import numpy as np

W, H = 400, 240
HDR = 16
MAGIC = 0x4E56
VERSION = 1
F_LAST = 0x01
CHUNK = 1472 - HDR

PORT_TX = 5004          # board's control + outbound stream
PORT_RX = 5005          # board's inbound stream

stop = threading.Event()


def gui_works():
    """
    Whether this build of OpenCV can open a window.

    Worth asking rather than assuming: a headless build raises from inside
    imshow, and the traceback names cmake flags, which is a long way from
    "there is no display backend".  It is also not a reason to stop — what this
    script is really for is the counters on the board, and those do not need a
    window.
    """
    try:
        cv2.namedWindow("__probe", cv2.WINDOW_AUTOSIZE)
        cv2.destroyWindow("__probe")
        return True
    except cv2.error:
        return False


def to_rgb565(bgr):
    """OpenCV BGR888 to the RGB565 the panel scans out, little endian."""
    b = bgr[:, :, 0].astype(np.uint16) >> 3
    g = bgr[:, :, 1].astype(np.uint16) >> 2
    r = bgr[:, :, 2].astype(np.uint16) >> 3
    return ((r << 11) | (g << 5) | b).astype("<u2").tobytes()


def from_rgb565(buf, w, h):
    p = np.frombuffer(buf, dtype="<u2").reshape(h, w).astype(np.uint32)
    b = ((p & 0x1F) << 3).astype(np.uint8)
    g = (((p >> 5) & 0x3F) << 2).astype(np.uint8)
    r = (((p >> 11) & 0x1F) << 3).astype(np.uint8)
    return np.dstack((b, g, r))


def sender(sock, board, use_pattern, fps, stats):
    """Capture, convert, chop, send.  Paced by the clock, not by sleep()."""
    cap = None
    if not use_pattern:
        cap = cv2.VideoCapture(0)
        if not cap.isOpened():
            print("no webcam; sending a pattern instead", file=sys.stderr)
            cap = None

    seq = 0
    period = 1.0 / fps
    due = time.monotonic()

    while not stop.is_set():
        now = time.monotonic()
        if now < due:
            time.sleep(due - now)
        due += period
        if due < time.monotonic():          # fell behind: give up the missed frames
            due = time.monotonic()

        if cap is not None:
            ok, bgr = cap.read()
            if not ok:
                continue
            bgr = cv2.resize(bgr, (W, H))
            bgr = cv2.flip(bgr, 1)          # a mirror is what a caller expects
            payload = to_rgb565(bgr)
        else:
            y, x = np.mgrid[0:H, 0:W].astype(np.uint16)
            payload = ((x + y + seq) & 0xFFFF).astype("<u2").tobytes()

        for off in range(0, len(payload), CHUNK):
            piece = payload[off:off + CHUNK]
            flags = F_LAST if off + len(piece) >= len(payload) else 0
            hdr = struct.pack(">HBBIIHH", MAGIC, VERSION, flags, seq, off, W, H)
            try:
                sock.sendto(hdr + piece, (board, PORT_RX))
            except OSError:
                stats["tx_fail"] += 1
        seq += 1
        stats["tx_frames"] += 1

    if cap is not None:
        cap.release()


def receiver(sock, stats, frames):
    """Reassemble by offset; hand each finished frame to the display thread."""
    cur_seq, buf, have, geom = None, None, 0, None

    def finish():
        nonlocal buf
        if buf is None or geom is None:
            return
        w, h = geom
        stats["rx_frames"] += 1
        if have == w * h * 2:
            stats["rx_whole"] += 1
        frames.append((w, h, bytes(buf)))
        del frames[:-1]                     # only the newest is worth drawing

    while not stop.is_set():
        try:
            d, _ = sock.recvfrom(2048)
        except socket.timeout:
            continue
        if len(d) <= HDR:
            continue
        magic, ver, flags, seq, off, w, h = struct.unpack(">HBBIIHH", d[:HDR])
        if magic != MAGIC or ver != VERSION:
            continue

        piece = d[HDR:]
        if seq != cur_seq:
            finish()
            cur_seq, geom = seq, (w, h)
            buf, have = bytearray(w * h * 2), 0
        if off + len(piece) <= len(buf):
            buf[off:off + len(piece)] = piece
            have += len(piece)
        if flags & F_LAST:
            finish()
            cur_seq, buf, geom = None, None, None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--board", default="10.42.0.2")
    ap.add_argument("--fps", type=float, default=30.0)
    ap.add_argument("--no-send", action="store_true")
    ap.add_argument("--pattern", action="store_true")
    ap.add_argument("--secs", type=float, default=0.0,
                    help="stop after this long (0 = until interrupted)")
    ap.add_argument("--snap", default=None,
                    help="write the newest received frame here as PNG, "
                         "once a second — for a build with no window")
    args = ap.parse_args()

    show = gui_works()
    if not show:
        print("no window: this OpenCV build has no GUI backend "
              "(pip uninstall opencv-python-headless brings it back).",
              file=sys.stderr)
        print("running anyway — the numbers are what matter.", file=sys.stderr)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 8 << 20)
    sock.bind(("", 0))
    sock.settimeout(0.5)

    # Subscribing is how the board learns where to send; the reply address is
    # this socket's, so nothing here has to know its own IP.
    sock.sendto(b"VSUB", (args.board, PORT_TX))
    print(f"calling {args.board} — "
          + ("q or Esc to hang up" if show else "Ctrl-C to hang up"))

    stats = {"tx_frames": 0, "tx_fail": 0, "rx_frames": 0, "rx_whole": 0}
    frames = []

    threads = [threading.Thread(target=receiver, args=(sock, stats, frames),
                                daemon=True)]
    if not args.no_send:
        threads.append(threading.Thread(
            target=sender, args=(sock, args.board, args.pattern, args.fps, stats),
            daemon=True))
    for t in threads:
        t.start()

    t0 = time.monotonic()
    last = t0
    snapped = t0
    try:
        while True:
            now = time.monotonic()

            if frames:
                w, h, data = frames[-1]
                if show:
                    cv2.imshow("board", from_rgb565(data, w, h))
                elif args.snap and now - snapped >= 1.0:
                    cv2.imwrite(args.snap, from_rgb565(data, w, h))
                    snapped = now

            if show:
                if cv2.waitKey(10) & 0xFF in (ord("q"), 27):
                    break
            else:
                time.sleep(0.05)

            if now - last >= 2.0:
                el = now - t0
                print(f"sent {stats['tx_frames']/el:5.1f}/s  "
                      f"recv {stats['rx_frames']/el:5.1f}/s  "
                      f"({stats['rx_whole']} whole of {stats['rx_frames']})")
                last = now

            if args.secs and now - t0 >= args.secs:
                break
    except KeyboardInterrupt:
        pass
    finally:
        stop.set()
        sock.sendto(b"VSTP", (args.board, PORT_TX))
        for t in threads:
            t.join(timeout=1.0)
        if show:
            cv2.destroyAllWindows()

    el = time.monotonic() - t0
    print(f"\nsent {stats['tx_frames']} frames ({stats['tx_fail']} failed), "
          f"received {stats['rx_frames']} ({stats['rx_whole']} whole) "
          f"in {el:.1f}s")
    return 0


if __name__ == "__main__":
    sys.exit(main())

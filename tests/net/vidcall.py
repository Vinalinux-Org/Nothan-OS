#!/usr/bin/env python3
"""
tests/net/vidcall.py - the laptop's end of a call with the board.

Both directions at once:

  laptop webcam ──► 400x240 RGB565 ──► board:5005 ──► HDMI panel, doubled
  board camera  ──► 320x240 RGB565 ──► http://localhost:8080

  ./vidcall.py                    a call, both directions
  ./vidcall.py --no-send          only watch what the board sends
  ./vidcall.py --pattern          send a generated pattern instead of the webcam
  ./vidcall.py --board 10.42.0.2

The picture is watched in a browser rather than in a window of its own.  A
window needs a GUI build of OpenCV, and the headless package — which another
tool on this machine depends on — installs over it; that is a viewer which
breaks for reasons that have nothing to do with this project.  Every browser
already plays multipart/x-mixed-replace, so the viewer is something the
machine has anyway, and it works from a phone on the same network too.

The JPEG exists only between here and the browser.  Nothing compressed goes
anywhere near the board: the wire carries raw RGB565, which is the design
being demonstrated.

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
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

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


PAGE = b"""<!doctype html><title>NothanOS camera</title>
<style>body{background:#111;margin:0;display:grid;place-items:center;height:100vh}
img{image-rendering:pixelated;width:min(96vw,960px)}</style>
<img src="/stream">"""


def serve_http(port, frames, stats):
    """
    Show the stream in a browser instead of a window.

    cv2.imshow needs a GUI build of OpenCV, and the headless package shadows
    the one installed here.  Encoding to JPEG does not need a GUI, and every
    browser already knows how to play multipart/x-mixed-replace — so the
    viewer is something the machine already has.  It also means the picture
    can be watched from a phone on the same network, which a window cannot do.

    JPEG is for the screen only.  Nothing compressed goes near the board: the
    wire format is still raw RGB565, which is the whole point of the design.
    """
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *a):
            pass                        # one line per frame is not a log

        def do_GET(self):
            if self.path != "/stream":
                self.send_response(200)
                self.send_header("Content-Type", "text/html")
                self.end_headers()
                self.wfile.write(PAGE)
                return

            self.send_response(200)
            self.send_header("Content-Type",
                             "multipart/x-mixed-replace; boundary=f")
            self.end_headers()
            last = -1
            try:
                while not stop.is_set():
                    if not frames or stats["rx_frames"] == last:
                        time.sleep(0.005)
                        continue
                    last = stats["rx_frames"]
                    w, h, data = frames[-1]
                    ok, jpg = cv2.imencode(".jpg", from_rgb565(data, w, h),
                                           [cv2.IMWRITE_JPEG_QUALITY, 85])
                    if not ok:
                        continue
                    self.wfile.write(b"--f\r\nContent-Type: image/jpeg\r\n"
                                     b"Content-Length: %d\r\n\r\n" % len(jpg))
                    self.wfile.write(jpg.tobytes())
                    self.wfile.write(b"\r\n")
            except (BrokenPipeError, ConnectionResetError):
                pass

    ThreadingHTTPServer(("", port), Handler).serve_forever(poll_interval=0.2)


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
    ap.add_argument("--http", type=int, default=8080,
                    help="port the browser watches on (default 8080)")
    ap.add_argument("--snap", default=None,
                    help="also write the newest frame here as a PNG, once a "
                         "second, for something to keep")
    args = ap.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 8 << 20)
    sock.bind(("", 0))
    sock.settimeout(0.5)

    # Subscribing is how the board learns where to send; the reply address is
    # this socket's, so nothing here has to know its own IP.
    sock.sendto(b"VSUB", (args.board, PORT_TX))
    print(f"calling {args.board} — Ctrl-C to hang up")
    print(f"watch it at http://localhost:{args.http}")

    stats = {"tx_frames": 0, "tx_fail": 0, "rx_frames": 0, "rx_whole": 0}
    frames = []

    threads = [threading.Thread(target=receiver, args=(sock, stats, frames),
                                daemon=True),
               threading.Thread(target=serve_http,
                                args=(args.http, frames, stats), daemon=True)]
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

            if frames and args.snap and now - snapped >= 1.0:
                w, h, data = frames[-1]
                cv2.imwrite(args.snap, from_rgb565(data, w, h))
                snapped = now

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

    el = time.monotonic() - t0
    print(f"\nsent {stats['tx_frames']} frames ({stats['tx_fail']} failed), "
          f"received {stats['rx_frames']} ({stats['rx_whole']} whole) "
          f"in {el:.1f}s")
    return 0


if __name__ == "__main__":
    sys.exit(main())

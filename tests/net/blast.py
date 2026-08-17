#!/usr/bin/env python3
"""
tests/net/blast.py - a one-way stream, the shape a video sender has.

Sends numbered datagrams at a chosen rate and never waits for anything.  The
board's sink counts what arrives and what is missing; nothing here can tell,
which is the point — a receiver of live media has no back channel either.

  ./blast.py --rate 12          12 Mbit/s, roughly one video stream
  ./blast.py --rate 0           as fast as the machine will go
  ./blast.py --size 512 --secs 10

Pacing is done by watching the clock rather than by sleeping per packet:
time.sleep() cannot resolve the tens of microseconds between frames at these
rates, and asking it to would measure the laptop's timer rather than the link.
"""

import argparse
import socket
import struct
import sys
import time

DEFAULT_HOST = '10.42.0.2'
SINK_PORT = 9


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--host', default=DEFAULT_HOST)
    ap.add_argument('--port', type=int, default=SINK_PORT)
    ap.add_argument('--size', type=int, default=1472,
                    help='UDP payload bytes (default 1472, one full frame)')
    ap.add_argument('--rate', type=float, default=12.0,
                    help='Mbit/s at the wire, or 0 for unpaced')
    ap.add_argument('--secs', type=float, default=5.0)
    args = ap.parse_args()

    if args.size < 4:
        sys.exit('size must be at least 4: the sequence number lives there')

    payload = bytes((i * 31 + 7) & 0xFF for i in range(args.size))
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 4 << 20)

    # A frame on the wire is the payload plus UDP, IP, Ethernet, CRC, preamble
    # and the inter-frame gap — 66 bytes that the link spends and the socket
    # never sees.  Pacing on payload alone would overshoot by a tenth.
    on_wire = args.size + 66
    interval = (on_wire * 8) / (args.rate * 1e6) if args.rate > 0 else 0.0

    print(f'{args.rate if args.rate else "unpaced"} Mbit/s, {args.size}B payload '
          f'({on_wire}B on the wire), {args.secs}s -> {args.host}:{args.port}')

    seq = 0
    start = time.perf_counter()
    deadline = start + args.secs
    errors = 0

    while True:
        now = time.perf_counter()
        if now >= deadline:
            break
        if interval:
            # Where this packet should have gone out.  Comparing against the
            # start rather than the previous packet keeps the average right
            # even when the sender is briefly descheduled.
            due = start + seq * interval
            if now < due:
                if due - now > 0.002:
                    time.sleep(due - now - 0.001)
                continue

        pkt = struct.pack('!I', seq) + payload[4:]
        try:
            s.sendto(pkt, (args.host, args.port))
        except OSError:
            errors += 1
        seq += 1

    elapsed = time.perf_counter() - start
    s.close()

    bits = seq * on_wire * 8
    print(f'sent {seq} datagrams in {elapsed:.2f}s = '
          f'{bits / elapsed / 1e6:.1f} Mbit/s at the wire, '
          f'{seq / elapsed:,.0f} pkt/s'
          + (f', {errors} send errors' if errors else ''))
    print('read the board\'s [SINK] line for what arrived')


if __name__ == '__main__':
    main()

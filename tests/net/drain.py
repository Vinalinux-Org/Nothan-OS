#!/usr/bin/env python3
"""
tests/net/drain.py - ask the board for a stream and count what shows up.

The mirror of blast.py.  There the laptop sent and the board counted; here the
board sends and the laptop counts, which is the half that has only ever had an
estimate behind it.

  ./drain.py --count 200000 --size 1472
  ./drain.py --count 500000 --size 64

Two independent numbers come out of one run: the board reports what it managed
to put on the wire and this reports what arrived.  They should agree, and where
they do not the difference is the loss — which is worth more than either
number alone, since a sender that counts its own output is grading its own
work.
"""

import argparse
import socket
import struct
import sys
import time

SOURCE_PORT = 19


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--host', default='10.42.0.2')
    ap.add_argument('--port', type=int, default=SOURCE_PORT)
    ap.add_argument('--count', type=int, default=200000)
    ap.add_argument('--size', type=int, default=1472)
    ap.add_argument('--idle', type=float, default=2.0,
                    help='seconds of silence that ends the run')
    args = ap.parse_args()

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # Large, because the whole stream lands here while Python is in recvfrom;
    # the kernel is the only thing buffering between the wire and this loop.
    s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 32 << 20)
    rcvbuf = s.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
    s.settimeout(args.idle)

    req = b'BLST' + struct.pack('!IH', args.count, args.size)
    s.sendto(req, (args.host, args.port))

    got = 0
    gaps = 0
    reordered = 0
    expect = 0
    first = None
    last = None
    started = False

    while True:
        try:
            data, _ = s.recvfrom(2048)
        except socket.timeout:
            break
        now = time.perf_counter()
        if len(data) < 4:
            continue
        seq = struct.unpack('!I', data[:4])[0]
        if not started:
            started = True
            first = now
            expect = seq
        last = now
        got += 1
        if seq > expect:
            gaps += seq - expect
        elif seq < expect:
            reordered += 1
        expect = seq + 1

    s.close()

    if not got:
        sys.exit('nothing arrived — is the source task bound on port 19?')

    elapsed = (last - first) or 1e-9
    on_wire = args.size + 66
    print(f'asked for {args.count} × {args.size}B')
    print(f'received  {got} ({100 * got / args.count:.2f}%), '
          f'{gaps} gaps, {reordered} reordered')
    print(f'elapsed   {elapsed:.3f}s = {got / elapsed:,.0f} pkt/s, '
          f'{got * on_wire * 8 / elapsed / 1e6:.1f} Mbit/s on the wire, '
          f'{got * args.size * 8 / elapsed / 1e6:.1f} Mbit/s of payload')
    print(f'(SO_RCVBUF {rcvbuf // 1024} KB — if losses look like a cliff, '
          f'suspect this before the board)')
    print("compare with the board's [SRC] line")


if __name__ == '__main__':
    main()

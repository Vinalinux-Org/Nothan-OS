#!/usr/bin/env python3
"""
tests/net/udpecho.py - answer whatever the board sends, and say what it was.

Every other script here talks first and waits for the board to react.  This one
does not talk at all until it is spoken to, which is the half that has never
been exercised: until the socket syscalls landed, nothing in the kernel ever
opened a conversation, so nothing ever had to resolve a link address before
sending.

Run it, then from the board's shell:

    udp 10.42.0.1 9999 hello

Two things are being tested at once and they fail differently:

  * nothing printed here          - the frame never left the board.  Most
                                    likely ARP never resolved, and the board
                                    will have said so itself.
  * printed here, board says
    "sent, no reply in 2 s"       - the board can send but not receive, so the
                                    fault is on the way back: the reply's
                                    destination port, or the receive path.

Written by Doan Phu Hai <haidoan2098@gmail.com>
"""

import argparse
import socket
import sys
import time


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=9999,
                    help="port to listen on (default 9999)")
    ap.add_argument("--bind", default="0.0.0.0",
                    help="address to listen on (default all)")
    ap.add_argument("--prefix", default="echo: ",
                    help="prepended to the reply, so a reply is visibly a "
                         "reply and not the board hearing itself")
    args = ap.parse_args()

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((args.bind, args.port))

    print(f"listening on {args.bind}:{args.port} — ctrl-c to stop")

    n = 0
    try:
        while True:
            data, peer = s.recvfrom(2048)
            n += 1
            stamp = time.strftime("%H:%M:%S")
            # Decoded loosely: this is a datagram off a wire, not a string we
            # wrote, and a decode error should not end the listener.
            text = data.decode("utf-8", errors="replace")
            print(f"[{stamp}] #{n} from {peer[0]}:{peer[1]} "
                  f"{len(data)} B: {text!r}")

            reply = (args.prefix + text).encode("utf-8")
            s.sendto(reply, peer)
    except KeyboardInterrupt:
        print(f"\n{n} datagrams answered")
        return 0


if __name__ == "__main__":
    sys.exit(main())

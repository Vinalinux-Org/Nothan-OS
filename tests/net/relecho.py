#!/usr/bin/env python3
"""
tests/net/relecho.py - the other end of the reliable transport.

Speaks the protocol in nothan/rel.h: a 12-byte header inside UDP, one message
outstanding at a time, acknowledged by sequence.

    ./relecho.py                       # answer on 9999
    ./relecho.py --drop-acks 1         # swallow every other ACK we send
    ./relecho.py --drop-data 1         # swallow every other DATA we receive

The two drop options are the reason this file is worth more than a plain echo
server.  Retransmission is the whole feature and the only thing that exercises
it is loss, which a working cable does not provide.  They fail differently and
both should end with the message still delivered exactly once:

  --drop-data   the board resends until one gets through.  This side should
                print each message once; the board should still see one reply.

  --drop-acks   the board resends a message this side already delivered.  This
                side must recognise the duplicate and NOT print it twice, while
                still acknowledging it — a receiver that stays silent about a
                duplicate makes the sender retransmit for ever.

Written by Doan Phu Hai <haidoan2098@gmail.com>
"""

import argparse
import socket
import struct
import sys
import time

MAGIC = 0x4E54
HDR = 12
DATA = 1
ACK = 2


def parse(pkt):
    """Return (type, session, seq, payload) or None.

    Every field is checked before it is used.  This is a test tool, but it is
    reading a wire, and a parser that trusts a length is how a test tool starts
    reporting the wrong failure.
    """
    if len(pkt) < HDR:
        return None
    magic, typ, rsvd, session, seq = struct.unpack(">HBBII", pkt[:HDR])
    if magic != MAGIC or typ not in (DATA, ACK) or rsvd != 0:
        return None
    return typ, session, seq, pkt[HDR:]


def build(typ, session, seq, payload=b""):
    return struct.pack(">HBBII", MAGIC, typ, 0, session, seq) + payload


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=9999)
    ap.add_argument("--bind", default="0.0.0.0")
    ap.add_argument("--prefix", default="echo: ")
    ap.add_argument("--drop-data", type=int, default=0,
                    metavar="N", help="drop 1 in every N+1 incoming DATA")
    ap.add_argument("--drop-acks", type=int, default=0,
                    metavar="N", help="drop 1 in every N+1 outgoing ACK")
    args = ap.parse_args()

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((args.bind, args.port))
    print(f"reliable echo on {args.bind}:{args.port} — ctrl-c to stop")

    # Per-peer receive state, exactly as rel.c keeps it: the session being
    # followed and the last sequence handed upward.
    seen = {}
    my_session = int(time.time()) & 0xFFFFFFFF
    my_seq = 0

    n_data = n_dup = n_bad = n_dropped_data = n_dropped_ack = 0

    try:
        while True:
            pkt, peer = s.recvfrom(2048)
            p = parse(pkt)
            if not p:
                n_bad += 1
                print(f"  ! {len(pkt)} B from {peer[0]}:{peer[1]} is not this "
                      f"protocol ({n_bad} so far)")
                continue

            typ, session, seq, payload = p

            if typ == ACK:
                # Answers to what this side sent back; nothing here retransmits
                # its own replies, so they are only counted.
                continue

            n_data += 1
            if args.drop_data and n_data % (args.drop_data + 1) == 0:
                n_dropped_data += 1
                print(f"  . dropping DATA seq={seq} on purpose "
                      f"({n_dropped_data} so far)")
                continue

            key = (peer, session)
            last = seen.get(key)
            fresh = last is None or seq == last + 1

            if fresh:
                seen[key] = seq
                text = payload.decode("utf-8", errors="replace")
                stamp = time.strftime("%H:%M:%S")
                print(f"[{stamp}] seq={seq} from {peer[0]}:{peer[1]} "
                      f"{len(payload)} B: {text!r}")

                my_seq += 1
                s.sendto(build(DATA, my_session, my_seq,
                               (args.prefix + text).encode("utf-8")), peer)
            else:
                n_dup += 1
                print(f"  = duplicate seq={seq} (already have {last}) "
                      f"— acknowledging without delivering")

            if args.drop_acks and n_data % (args.drop_acks + 1) == 0:
                n_dropped_ack += 1
                print(f"  . dropping ACK seq={seq} on purpose "
                      f"({n_dropped_ack} so far)")
                continue

            s.sendto(build(ACK, session, seq), peer)

    except KeyboardInterrupt:
        print(f"\n{n_data} DATA ({n_dup} duplicates, {n_bad} not ours); "
              f"dropped {n_dropped_data} DATA and {n_dropped_ack} ACK on purpose")
        return 0


if __name__ == "__main__":
    sys.exit(main())

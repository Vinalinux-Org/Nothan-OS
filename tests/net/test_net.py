#!/usr/bin/env python3
"""
tests/net/test_net.py - the judge.

Builds frames the way a real sender would and checks what comes back against
what the protocol says it should be, computing the checksums independently
rather than asking the stack whether it agrees with itself.  Where the two
disagree the reference is this file, because a stack that grades its own
arithmetic will always pass.
"""

import struct
import subprocess
import sys

BOARD_MAC = bytes.fromhex('880ce05050d6')
PEER_MAC = bytes.fromhex('001122334455')
BOARD_IP = bytes([10, 42, 0, 2])
PEER_IP = bytes([10, 42, 0, 1])

ECHO_PORT = 7
DEAF_PORT = 9


def cksum(data, seed=0):
    """RFC 1071, computed here so the stack is not its own reference."""
    if len(data) % 2:
        data += b'\0'
    s = seed + sum(struct.unpack('!%dH' % (len(data) // 2), data))
    while s >> 16:
        s = (s & 0xFFFF) + (s >> 16)
    return ~s & 0xFFFF


def pseudo(src, dst, proto, length):
    return sum(struct.unpack('!HHHH', src + dst)) + proto + length


def eth(dst=BOARD_MAC, src=PEER_MAC, ethertype=0x0800):
    return dst + src + struct.pack('!H', ethertype)


def ipv4(payload, proto=17, src=PEER_IP, dst=BOARD_IP, ttl=64, ident=0x1234,
         frag=0x4000, ihl=5, version=4, tot_len=None, bad_cksum=False):
    hdr_len = ihl * 4
    if tot_len is None:
        tot_len = hdr_len + len(payload)
    h = struct.pack('!BBHHHBBH', (version << 4) | ihl, 0, tot_len, ident,
                    frag, ttl, proto, 0) + src + dst
    h += b'\0' * (hdr_len - 20)
    c = cksum(h)
    if bad_cksum:
        c ^= 0x0100
    return h[:10] + struct.pack('!H', c) + h[12:] + payload


def udp(data, sport=40000, dport=ECHO_PORT, src=PEER_IP, dst=BOARD_IP,
        length=None, checksum=None):
    if length is None:
        length = 8 + len(data)
    h = struct.pack('!HHHH', sport, dport, length, 0) + data
    if checksum is None:
        c = cksum(h, pseudo(src, dst, 17, length))
        c = c if c else 0xFFFF
    else:
        c = checksum
    return h[:6] + struct.pack('!H', c) + h[8:]


def run(frames):
    """Feed frames; return a list of (list-of-tx-frames) per input, + stats."""
    stdin = '\n'.join(f.hex() for f in frames) + '\n'
    p = subprocess.run(['./harness'], input=stdin, capture_output=True,
                       text=True)
    groups, cur, stats = [], [], {}
    for line in p.stdout.splitlines():
        if line.startswith('TX '):
            cur.append(bytes.fromhex(line[3:]))
        elif line == 'END':
            groups.append(cur)
            cur = []
        elif line.startswith('STATS '):
            for kv in line[6:].split():
                k, v = kv.split('=')
                stats[k] = int(v)
    return groups, stats


RESULTS = []


def check(name, ok, detail=''):
    RESULTS.append((name, ok, detail))
    print(f'{"PASS" if ok else "FAIL"}  {name}' + (f': {detail}' if detail and not ok else ''))


def check_echo_reply(name, request, payload):
    """A well formed echo reply carrying exactly `payload`."""
    groups, _ = run([request])
    tx = groups[0]
    if len(tx) != 1:
        return check(name, False, f'{len(tx)} frames sent, expected 1')

    r = tx[0]
    errs = []
    if r[0:6] != PEER_MAC:
        errs.append('eth dst')
    if r[6:12] != BOARD_MAC:
        errs.append('eth src')
    if r[12:14] != b'\x08\x00':
        errs.append('ethertype')

    ip = r[14:34]
    if cksum(ip) != 0:
        errs.append('ip checksum')
    if ip[9] != 17:
        errs.append('ip proto')
    if ip[8] != 64:
        errs.append(f'ttl={ip[8]}')
    if ip[12:16] != BOARD_IP or ip[16:20] != PEER_IP:
        errs.append('ip addresses')
    if struct.unpack('!H', ip[2:4])[0] != len(r) - 14:
        errs.append('ip tot_len')

    u = r[34:]
    sport, dport, ulen, ucsum = struct.unpack('!HHHH', u[:8])
    if sport != ECHO_PORT:
        errs.append(f'src port {sport}')
    if dport != 40000:
        errs.append(f'dst port {dport}')
    if ulen != len(u):
        errs.append('udp length')
    if ucsum == 0:
        errs.append('udp checksum sent as zero')
    if cksum(u, pseudo(ip[12:16], ip[16:20], 17, ulen)) != 0:
        errs.append('udp checksum')
    if u[8:] != payload:
        errs.append('payload changed')

    check(name, not errs, ', '.join(errs))


def check_silent(name, frame):
    groups, _ = run([frame])
    check(name, len(groups[0]) == 0,
          f'{len(groups[0])} frames sent, expected none')


print('=== echo replies ===')
for n in (0, 1, 8, 63, 64, 512, 1472):
    data = bytes((i * 7 + 3) & 0xFF for i in range(n))
    check_echo_reply(f'{n:>4} byte payload',
                     eth() + ipv4(udp(data)), data)

check_echo_reply('sender omitted the checksum (0 is legal)',
                 eth() + ipv4(udp(b'nocsum', checksum=0)), b'nocsum')

# A payload whose *reply* checksums to zero must go out as 0xFFFF instead.
# The search has to be run over the reply — ports swapped, addresses swapped —
# because that is the datagram whose checksum the stack computes.
found = None
for i in range(0, 65536):
    d = struct.pack('!H', i)
    if cksum(struct.pack('!HHHH', ECHO_PORT, 40000, 10, 0) + d,
             pseudo(BOARD_IP, PEER_IP, 17, 10)) == 0:
        found = d
        break
if found is not None:
    groups, _ = run([eth() + ipv4(udp(found))])
    r = groups[0][0] if groups[0] else b''
    sent = struct.unpack('!H', r[40:42])[0] if len(r) >= 42 else -1
    check('computed-zero checksum goes out as 0xffff', sent == 0xFFFF,
          f'sent 0x{sent:04x}')
else:
    check('computed-zero checksum goes out as 0xffff', False, 'no input found')

print()
print('=== refused ===')
check_silent('bad udp checksum',
             eth() + ipv4(udp(b'x' * 20, checksum=0x1234)))
check_silent('udp length larger than the frame',
             eth() + ipv4(udp(b'hello', length=900)))
check_silent('udp length below the header',
             eth() + ipv4(udp(b'hello', length=4)))
check_silent('truncated udp header',
             eth() + ipv4(b'\x00\x01\x00\x07'))
check_silent('no socket on that port',
             eth() + ipv4(udp(b'hello', dport=1234)))
check_silent('bad ip checksum',
             eth() + ipv4(udp(b'hello'), bad_cksum=True))
check_silent('fragment', eth() + ipv4(udp(b'hello'), frag=0x2001))
check_silent('not our ip', eth() + ipv4(udp(b'hello'), dst=bytes([10, 42, 0, 9])))

# The pseudo header must actually be part of the sum: a datagram whose UDP
# checksum was computed for a different destination has to be refused.
wrong = udp(b'hello', dst=bytes([10, 42, 0, 9]))
check_silent('checksum computed for another host', eth() + ipv4(wrong))

print()
print('=== the ring ===')
# Port 9 is bound and never drained: nine datagrams into eight slots.
frames = [eth() + ipv4(udp(bytes([i]) * 4, dport=DEAF_PORT))
          for i in range(9)]
groups, stats = run(frames)
check('eight slots accepted, the ninth dropped',
      stats.get('deaf_in') == 8 and stats.get('deaf_dropped') == 1,
      f'in={stats.get("deaf_in")} dropped={stats.get("deaf_dropped")}')
check('every accepted datagram woke the owner, and only those',
      stats.get('deaf_wakes') == stats.get('deaf_in') == 8,
      f'wakes={stats.get("deaf_wakes")} accepted={stats.get("deaf_in")}')

# Draining between arrivals must let the ring be reused indefinitely.
frames = [eth() + ipv4(udp(b'round%d' % i)) for i in range(40)]
groups, stats = run(frames)
replied = sum(len(g) for g in groups)
check('forty through eight slots, drained each time',
      replied == 40 and stats.get('echo_out') == 40,
      f'{replied} replies, echo_out={stats.get("echo_out")}')

print()
print('=== icmp still answers (ip demux did not break) ===')
icmp = bytes([8, 0, 0, 0]) + struct.pack('!HH', 0x1234, 1) + b'ping' * 8
icmp = icmp[:2] + struct.pack('!H', cksum(icmp)) + icmp[4:]
groups, _ = run([eth() + ipv4(icmp, proto=1)])
ok = len(groups[0]) == 1 and groups[0][0][34] == 0
check('icmp echo reply', ok)

arp = (eth(dst=b'\xff' * 6, ethertype=0x0806)
       + struct.pack('!HHBBH', 1, 0x0800, 6, 4, 1)
       + PEER_MAC + PEER_IP + b'\0' * 6 + BOARD_IP)
groups, _ = run([arp])
ok = len(groups[0]) == 1 and groups[0][0][21] == 2
check('arp reply', ok)

print()
failed = [n for n, ok, _ in RESULTS if not ok]
print(f'{len(RESULTS) - len(failed)}/{len(RESULTS)} passed')
sys.exit(1 if failed else 0)

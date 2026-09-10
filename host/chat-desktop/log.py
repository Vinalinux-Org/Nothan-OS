"""
log.py - mot cho duy nhat de app noi ra ngoai.

Truoc day moi file tu print theo kieu cua no: net.py co "[net] ...",
main.py co "[app] ...", con lai la print tran. Doc mot lan chay dai thi
khong biet dong nao den truoc dong nao, va khong tat bot duoc phan on ao
khi dang tim mot thu khac.

Ba thu file nay them, va deu la thu chi co gia tri khi CO SAN chu khong
phai khi can den:

  - dau thoi gian, vi mot cuoc goi hong sau muoi giay va mot cuoc goi
    hong ngay lap tuc la hai loi khac nhau, ma dong log thi giong het
  - ten mien (net/video/call/app), de loc
  - muc do, de tat bot mien nao dang on

Doi chieu voi board: UART cua board in "[GUI] chat: to 10.42.0.1: ..."
va o day in "[chat] gui toi 10.42.0.1: ...". Hai dau cua cung mot tin
nen doc canh nhau duoc, nen dinh dang co y giu gan giong.

Written by Doan Phu Hai <haidoan2098@gmail.com>
"""

import os
import sys
import time

# Muc do, thap den cao.
DEBUG, INFO, WARN, ERROR = range(4)

_NAME = {DEBUG: "  ", INFO: "  ", WARN: "! ", ERROR: "!!"}

# Mac dinh in tu INFO tro len. Dat NOTHAN_LOG=debug de xem het.
_LEVEL = {
    "debug": DEBUG, "info": INFO, "warn": WARN, "error": ERROR,
}.get(os.environ.get("NOTHAN_LOG", "").lower(), INFO)

_start = time.monotonic()

# Nguoi nghe them, de mot cua so log trong giao dien co the hien cung
# nhung dong nay ma khong phai cuop stdout.
_sinks = []


def add_sink(fn) -> None:
    """Goi @fn(line) cho moi dong tu day tro di."""
    _sinks.append(fn)


def _emit(level: int, domain: str, msg: str) -> None:
    if level < _LEVEL:
        return
    # Thoi gian tinh tu luc khoi dong chu khong phai gio trong ngay: cai
    # dang doc la "bao lau sau khi bam nut", khong phai "luc may gio".
    line = f"[{time.monotonic() - _start:7.3f}] {_NAME[level]}{domain:5} {msg}"
    print(line, file=sys.stderr if level >= WARN else sys.stdout, flush=True)
    for fn in _sinks:
        try:
            fn(line)
        except Exception:
            # Mot cho nghe hong khong duoc lam hong duong log - do la thu
            # duy nhat con lai de biet vi sao no hong.
            pass


def debug(domain: str, msg: str) -> None: _emit(DEBUG, domain, msg)
def info(domain: str, msg: str) -> None:  _emit(INFO, domain, msg)
def warn(domain: str, msg: str) -> None:  _emit(WARN, domain, msg)
def error(domain: str, msg: str) -> None: _emit(ERROR, domain, msg)

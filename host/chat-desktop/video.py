"""
video.py - nhan luong hinh tu board va rap lai thanh khung.

Board gui hinh tho: khong codec, khong RTP, khong WebRTC. Mot khung
320x240 RGB565 la 153600 byte, chia thanh 106 datagram, moi cai 16 byte
header roi den mot mieng pixel. os-architecture.md §7.2 chon vay va ly do
van dung: day day 100 Mbit chi co hai may, con mot bo giai ma la phan lon
nhat trong ca WebRTC ma o day khong ai can.

DIEU KHIEN LUONG la hai lenh bon byte toi cong 5004 cua board:

    VSUB   bat dau gui ve dia chi vua gui lenh
    VSTP   dung

Board hoc dia chi tu chinh goi VSUB, nen khong phai cau hinh gi o dau ca.

KHUNG THIEU THI BO, khong doi. Mot khung video den muon thi vo dung -
no mo ta mot khoanh khac da qua, va ve no ra chi lam hinh giat lui. Day
la khac biet co ban voi tin nhan, va la ly do §7.2 co hai transport chu
khong phai mot.

Written by Doan Phu Hai <haidoan2098@gmail.com>
"""

import socket
import struct

import log

PORT_TX = 5004          # cong dieu khien + luong ra cua board
PORT_RX = 5005          # cong board nhan hinh vao

HDR = struct.Struct(">HBBIIHH")     # magic, ver, flags, seq, off, w, h
MAGIC = 0x4E56
VERSION = 1
F_LAST = 0x01

MSG_SUBSCRIBE = b"VSUB"
MSG_STOP = b"VSTP"


class VideoLink:
    """
    Mot socket, mot luong vao.

    Khong chan, giong ChatLink: tick() vet het roi tra ve. Goi tu cung
    QTimer voi chat cung duoc - hai thu nay khong dung chung cong nen
    khong vuong nhau.
    """

    def __init__(self, board_ip: str):
        self._board = board_ip
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.setblocking(False)
        self._on = False

        # Khung dang rap. @seq nhan dien no; mieng nao thuoc khung khac
        # thi bo khung cu di chu khong tron hai khung vao nhau.
        self._seq = None
        self._buf = None
        self._geom = None
        self._have = 0

        # Nguoi goi gan vao: nhan (bytes RGB565, w, h) khi mot khung du.
        self.on_frame = None

        self.frames = 0
        self.partial = 0
        self.stray = 0

    # -- dieu khien ------------------------------------------------------

    def start(self) -> None:
        if self._on:
            return
        # Buoc bind truoc khi gui, de dia chi nguon trong goi VSUB la cai
        # board se gui hinh ve. Khong bind thi kernel van chon mot cong,
        # nhung ta khong biet no la cai nao va khong doc duoc tu do.
        try:
            self._sock.bind(("0.0.0.0", 0))
        except OSError:
            pass
        self._sock.sendto(MSG_SUBSCRIBE, (self._board, PORT_TX))
        self._on = True
        log.info("video", f"xin luong tu {self._board}:{PORT_TX}")

    def stop(self) -> None:
        if not self._on:
            return
        self._sock.sendto(MSG_STOP, (self._board, PORT_TX))
        self._on = False
        self._reset()
        log.info("video", f"dung luong — {self.frames} khung, "
                          f"{self.partial} khung thieu bi bo")

    def close(self) -> None:
        self.stop()
        self._sock.close()

    # -- nhip ------------------------------------------------------------

    def tick(self) -> None:
        while True:
            try:
                data, _ = self._sock.recvfrom(2048)
            except BlockingIOError:
                return
            except OSError as e:
                log.warn("video", f"loi doc socket: {e}")
                return
            self._on_datagram(data)

    def _reset(self) -> None:
        self._seq = None
        self._buf = None
        self._geom = None
        self._have = 0

    def _on_datagram(self, d: bytes) -> None:
        if len(d) <= HDR.size:
            return
        magic, ver, flags, seq, off, w, h = HDR.unpack(d[:HDR.size])
        if magic != MAGIC or ver != VERSION:
            self.stray += 1
            return
        if not w or not h:
            self.stray += 1
            return

        piece = d[HDR.size:]

        if seq != self._seq:
            # Khung moi bat dau. Cai dang rap do dang thi bo - dem no de
            # con so mat mat co cho hien ra, chu khong bien mat lang le.
            if self._buf is not None and self._have < len(self._buf):
                self.partial += 1
            self._seq = seq
            self._geom = (w, h)
            self._buf = bytearray(w * h * 2)
            self._have = 0

        # Kiem bien truoc khi ghi. Day la byte tu day dan: mot offset sai
        # se ghi ra ngoai buffer, va o Python thi no khong no ra loi ma
        # lang le keo dai mang - dung thu can bat nhat.
        if off + len(piece) <= len(self._buf):
            self._buf[off:off + len(piece)] = piece
            self._have += len(piece)
        else:
            self.stray += 1

        if flags & F_LAST:
            if self._have == len(self._buf) and self.on_frame:
                self.frames += 1
                self.on_frame(bytes(self._buf), *self._geom)
            elif self._have != len(self._buf):
                self.partial += 1
            self._reset()

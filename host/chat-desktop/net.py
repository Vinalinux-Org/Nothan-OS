"""
net.py - dau kia cua duong truyen, noi voi board.

Board mo socket kieu SOCK_RELIABLE, nen day khong phai UDP tran: ben trong
moi datagram la giao thuc trong nothan-kernel/include/nothan/rel.h, va
ben ngoai no la khung tin nhan trong userspace/gui/services/chat_net.h.
Hai lop, viet lai o day bang Python, vi board khong the noi chuyen voi mot
dau kia chi biet gui byte tho.

    rel   12 byte dau: magic, kieu, session, so thu tu
    chat  1 byte kieu: TEXT / INVITE / ACCEPT / REJECT / BYE

CHI MOT TIN DANG BAY, dung nhu board: gui mot tin, cho ACK, het gio thi
gui lai. Khong phai vi don gian hon ma vi phia kia lam vay - mot cua so
truot o day se gui ba tin lien tiep cho mot may chi xu ly tung cai mot,
va hai cai sau se bi vut di nhu la "vuot truoc".

Written by Doan Phu Hai <haidoan2098@gmail.com>
"""

import random
import socket
import struct
import time

import log

# --- rel.h ---------------------------------------------------------------
REL_MAGIC = 0x4E54          # 'N','T'
REL_HDR = struct.Struct(">HBBII")   # magic, type, reserved, session, seq
REL_DATA = 1
REL_ACK = 2

REL_RETRY_S = 0.200
REL_RETRIES = 10

# --- chat_net.h ----------------------------------------------------------
CHAT_PORT = 6000
MSG_TEXT = 1
MSG_INVITE = 2
MSG_ACCEPT = 3
MSG_REJECT = 4
MSG_BYE = 5

MSG_NAME = {
    MSG_TEXT: "TEXT", MSG_INVITE: "INVITE", MSG_ACCEPT: "ACCEPT",
    MSG_REJECT: "REJECT", MSG_BYE: "BYE",
}


class _Peer:
    """Trang thai voi mot dau kia. Gui va nhan doc lap nhau."""

    def __init__(self):
        # Gui
        self.session = random.getrandbits(32)
        self.next_seq = 1
        self.queue = []          # cac tin cho den luot
        self.in_flight = None    # (seq, payload) da gui, dang cho ACK
        self.deadline = 0.0
        self.retries = 0
        # Nhan
        self.peer_session = None
        self.peer_seq = 0


class ChatLink:
    """
    Mot socket, nhieu dau kia.

    Khong chan: tick() lam het viec va tra ve ngay. Goi no tu QTimer thi
    vong lap giao dien khong bao gio phai doi mang - va 20 ms mot nhip la
    dung nhip board dung, nen hai ben het gio gan nhu cung luc.
    """

    def __init__(self, port=CHAT_PORT):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._sock.bind(("0.0.0.0", port))
        self._sock.setblocking(False)
        self._peers = {}

        # Nguoi goi gan vao. De trong thi tin den bi bo, khong phai loi.
        self.on_text = None       # (addr, str)
        self.on_control = None    # (addr, int)

        log.info("net", f"nghe cong {port}, transport tin cay")

    # -- gui -------------------------------------------------------------

    def send_text(self, addr, text: str) -> None:
        self._queue(addr, bytes([MSG_TEXT]) + text.encode("utf-8"))

    def send_control(self, addr, msg_type: int) -> None:
        self._queue(addr, bytes([msg_type]))

    def _queue(self, addr, payload: bytes) -> None:
        p = self._peers.setdefault(addr, _Peer())
        # Hang doi bon, giong board. Day la mot nguoi go phim, khong phai
        # mot luong du lieu - hang day nghia la dau kia da im mot luc.
        if len(p.queue) >= 4:
            log.warn("net", f"hang doi day, bo mot tin toi {addr}")
            return
        p.queue.append(payload)

    # -- nhip --------------------------------------------------------------

    def tick(self) -> None:
        self._drain_socket()
        now = time.monotonic()
        for addr, p in self._peers.items():
            self._pump_out(addr, p, now)

    def _pump_out(self, addr, p, now) -> None:
        if p.in_flight is None:
            if not p.queue:
                return
            payload = p.queue.pop(0)
            p.in_flight = (p.next_seq, payload)
            p.next_seq += 1
            p.retries = 0
            p.deadline = now + REL_RETRY_S
            self._put_data(addr, p, *p.in_flight)
            return

        if now < p.deadline:
            return

        p.retries += 1
        if p.retries > REL_RETRIES:
            # Bo cuoc. Giu lai thi moi tin phia sau tac theo, va mot may
            # dang tat se lam im hoi thoai vinh vien thay vi tam thoi.
            log.warn("net", f"bo cuoc voi {addr} sau {REL_RETRIES} lan gui lai")
            p.in_flight = None
            return

        p.deadline = now + REL_RETRY_S
        self._put_data(addr, p, *p.in_flight)

    def _put_data(self, addr, p, seq, payload) -> None:
        hdr = REL_HDR.pack(REL_MAGIC, REL_DATA, 0, p.session, seq)
        self._sock.sendto(hdr + payload, addr)

    def _put_ack(self, addr, session, seq) -> None:
        self._sock.sendto(REL_HDR.pack(REL_MAGIC, REL_ACK, 0, session, seq),
                          addr)

    # -- nhan --------------------------------------------------------------

    def _drain_socket(self) -> None:
        # Vet het chu khong lay mot goi: mot nhip 20 ms co the chua nhieu
        # datagram, va bo lai thi hang doi cua he dieu hanh cu day len.
        while True:
            try:
                data, addr = self._sock.recvfrom(2048)
            except BlockingIOError:
                return
            except OSError as e:
                log.error("net", f"loi doc socket: {e}")
                return
            self._on_datagram(data, addr)

    def _on_datagram(self, data: bytes, addr) -> None:
        # Moi truong hop deu kiem do dai truoc khi doc truong. Day la byte
        # tu day dan, khong phai thu minh viet ra.
        if len(data) < REL_HDR.size:
            return
        magic, typ, rsvd, session, seq = REL_HDR.unpack(data[:REL_HDR.size])
        if magic != REL_MAGIC or rsvd != 0:
            return

        p = self._peers.setdefault(addr, _Peer())

        if typ == REL_ACK:
            # Chi tin dang bay moi duoc xac nhan, va chi trong session cua
            # chinh minh: mot ACK muon tu session truoc se rut mot tin chua
            # bao gio den noi.
            if p.in_flight and session == p.session and seq == p.in_flight[0]:
                p.in_flight = None
                p.retries = 0
            return

        if typ != REL_DATA:
            return

        payload = data[REL_HDR.size:]

        if p.peer_session is None or session != p.peer_session:
            # Dau kia moi, hoac vua khoi dong lai. Tin va lay so thu tu cua
            # no lam moc, chu khong doi bat dau tu 1 - bat no lam vay se
            # bien mot lan reboot thanh mot duong truyen hong.
            p.peer_session = session
            p.peer_seq = seq
            deliver = True
        elif seq == p.peer_seq + 1:
            p.peer_seq = seq
            deliver = True
        elif seq <= p.peer_seq:
            # Da nhan roi. ACK bi mat chu khong phai tin - im lang o day la
            # cach mot duong truyen tot gui lai mai mai.
            deliver = False
        else:
            # Vuot truoc. Stop-and-wait khong the sinh ra chuyen nay; bo va
            # khong ACK, de dong ho ben kia gui lai dung cai con thieu.
            return

        self._put_ack(addr, session, seq)

        if not deliver or not payload:
            return

        kind = payload[0]
        if kind == MSG_TEXT:
            if self.on_text:
                self.on_text(addr, payload[1:].decode("utf-8", "replace"))
        else:
            log.info("call", f"{MSG_NAME.get(kind, kind)} tu {addr[0]}")
            if self.on_control:
                self.on_control(addr, kind)

    def close(self) -> None:
        self._sock.close()


def local_addr_for(peer_ip: str):
    """Dia chi cua may nay, nhin tu phia @peer_ip.

    May host co nhieu interface - WiFi, vmnet, dây - va cai nao duoc dung
    la do bang dinh tuyen quyet dinh, khong phai do ta chon. Mo mot socket
    UDP va "connect" toi dau kia khong gui goi nao ra day, nhung buoc kernel
    chon duong, va getsockname() tra ve dung dia chi no se dat vao goi tin.

    Hoi kernel thay vi doan la khac biet giua mot con so dung va mot con so
    trong co ve dung: nguoi ben board se doc con so nay de go vao may ho.
    """
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect((peer_ip, 9))
        return s.getsockname()[0]
    except OSError:
        return None
    finally:
        s.close()


def parse_addr(text: str, default_port=CHAT_PORT):
    """ "10.42.0.2:6000" hoac "10.42.0.2" thanh (ip, port), hoac None.

    Tu choi thay vi doan: mot dia chi go sai ma van dung duoc se gui tin
    toi mot may khong ai dinh, va loi lo ra hai tang duoi duoi dang ARP
    hoi ve mot nguoi la.
    """
    text = text.strip()
    host, _, port = text.partition(":")
    parts = host.split(".")
    if len(parts) != 4:
        return None
    try:
        octets = [int(x) for x in parts]
        if any(o < 0 or o > 255 for o in octets):
            return None
        p = int(port) if port else default_port
        if not (0 < p < 65536):
            return None
    except ValueError:
        return None
    return (host, p)

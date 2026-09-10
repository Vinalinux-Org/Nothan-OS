"""
main.py - cua so chinh cua app nhan tin tren may host.

Hai cot: danh sach hoi thoai ben trai, khung chat ben phai. Day moi la
phan giao dien - chua co socket, chua co luu tru; du lieu ben duoi la
mot dict trong bo nho, mat khi dong app. Cho noi mang la ham
send_message(): moi thu khac chi cham vao store, khong cham vao I/O.

Written by Doan Phu Hai <haidoan2098@gmail.com>
"""

import sys

from PySide6.QtCore import Qt, QSize, QTimer
from PySide6.QtWidgets import (
    QApplication, QFrame, QHBoxLayout, QInputDialog, QLabel, QLineEdit,
    QListWidget, QMessageBox,
    QListWidgetItem, QMainWindow, QPushButton, QScrollArea, QSizePolicy,
    QSplitter, QVBoxLayout, QWidget,
)

import icons
import log
import net
import theme
import call as callui
from call import CallWindow
from widgets import Avatar, Bubble, ConversationRow


# Danh sach hoi thoai. Mot cai, va no la board.
#
# Hai hoi thoai gia truoc day tro 192.168.7.x - khong co ai o do, nen nhan
# vao chung chi lam transport gui lai muoi lan roi bo cuoc. Mot danh sach
# ngan va that thi de doc hon mot danh sach dai va nua that, va nut soan
# tin o goc tren la cach them nguoi moi.
#
# Tin nhan bat dau rong: nhung dong hoi thoai cu la do minh go ra, khong
# phai do ai gui, va mot cua so chat mo len voi nhung cau khong ai noi la
# thu khien nguoi dung khong tin phan con lai cua man hinh.
CONVERSATIONS = [
    {
        "name": "BeagleBone Black",
        "addr": "10.42.0.2:6000",
        "when": "",
        "messages": [],
    },
]


class ChatWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Nothan Chat")
        self.resize(1080, 720)
        self.setMinimumSize(760, 520)

        self._convs = CONVERSATIONS
        self._current = 0
        self._call = None
        self._call_peer = None      # (ip, port) cua cuoc goi dang dien ra

        # Mang truoc giao dien: neu cong da bi chiem thi biet ngay, chu
        # khong phai sau khi go mot tin va thay no khong di dau ca.
        self._link = net.ChatLink()
        self._link.on_text = self._on_net_text
        self._link.on_control = self._on_net_control

        # 20 ms mot nhip, dung nhip task rel tren board. Bang nhau nghia la
        # hai ben gui lai va het gio gan nhu cung luc, nen mot ben khong bao
        # gio bo cuoc trong khi ben kia van con dang co.
        self._pump = QTimer(self)
        self._pump.timeout.connect(self._link.tick)
        self._pump.start(20)

        split = QSplitter(Qt.Horizontal)
        split.setChildrenCollapsible(False)
        split.setHandleWidth(1)
        split.addWidget(self._build_sidebar())
        split.addWidget(self._build_thread())
        split.setStretchFactor(0, 0)
        split.setStretchFactor(1, 1)
        split.setSizes([320, 760])
        self.setCentralWidget(split)

        self.conv_list.setCurrentRow(0)

    # ---------------------------------------------------------------- cot trai

    def _build_sidebar(self) -> QWidget:
        panel = QWidget()
        panel.setObjectName("Sidebar")
        panel.setMinimumWidth(260)
        panel.setMaximumWidth(420)

        box = QVBoxLayout(panel)
        box.setContentsMargins(12, 14, 12, 10)
        box.setSpacing(10)

        head = QHBoxLayout()
        head.setContentsMargins(2, 0, 0, 0)
        title = QLabel("Đoạn chat")
        title.setObjectName("SidebarTitle")
        head.addWidget(title, 1)
        compose = self._icon_button("compose", "Thêm liên hệ", theme.TEXT)
        compose.clicked.connect(self._on_add_contact)
        head.addWidget(compose)
        box.addLayout(head)

        search = QLineEdit()
        search.setObjectName("SearchBox")
        search.setPlaceholderText("Tìm kiếm")
        # Icon nam trong o thay vi nhet vao chuoi placeholder: no khong bi
        # xoa mat khi nguoi dung go, va mau do ta chon chu khong do font.
        search.addAction(icons.icon("search", theme.TEXT_DIM, 16, 1.8),
                         QLineEdit.LeadingPosition)
        search.textChanged.connect(self._on_filter)
        box.addWidget(search)

        self.conv_list = QListWidget()
        self.conv_list.setObjectName("ConvList")
        self.conv_list.setSpacing(1)
        self.conv_list.setVerticalScrollMode(QListWidget.ScrollPerPixel)
        self.conv_list.currentRowChanged.connect(self._on_select)

        for c in self._convs:
            item = QListWidgetItem(self.conv_list)
            row = ConversationRow(c["name"], self._preview(c), c["when"])
            item.setSizeHint(QSize(0, 66))
            self.conv_list.addItem(item)
            self.conv_list.setItemWidget(item, row)

        box.addWidget(self.conv_list, 1)

        # Dia chi cua may nay, de nguoi ben board go vao may ho. Hoi kernel
        # duong nao se duoc dung chu khong ghi cung: neu day mang chua len
        # thi con so nay se khac, va no khac dung luc no nen khac.
        self.self_lbl = QLabel()
        self.self_lbl.setObjectName("SelfAddr")
        self.self_lbl.setToolTip("Địa chỉ máy này — người bên board nhập số này")
        box.addWidget(self.self_lbl)
        self._refresh_self_addr()

        return panel

    def _refresh_self_addr(self) -> None:
        peer = net.parse_addr(self._convs[0]["addr"]) if self._convs else None
        ip = net.local_addr_for(peer[0]) if peer else None
        self.self_lbl.setText(f"Máy này: {ip}:{net.CHAT_PORT}" if ip
                              else "Máy này: chưa có đường mạng")

    @staticmethod
    def _preview(conv: dict) -> str:
        """Dong xem truoc: tin cuoi cung, kem 'Bạn:' neu la cua minh."""
        if not conv["messages"]:
            return "Chưa có tin nhắn"
        sender, text = conv["messages"][-1]
        return f"Bạn: {text}" if sender is None else text

    def _on_filter(self, needle: str) -> None:
        needle = needle.strip().lower()
        for i, c in enumerate(self._convs):
            self.conv_list.item(i).setHidden(needle not in c["name"].lower())

    # --------------------------------------------------------------- cot phai

    def _build_thread(self) -> QWidget:
        panel = QWidget()
        box = QVBoxLayout(panel)
        box.setContentsMargins(0, 0, 0, 0)
        box.setSpacing(0)

        box.addWidget(self._build_header())

        # Vung tin nhan: mot QScrollArea boc mot cot dung. Bong bong duoc
        # them vao truoc dau chen cuoi cung, nen chung luon bi day xuong day
        # khi it tin - giong moi app nhan tin.
        self.scroll = QScrollArea()
        self.scroll.setObjectName("ThreadScroll")
        self.scroll.setWidgetResizable(True)
        self.scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

        holder = QWidget()
        self.msg_box = QVBoxLayout(holder)
        self.msg_box.setContentsMargins(0, 12, 0, 12)
        self.msg_box.setSpacing(3)
        # Dau chen nam o TREN cung: it tin thi chung bi day xuong day khung,
        # dung cho o soan tin - chu khong lo lung o dinh man hinh.
        self.msg_box.addStretch(1)
        self.scroll.setWidget(holder)
        box.addWidget(self.scroll, 1)

        box.addWidget(self._build_composer())
        return panel

    def _build_header(self) -> QWidget:
        bar = QFrame()
        bar.setObjectName("ThreadHeader")
        bar.setFixedHeight(62)

        row = QHBoxLayout(bar)
        row.setContentsMargins(14, 8, 12, 8)
        row.setSpacing(10)

        self.hdr_avatar = Avatar("?", 40)
        row.addWidget(self.hdr_avatar)

        col = QVBoxLayout()
        col.setContentsMargins(0, 0, 0, 0)
        col.setSpacing(1)
        self.hdr_name = QLabel("")
        self.hdr_name.setObjectName("PeerName")
        self.hdr_addr = QLabel("")
        self.hdr_addr.setObjectName("PeerAddr")
        col.addWidget(self.hdr_name)
        col.addWidget(self.hdr_addr)
        row.addLayout(col, 1)

        call_btn = self._icon_button("video", "Gọi video", theme.BLUE)
        call_btn.clicked.connect(self._on_call)
        row.addWidget(call_btn)

        return bar

    def _build_composer(self) -> QWidget:
        bar = QWidget()
        bar.setObjectName("Composer")
        outer = QHBoxLayout(bar)
        outer.setContentsMargins(12, 8, 12, 12)
        outer.setSpacing(8)

        wrap = QFrame()
        wrap.setObjectName("InputWrap")
        inner = QHBoxLayout(wrap)
        inner.setContentsMargins(12, 0, 6, 0)
        inner.setSpacing(4)

        self.input = QLineEdit()
        self.input.setObjectName("Input")
        self.input.setPlaceholderText("Aa")
        self.input.returnPressed.connect(self._on_send)
        self.input.textChanged.connect(self._on_typing)
        inner.addWidget(self.input, 1)

        self.send_btn = QPushButton()
        self.send_btn.setObjectName("SendBtn")
        self.send_btn.setFixedSize(32, 32)
        self.send_btn.setIcon(icons.icon("send", theme.BLUE, 18))
        self.send_btn.setIconSize(QSize(18, 18))
        self.send_btn.setCursor(Qt.PointingHandCursor)
        self.send_btn.setEnabled(False)
        self.send_btn.clicked.connect(self._on_send)
        inner.addWidget(self.send_btn)

        wrap.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        wrap.setFixedHeight(38)
        outer.addWidget(wrap)
        return bar

    def _icon_button(self, name: str, tip: str, color: str) -> QPushButton:
        """Nut chi co icon, ve bang vector - dung cho ca hai cot."""
        b = QPushButton()
        b.setObjectName("IconBtn")
        b.setFixedSize(34, 34)
        b.setIcon(icons.icon(name, color, 19))
        b.setIconSize(QSize(19, 19))
        b.setToolTip(tip)
        b.setCursor(Qt.PointingHandCursor)
        return b

    # ------------------------------------------------------------------ hanh vi

    def _on_typing(self, text: str) -> None:
        # Mau cua QIcon khong theo QSS duoc, nen ta ve lai icon khi doi trang
        # thai thay vi chi bat/tat nut.
        on = bool(text.strip())
        self.send_btn.setEnabled(on)
        self.send_btn.setIcon(
            icons.icon("send", theme.BLUE if on else theme.TEXT_DIM, 18))

    def _on_select(self, row: int) -> None:
        if row < 0:
            return
        self._current = row
        conv = self._convs[row]

        self.hdr_name.setText(conv["name"])
        self.hdr_addr.setText(conv["addr"])

        # Avatar header phai doi theo hoi thoai; Avatar ve theo ten nen cach
        # gon nhat la thay han widget cu.
        new_av = Avatar(conv["name"], 40)
        self.hdr_avatar.parentWidget().layout().replaceWidget(
            self.hdr_avatar, new_av)
        self.hdr_avatar.deleteLater()
        self.hdr_avatar = new_av

        self._reload_messages()

    def _reload_messages(self) -> None:
        # Xoa het bong bong cu, giu lai dau chen o cuoi.
        while self.msg_box.count() > 1:
            item = self.msg_box.takeAt(1)
            w = item.widget()
            if w is not None:
                w.deleteLater()

        msgs = self._convs[self._current]["messages"]
        for i, (sender, text) in enumerate(msgs):
            prv = msgs[i - 1][0] if i else object()
            nxt = msgs[i + 1][0] if i + 1 < len(msgs) else object()
            self._add_bubble(sender, text, show_avatar=sender != nxt,
                             first=sender != prv, last=sender != nxt)
        self._scroll_to_bottom()

    def _add_bubble(self, sender, text: str, show_avatar: bool = True,
                    first: bool = True, last: bool = True):
        b = Bubble(text, mine=sender is None, sender=sender or "",
                   show_avatar=show_avatar, first=first, last=last)
        self.msg_box.addWidget(b)
        return b

    def _scroll_to_bottom(self) -> None:
        # Doi Qt tinh xong bo cuc roi moi cuon, khong thi thanh cuon van con
        # do dai cu va ta cuon hut.
        bar = self.scroll.verticalScrollBar()
        QApplication.processEvents()
        bar.setValue(bar.maximum())

    def _on_send(self) -> None:
        text = self.input.text().strip()
        if not text:
            return
        self.input.clear()

        conv = self._convs[self._current]
        joins = bool(conv["messages"]) and conv["messages"][-1][0] is None
        if joins:
            # Bong bong ngay tren khong con la tin cuoi cua chuoi nua.
            prev = self.msg_box.itemAt(self.msg_box.count() - 1).widget()
            if isinstance(prev, Bubble):
                prev.set_last(False)
        conv["messages"].append((None, text))
        self._add_bubble(None, text, first=not joins, last=True)
        self._scroll_to_bottom()

        # Cap nhat dong xem truoc ben trai cho khop.
        item = self.conv_list.item(self._current)
        row = self.conv_list.itemWidget(item)
        if isinstance(row, ConversationRow):
            row.set_preview(self._preview(conv), "Vừa xong")

        self.send_message(conv["addr"], text)

    # ------------------------------------------------------------ them nguoi

    def _on_add_contact(self) -> None:
        """Hoi ten va dia chi, roi them mot hoi thoai.

        Go tay, vi tren mang nay khong co gi tu gioi thieu. Mot may tra loi
        moi thong bao tren doan day se gom ve bat cu thu gi khac dang cam -
        tim kiem tu dong la mot thiet ke rieng, va cho den khi no ton tai
        thi giao dien trung thuc la giao dien biet hoi.
        """
        name, ok = QInputDialog.getText(self, "Liên hệ mới", "Tên:")
        if not ok or not name.strip():
            return

        addr, ok = QInputDialog.getText(
            self, "Liên hệ mới", "Địa chỉ (ví dụ 10.42.0.2):",
            text="10.42.0.")
        if not ok:
            return

        if net.parse_addr(addr) is None:
            QMessageBox.warning(self, "Địa chỉ không hợp lệ",
                                f"Không đọc được “{addr}”.")
            return

        # Chuan hoa ve dang co cong, de moi dong trong danh sach doc giong
        # nhau du nguoi dung co go cong hay khong.
        ip, port = net.parse_addr(addr)
        self._convs.append({
            "name": name.strip(),
            "addr": f"{ip}:{port}",
            "when": "",
            "messages": [],
        })

        item = QListWidgetItem(self.conv_list)
        row = ConversationRow(name.strip(), f"{ip}:{port}", "")
        item.setSizeHint(QSize(0, 66))
        self.conv_list.addItem(item)
        self.conv_list.setItemWidget(item, row)
        self.conv_list.setCurrentRow(self.conv_list.count() - 1)

    def send_message(self, addr: str, text: str) -> None:
        """Duong cat giua giao dien va mang. Chi ham nay biet den socket."""
        target = net.parse_addr(addr)
        if target is None:
            log.warn("chat", f"dia chi khong hop le: {addr}")
            return
        self._link.send_text(target, text)

    # ------------------------------------------------------------------- mang

    def _conv_by_addr(self, addr):
        """Tim hoi thoai theo dia chi IP cua goi tin den, hoac None.

        Khop theo dia chi chu khong tin mot cai ten trong noi dung: dia chi
        nguon la thu duy nhat tren day ma may nay khong phai duoc ai noi cho
        biet. Tin tu mot dia chi la thi bo - tu sinh hoi thoai tu luu luong
        di qua la cach mot cua so chat day nhung thu khong lien quan.
        """
        for i, c in enumerate(self._convs):
            parsed = net.parse_addr(c["addr"])
            if parsed and parsed[0] == addr[0]:
                return i
        return None

    def _on_net_text(self, addr, text: str) -> None:
        idx = self._conv_by_addr(addr)
        if idx is None:
            log.warn("chat", f"tin tu {addr[0]} khong thuoc hoi thoai nao, bo")
            return

        conv = self._convs[idx]
        conv["messages"].append((conv["name"], text))

        if idx == self._current:
            self._add_bubble(conv["name"], text)
            self._scroll_to_bottom()

        row = self.conv_list.itemWidget(self.conv_list.item(idx))
        if isinstance(row, ConversationRow):
            row.set_preview(self._preview(conv), "Vừa xong")

    def _on_net_control(self, addr, msg_type: int) -> None:
        idx = self._conv_by_addr(addr)
        if idx is None:
            return
        conv = self._convs[idx]

        if msg_type == net.MSG_INVITE:
            if self._call is not None:
                # Ban. Tra loi thay vi im lang: im lang bat nguoi goi nhin
                # man hinh cho ba muoi giay de biet mot dieu may nay da biet.
                self._link.send_control(addr, net.MSG_REJECT)
                return
            self._call_peer = addr
            self._open_call(conv, callui.RINGING)

        elif msg_type == net.MSG_ACCEPT:
            if self._call and self._call_peer == addr:
                self._call.set_state(callui.ACTIVE)

        elif msg_type in (net.MSG_REJECT, net.MSG_BYE):
            if self._call and self._call_peer == addr:
                self._call.set_state(callui.IDLE)

    # ------------------------------------------------------------------- goi

    def _open_call(self, conv, state) -> None:
        addr = self._call_peer

        self._call = CallWindow(
            conv["name"], conv["addr"], state, self,
            on_accept=lambda: self._call_accept(addr),
            on_reject=lambda: self._link.send_control(addr, net.MSG_REJECT),
            on_hangup=lambda: self._link.send_control(addr, net.MSG_BYE),
            board_ip=addr[0],
        )
        self._call.exec()
        self._call = None
        self._call_peer = None

    def _call_accept(self, addr) -> None:
        self._link.send_control(addr, net.MSG_ACCEPT)
        if self._call:
            self._call.set_state(callui.ACTIVE)

    def _on_call(self) -> None:
        conv = self._convs[self._current]
        target = net.parse_addr(conv["addr"])
        if target is None or self._call is not None:
            return

        # Trang thai doi truoc khi loi moi duoc xac nhan, va do khong phai
        # lac quan: transport chiu trach nhiem giao no. Cai nguoi dung thay
        # la "dang goi", va dieu do dung tu khoanh khac ho bam nut.
        self._link.send_control(target, net.MSG_INVITE)
        self._call_peer = target
        self._open_call(conv, callui.CALLING)


def main() -> int:
    app = QApplication(sys.argv)
    app.setApplicationName("Nothan Chat")
    app.setStyleSheet(theme.QSS)
    win = ChatWindow()
    win.show()
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())

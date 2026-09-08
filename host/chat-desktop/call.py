"""
call.py - cua so cuoc goi video.

Chua co video that: khung den o giua la cho danh san cho luong anh tu
board. Nhung cuoc goi thi that - co loi moi tren day, co dau kia bam nhan
hoac tu choi, co luc ca hai cung dong y. Man hinh nay hien dung trang thai
do, vi mot cua so ghi "dang goi" trong khi khong ai duoc moi la thu tra
loi sai cau hoi duy nhat nguoi dung dang hoi.

Written by Doan Phu Hai <haidoan2098@gmail.com>
"""

from PySide6.QtCore import QSize, Qt, QTimer
from PySide6.QtWidgets import (
    QDialog, QFrame, QHBoxLayout, QLabel, QPushButton, QVBoxLayout,
)

import icons
import theme
from widgets import Avatar

# Cung bon trang thai ma call.h tren board dung. Giong nhau la co y: mot
# cuoc goi la mot cau hoi ve viec hai nguoi co dang noi chuyen khong, va
# hai dau cua no khong nen tra loi bang hai bo tu vung.
IDLE, CALLING, RINGING, ACTIVE = range(4)

_LABEL = {
    CALLING: "Đang gọi…",
    RINGING: "Cuộc gọi đến",
    ACTIVE: "Đã kết nối",
    IDLE: "Đã kết thúc",
}


class CallWindow(QDialog):
    """
    @state ban dau la CALLING khi minh goi di, RINGING khi co nguoi goi den.

    Ba callback, tat ca deu tuy chon: cua so nay chi bao ra "nguoi dung vua
    bam gi", con quyet dinh gui goi tin nao la viec cua nguoi goi no.
    """

    def __init__(self, peer: str, addr: str, state: int, parent=None,
                 on_accept=None, on_reject=None, on_hangup=None):
        super().__init__(parent)
        self.setWindowTitle(f"Cuộc gọi · {peer}")
        self.setObjectName("CallRoot")
        self.setMinimumSize(520, 620)
        self.resize(560, 680)

        self._state = state
        self._seconds = 0
        self._on_accept = on_accept
        self._on_reject = on_reject
        self._on_hangup = on_hangup
        self._closing = False

        root = QVBoxLayout(self)
        root.setContentsMargins(24, 28, 24, 24)
        root.setSpacing(14)

        root.addWidget(Avatar(peer, 92), 0, Qt.AlignHCenter)

        name = QLabel(peer)
        name.setObjectName("CallName")
        name.setAlignment(Qt.AlignHCenter)
        root.addWidget(name)

        self.state_lbl = QLabel()
        self.state_lbl.setObjectName("CallState")
        self.state_lbl.setAlignment(Qt.AlignHCenter)
        root.addWidget(self.state_lbl)

        self._addr = addr

        stage = QFrame()
        stage.setObjectName("CallStage")
        stage_l = QVBoxLayout(stage)
        hint = QLabel("Chưa có luồng video")
        hint.setAlignment(Qt.AlignCenter)
        hint.setStyleSheet(f"color: {theme.TEXT_DIM}; background: transparent;")
        stage_l.addWidget(hint)
        root.addWidget(stage, 1)

        ctl = QHBoxLayout()
        ctl.setSpacing(16)
        ctl.addStretch(1)

        self.accept_btn = QPushButton()
        self.accept_btn.setObjectName("AcceptBtn")
        self.accept_btn.setFixedSize(54, 54)
        self.accept_btn.setIcon(icons.icon("call", "#ffffff", 24))
        self.accept_btn.setIconSize(QSize(24, 24))
        self.accept_btn.setCursor(Qt.PointingHandCursor)
        self.accept_btn.setToolTip("Nhận")
        self.accept_btn.clicked.connect(self._accept_clicked)
        ctl.addWidget(self.accept_btn)

        self.hangup_btn = QPushButton()
        self.hangup_btn.setObjectName("HangupBtn")
        self.hangup_btn.setFixedSize(54, 54)
        self.hangup_btn.setIcon(icons.icon("hangup", "#ffffff", 24))
        self.hangup_btn.setIconSize(QSize(24, 24))
        self.hangup_btn.setCursor(Qt.PointingHandCursor)
        self.hangup_btn.setToolTip("Kết thúc")
        self.hangup_btn.clicked.connect(self._hangup_clicked)
        ctl.addWidget(self.hangup_btn)

        ctl.addStretch(1)
        root.addLayout(ctl)

        # Dong ho chi chay khi da ket noi. Dem tu luc mo cua so se dem ca
        # thoi gian cho chuong, va con so do khong phai do dai cuoc goi.
        self._tick = QTimer(self)
        self._tick.timeout.connect(self._on_tick)
        self._tick.start(1000)

        self.set_state(state)

    # -- ben ngoai goi vao khi mang bao trang thai doi --------------------

    def set_state(self, state: int) -> None:
        self._state = state

        if state == ACTIVE:
            m, s = divmod(self._seconds, 60)
            self.state_lbl.setText(f"{m:02d}:{s:02d}")
        else:
            self.state_lbl.setText(f"{_LABEL.get(state, '')}  ·  {self._addr}")

        # Nut nhan chi ton tai khi dang co chuong. Hien no roi lam mo di
        # la mo ta giao thuc; an han di la mo ta viec nguoi dung lam duoc.
        self.accept_btn.setVisible(state == RINGING)
        self.hangup_btn.setToolTip("Từ chối" if state == RINGING
                                   else "Kết thúc")

        if state == IDLE:
            self._close_quietly()

    def _close_quietly(self) -> None:
        """Dong ma khong bao hangup nguoc lai - cuoc goi da ket thuc roi."""
        self._closing = True
        self.accept()

    # -- nut -------------------------------------------------------------

    def _accept_clicked(self) -> None:
        if self._on_accept:
            self._on_accept()

    def _hangup_clicked(self) -> None:
        cb = self._on_reject if self._state == RINGING else self._on_hangup
        if cb:
            cb()
        self._close_quietly()

    def closeEvent(self, ev):
        # Bam dau X cua cua so cung la cup may. Khong bao thi dau kia con
        # ngoi trong mot cuoc goi da khong con ai.
        if not self._closing and self._on_hangup:
            self._on_hangup()
        super().closeEvent(ev)

    def _on_tick(self) -> None:
        if self._state != ACTIVE:
            return
        self._seconds += 1
        m, s = divmod(self._seconds, 60)
        self.state_lbl.setText(f"{m:02d}:{s:02d}")

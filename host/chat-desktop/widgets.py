"""
widgets.py - cac manh nho lap nen giao dien.

Ba thu: avatar tron, mot dong trong danh sach hoi thoai, va mot bong bong
tin nhan. Tach ra day vi ca ba deu duoc dung lai nhieu lan va deu co
phan ve/do rong rieng ma main.py khong can biet.

Written by Doan Phu Hai <haidoan2098@gmail.com>
"""

from PySide6.QtCore import QPointF, QRectF, QSize, Qt
from PySide6.QtGui import (
    QColor, QFont, QLinearGradient, QPainter, QPainterPath, QPen,
)
from PySide6.QtWidgets import (
    QHBoxLayout, QLabel, QSizePolicy, QVBoxLayout, QWidget,
)

import theme


def _initials(name: str) -> str:
    """Lay toi da hai chu cai dau lam noi dung avatar."""
    parts = [p for p in name.split() if p]
    if not parts:
        return "?"
    if len(parts) == 1:
        return parts[0][0].upper()
    return (parts[0][0] + parts[-1][0]).upper()


class Avatar(QWidget):
    """
    Vong tron mau kem chu cai dau.

    Khong dung anh that vi app chua co cho nao de lay anh ve; mot vong tron
    co mau on dinh theo ten da du de mat phan biet duoc cac hoi thoai, va
    khong keo theo file anh nao vao repo.
    """

    def __init__(self, name: str, size: int = 48, parent=None):
        super().__init__(parent)
        self._name = name
        self._size = size
        self.setFixedSize(size, size)

    def sizeHint(self) -> QSize:
        return QSize(self._size, self._size)

    def paintEvent(self, event) -> None:
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        s = self._size

        # Chuyen sac tu tren xuong thay vi mot mau bet: chi lech chung 12%
        # do sang, du de vong tron co khoi chu khong phang li.
        base = QColor(theme.avatar_color(self._name))
        grad = QLinearGradient(0, 0, 0, s)
        grad.setColorAt(0.0, base.lighter(118))
        grad.setColorAt(1.0, base)

        p.setPen(Qt.NoPen)
        p.setBrush(grad)
        p.drawEllipse(0, 0, s, s)

        f = self.font()
        f.setPointSizeF(max(7.0, s * 0.33))
        f.setBold(True)
        f.setLetterSpacing(QFont.PercentageSpacing, 96)
        p.setFont(f)
        p.setPen(QPen(QColor(255, 255, 255, 235)))
        p.drawText(self.rect(), Qt.AlignCenter, _initials(self._name))
        p.end()


class ConversationRow(QWidget):
    """
    Mot dong trong danh sach ben trai: avatar, ten, dong xem truoc.

    La QWidget dat vao QListWidgetItem chu khong phai text thuan, vi mot dong
    can hai co chu khac nhau va mot avatar ve tay - QListWidgetItem khong lam
    duoc chuyen do.
    """

    def __init__(self, name: str, preview: str, when: str, parent=None):
        super().__init__(parent)
        self.setAttribute(Qt.WA_TranslucentBackground)

        row = QHBoxLayout(self)
        row.setContentsMargins(10, 8, 12, 8)
        row.setSpacing(11)

        row.addWidget(Avatar(name, 46))

        col = QVBoxLayout()
        col.setContentsMargins(0, 0, 0, 0)
        col.setSpacing(3)

        self.name_lbl = QLabel(name)
        self.name_lbl.setObjectName("ConvName")

        self.preview_lbl = QLabel()
        self.preview_lbl.setObjectName("ConvPreview")
        self.preview_lbl.setTextFormat(Qt.PlainText)
        self._preview_text = f"{preview}  ·  {when}"

        col.addWidget(self.name_lbl)
        col.addWidget(self.preview_lbl)
        row.addLayout(col, 1)

    def set_preview(self, preview: str, when: str) -> None:
        self._preview_text = f"{preview}  ·  {when}"
        self._elide()

    def _elide(self) -> None:
        # QLabel khong tu cat chu, va neu de nguyen thi mot tin dai se noi
        # rong ca cot trai. Nen ta tu cat theo be ngang thuc te va them "…".
        avail = max(60, self.width() - 80)
        fm = self.preview_lbl.fontMetrics()
        self.preview_lbl.setText(
            fm.elidedText(self._preview_text, Qt.ElideRight, avail))

    def resizeEvent(self, event) -> None:
        super().resizeEvent(event)
        self._elide()


class _BubbleLabel(QLabel):
    """
    Chu cua tin nhan, tu ve cai nen bo goc phia sau.

    Khong de QSS ve nen, vi bon goc phai bo khac nhau: goc quay ve phia
    tin lien truoc/lien sau cua cung mot nguoi bo nho lai, de mot chuoi
    tin doc ra thanh mot khoi thay vi bon vien tach roi.
    """

    R_BIG = 17.0
    R_SMALL = 5.0

    def __init__(self, text: str, mine: bool, first: bool, last: bool,
                 parent=None):
        super().__init__(text, parent)
        self._mine = mine
        self._first = first
        self._last = last

    def _radii(self) -> tuple[float, float, float, float]:
        """Ban kinh bon goc theo thu tu tren-trai, tren-phai, duoi-phai, duoi-trai."""
        big, small = self.R_BIG, self.R_SMALL
        tl = tr = br = bl = big
        if self._mine:
            if not self._first:
                tr = small
            if not self._last:
                br = small
        else:
            if not self._first:
                tl = small
            if not self._last:
                bl = small
        return tl, tr, br, bl

    def paintEvent(self, event) -> None:
        tl, tr, br, bl = self._radii()
        r = QRectF(self.rect()).adjusted(0.0, 0.0, -0.01, -0.01)

        path = QPainterPath()
        path.moveTo(r.left() + tl, r.top())
        path.lineTo(r.right() - tr, r.top())
        path.quadTo(r.right(), r.top(), r.right(), r.top() + tr)
        path.lineTo(r.right(), r.bottom() - br)
        path.quadTo(r.right(), r.bottom(), r.right() - br, r.bottom())
        path.lineTo(r.left() + bl, r.bottom())
        path.quadTo(r.left(), r.bottom(), r.left(), r.bottom() - bl)
        path.lineTo(r.left(), r.top() + tl)
        path.quadTo(r.left(), r.top(), r.left() + tl, r.top())
        path.closeSubpath()

        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        p.setPen(Qt.NoPen)
        p.setBrush(QColor(theme.BLUE if self._mine else theme.BUBBLE_PEER))
        p.drawPath(path)
        p.end()

        super().paintEvent(event)   # chu ve de len nen


class Bubble(QWidget):
    """
    Mot bong bong tin nhan, kem avatar nho neu la cua doi phuong.

    Do rong toi da dat theo phan tram cua khung chat chu khong phai mot so
    pixel co dinh, de khi phong to cua so bong bong khong keo dai het man
    hinh - doc mot dong qua dai rat met. Con do rong that thi bam theo be
    ngang cua chu, vi QLabel co wordWrap tu no bao mot sizeHint rat hep va
    de nguyen thi tin ngan cung bi be lam nhieu dong.
    """

    MAX_RATIO = 0.62
    PAD = 36  # padding trai+phai trong QSS, cong du hao lam tron cua font

    def __init__(self, text: str, mine: bool, sender: str = "",
                 show_avatar: bool = True, first: bool = True,
                 last: bool = True, parent=None):
        super().__init__(parent)
        self._mine = mine
        self._text = text

        row = QHBoxLayout(self)
        row.setContentsMargins(14, 0, 14, 0)
        row.setSpacing(8)

        self.label = _BubbleLabel(text, mine, first, last)
        self.label.setObjectName("BubbleMine" if mine else "BubblePeer")
        self.label.setWordWrap(True)
        self.label.setTextFormat(Qt.PlainText)
        self.label.setTextInteractionFlags(Qt.TextSelectableByMouse)
        self.label.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Preferred)

        if mine:
            row.addStretch(1)
            row.addWidget(self.label, 0, Qt.AlignRight | Qt.AlignBottom)
        else:
            if show_avatar:
                slot = Avatar(sender or "?", 28)
            else:
                # Cung mot nguoi gui lien tiep thi chi tin cuoi cung deo
                # avatar; cac tin tren no chua mot o trong cung be de le
                # trai van thang hang.
                slot = QWidget()
                slot.setFixedSize(28, 1)
            row.addWidget(slot, 0, Qt.AlignBottom)
            row.addWidget(self.label, 0, Qt.AlignLeft | Qt.AlignBottom)
            row.addStretch(1)

    def set_last(self, last: bool) -> None:
        """Danh dau lai vi tri cuoi chuoi khi co tin moi noi vao ben duoi."""
        self.label._last = last
        self.label.update()

    def _natural_width(self) -> int:
        """Be ngang neu viet tren mot dong (dong dai nhat neu co xuong dong)."""
        fm = self.label.fontMetrics()
        return max(fm.horizontalAdvance(line)
                   for line in self._text.split("\n")) + self.PAD

    def resizeEvent(self, event) -> None:
        super().resizeEvent(event)
        limit = max(160, int(self.width() * self.MAX_RATIO))
        self.label.setFixedWidth(min(self._natural_width(), limit))

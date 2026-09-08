"""
icons.py - bo icon ve bang vector.

Truoc day cho la emoji. Emoji trong ra khac nhau tren tung may, mau thi
do font quyet dinh chu khong phai giao dien, va o co nho thi nhoe. Nen ta
tu ve: net 2px, dau bo tron, mau truyen vao - lay net dong nhat voi phan
con lai cua app.

Moi icon duoc mo ta trong luoi 24x24 roi thu nho ve kich thuoc that, va
ve o do phan giai cua man hinh (devicePixelRatio) nen khong ram tren
man hinh HiDPI.

Written by Doan Phu Hai <haidoan2098@gmail.com>
"""

from PySide6.QtCore import QPointF, QRectF, Qt
from PySide6.QtGui import QColor, QIcon, QPainter, QPainterPath, QPen, QPixmap

GRID = 24.0


def _search() -> tuple[list[QPainterPath], list[QPainterPath]]:
    p = QPainterPath()
    p.addEllipse(QPointF(10.5, 10.5), 6.3, 6.3)
    p.moveTo(15.2, 15.2)
    p.lineTo(20.0, 20.0)
    return [p], []


def _video() -> tuple[list[QPainterPath], list[QPainterPath]]:
    body = QPainterPath()
    body.addRoundedRect(QRectF(2.0, 6.0, 13.5, 12.0), 3.0, 3.0)
    lens = QPainterPath()
    lens.moveTo(15.5, 10.5)
    lens.lineTo(22.0, 6.8)
    lens.lineTo(22.0, 17.2)
    lens.lineTo(15.5, 13.5)
    lens.closeSubpath()
    return [body, lens], []


def _send() -> tuple[list[QPainterPath], list[QPainterPath]]:
    # May bay giay, to dac: o co 18px mot hinh to nhin ro hon hinh vien.
    p = QPainterPath()
    p.moveTo(21.5, 12.0)
    p.lineTo(3.0, 20.5)
    p.lineTo(6.4, 12.0)
    p.lineTo(3.0, 3.5)
    p.closeSubpath()
    notch = QPainterPath()
    notch.moveTo(6.4, 12.0)
    notch.lineTo(13.5, 12.0)
    return [notch], [p]


def _mic() -> tuple[list[QPainterPath], list[QPainterPath]]:
    p = QPainterPath()
    p.addRoundedRect(QRectF(9.0, 2.5, 6.0, 11.0), 3.0, 3.0)
    arc = QPainterPath()
    arc.moveTo(5.0, 11.0)
    arc.quadTo(5.0, 18.5, 12.0, 18.5)
    arc.quadTo(19.0, 18.5, 19.0, 11.0)
    stand = QPainterPath()
    stand.moveTo(12.0, 18.5)
    stand.lineTo(12.0, 21.5)
    return [p, arc, stand], []


def _cam() -> tuple[list[QPainterPath], list[QPainterPath]]:
    return _video()


def _close() -> tuple[list[QPainterPath], list[QPainterPath]]:
    # Dau nhan cho nut ket thuc. Da thu ve ong nghe gac may, nhung o co
    # 24px mot cai ong nghe chi con la mot cai moc - khong ai doc ra. Dau
    # nhan trong vong tron do thi khong the hieu nham.
    p = QPainterPath()
    p.moveTo(6.5, 6.5)
    p.lineTo(17.5, 17.5)
    p.moveTo(17.5, 6.5)
    p.lineTo(6.5, 17.5)
    return [p], []


def _check() -> tuple[list[QPainterPath], list[QPainterPath]]:
    p = QPainterPath()
    p.moveTo(5.0, 12.6)
    p.lineTo(10.0, 17.5)
    p.lineTo(19.0, 7.0)
    return [p], []


def _compose() -> tuple[list[QPainterPath], list[QPainterPath]]:
    p = QPainterPath()
    p.moveTo(16.5, 3.5)
    p.lineTo(20.5, 7.5)
    p.lineTo(8.5, 19.5)
    p.lineTo(3.5, 20.5)
    p.lineTo(4.5, 15.5)
    p.closeSubpath()
    return [p], []


# Vai icon can net day hon hoac phai xoay. De o day thay vi nhet vao ham ve,
# de moi hinh chi lo phan hinh cua no.
_WIDTH = {"hangup": 1.15, "call": 1.15}
_ROTATE: dict[str, float] = {}


_SHAPES = {
    "search": _search,
    "call": _check,
    "video": _video,
    "send": _send,
    "mic": _mic,
    "cam": _cam,
    "hangup": _close,
    "compose": _compose,
}


def pixmap(name: str, color: str, size: int, width: float = 2.0,
           dpr: float = 1.0) -> QPixmap:
    """Ve mot icon ra QPixmap o kich thuoc va mau cho truoc."""
    strokes, fills = _SHAPES[name]()

    px = QPixmap(int(size * dpr), int(size * dpr))
    px.setDevicePixelRatio(dpr)
    px.fill(Qt.transparent)

    p = QPainter(px)
    p.setRenderHint(QPainter.Antialiasing)
    p.scale(size / GRID, size / GRID)

    angle = _ROTATE.get(name)
    if angle is not None:
        p.translate(GRID / 2, GRID / 2)
        p.rotate(angle)
        p.translate(-GRID / 2, -GRID / 2)

    width *= _WIDTH.get(name, 1.0)

    c = QColor(color)
    pen = QPen(c)
    # Net phai thu nho theo icon, khong thi icon nho se dac lai thanh mot cuc.
    pen.setWidthF(width * GRID / size)
    pen.setCapStyle(Qt.RoundCap)
    pen.setJoinStyle(Qt.RoundJoin)

    p.setPen(Qt.NoPen)
    p.setBrush(c)
    for path in fills:
        p.drawPath(path)

    p.setPen(pen)
    p.setBrush(Qt.NoBrush)
    for path in strokes:
        p.drawPath(path)
    p.end()
    return px


def icon(name: str, color: str, size: int = 20, width: float = 2.0) -> QIcon:
    """QIcon cho nut bam."""
    return QIcon(pixmap(name, color, size, width, dpr=2.0))

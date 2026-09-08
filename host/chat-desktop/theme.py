"""
theme.py - mau sac va stylesheet dung chung cho app.

Tong toi kieu Messenger: nen gan den, chu sang, bong bong bo tron han.
Gom mot cho de sau nay doi tong mau chi sua mot file, va de cac widget
khong ai tu bia mau rieng.

Written by Doan Phu Hai <haidoan2098@gmail.com>
"""

# Bang mau. Xanh chu dao la mau bong bong cua minh, phan con lai la cac
# muc xam de phan tang: nen < the < duong ke < chu phu.
BLUE = "#0a7cff"
BLUE_DARK = "#0064d2"
BG = "#181818"           # nen cot phai, cho tin nhan
SIDEBAR_BG = "#1c1c1d"   # nen cot trai
CARD = "#242526"         # thanh header, o soan tin
BUBBLE_PEER = "#303031"  # bong bong cua doi phuong
BORDER = "#2f3031"
TEXT = "#e4e6eb"
TEXT_DIM = "#a0a4a8"
SELECTED = "#2c3a4d"
HOVER = "#2a2b2c"
DANGER = "#e4384a"

# Mau avatar. Ten nao cung phai ra mot mau on dinh, nen ta bam theo tong
# byte cua ten roi lay du trong danh sach nay - cung mot nguoi luon cung
# mot mau, khong phu thuoc thu tu nap.
AVATAR_COLORS = [
    "#0a7cff", "#31a24c", "#f5a623", "#a259ff",
    "#e4384a", "#2abba7", "#ef6aa8", "#4dc4ff",
]


def avatar_color(name: str) -> str:
    """Chon mau avatar on dinh cho mot cai ten."""
    return AVATAR_COLORS[sum(name.encode()) % len(AVATAR_COLORS)]


# Stylesheet toan app. Viet o day thay vi rai setStyleSheet khap noi,
# vi rai ra thi khong con biet cai nao dang thang cai nao.
QSS = f"""
QWidget {{
    font-family: "Segoe UI", "Noto Sans", "DejaVu Sans", sans-serif;
    font-size: 14px;
    color: {TEXT};
    background: {BG};
}}

/* --- cot trai --- */
#Sidebar {{
    background: {SIDEBAR_BG};
    border-right: 1px solid {BORDER};
}}
#SidebarTitle {{
    font-size: 20px;
    font-weight: 700;
    background: transparent;
}}
#SearchBox {{
    background: {BUBBLE_PEER};
    border: none;
    border-radius: 17px;
    padding: 7px 12px;
    color: {TEXT};
    selection-background-color: {BLUE};
}}
#SearchBox:focus {{
    background: {HOVER};
}}
#ConvList {{
    background: transparent;
    border: none;
    outline: none;
}}
#ConvList::item {{
    border: none;
    padding: 0px;
    border-radius: 8px;
}}
#ConvList::item:hover {{
    background: {HOVER};
}}
#ConvList::item:selected {{
    background: {SELECTED};
}}

/* --- cot phai --- */
#ThreadHeader {{
    background: {SIDEBAR_BG};
    border-bottom: 1px solid {BORDER};
}}
#PeerName {{
    font-size: 15px;
    font-weight: 600;
    background: transparent;
}}
#PeerAddr {{
    font-size: 12px;
    color: {TEXT_DIM};
    background: transparent;
}}
#ThreadScroll {{
    background: {BG};
    border: none;
}}
#Composer {{
    background: {BG};
}}
#InputWrap {{
    background: {BUBBLE_PEER};
    border-radius: 18px;
}}
#Input {{
    background: transparent;
    border: none;
    padding: 8px 6px;
    font-size: 14px;
    color: {TEXT};
}}

/* --- nut --- */
#IconBtn {{
    background: transparent;
    border: none;
    border-radius: 17px;
}}
#IconBtn:hover {{
    background: {HOVER};
}}
#SendBtn {{
    background: transparent;
    border: none;
    border-radius: 16px;
}}
#SendBtn:hover {{
    background: {HOVER};
}}
#SendBtn:disabled {{
    color: {TEXT_DIM};
}}

/* --- ten, gio, preview trong danh sach --- */
#ConvName {{ font-weight: 600; background: transparent; }}
#ConvPreview {{ color: {TEXT_DIM}; font-size: 13px; background: transparent; }}

/* --- bong bong tin nhan --- */
#BubbleMine {{
    background: transparent;
    color: white;
    padding: 8px 13px;
}}
#BubblePeer {{
    background: transparent;
    color: {TEXT};
    padding: 8px 13px;
}}
#DayMark {{
    color: {TEXT_DIM};
    font-size: 12px;
    background: transparent;
}}
#Status {{
    color: {TEXT_DIM};
    font-size: 11px;
    background: transparent;
}}

/* --- thanh cuon mong, khong chiem cho --- */
QScrollBar:vertical {{
    background: transparent;
    width: 8px;
    margin: 0px;
}}
QScrollBar::handle:vertical {{
    background: #4a4b4c;
    border-radius: 4px;
    min-height: 30px;
}}
QScrollBar::handle:vertical:hover {{ background: #5a5b5c; }}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{ height: 0px; }}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {{ background: none; }}

/* --- man hinh cuoc goi --- */
#CallRoot {{ background: #0f0f10; }}
#CallName {{ font-size: 22px; font-weight: 600; background: transparent; }}
#CallState {{ font-size: 14px; color: {TEXT_DIM}; background: transparent; }}
#CallStage {{ background: #000000; border-radius: 12px; }}
#HangupBtn {{
    background: {DANGER};
    border: none;
    border-radius: 27px;
}}
#HangupBtn:hover {{ background: #c62b3b; }}
#AcceptBtn {{
    background: #22c55e;
    border: none;
    border-radius: 27px;
}}
#AcceptBtn:hover {{ background: #16a34a; }}
#CallCtlBtn {{
    background: {BUBBLE_PEER};
    border: none;
    border-radius: 27px;
}}
#CallCtlBtn:checked {{
    background: #e4e6eb;
}}
#CallCtlBtn:hover {{ background: {HOVER}; }}
"""

# NothanOS

**NothanOS là hệ điều hành mã nguồn mở viết từ đầu cho kiến trúc ARM, hiện đang chạy trên AM335x (ARMv7-A).**

Phát triển bởi **Vinalinux**. Toàn bộ stack — bootloader, kernel, libc, userspace, compiler — đều do nhóm tự viết. Không fork Linux, không port BSD, không nhúng upstream code. Mỗi dòng trong repo đều được tự tay gõ, hiểu rõ và chịu trách nhiệm.

---

## Mục tiêu

NothanOS được xây dựng với hai mục tiêu song hành:

**Học tập.** Cách tốt nhất để hiểu một hệ điều hành không phải là đọc sách về nó, mà là tự tay viết ra nó. Từ bootloader nạp kernel khỏi SD card, MMU dựng page table, scheduler chia thời gian CPU, driver đẩy byte qua UART, cho đến shell phân tích lệnh người dùng — mỗi lớp trong NothanOS là một câu hỏi đã được trả lời bằng code, không phải bằng tóm tắt.

**Tự chủ công nghệ.** Trong các lĩnh vực đòi hỏi kiểm soát tuyệt đối — quốc phòng, công nghiệp, hạ tầng trọng yếu — việc xây dựng sản phẩm trên một kernel hàng triệu dòng do bên thứ ba duy trì là một rủi ro chiến lược. NothanOS hướng đến vai trò một stack thay thế cho các nhóm cần sở hữu toàn diện công nghệ nền: đọc được, sửa được, audit được, và không phụ thuộc vào bất kỳ thượng nguồn nào.

---

## Kiến trúc

**Monolithic kernel.** Driver, filesystem, scheduler và memory manager cùng chia sẻ không gian địa chỉ kernel tại VA `0xC0000000`. Mô hình này hy sinh sự cô lập của microkernel để đổi lấy sự đơn giản trong thiết kế và hiệu năng truy cập trực tiếp — một sự đánh đổi hợp lý cho phần cứng nhúng nhỏ và cho mục tiêu giáo dục.

**Kế thừa triết lý Unix, nhưng không bị ràng buộc bởi lịch sử Unix.** Mô hình process, file descriptor, syscall, shell — những ý tưởng này đã được kiểm chứng qua nửa thế kỷ, và NothanOS không có lý do gì để phát minh lại chúng. Nhưng cũng có những quyết định trong Unix tồn tại đơn giản vì lý do tương thích lịch sử, không phải vì chúng là thiết kế tốt. Khi gặp những trường hợp như vậy, NothanOS sẵn sàng đi đường khác:

- `spawn` thay cho `fork`+`exec`. `fork` ra đời do giới hạn của PDP-11, không phải vì copy-on-write toàn bộ không gian địa chỉ là cách đúng để tạo process mới.
- Driver model học theo platform bus của Linux về mặt hình thức, nhưng viết lại gọn hơn — không phải gánh hàng chục năm tương thích ngược.
- Không có syscall, API hay hành vi nào được giữ lại với lý do duy nhất là "để cho khớp với chuẩn cũ".

Quy tắc: học pattern từ những kernel trưởng thành, hiểu vì sao họ làm như vậy, rồi viết từ đầu. Không copy, không port, không paraphrase.

---

## Phần cứng đích (hiện tại)

Board/SoC dưới đây là target đang dùng để dev và test — kernel không hardcode giả định riêng cho AM335x, hướng tới porting sang SoC ARM khác sau này.

| Thành phần | Thông số |
| --- | --- |
| Board | BeagleBone Black Rev.C |
| SoC | TI AM3358 (ARMv7-A Cortex-A8, 1 GHz) |
| RAM | 512 MB DDR3 @ `0x80000000` |
| Boot | microSD (MMC0) → ROM → MLO → kernel |
| Console | UART0 @ 115200 8N1 — JTAG không được sử dụng |

---

## Yêu cầu hệ thống

### Phần cứng

- **BeagleBone Black** Rev.C
- **microSD card** ≥ 128 MB
- **USB-to-Serial adapter** 3.3V TTL cho UART console — nối GND, RX, TX
- **Nguồn 5V/2A**

### Phần mềm

Host OS khuyến nghị: Ubuntu 22.04 LTS. Cài dependencies bằng script tự động:

```bash
sudo bash scripts/setup-environment.sh
```

Hoặc cài tay:

```bash
sudo apt-get install -y \
    gcc-arm-none-eabi binutils-arm-none-eabi \
    build-essential make git \
    screen minicom parted dosfstools
```

Kiểm tra toolchain:

```bash
arm-none-eabi-gcc --version
```

Để dùng serial console không cần `sudo`, thêm tài khoản vào group `dialout` (cần logout/login lại):

```bash
sudo usermod -a -G dialout $USER
```

> ⚠️ **Hard rule**: chỉ dùng `arm-none-eabi-*`. Tuyệt đối không mix với `arm-linux-gnueabihf-*` — một là toolchain bare-metal, một là toolchain Linux userspace, ABI và runtime hoàn toàn khác nhau.

---

## Build và chạy

### Bước 1: Clone

```bash
git clone https://github.com/Vinalinux-Org/Nothan-OS.git
cd Nothan-OS
```

### Bước 2: Build

```bash
make              # bootloader + userspace + kernel
make bootloader   # chỉ MLO
make kernel       # chỉ kernel
make userspace    # chỉ userspace
make clean        # xoá artifacts
```

Output:

- `bootloader/MLO` — first-stage bootloader
- `nothan-kernel/build/kernel.bin` — kernel với userspace binaries nhúng sẵn
- `userspace/build/{shell,gui,phone_daemon}.bin` — 3 process kernel spawn thẳng khi boot

### Bước 3: Flash SD card

Xác định device của SD card:

```bash
lsblk    # trước khi cắm
lsblk    # sau khi cắm — device mới xuất hiện chính là SD card (ví dụ /dev/sdb)
```

Lần đầu cần khởi tạo partition; những lần sau chỉ cần deploy:

```bash
sudo ./scripts/setup-sdcard.sh /dev/sdX       # tạo partition + filesystem (một lần)
sudo ./scripts/deploy-and-flash.sh /dev/sdX   # build + deploy rootfs + flash MLO/kernel
```

> ⚠️ Kiểm tra kỹ device trước khi chạy. `dd` ghi nhầm ổ sẽ phá huỷ dữ liệu mà không có cảnh báo.

### Bước 4: Kết nối serial console

```bash
screen /dev/ttyUSB0 115200
# hoặc
minicom -D /dev/ttyUSB0 -b 115200
```

### Bước 5: Boot BeagleBone Black

1. Cắm SD card vào BBB
2. Giữ nút **BOOT** (gần SD card slot)
3. Cắm nguồn 5V
4. Thả nút BOOT sau 2–3 giây

---

## Cấu trúc project

```text
Nothan-OS/
├── bootloader/         Stage-2 MLO: clock PLL, DDR3 init, nạp kernel từ SD
│
├── nothan-kernel/      Kernel chính (xem nothan-kernel/README.md)
│   ├── arch/arm/       Boot asm, vectors, MMU, context switch, board-bbb
│   ├── kernel/         sched, syscall, irq, time, vfs, fs (FAT32, devfs, procfs)
│   ├── mm/             buddy allocator, slab cache
│   ├── drivers/        platform bus + UART, INTC, DMTimer, MMC, GPIO, pinctrl
│   ├── init/           kernel entry (main.c)
│   └── include/nothan/ subsystem headers
│
├── userspace/          Ứng dụng user + libc tối giản
│   ├── sh/             Shell: pipe, redirect, background job, builtins
│   ├── gui/            Ứng dụng demo (LVGL) chạy trên nền OS — GUI + telephony
│   ├── phone_daemon/   Demo backend: điều khiển modem qua AT command
│   ├── sim/            Host simulator (SDL) để dev GUI không cần board thật
│   └── lib/            nothanlibc (POSIX subset) + LVGL vendored
│
├── compiler/           NothCC — trình biên dịch C tự viết bằng Python
│   └── toolchain/      frontend (lexer/parser) → IR → backend ARMv7
│
├── reference/          Tài liệu phần cứng (AM335x TRM, ARM ARM, BBB schematics)
│
├── scripts/            setup-environment, setup-sdcard, deploy-and-flash, install-compiler
│
├── CLAUDE.md           Hard rules: toolchain, driver model, debug workflow
├── Makefile            Top-level orchestration
├── LICENSE             MIT
└── README.md
```

---

## Nguyên tắc thiết kế

| Nguyên tắc | Ví dụ |
| --- | --- |
| **100% hand-written** | Không fork/port Linux, musl, BusyBox, lwIP. Mọi dòng tự viết. |
| **Linux-inspired API shape** | `task_struct`, `platform_driver`, `spinlock_t` — pattern Linux, code NothanOS |
| **Diverge khi có lý do** | `spawn` thay `fork`, driver model gọn hơn — không giữ legacy chỉ vì compatibility |
| **Simplicity over features** | `MAX_TASKS = 5`, single priority, buddy allocator đơn giản |
| **Correctness over performance** | Flush toàn bộ TLB khi switch, không nested interrupts |
| **Real hardware only** | Mọi feature phải pass trên BBB thật, không QEMU |
| **UART log là công cụ duy nhất** | Không JTAG. Driver log với prefix `[MODULE]`. Abort dump DFSR/DFAR đầy đủ. |

---

## Nguyên tắc đóng góp mã nguồn

NothanOS chào đón đóng góp từ cộng đồng. Để giữ codebase nhất quán với triết lý của dự án, mọi contribution cần tuân theo những nguyên tắc dưới đây.

**Về nguồn gốc mã.** Không copy, port, hay paraphrase code từ Linux, BSD, musl, BusyBox, hay bất kỳ upstream nào khác. Được phép học pattern từ những kernel trưởng thành, nhưng phải hiểu lý do tại sao họ làm như vậy, rồi viết lại từ đầu bằng cách diễn đạt của mình. Nếu một đoạn code có thể bị nhầm là sao chép, hãy refactor cho đến khi không còn nhầm được nữa.

**Về kiến trúc driver.** Driver phải portable — không hardcode base address, IRQ number, hay bất kỳ giá trị nào đặc thù cho BeagleBone Black. Mọi dữ liệu board-specific nằm trong `arch/arm/mach-omap2/board-bbb.c`. Mọi hardware init xảy ra trong `probe()`; không có `xxx_init()` public gọi từ `main.c`.

**Về toolchain.** Chỉ dùng `arm-none-eabi-*` cho kernel và userspace. Không mix với `arm-linux-gnueabihf-*` — ABI và runtime hoàn toàn khác nhau, mix vào sẽ gây lỗi khó truy ngược.

**Về code style.** Code tuân theo chuẩn `gnu11` (C11 với GNU extension), khớp với `-std=gnu11` trong Makefile. Indent 8 space (hard tab), brace theo K&R style, dòng không quá 100 ký tự. Đặt tên theo convention của subsystem: hàm `lower_snake_case`, struct `lower_snake_case`, macro và hằng số `UPPER_SNAKE_CASE`. Comment tập trung vào *vì sao* và những ràng buộc không lộ qua tên hàm — không lặp lại điều mà code đã nói rõ.

**Về header bản quyền.** Mọi file source (`.c`, `.S`, `.h`) phải mở đầu bằng header xác định tác giả và file path. Format chuẩn:

```c
/*
 * path/to/file.c - Mô tả ngắn về vai trò của file
 *
 * Written by <Tên đầy đủ> <email@example.com>
 */
```

Đây không chỉ là formality — header là dấu vết khẳng định mỗi dòng code do người thật viết ra, phù hợp với mục tiêu sovereignty của dự án. Khi sửa đáng kể một file đã có header người khác, thêm dòng `Modified by ...` bên dưới, không xoá tác giả gốc.

**Về commit và pull request.** Một commit giải quyết một việc. Message viết bằng tiếng Anh theo format của NothanOS: dòng tiêu đề `Type(scope): mô tả ngắn` — `Type` viết hoa chữ cái đầu (`Feat`, `Fix`, `Chore`, `Refactor`, `Docs`), `scope` là subsystem (`kernel`, `mmc`, `pinctrl`, `bootloader`, `docs`, ...). Sau dòng tiêu đề, cách một dòng trống rồi viết phần body mô tả cụ thể đã làm những gì và vì sao — đặc biệt với commit có nhiều thay đổi liên quan. Ví dụ:

```text
Feat(pinctrl): pin mux subsystem — static table, AM335x Control Module

- Thêm pinctrl-am335x driver với bảng cấu hình tĩnh
- Map Control Module và viết API pinctrl_select cho driver consumer
- Tích hợp pin mux vào probe của UART, MMC, GPIO
```

Tiêu đề nên ngắn và đủ thông tin để đọc mà không cần mở diff. Body tập trung vào *làm gì* và *vì sao* — phần *thế nào* đã có trong diff.

---

## License

MIT — xem [LICENSE](LICENSE). Phát triển bởi **Vinalinux**.

# NothCC - Trình Biên Dịch C cho NothanOS

Trình biên dịch cross-compiler hoàn chỉnh, sinh binary ARMv7-A chạy trên NothanOS.

## Tổng Quan

NothCC là compiler viết bằng Python, thực hiện toàn bộ pipeline từ source C đến ELF binary:

```
Source (.c) → Preprocessor → Lexer → Parser → Semantic → IR → CodeGen → Assembler → Linker → ELF
```

**Đặc điểm**:
- Pipeline hoàn chỉnh tự implement (không dùng GCC frontend)
- Target: ARMv7-A (BeagleBone Black)
- Tuân thủ AAPCS calling convention
- Runtime library tích hợp (reflibc)
- Output: ELF32 ARM binary

## Ngôn Ngữ Hỗ Trợ (Subset C)

**Kiểu dữ liệu**: `int`, `char`, pointer, array

**Cấu trúc điều khiển**: `if/else`, `while`, `for`, `return`, `break`, `continue`

**Hàm**: Định nghĩa, gọi hàm, đệ quy (tối đa 4 tham số qua r0-r3)

**Toán tử**: Số học, so sánh, logic, bitwise, gán

**Giới hạn**: Không hỗ trợ `const`, `unsigned`, `void` parameter, variadic, `struct/union/enum`, floating-point

## Cài Đặt

### Yêu Cầu Hệ Thống

Ubuntu 22.04 LTS (khuyến nghị)

```bash
# Cài system dependencies
sudo apt-get update
sudo apt-get install python3 python3-pip python3-venv
sudo apt-get install binutils-arm-linux-gnueabihf

# Hoặc dùng script tự động từ root project (khuyến nghị)
sudo bash scripts/setup-environment.sh
```

### Cài Đặt NothCC

```bash
# Từ thư mục gốc project
bash scripts/install_compiler.sh
```

Script sẽ:
- Copy toolchain vào `~/.local/lib/nothcc`
- Tạo wrapper `~/.local/bin/nothcc`
- Thêm vào PATH

### Python Dependencies (Development/Testing)

Nếu muốn chạy test suite hoặc develop compiler:

```bash
# Từ thư mục compiler/
pip3 install -r requirements.txt
```

Bao gồm: `pytest`, `pytest-cov`, `hypothesis`, `black`, `mypy`.

> **Lưu ý**: Các package này dành cho phát triển compiler, **không** cần thiết để chỉ dùng `nothcc` compile chương trình.

### Kiểm Tra

```bash
nothcc --version
# nothcc 0.1.0
```

## Sử Dụng

### Biên Dịch Cơ Bản

```bash
# Compile ra executable
nothcc program.c -o program

# Chỉ sinh assembly
nothcc program.c -S -o program.s

# Compile ra object file
nothcc program.c -c -o program.o
```

### Debug

```bash
# Dump tokens
nothcc program.c --dump-tokens

# Dump AST
nothcc program.c --dump-ast

# Dump IR
nothcc program.c --dump-ir

# Verbose output
nothcc program.c -v
```

### Tùy Chọn Nâng Cao

```bash
# Optimization level
nothcc program.c -O1 -o program

# Giữ file trung gian
nothcc program.c --keep-temps -o program

# Custom linker script
nothcc program.c -T custom.ld -o program
```

## Runtime Library (reflibc)

### System Calls

```c
int  write(int fd, char* buf, int count);
int  read(int fd, char* buf, int count);
void exit(int status);
int  yield();
```

### I/O Functions

```c
int  strlen(char* s);
int  putchar(int c);
int  puts(char* str);
void print_str(char* str);
void print_int(int val);
void print_hex(int val);
```

## Ví Dụ

### Hello World

```c
#include "reflibc.h"

int main() {
    print_str("Hello, NothanOS!\n");
    return 0;
}
```

Compile:
```bash
nothcc hello.c -o hello
```

### Fibonacci

```c
#include "reflibc.h"

int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main() {
    int i;
    for (i = 0; i < 10; i = i + 1) {
        print_int(fib(i));
        print_str("\n");
    }
    return 0;
}
```

## Chạy Trên NothanOS

### Bước 1: Compile

```bash
nothcc program.c -o program
```

### Bước 2: Nhúng Vào Kernel

```bash
cp program nothan-kernel/src/kernel/files/
cd nothan-kernel
make
```

### Bước 3: Flash Lên SD Card

```bash
cd ../..
bash scripts/flash_sdcard.sh /dev/sdX
```

### Bước 4: Chạy Trên Board

```
$ ls
program  hello.txt  info.txt

$ exec program
Hello, NothanOS!
```

## Cấu Trúc Project

```
compiler/
├── toolchain/              # Source code compiler
│   ├── main.py             # Entry point
│   ├── frontend/           # Lexer, Parser, Semantic
│   ├── middleend/          # IR generation
│   ├── backend/armv7a/     # Code generator
│   └── runtime/            # reflibc
├── tests/                  # Test suite
├── docs/                   # Tài liệu kỹ thuật
└── nothcc.spec              # PyInstaller spec
```

## Testing

```bash
# Chạy test suite
make test

# Test cụ thể
python3 -m pytest tests/test_integration.py -v
```

## Tài Liệu

Tài liệu kỹ thuật chi tiết trong `docs/`:

- `architecture.md` - Kiến trúc compiler
- `subset_c_spec.md` - Đặc tả ngôn ngữ
- `ir_format.md` - Format IR
- `codegen_strategy.md` - Chiến lược sinh mã
- `usage_guide.md` - Hướng dẫn chi tiết

## Troubleshooting

**Import Error**:
```bash
bash scripts/install_compiler.sh
```

**Assembler not found**:
```bash
sudo apt-get install binutils-arm-linux-gnueabihf
```

**Binary không chạy trên NothanOS**:
- Kiểm tra: `file program` (phải là ELF32 ARM)
- Đảm bảo `#include "reflibc.h"`
- Compile với `-v` để xem linking

## Giới Hạn

- Chỉ compile single file (không hỗ trợ separate compilation)
- Không có preprocessor macro phức tạp
- Không có `struct/union/enum`
- Không có floating-point
- Không có dynamic memory (`malloc/free`)
- Chỉ target ARMv7-A

Các giới hạn này là by design để giữ compiler đơn giản và tập trung vào mục tiêu giáo dục.

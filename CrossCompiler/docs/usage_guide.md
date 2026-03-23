# VinCC Compiler — Usage Guide

> **Phạm vi:** Hướng dẫn install, basic usage, compiler options, debug flags, example programs, và deploy lên VinixOS.
> **Yêu cầu trước:** [architecture.md](architecture.md) — hiểu pipeline compiler.
> **Files liên quan:** `toolchain/main.py`, `scripts/install_compiler.sh`

---

## Quick Start (3 bước)

```bash
# 1. Install toolchain
sudo apt-get install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf

# 2. Build runtime library
cd CrossCompiler && make runtime

# 3. Compile và chạy
python3 -m toolchain.main -o hello hello.c
```

---

## Installation

### Prerequisites

| Requirement | Version | Install |
|-------------|---------|---------|
| Linux x86_64 | Ubuntu/Debian | — |
| Python | 3.8+ | `sudo apt install python3` |
| ARM toolchain | any | `sudo apt install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf` |

**Verify ARM toolchain:**
```bash
arm-linux-gnueabihf-as --version
arm-linux-gnueabihf-ld --version
```

### Setup

```bash
# Clone repo
git clone <repository-url>
cd VinixOS

# Install Python dependencies
cd CrossCompiler
pip3 install -r requirements.txt

# Build runtime library (crt0.S, syscalls.S, divmod.S)
make runtime
```

---

## Compiler Options

| Option | Mô Tả | Example |
|--------|-------|---------|
| `-o <file>` | Output file path | `-o myprogram` |
| `-S` | Emit assembly only, không assemble/link | `-S -o out.s` |
| `--dump-tokens` | In token stream sau lexing | Debug lexer |
| `--dump-ast` | In AST sau parsing | Debug parser |
| `--dump-ir` | In IR sau IR generation | Debug codegen |
| `--help` | Show help | — |
| `--version` | Show version | — |

**Usage pattern:**
```bash
python3 -m toolchain.main [options] <source_file>
```

---

## Debug Options

### `--dump-tokens`

```bash
python3 -m toolchain.main --dump-tokens hello.c
```

Output:
```
=== TOKENS ===
Token(type=INT,        value='int',  line=1, col=1)
Token(type=IDENTIFIER, value='main', line=1, col=5)
Token(type=LPAREN,     value='(',    line=1, col=9)
...
```

### `--dump-ast`

```bash
python3 -m toolchain.main --dump-ast hello.c
```

Output:
```
=== AST ===
Program(
  declarations=[
    FunctionDecl(name='main', return_type='int',
      body=CompoundStmt([...]))
  ]
)
```

### `--dump-ir`

```bash
python3 -m toolchain.main --dump-ir hello.c
```

Output:
```
=== IR ===
function_entry main
t0 = 5 add 3
x = t0
return 0
function_exit main
```

### Emit Assembly Only (`-S`)

```bash
python3 -m toolchain.main -S -o hello.s hello.c
cat hello.s   # Inspect generated ARM assembly
```

---

## Example Programs

### Hello World

```c
/* hello.c */
int write(int fd, char* buf, int count);
void exit(int status);

int main() {
    char msg[] = "Hello, VinixOS!\n";
    write(1, msg, 16);
    exit(0);
}
```

```bash
python3 -m toolchain.main -o hello hello.c
```

### Factorial (Recursion)

```c
/* factorial.c */
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main() {
    int result = factorial(5);  /* result = 120 */
    return 0;
}
```

```bash
python3 -m toolchain.main -o factorial factorial.c
```

### Array Operations

```c
/* array.c */
int sum_array(int* arr, int size) {
    int sum = 0;
    int i;
    for (i = 0; i < size; i = i + 1) {
        sum = sum + arr[i];
    }
    return sum;
}

int main() {
    int data[5];
    data[0] = 1; data[1] = 2; data[2] = 3;
    data[3] = 4; data[4] = 5;
    int total = sum_array(data, 5);  /* total = 15 */
    return total;
}
```

### String Output via UART

```c
/* shell_hello.c — chạy trên VinixOS */
int write(int fd, char* buf, int count);
void yield(void);
void exit(int status);

int strlen(char* s) {
    int n = 0;
    while (s[n] != '\0') n = n + 1;
    return n;
}

int main() {
    char* msg = "Hello from VinCC!\n";
    write(1, msg, strlen(msg));
    exit(0);
    return 0;
}
```

---

## Deploy lên VinixOS

### Automatic (via kernel embed)

```bash
# 1. Compile chương trình
python3 -m toolchain.main -o myapp myapp.c

# 2. Copy binary vào initfs (sẽ có trong RAMFS)
cp myapp VinixOS/initfs/myapp

# 3. Build lại kernel (embed file mới)
make -C VinixOS kernel

# 4. Flash lên SD card
bash scripts/flash_sdcard.sh /dev/sdX
```

### Manual Deploy

```bash
# Deploy qua serial/SSH nếu VinixOS đang chạy
scp myapp user@beaglebone:/
```

---

## Error Messages

### Lexer Errors

```
hello.c:5:3: lexer error: invalid character '@'
```

### Parser Errors

```
hello.c:10:1: parser error: expected '}' before end of file
hello.c:7:5: parser error: unexpected token 'else' (expected expression)
```

### Semantic Errors

```
hello.c:3:5: semantic error: undefined variable 'x'
hello.c:8:3: semantic error: type mismatch: expected int, got char*
hello.c:2:1: semantic error: duplicate declaration of 'foo'
```

---

## Build System Integration

### Makefile

```makefile
VINCC = python3 -m toolchain.main
CFLAGS =

SRCS = main.c utils.c
TARGET = myapp

$(TARGET): $(SRCS)
	$(VINCC) $(CFLAGS) -o $@ $<

debug:
	$(VINCC) --dump-ir --dump-ast -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) *.s *.o
```

---

## Limitations Cần Biết

| Feature | Status | Ghi Chú |
|---------|--------|---------|
| `++`/`--` operators | Không hỗ trợ | Dùng `i = i + 1` |
| `struct`/`union` | Không hỗ trợ | Chỉ basic types |
| Multi-dimensional arrays | Không hỗ trợ | Dùng 1D + manual indexing |
| `typedef` | Không hỗ trợ | — |
| Hex/octal literals | Không hỗ trợ | Chỉ decimal |
| `printf` | Không có | Dùng `write()` syscall |
| Standard library | Không có | Dùng VinixOS syscalls |
| Function pointers | Không hỗ trợ | — |
| Variadic functions | Không hỗ trợ | — |

---

## Tóm Tắt

| Concept | Ý Nghĩa |
|---------|---------|
| `python3 -m toolchain.main` | Entry point — không cần install, chạy trực tiếp |
| Runtime library | Link tự động — `crt0.S`, `syscalls.S`, `divmod.S` |
| Base address `0x40000000` | Output ELF khớp với VinixOS user space |
| Software division | Mọi `/` và `%` đều dùng `__aeabi_idiv` |
| `write(1, buf, len)` | Standard output qua UART syscall |
| No stdlib | Phải implement hoặc dùng VinixOS syscalls |

---

## Xem Thêm

- [architecture.md](architecture.md) — Pipeline chi tiết
- [subset_c_spec.md](subset_c_spec.md) — Ngôn ngữ được support
- [VinixOS/docs/06-syscall-mechanism.md](../../VinixOS/docs/06-syscall-mechanism.md) — Syscall ABI

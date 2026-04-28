# Subset C — Language Specification

> **Phạm vi:** Định nghĩa đầy đủ ngôn ngữ Subset C được VinCC compiler hỗ trợ — lexical elements, types, declarations, statements, expressions, và limitations.
> **Yêu cầu trước:** [architecture.md](architecture.md) — hiểu compiler pipeline.
> **Files liên quan:** `toolchain/frontend/lexer/token.py`, `toolchain/frontend/parser/ast_nodes.py`, `toolchain/frontend/semantic/type_checker.py`

---

## Tổng Quan

Subset C là một proper subset của C99, bao gồm:
- Basic types: `int`, `char`, `void`
- Pointers và 1D arrays
- Control flow: `if/else`, `while`, `for`, `return`
- Functions với tối đa 4 parameters
- VinixOS syscall interface

> **Không support:** `struct`, `union`, `typedef`, `enum`, `switch`, `goto`, `do-while`, `++/--`, `+=/-=`, hex literals, multi-dimensional arrays, function pointers, variadic functions.

---

## Lexical Elements

### Keywords

```
int   char   void   if   else   while   for   return
```

### Identifiers

- Bắt đầu bằng letter (`a-z`, `A-Z`) hoặc underscore `_`
- Theo sau bởi letters, digits (`0-9`), hoặc underscores
- Case-sensitive

```c
x          counter       _temp        myVariable
MAX_SIZE   i             result123
```

### Literals

**Integer literals** — decimal only:

| Type | Range | Examples |
|------|-------|---------|
| Decimal int | -2147483648 to 2147483647 | `0`, `42`, `1000` |

> Không support: hex (`0xFF`), octal (`0777`), binary (`0b101`).

**Character literals:**

| Escape | Value |
|--------|-------|
| `'a'` – `'z'`, `'A'` – `'Z'`, `'0'` – `'9'` | ASCII value |
| `'\n'` | Newline (10) |
| `'\t'` | Tab (9) |
| `'\r'` | Carriage return (13) |
| `'\\'` | Backslash (92) |
| `'\''` | Single quote (39) |
| `'\"'` | Double quote (34) |

### Operators

**Arithmetic:**

| Op | Operation | Precedence |
|----|-----------|-----------|
| `*` | Multiply | 5 |
| `/` | Divide (software) | 5 |
| `%` | Modulo (software) | 5 |
| `+` | Add | 4 |
| `-` | Subtract | 4 |

**Comparison:**

| Op | Meaning |
|----|---------|
| `==` | Equal |
| `!=` | Not equal |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less or equal |
| `>=` | Greater or equal |

**Logical:**

| Op | Meaning |
|----|---------|
| `&&` | Logical AND (short-circuit) |
| `\|\|` | Logical OR (short-circuit) |
| `!` | Logical NOT |

**Bitwise:**

| Op | Meaning |
|----|---------|
| `&` | Bitwise AND |
| `\|` | Bitwise OR |
| `^` | Bitwise XOR |
| `<<` | Left shift |
| `>>` | Right shift (arithmetic) |

**Other:**

| Op | Meaning |
|----|---------|
| `=` | Assignment |
| `*` | Dereference (unary) |
| `&` | Address-of (unary) |
| `-` | Negation (unary) |
| `!` | Logical NOT (unary) |
| `[]` | Array subscript |

### Comments

```c
// Single-line comment

/* Multi-line
   comment */
```

---

## Data Types

| Type | Size | Range / Notes |
|------|------|--------------|
| `int` | 32-bit signed | -2,147,483,648 to 2,147,483,647 |
| `char` | 8-bit | Character; used for strings |
| `void` | — | Return type only; không declare variable |
| `int*` | 32-bit pointer | Pointer to int |
| `char*` | 32-bit pointer | Pointer to char (string) |
| `int[N]` | N × 4 bytes | 1D array; decays to `int*` |
| `char[N]` | N × 1 byte | 1D array; decays to `char*` |

---

## Declarations

### Variables

```c
int x;              /* uninitialized */
int y = 5;          /* initialized   */
char c = 'a';
int* p;             /* pointer       */
int arr[10];        /* array of 10   */
char str[64];
```

### Functions

```c
/* Definition */
int add(int a, int b) {
    return a + b;
}

void print_n(int n) {
    /* no return value */
}

/* Forward declaration (prototype) */
int multiply(int x, int y);
```

**Function constraints:**

| Constraint | Value |
|-----------|-------|
| Max parameters | 4 |
| Argument passing | AAPCS: r0-r3 (first 4) |
| Return value | r0 (int/char/pointer) |
| Recursion | Supported |
| Nested functions | Not supported |

---

## Statements

### `if` / `if-else`

```c
if (condition) {
    /* then branch */
}

if (x > 0) {
    positive();
} else {
    non_positive();
}
```

### `while`

```c
while (condition) {
    /* body */
}

int i = 0;
while (i < 10) {
    i = i + 1;    /* Note: i++ not supported */
}
```

### `for`

```c
for (init; condition; update) {
    /* body */
}

int i;
for (i = 0; i < 10; i = i + 1) {
    arr[i] = i * 2;
}
```

### `return`

```c
return;           /* void function  */
return 0;         /* int function   */
return x + y;     /* expression     */
```

### Compound Statement (Block)

```c
{
    int local_var = 5;  /* Block-scoped */
    local_var = local_var + 1;
}
/* local_var không tồn tại ở đây */
```

---

## Expressions

### Operator Precedence (cao → thấp)

| Level | Operators | Associativity |
|-------|-----------|--------------|
| 7 | `!` `-` `*` `&` (unary) | Right |
| 6 | `[]` (subscript) | Left |
| 5 | `*` `/` `%` | Left |
| 4 | `+` `-` | Left |
| 3 | `<<` `>>` | Left |
| 2 | `<` `>` `<=` `>=` `==` `!=` | Left |
| 1 | `&` `^` `\|` `&&` `\|\|` | Left |
| 0 | `=` | Right |

### Pointer Operations

```c
int x = 5;
int* p = &x;     /* address-of: p points to x */
*p = 10;         /* dereference: x is now 10  */
int y = *p;      /* y = 10                    */
```

### Array Operations

```c
int arr[5];
arr[0] = 42;         /* indexed write    */
int val = arr[2];    /* indexed read     */

int* ptr = arr;      /* array decays to pointer */
ptr[1] = 99;         /* pointer indexing        */
```

### Function Calls

```c
int result = add(3, 4);
print_msg("hello");
factorial(n - 1);    /* Recursive call */
```

---

## Type System

### Implicit Conversions

| From | To | Notes |
|------|----|-------|
| `char` | `int` | Zero-extended |
| `int` | `char` | Truncated to 8-bit |
| `int[N]` | `int*` | Array decay |
| `char[N]` | `char*` | Array decay |

### Type Compatibility Rules

- Assignment: both sides must be compatible
- Comparison: both operands must be same base type
- Arithmetic: both operands must be `int` or `char`
- Function call: argument types phải match parameter types

---

## Scoping Rules

- **File scope:** Function declarations và global variables
- **Block scope:** Variables declared trong `{ }` — local to that block
- **Nested scopes:** Inner scope có thể shadow outer scope variable
- **No implicit globals:** Phải declare trước khi dùng

```c
int global_x = 10;    /* File scope */

int foo(int a) {
    int local_y = a;  /* Block scope — foo only */
    {
        int z = 5;    /* Nested block scope */
    }
    /* z không accessible ở đây */
    return local_y;
}
```

---

## VinixOS Syscall Interface

Syscalls được gọi qua function declarations — compiler biết là syscall:

```c
/* Declare syscalls như function prototypes */
int write(int fd, char* buf, int count);
int read(int fd, char* buf, int count);
void exit(int status);
void yield(void);
int open(char* path, int flags);
int read_file(int fd, char* buf, int count);
int close(int fd);

/* Usage */
int main() {
    char msg[] = "Hello!\n";
    write(1, msg, 7);   /* fd=1: stdout (UART) */
    exit(0);
}
```

**File descriptor conventions:**

| FD | Meaning |
|----|---------|
| 0 | stdin (UART RX) |
| 1 | stdout (UART TX) |
| 2 | stderr (UART TX) |
| 3+ | File handles từ `open()` |

---

## Limitations

| Category | Limitation |
|----------|-----------|
| Operators | Không có `++`, `--`, `+=`, `-=`, `*=`, `/=`, `?:` |
| Types | Không có `struct`, `union`, `enum`, `typedef` |
| Arrays | Chỉ 1D; không có multi-dimensional |
| Literals | Chỉ decimal; không có hex/octal/float |
| Functions | Tối đa 4 parameters; không có variadic |
| Pointers | Không có function pointers; không có `void*` |
| Standard Library | Không có `printf`, `malloc`, `free`, `string.h` |
| Preprocessor | Không có `#include`, `#define`, `#ifdef` |

---

## Tóm Tắt

| Concept | Ý Nghĩa |
|---------|---------|
| 8 keywords | `int char void if else while for return` — minimal but sufficient |
| 3 base types | `int` (32-bit), `char` (8-bit), `void` |
| Max 4 params | AAPCS r0-r3 cho arguments |
| No stdlib | Dùng VinixOS syscalls cho I/O |
| Decimal only | Không có hex/octal literals |
| Software div | `/` và `%` → `__aeabi_idiv` (chậm hơn native) |
| Array decay | `arr[]` → `arr*` khi pass đến function |

---

## Xem Thêm

- [usage_guide.md](usage_guide.md) — Cách compile và ví dụ cụ thể
- [architecture.md](architecture.md) — Compiler handles types/scopes như thế nào
- [ir_format.md](ir_format.md) — Subset C được lowered xuống IR như thế nào

# Kiến Trúc Compiler (VinCC)

> **Phạm vi:** Pipeline tổng thể từ source code đến ELF binary — 7 phases, module organization, và design decisions.
> **Yêu cầu trước:** Không có — đây là tài liệu entry point cho CrossCompiler.
> **Files liên quan:** `toolchain/main.py`, `toolchain/frontend/`, `toolchain/middleend/`, `toolchain/backend/`, `toolchain/runtime/`

---

## Pipeline Tổng Thể

```
Source Code (.c)
    ↓
[Lexer]              → Token stream
    ↓
[Parser]             → Abstract Syntax Tree (AST)
    ↓
[Semantic Analyzer]  → Annotated AST + Symbol Table
    ↓
[IR Generator]       → Three-Address Code (IR)
    ↓
[Code Generator]     → ARM Assembly (.s)
    ↓
[Assembler]          → Object File (.o)  — invokes arm-linux-gnueabihf-as
    ↓
[Linker]             → ELF Executable   — invokes arm-linux-gnueabihf-ld
```

### Quick Reference — Tất Cả Phases

| Phase | Module | Input | Output | Ghi Chú |
|-------|--------|-------|--------|---------|
| Lexer | `frontend.lexer` | Source text | Token list | State machine, track line/col |
| Parser | `frontend.parser` | Token list | AST | Recursive descent, operator precedence |
| Semantic | `frontend.semantic` | AST | Annotated AST + Symbol Table | Type check, scope resolution |
| IR Gen | `middleend.ir` | Annotated AST | IR instructions | Three-Address Code (3AC) |
| Code Gen | `backend.armv7a` | IR instructions | ARM Assembly (.s) | Register allocation, AAPCS |
| Assembler | `backend.armv7a.assembler` | .s file | .o file | Wraps `arm-linux-gnueabihf-as` |
| Linker | `backend.armv7a.linker` | .o files | ELF binary | Wraps `arm-linux-gnueabihf-ld` |

---

## Các Compiler Phases Chi Tiết

### 1. Lexer (Phân Tích Từ Vựng)

**Module:** `compiler.frontend.lexer` — `lexer.py`, `token.py`

**Chức năng:** Chuyển source code text thành stream of tokens.

**Xử lý:**
- State machine nhận diện keywords, identifiers, literals, operators
- Track line và column numbers cho error reporting
- Bỏ qua whitespace và comments (`//` và `/* */`)
- Detect lexical errors (invalid characters, unterminated literals)

**Token Types:**

| Category | Examples |
|----------|---------|
| Keywords | `int char void if else while for return` |
| Identifiers | `counter`, `_temp`, `myFunction` |
| Literals | `42`, `'a'`, `'\n'` |
| Operators | `+ - * / % == != < > && \|\| & \| ^ << >>` |
| Delimiters | `( ) { } [ ] ; ,` |

### 2. Parser (Phân Tích Cú Pháp)

**Module:** `compiler.frontend.parser` — `parser.py`, `ast_nodes.py`

**Chức năng:** Chuyển token stream thành Abstract Syntax Tree.

**Xử lý:**
- Recursive descent parsing với operator precedence climbing
- Build AST nodes cho declarations, statements, expressions
- Validate syntax theo Subset C grammar
- Panic mode recovery để collect nhiều syntax errors

**AST Node Types:**

| Category | Node Types |
|----------|-----------|
| Program | `Program` (root) |
| Declarations | `FunctionDecl`, `VarDecl` |
| Statements | `CompoundStmt`, `IfStmt`, `WhileStmt`, `ForStmt`, `ReturnStmt`, `ExprStmt` |
| Expressions | `BinaryOp`, `UnaryOp`, `Assignment`, `FunctionCall`, `ArrayAccess`, `Identifier`, `IntLiteral`, `CharLiteral` |

### 3. Semantic Analyzer (Phân Tích Ngữ Nghĩa)

**Module:** `compiler.frontend.semantic` — `semantic_analyzer.py`, `symbol_table.py`, `type_checker.py`

**Chức năng:** Validate semantic correctness và build symbol table.

**Xử lý:**
- Traverse AST và build symbol table với scope management
- Verify tất cả variables/functions được declare trước khi dùng
- Check type compatibility trong assignments, function calls, returns
- Enforce lexical scoping — detect duplicate declarations

**Symbol Table:** Scope stack → Symbol entries với `{name, type, scope_level, offset}`

**Type System:**

| Type | Size | Notes |
|------|------|-------|
| `int` | 32-bit | Default integer type |
| `char` | 8-bit | Character type |
| `void` | — | Return type only |
| `int*`, `char*` | 32-bit | Pointer types |
| `int[]`, `char[]` | — | Treated as pointer |

### 4. IR Generator (Sinh Intermediate Representation)

**Module:** `compiler.middleend.ir` — `ir_generator.py`, `ir_instructions.py`, `generators.py`

**Chức năng:** Chuyển Annotated AST thành Three-Address Code (3AC).

**Xử lý:**
- Traverse AST → sinh IR instructions
- Generate temporaries (`t0`, `t1`, ...) cho intermediate values
- Generate labels (`L0`, `L1`, ...) cho control flow
- Flatten expressions thành 3-operand format
- Generate function entry/exit sequences

**IR Instruction Categories:**

| Category | Types |
|----------|-------|
| Arithmetic | `add`, `sub`, `mul`, `div`, `mod` |
| Logical/Bitwise | `and`, `or`, `xor`, `shl`, `shr` |
| Comparison | `eq`, `ne`, `lt`, `gt`, `le`, `ge` |
| Unary | `neg`, `not`, `deref`, `addr` |
| Memory | `load`, `store`, `assign` |
| Control Flow | `label`, `goto`, `if_goto` |
| Function | `param`, `call`, `return`, `func_entry`, `func_exit` |

### 5. Code Generator (Sinh Mã ARMv7-A)

**Module:** `compiler.backend.armv7a` — `code_generator.py`, `register_allocator.py`, `syscall_support.py`

**Chức năng:** Chuyển IR thành ARM assembly text.

**Register Layout:**

| Registers | Role | Save/Restore |
|-----------|------|-------------|
| `r0–r3` | Arguments + scratch | Caller-saved (không cần save) |
| `r4–r11` | Temporaries + local vars | Callee-saved (prologue/epilogue) |
| `r12` (ip) | Intra-procedure scratch | Caller-saved |
| `r13` (sp) | Stack pointer | — |
| `r14` (lr) | Link register (return address) | Saved trong prologue |
| `r15` (pc) | Program counter | — |

**Instruction Selection:**

| Operation | ARM Instruction(s) |
|-----------|-------------------|
| Add/Sub/Mul | `add`, `sub`, `mul` |
| Division | `bl __aeabi_idiv` (software — Cortex-A8 không có SDIV) |
| Logical | `and`, `orr`, `eor` |
| Shift | `lsl`, `asr` |
| Compare + branch | `cmp` + `beq/bne/bgt/blt/bge/ble` |
| Memory | `ldr`, `str` với offsets |
| Syscall | `mov r7, #num` + `svc #0` |

**AAPCS Function Frame:**

```asm
; Prologue
push    {r4-r11, lr}      ; Save callee-saved regs + return addr
sub     sp, sp, #N        ; Allocate local variable space

; ... body ...

; Epilogue
add     sp, sp, #N        ; Deallocate locals
pop     {r4-r11, pc}      ; Restore regs + return (pc = lr)
```

### 6. Assembler

**Module:** `compiler.backend.armv7a.assembler`

**Chức năng:** Invoke `arm-linux-gnueabihf-as` để assemble `.s` → `.o`.

```bash
arm-linux-gnueabihf-as -mcpu=cortex-a8 -o output.o input.s
```

### 7. Linker

**Module:** `compiler.backend.armv7a.linker`

**Chức năng:** Invoke `arm-linux-gnueabihf-ld` để link `.o` files → ELF binary.

**Runtime Library được link tự động:**

| File | Chức Năng |
|------|----------|
| `crt0.S` | C runtime startup — setup stack, call `main()`, `sys_exit()` |
| `syscalls.S` | Syscall wrappers — `write`, `read`, `exit`, `yield` |
| `divmod.S` | Software division — `__aeabi_idiv`, `__aeabi_idivmod` |
| `app.ld` | Linker script — base `0x40000000`, user space layout |

**Memory Layout của output binary:**

```
0x40000000   .text   (code)
             .rodata (read-only data)
             .data   (initialized globals)
             .bss    (zero-initialized globals)
0x40100000   Stack (16KB, grows down)
```

---

## Module Organization

```
toolchain/
├── main.py                         ← Compiler driver (entry point)
├── common/
│   ├── error.py                    ← ErrorCollector, error formatting
│   └── config.py                   ← CompilerConfig dataclass
├── frontend/
│   ├── lexer/
│   │   ├── token.py                ← Token types, Token class
│   │   └── lexer.py                ← Lexer implementation
│   ├── parser/
│   │   ├── ast_nodes.py            ← AST node definitions
│   │   └── parser.py               ← Recursive descent parser
│   └── semantic/
│       ├── symbol_table.py         ← Symbol table, scope management
│       ├── type_checker.py         ← Type compatibility rules
│       └── semantic_analyzer.py    ← AST traversal + validation
├── middleend/
│   └── ir/
│       ├── ir_instructions.py      ← IR instruction class definitions
│       ├── generators.py           ← Temp/label name generators
│       └── ir_generator.py         ← AST → IR translation
├── backend/
│   └── armv7a/
│       ├── register_allocator.py   ← Linear scan + spilling
│       ├── code_generator.py       ← IR → ARM assembly
│       ├── syscall_support.py      ← Syscall code generation
│       ├── assembler.py            ← Wrapper for arm-linux-gnueabihf-as
│       └── linker.py               ← Wrapper for arm-linux-gnueabihf-ld
└── runtime/
    ├── crt0.S
    ├── syscalls.S
    ├── divmod.S
    └── app.ld
```

---

## Error Handling

**Error format:**
```
<filename>:<line>:<column>: <phase> error: <message>
```

**Approach:** `ErrorCollector` thu thập errors từ tất cả phases — tiếp tục compilation để collect nhiều errors cùng lúc, report all errors at the end.

**Exit codes:**

| Code | Ý Nghĩa |
|------|---------|
| 0 | Compilation success |
| 1 | One or more errors |

---

## Design Decisions

| Decision | Rationale | Alternative Considered |
|----------|-----------|----------------------|
| Python implementation | Rapid development, rich stdlib, easy testing | C/C++ — rejected: slower to develop |
| Three-Address Code IR | Simple, uniform, easy assembly mapping | SSA form — rejected: overkill for this scope |
| Software division | Cortex-A8 không có hardware SDIV | `__aeabi_idiv` từ ARM EABI runtime |
| Linear scan register allocation | Simple, predictable | Graph coloring — rejected: complex, unnecessary |
| AAPCS calling convention | Compatible với C libs + ARM EABI | Custom ABI — rejected: interop issues |
| Base address `0x40000000` | VinixOS user space layout | — |

---

## Performance Characteristics

| Phase | Complexity |
|-------|-----------|
| Lexer | O(n) — source length |
| Parser | O(n) — token count |
| Semantic | O(n) — AST nodes |
| IR Generation | O(n) — AST nodes |
| Code Generation | O(m) — IR instructions |

**Generated code quality:** Equivalent to GCC `-O0` — no optimization, focus on correctness.

---

## Tóm Tắt

| Concept | Ý Nghĩa |
|---------|---------|
| 7-phase pipeline | Source → Tokens → AST → Annotated AST → IR → Assembly → .o → ELF |
| Python implementation | Rapid development, không cần compile compiler |
| Three-Address Code | Mỗi IR instruction ≤ 3 operands — dễ map sang assembly |
| AAPCS compliance | Compatible với ARM toolchain và C libraries |
| Software division | Cortex-A8 không có SDIV — dùng `__aeabi_idiv` |
| Base `0x40000000` | Khớp với VinixOS user space VA |

---

## Xem Thêm

- [usage_guide.md](usage_guide.md) — Cách install và sử dụng compiler
- [subset_c_spec.md](subset_c_spec.md) — Ngôn ngữ Subset C được support
- [ir_format.md](ir_format.md) — Chi tiết IR instruction format
- [codegen_strategy.md](codegen_strategy.md) — Register allocation và instruction selection

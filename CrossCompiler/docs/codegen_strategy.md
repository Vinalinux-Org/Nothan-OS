# Code Generation Strategy

> **Phạm vi:** IR → ARM assembly — register allocation, AAPCS calling convention, instruction selection, stack frame layout, và syscall generation.
> **Yêu cầu trước:** [ir_format.md](ir_format.md) — hiểu IR input; [architecture.md](architecture.md) — codegen là backend phase.
> **Files liên quan:** `toolchain/backend/armv7a/code_generator.py`, `toolchain/backend/armv7a/register_allocator.py`, `toolchain/backend/armv7a/syscall_support.py`

---

## Register Layout

### ARM Register Roles

| Register | ABI Name | Role | Save/Restore |
|----------|----------|------|-------------|
| `r0` | `a1` | Argument 1 / Return value | Caller-saved |
| `r1` | `a2` | Argument 2 | Caller-saved |
| `r2` | `a3` | Argument 3 | Caller-saved |
| `r3` | `a4` | Argument 4 | Caller-saved |
| `r4` | `v1` | Local / temporary | **Callee-saved** |
| `r5` | `v2` | Local / temporary | **Callee-saved** |
| `r6` | `v3` | Local / temporary | **Callee-saved** |
| `r7` | `v4` | Local / temporary / **syscall#** | **Callee-saved** |
| `r8` | `v5` | Local / temporary | **Callee-saved** |
| `r9` | `v6` | Local / temporary | **Callee-saved** |
| `r10` | `v7` | Local / temporary | **Callee-saved** |
| `r11` | `fp` | Frame pointer (optional) | **Callee-saved** |
| `r12` | `ip` | Intra-procedure scratch | Caller-saved |
| `r13` | `sp` | Stack pointer | — |
| `r14` | `lr` | Link register (return address) | Saved in prologue |
| `r15` | `pc` | Program counter | — |

> **Temporaries pool:** `r4–r11` (8 registers). Khi IR generates nhiều temporaries hơn 8, chúng bị spill xuống stack.

---

## Register Allocation

### Strategy: Linear Scan

1. **Allocate on-demand:** Mỗi IR temporary được assign một register từ `r4–r11` theo thứ tự.
2. **Track mapping:** `{temp_name → register}` và `{var_name → stack_offset}`.
3. **Spilling:** Khi hết registers, spill least recently used (LRU) temporary xuống stack.
4. **Reload on use:** Load spilled temporary từ stack khi cần dùng lại.

### Ví Dụ Allocation

**IR:**

```
t0 = x add y
t1 = t0 mul 2
t2 = t1 sub z
result = t2
```

**Allocation:**

| Temporary | Register |
|-----------|---------|
| `t0` | `r4` |
| `t1` | `r5` |
| `t2` | `r6` |
| `result` | `r7` |

**Generated Assembly:**

```asm
ldr r0, [fp, #-8]    @ load x
ldr r1, [fp, #-12]   @ load y
add r4, r0, r1       @ t0 = x + y
mov r0, #2
mul r5, r4, r0       @ t1 = t0 * 2
ldr r0, [fp, #-16]   @ load z
sub r6, r5, r0       @ t2 = t1 - z
str r6, [fp, #-20]   @ store result
```

### Spilling Example

Khi tất cả `r4–r11` đều occupied và cần thêm temporary:

```asm
@ Spill r4 (oldest) to stack
str r4, [sp, #-4]!   @ push r4 onto stack
                     @ r4 now available for new temporary
```

Khi cần dùng lại value đã spill:

```asm
ldr r4, [sp], #4     @ pop r4 from stack
```

---

## AAPCS Calling Convention

### Function Prologue

```asm
; function_entry foo
push    {r4-r11, lr}     @ Save callee-saved registers + return addr (36 bytes)
sub     sp, sp, #N       @ Allocate N bytes for local variables
                         @ N = (num_locals * 4) rounded up to 8-byte alignment
```

### Function Epilogue

```asm
; function_exit foo
add     sp, sp, #N       @ Deallocate local variables
pop     {r4-r11, pc}     @ Restore regs; pc = lr → return to caller
```

> **`pop {r4-r11, pc}`:** Pop lr (saved as pc) → return trực tiếp, không cần `bx lr`.

### Stack Frame Layout

```
[higher address]
    LR (return address)     ← saved by push {r4-r11, lr}
    r11 (fp)
    r10
    ...
    r4
    local_var_n             ← sp + 4*(n-1)  [allocated by sub sp, sp, #N]
    ...
    local_var_1             ← sp + 4
    local_var_0             ← sp            ← current SP
[lower address]
```

### Argument Passing (AAPCS)

| Position | Register | Notes |
|----------|---------|-------|
| Arg 1 | `r0` | Also return value |
| Arg 2 | `r1` | |
| Arg 3 | `r2` | |
| Arg 4 | `r3` | |
| Arg 5+ | Stack | Push in reverse order (không implement) |

> VinixOS Subset C hỗ trợ tối đa 4 parameters — stack arguments không cần thiết.

---

## Instruction Selection

### IR → ARM Mapping

| IR Operation | ARM Instructions | Notes |
|-------------|-----------------|-------|
| `t = a add b` | `add rd, ra, rb` | |
| `t = a sub b` | `sub rd, ra, rb` | |
| `t = a mul b` | `mul rd, ra, rb` | |
| `t = a div b` | `bl __aeabi_idiv` | Software division |
| `t = a mod b` | `bl __aeabi_idivmod` | r1 = remainder |
| `t = a and b` | `and rd, ra, rb` | |
| `t = a or b` | `orr rd, ra, rb` | |
| `t = a xor b` | `eor rd, ra, rb` | |
| `t = a shl b` | `lsl rd, ra, rb` | |
| `t = a shr b` | `asr rd, ra, rb` | Arithmetic shift right |
| `t = a eq b` | `cmp ra, rb` + `moveq` | |
| `t = a ne b` | `cmp ra, rb` + `movne` | |
| `t = a lt b` | `cmp ra, rb` + `movlt` | |
| `t = a gt b` | `cmp ra, rb` + `movgt` | |
| `t = neg a` | `neg rd, ra` | or `rsb rd, ra, #0` |
| `t = not a` | `cmp ra, #0` + `moveq r, #1` | Logical NOT |
| `t = deref p` | `ldr rd, [rp]` | |
| `t = addr x` | `add rd, fp, #offset` | |
| `t = load base off` | `ldr rd, [rb, #off]` | |
| `store base off val` | `str rv, [rb, #off]` | |
| `goto L` | `b L` | |
| `if t goto L` | `cmp rt, #0` + `bne L` | |
| `param a` | `mov r0, ra` (r1, r2, r3) | Sequential fill |
| `call f nargs` | `bl f` | Result in r0 |

### Comparison Pattern

```asm
@ t = x lt y  →  t = (x < y) ? 1 : 0
cmp     r0, r1      @ compare x, y
movlt   r4, #1      @ if less-than: r4 = 1
movge   r4, #0      @ else: r4 = 0
```

### Software Division

Cortex-A8 không có hardware division instruction:

```asm
@ t = a div b
mov     r0, ra      @ dividend in r0
mov     r1, rb      @ divisor in r1
bl      __aeabi_idiv @ result in r0
mov     r4, r0      @ move result to assigned temp register
```

> `__aeabi_idiv` và `__aeabi_idivmod` được provide bởi `toolchain/runtime/divmod.S`.

---

## Syscall Generation

Syscalls được trigger bởi `call` IR với syscall name được nhận ra:

```asm
@ write(1, msg, len) → syscall #0
mov     r0, #1          @ fd = 1 (stdout)
mov     r1, r_msg       @ buf
mov     r2, r_len       @ len
mov     r7, #0          @ SYS_WRITE = 0
svc     #0              @ trigger SVC exception
@ return value in r0
```

**Syscall numbers (VinixOS):**

| # | Syscall | r0 | r1 | r2 |
|---|---------|----|----|-----|
| 0 | `write` | fd | buf | count |
| 1 | `exit` | status | — | — |
| 2 | `yield` | — | — | — |
| 3 | `read` | fd | buf | count |
| 6 | `open` | path | flags | — |
| 7 | `read_file` | fd | buf | count |
| 8 | `close` | fd | — | — |

---

## End-to-End Example

**Source:**

```c
int add(int a, int b) {
    int result = a + b;
    return result;
}
```

**IR:**

```
function_entry add
t0 = a add b
result = t0
return result
function_exit add
```

**Generated Assembly:**

```asm
add:
    push    {r4-r11, lr}     @ prologue: save regs
    sub     sp, sp, #8       @ allocate 2 local vars (result, t0)

    @ t0 = a add b
    @ a in r0, b in r1 (AAPCS arguments)
    add     r4, r0, r1       @ r4 = t0 = a + b

    @ result = t0
    str     r4, [sp, #0]     @ store result on stack

    @ return result
    ldr     r0, [sp, #0]     @ load result into r0 (return value)

    add     sp, sp, #8       @ epilogue: deallocate
    pop     {r4-r11, pc}     @ restore regs + return
```

---

## Key Design Decisions

| Decision | Rationale | Trade-off |
|----------|-----------|-----------|
| Linear scan allocation | Simple, no need for interference graph | Không optimal như graph coloring |
| Software division | Cortex-A8 không có SDIV | Slower but correct |
| Save/restore all r4-r11 | Simple — không cần track which regs used | Slight overhead cho simple functions |
| Base address `0x40000000` | Match VinixOS user space | Fixed — không support PIE |
| No optimization | Focus on correctness | Output equivalent to `-O0` |

---

## Tóm Tắt

| Concept | Ý Nghĩa |
|---------|---------|
| r4-r11 = temporaries pool | 8 callee-saved registers cho IR temporaries |
| Linear scan allocation | Assign r4, r5, ... sequentially; spill to stack khi hết |
| AAPCS function frame | `push {r4-r11, lr}` prologue; `pop {r4-r11, pc}` epilogue |
| Arguments in r0-r3 | First 4 params; return value in r0 |
| Software division | `__aeabi_idiv` từ `runtime/divmod.S` |
| Syscall via r7+svc | `mov r7, #num; svc #0` — matches VinixOS ABI |
| No optimization | Correctness over performance — equivalent to GCC -O0 |

---

## Xem Thêm

- [ir_format.md](ir_format.md) — IR instructions được map ở đây
- [architecture.md](architecture.md) — Codegen trong compiler pipeline
- [VinixOS/docs/06-syscall-mechanism.md](../../VinixOS/docs/06-syscall-mechanism.md) — Kernel-side syscall handling

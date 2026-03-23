# IR Format — Three-Address Code

> **Phạm vi:** Định nghĩa đầy đủ format IR (Three-Address Code), tất cả instruction types, ví dụ generation, và các properties quan trọng.
> **Yêu cầu trước:** [architecture.md](architecture.md) — IR là middle-end của pipeline; [subset_c_spec.md](subset_c_spec.md) — biết ngôn ngữ input.
> **Files liên quan:** `toolchain/middleend/ir/ir_instructions.py`, `toolchain/middleend/ir/ir_generator.py`, `toolchain/middleend/ir/generators.py`

---

## Three-Address Code (3AC)

Mỗi IR instruction có tối đa **3 operands**:

```
result = operand1  op  operand2
```

**Tại sao 3AC:**
- Simple và uniform — dễ analyze, transform, và map sang assembly
- Explicit temporaries cho intermediate values
- Explicit control flow với labels và jumps
- Không phụ thuộc vào AST structure sau khi generate

---

## IR Instruction Types

### Quick Reference Table

| Category | Type | Format | Mô Tả |
|----------|------|--------|-------|
| Binary op | `BinaryOpIR` | `result = a op b` | Arithmetic, logical, comparison |
| Unary op | `UnaryOpIR` | `result = op a` | Neg, not, deref, addr |
| Assign | `AssignIR` | `dest = src` | Simple assignment |
| Load | `LoadIR` | `result = load base offset` | Load từ memory |
| Store | `StoreIR` | `store base offset value` | Write vào memory |
| Label | `LabelIR` | `label L0` | Jump target |
| Goto | `GotoIR` | `goto L0` | Unconditional jump |
| Cond goto | `CondGotoIR` | `if cond goto L0` | Conditional jump |
| Param | `ParamIR` | `param arg` | Setup function argument |
| Call | `CallIR` | `result = call func nargs` | Function call |
| Return | `ReturnIR` | `return [value]` | Return từ function |
| Func entry | `FunctionEntryIR` | `function_entry name` | Function begin marker |
| Func exit | `FunctionExitIR` | `function_exit name` | Function end marker |

---

## Binary Operations (`BinaryOpIR`)

**Format:** `result = operand1 op operand2`

**Operators:**

| Category | Operators |
|----------|---------|
| Arithmetic | `add`, `sub`, `mul`, `div`, `mod` |
| Logical/Bitwise | `and`, `or`, `xor`, `shl`, `shr` |
| Comparison | `eq`, `ne`, `lt`, `gt`, `le`, `ge` |

**Examples:**

```
t0 = x add y        /* t0 = x + y        */
t1 = t0 mul 2       /* t1 = t0 * 2       */
t2 = a lt b         /* t2 = (a < b)      */
t3 = x shl 3        /* t3 = x << 3       */
t4 = p and 0xFF     /* t4 = p & 0xFF     */
```

**C source → IR:**

```c
int z = (x + y) * 2;
```

```
t0 = x add y
t1 = t0 mul 2
z  = t1
```

---

## Unary Operations (`UnaryOpIR`)

**Format:** `result = op operand`

| Operator | Meaning | C equivalent |
|----------|---------|-------------|
| `neg` | Arithmetic negation | `-x` |
| `not` | Logical NOT | `!x` |
| `deref` | Pointer dereference | `*p` |
| `addr` | Address-of | `&x` |

**Examples:**

```
t0 = neg x         /* t0 = -x    */
t1 = not t0        /* t1 = !t0   */
t2 = deref p       /* t2 = *p    */
t3 = addr x        /* t3 = &x    */
```

---

## Assignment (`AssignIR`)

**Format:** `dest = source`

```
x = 5              /* Constant assignment  */
y = t0             /* Temp to variable     */
arr = t1           /* Array/pointer assign */
```

---

## Memory Operations

### Load (`LoadIR`)

**Format:** `result = load base offset`

```
t0 = load arr 0    /* t0 = arr[0]  (byte offset 0)  */
t1 = load arr 4    /* t1 = arr[1]  (byte offset 4)  */
t2 = load p 0      /* t2 = *p      (deref pointer)  */
```

### Store (`StoreIR`)

**Format:** `store base offset value`

```
store arr 0 5      /* arr[0] = 5   */
store arr 8 t0     /* arr[2] = t0  */
store p 0 x        /* *p = x       */
```

> **Byte offsets:** Mỗi `int` = 4 bytes. `arr[i]` → offset = `i * 4`.

---

## Control Flow

### Label (`LabelIR`)

**Format:** `label L<n>`

```
label L0           /* Jump target */
label L_end
```

### Goto (`GotoIR`)

**Format:** `goto L<n>`

```
goto L0            /* Unconditional jump */
goto L_end
```

### Conditional Goto (`CondGotoIR`)

**Format:** `if condition goto L<n>`

```
if t2 goto L0      /* if t2 != 0, jump to L0 */
if t3 goto L_true
```

---

## Function Operations

### Function Entry/Exit

```
function_entry factorial    /* Begin function */
    ...
function_exit factorial     /* End function   */
```

### Parameters (`ParamIR`)

**Format:** `param argument`

Push arguments trước khi call. Thứ tự: left-to-right.

```
param x
param y
t0 = call add 2    /* add(x, y) — 2 arguments */
```

### Call (`CallIR`)

**Format:** `result = call function_name nargs`

```
t0 = call factorial 1    /* result = factorial(n) */
t1 = call write 2        /* write(buf, len)       */
     call exit 1         /* exit(0) — no result   */
```

### Return (`ReturnIR`)

**Format:** `return [value]`

```
return 0           /* return 0;         */
return t1          /* return t1;        */
return             /* void function     */
```

---

## Control Flow Generation

### `if-else`

```c
if (x > 0) {
    y = 1;
} else {
    y = -1;
}
```

```
t0 = x gt 0
if t0 goto L0       /* if (x > 0) jump to then */
y = -1              /* else branch              */
goto L1
label L0
y = 1               /* then branch              */
label L1
```

### `while` Loop

```c
while (i < 10) {
    i = i + 1;
}
```

```
label L0            /* loop condition check */
t0 = i lt 10
if t0 goto L1       /* if condition true, enter body */
goto L2             /* exit loop */
label L1
t1 = i add 1
i = t1
goto L0             /* back to condition */
label L2
```

### `for` Loop

```c
for (i = 0; i < n; i = i + 1) {
    sum = sum + arr[i];
}
```

```
i = 0               /* init */
label L0
t0 = i lt n         /* condition */
if t0 goto L1
goto L2             /* exit */
label L1
t1 = i mul 4        /* arr[i]: byte offset */
t2 = load arr t1    /* arr[i] value */
t3 = sum add t2
sum = t3
t4 = i add 1        /* update */
i = t4
goto L0
label L2
```

### Recursive Function

```c
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
```

```
function_entry factorial
t0 = n le 1
if t0 goto L0
t1 = n sub 1
param t1
t2 = call factorial 1
t3 = n mul t2
return t3
label L0
return 1
function_exit factorial
```

---

## Temporary và Label Naming

**Temporaries:** `t0`, `t1`, `t2`, ... — auto-generated, sequential

**Labels:** `L0`, `L1`, `L2`, ... — auto-generated, sequential

Cả hai đều là **module từ `generators.py`:**

```python
class TempGenerator:
    def __init__(self): self._counter = 0
    def next(self): name = f"t{self._counter}"; self._counter += 1; return name

class LabelGenerator:
    def __init__(self): self._counter = 0
    def next(self): name = f"L{self._counter}"; self._counter += 1; return name
```

---

## IR Properties

| Property | Value |
|----------|-------|
| Representation | In-memory list of IR instruction objects |
| Max operands | 3 per instruction |
| Temporaries | Unlimited (spill to stack if needed) |
| Control flow | Explicit labels + goto (no structured loops) |
| Function scope | `function_entry` ... `function_exit` markers |
| Type information | Implicit — all values are 32-bit (int/pointer) |

---

## Tóm Tắt

| Concept | Ý Nghĩa |
|---------|---------|
| 3AC format | `result = op1 op op2` — tối đa 3 operands, uniform và simple |
| Explicit temporaries | `t0`, `t1`, ... — intermediate values không cần name |
| Explicit control flow | `label` + `goto` + `if goto` — không có structured loops |
| Load/Store | Memory access qua byte offsets — array indexing được flatten |
| function_entry/exit | Marker cho code generator biết function boundaries |
| Sequential labels | `L0`, `L1`, ... — unique across entire compilation unit |

---

## Xem Thêm

- [codegen_strategy.md](codegen_strategy.md) — IR → ARM assembly mapping
- [architecture.md](architecture.md) — IR vị trí trong pipeline
- [subset_c_spec.md](subset_c_spec.md) — Nguồn gốc của IR từ Subset C

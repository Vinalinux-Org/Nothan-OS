# ARM Architecture

Tài liệu kiến trúc ARM — programmer's model, instruction set, exception handling, assembly. Dùng khi viết code assembly, hiểu exception/interrupt flow, hoặc debug low-level issues.

## Files

| File | Mô tả |
|------|-------|
| `01_An_Overview_of_Computing_Systems.md` | Tổng quan hệ thống máy tính, memory hierarchy |
| `02_The_Programmers_Model.md` | Registers (R0-R15, CPSR), processor modes, pipeline |
| `03_Introduction_to_Instruction_Sets_v4T_and_v7-M.md` | Giới thiệu ARM/Thumb instruction sets |
| `04_Assembler_Rules_and_Directives.md` | Cú pháp assembler, directives (.section, .global, .align...) |
| `05_Loads_Stores_and_Addressing.md` | LDR/STR, addressing modes, memory access |
| `06_Constants_and_Literal_Pools.md` | Load constant, literal pool, MOV/MVN |
| `07_Integer_Logic_and_Arithmetic.md` | ALU instructions, flags, condition codes |
| `08_Branches_and_Loops.md` | B, BL, BX, conditional branches, loop patterns |
| `09_Introduction_to_Floating-Point_Basics_Data_Types_a.md` | FPU basics, data types |
| `10_Introduction_to_Floating-Point_Rounding_and_Except.md` | FPU rounding, exceptions |
| `11_Floating-Point_Data-Processing_Instructions.md` | VADD, VMUL, VCVT... |
| `12_Tables.md` | Lookup tables, jump tables |
| `13_Subroutines_and_Stacks.md` | PUSH/POP, AAPCS calling convention, stack frame |
| `14_ARM7TDMI_Exception_Handling.md` | Exception vectors, modes, IRQ/FIQ handling (ARM7) |
| `15_v7-M_Exception_Handling.md` | Exception handling trên Cortex-M (v7-M) |
| `16_Memory-Mapped_Peripherals.md` | MMIO, volatile, peripheral access patterns |
| `17_ARM_Thumb_and_Thumb-2_Instructions.md` | Thumb/Thumb-2 encoding, interworking |
| `18_Mixing_C_and_Assembly.md` | Inline assembly, C/ASM interface, AAPCS |

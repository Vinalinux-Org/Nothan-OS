# Chapter 4: Assembler Rules and Directives


## 4.1 INTRODUCTION

The ARM assembler included with the RealView Microcontroller Development Kit contains an extensive set of features found on most assemblers—essential for experienced programmers, but somewhat unnerving if you are forced to wade through volumes of documentation as a beginner. Code Composer Studio also has a nice assembler with myriad features, but the details in the ARM Assembly Language Tools User’s Guide run on for more than three hundred pages. In an attempt to cut right to the heart of programming, we now look at rules for the assembler, the structure of a program, and directives, which are instructions to the assembler for creating areas of code, aligning data, marking the end of your code, and so forth. These are unlike processor instructions, which tell the processor to add two numbers or jump somewhere in your code, since they never turn into actual machine instructions. Although both the ARM and TI assemblers are easy to learn, be aware that other assemblers have slightly different rules; e.g., gnu tools have directives that are preceded with a period and labels that are followed by a colon. It’s a Catch-22 situation really, as you cannot learn assembly without knowing how to use directives, but it’s that you will use every directive or every assembler option immediately, so for now, we start with what is essential. Read this chapter to get an overview of what’s possible, but don’t panic. As we proceed through more chapters of the book, you may find yourself flipping back to this chapter quite often, which is normal. You can, of course, refer back to the RealView Assembler User’s Guide found in the RVMDK tools or the Code Composer Studio documentation for the complete specifications of the assemblers if you need even more detail.


## 4.2 STRUCTURE OF ASSEMBLY LANGUAGE MODULES

We begin by examining a very simple module as a starting point. Consider the following code:


```asm
AREA ARMex, CODE, READONLY
; Name this block of code ARMex
ENTRY			            ; Mark first instruction to execute
start MOV    r0, #10      ; Set up parameters
MOV    r1, #3
```


```asm
ADD r0, r0, r1              ; r0 = r0 + r1
stop     B   stop		                  ; infinite loop
END			                      ; Mark end of file
```


While the routine may appear a little cryptic, it only does one thing: it adds the numbers 10 and 3 together. The rest of the code consists of directives for the assem-

```asm
bler and an instruction at the end to put the processor in an infinite loop. You can see
```

that there is some structure to the lines of code, and the general form of source lines in your assembly files is

{label} {instruction|directive|pseudo-instruction} {;comment}

where each field in braces is optional. Labels are names that you choose to represent an address somewhere in memory, and while they eventually do need to be translated into a numeric value, as a programmer you simply work with the name throughout your code. The linker will calculate the correct address during the linkage process that follows assembly. Note that a label name can only be defined once in your code, and labels must start at the beginning of the line (there are some assemblers that will allow you to place the label at any point, but they require delimiters such as a colon). The instructions, directives, and pseudo-instructions (such as ADR that we will see in Chapter 6) must be preceded by a white space, either a tab or any number of spaces, even if you don’t have a label at the beginning. One of the most common mistakes new programmers make is starting an instruction in column one. To make your code more readable, you may use blank lines, since all three sections of the source line are optional. ARM and Thumb instructions available on the ARM7TDMI are from the ARM version 4T instruction set; the Thumb-2 instructions used on the Cortex-M4 are from the v7-M instruction set. All of these can be found in the respective Architectural Reference Manuals, along with their mnemonics and uses. Just to start us off, the ARM instructions for the ARM7TDMI are also listed in Table 4.1, and we’ll slowly introduce the v7-M instructions throughout the text. There are many

```asm
directives and pseudo-instructions, but we will cover only a handful throughout this
```

chapter to get a sense of what is possible. The current ARM/Thumb assembler language, called Unified Assembler Language (UAL), has superseded earlier versions of both the ARM and Thumb assembler languages (we saw a few Thumb instructions in Chapter 3, and we’ll see more throughout the book, particularly in Chapter 17). To give you some idea of the subtle changes involved, compare the two formats for performing a shift operation:


```asm
Old ARM format                                 UAL format
```

MOV <Rd>, <Rn>, LSL shift LSL <Rd>, <Rn>, shift LDR{cond}SB LDRSB{cond} LDMFD sp!,{reglist} PUSH {reglist}

Code written using UAL can be assembled for ARM, Thumb, or Thumb-2, which is an extension of the Thumb instruction set found on the more recent ARM


> **TABLE 4.1**: 


```asm
ARM Version 4T Instruction Set
ADC            ADD             AND             B              BL
```

BX CDP CMN CMP EOR

```asm
LDC            LDM             LDR             LDRB           LDRBT
```

LDRH LDRSB LDRSH LDRT MCR

```asm
MLA            MOV             MRC             MRS            MSR
MUL            MVN             ORR             RSB            RSC
SBC            SMLAL           SMULL           STC            STM
STR            STRB            STRBT           STRH           STRT
```

SUB SWIa SWP SWPB TEQ

```asm
TST            UMLAL           UMULL
```


a The SWI instruction was deprecated in the latest version of the ARM Architectural Reference Manual (2007c), so while you should use the SVC instruction, you may still see this instruction in some older code.


processors, e.g., Cortex-A8. However, you’re likely to find a great a deal of code written using the older format, so be mindful of the changes when you review older programs. Also be aware that a disassembly of your code will show the UAL nota-

```asm
tions if you are using the RealView tools or Code Composer Studio. You can find
```

more details on UAL formats in the RealView Assembler User’s Guide located in the RVMDK tools. We’ll examine commented code throughout the book, but in general it is a good idea to document your code as much as possible, with clear statements about the operation of certain lines. Remember that on large projects, you will probably not be the only one reading your code. Guidelines for good comments include the following:

- Don’t comment the obvious. If you’re adding one to a register, don’t write “Register r3 + 1.” • Use concise language when describing what registers hold or how a function behaves. • Comment the sections of code where you think another programmer might have a difficult time following your reasoning. Complicated algorithms usually require a deep understanding of the code, and a bug may take days to find without adequate documentation. • In addition to commenting individual instructions, include a short description of functions, subroutines, or long segments of code. • Do not abbreviate, if possible. • Acronyms should be avoided, but this can be difficult sometimes, since peripheral register names tend to be shortened. For example, VIC0_VA7R might not mean much in a comment, so if you use the name in the instruction, describe what the register does.


of a comment, unless you have the semicolon inside of a string constant, for example,

abc SETS “This is a semicolon;”

Here, a string is assigned to the variable abc, but since the semicolon lies within quotes, there is no comment on this line. The end of the line is the end of the comment, and a comment can occupy the entire line if you wish. The TI assembler will allow you to place either an asterisk (*) or a semicolon in column 1 to denote a comment, or a semicolon anywhere else on the line. At some point, you will begin using constants in your assembly, and they are allowed in a handful of formats:

- Decimal, for example, 123 • Hexadecimal, for example, 0x3F • n_xxx (Keil only) where: n is a base between 2 and 9 xxx is a number in that base

Character constants consist of opening and closing single quotes, enclosing either a single character or an escaped character, using the standard C escape characters (recall that escape characters are those that act as nonprinting characters, such as \n for creating a new line). String constants are contained within double quotes. The standard C escape sequences can be used within string constants, but they are done differently by assemblers. For example, in the Keil tools, you could say something like

MOV r3, #’A’ ; single character constant GBLS str1 ; set the value of global string variable str1 SETS “Hello world!\n”

In the Code Composer Studio tools, you might say

.string “Hello world!”

which places 8-bit characters in the string into a section of code, but the .string directive neither adds a NUL character at the end of the characters nor interprets escape characters. Instead, you could say

.cstring “Hello world!\n”

which both adds the NUL character for you and correctly interprets the \n escape character at the end. Before we move into directives, we need to cover a few housekeeping rules. For the Keil tools, there are case rules associated with your commands, so while you can write the instruction mnemonics, directives, and symbolic register names in either uppercase or lowercase, you cannot mix them. For example ADD or add are acceptable, but not Add. When it comes to mnemonics, the TI assembler is case-insensitive.


To make the source file easier to read, the Keil tools allow you to split up a single line into several lines by placing a backslash character (\) at the end of a line. If you had a long string, you might write

ISR_Stack_Size EQU (UND_Stack_Size + SVC_Stack_Size + ABT_Stack_Size + \ FIQ_Stack_Size + IRQ_Stack_Size)

There must not be any other characters following the backslash, such as a space or a tab. The end-of-line sequence is treated as a white space by the assembler. Using the Keil tools, you may have up to 4095 characters for any given line, including any extensions using backslashes. The TI tools only allow 400 characters per line—anything longer is truncated. For either tool, keep the lines relatively short for easier reading!


## 4.3 PREDEFINED REGISTER NAMES

Most assemblers have a set of register names that can be used interchangeably in your code, mostly to make it easier to read. The ARM assembler is no different, and includes a set of predefined, case-sensitive names that are synonymous with registers. While the tools recognize predeclared names for basic registers, status registers, floating-point registers, and coprocessors, only the following are of immediate use to us:

r0-r15 or R0-R15 s0-s31 or S0-S31 a1-a4 (argument, result, or scratch registers, synonyms for r0 to r3) sp or SP (stack pointer, r13) lr or LR (Link Register, r14) pc or PC (Program Counter, r15) cpsr or CPSR (current program status register) spsr or SPSR (saved program status register) apsr or APSR (application program status register)


## 4.4 FREQUENTLY USED DIRECTIVES

A complete description of the assembler directives can be found in Section 4.3 of the RealView Assembler User’s Guide or Chapter 4 of ARM Assembly Language Tools User’s Guide; however, in order to start coding, you only need a few. We’ll examine the more frequently used directives first, shown in Table 4.2, and leave the others as reference material should you require them. Then we’ll move on to macros in the next section.

4.4.1 Defining a Block of Data or Code As you create code, particularly compiled code from C programs, the tools will need to be told how to treat all the different parts of it—data sections, program sections,


> **TABLE 4.2**: 

Frequently Used Directives Keil Directive CCS Directive Uses AREA .sect Defines a block of code or data RN .asg Can be used to associate a register with a name

```asm
EQU                 .equ                   Equates a symbol to a numeric constant
```

ENTRY Declares an entry point to your program DCB, DCW, DCD .byte, .half, .word Allocates memory and specifies initial runtime contents

```asm
ALIGN               .align                 Aligns data or code to a particular memory boundary
SPACE               .space                 Reserves a zeroed block of memory of a particular size
```

LTORG Assigns the starting point of a literal pool

```asm
END                 .end                   Designates the end of a source file
```


blocks of coefficients, etc. These sections, which are indivisible and named, then get manipulated by the linker and ultimately end up in the correct type of memory in a system. For example, data, which could be read-write information, could get stored in RAM, as opposed to the program code which might end up in Flash memory. Normally you will have separate sections for your program and your data, especially in larger programs. Blocks of coefficients or tables can be placed in a section of their own. Since the two main tool sets that we’ll use throughout the book do things in very different ways, both formats are presented below.

4.4.1.1 Keil Tools You tell the assembler to begin a new code or data section using the AREA directive, which has the following syntax:


```asm
AREA sectionname{,attr}{,attr}…
```


where sectionname is the name that the section is to be given. Sections can be given almost any name, but if you start a section name with a digit, it must be enclosed in bars, e.g., |1_DataArea|; otherwise, the assembler reports a missing section name error. There are some names you cannot use, such as |.text|, since this is used by the C compiler (but it would be a rather odd name to pick at random). Your code must have at least one AREA directive in it, which you’ll usually find in the first few lines of a program. Table 4.3 shows some of the attributes that are available, but a full list can be found in the RealView Assembler User Guide in the Keil tools.

EXAMPLE 4.1 The following example defines a read-only code section named Example.


```asm
AREA Example,CODE,READONLY ; An example code section.
; code
```


> **TABLE 4.3**: 

Valid Section Attributes (Keil Tools) ALIGN = expr This aligns a section on a 2expr-byte boundary (note that this is different from the

```asm
ALIGN directive); e.g., if expr = 10, then the section is aligned to a 1KB
```

boundary. CODE The section is machine code (READONLY is the default) DATA The section is data (READWRITE is the default) READONLY The section can be placed in read-only memory (default for sections of CODE) READWRITE The section can be placed in read-write memory (default for sections of DATA)


4.4.1.2 Code Composer Studio Tools It’s often helpful to break up large assembly files into sections, e.g., creating a separate section for large data sets or blocks of coefficients. In fact, the TI assembler has directives to address similar concepts. Table 4.4 shows some of the directives used to create sections. The .sect directive is similar to the AREA directive in that you use it to create an initialized section, to put either your code or some initialized data there. Sections can be made read-only or read-write, just as with Keil tools. You can make as many sections as you like; however, it is usually best to make only as many as needed. An example of a section of data called Coefficients might look like

.sect “Coefficients”

```asm
.float 0.05
.float 2.03
.word 0AAh
```


The default section is the .text section, which is where your assembly program will normally sit, and in fact, you can create it either by saying

.sect “.text”


> **TABLE 4.4**: 

TI Assembler Section Directives Directive Use Uninitialized sections .bss Reserves space in the .bss section .usect Reserves space in a specified uninitialized named section Initialized sections .text The default section where the compiler places code

```asm
.data        Normally used for pre-initialized variables or tables
```

.sect Defines a named section similar to the default .text and

```asm
.data sections
```


or by simply typing


```asm
.text
```


Anything after this will be placed in the .text section. As we’ll see in Chapter 5 for both the Keil tools and the Code Composer Studio tools, there is a linker command file and a memory map that determines where all of these sections ultimately end up in memory. As with most silicon vendors, TI ships a default linker command file for their MCUs, so you shouldn’t need to modify anything to get up and running.


### 4.4.2 Register Name Definition

4.4.2.1 Keil Tools In the ARM assembler that comes with the Keil tools, there is a directive RN that defines a register name for a specified register. It’s not mandatory to use such a directive, but it can help in code readability. The syntax is

name RN expr

where name is the name to be assigned to the register. Obviously name cannot be the same as any of the predefined names listed in Section 4.3. The expr parameter takes on values from 0 to 15. Mind that you do not assign two or more names to the same register.

EXAMPLE 4.2 The following registers have been given names that can be used throughout ­further code: coeff1 RN 8 ; coefficient 1 coeff2 RN 9 ; coefficient 2 dest RN 0 ; register 0 holds the pointer to

```asm
; destination matrix
```


4.4.2.2 Code Composer Studio You can assign names to registers using the .asg directive. The syntax is

.asg “character string”, substitution symbol

For example, you might say .asg R13, STACKPTR

```asm
ADD      STACKPTR, STACKPTR, #3
```


4.4.3 Equating a Symbol to a Numeric Constant It is frequently useful to give a symbolic name to a numeric constant, a registerrelative value, or a program-relative value. Such a directive is similar to the use of #define to define a constant in C. Note that the assembler doesn’t actually place


anything at a particular memory location. It merely equates a label with an operand, either a value or another label, for example.

4.4.3.1 Keil Tools The syntax for the EQU directive is

name EQU expr{,type}

where name is the symbolic name to assign to the value, expr is a register-relative address, a program-relative address, an absolute address, or a 32-bit integer constant. The parameter type is optional and can be any one of

ARM THUMB CODE16 CODE32 DATA

EXAMPLE 4.3 SRAM_BASE EQU 0x04000000 ; assigns SRAM a base address abc EQU 2 ; a ssigns the value 2 to the symbol abc xyz EQU label+8 ;  assigns the address (label+8)

```asm
; to the symbol xyz
```

fiq EQU 0x1C, CODE32 ; assigns the absolute address

```asm
; 0
```

x1C to the symbol fiq, and marks it

```asm
; as code
```


4.4.3.2 Code Composer Studio There are two identical (and interchangeable) directives for equating names with

```asm
constants and other values: .set and .equ. Notice that registers can be given names
```

using these directives as well as values. Their syntax is


```asm
symbol .set
```

value

```asm
symbol .equ  value
```


EXAMPLE 4.4

```asm
AUX_R4     .set     R4                ; equate symbol AUX_R4 to register R4
OFFSET     .equ     50/2 + 3          ; equate OFFSET to a numeric value
ADD      r0, AUX_R4,       #OFFSET
```


4.4.4 Declaring an Entry Point In the Keil tools, the ENTRY directive declares an entry point to a program. The syntax is

ENTRY

Your program must have at least one ENTRY point for a program; otherwise, a warning is generated at link time. If you have a project with multiple source files, not


every source file will have an ENTRY directive, and any single source file should only have one ENTRY directive. The assembler will generate an error if more than

```asm
one ENTRY exists in a single source file.
```


EXAMPLE 4.5

```asm
AREA ARMex, CODE, READONLY
ENTRY	    ; Entry point for the application
```


### 4.4.5 Allocating Memory and Specifying Contents

When writing programs that contain tables or data that must be configured before the program begins, it is necessary to specify exactly what memory looks like. Strings, floating-point constants, and even addresses can be stored in memory as data using various directives.

4.4.5.1 Keil Tools One of the more common directives, DCB, actually defines the initial runtime contents of memory. The syntax is {label} DCB expr{,expr}…

where expr is either a numeric expression that evaluates to an integer in the range −128 to 255, or a quoted string, where the characters of the string are stored consecutively in memory. Since the DCB directive affects memory at the byte level, you should use an ALIGN directive afterward if any instructions follow to ensure that the instruction is aligned correctly in memory.

EXAMPLE 4.6 Unlike strings in C, ARM assembler strings are not null-terminated. You can con-

```c
struct a null-terminated string using DCB as follows:
```


```asm
C_string DCB “C_string”,0
```


If this string started at address 0x4000 in memory, it would look like

Address ASCII equivalent 0x4000 43 C 0x4001 5F _ 0x4002 73 s 0x4003 74 t 0x4004 72 r 0x4005 69 i 0x4006 6E n 0x4007 67 g 0x4008 00

Compare this to the way to that the Code Composer Studio assembler did the same thing using the .cstring directive in Section 4.2.


In addition to the directive for allocating memory at the resolution of bytes, there are directives for reserving and defining halfwords and words, with and without alignment. The DCW directive allocates one or more halfwords of memory, aligned on two-byte boundaries (DCWU does the same thing, only without the memory alignment). The syntax for these directives is

{label} DCW{U} expr{,expr}…

where expr is a numeric expression that evaluates to an integer in the range −32768 to 65535. Another frequently used directive, DCD, allocates one or more words of memory, aligned on four-byte boundaries (DCDU does the same thing, only without the memory alignment). The syntax for these directives is

{label} DCD{U} expr{,expr}

where expr is either a numeric expression or a program-relative expression. DCD inserts up to 3 bytes of padding before the first defined word, if necessary, to achieve a 4-byte alignment. If alignment isn’t required, then use the DCDU directive.

EXAMPLE 4.7

```asm
coeff DCW       0xFE37, 0x8ECC           ; defines 2 halfwords
data1 DCD       1,5,20                   ; defines 3 words containing
; decimal values 1, 5, and 20
data2 DCD        mem06 + 4               ; defines 1 word containing 4 +
; 
```

the address of the label mem06


```asm
AREA         MyData, DATA, READWRITE
DCB          255            ; now misaligned...
```

data3 DCDU 1,5,20 ; defines 3 words containing

```asm
; 1, 5, and 20 not word aligned
```


4.4.5.2 Code Composer Studio There are similar directives in CCS for initializing memory, each directive specifying the width of the values being used. For placing one or more values into consecutive bytes of the current section, you can use either the .byte or .char directive. The syntax is

{label} .byte value1{,…,valuen}

where value can either be a string in quotes or some other expression that gets evaluated assuming the data is 8-bit signed data.

EXAMPLE 4.8 If you wanted to place a few constants and some short strings in memory, you could say


```asm
LAB1     .byte      10, −1, “abc”, ‘a’
```


and in memory the values would appear as

0A FF 61 62 63 61

For halfword values, there are .half and .short directives which will always align the data to halfword boundaries in the section. For word length values, there are .int, .long, and .word directives, which also align the data to word boundaries in the section. There is even a .float directive (for single-precision floating-point values) and a .double directive (for double-precision floating-point values)!


### 4.4.6 Aligning Data or Code to Appropriate Boundaries

Sometimes you must ensure that your data and code are aligned to appropriate boundaries. This is typically required in circumstances where it’s necessary or optimal to have your data aligned a particular way. For example, the ARM940T processor has a cache with 16-byte cache lines, and to maximize the efficiency of the cache, you might try to align your data or function entries along 16-byte boundaries. For those processors where you can load and store double words (64 bits), such as the ARM1020E or ARM1136EJ-S, the data must be on an 8-byte boundary. A label on a line by itself can be arbitrarily aligned, so you might use ALIGN 4 before the label

```asm
to align your ARM code, or ALIGN 2 to align Thumb code.
```


4.4.6.1 Keil Tools The ALIGN directive aligns the current location to a specified boundary by padding with zeros. The syntax is

ALIGN {expr{,offset}}

where expr is a numeric expression evaluating to any power of two from 20 to 231, and offset can be any numeric expression. The current location is aligned to the next address of the form

offset + n * expr

If expr is not specified, ALIGN sets the current location to the next word (four byte) boundary.

EXAMPLE 4.9

```asm
AREA OffsetExample, CODE
DCB 1		     ; This example places the two
ALIGN 4,3	     ; bytes in the first and fourth
DCB 1		     ; bytes of the same word

AREA Example, CODE, READONLY
start		 LDR r6, = label1
; code
```

MOV pc,lr


```asm
label1		           DCB 1		      ; pc now misaligned
ALIGN		      ; ensures that subroutine1 addresses
subroutine1        MOV r5, #0x5 ; the following instruction
```


4.4.6.2 Code Composer Studio The .align directive can be used to align the section Program Counter to a particular boundary within the current section. The syntax is


```asm
.align {size in bytes}
```


If you do not specify a size, the default is one byte. Otherwise, a size of 2 aligns code or data to a halfword boundary, a size of 4 aligns to a word boundary, etc.

4.4.7 Reserving a Block of Memory You may wish to reserve a block of memory for variables, tables, or storing data during routines. The SPACE and .space directives reserve a zeroed block of memory.

4.4.7.1 Keil Tools The syntax is

{label} SPACE expr

where expr evaluates to the number of zeroed bytes to reserve. You may also want to use the ALIGN directive after using a SPACE directive, to align any code that follows.

EXAMPLE 4.10

```asm
AREA MyData, DATA, READWRITE
data1 SPACE 255 ; defines 255 bytes of zeroed storage
```


4.4.7.2 Code Composer Studio There are actually two directives that reserve memory—the .space and .bes directives. When a label is used with the .space directive, it points to the first byte reserved in memory, while the .bes points to the last byte reserved. The syntax for the two is

{label} .space size (in bytes) {label} .bes size (in bytes)

EXAMPLE 4.11 RES_1: .space 100 ; RES_1 points to the first byte RES_2: .bes 30 ; RES_2 points to the last byte

As an aside, there is also a .bss directive for reserving uninitialized space—­ consult Chapter 4 of ARM Assembly Language Tools User’s Guide for all the details.


### 4.4.8 Assigning Literal Pool Origins

Literal pools are areas of data that the ARM assembler creates for you at the end of every code section, specifically for constants that cannot be created with rotation schemes or that do not fit into an instruction’s supported formats. Chapter 6 discusses literal pools at length, but you should at least see the uses for the LTORG directive here. Situations arise where you might have to give the assembler a bit of help in placing literal pools, since they are placed at the end of code sections, and these ends rely on the AREA directives at the beginning of sections that follow (or the end of your code).

EXAMPLE 4.12 Consider the code below. An LDR pseudo-instruction is used to move the constant 0x55555555 into register r1, which ultimately gets converted into a real LDR instruction with a PC-relative offset. This offset must be calculated by the assembler, but the offset has limits (4 kilobytes). Imagine then that we reserve 4200 bytes of memory just at the end of our code—the literal pool would go after the big, empty block of memory, but this is too far away. An LTORG directive is required to force the assembler to put the literal pool after the MOV instruction instead, allowing an offset to be calculated that is within the 4 kilobyte range. In larger programs, you may find yourself making several literal pools, so place them after unconditional branches or subroutine return instructions. This prevents the processor from executing the constants as instructions.


```asm
AREA Example, CODE, READONLY
start BL     func1
```

func1 ; function body

```asm
; code
LDR    r1, = 0x55555555 ; => LDR R1, [pc, #offset to lit
; pool 1]
; code
MOV    pc,lr            ; end function
LTORG		                 ; l
```

it. pool 1 contains literal

```asm
; 0x55555555
data  SPACE 4200              ; c
```

lears 4200 bytes of memory,

```asm
; s
```

tarting at current location

```asm
END		                   ; d
```

efault literal pool is empty

Note that the Keil tools permit the use of the LDR pseudo-instruction, but Code Composer Studio does not, so there is no equivalent of the LTORG directive in the CCS assembler.


4.4.9 Ending a Source File This is the easiest of the directives—END simply tells the assembler you’re at the end of a source file. The syntax for the Keil tools is


```asm
END
```


and for Code Composer Studio, it’s


```asm
.end
```


When you terminate your source file, place the directive on a line by itself.


## 4.5 MACROS

Macro definitions allow a programmer to build definitions of functions or operations once, and then call this operation by name throughout the code, saving some writing time. In fact, macros can be part of a process known as conditional assembly, wherein parts of the source file may or may not be assembled based on certain variables, such as the architecture version (or a variable that you specify yourself). While this topic is not discussed here, you can find all the specifics about conditional assembly, along with the directives involved, in the Directives Reference section of the RealView Assembler User’s Guide or the Macro Description chapter of the ARM Assembly Language Tools User’s Guide from TI. The use of macros is neither recommended nor discouraged, as there are advan-

```asm
tages and disadvantages to using them. You can generally shorten your source code
```

by using them, but when the macros are expanded, they may chew up memory space because of their frequent use. Macros can sometimes be quite large. Using macros does allow you to change your code more quickly, since you usually only have to edit one block, rather than multiple instances of the same type of code. You can also define a new operation in your code by writing it as a macro and then calling it when-

```asm
ever it is needed. Just be sure to document the new operation thoroughly, as someone
```

unfamiliar with your code may one day have to read it! Note that macros are not the same thing as a subroutine call, since the macro definitions are substituted at assembly time, replacing the macro call with the actual assembly code. It is sometimes actually easier to follow the logic of source code if repeated sections are replaced with a macro, but they are not required in writing assembly. Let’s examine macros using only the Keil tools—the concept translates easily to Code Composer Studio. Two directives are used to define a macro: MACRO and MEND. The syntax is

MACRO

{$label} macroname{$cond} {$parameter{,$parameter}…}


```asm
; code
```

MEND

where $label is a parameter that is substituted with a symbol given when the macro is invoked. The symbol is usually a label. The macro name must not begin with an instruction or directive name. The parameter $cond is a special parameter designed to contain a condition code; however, values other than valid condition codes are permitted. The term $parameter is substituted when the macro is invoked. Within the macro body, parameters such as $label, $parameter, or $cond can be used in the same way as other variables. They are given new values each time the


macro is invoked. Parameters must begin with $ to distinguish them from ordinary symbols. Any number of parameters can be used. The $label field is optional, and the macro itself defines the locations of any labels.

EXAMPLE 4.13 Suppose you have a sequence of instructions that appears multiple times in your code—in this case, two ADD instructions followed by a multiplication. You could define a small macro as follows:


```asm
MACRO
; macro definition:
;
; vara = 8 * (varb + varc + 6)
```


$Label_1 AddMul $vara, $varb, $varc

$Label_1

```asm
ADD $vara, $varb, $varc		                ; add two terms
ADD $vara, $vara, #6		                   ; add 6 to the sum
LSL $vara, $vara, #3		                   ; multiply by 8
MEND
```


In your source code file, you can then instantiate the macro as many times as you like. You might call the sequence as


```asm
; invoke the macro
```

CSet1 AddMul r0, r1, r2

```asm
; the rest of your code
```


and the assembler makes the necessary substitutions, so that the assembly listing actually reads as


```asm
; invoke the macro
```

CSet1 ADD r0, r1, r2 ADD r0, r0, #6 LSL r0, r0, #3

```asm
; the rest of your code
```


## 4.6 MISCELLANEOUS ASSEMBLER FEATURES

While your first program will not likely contain many of these, advanced programmers typically throw variables, literals, and complex expressions into their code to save time in writing assembly. Consult the RealView Assembler User’s Guide

```asm
or ARM Assembly Language Tools User’s Guide for the complete set of rules and
```

allowable expressions, but we can adopt a few of the most common operations for our own use throughout the book.

4.6.1 Assembler Operators Primitive operations can be performed on data before it is used in an instruction. Note that these operators apply to the data—they are not part of an instruction.


Operators can be used on a single value (unary operators) or two values (binary operators). Unary operators are not that common; however, binary operators prove to be quite handy for shuffling bits across a register or creating masks. Some of the most useful binary operators are


Keil Tools Code Composer Studio

A modulo B A:MOD:B A%B Rotate A left by B bits A:ROL:B Rotate A right by B bits A:ROR:B Shift A left by B bits A:SHL:B or A << B A << B Shift A right by B bits A:SHR:B or A >> B A >> B Add A to B A+B A+B

```asm
Subtract B from A                   A−B                             A−B
Bitwise AND of A and B              A:AND:B                         A&B
```

Bitwise Exclusive OR of A and B A:EOR:B A^B Bitwise OR of A and B A:OR:B A|B


These types of operators creep into macros especially, and should you find yourself writing conditional assembly files, for whatever reason, you may decide to use these types of operators to control the creation of the source code.

EXAMPLE 4.14 To set a particular bit in a register (say if it were a bit to enable/disable the caches, a branch predictor, interrupts, etc.) you might have the control register copied to a general-purpose register first. Then the bit of interest would be modified using an OR operation, and the control register would be stored back. The OR instruction might look like


```asm
ORR r1, r1, #1:SHL:3   ; set CCREG[3]
```


Here, a 1 is shifted left three bits. Assuming you like to call register r1 CCREG, you have now set bit 3. The advantage in writing it this way is that you are more likely to understand that you wanted a one in a particular bit location, rather than simply using a logical operation with a value such as 0x8.

You can even use these operators in the creation of constants, for example,

DCD (0x8321:SHL:4):OR:2

which could move this two-byte field to the left by four bits, and then set bit 1 of the resulting constant with the use of the OR operator. This might be easier to read, since you may need a two-byte value shifted, and reading the original before the shift may help in understanding what the code does. It is not necessary to do this, but again, it provides some insight into the code’s behavior.


To create very specific bit patterns quickly, you can string together many operators in the same field, such as


```asm
MOV    r0, #((1:SHL:14):OR:(1:SHL:12))
```


which may look a little odd, but in effect we are putting the constant 0x5000 into register r0 by taking two individual bits, shifting them to the left, and then ORing the two patterns (convince yourself of this). It would look very similar in the Code Composer Studio tools as


```asm
MOV      r0, #((1 <<14) | (1 <<12))
```


You may wonder why we’re creating such a strange configuration and not something simpler, such as


```asm
MOV    r0, #0x5000
```


which is clearly easier to enter. Again, it depends on the context of the program. The programmer may need to load a configuration register, which often has very specific bit fields for functions, and the former notation will remind the reader that you are enabling two distinct bits in that register.


4.6.2 Math Functions in CCS There are a number of built-in functions within Code Composer Studio that make math operations a bit easier. Some of the many included functions are

$$cos(expr) Returns the cosine of expr as a floating-point value $$sin(expr) Returns the sine of expr as a floating-point value $$log(expr) Returns the natural logarithm of expr, where expr > 0 $$max(expr1, expr2) Returns the maximum of two values $$sqrt(expr)	Returns the square root of expr, where expr >= 0, as a floating-point value

You may never use these in your code; however, for algorithmic development, they often prove useful for quick tests and checks of your own routines.


EXAMPLE 4.15 You can build a list of trigonometric values very quickly in a data section by saying something like


```asm
.float		            $$cos(0.434)
.float		            $$cos(0.348)
.float		            $$sin(0.943)
.float		            $$tan(0.342)
```


## 4.7 EXERCISES

1. What is wrong with the following program?

```asm
AREA		            ARMex2, CODE, READONLY
ENTRY
```

start MOV r0, #6 ADD r1, r2, #2

```asm
END
```


2. What is another way of writing the following line of code?

MOV PC, LR

3. Use a Keil directive to assign register r6 to the name bouncer.

4. Use a Code Composer Studio directive to assign register r2 to the name FIR _ index.

5. Fill in the missing Keil directive below: SRAM_BASE 0x2000 MOV r12, #SRAM_BASE STR r6, [r12]

6. What is the purpose of a macro?

7. Create a mask (bit pattern) in memory using the DCD directive (Keil) and the SHL and OR operators for the following cases. Repeat the exercise using

```asm
the .word directive (CCS) and the << and | operators. Remember that bit 31
```

is the most significant bit of a word and bit 0 is the least significant bit. a. The upper two bytes of the word are 0xFFEE and the least significant bit is set. b. Bits 17 and 16 are set, and the least significant byte of the word is 0x8F. c. Bits 15 and 13 are set (hint: do this with two SHL directives). d. Bits 31 and 23 are set.

8. Give the Keil directive that assigns the address 0x800C to the symbol INTEREST.

9. What constant would be created if the following operators are used with a

```asm
DCD directive? For example,

MASK DCD 0x5F:ROL:3
```


a. 0x5F:SHR:2 b. 0x5F:AND:0xFC c. 0x5F:EOR:0xFF d. 0x5F:SHL:12


10. What constant would be created if the following operators are used with a .word directive? For example,


```asm
MASK .word 0x9B < <3
```


a. 0x9B>>2 b. 0x9B & 0xFC c. 0x9B ^ 0xFF d. 0x9B<<12

11. What instruction puts the ASCII representation of the character “R” in register r11?

12. Give the Keil directive to reserve a block of zeroed memory, holding 40

```asm
words and labeled coeffs.
```


13. Give the CCS directive to reserve a block of zeroed memory, holding 40

```asm
words and labeled coeffs.
```


14. Explain the difference between Keil’s EQU, DCD, and RN directives. Which, if any, would be used for the following cases? a. Assigning the Abort mode’s bit pattern (0x17) to a new label called Mode_ABT. b. Storing sequential byte-sized numbers in memory to be used for copying to another location in memory. c. Storing the contents of register r12 to memory address 0x40000004. d. Associating a particular microcontroller’s predefined memory-mapped register address with a name from the chip’s documentation, for example, VIC0_VA7R.

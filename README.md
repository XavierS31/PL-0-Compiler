# PL/0 Compiler

This repository contains the complete PL0 compiler pipeline for HW4 in COP3402 System Software. The project integrates the scanner from HW2, the parser and code generator from HW3, and the virtual machine from HW1. Together these components form a full working compiler that reads a PL0 source program and executes the resulting PM0 assembly instructions.

---

## Overview

This project implements a full compilation pipeline for a small educational programming language called PL0. The compiler consists of three C programs that operate in sequence:

1. Scanner (lex.c)  
2. Parser and Code Generator (parsercodegen_complete.c)  
3. Virtual Machine (vm.c)  

Each stage transforms the input into a form required by the next stage, resulting in a complete compiler system.

---

## What is PL0

PL0 is a simple structured teaching language designed to illustrate compiler construction concepts. It includes:

- Constant declarations  
- Variable declarations  
- Arithmetic expressions  
- Assignment statements  
- If then else fi  
- While do  
- Procedure declarations  
- Procedure calls  
- Nested blocks  
- Odd and even tests  
- Parenthesized expressions  

Every PL0 program ends with a period.

### Example Program

```
var x;

begin
    x := 5;
    if x > 3 then
        x := x - 1
    else
        x := x + 1
    fi
end.
```

---

## Compiler Structure

The compiler operates in three stages that run one after another.

---

### 1. Scanner Stage (lex.c)

The scanner reads the raw PL0 source code and converts it into tokens.  
Tokens include identifiers, numbers, operators, and keywords.

Output file: `tokens.txt`

Example:

Input:
```
x := 10;
```

Output:
```
identsym x
becomesym :=
numbersym 10
semicolonsym ;
```

---

### 2. Parser and Code Generator Stage (parsercodegen_complete.c)

The parser reads `tokens.txt` and uses recursive descent parsing to verify syntax and generate PM0 assembly code.

This stage performs:

- Syntax checking  
- Symbol table creation and management  
- Lexical level tracking  
- Emitting PM0 assembly instructions  
- Backpatching jump instructions  
- Allocating stack space  
- Supporting procedure declarations and calls  

Output file: `elf.txt`

Generated instructions include:

- `LIT` for constants  
- `LOD` and `STO` for loading and storing values  
- `CAL` for procedure calls  
- `OPR` for arithmetic and logic  
- `JMP` and `JPC` for jumps  
- `INC` for memory allocation  
- `RET` for returning from procedures  

---

### 3. Virtual Machine Stage (vm.c)

The virtual machine reads `elf.txt` and executes PM0 instructions. It simulates:

- A runtime stack  
- Base and stack pointers  
- Static links for lexical scoping  
- Activation records  
- Arithmetic and logical operations  
- Control flow  
- Memory load and store  
- EVEN instruction (`OPR 0 11`) which pushes 1 if the value is even and 0 if not  

This stage produces the final execution output of the PL0 program.

---

## Input and Output Summary

### Input to Scanner
A PL0 program file such as `input.txt`.

### Output of Scanner
`tokens.txt`

### Output of Parser/Code Generator
`elf.txt`

### Output of Virtual Machine
Execution results printed to the terminal.

---

## How to Build and Run

### Compile on Eustis

```
gcc -O2 -std=c11 -o lex lex.c
gcc -O2 -std=c11 -o parsercodegen_complete parsercodegen_complete.c
gcc -O2 -std=c11 -o vm vm.c
```

### Full Pipeline

```
./lex input.txt
./parsercodegen_complete
./vm elf.txt
```

---

## Repository Contents

- lex.c  
- parsercodegen_complete.c  
- vm.c  
- Example PL0 programs  
- README.md  


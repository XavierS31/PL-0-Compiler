/*
Assignment:
HW4 - Complete Parser and Code Generator for PL/0
(with Procedures, Call, and Else)
Author(s): <Gregory Berzinski>, <Xavier Soto>
Language: C (only)
To Compile:
Scanner:
gcc -O2 -std=c11 -o lex lex.c
Parser/Code Generator:
gcc -O2 -std=c11 -o parsercodegen_complete parsercodegen_complete.c
Virtual Machine:
gcc -O2 -std=c11 -o vm vm.c
To Execute (on Eustis):
./lex <input_file.txt>
./parsercodegen_complete
./vm elf.txt
where:
<input_file.txt> is the path to the PL/0 source program
Notes:
- lex.c accepts ONE command-line argument (input PL/0 source file)
- parsercodegen_complete.c accepts NO command-line arguments
- Input filename is hard-coded in parsercodegen_complete.c
- Implements recursive-descent parser for extended PL/0 grammar
- Supports procedures, call statements, and if-then-else
- Generates PM/0 assembly code (see Appendix A for ISA)
- VM must support EVEN instruction (OPR 0 11)
- All development and testing performed on Eustis
Class: COP3402 - System Software - Fall 2025
Instructor: Dr. Jie Lin
Due Date: Friday, November 21, 2025 at 11:59 PM ET
*/
#include <stdio.h>
#include <stdlib.h>
// Define fixed size
#define MAX_PAS 500
// Process Address Space
int pas[MAX_PAS];
// Global operation Names
char *operationNames[] = {
    "Invalid Operation", // 0 (invalid)
    "LIT",               // 1
    "OPR",               // 2
    "LOD",               // 3
    "STO",               // 4
    "CAL",               // 5
    "INC",               // 6
    "JMP",               // 7
    "JPC",               // 8
    "SYS"                // 9
};
// Registers
int PC, BP, SP;
// highest stack address to print (fixed after init)
int STACK_TOP;
struct
{
    int OP, L, M;
} IR;
// Helper base function to follow static links
int base(int BP, int L)
{
    int arb = BP;
    while (L > 0)
    {
        arb = pas[arb]; // follow static link
        L--;
    }
    return arb;
}
// Helper: print trace of stack with AR separator
void printStack()
{
    // walks from highest stack address to lowest address
    for (int i = STACK_TOP; i >= SP; i--)
    {
        // put '|' before AR (after first cycle)
        if (i == BP && i != SP && BP != STACK_TOP)
            printf("| ");
        printf("%2d ", pas[i]);
    }
    printf("\n");
}
int main(int argc, char *argv[])
{
    // Handle the Command Line
    if (argc != 2)
    {
        printf("Error: expected 1 argument (input file)\n");
        return 1;
    }
    // open the file passed on the command line
    FILE *in = fopen(argv[1], "r");
    if (!in)
    {
        printf("Error: cannot open input file\n");
        return 1;
    }
    // Initialize the pas[] values to 0
    for (int i = 0; i < MAX_PAS; i++)
    {
        pas[i] = 0;
    }
    // Load code into PAS from address 499 downward
    int op, l, m;
    int addr = MAX_PAS - 1;
    while (fscanf(in, "%d %d %d", &op, &l, &m) == 3)
    {
        pas[addr--] = op;
        pas[addr--] = l;
        pas[addr--] = m;
    }
    fclose(in);
    // Initialize Registers
    PC = MAX_PAS - 1; // first OP is at 499
    SP = addr + 1;    // first free cell below code
    BP = SP - 1;
    STACK_TOP = SP - 1; // stack initially empty; establish top boundary for printing
        // Print header
        printf(" L M PC BP SP stack\n");
    // Print initial state
    printf("Initial values : %d %d %d\n", PC, BP, SP);
    // Fetch-Execute Loop
    int halt = 0;
    while (!halt)
    {
        // Fetch
        IR.OP = pas[PC];
        IR.L = pas[PC - 1];
        IR.M = pas[PC - 2];
        PC = PC - 3;
        // Execute for Operations of PM/0 (1-9)
        switch (IR.OP)
        {
        // LIT (1)
        case 1:
            /*
            LIT: push literal
            sp <- sp - 1
            pas[sp] <- M
            */
            SP = SP - 1;
            pas[SP] = IR.M;
            break;
        // OPR (2)
        case 2:
            // Arithmetic and relational operations
            switch (IR.M)
            {
            // RTN (M = 0)
            case 0:
                // Return from subroutine and restore caller's AR
                SP = BP + 1;
                BP = pas[SP - 2];
                PC = pas[SP - 3];
                break;
            // ADD (M = 1)
            case 1:
                pas[SP + 1] = pas[SP + 1] + pas[SP];
                SP = SP + 1;
                break;
            // SUB (M = 2)
            case 2:
                pas[SP + 1] = pas[SP + 1] - pas[SP];
                SP = SP + 1;
                break;
            // MUL (M = 3)
            case 3:
                pas[SP + 1] = pas[SP + 1] * pas[SP];
                SP = SP + 1;
                break;
            // DIV (M = 4)
            case 4:
                pas[SP + 1] = pas[SP + 1] / pas[SP];
                SP = SP + 1;
                break;
            // EQL (M = 5)
            case 5:
                pas[SP + 1] = (pas[SP + 1] == pas[SP]);
                SP = SP + 1;
                break;
            // NEQ (M = 6)
            case 6:
                pas[SP + 1] = (pas[SP + 1] != pas[SP]);
                SP = SP + 1;
                break;
            // LSS (M = 7)
            case 7:
                pas[SP + 1] = (pas[SP + 1] < pas[SP]);
                SP = SP + 1;
                break;
            // LEQ (M = 8)
            case 8:
                pas[SP + 1] = (pas[SP + 1] <= pas[SP]);
                SP = SP + 1;
                break;
            // GTR (M = 9)
            case 9:
                pas[SP + 1] = (pas[SP + 1] > pas[SP]);
                SP = SP + 1;
                break;
            // GEQ (M = 10)
            case 10:
                pas[SP + 1] = (pas[SP + 1] >= pas[SP]);
                SP = SP + 1;
                break;
            // EVEN (M = 11) ------------- HW4 ADDITION -------------
            case 11:
                /*
                EVEN: unary test on top of stack
                pas[sp] <- (pas[sp] % 2 == 0)
                sp unchanged
                */
                pas[SP] = (pas[SP] % 2 == 0);
                break;
            // --------------------------- END HW4 ADDITION ---------
            default:
                printf("Invalid M input\n");
                break;
            }
            break;
        // LOD (3)
        case 3:
            /*
            Load value to top of stack from offset M
            in the AR L static levels down:
            sp <- sp - 1
            pas[sp] <- pas[base(bp,L) - M]
            */
            SP = SP - 1;
            pas[SP] = pas[base(BP, IR.L) - IR.M];
            break;
        // STO (4)
        case 4:
            /*
            Store top of stack into offset M
            in the AR L static levels down:
            pas[base(bp,L) - M] <- pas[sp]
            sp <- sp + 1
            */
            pas[base(BP, IR.L) - IR.M] = pas[SP];
            SP = SP + 1;
            break;
        // CAL (5)
        case 5:
            /*
            Call procedure at code address M; create activation record.
            pas[sp-1] <- base(bp,L) (static link)
            pas[sp-2] <- bp (dynamic link)
            pas[sp-3] <- pc (return address)
            bp <- sp - 1
            pc <- mapped address of M
            */
            pas[SP - 1] = base(BP, IR.L); // static link
            pas[SP - 2] = BP;             // dynamic link
            pas[SP - 3] = PC;             // return address
            BP = SP - 1;
            PC = (MAX_PAS - 1) - IR.M; // map IR.M (word offset) to op address 
        break;
        // INC (6)
        case 6:
            /*
            Allocate M locals on the stack:
            sp <- sp - M
            */
            SP = SP - IR.M;
            break;
        // JMP (7)
        case 7:
            /*
            Unconditional jump:
            pc <- mapped address of M
            */
            PC = (MAX_PAS - 1) - IR.M;
            break;
        // JPC (8)
        case 8:
            /*
            Conditional jump:
            if pas[sp] == 0 then pc <- mapped address of M
            sp <- sp + 1
            */
            if (pas[SP] == 0)
            {
                PC = (MAX_PAS - 1) - IR.M;
            }
            SP = SP + 1;
            break;
        // SYS (9)
        case 9:
            if (IR.M == 1)
            {
                /*
                Output integer value at top of stack; then pop.
                */
                printf("Output result is : %d\n", pas[SP]);
                SP = SP + 1;
            }
            else if (IR.M == 2)
            {
                /*
                Read an integer from stdin and push it
                */
                printf("Please Enter an Integer : ");
                fflush(stdout);
                SP = SP - 1;
                if (scanf("%d", &pas[SP]) != 1)
                {
                    printf("Error: invalid input\n");
                    halt = 1;
                }
            }
            else if (IR.M == 3)
            {
                // Halt the program
                halt = 1;
            }
            else
            {
                printf("Invalid SYS M: %d\n", IR.M);
            }
            break;
        default:
            printf("Error: invalid opcode %d\n", IR.OP);
            halt = 1;
        }
        // Print trace after executing instruction
        const char *mn = operationNames[IR.OP];
        if (IR.OP == 2)
        {
            switch (IR.M)
            {
            case 0:
                mn = "RTN";
                break;
            case 1:
                mn = "ADD";
                break;
            case 2:
                mn = "SUB";
                break;
            case 3:
                mn = "MUL";
                break;
            case 4:
                mn = "DIV";
                break;
            case 5:
                mn = "EQL";
                break;
            case 6:
                mn = "NEQ";
                break;
            case 7:
                mn = "LSS";
                break;
            case 8:
                mn = "LEQ";
                break;
            case 9:
                mn = "GTR";
                break;
            case 10:
                mn = "GEQ";
                break;
            case 11:
                mn = "EVEN";
                break; // HW4: mnemonic for OPR 11
            default:
                mn = "OPR";
                break;
            }
        }
        // Print each operation with formatting for L, M, PC, BP & SP
        printf("%-7s %3d %9d %5d %5d %5d ", mn, IR.L, IR.M, PC, BP, SP);
        printStack();
    }
    return 0;
}
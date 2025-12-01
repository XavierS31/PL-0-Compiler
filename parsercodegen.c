// /*
// Assignment:
// HW3 - Parser and Code Generator for PL/0
// Author(s): <Xavier Soto>, <Gregory Berzinski> 
// Language: C (only)
// To Compile:
// Scanner:
// gcc -O2 -std=c11 -o lex lex.c
// Parser/Code Generator:
// gcc -O2 -std=c11 -o parsercodegen parsercodegen.c
// To Execute (on Eustis):
// ./lex <input_file.txt>
// ./parsercodegen
// where:
// <input_file.txt> is the path to the PL/0 source program
// Notes:\
// - lex.c accepts ONE command-line argument (input PL/0 source file)
// - parsercodegen.c accepts NO command-line arguments
// - Input filename is hard-coded in parsercodegen.c
// - Implements recursive-descent parser for PL/0 grammar
// - Generates PM/0 assembly code (see Appendix A for ISA)
// - All development and testing performed on Eustis
// Class: COP3402 - System Software - Fall 2025
// Instructor: Dr. Jie Lin
// Due Date: Friday, October 31, 2025 at 11:59 PM ET
// */

//Standard Headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Token IDs defined as global constants
#define errorsym       0   // invalid to print (not used for skip detection)
#define skipsym        1   // skip / ignore token (presence => scanning error)
#define identsym       2   // identifier
#define numbersym      3   // number
#define plussym        4   // +
#define minussym       5   // -
#define multsym        6   // *
#define slashsym       7   // /
#define eqsym          8   // =
#define neqsym         9   // <>
#define lessym         10  // <
#define leqsym         11  // <=
#define gtrsym         12  // >
#define geqsym         13  // >=
#define lparentsym     14  // (
#define rparentsym     15  // )
#define commasym       16  // ,
#define semicolonsym   17  // ;
#define periodsym      18  // .
#define becomessym     19  // :=
#define beginsym       20  // begin
#define endsym         21  // end
#define ifsym          22  // if
#define fisym          23  // fi
#define thensym        24  // then
#define whilesym       25  // while
#define dosym          26  // do
#define callsym        27  // call (unused in HW3)
#define constsym       28  // const
#define varsym         29  // var
#define procsym        30  // procedure (unused in HW3)
#define writesym       31  // write
#define readsym        32  // read
#define elsesym        33  // else (unused in HW3)
#define evensym        34  // even (rare; many grammars use 'odd')
#define eofsym        -1   /* NEW: explicit end-of-file sentinel so '.' must be real */

//PM/0 Instruction + Code Buffer
typedef struct {
  int op; // opcode
  int l;  // level (always 0 in HW3)
  int m;  // modifier / address / immediate
} instruction;

#define MAX_CODE_LENGTH 1000
static instruction codebuf[MAX_CODE_LENGTH];
static int cx = 0; // instruction index

//OPcodes
#define OP_LIT 1
#define OP_OPR 2
#define OP_LOD 3
#define OP_STO 4
#define OP_CAL 5
#define OP_INC 6
#define OP_JMP 7
#define OP_JPC 8
#define OP_SYS 9

//Function to convert the instruction index to word address
static inline int WA(int instr_index) { return instr_index * 3; }

//OPR sub-opcodes
#define OPR_ADD  1
#define OPR_SUB  2
#define OPR_MUL  3
#define OPR_DIV  4
#define OPR_EQL  5
#define OPR_NEQ  6
#define OPR_LSS  7
#define OPR_LEQ  8
#define OPR_GTR  9
#define OPR_GEQ  10
#define OPR_ODD  11   // if you ever need ODD/EVEN checks

//Symbol Table
typedef struct {
  int  kind;      // 1 = const, 2 = var
  char name[12];  // identifier (<= 11 chars)
  int  val;       // const value
  int  level;     // always 0
  int  addr;      // var address (for LOD/STO)
  int  mark;      // 0 active, 1 marked
} symbol;

#define MAX_SYMBOL_TABLE_SIZE 500
static symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];
static int symCount = 0;

//Variable Address
static int nextVarAddr = 3;

//Token Buffer Read from tokens.txt
typedef struct {
  int  type;       // token type number
  char lexeme[64]; // text for ident/number
  int  hasLexeme;  // 1 if lexeme present
} TokenRec;

//Token Buffer Read from tokens.txt
#define MAX_TOKENS 10000
static TokenRec tokens[MAX_TOKENS];
static int tokCount = 0;
static int t = 0; // current token index

//helper function
static int currentToken(void) 
{
  return (t < tokCount) ? tokens[t].type : eofsym;
}
//helper function to get next characteer
static void advance(void) 
{ 
    if (t < tokCount) t++; 
}

//Emission + Output
static void emit(int op, int l, int m) 
{
  //If the code array overflows, return an error
  if (cx >= MAX_CODE_LENGTH) {
    printf("Error: code array overflow\n");
    exit(1);
  }
  //Add the opcode, level, and modifier to the code array
  codebuf[cx].op = op; codebuf[cx].l = l; codebuf[cx].m = m;
  cx++;
}

//Function to get the mnemonic of the opcode
static const char* op_mnemonic(int op) 
{
  switch (op) {
    case OP_LIT: return "LIT"; case OP_OPR: return "OPR"; case OP_LOD: return "LOD"; case OP_STO: return "STO";
    case OP_CAL: return "CAL"; case OP_INC: return "INC"; case OP_JMP: return "JMP"; case OP_JPC: return "JPC"; case OP_SYS: return "SYS";
    default: return "?";
  }
}

//Function to write the ELF file .txt
static void write_elf(void) {
  FILE *f = fopen("elf.txt", "w");

  //If the file cannot be opened, return an error
  if (!f) { printf("Error: could not open elf.txt for writing\n"); exit(1); }
  for (int i = 0; i < cx; i++) {
    fprintf(f, "%d %d %d\n", codebuf[i].op, codebuf[i].l, codebuf[i].m);
  }
  fclose(f);
}

//Function to print the code to the terminal
static void print_code_to_terminal(void) {
  printf("Assembly Code:\n\n");
  printf("Line    OP   L   M\n");
  for (int i = 0; i < cx; i++) {
    printf("%3d %6s %3d %3d\n", i, op_mnemonic(codebuf[i].op), codebuf[i].l, codebuf[i].m);
  }

  printf("\n");

  //Print the symbol table to the terminal
  printf("Symbol Table:\n\n");
  printf("Kind | Name       | Value | Level | Address | Mark\n");
  printf("-----+------------+-------+-------+---------+-----\n");
  for (int i = 0; i < symCount; i++) {
    printf("%4d | %10s | %5d | %5d | %7d | %4d\n", symbol_table[i].kind, symbol_table[i].name, symbol_table[i].val, symbol_table[i].level, symbol_table[i].addr, symbol_table[i].mark);
  }
  printf("\n");
}

//Error Handling
static void fatal_error(const char *msg) {
  printf("Error: %s\n", msg);

  FILE *f = fopen("elf.txt", "w");
    if (f) 
    {
        fprintf(f, "Error: %s\n", msg); fclose(f); 
    }
  exit(1);
}

//Error Handling Functions
static void scanning_error(void) { fatal_error("Scanning error detected by lexer (skipsym present)"); }
static void err_program_period(void){ fatal_error("program must end with period"); }
static void err_id_after_kw(void){ fatal_error("const, var, and read keywords must be followed by identifier"); }
static void err_symbol_redecl(void){ fatal_error("symbol name has already been declared"); }
static void err_const_assign_eq(void){ fatal_error("constants must be assigned with ="); }
static void err_const_int_value(void){ fatal_error("constants must be assigned an integer value"); }
static void err_decl_end_semicolon(void){ fatal_error("constant and variable declarations must be followed by a semicolon"); }
static void err_undeclared_ident(void){ fatal_error("undeclared identifier"); }
static void err_only_var_assign(void){ fatal_error("only variable values may be altered"); }
static void err_assign_becomes(void){ fatal_error("assignment statements must use :="); }
static void err_begin_end(void){ fatal_error("begin must be followed by end"); }
static void err_if_then(void){ fatal_error("if must be followed by then"); }
static void err_while_do(void){ fatal_error("while must be followed by do"); }
static void err_condition_relop(void){ fatal_error("condition must contain comparison operator"); }
static void err_rparen_after_lparen(void){ fatal_error("right parenthesis must follow left parenthesis"); }
static void err_arith_missing(void){ fatal_error("arithmetic equations must contain operands, parentheses, numbers, or symbols"); }

//Function to find the symbol in the symbol table
static int findSymbol(const char *name) {
  for (int i = symCount - 1; i >= 0; i--) {
    if (!symbol_table[i].mark && strcmp(symbol_table[i].name, name) == 0) return i;
  }
  return -1;
}

//Function to add the constant to the symbol table
static void addConst(const char *name, int value) {
  if (symCount >= MAX_SYMBOL_TABLE_SIZE) fatal_error("symbol table overflow");
    symbol_table[symCount].kind = 1;
    strncpy(symbol_table[symCount].name, name, sizeof(symbol_table[symCount].name)-1);
    symbol_table[symCount].name[sizeof(symbol_table[symCount].name)-1] = '\0';
    symbol_table[symCount].val = value;
    symbol_table[symCount].level = 0;
    symbol_table[symCount].addr = 0;
    symbol_table[symCount].mark = 0;
    symCount++;
}

//Function to add the variable to the symbol table
static void addVar(const char *name, int addr) {
  if (symCount >= MAX_SYMBOL_TABLE_SIZE) fatal_error("symbol table overflow");
    symbol_table[symCount].kind = 2;
    strncpy(symbol_table[symCount].name, name, sizeof(symbol_table[symCount].name)-1);
    symbol_table[symCount].name[sizeof(symbol_table[symCount].name)-1] = '\0';
    symbol_table[symCount].val = 0;
    symbol_table[symCount].level = 0;
    symbol_table[symCount].addr = addr;
  symbol_table[symCount].mark = 0;
  symCount++;
}

//Functions
static void program(void);
static void block(void);
static void const_declaration(void);
static int  var_declaration(void);
static void statement(void);
static void condition(void);
static void expression(void);
static void term(void);
static void factor(void);

//small parser helpers for clarity 
static int accept(int ty) {
  if (currentToken() == ty) { advance(); return 1; }
  return 0;
}

//Function to expect the token
static void expect_tok(int ty, void (*onError)(void)) {
  if (!accept(ty)) onError();
}

//Function to check if the token is a relational operator
static int isRelOp(int ty) {
  return (ty==eqsym || ty==neqsym || ty==lessym || ty==leqsym || ty==gtrsym || ty==geqsym);
}

//Function to parse the program
static void program(void) {
  
  //Emit the JMP opcode to jump to the start of the program
  emit(OP_JMP, 0, 3);  /* JMP 0 3 */

  //Function to parse the block
  block();

  //If the current token is not periodsym, return an error
  expect_tok(periodsym, err_program_period);

  //Emit the SYS opcode to halt the program
  emit(OP_SYS, 0, 3);  

  
}

//Function to parse the block
static void block(void) {
  int startSym = symCount; // save current symbol count before new declarations

  const_declaration();
  int nvars = var_declaration();

  //Function to reserve stack space: 3 + nvars
  emit(OP_INC, 0, 3 + nvars);

  //Function to parse the statement
  statement();
  // Mark symbols only if this is a nested block (but HW3 has no nested blocks/procedures)
   for (int i = startSym; i < symCount; i++) {
    symbol_table[i].mark = 1;
 }
}

//Function to parse the const declaration
static void const_declaration(void) 
{
  //If the current token is not constsym, return
  if (currentToken() != constsym) return;

  //Loop to parse the const declaration
  do {
    //Advance the token
    advance(); 

    //If the current token is not identsym, return an error
    if (currentToken() != identsym) err_id_after_kw();

    //Copy the lexeme to the name array
    char name[64] = "";
    if (tokens[t].hasLexeme) strncpy(name, tokens[t].lexeme, sizeof(name)-1);

    if (findSymbol(name) != -1) err_symbol_redecl();
    advance();

    if (currentToken() != eqsym) err_const_assign_eq();
    advance();

    if (currentToken() != numbersym) err_const_int_value();
    int val = 0; if (tokens[t].hasLexeme) val = atoi(tokens[t].lexeme);
    addConst(name, val);
    advance();

    if (currentToken() != commasym) break;
    //Loop to parse the next const
  } while (currentToken() == commasym);

  //If the current token is not semicolonsym, return an error
  expect_tok(semicolonsym, err_decl_end_semicolon);
}

//Function to parse the var declaration
static int var_declaration(void) 
{
  int count = 0;
  if (currentToken() != varsym) return 0;

  //Loop to parse the var declaration
  do {
    advance(); /* consume 'var' or ',' */

    if (currentToken() != identsym) err_id_after_kw();

    char name[64] = "";
    if (tokens[t].hasLexeme) strncpy(name, tokens[t].lexeme, sizeof(name)-1);

    if (findSymbol(name) != -1) err_symbol_redecl();
    addVar(name, nextVarAddr++);
    count++;

    advance();

    if (currentToken() != commasym) break;
  } while (currentToken() == commasym);

  //If the current token is not semicolonsym, return an error
  expect_tok(semicolonsym, err_decl_end_semicolon);
  return count;
}

//Function to emit the JPC opcode to jump to the next instruction
static inline int emit_jpc_placeholder(void) 
{ 
  //Emit the JPC opcode to jump to the next instruction
  emit(OP_JPC, 0, 0);
  return cx - 1;
}

//Function to set the target instruction index
static inline void set_target(int instr_index, int target_instr_index) {
  codebuf[instr_index].m = WA(target_instr_index);
}

//Function to emit the JMP opcode to jump to the target instruction index
static inline void emit_jmp_to(int target_instr_index) {
  emit(OP_JMP, 0, WA(target_instr_index));
}

//Function to parse the statement
static void statement(void) 
{
  int ty = currentToken();

  //If the current token is identsym, parse the assignment and return
  if (ty == identsym) 
  {
    //Copy the lexeme to the name array
    char name[64] = "";
    if (tokens[t].hasLexeme) strncpy(name, tokens[t].lexeme, sizeof(name)-1);
    int idx = findSymbol(name);
    if (idx == -1) err_undeclared_ident();
    if (symbol_table[idx].kind != 2) err_only_var_assign();

    //Advance the token
    advance();

    //If the current token is not becomessym, return an error
    if (currentToken() != becomessym) err_assign_becomes();

    //Advance the token
    advance();

    //Parse the expression
    expression();

    //Emit the STO opcode to store the value
    emit(OP_STO, 0, symbol_table[idx].addr); 
    return;
  }

  //If the current token is beginsym, parse the statement and return
  if (ty == beginsym) 
  {
    //Advance the token
    advance();
    //Parse the statement
    statement();
    //Loop to parse the next statement
    while (accept(semicolonsym)) {
      statement();
    }
    expect_tok(endsym, err_begin_end);
    return;
  }

  //If the current token is ifsym, parse the condition and return
  if (ty == ifsym) 
  {
    advance();

    condition();

    int jpcIdx = emit_jpc_placeholder();  /* JPC 0 ? */

    expect_tok(thensym, err_if_then);
    statement();

    set_target(jpcIdx, cx);               /* backpatch to next instr */

    if (accept(fisym)) { /* optional fi */ }
    return;
  }

  //If the current token is whilesym, parse the condition and return
  if (ty == whilesym) 
  {
    advance();

    int loopStart = cx;
    condition();

    expect_tok(dosym, err_while_do);

    int jpcIdx = emit_jpc_placeholder();
    statement();
    emit_jmp_to(loopStart);
    set_target(jpcIdx, cx);
    return;
  }

  //If the current token is readsym, parse the identifier and return
  if (ty == readsym) 
  {
    advance();
    if (currentToken() != identsym) err_id_after_kw();

    char name[64] = "";
    if (tokens[t].hasLexeme) strncpy(name, tokens[t].lexeme, sizeof(name)-1);
    int idx = findSymbol(name);
    if (idx == -1) err_undeclared_ident();
    if (symbol_table[idx].kind != 2) err_only_var_assign();

    emit(OP_SYS, 0, 2);                         /* read int */
    emit(OP_STO, 0, symbol_table[idx].addr);    /* store */
    advance();
    return;
  }

  if (ty == writesym) 
  {
    advance();
    expression();
    emit(OP_SYS, 0, 1);                         /* write */
    return;
  }

  /* epsilon/empty statement allowed */
}

//Function to parse the condition
static void condition(void) {

  //If the current token is evensym, parse the expression and return
  if (currentToken() == evensym) 
  {
    advance();
    expression();
    emit(OP_OPR, 0, OPR_ODD);  //odd
    return;
  }

  expression();
  int rel = currentToken();

  //If the current token is not a relational operator, return an error
  if (!isRelOp(rel)) err_condition_relop();
  advance();
  expression();

  int m = 0;

  //Switch statement to determine the operation to perform
  switch (rel) {
    case eqsym:  m = OPR_EQL; break;
    case neqsym: m = OPR_NEQ; break;
    case lessym: m = OPR_LSS; break;
    case leqsym: m = OPR_LEQ; break;
    case gtrsym: m = OPR_GTR; break;
    case geqsym: m = OPR_GEQ; break;
    default: err_condition_relop();
  }
  emit(OP_OPR, 0, m);
}

//Function to parse the expression
static void expression(void) {

  int leading = currentToken();

  //If the current token is minussym, parse the expression and return
  if (leading == minussym) 
  {
    advance();                 /* consume '-' */
    emit(OP_LIT, 0, 0);        /* push 0 first */
    term();                    /* parse the value */
    emit(OP_OPR, 0, OPR_SUB);  /* 0 - value */
  }
  else 
  {

    if (leading == plussym) advance();  /* optional '+' */
    term();
  }

  //If the current token is plussym or minussym, parse the expression and return
  while (currentToken() == plussym || currentToken() == minussym) 
  {
    int op = currentToken(); advance();
    term();
    emit(OP_OPR, 0, (op==plussym) ? OPR_ADD : OPR_SUB);
  }
}

//Function to parse the term
static void term(void) {

  factor();

  //If the current token is multsym or slashsym, parse the term and return
  while (currentToken() == multsym || currentToken() == slashsym) {
    int op = currentToken(); advance();
    factor();
    emit(OP_OPR, 0, (op==multsym) ? OPR_MUL : OPR_DIV);
  }
}

//Function to parse the factor
static void factor(void) 
{
  int ty = currentToken();

  //If the current token is identsym, parse the identifier and return
  if (ty == identsym) {
    char name[64] = "";
    if (tokens[t].hasLexeme) strncpy(name, tokens[t].lexeme, sizeof(name)-1);
    int idx = findSymbol(name);
    if (idx == -1) err_undeclared_ident();

    if (symbol_table[idx].kind == 1) {
      emit(OP_LIT, 0, symbol_table[idx].val);         /* const → LIT */
    } else {
      emit(OP_LOD, 0, symbol_table[idx].addr);        /* var   → LOD */
    }
    advance();
    return;
  }

  //If the current token is numbersym, parse the value and return
  if (ty == numbersym) {
    int val = 0; if (tokens[t].hasLexeme) val = atoi(tokens[t].lexeme);
    emit(OP_LIT, 0, val);                             /* LIT value */
    advance();
    return;
  }

  //If the current token is lparentsym, parse the expression and return
  if (ty == lparentsym) {
    advance();
    expression();
    expect_tok(rparentsym, err_rparen_after_lparen);
    return;
  }

  err_arith_missing();
}


//Token File Loader
static void load_tokens_or_die(void) 
{
  /*
  Opens the Hardcoded tokens.txt file.
  The file tokens.txt has the token list generated by the lexer.
  The tokens.txt file must be the tokens in a single line separated by spaces.
  It doesnt include the output format from homework 2, just the token list.
  Our lex.c file outputs with lexeme table and then token list.
  Input for parsercodegen.c must be the token list only.
   */
  FILE *fp = fopen("tokens.txt", "r");
  if (!fp) { printf("Error: tokens.txt not found.\n"); exit(1); }

  //Set the token count to 0 as we start loading the tokens
  tokCount = 0;

  //Loop to load the tokens
  while (tokCount < MAX_TOKENS) 
  {
    int ty;
    //If the end of the tokens is not found, break the loop
    if (fscanf(fp, "%d", &ty) != 1) break; 

    //Set the token type
    tokens[tokCount].type = ty;
    //Set the token hasLexeme to 0
    tokens[tokCount].hasLexeme = 0;
    //Set the token lexeme to an empty string
    tokens[tokCount].lexeme[0] = '\0';

    //If the token type is identsym or numbersym, set the token hasLexeme to 1
    if (ty == identsym || ty == numbersym) {
      //If the token lexeme is found, set the token hasLexeme to 1
      if (fscanf(fp, "%s", tokens[tokCount].lexeme) == 1) {
        tokens[tokCount].hasLexeme = 1;
      }
    }
    //Increment the token count
    tokCount++;
  }
  //Close the file
  fclose(fp);
}

//helper function for skipsym
static int contains_skipsym(void) 
{
  //Loop to check if the tokens contain skipsym
  for (int i = 0; i < tokCount; i++) {
    //If the token type is skipsym, return 1
    if (tokens[i].type == skipsym) return 1;
    //If the token type is not skipsym, return 0
  }

  //If the tokens do not contain skipsym, return 0
  return 0;
}

//Main
int main(void) 
{
  //Function to load the tokens
  load_tokens_or_die();

  //If the lexer output contains skipsym (1), stop immediately 
  if (contains_skipsym()) 
  {
    scanning_error();
  }

  //Function to parse the program
  program();

  //Function to write the ELF file .txt
  write_elf();

  //Print Function to the terminal
  print_code_to_terminal();
  return 0;
}

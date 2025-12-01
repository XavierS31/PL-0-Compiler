/*
Assignment :
lex - Lexical Analyzer for PL /0
Author : Xavier Soto and Gregory Berzinski
Language : C ( only )
To Compile :
gcc - O2 - std = c11 -o lex lex . c
To Execute ( on Eustis ):
./ lex < input file >
where :
< input file > is the path to the PL /0 source program
Notes :
- Implement a lexical analyser for the PL /0 language .
- The program must detect errors such as
- numbers longer than five digits
- identifiers longer than eleven characters
- invalid characters .
- The output format must exactly match the specification.
- Tested on Eustis .
Class : COP 3402 - System Software - Fall 2025
Instructor : Dr . Jie Lin
Due Date : Friday , October 3 , 2025 at 11:59 PM ET
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//Token Type Enumeration in C
typedef enum {
    errorsym = 0, //holds the invalid to print
    skipsym = 1 , // Skip / ignore token
    identsym , // Identifier
    numbersym , // Number
    plussym , // +
    minussym , // -
    multsym , // *
    slashsym , // /
    eqsym , // =
    neqsym , // <>
    lessym , // <
    leqsym , // <=
    gtrsym , // >
    geqsym , // >=
    lparentsym , // (
    rparentsym , // )
    commasym , // ,
    semicolonsym , // ;
    periodsym , // .
    becomessym , // :=
    beginsym , // begin
    endsym , // end
    ifsym , // if
    fisym , // fi
    thensym , // then
    whilesym , // while
    dosym , // do
    callsym , // call
    constsym , // const
    varsym , // var
    procsym , // procedure
    writesym , // write
    readsym , // read
    elsesym , // else
    evensym, // even
} TokenType ;

//Global Table / Reserved Words
const char *reservedWords[] = {
    "begin","end","if","fi","then","while","do","call",
    "const","var","procedure","write","read","else","even"
};

//error messages
const char *errorMessage[] = {
    //0, 1, 2, 3 for struct error variable
    "Identifier too long","Number too long", "Invalid Symbol", "Unclosed comment"
};


//Global Reservedtokens
const TokenType reservedTokens[] = {
    beginsym, endsym, ifsym, fisym, thensym, whilesym, dosym, callsym,
    constsym, varsym, procsym, writesym, readsym, elsesym, evensym,
};

//global variables
#define MAX_IDENT_LENGTH 11 //Identifier must be max 11 words
#define MAX_NUM_LENGTH 5 //Variable must not exceed the Then Thousand mark (5 Digits)


//Struct for saving each token
typedef struct {
    TokenType type;
    char lexeme[64];   // for identifier or number string
    int errors;
} Token;

//Functions
int isReservedWord(const char *word);
Token getNextToken(FILE *fp);
void printSource(FILE *fp);
void printLexemeTable(Token tokens[], int count);
void printTokenList(Token tokens[], int count);

//Main
int main(int argc, char *argv[]) {

    //Checks for proper arguments when running in terminal
    if (argc != 2) {
        printf("Usage: ./lex <input file>\n");
        return 1;
    }

    //Reading file.txt
    FILE *fp = fopen(argv[1], "r");

    //Check if file exists.
    if (!fp) {

        //Error message for file not found
        perror("File open failed");
        return 1;
    }

    //Print Source Program from File
    //printf("Source Program:\n\n");
    //printSource(fp);

    // rewind file to scan again
    rewind(fp);

    //Tokenize
    Token tokens[1000]; //Array to store tokens
    int count = 0;

    //Loop to start getting tokens
    Token t;
    while (1) {
        t = getNextToken(fp);
        
        // Stop if EOF reached
        if (feof(fp) && t.type == skipsym && strlen(t.lexeme) == 0) break;

        // skip whitespace/comments
        if (t.type == skipsym) continue; 

        // nothing valid
        if (strlen(t.lexeme) == 0) continue; 
        
        //Save the Token to array of tokens 
        tokens[count++] = t;
    }

    //Call Function to Print Lexeme Table
    //printLexemeTable(tokens, count);

    //Call Function to print the Token List
    printTokenList(tokens, count);


    //Close File 
    fclose(fp);
    return 0;
}

//Function that takes in the word and checks if its reserved
int isReservedWord(const char *word) {

    //Loops through the global array of reserved words 
    for (int i = 0; i < sizeof(reservedWords)/sizeof(reservedWords[0]); i++) {

        //Checks if word is used
        if (strcmp(word, reservedWords[i]) == 0) {

            //IF reserved, return its token
            return reservedTokens[i];  
        }
    }

    //return -1 as word not reserved
    return -1; 
}

//Function that classifies each input as token
Token getNextToken(FILE *fp) {

    //Initialize variables
    Token t;
    int c;

    //Skip whitespace and comments
    while ((c = fgetc(fp)) != EOF) {
        if (isspace(c)) continue;  // skip spaces, tabs, newlines

        // Comment handling: /* ... */
        if (c == '/') {
            int next = fgetc(fp);
            if (next == '*') {
                int prev = 0;
                while ((c = fgetc(fp)) != EOF) {
                    if (prev == '*' && c == '/') break; // end of comment
                    prev = c;
                }
                // Check if comment was closed before EOF
                if (c == EOF) {
                    // Unclosed comment detected
                    t.type = errorsym;
                    strcpy(t.lexeme, "/*");
                    t.errors = 3; // Unclosed comment
                    return t;
                }
                continue; // back to outer loop
            } else {
                if (next != EOF) ungetc(next, fp); // not a comment
                t.type = slashsym;
                strcpy(t.lexeme, "/");
                return t;
            }
        }
        break; // found non-whitespace, non-comment
    }

    // End of file
    if (c == EOF) {
        t.type = skipsym;
        strcpy(t.lexeme, "");
        return t;
    }

    //Identifiers or Reserved Words
    if (isalpha(c)) {
        char buffer[64];
        int len = 0;
        buffer[len++] = c;

        while ((c = fgetc(fp)) != EOF && isalnum(c) && len < 63) {
            buffer[len++] = c;
        }

        //account for null
        buffer[len] = '\0';
        if (c != EOF) ungetc(c, fp);

        //see if its reserved word
        int reserved = isReservedWord(buffer);

        //Check if its reserved
        if (reserved != -1) {
            t.type = reserved;
        } else {
            if (len > MAX_IDENT_LENGTH) {
                t.type = errorsym;
                strcpy(t.lexeme, buffer);
                t.errors = 0;
                return t;
            }
            t.type = identsym;
        }
        strcpy(t.lexeme, buffer);
        return t;
    }

    //Numbers
    if (isdigit(c)) {
        char buffer[64];
        int len = 0;
        buffer[len++] = c;

        while ((c = fgetc(fp)) != EOF && isdigit(c) && len < 63) {
             buffer[len++] = c;
        }

        //save null
        buffer[len] = '\0';
        if (c != EOF) ungetc(c, fp);

        //check if its valid
        if (len > MAX_NUM_LENGTH) {
            t.type = errorsym;
            strcpy(t.lexeme, buffer);
            t.errors = 1;
        } else {
            t.type = numbersym;
            strcpy(t.lexeme, buffer);
        }
        return t;
    }

    //Handle Special Symbols
    switch (c) {
        case '+': t.type = plussym; strcpy(t.lexeme, "+"); return t; //+
        case '-': t.type = minussym; strcpy(t.lexeme, "-"); return t; //-
        case '*': t.type = multsym; strcpy(t.lexeme, "*"); return t; //*
        case '=': t.type = eqsym; strcpy(t.lexeme, "="); return t; // =
        case ',': t.type = commasym; strcpy(t.lexeme, ","); return t; // ,
        case ';': t.type = semicolonsym; strcpy(t.lexeme, ";"); return t; // ;
        case '.': t.type = periodsym; strcpy(t.lexeme, "."); return t; // .
        case '(': t.type = lparentsym; strcpy(t.lexeme, "("); return t; // ()
        case ')': t.type = rparentsym; strcpy(t.lexeme, ")"); return t; // )

        //Check for :=
        //First start with : 
        //Then Check the =
        case ':': {
            int next = fgetc(fp);
            if (next == '=') {
                t.type = becomessym;
                strcpy(t.lexeme, ":=");
            } else {
                if (next != EOF) ungetc(next, fp);
                t.type = errorsym;
                t.lexeme[0] = ':';    // save the offending character
                t.lexeme[1] = '\0';
                t.errors = 2; 
            }
            return t;
        }

        //Account for <= , <> , or simply <
        case '<': {

            int next = fgetc(fp);

            if (next == '=') {
                t.type = leqsym; strcpy(t.lexeme, "<=");
            } else if (next == '>') {
                t.type = neqsym; strcpy(t.lexeme, "<>");
            } else {
                if (next != EOF) ungetc(next, fp);
                t.type = lessym; strcpy(t.lexeme, "<");
            }
            return t;
        }

        //Account for >= or simply <
        case '>': {
            int next = fgetc(fp);

            if (next == '=') {
                t.type = geqsym; strcpy(t.lexeme, ">=");
            } else {
                if (next != EOF) ungetc(next, fp);
                t.type = gtrsym; strcpy(t.lexeme, ">");
            }
            return t;
        }
    }

    //Invalid symbol
    // Invalid symbol (anything else not matched above)
    t.type = errorsym;
    t.lexeme[0] = (char)c;   // save the actual bad character
    t.lexeme[1] = '\0';      // null terminator
    t.errors = 2;            //Invalid Symbol

return t;

}



//function to print the File Source Program
void printSource(FILE *fp) {
    int c;
    while ((c = fgetc(fp)) != EOF) {
       putchar(c);
    }
    
}

//Function that Prints the Lexeme Table
//void printLexemeTable(Token tokens[], int count) {

    //Lexeme Table Header
    //printf("\n\nLexeme Table:\n\n");

    //Lexeme and Token
    //printf("lexeme    token type\n");

    //Loop to print each token's Lexeme and Token Type #
    /*for (int i = 0; i < count; i++) {

        //check errors
        if (tokens[i].type == errorsym){
            //printf("%-9s %s\n", tokens[i].lexeme, errorMessage[tokens[i].errors]);    

        } else{
            //printf("%-9s %d\n", tokens[i].lexeme, tokens[i].type);
        }
    }
}*/   

//Function that prints the Token List
void printTokenList(Token tokens[], int count) {


    //Header
    //printf("\nToken List:\n\n");


    for (int i = 0; i < count; i++) {

        if(tokens[i].type == errorsym){
            tokens[i].type ++;
        }
        printf("%d ", tokens[i].type);
        
        //If var or identifier, print it
        if(((tokens[i].type == 2) || (tokens[i].type == 3)) && !(tokens[i].type == errorsym)){

            printf("%s ", tokens[i].lexeme);
        }

    


    }
    printf("\n");
}
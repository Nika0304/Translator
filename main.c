#include "utils.h"
#include "lexer.h"
#include <stdlib.h>
#include <stdio.h>
#include "parser.h"
#include "ad.h"

int main()
{
    char *buffer = loadFile("tests/testat.c"); // citeste fisierul
    Token *tokens = tokenize(buffer);          // face analiza lexicala
    showTokens(tokens);                        // afiseaza tokenii

    pushDomain();

    vmInit();          

    parse(tokens);
    showDomain(symTable, "global");

    Instr *testCode = genTestProgramDouble();
    run(testCode);

    dropDomain();

    printf("\nProgram corect sintactic\n");

    freeTokens(tokens);
    free(buffer);

    return 0;
}
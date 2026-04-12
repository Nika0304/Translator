#include "utils.h"
#include "lexer.h"
#include <stdlib.h>
#include <stdio.h>
#include "parser.h"
int main()
{
    char *buffer = loadFile("tests/testparser.c"); //citeste fisierul
    Token *tokens = tokenize(buffer); //face analiza lexicala
    showTokens(tokens); //afiseaza tokenii
    parse(tokens);
    printf("\nProgram corect sintactic\n");
    freeTokens(tokens); //Eliberează linked list de tokeni 
    free(buffer); //Eliberează buffer ul citit din fișier

    return 0;
}

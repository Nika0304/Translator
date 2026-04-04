#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"

Token *tokens;  // single linked list of tokens
Token *lastTk;      // the last token in list

int line=1;     // the current line in the input file

// adds a token to the end of the tokens list and returns it
// sets its code and line
Token *addTk(int code){
    Token *tk=safeAlloc(sizeof(Token));
    tk->code=code;
    tk->line=line;
    tk->next=NULL;
    if(lastTk){
        lastTk->next=tk;
        }else{
        tokens=tk;
        }
    lastTk=tk;
    return tk;
    }

char *extract(const char *begin,const char *end){
    int len = end - begin;
    char *str = safeAlloc(len + 1);
    strncpy(str, begin, len);
    str[len] = '\0';
    return str;
    }


//de completat functia 
Token *tokenize(const char *pch){
    const char *start;
    Token *tk;
    for(;;){
        switch(*pch){
            case ' ':case '\t':pch++;break;
            case '\r':      // handles different kinds of newlines (Windows: \r\n, Linux: \n, MacOS, OS X: \r or \n)
                if(pch[1]=='\n')pch++;
            case '\n':
                line++;
                pch++;
                break;
            case '\0':addTk(END);return tokens;
            //delimitatori
            case ',':addTk(COMMA); pch++; break;
            case ';':addTk(SEMICOLON); pch++;  break;
            case '(':addTk(LPAR); pch++;  break;
            case ')':addTk(RPAR); pch++;  break;
            case '[':addTk(LBRACKET); pch++;  break;
            case ']':addTk(RBRACKET); pch++;  break;
            case '{':addTk(LACC); pch++;  break;
            case '}':addTk(RACC); pch++;  break;

            case '+':addTk(ADD); pch++;  break;
            case '-':addTk(SUB); pch++;  break;
            case '*':addTk(MUL); pch++;  break;
            case '/':
                if(pch[1] == '/'){
                    pch += 2;
                    while(*pch != '\n' && *pch != '\r' && *pch != '\0') pch++;
                }else{
                    addTk(DIV);
                    pch++;
                }
                break;
            case '.':addTk(DOT); pch++;  break;
            case '=':
                if(pch[1]=='='){
                    addTk(EQUAL);
                    pch+=2;
                    }else{
                    addTk(ASSIGN);
                    pch++;
                    }
                break;
            case '&':
                if(pch[1]=='&'){
                    addTk(AND);
                    pch+=2;
                }else err("invalid char: &");
                break;
            case '|':
                if(pch[1]=='|'){
                    addTk(OR);
                    pch+=2;
                }else err("invalid char: |");
                break;
            case '!':
                if(pch[1]=='='){
                    addTk(NOTEQ);
                    pch+=2;
                }else{
                    addTk(NOT);
                    pch++;
                }
                break;
            case '<':
                if(pch[1]=='='){
                    addTk(LESSEQ);
                    pch+=2;
                }else{
                    addTk(LESS);
                    pch++;
                }
                break;
            case '>':
                if(pch[1]=='='){
                    addTk(GREATEREQ);
                    pch+=2;
                }else{
                    addTk(GREATER);
                    pch++;
                }
                break;
                //mai mult de 2 caractere
            default:
                if(isalpha(*pch)||*pch=='_'){
                    for(start=pch++;isalnum(*pch)||*pch=='_'; pch++) {}
                    char *text=extract(start,pch); 

                    if(strcmp(text,"char")==0) {addTk(TYPE_CHAR); free(text);}
                    else if(strcmp(text,"double")==0) {addTk(TYPE_DOUBLE); free(text);}
                    else if(strcmp(text,"else")==0) {addTk(ELSE); free(text);}
                    else if(strcmp(text,"if")==0) {addTk(IF); free(text);}
                    else if(strcmp(text,"int")==0) {addTk(TYPE_INT); free(text);}
                    else if(strcmp(text,"return")==0) {addTk(RETURN); free(text);}
                    else if(strcmp(text,"struct")==0) {addTk(STRUCT); free(text);}
                    else if(strcmp(text,"void")==0) {addTk(VOID); free(text);}
                    else if(strcmp(text,"while")==0) {addTk(WHILE); free(text);}
                    else{
                        tk=addTk(ID);
                        tk->text=text;
                        }
                    }

                //pentru INT si DOUBLE
                else if(isdigit(*pch))
                {
                    start = pch;
                    while(isdigit(*pch)) pch++;

                    if(*pch == '.')
                    {
                        pch++;
                        if(!isdigit(*pch)) err("invalid DOUBLE");
                        while(isdigit(*pch)) pch++;

                        if(*pch == 'e' || *pch == 'E')
                        {
                            pch++;
                            if (*pch == '+' || *pch == '-') pch++;
                            if(!isdigit(*pch)) err("invalid DOUBLE");
                            while(isdigit(*pch)) pch++;
                        }

                        char *text = extract(start, pch);
                        tk = addTk(DOUBLE);
                        tk->d = atof(text);
                        free(text);

                    }
                    else if(*pch == 'e' || *pch == 'E')
                    {
                        pch++;

                        if (*pch == '+' || *pch == '-') pch++;
                        if(!isdigit(*pch)) err("invalid DOUBLE");
                        while(isdigit(*pch)) pch++;

                        char *text = extract(start, pch);
                        tk = addTk(DOUBLE);
                        tk->d = atof(text);
                        free(text);

                    }
                    else
                    {
                        char *text = extract(start, pch);
                        tk = addTk(INT);
                        tk->i = atoi(text);
                        free(text);
                    }
                }

                else if(*pch == '\'')
                {
                    pch++;

                    char value;

                    if(*pch == '\\')
                    {
                        pch++;
                        switch(*pch)
                        {
                            case 'a': value = '\a'; break;
                            case 'b': value = '\b'; break;
                            case 'f': value = '\f'; break;
                            case 'n': value = '\n'; break;
                            case 'r': value = '\r'; break;
                            case 't': value = '\t'; break;
                            case 'v': value = '\v'; break;
                            case '\\': value = '\\'; break;
                            case '\'': value = '\''; break;
                            case '"': value = '\"'; break;
                            case '0': value = '\0'; break;
                            default: err("invalid CHAR");
                        }
                        pch++;
                    }
                    else
                    {
                        if(*pch == '\'' || *pch == '\\' || *pch == '\0' || *pch == '\n' || *pch == '\r'){
                            err("invalid CHAR");
                        }
                        value = *pch;
                        pch++;
                    }

                    if(*pch != '\''){
                        err("missing closing quote for CHAR");
                    }
                    pch++;

                    tk = addTk(CHAR);
                    tk->c = value;
                }


                else if(*pch == '"'){
                    pch++;

                    char *buf = safeAlloc(128);
                    int n = 0;

                    while(*pch != '"'){
                        if(*pch == '\0' || *pch == '\n' || *pch == '\r')
                            err("unterminated string");

                        if(*pch == '\\'){
                            pch++;
                            switch(*pch){
                                case 'a': buf[n++] = '\a'; break;
                                case 'b': buf[n++] = '\b'; break;
                                case 'f': buf[n++] = '\f'; break;
                                case 'n': buf[n++] = '\n'; break;
                                case 'r': buf[n++] = '\r'; break;
                                case 't': buf[n++] = '\t'; break;
                                case 'v': buf[n++] = '\v'; break;
                                case '\\': buf[n++] = '\\'; break;
                                case '\'': buf[n++] = '\''; break;
                                case '"': buf[n++] = '\"'; break;
                                case '0': buf[n++] = '\0'; break;
                                default: err("invalid escape in STRING");
                            }
                            pch++;
                        }
                        else{
                            buf[n++] = *pch;
                            pch++;
                        }
                    }

                    buf[n] = '\0';
                    pch++;

                    tk = addTk(STRING);
                    tk->text = buf;
                }
                else err("invalid char: %c (%d)",*pch,*pch);
            }
        }
    }

//de modificat ca sa obtinem ca in lista-de atomi.txt
void showTokens(const Token *tokens){
    for(const Token *tk = tokens; tk; tk = tk->next){
        printf("%d\t", tk->line);

        switch(tk->code){
            case ID: printf("ID:%s", tk->text); break;
            case TYPE_CHAR: printf("TYPE_CHAR"); break;
            case TYPE_DOUBLE: printf("TYPE_DOUBLE"); break;
            case ELSE: printf("ELSE"); break;
            case IF: printf("IF"); break;
            case TYPE_INT: printf("TYPE_INT"); break;
            case RETURN: printf("RETURN"); break;
            case STRUCT: printf("STRUCT"); break;
            case VOID: printf("VOID"); break;
            case WHILE: printf("WHILE"); break;
            case INT: printf("INT:%d", tk->i); break;
            case DOUBLE: printf("DOUBLE:%.2f", tk->d); break;
            case CHAR: printf("CHAR:%c", tk->c); break;
            case STRING: printf("STRING:%s", tk->text); break;
            case COMMA: printf("COMMA"); break;
            case SEMICOLON: printf("SEMICOLON"); break;
            case LPAR: printf("LPAR"); break;
            case RPAR: printf("RPAR"); break;
            case LBRACKET: printf("LBRACKET"); break;
            case RBRACKET: printf("RBRACKET"); break;
            case LACC: printf("LACC"); break;
            case RACC: printf("RACC"); break;
            case END: printf("END"); break;
            case ADD: printf("ADD"); break;
            case SUB: printf("SUB"); break;
            case MUL: printf("MUL"); break;
            case DIV: printf("DIV"); break;
            case DOT: printf("DOT"); break;
            case AND: printf("AND"); break;
            case OR: printf("OR"); break;
            case NOT: printf("NOT"); break;
            case ASSIGN: printf("ASSIGN"); break;
            case EQUAL: printf("EQUAL"); break;
            case NOTEQ: printf("NOTEQ"); break;
            case LESS: printf("LESS"); break;
            case LESSEQ: printf("LESSEQ"); break;
            case GREATER: printf("GREATER"); break;
            case GREATEREQ: printf("GREATEREQ"); break;
        }

        printf("\n");
    }
}

void freeTokens(Token *tokens){
    Token *tk = tokens;
    while(tk){
        Token *next = tk->next;
        if(tk->code == ID || tk->code == STRING){
            free(tk->text);
        }
        free(tk);
        tk = next;
    }
}
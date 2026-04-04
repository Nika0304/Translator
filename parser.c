#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

Token *iTk;		// the iterator in the tokens list
Token *consumedTk;		// the last consumed token

void tkerr(const char *fmt,...){
	fprintf(stderr,"error in line %d: ",iTk->line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
	}

bool consume(int code){
	if(iTk->code==code){
		consumedTk=iTk;
		iTk=iTk->next;
		return true;
		}
	return false;
	}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase(){
	Token *start = iTk; //memoram pozitia curenta, ca sa nu lasam tokeni consumati pe ramura de false

	if(consume(TYPE_INT)){
		return true;
		}
	if(consume(TYPE_DOUBLE)){
		return true;
		}
	if(consume(TYPE_CHAR)){
		return true;
		}
	if(consume(STRUCT)){
		if(consume(ID)){
			return true;
			}
		tkerr("lipseste numele structurii");
		}
	iTk = start;
	return false;
	}

// unit: ( structDef | fnDef | varDef )* END
bool unit()
{
	for(;;)
	{
		if(structDef()){}
		else if(fnDef()){}
		else if(varDef()){}
		else break;
	}
	if(consume(END))
	{
		return true;
	}
	return false;
	}
// arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl()
{
    Token *start = iTk;

    if(consume(LBRACKET))
    {
        consume(INT); // optional
        if(consume(RBRACKET))
        {
            return true;
        }
        tkerr("lipseste ]");
    }

    iTk = start;
    return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef()
{
    Token *start = iTk;

    if(typeBase()){
        if(consume(ID)){
            if(arrayDecl()){}   // optional
            if(consume(SEMICOLON)){
                return true;
            }
            tkerr("lipseste ;");
        }
        tkerr("lipseste numele variabilei");
    }
    iTk = start;
    return false;
}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef()
{
    Token *start = iTk;

    if(consume(STRUCT))
    {
        if(consume(ID))
        {
            if(consume(LACC))
            {
                for(;;)
                {
                    if(varDef()){}
                    else break;
                }

                if(consume(RACC))
                {
                    if(consume(SEMICOLON))
                    {
                        return true;
                    }
                    tkerr("lipseste ; dupa struct");
                }
                tkerr("lipseste } la finalul structurii");
            }

            iTk = start;
            return false;
        }
        tkerr("lipseste numele structurii");
    }

    iTk = start;
    return false;
}

// fnParam: typeBase ID arrayDecl?
bool fnParam()
{
    Token *start = iTk;

    if(typeBase())
    {
        if(consume(ID))
        {
            if(arrayDecl()){}   // optional
            return true;
        }
        tkerr("lipseste numele parametrului");
    }

    iTk = start;
    return false;
}
//stmCompound: LACC (varDef | stm)* RACC
bool stmCompound()
{
    Token *start = iTk;

    if(consume(LACC))
    {
        for(;;)
        {
            if(varDef()){}
            else if(stm()){}
            else break;
        }
        if(consume(RACC))
        {
            return true;
        }
        tkerr("lipseste }");
    }
    iTk = start;
    return false;
}

//expAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign()
{
    Token *start = iTk;

    if(exprUnary())
    {
        if(consume(ASSIGN))
        {
            if (exprAssign())
            {
                return true;
            }
            tkerr("lipseste expresia dupa =");
        }
    }
    iTk = start;
    if(exprOr())
    {
        return true;
    }
    iTk = start;
    return false;
}
//expr: exprAssign
bool expr()
{
    return exprAssign();
}
/*stm: stmCompound 
        | IF LPAR expr RPAR stm (ELSE stm)?
        | WHILE LPAR expr RPAR stm
        | RETURN expr? SEMICOLON
        | expr? SEMICOLON
*/
bool stm()
{
    Token *start = iTk;

    if (stmCompound()) 
    {
        return true;
    }
    //| IF LPAR expr RPAR stm (ELSE stm)?
    iTk = start;
    if (consume(IF))
    {
        if(consume(LPAR))
        {
            if(expr())
            {
                if(consume(RPAR))
                {
                    if(stm())
                    {
                        if(consume(ELSE))
                        {
                            if(!stm())
                            {
                                tkerr("lipseste instructiunea dupa else");
                            }
                        }
                        return true;
                    }
                    tkerr("lipseste instructiunea dupa if");
                }
                tkerr("conditie invalida in if sau lipseste )");
            }
            tkerr("conditie invalida in if");
        }
        tkerr("lipseste ( dupa if");
    }
    // | WHILE LPAR expr RPAR stm
    iTk = start;
    if (consume(WHILE))
    {
        if(consume(LPAR))
        {
            if(expr())
            {
                if(consume(RPAR))
                {
                    if(stm())
                    {
                        return true;
                    }
                    tkerr("lipseste instructiunea dupa while");
                }
                tkerr("conditie invalida in while sau lipseste )");
            }
            tkerr("conditie invalida in while");
        }
        tkerr("lipseste ( dupa while");
    }

    //| RETURN expr? SEMICOLON
    iTk = start;
    if (consume(RETURN))
    {
        if(expr()){} //optional
        if(consume(SEMICOLON))
        {
            return true;
        }
        tkerr("lipseste ; dupa return");
    }

    //| expr? SEMICOLON

    iTk = start;
    if(expr())
    {
        if(consume(SEMICOLON))
        {
            return true;
        }
        tkerr("lipseste ; dupa expresie");
    }

    iTk = start;
    if(consume(SEMICOLON))
    {
        return true;
    }

    iTk = start;
    return false;
}

// fnDef: (typeBase | VOID) ID LPAR (fnParam(COMMA fnParam)*)? RPAR stmCompound
bool fnDef()
{
    Token *start = iTk;

    // cazul: VOID ID 
    if(consume(VOID))
    {
        if(consume(ID))
        {
            if(consume(LPAR))
            {
                if(fnParam())
                {
                    while(consume(COMMA))
                    {
                        if(!fnParam())
                        {
                            tkerr("lipseste parametrul dupa ,");
                        }
                    }
                }

                if(consume(RPAR))
                {
                    if(stmCompound())
                    {
                        return true;
                    }
                    tkerr("lipseste corpul functiei");
                }
                tkerr("lipseste )");
            }
            tkerr("lipseste ( dupa numele functiei");
        }
        tkerr("lipseste numele functiei");
    }

    iTk = start;

    // cazul: typeBase ID 
    if(typeBase())
    {
        if(consume(ID))
        {
            if(consume(LPAR))
            {
                if(fnParam())
                {
                    while(consume(COMMA))
                    {
                        if(!fnParam())
                        {
                            tkerr("lipseste parametrul dupa ,");
                        }
                    }
                }

                if(consume(RPAR))
                {
                    if(stmCompound())
                    {
                        return true;
                    }
                    tkerr("lipseste corpul functiei");
                }
                tkerr("lipseste )");
            }

            // aici poate fi varDef, deci NU dam eroare
            iTk = start;
            return false;
        }

        tkerr("lipseste numele functiei sau variabilei");
    }

    iTk = start;
    return false;
}


void parse(Token *tokens){
	iTk=tokens;
	if(!unit())tkerr("syntax error");
	}

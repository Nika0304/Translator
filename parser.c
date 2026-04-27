#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "parser.h"
#include "utils.h"

Token *iTk;		// the iterator in the tokens list
Token *consumedTk;		// the last consumed token
Symbol *owner = NULL;

void tkerr(const char *fmt,...)
{
	fprintf(stderr,"error in line %d: ",iTk->line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
	}

bool consume(int code)
{
	if(iTk->code==code){
		consumedTk=iTk;
		iTk=iTk->next;
		return true;
		}
	return false;
	}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID + Analiza de domeniu
bool typeBase(Type *t)
{
    t->n = -1;

    if(consume(TYPE_INT)){
        t->tb = TB_INT;
        return true;
    }
    if(consume(TYPE_DOUBLE)){
        t->tb = TB_DOUBLE;
        return true;
    }
    if(consume(TYPE_CHAR)){
        t->tb = TB_CHAR;
        return true;
    }
    if(consume(STRUCT)){
        if(consume(ID)){
            Token *tkName = consumedTk;

            t->tb = TB_STRUCT;
            t->s = findSymbol(tkName->text);

            if(!t->s)tkerr("structura nedefinita: %s",tkName->text);

            return true;
        }
    }
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
    tkerr("eroare de sintaxa");
	return false;
	}


// arrayDecl: LBRACKET INT? RBRACKET + Analiza de domeniu
bool arrayDecl(Type *t)
{
    Token *start = iTk;

    if(consume(LBRACKET))
    {
        if(consume(INT))
        {
            Token *tkSize = consumedTk;
            t->n = tkSize->i;
        }
        else
        {
            t->n = 0;
        }

        if(consume(RBRACKET))
        {
            return true;
        }
        tkerr("lipseste ]");
    }

    iTk = start;
    return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON + Analiza de domeniu
bool varDef()
{
    Token *start = iTk;
    Type t;

    if (typeBase(&t))
    {
        if (consume(ID))
        {
            Token *tkName = consumedTk;

            if (arrayDecl(&t))
            {
                if (t.n == 0)
                {
                    tkerr("a vector variable must have a specified dimension");
                }
            }

            if (consume(SEMICOLON))
            {
                Symbol *var = findSymbolInDomain(symTable, tkName->text);
                if (var)
                {
                    tkerr("symbol redefinition: %s", tkName->text);
                }

                var = newSymbol(tkName->text, SK_VAR);
                var->type = t;
                var->owner = owner;

                addSymbolToDomain(symTable, var);

                if (owner)
                {
                    switch (owner->kind)
                    {
                        case SK_FN:
                            var->varIdx = symbolsLen(owner->fn.locals);
                            addSymbolToList(&owner->fn.locals, dupSymbol(var));
                            break;

                        case SK_STRUCT:
                            var->varIdx = typeSize(&owner->type);
                            addSymbolToList(&owner->structMembers, dupSymbol(var));
                            break;
                        default:
                            break;
                    }
                }
                else
                {
                    var->varMem = safeAlloc(typeSize(&t));
                }

                return true;
            }

            tkerr("lipseste ;");
        }

        tkerr("lipseste numele variabilei");
    }

    iTk = start;
    return false;
}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON + Analiza de domeniu
bool structDef()
{
    Token *start = iTk; //salvează poziția curentă în lista de tokeni, daca nu reuseste parsarea revenim

    if(consume(STRUCT))
    {
        if(consume(ID))
        {
            Token *tkName = consumedTk;

            if(consume(LACC))
            {

                Symbol *s = findSymbolInDomain(symTable, tkName->text);
                if (s)
                {
                    tkerr("symbol redefinition: %s", tkName->text);
                }
                s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));

                s->type.tb = TB_STRUCT;
                s->type.s = s;
                s->type.n = -1;

                pushDomain();
                owner = s;

                for(;;)
                {
                    if(varDef()){}
                    else break;
                }

                if(consume(RACC))
                {
                    if(consume(SEMICOLON))
                    {
                        owner = NULL;
                        dropDomain();
                        return true;
                    }
                    owner = NULL;
                    dropDomain();
                    tkerr("lipseste ; dupa struct");
                }
                owner = NULL;
                dropDomain();
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
    Type t;
    Token *tkName;

    if (typeBase(&t))
    {
        if (consume(ID))
        {
            tkName = consumedTk;
            if (arrayDecl(&t)) { t.n = 0; }
            Symbol *param = findSymbolInDomain(symTable, tkName->text);
            if(param)
            {
                tkerr("symbol redefinition: %s", tkName->text);
            }
            param = newSymbol(tkName->text, SK_PARAM);
            param->type = t;
            param->owner = owner;
            param->paramIdx = symbolsLen(owner->fn.params);

            addSymbolToDomain(symTable, param); //îl face vizibil în corpul funcției.
            addSymbolToList(&owner->fn.params, dupSymbol(param));// il salveaza in lista de parametri a functiei
        
            return true;
        }
        tkerr("lipseste numele parametrului");
    }

    iTk = start;
    return false;
}

//stmCompound: LACC (varDef | stm)* RACC
bool stmCompound(bool newDomain)
{
    Token *start = iTk;

    if(consume(LACC))
    {
        if(newDomain) pushDomain();
        for(;;)
        {
            if(varDef()){}
            else if(stm()){}
            else break;
        }
        if(consume(RACC))
        {
            if(newDomain) dropDomain();
            return true;
        }
        if(newDomain) dropDomain();//curatam domeniul inainte de eroare
        tkerr("lipseste }");
    }
    iTk = start;
    return false;
}

//exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )? | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary()
{
    Token *start = iTk;

    if (consume(ID))
    {
        if (consume(LPAR))
        {
            if (expr())
            {
                while (consume(COMMA))
                {
                    if (!expr())
                    {
                        tkerr("lipseste expresia dupa ,");
                    }
                }
            }

            if (consume(RPAR))
            {
                return true;
            }
            tkerr("lipseste )");
        }
        return true;
    }

    if (consume(INT))
    {
        return true;
    }

    if (consume(DOUBLE))
    {
        return true;
    }

    if (consume(CHAR))
    {
        return true;
    }

    if (consume(STRING))
    {
        return true;
    }

    if (consume(LPAR))
    {
        if (expr())
        {
            if (consume(RPAR))
            {
                return true;
            }
            tkerr("lipseste )");
        }
        tkerr("lipseste expresia dintre paranteze");
    }

    iTk = start;
    return false;
}

//exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim | DOT ID exprPostfixPrim | ε
bool exprPostfixPrim()
{
    if (consume(LBRACKET))
    {
        if (expr())
        {
            if (consume(RBRACKET))
            {
                return exprPostfixPrim();
            }
            tkerr("lipseste ]");
        }
        tkerr("lipseste expresia din []");
    }

    if (consume(DOT))
    {
        if (consume(ID))
        {
            return exprPostfixPrim();
        }
        tkerr("lipseste numele campului dupa .");
    }

    return true;
}

//exprPostfix: exprPrimary exprPostfixPrim
bool exprPostfix()
{
    Token *start = iTk;

    if (exprPrimary())
    {
        if (exprPostfixPrim())
        {
            return true;
        }
    }

    iTk = start;
    return false;
}

//exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary()
{
    Token *start = iTk;

    if (consume(SUB))
    {
        if (exprUnary())
        {
            return true;
        }
        tkerr("lipseste expresia dupa operatorul unar '-'");
    }

    iTk = start;
    if (consume(NOT))
    {
        if (exprUnary())
        {
            return true;
        }
        tkerr("lipseste expresia dupa operatorul unar '!'");
    }

    iTk = start;
    if (exprPostfix())
    {
        return true;
    }

    iTk = start;
    return false;
}

//exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast()
{
    Token *start = iTk;

    if (consume(LPAR))
    {
        Type t;
        if (typeBase(&t))
        {
            if (arrayDecl(&t)) {}   // optional

            if (consume(RPAR))
            {
                if (exprCast())
                {
                    return true;
                }
                tkerr("lipseste expresia dupa cast");
            }
            tkerr("lipseste ) dupa cast");
        }

        iTk = start;
    }

    if (exprUnary())
    {
        return true;
    }

    iTk = start;
    return false;
}

//exprMul: exprCast exprMulPrim
bool exprMul()
{
    Token *start = iTk;

    if (exprCast())
    {
        if (exprMulPrim())
        {
            return true;
        }
    }

    iTk = start;
    return false;
}

//exprMulPrim: ( MUL | DIV ) exprCast exprMulPrim | ε
bool exprMulPrim()
{
    if (consume(MUL))
    {
        if (exprCast())
        {
            return exprMulPrim();
        }
        tkerr("lipseste expresia dupa *");
    }
    if (consume(DIV))
    {
        if (exprCast())
        {
            return exprMulPrim();
        }
        tkerr("lipseste expresia dupa /");
    }
    return true;
}

//exprAdd: exprMul exprAddPrim
bool exprAdd()
{
    Token *start = iTk;

    if (exprMul())
    {
        if (exprAddPrim())
        {
            return true;
        }
    }

    iTk = start;
    return false;
}

//exprAddPrim: ( ADD | SUB ) exprMul exprAddPrim | ε
bool exprAddPrim()
{
    if (consume(ADD))
    {
        if (exprMul())
        {
            return exprAddPrim();
        }
        tkerr("lipseste expresia dupa +");
    }
    if (consume(SUB))
    {
        if (exprMul())
        {
            return exprAddPrim();
        }
        tkerr("lipseste expresia dupa -");
    }
    return true;
}

//exprRel: exprAdd exprRelPrim
bool exprRel()
{
    Token *start = iTk;

    if (exprAdd())
    {
        if (exprRelPrim())
        {
            return true;
        }
    }

    iTk = start;
    return false;
}

//exprRelPrim: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRelPrim | ε
bool exprRelPrim()
{
    if (consume(LESS))
    {
        if (exprAdd())
            return exprRelPrim();
        tkerr("lipseste expresia dupa <");
    }
    else if (consume(LESSEQ))
    {
        if (exprAdd())
            return exprRelPrim();
        tkerr("lipseste expresia dupa <=");
    }
    else if (consume(GREATER))
    {
        if (exprAdd())
            return exprRelPrim();
        tkerr("lipseste expresia dupa >");
    }
    else if (consume(GREATEREQ))
    {
        if (exprAdd())
            return exprRelPrim();
        tkerr("lipseste expresia dupa >=");
    }
    return true;
}

// exprEq: exprRel exprEqPrim
bool exprEq() 
{
    Token *start = iTk;
    if (exprRel()) {
        if (exprEqPrim()) {
            return true;
        }
    }
    iTk = start;
    return false;
}

// exprEqPrim: (EQUAL|NOTEQ) exprRel exprEqPrim | ε
bool exprEqPrim() 
{
    if (consume(EQUAL)) {
        if (exprRel()) {
            return exprEqPrim();
        }
        tkerr("lipseste expresia dupa == ");
    }
    if (consume(NOTEQ)) {
        if (exprRel()) {
            return exprEqPrim();
        }
        tkerr("lipseste expresia dupa !=");
    }
    return true;
}

// exprAnd: exprEq exprAndPrim
bool exprAnd()
{
    Token *start = iTk;
    if (exprEq()) {
        if (exprAndPrim()) {
            return true;
        }
    }
    iTk = start;
    return false;
}

// exprAndPrim: AND exprEq exprAndPrim | ε
bool exprAndPrim() 
{
    if (consume(AND)) {
        if (exprEq()) {
            return exprAndPrim();
        }
        tkerr("lipseste expresia dupa &&");
    }
    return true;
}

// exprOr: exprAnd exprOrPrim
bool exprOr() 
{
    Token *start = iTk;
    if (exprAnd()) {
        if (exprOrPrim()) {
            return true;
        }
    }
    iTk = start;
    return false;
}

// exprOrPrim: OR exprAnd exprOrPrim | ε
bool exprOrPrim() 
{
    if (consume(OR)) {
        if (exprAnd()) {
            return exprOrPrim();
        }
        tkerr("lipseste expresia dupa ||");
    }
    return true;
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

    // stmCompound
    if (stmCompound(true)) {
        return true;
    }

    // IF LPAR expr RPAR stm ( ELSE stm )?
    iTk = start;
    if (consume(IF)) {
        if (consume(LPAR)) {
            if (expr()) {
                if (consume(RPAR)) {
                    if (stm()) {
                        if (consume(ELSE)) {
                            if (stm()) {
                                return true;
                            } else {
                                tkerr("lipseste instructiune dupa else");
                            }
                        }
                        return true;
                    } else {
                        tkerr("lipseste instructiune dupa if");
                    }
                } else {
                    tkerr("conditie invalida pentru if sau lipseste )");
                }
            } else {
                tkerr("lipseste conditia din if");
            }
        } else {
            tkerr("lipseste ( dupa if");
        }
    }

    // WHILE LPAR expr RPAR stm
    iTk = start;
    if (consume(WHILE)) {
        if (consume(LPAR)) {
            if (expr()) {
                if (consume(RPAR)) {
                    if (stm()) {
                        return true;
                    } else {
                        tkerr("lipseste instructiune dupa while");
                    }
                } else {
                    tkerr("conditie invalida pentru while sau lipseste )");
                }
            } else {
                tkerr("lipseste conditia din while");
            }
        } else {
            tkerr("lipseste ( dupa while");
        }
    }

    // RETURN expr? SEMICOLON
    iTk = start;
    if (consume(RETURN)) {
        expr();  // optional
        if (consume(SEMICOLON)) {
            return true;
        } else {
            tkerr("lipseste ; dupa return");
        }
    }

    // expr? SEMICOLON
    iTk = start;
    if (expr()) 
    {
        if (consume(SEMICOLON)) 
        {
            return true;
        } else 
        {
            tkerr("lipseste ; dupa expresie");
        }
    }

    iTk = start;
    if (consume(SEMICOLON)) 
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
    Type t;
    Token *tkName;

    // cazul 1: functie cu void
    if (consume(VOID))
    {
        t.tb = TB_VOID;
        if (!consume(ID))
        {
            tkerr("lipseste numele functiei");
        }
        tkName = consumedTk;
        if (!consume(LPAR))
        {
            tkerr("lipseste ( dupa numele functiei");
        }
    }
    // cazul 2: functie cu typeBase
    else if (typeBase(&t))
    {
        if (!consume(ID))
        {
            iTk = start;
            return false;   // lasă varDef să dea eroarea
        }
        tkName = consumedTk;
        if (!consume(LPAR))
        {
            iTk = start;
            return false;   // poate fi varDef
        }
    }
    else
    {
        iTk = start;
        return false;
    }

    Symbol *fn = findSymbolInDomain(symTable, tkName->text);

    if (fn)
    {
        tkerr("symbol redefinition: %s", tkName->text);
    }

    fn = newSymbol(tkName->text, SK_FN);
    fn->type = t;
    addSymbolToDomain(symTable, fn);

    owner = fn;
    pushDomain();

    if (fnParam())
    {
        while (consume(COMMA))
        {
            if (!fnParam())
            {
                tkerr("lipseste parametrul dupa , sau parametru invalid");
            }
        }
    }

    if (!consume(RPAR))
    {
        tkerr("parametru invalid sau lipseste )");
    }

    if (stmCompound(false))
    {
        dropDomain();
        owner = NULL;
        return true;
    }

    dropDomain(); //curatam domeniul inainte de eroare
    owner = NULL;
    tkerr("lipseste corpul functiei");
    return false;
}

void parse(Token *tokens){
	iTk=tokens;
	if(!unit())tkerr("syntax error");
	}



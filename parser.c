#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "parser.h"

Token *iTk;     // the iterator in the tokens list
Token *consumedTk;      // the last consumed token
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
    tkerr("syntax error");
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
        tkerr("missing ]");
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

            tkerr("missing ;");
        }

        tkerr("missing variable name or missing { in struct definition");
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
                    tkerr("missing ; after struct");
                }
                owner = NULL;
                dropDomain();
                tkerr("missing } at the end of struct");
            }

            iTk = start;
            return false; 
        }
        tkerr("missing struct name");
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
        tkerr("missing parameter name");
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
        tkerr("missing } in compound statement");
    }
    iTk = start;
    return false;
}

//exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )? | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary(Ret *r)
{
    Token *start = iTk;

    if (consume(ID))
    {
        Token *tkName = consumedTk;
        Symbol *s = findSymbol(tkName->text);
        if(!s)
        {
            tkerr("undefined id: %s", tkName->text);
        }
        if (consume(LPAR))
        {
            if(s->kind != SK_FN)
            {
                tkerr("only a function can be called");
            }
            Ret rArg;
            Symbol *param = s->fn.params;
            if (expr(&rArg))
            {
                if(!param)
                {
                    tkerr("too many arguments in function call");
                }
                if(!convTo(&rArg.type, &param->type))
                {
                    tkerr("in call, cannot convert the argument type to the parameter type");
                }
                param = param->next;
                while (consume(COMMA))
                {
                    if (expr(&rArg))
                    {
                        if(!param)
                    {
                        tkerr("too many arguments in function call");
                    }
                    if(!convTo(&rArg.type, &param->type))
                    {
                        tkerr("in call, cannot convert the argument type to the parameter type");
                    }
                    param = param->next;
                    }
                    else tkerr("missing expression after ,");
                }
            }

            if (consume(RPAR))
            {
                if(param)
                {
                    tkerr("too few arguments in function call");
                }
                *r = (Ret){s->type, false, true};
                return true;
            }
            tkerr("missing )");
        }
        if(s->kind == SK_FN)
        {
            tkerr("a function can only be called");
        }
        *r = (Ret){s->type, true, s->type.n >= 0};
        return true;
    }

    if (consume(INT))
    {
        *r = (Ret){{TB_INT, NULL, -1}, false, true};
        return true;
    }

    if (consume(DOUBLE))
    {
        *r = (Ret){{TB_DOUBLE, NULL, -1}, false, true};
        return true;
    }

    if (consume(CHAR))
    {
        *r = (Ret){{TB_CHAR, NULL, -1}, false, true};
        return true;
    }

    if (consume(STRING))
    {
        *r = (Ret){{TB_CHAR, NULL, 0}, false, true};
        return true;
    }

    if (consume(LPAR))
    {
        if (expr(r))
        {
            if (consume(RPAR))
            {
                return true;
            }
            tkerr("missing )");
        }
        iTk = start;
    }

    iTk = start;
    return false;
}


//exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim | DOT ID exprPostfixPrim | ε
bool exprPostfixPrim(Ret *r)
{
    Ret idx;
    if (consume(LBRACKET))
    {
        if (expr(&idx))
        {
            if (consume(RBRACKET))
            {
                if(r->type.n < 0 )
                {
                    tkerr("only an array can be indexed");
                }
                Type tInt = {TB_INT, NULL, -1};
                if(!convTo(&idx.type, &tInt))
                {
                    tkerr("the index is not convertible to int");
                }
                r->type.n = -1;
                r->lval = true;
                r->ct = false;
                return exprPostfixPrim(r);
            }
            tkerr("missing ]");
        }
        tkerr("missing expression inside []");
    }

    if (consume(DOT))
    {
        if (consume(ID))
        {
            Token *tkName = consumedTk;
            if(r->type.tb != TB_STRUCT)
            {
                tkerr("a field can only be selected from a struct");
            }
            Symbol *s = findSymbolInList(r->type.s->structMembers, tkName->text);
            if(!s)
            {
                tkerr("the structure %s does not have a field %s", r->type.s->name, tkName->text);
            }
            *r = (Ret){s->type, true, s->type.n >= 0};
            return exprPostfixPrim(r);
        }
        tkerr("missing field name after .");
    }

    return true;
}

//exprPostfix: exprPrimary exprPostfixPrim
bool exprPostfix(Ret *r)
{
    Token *start = iTk;

    if (exprPrimary(r))
    {
        if (exprPostfixPrim(r))
        {
            return true;
        }
    }

    iTk = start;
    return false;
}

//exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary(Ret *r)
{
    Token *start = iTk;

    if (consume(SUB))
    {
        if (exprUnary(r))
        {
            if(!canBeScalar(r))
            {
                tkerr("unary - must have a scalar operand");
            }
            r->lval = false;
            r->ct = true;
            return true;
        }
        tkerr("missing expression after unary '-'");
    }

    iTk = start;
    if (consume(NOT))
    {
        if (exprUnary(r))
        {
            if(!canBeScalar(r))
            {
                tkerr("unary ! must have a scalar operand");
            }
            r->type = (Type){TB_INT, NULL, -1};
            r->lval = false;
            r->ct = true;
            return true;
        }
        tkerr("missing expression after unary '!'");
    }

    iTk = start;
    if (exprPostfix(r))
    {
        return true;
    }

    iTk = start;
    return false;
}

//exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast(Ret *r)
{
    Token *start = iTk;

    if (consume(LPAR))
    {
        Type t;
        Ret op;

        if (typeBase(&t))
        {
            if (arrayDecl(&t)) {}   // optional

            if (consume(RPAR))
            {
                if (exprCast(&op))
                {
                    if(t.tb == TB_STRUCT) 
                    {
                        tkerr("cannot convert to a struct type");
                    }
                    if(op.type.tb == TB_STRUCT)
                    {
                        tkerr("cannot convert a struct");
                    }
                    if(op.type.n >= 0 && t.n <0)
                    {
                        tkerr("an array can be converted only to another array");
                    }
                    if(op.type.n < 0 && t.n >= 0)
                    {
                        tkerr("a scalar can be converted only to another scalar");
                    }
                    *r = (Ret){t, false, true};
                    return true;
                }
               tkerr("missing expression after cast");
               return false;
            }
            else
            {
                tkerr("missing ) after cast");
            }
        }

        iTk = start;
    }

    if (exprUnary(r))
    {
        return true;
    }

    iTk = start;
    return false;
}


//exprMul: exprCast exprMulPrim
bool exprMul(Ret *r)
{
    Token *start = iTk;

    if (exprCast(r))
    {
        if (exprMulPrim(r))
        {
            return true;
        }
    }

    iTk = start;
    return false;
}

//exprMulPrim: ( MUL | DIV ) exprCast exprMulPrim | ε
bool exprMulPrim(Ret *r)
{
    Ret right;

    if (consume(MUL))
    {
        if (exprCast(&right))
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr("invalid operand type for *");
            }
            *r = (Ret){tDst,false, true};
            return exprMulPrim(r);
        }
        tkerr("missing expression after *");
    }
    if (consume(DIV))
    {
        if (exprCast(&right))
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr("invalid operand type for /");
            }
            *r = (Ret){tDst,false, true};
            return exprMulPrim(r);
        }
        tkerr("missing expression after /");
    }
    return true;
}

//exprAdd: exprMul exprAddPrim
bool exprAdd(Ret *r)
{
    Token *start = iTk;

    if (exprMul(r))
    {
        if (exprAddPrim(r))
        {
            return true;
        }
    }

    iTk = start;
    return false;
}

//exprAddPrim: ( ADD | SUB ) exprMul exprAddPrim | ε
bool exprAddPrim(Ret *r)
{
    Ret right;

    if (consume(ADD))
    {
        if (exprMul(&right))
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr("invalid operand type for +");
            }
            *r = (Ret){tDst,false, true};
            return exprAddPrim(r);
        }
        tkerr("missing expression after +");
    }
    if (consume(SUB))
    {
        if (exprMul(&right))
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr("invalid operand type for -");
            }
            *r = (Ret){tDst, false, true};
            return exprAddPrim(r);
        }
        tkerr("missing expression after -");
    }
    return true;
}

//exprRel: exprAdd exprRelPrim
bool exprRel(Ret *r)
{
    Token *start = iTk;

    if (exprAdd(r))
    {
        if (exprRelPrim(r))
        {
            return true;
        }
    }

    iTk = start;
    return false;
}

//exprRelPrim: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRelPrim | ε
bool exprRelPrim(Ret *r)
{
    Ret right;

    if (consume(LESS))
    {
        if (exprAdd(&right))
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr("invalid operand type for <");
            }
            *r = (Ret){{TB_INT,NULL,-1}, false, true};
            return exprRelPrim(r);
        }
        tkerr("missing expression after <");
    }
    else if (consume(LESSEQ))
    {
        if (exprAdd(&right))
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr("invalid operand type for <=");
            }
            *r = (Ret){{TB_INT,NULL,-1}, false, true};
            return exprRelPrim(r);
        }
        tkerr("missing expression after <=");
    }
    else if (consume(GREATER))
    {
        if (exprAdd(&right))
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr("invalid operand type for >");
            }
            *r = (Ret){{TB_INT,NULL,-1}, false, true};
            return exprRelPrim(r);
        }
        tkerr("missing expression after >");
    }
    else if (consume(GREATEREQ))
    {
        if (exprAdd(&right))
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr("invalid operand type for >=");
            }
            *r = (Ret){{TB_INT,NULL,-1}, false, true};
            return exprRelPrim(r);
        }
        tkerr("missing expression after >=");
    }
    return true;
}

// exprEq: exprRel exprEqPrim
bool exprEq(Ret *r) 
{
    Token *start = iTk;
    if (exprRel(r)) {
        if (exprEqPrim(r)) {
            return true;
        }
    }
    iTk = start;
    return false;
}

// exprEqPrim: (EQUAL|NOTEQ) exprRel exprEqPrim | ε
bool exprEqPrim(Ret *r) 
{
    Ret right;
    if (consume(EQUAL)) 
    {
        if (exprRel(&right)) 
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr("invalid operand type for ==");
            }
            *r = (Ret){{TB_INT,NULL,-1}, false, true};
            return exprEqPrim(r);
        }
        tkerr("missing expression after ==");
    }
    if (consume(NOTEQ)) 
    {
        if (exprRel(&right)) 
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr("invalid operand type for !=");
            }
            *r = (Ret){{TB_INT,NULL,-1}, false, true};
            return exprEqPrim(r);
        }
        tkerr("missing expression after !=");
    }
    return true;
}

// exprAnd: exprEq exprAndPrim
bool exprAnd(Ret *r)
{
    Token *start = iTk;
    if (exprEq(r)) {
        if (exprAndPrim(r)) {
            return true;
        }
    }
    iTk = start;
    return false;
}

// exprAndPrim: AND exprEq exprAndPrim | ε
bool exprAndPrim(Ret *r) 
{
    if (consume(AND)) {
        Ret right;
        if (exprEq(&right)) {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr("invalid operand type for &&");
            }
            *r = (Ret){{TB_INT,NULL,-1}, false, true};
            return exprAndPrim(r);
        }
        tkerr("missing expression after &&");
    }
    return true;
}

// exprOr: exprAnd exprOrPrim
bool exprOr(Ret *r) 
{
    Token *start = iTk;
    if (exprAnd(r)) {
        if (exprOrPrim(r)) 
        {
            return true;
        }
    }
    iTk = start;
    return false;
}

// exprOrPrim: OR exprAnd exprOrPrim | ε
bool exprOrPrim(Ret *r) 
{
    if (consume(OR)) {
        Ret right;

        if (exprAnd(&right)) 
        {
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst))
            {
                tkerr("invalid operand type for ||");
            }
            *r = (Ret){{TB_INT,NULL,-1}, false, true};
            return exprOrPrim(r);
        }
        tkerr("missing expression after ||");
    }
    return true;
}

//expAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign(Ret *r)
{
    Token *start = iTk;
    Ret rDst;

    if(exprUnary(&rDst))
    {
        if(consume(ASSIGN))
        {
            if (exprAssign(r))
            {
                if(!rDst.lval)
                {
                    tkerr("the assign destination must be a left-value");
                }
                if(rDst.ct)
                {
                    tkerr("the assign destination cannot be constant");
                }
                if(!canBeScalar(&rDst))
                {
                    tkerr("the assign destination must be a scalar");
                }
                if(!canBeScalar(r))
                {
                    tkerr("the assign source must be scalar");
                }
                if(!convTo(&r->type, &rDst.type))
                {
                    tkerr("the assign source cannot be converted to destination");
                }
                r->lval = false;
                r->ct = true;

                return true;
            }
            tkerr("missing expression after =");
        }
    }
    iTk = start;
    if(exprOr(r))
    {
        return true;
    }
    iTk = start;
    return false;
}


//expr: exprAssign
bool expr(Ret *r)
{
    return exprAssign(r);
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

    Ret rCond;
    Ret rExpr;

    // stmCompound
    if (stmCompound(true)) 
    {
        return true;
    }

    // IF LPAR expr RPAR stm ( ELSE stm )?
    iTk = start;
    if (consume(IF)) 
    {
        if (consume(LPAR)) 
        {
            if (expr(&rCond)) 
            {
                if(!canBeScalar(&rCond))
                {
                    tkerr("the if condition must be a scalar value");
                }
                if (consume(RPAR)) 
                {
                    if (stm()) 
                    {
                        if (consume(ELSE)) 
                        {
                            if (stm()) 
                            {
                                return true;
                            } 
                            else 
                            {
                                tkerr("missing statement after else");
                            }
                        }
                        return true;
                    } 
                    else 
                    {
                        tkerr("missing statement after if");
                    }
                } 
                else 
                {
                    tkerr("invalid if condition or missing )");
                }
            } 
            else 
            {
                tkerr("missing if condition");
            }
        } 
        else 
        {
            tkerr("missing ( after if");
        }
    }

    // WHILE LPAR expr RPAR stm
    iTk = start;
    if (consume(WHILE)) 
    {
        if (consume(LPAR))
        {
            if (expr(&rCond)) 
            {
                if(!canBeScalar(&rCond))
                {
                    tkerr("the while condition must be a scalar value");
                }
                if (consume(RPAR)) 
                {
                    if (stm()) 
                    {
                        return true;
                    } 
                    else 
                    {
                        tkerr("missing statement after while");
                    }
                } 
                else 
                {
                    tkerr("invalid while condition or missing )");
                }
            } 
            else 
            {
                tkerr("missing while condition");
            }
        } 
        else 
        {
            tkerr("missing ( after while");
        }
    }

    // RETURN expr? SEMICOLON
    iTk = start;
    if (consume(RETURN)) {
        if (expr(&rExpr))
        {  // optional
            if(owner->type.tb == TB_VOID) 
            {
                tkerr("a void function cannot return a value");
            }
            if(!canBeScalar(&rExpr))
            {
                tkerr("the return value must be a scalar value");
            }
            if(!convTo(&rExpr.type, &owner->type))
            {
                tkerr("cannot convert the return expression type to the function return type");
            }
        }
        else if(owner->type.tb != TB_VOID)
        {
            tkerr("a non-void function must return a value");
        }
        if (consume(SEMICOLON)) 
        {
            return true;
        } else {
            tkerr("missing ; after return");
        }
    }

    // expr? SEMICOLON
    iTk = start;
    if (expr(&rExpr)) 
    {
        if (consume(SEMICOLON)) 
        {
            return true;
        } else 
        {
            tkerr("missing ; after expression");
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
            tkerr("missing function name");
        }
        tkName = consumedTk;
        if (!consume(LPAR))
        {
            tkerr("missing ( after function name");
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
                tkerr("missing/invalid parameter after ,");
            }
        }
    }

    if (!consume(RPAR))
    {
        tkerr("invalid parameter or missing )");
    }

    if (stmCompound(false))
    {
        dropDomain();
        owner = NULL;
        return true;
    }

    dropDomain(); //curatam domeniul inainte de eroare
    owner = NULL;
    tkerr("missing function body");
    return false;
}

void parse(Token *tokens){
    iTk=tokens;
    if(!unit())tkerr("syntax error");
    }




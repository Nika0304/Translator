#pragma once

#include "lexer.h"
#include <stdbool.h>
#include "ad.h"

void parse(Token *tokens);

bool typeBase(Type *t);
bool arrayDecl(Type *t);
bool varDef();
bool structDef();
bool fnParam();
bool fnDef();
bool stmCompound(bool newDomain);
bool stm();
bool expr();
bool exprAssign();
bool exprOr();
bool exprOrPrim();
bool exprAnd();
bool exprAndPrim();
bool exprEq();
bool exprEqPrim();
bool exprRel();
bool exprRelPrim();
bool exprAdd();
bool exprAddPrim();
bool exprMul();
bool exprMulPrim();
bool exprCast();
bool exprUnary();
bool exprPostfix();
bool exprPostfixPrim();
bool exprPrimary();
bool unit();

#pragma once

#include "lexer.h"
#include <stdbool.h>
void parse(Token *tokens);

bool unit();
bool structDef();
bool varDef();
bool typeBase();
bool arrayDecl();
bool fnParam();
bool fnDef();
bool stmCompound();
bool exprAssign();
bool expr();
bool stm();

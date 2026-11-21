/**
 * @file codegen.c
 * @brief Generátor cieľového kódu IFJcode26
 *
 * Implementace překladače imperativního jazyka IFJ26.
 * Projekt z predmetov IFJ a IAL, FIT VUT v Brne, 2026/2027.
 *
 * @author Patrik Bukovský (xbukovp00)
 * @author Tomáš Szalay (xszalat00)
 * @author Mikuláš Gärtner (xgartnm00)
 * @author Daniel Malček (xmalced00)
 *
 * @copyright Copyright (C) 2026 Patrik Bukovský, Tomáš Szalay,
 *            Mikuláš Gärtner, Daniel Malček
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "codegen.h"
#include "dynstr.h"
#include "error.h"
#include "semantic.h"

#include <stdarg.h>
#include <string.h>

void appendFormat(CodeGen *gen, DynStr *target, const char *format, va_list args) {
    if (gen->error != ERR_OK) {
        return;
    }

    va_list argsCopy;

    // najprv sa zisti dlzka vysledku, potom sa zapise na koniec bufferu
    va_copy(argsCopy, args);
    int length = vsnprintf(NULL, 0, format, argsCopy);
    va_end(argsCopy);

    if (length < 0 || !dynstr_reserve(target, (size_t)length)) {
        gen->error = ERR_INTERNAL;
        return;
    }

    vsnprintf(target->data + target->len, (size_t)length + 1, format, args);
    target->len += (size_t)length;
}

void emit(CodeGen *gen, const char *format, ...) {
    va_list args;

    va_start(args, format);
    appendFormat(gen, &gen->code, format, args);
    va_end(args);
}

void emitDeclaration(CodeGen *gen, const char *format, ...) {
    va_list args;

    va_start(args, format);
    appendFormat(gen, &gen->declarations, format, args);
    va_end(args);
}

void emitHeader(CodeGen *gen) { fprintf(gen->out, ".IFJcode26\n"); }

void emitHelperVars(CodeGen *gen) {
    emitDeclaration(gen, "DEFVAR LF@%%t0\n");
    emitDeclaration(gen, "DEFVAR LF@%%t1\n");
    emitDeclaration(gen, "DEFVAR LF@%%t2\n");
    emitDeclaration(gen, "DEFVAR LF@%%t3\n");
    emitDeclaration(gen, "DEFVAR LF@%%t4\n");
    emitDeclaration(gen, "DEFVAR LF@%%trash\n");
}

void emitString(CodeGen *gen, const char *text) {
    emit(gen, "string@");

    for (size_t charIndex = 0; text[charIndex] != '\0'; charIndex++) {
        unsigned char currentChar = (unsigned char)text[charIndex];

        // biele znaky, # a \ sa musia zapisat ako \ddd
        if (currentChar <= 32 || currentChar == 35 || currentChar == 92) {
            emit(gen, "\\%03d", currentChar);
        } else {
            emit(gen, "%c", currentChar);
        }
    }
}

void emitVar(CodeGen *gen, const char *name, Symbol *symbol) {
    emit(gen, "LF@%s$%d", name, symbol->cgId);
}

void emitLiteral(CodeGen *gen, LiteralNode *literal) {
    if (literal->litType == LIT_INT) {
        emit(gen, "int@%lld", literal->litValue.int_value);
    } else if (literal->litType == LIT_DOUBLE) {
        // desatinne cislo sa zapisuje vo formate %a
        emit(gen, "float@%a", literal->litValue.float_value);
    } else if (literal->litType == LIT_STRING) {
        emitString(gen, literal->litValue.string_value);
    } else if (literal->litType == LIT_BOOLEAN) {
        if (literal->litValue.bool_value) {
            emit(gen, "bool@true");
        } else {
            emit(gen, "bool@false");
        }
    } else if (literal->litType == LIT_UNIT) {
        // hodnota () je za behu nil
        emit(gen, "nil@nil");
    }
}

void initCodeGen(CodeGen *gen, FILE *out) {
    gen->out = out;
    gen->labelCounter = 0;
    gen->varCounter = 0;
    gen->function = NULL;
    gen->error = ERR_OK;

    for (int helper = 0; helper < HELPER_COUNT; helper++) {
        gen->helperUsed[helper] = false;
    }

    // oba buffre sa inicializuju vzdy, aby sa dali uvolnit aj pri chybe
    bool isCodeReady = dynstr_init(&gen->code);
    bool isDeclarationsReady = dynstr_init(&gen->declarations);

    if (!isCodeReady || !isDeclarationsReady) {
        gen->error = ERR_INTERNAL;
    }
}

void freeCodeGen(CodeGen *gen) {
    dynstr_free(&gen->code);
    dynstr_free(&gen->declarations);
}

int newLabel(CodeGen *gen) {
    gen->labelCounter++;
    return gen->labelCounter;
}

DefFuncNode *findMain(ProgramNode *prog) {
    for (size_t itemIndex = 0; itemIndex < prog->itemsCount; itemIndex++) {
        ASTNode *item = prog->items[itemIndex];

        if (item->kind == AST_FUNCTION_DEF && ((DefFuncNode *)item)->isMain) {
            return (DefFuncNode *)item;
        }
    }

    return NULL;
}

void genPrint(CodeGen *gen, CallNode *call) {
    ASTNode *arg = call->args[0];

    // WRITE nema zasobnikovu verziu
    genExpr(gen, arg);
    emit(gen, "POPS LF@%%t0\n");

    // hodnota Unit je za behu nil, vypisuje sa ako "()"
    if (arg->type->kind == TY_UNIT) {
        emit(gen, "WRITE string@()\n");
    } else {
        emit(gen, "WRITE LF@%%t0\n");
    }

    // print vracia ()
    emit(gen, "PUSHS nil@nil\n");
}

void genFunctionCall(CodeGen *gen, CallNode *call) {
    DefFuncNode *function = (DefFuncNode *)call->symbol->definition;

    // argumenty idu na zasobnik zlava doprava, funkcia si ich vyberie sama
    for (size_t argIndex = 0; argIndex < call->argsCount; argIndex++) {
        genExpr(gen, call->args[argIndex]);
    }

    emit(gen, "CALL f$%s\n", function->name);
}

void genReadLine(CodeGen *gen) {
    // READ vrati riadok aj s koncom riadku, pri EOF prazdny retazec
    emit(gen, "READ LF@%%t0 string\n");
    emit(gen, "PUSHS LF@%%t0\n");
}

void genReadNumber(CodeGen *gen, const char *typeName, const char *zero) {
    int okLabel = newLabel(gen);

    // READ vrati pri chybnom vstupe alebo EOF nil, vtedy sa vracia 0
    emit(gen, "READ LF@%%t0 %s\n", typeName);
    emit(gen, "JUMPIFNEQ L$%d LF@%%t0 nil@nil\n", okLabel);
    emit(gen, "MOVE LF@%%t0 %s\n", zero);
    emit(gen, "LABEL L$%d\n", okLabel);
    emit(gen, "PUSHS LF@%%t0\n");
}

void genValueToStr(CodeGen *gen, Type *type) {
    type = getRealType(type);

    if (type->kind == TY_INT) {
        emit(gen, "INT2STRS\n");
    } else if (type->kind == TY_DOUBLE) {
        // FLOAT2STR pise vo formate %.2f, presne ako toStr
        emit(gen, "FLOAT2STRS\n");
    } else if (type->kind == TY_UNIT) {
        // hodnota () je nil, ako retazec je "()"
        emit(gen, "POPS LF@%%trash\n");
        emit(gen, "PUSHS string@()\n");
    }
    // retazec uz je retazec, nic sa nerobi
}

void genToStr(CodeGen *gen, CallNode *call) {
    genExpr(gen, call->args[0]);
    genValueToStr(gen, call->args[0]->type);
}

void genOrd(CodeGen *gen, CallNode *call) {
    int badLabel = newLabel(gen);
    int endLabel = newLabel(gen);

    genExpr(gen, call->args[0]);
    genExpr(gen, call->args[1]);
    emit(gen, "POPS LF@%%t1\n");
    emit(gen, "POPS LF@%%t0\n");

    // index mimo retazec (aj prazdny retazec) vracia -1
    emit(gen, "LT LF@%%t2 LF@%%t1 int@0\n");
    emit(gen, "JUMPIFEQ L$%d LF@%%t2 bool@true\n", badLabel);
    emit(gen, "STRLEN LF@%%t2 LF@%%t0\n");
    emit(gen, "LT LF@%%t2 LF@%%t1 LF@%%t2\n");
    emit(gen, "JUMPIFEQ L$%d LF@%%t2 bool@false\n", badLabel);

    emit(gen, "STRI2INT LF@%%t0 LF@%%t0 LF@%%t1\n");
    emit(gen, "PUSHS LF@%%t0\n");
    emit(gen, "JUMP L$%d\n", endLabel);

    emit(gen, "LABEL L$%d\n", badLabel);
    emit(gen, "PUSHS int@-1\n");
    emit(gen, "LABEL L$%d\n", endLabel);
}

void genJumpIfTrue(CodeGen *gen, const char *left, const char *operator, const char *right,
                   int label) {
    // vysledok porovnania ide na zasobnik a pri true sa skace
    emit(gen, "PUSHS %s\n", left);
    emit(gen, "PUSHS %s\n", right);
    emit(gen, "%s\n", operator);
    emit(gen, "PUSHS bool@true\n");
    emit(gen, "JUMPIFEQS L$%d\n", label);
}

void genSubStr(CodeGen *gen, CallNode *call) {
    int errorLabel = newLabel(gen);
    int okLabel = newLabel(gen);

    // retazec do %t0, zaciatok do %t1, koniec do %t2
    genExpr(gen, call->args[0]);
    genExpr(gen, call->args[1]);
    genExpr(gen, call->args[2]);
    emit(gen, "POPS LF@%%t2\n");
    emit(gen, "POPS LF@%%t1\n");
    emit(gen, "POPS LF@%%t0\n");
    emit(gen, "STRLEN LF@%%t3 LF@%%t0\n");

    // start < 0, end < 0, start > end, start >= dlzka, end > dlzka
    genJumpIfTrue(gen, "LF@%t1", "LTS", "int@0", errorLabel);
    genJumpIfTrue(gen, "LF@%t2", "LTS", "int@0", errorLabel);
    genJumpIfTrue(gen, "LF@%t1", "GTS", "LF@%t2", errorLabel);
    genJumpIfTrue(gen, "LF@%t1", "LTS", "LF@%t3", okLabel);
    emit(gen, "JUMP L$%d\n", errorLabel);
    emit(gen, "LABEL L$%d\n", okLabel);
    genJumpIfTrue(gen, "LF@%t2", "GTS", "LF@%t3", errorLabel);

    int loopLabel = newLabel(gen);
    int doneLabel = newLabel(gen);
    int endLabel = newLabel(gen);

    // znaky od start po end - 1 sa postupne pripajaju do %t3
    emit(gen, "MOVE LF@%%t3 string@\n");
    emit(gen, "LABEL L$%d\n", loopLabel);
    emit(gen, "PUSHS LF@%%t1\n");
    emit(gen, "PUSHS LF@%%t2\n");
    emit(gen, "LTS\n");
    emit(gen, "PUSHS bool@false\n");
    emit(gen, "JUMPIFEQS L$%d\n", doneLabel);
    emit(gen, "GETCHAR LF@%%t4 LF@%%t0 LF@%%t1\n");
    emit(gen, "CONCAT LF@%%t3 LF@%%t3 LF@%%t4\n");
    emit(gen, "ADD LF@%%t1 LF@%%t1 int@1\n");
    emit(gen, "JUMP L$%d\n", loopLabel);

    emit(gen, "LABEL L$%d\n", doneLabel);
    emit(gen, "PUSHS LF@%%t3\n");
    emit(gen, "JUMP L$%d\n", endLabel);

    emit(gen, "LABEL L$%d\n", errorLabel);
    emit(gen, "EXIT int@40\n");
    emit(gen, "LABEL L$%d\n", endLabel);
}

void genCall(CodeGen *gen, CallNode *call) {
    if (call->callKind == CALL_FUNC) {
        genFunctionCall(gen, call);
        return;
    }

    if (call->callKind != CALL_BUILTIN || call->caller->kind != AST_ID) {
        return;
    }

    const char *name = ((IDNode *)call->caller)->name;

    if (strcmp(name, "print") == 0) {
        genPrint(gen, call);
    } else if (strcmp(name, "readLine") == 0) {
        genReadLine(gen);
    } else if (strcmp(name, "readInt") == 0) {
        genReadNumber(gen, "int", "int@0");
    } else if (strcmp(name, "readDouble") == 0) {
        genReadNumber(gen, "float", "float@0x0p+0");
    } else if (strcmp(name, "toStr") == 0) {
        genToStr(gen, call);
    } else if (strcmp(name, "toInt") == 0) {
        // FLOAT2INT oseka desatinnu cast
        genExpr(gen, call->args[0]);
        emit(gen, "FLOAT2INTS\n");
    } else if (strcmp(name, "strLength") == 0) {
        // STRLEN nema zasobnikovu verziu
        genExpr(gen, call->args[0]);
        emit(gen, "POPS LF@%%t0\n");
        emit(gen, "STRLEN LF@%%t0 LF@%%t0\n");
        emit(gen, "PUSHS LF@%%t0\n");
    } else if (strcmp(name, "ord") == 0) {
        genOrd(gen, call);
    } else if (strcmp(name, "subStr") == 0) {
        genSubStr(gen, call);
    } else if (strcmp(name, "chr") == 0) {
        // hodnota mimo 0 az 127 skonci chybou interpretu 58
        genExpr(gen, call->args[0]);
        emit(gen, "INT2CHARS\n");
    }
}

void genBinary(CodeGen *gen, BinaryNode *binary) {
    // operandy sa vyhodnocuju zlava doprava, pravy je na vrchu zasobnika
    genExpr(gen, binary->left);
    genExpr(gen, binary->right);

    if (binary->operator == TOK_PLUS && binary->node.type->kind == TY_STRING) {
        // CONCAT nema zasobnikovu verziu, operandy sa vyberu do pomocnych premennych
        emit(gen, "POPS LF@%%t1\n");
        emit(gen, "POPS LF@%%t0\n");
        emit(gen, "CONCAT LF@%%t0 LF@%%t0 LF@%%t1\n");
        emit(gen, "PUSHS LF@%%t0\n");
    } else if (binary->operator == TOK_PLUS) {
        emit(gen, "ADDS\n");
    } else if (binary->operator == TOK_MINUS) {
        emit(gen, "SUBS\n");
    } else if (binary->operator == TOK_MULTIPLY) {
        emit(gen, "MULS\n");
    } else if (binary->operator == TOK_DIVIDE) {
        // delenie Double je desatinne, delenie Int celociselne
        if (binary->node.type->kind == TY_DOUBLE) {
            emit(gen, "DIVS\n");
        } else {
            emit(gen, "IDIVS\n");
        }
    } else if (binary->operator == TOK_LESS) {
        emit(gen, "LTS\n");
    } else if (binary->operator == TOK_GREATER) {
        emit(gen, "GTS\n");
    } else if (binary->operator == TOK_EQUAL) {
        emit(gen, "EQS\n");
    } else if (binary->operator == TOK_LESS_EQUAL) {
        // a <= b je to iste ako !(a > b)
        emit(gen, "GTS\n");
        emit(gen, "NOTS\n");
    } else if (binary->operator == TOK_GREATER_EQUAL) {
        // a >= b je to iste ako !(a < b)
        emit(gen, "LTS\n");
        emit(gen, "NOTS\n");
    } else if (binary->operator == TOK_NOT_EQUAL) {
        emit(gen, "EQS\n");
        emit(gen, "NOTS\n");
    }
}

void genUnary(CodeGen *gen, UnaryNode *unary) {
    genExpr(gen, unary->right);

    // -x sa pocita ako x * -1
    if (unary->operator == TOK_MINUS) {
        if (unary->node.type->kind == TY_DOUBLE) {
            emit(gen, "PUSHS float@%a\n", -1.0);
        } else {
            emit(gen, "PUSHS int@-1\n");
        }
        emit(gen, "MULS\n");
    }
}

void genExpr(CodeGen *gen, ASTNode *node) {
    // kazdy vyraz necha na zasobniku presne jednu hodnotu
    if (node->kind == AST_LIT) {
        emit(gen, "PUSHS ");
        emitLiteral(gen, (LiteralNode *)node);
        emit(gen, "\n");
    } else if (node->kind == AST_ID) {
        IDNode *id = (IDNode *)node;

        emit(gen, "PUSHS ");
        emitVar(gen, id->name, id->symbol);
        emit(gen, "\n");
    } else if (node->kind == AST_BINARY) {
        genBinary(gen, (BinaryNode *)node);
    } else if (node->kind == AST_UNARY) {
        genUnary(gen, (UnaryNode *)node);
    } else if (node->kind == AST_CONVERT) {
        // implicitny prevod Int na Double, ktory vlozila semantika
        genExpr(gen, ((ConvertNode *)node)->operand);
        emit(gen, "INT2FLOATS\n");
    } else if (node->kind == AST_CALL) {
        genCall(gen, (CallNode *)node);
    }
}

void genVarDef(CodeGen *gen, VarOrValNode *varDef) {
    // kazda premenna dostane vlastne cislo, takze prekryte premenne maju rozne mena
    gen->varCounter++;
    varDef->symbol->cgId = gen->varCounter;

    // DEFVAR ide na zaciatok, aby sa v cykle nevykonal viackrat
    emitDeclaration(gen, "DEFVAR LF@%s$%d\n", varDef->name, varDef->symbol->cgId);
    genExpr(gen, varDef->init);
    emit(gen, "POPS ");
    emitVar(gen, varDef->name, varDef->symbol);
    emit(gen, "\n");
}

void genAssign(CodeGen *gen, AssignNode *assign) {
    genExpr(gen, assign->value);
    emit(gen, "POPS ");
    emitVar(gen, assign->name, assign->symbol);
    emit(gen, "\n");
}

void genIf(CodeGen *gen, IfNode *ifNode) {
    int elseLabel = newLabel(gen);
    int endLabel = newLabel(gen);

    // pri nepravdivej podmienke sa skace na else vetvu
    genExpr(gen, ifNode->condition);
    emit(gen, "PUSHS bool@false\n");
    emit(gen, "JUMPIFEQS L$%d\n", elseLabel);

    genStatement(gen, ifNode->thenBranch);
    emit(gen, "JUMP L$%d\n", endLabel);

    emit(gen, "LABEL L$%d\n", elseLabel);
    genStatement(gen, ifNode->elseBranch);
    emit(gen, "LABEL L$%d\n", endLabel);
}

void genWhile(CodeGen *gen, WhileNode *whileNode) {
    int startLabel = newLabel(gen);
    int endLabel = newLabel(gen);

    emit(gen, "LABEL L$%d\n", startLabel);
    genExpr(gen, whileNode->condition);
    emit(gen, "PUSHS bool@false\n");
    emit(gen, "JUMPIFEQS L$%d\n", endLabel);

    genStatement(gen, whileNode->body);
    emit(gen, "JUMP L$%d\n", startLabel);
    emit(gen, "LABEL L$%d\n", endLabel);
}

void genReturn(CodeGen *gen, ReturnNode *returnNode) {
    if (isUnitType(gen->function->returnType)) {
        // funkcia typu Unit vrati () az na konci, hodnota za return sa len vyhodnoti
        if (returnNode->returnValue != NULL) {
            genExpr(gen, returnNode->returnValue);
            emit(gen, "POPS LF@%%trash\n");
        }
    } else if (returnNode->returnValue == NULL) {
        // prazdny return vracia ()
        emit(gen, "PUSHS nil@nil\n");
    } else {
        genExpr(gen, returnNode->returnValue);
    }

    emit(gen, "JUMP f$%s$end\n", gen->function->name);
}

void genBlock(CodeGen *gen, BlockNode *block) {
    for (size_t commandIndex = 0; commandIndex < block->commandOrderCount; commandIndex++) {
        genStatement(gen, block->commandOrder[commandIndex]);
    }
}

void genValue(CodeGen *gen, ASTNode *node) {
    if (node->kind == AST_BLOCK && ((BlockNode *)node)->commandOrderCount == 0) {
        // prazdny blok ma hodnotu ()
        emit(gen, "PUSHS nil@nil\n");
    } else if (node->kind == AST_BLOCK) {
        BlockNode *block = (BlockNode *)node;

        // hodnotu bloku urcuje jeho posledny prikaz
        for (size_t commandIndex = 0; commandIndex + 1 < block->commandOrderCount; commandIndex++) {
            genStatement(gen, block->commandOrder[commandIndex]);
        }
        genValue(gen, block->commandOrder[block->commandOrderCount - 1]);
    } else if (node->kind == AST_EXPR_STATEMENT) {
        genValue(gen, ((LonelyExprNode *)node)->expr);
    } else if (node->kind == AST_VAR_DEF || node->kind == AST_ASSIGN || node->kind == AST_WHILE ||
               node->kind == AST_IF || node->kind == AST_RETURN) {
        // definicia, priradenie, cyklus a podmieneny prikaz nemaju hodnotu, vysledok je ()
        // za returnom sa sem program nedostane
        genStatement(gen, node);
        emit(gen, "PUSHS nil@nil\n");
    } else {
        genExpr(gen, node);
    }
}

void genStatement(CodeGen *gen, ASTNode *node) {
    if (node == NULL) {
        return;
    }

    if (node->kind == AST_BLOCK) {
        genBlock(gen, (BlockNode *)node);
    } else if (node->kind == AST_EXPR_STATEMENT) {
        genStatement(gen, ((LonelyExprNode *)node)->expr);
    } else if (node->kind == AST_VAR_DEF) {
        genVarDef(gen, (VarOrValNode *)node);
    } else if (node->kind == AST_ASSIGN) {
        genAssign(gen, (AssignNode *)node);
    } else if (node->kind == AST_IF) {
        genIf(gen, (IfNode *)node);
    } else if (node->kind == AST_WHILE) {
        genWhile(gen, (WhileNode *)node);
    } else if (node->kind == AST_RETURN) {
        genReturn(gen, (ReturnNode *)node);
    } else if (node->kind == AST_CALL) {
        // vysledok volania sa nepouzije, zahodi sa
        genCall(gen, (CallNode *)node);
        emit(gen, "POPS LF@%%trash\n");
    } else {
        // vyraz ako prikaz sa vyhodnoti a jeho hodnota sa zahodi
        genExpr(gen, node);
        emit(gen, "POPS LF@%%trash\n");
    }
}

void writeBuffers(CodeGen *gen) {
    fputs(gen->declarations.data, gen->out);
    fputs(gen->code.data, gen->out);

    // buffre sa vyprazdnia pre dalsiu funkciu
    dynstr_clear(&gen->declarations);
    dynstr_clear(&gen->code);
}

void genParams(CodeGen *gen, DefFuncNode *function) {
    for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++) {
        ParamList *paramList = &function->paramList[listIndex];

        for (size_t paramIndex = 0; paramIndex < paramList->params_count; paramIndex++) {
            Param *param = &paramList->params[paramIndex];

            gen->varCounter++;
            param->symbol->cgId = gen->varCounter;
            emitDeclaration(gen, "DEFVAR LF@%s$%d\n", param->name, param->symbol->cgId);
        }
    }

    // argumenty su na zasobniku zlava doprava, takze sa vyberaju od posledneho
    for (size_t listIndex = function->paramListCount; listIndex > 0; listIndex--) {
        ParamList *paramList = &function->paramList[listIndex - 1];

        for (size_t paramIndex = paramList->params_count; paramIndex > 0; paramIndex--) {
            Param *param = &paramList->params[paramIndex - 1];

            emit(gen, "POPS ");
            emitVar(gen, param->name, param->symbol);
            emit(gen, "\n");
        }
    }
}

void genFunction(CodeGen *gen, DefFuncNode *function) {
    // pomocne premenne ma kazda funkcia vo svojom ramci
    emitHelperVars(gen);
    genParams(gen, function);

    gen->function = function;

    if (isUnitType(function->returnType)) {
        // pri type Unit sa hodnota tela zahodi a funkcia vrati ()
        genStatement(gen, function->body);
        emit(gen, "LABEL f$%s$end\n", function->name);
        emit(gen, "PUSHS nil@nil\n");
    } else {
        // navratova hodnota je hodnota tela, ostane na zasobniku
        genValue(gen, function->body);

        // sem skace return, hodnota uz je na zasobniku
        emit(gen, "LABEL f$%s$end\n", function->name);
    }

    if (gen->error != ERR_OK) {
        return;
    }

    // kazda funkcia ma vlastny lokalny ramec
    fprintf(gen->out, "LABEL f$%s\n", function->name);
    fprintf(gen->out, "CREATEFRAME\n");
    fprintf(gen->out, "PUSHFRAME\n");
    writeBuffers(gen);
    fprintf(gen->out, "POPFRAME\n");
    fprintf(gen->out, "RETURN\n");
}

void useHelper(CodeGen *gen, HelperKind helper) { gen->helperUsed[helper] = true; }

void writeHelper(CodeGen *gen, HelperKind helper) {
    // kazda pomocna funkcia ma vlastny ramec, argumenty berie zo zasobnika
    // a vysledok necha na zasobniku, rovnako ako funkcie z programu
    (void)helper;

    if (gen->error != ERR_OK) {
        return;
    }
    writeBuffers(gen);
}

void writeHelpers(CodeGen *gen) {
    for (int helper = 0; helper < HELPER_COUNT; helper++) {
        if (gen->helperUsed[helper]) {
            writeHelper(gen, (HelperKind)helper);
        }
    }
}

int codegenGenerate(ProgramNode *prog, FILE *out) {
    CodeGen gen;
    initCodeGen(&gen, out);

    DefFuncNode *mainFunction = findMain(prog);
    if (mainFunction == NULL) {
        gen.error = ERR_INTERNAL;
    }

    // program zacne volanim main a po navrate skonci
    if (gen.error == ERR_OK) {
        emitHeader(&gen);
        emit(&gen, "CALL f$main\n");
        emit(&gen, "EXIT int@0\n");
        writeBuffers(&gen);
    }

    for (size_t itemIndex = 0; itemIndex < prog->itemsCount && gen.error == ERR_OK; itemIndex++) {
        ASTNode *item = prog->items[itemIndex];

        if (item->kind == AST_FUNCTION_DEF) {
            genFunction(&gen, (DefFuncNode *)item);
        }
    }

    // pomocne funkcie za behu sa vypisu az za vsetkymi funkciami
    if (gen.error == ERR_OK) {
        writeHelpers(&gen);
    }

    int result = gen.error;
    freeCodeGen(&gen);
    return result;
}

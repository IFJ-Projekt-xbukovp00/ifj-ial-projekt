/**
 * @file test_semantic_body.c
 * @brief Unit testy kontroly tiel funkcií v sémantike (premenné, rozsahy, priradenie)
 *
 * Použitie AI: tento súbor vygenerovala AI (Claude Opus 5.5).
 * Prompt: "Vyrob testy ako pre zvyšok semntickeho analyzatora, tentokrát ale na ich telá,
 *   podľa zadania .zadanie, robíme so všetkými rožšíreniami."
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

#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>

int failureCount = 0;

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "%s:%d: CHECK zlyhal: %s\n", __FILE__, __LINE__, #condition);          \
            failureCount++;                                                                        \
        }                                                                                          \
    } while (0)

/* pomocné funkcie na AST, kým nie je ast.c */

ProgramNode *newProgram(void) {
    ProgramNode *program = calloc(1, sizeof(ProgramNode));
    program->node.kind = AST_PROGRAM;
    return program;
}

void addItem(ProgramNode *program, ASTNode *item) {
    program->items = realloc(program->items, (program->itemsCount + 1) * sizeof(ASTNode *));
    program->items[program->itemsCount++] = item;
}

DefFuncNode *newFunction(ProgramNode *program, const char *name, bool isMain, Type *returnType,
                         ASTNode *body) {
    DefFuncNode *function = calloc(1, sizeof(DefFuncNode));
    function->node.kind = AST_FUNCTION_DEF;
    function->name = (char *)name;
    function->isMain = isMain;
    function->returnType = returnType;
    function->body = body;
    addItem(program, (ASTNode *)function);
    return function;
}

void addParamList(DefFuncNode *function) {
    function->paramList =
        realloc(function->paramList, (function->paramListCount + 1) * sizeof(ParamList));
    function->paramList[function->paramListCount].params = NULL;
    function->paramList[function->paramListCount].params_count = 0;
    function->paramListCount++;
}

void addParam(DefFuncNode *function, const char *name, Type *type) {
    ParamList *paramList = &function->paramList[function->paramListCount - 1];
    paramList->params =
        realloc(paramList->params, (paramList->params_count + 1) * sizeof(Param));
    Param *param = &paramList->params[paramList->params_count++];
    param->name = (char *)name;
    param->type = type;
    param->line = 1;
    param->symbol = NULL;
}

/* @main def main(): Unit = body */
DefFuncNode *addMain(ProgramNode *program, ASTNode *body) {
    DefFuncNode *mainFunction =
        newFunction(program, "main", true, type_get_basic(TY_UNIT), body);
    addParamList(mainFunction);
    return mainFunction;
}

BlockNode *newBlock(void) {
    BlockNode *block = calloc(1, sizeof(BlockNode));
    block->node.kind = AST_BLOCK;
    return block;
}

void addCommand(BlockNode *block, ASTNode *command) {
    block->commandOrder =
        realloc(block->commandOrder, (block->commandOrderCount + 1) * sizeof(ASTNode *));
    block->commandOrder[block->commandOrderCount++] = command;
}

LiteralNode *newLiteral(LiteralType litType) {
    LiteralNode *literal = calloc(1, sizeof(LiteralNode));
    literal->node.kind = AST_LIT;
    literal->litType = litType;
    return literal;
}

ASTNode *newIntLiteral(long long value) {
    LiteralNode *literal = newLiteral(LIT_INT);
    literal->litValue.int_value = value;
    return (ASTNode *)literal;
}

ASTNode *newDoubleLiteral(double value) {
    LiteralNode *literal = newLiteral(LIT_DOUBLE);
    literal->litValue.float_value = value;
    return (ASTNode *)literal;
}

ASTNode *newStringLiteral(const char *value) {
    LiteralNode *literal = newLiteral(LIT_STRING);
    literal->litValue.string_value = (char *)value;
    return (ASTNode *)literal;
}

ASTNode *newBoolLiteral(bool value) {
    LiteralNode *literal = newLiteral(LIT_BOOLEAN);
    literal->litValue.bool_value = value;
    return (ASTNode *)literal;
}

ASTNode *newUnitLiteral(void) {
    return (ASTNode *)newLiteral(LIT_UNIT);
}

ASTNode *newId(const char *name) {
    IDNode *id = calloc(1, sizeof(IDNode));
    id->node.kind = AST_ID;
    id->name = (char *)name;
    return (ASTNode *)id;
}

ASTNode *newVarDef(const char *name, bool isMutable, Type *declaredType, ASTNode *init) {
    VarOrValNode *varDef = calloc(1, sizeof(VarOrValNode));
    varDef->node.kind = AST_VAR_DEF;
    varDef->name = (char *)name;
    varDef->isMutable = isMutable;
    varDef->declaredType = declaredType;
    varDef->init = init;
    return (ASTNode *)varDef;
}

ASTNode *newAssign(const char *name, ASTNode *value) {
    AssignNode *assign = calloc(1, sizeof(AssignNode));
    assign->node.kind = AST_ASSIGN;
    assign->name = (char *)name;
    assign->value = value;
    return (ASTNode *)assign;
}

ASTNode *newExprStatement(ASTNode *expr) {
    LonelyExprNode *statement = calloc(1, sizeof(LonelyExprNode));
    statement->node.kind = AST_EXPR_STATEMENT;
    statement->expr = expr;
    return (ASTNode *)statement;
}

/* uvoľnenie AST (aj ConvertNode, ktoré vložila sémantika) */

void freeNode(ASTNode *node) {
    if (node == NULL) {
        return;
    }

    if (node->kind == AST_BLOCK) {
        BlockNode *block = (BlockNode *)node;
        for (size_t commandIndex = 0; commandIndex < block->commandOrderCount; commandIndex++) {
            freeNode(block->commandOrder[commandIndex]);
        }
        free(block->commandOrder);
    } else if (node->kind == AST_VAR_DEF) {
        freeNode(((VarOrValNode *)node)->init);
    } else if (node->kind == AST_ASSIGN) {
        freeNode(((AssignNode *)node)->value);
    } else if (node->kind == AST_EXPR_STATEMENT) {
        freeNode(((LonelyExprNode *)node)->expr);
    } else if (node->kind == AST_CONVERT) {
        freeNode(((ConvertNode *)node)->operand);
    }

    free(node);
}

void freeFunction(DefFuncNode *function) {
    for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++) {
        free(function->paramList[listIndex].params);
    }
    free(function->paramList);
    free(function->locals);
    freeNode(function->body);
}

void freeProgram(ProgramNode *program) {
    for (size_t itemIndex = 0; itemIndex < program->itemsCount; itemIndex++) {
        ASTNode *item = program->items[itemIndex];
        if (item->kind == AST_FUNCTION_DEF) {
            freeFunction((DefFuncNode *)item);
        } else if (item->kind == AST_EXTENSION) {
            ExtensionNode *extension = (ExtensionNode *)item;
            for (size_t methodIndex = 0; methodIndex < extension->methodsCount; methodIndex++) {
                freeFunction((DefFuncNode *)extension->methods[methodIndex]);
                free(extension->methods[methodIndex]);
            }
            free(extension->methods);
        }
        free(item);
    }
    free(program->items);
    free(program);
}

/* spustí analýzu a vráti chybu; AST nechá na kontrolu, uvoľní ho finishAnalyze */

Symtable testTable;

ErrorCode runAnalyze(ProgramNode *program) {
    symtableInit(&testTable);
    ErrorCode error = ERR_OK;
    semanticAnalyze((ASTNode *)program, &testTable, &error);
    return error;
}

void finishAnalyze(ProgramNode *program) {
    symtableFree(&testTable);
    freeProgram(program);
}

/* spustí analýzu, všetko uvoľní a vráti chybu */
ErrorCode analyze(ProgramNode *program) {
    ErrorCode error = runAnalyze(program);
    finishAnalyze(program);
    return error;
}

/* program, ktorý má len @main s daným telom */
ErrorCode analyzeMainBody(BlockNode *body) {
    ProgramNode *program = newProgram();
    addMain(program, (ASTNode *)body);
    return analyze(program);
}

/* testy: nedefinované mená (3) */

void testUndefinedVariable(void) {
    /* print(y) -> zatiaľ len samotné y */
    BlockNode *body = newBlock();
    addCommand(body, newExprStatement(newId("y")));
    CHECK(analyzeMainBody(body) == ERR_UNDEF);
}

void testUseBeforeDefinition(void) {
    /* x ; val x = 1 */
    BlockNode *body = newBlock();
    addCommand(body, newExprStatement(newId("x")));
    addCommand(body, newVarDef("x", false, NULL, newIntLiteral(1)));
    CHECK(analyzeMainBody(body) == ERR_UNDEF);
}

void testOutOfScope(void) {
    /* { val y = 1 } ; y */
    BlockNode *inner = newBlock();
    addCommand(inner, newVarDef("y", false, NULL, newIntLiteral(1)));
    BlockNode *body = newBlock();
    addCommand(body, (ASTNode *)inner);
    addCommand(body, newExprStatement(newId("y")));
    CHECK(analyzeMainBody(body) == ERR_UNDEF);
}

void testUndefinedLocalType(void) {
    /* val x: Foo = 1 */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("x", false, type_create_named("Foo"), newIntLiteral(1)));
    CHECK(analyzeMainBody(body) == ERR_UNDEF);
}

void testAssignUndefined(void) {
    /* x = 1 */
    BlockNode *body = newBlock();
    addCommand(body, newAssign("x", newIntLiteral(1)));
    CHECK(analyzeMainBody(body) == ERR_UNDEF);
}

/* testy: redefinície (4) */

void testRedefineVariable(void) {
    /* val x = 1 ; val x = 2 */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("x", false, NULL, newIntLiteral(1)));
    addCommand(body, newVarDef("x", false, NULL, newIntLiteral(2)));
    CHECK(analyzeMainBody(body) == ERR_REDEF);
}

void testRedefineVarVal(void) {
    /* var x = 1 ; val x = 2 */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("x", true, NULL, newIntLiteral(1)));
    addCommand(body, newVarDef("x", false, NULL, newIntLiteral(2)));
    CHECK(analyzeMainBody(body) == ERR_REDEF);
}

void testShadowingInSubBlock(void) {
    /* val x = 1 ; { val x = "a" ; x } ; x */
    BlockNode *inner = newBlock();
    addCommand(inner, newVarDef("x", false, NULL, newStringLiteral("a")));
    ASTNode *innerUse = newId("x");
    addCommand(inner, newExprStatement(innerUse));

    BlockNode *body = newBlock();
    addCommand(body, newVarDef("x", false, NULL, newIntLiteral(1)));
    addCommand(body, (ASTNode *)inner);
    ASTNode *outerUse = newId("x");
    addCommand(body, newExprStatement(outerUse));

    ProgramNode *program = newProgram();
    addMain(program, (ASTNode *)body);
    CHECK(runAnalyze(program) == ERR_OK);
    CHECK(innerUse->type == type_get_basic(TY_STRING));
    CHECK(outerUse->type == type_get_basic(TY_INT));
    CHECK(((IDNode *)innerUse)->symbol != ((IDNode *)outerUse)->symbol);
    finishAnalyze(program);
}

void testParamShadowedInBody(void) {
    /* def f(a: Int): Int = { val a = 2 ; a } -> telo je vlastný rozsah */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("a", false, NULL, newIntLiteral(2)));
    addCommand(body, newExprStatement(newId("a")));

    ProgramNode *program = newProgram();
    DefFuncNode *function =
        newFunction(program, "f", false, type_get_basic(TY_INT), (ASTNode *)body);
    addParamList(function);
    addParam(function, "a", type_get_basic(TY_INT));
    addMain(program, (ASTNode *)newBlock());
    CHECK(analyze(program) == ERR_OK);
}

/* testy: typy (6) */

void testInitTypeMismatch(void) {
    /* val x: Int = 1.5 */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("x", false, type_get_basic(TY_INT), newDoubleLiteral(1.5)));
    CHECK(analyzeMainBody(body) == ERR_TYPE);
}

void testAssignOtherType(void) {
    /* var x = 1 ; x = "a" */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("x", true, NULL, newIntLiteral(1)));
    addCommand(body, newAssign("x", newStringLiteral("a")));
    CHECK(analyzeMainBody(body) == ERR_TYPE);
}

void testAssignDoubleToInt(void) {
    /* var x = 1 ; x = 1.5 */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("x", true, NULL, newIntLiteral(1)));
    addCommand(body, newAssign("x", newDoubleLiteral(1.5)));
    CHECK(analyzeMainBody(body) == ERR_TYPE);
}

void testBooleanVariable(void) {
    /* val b: Boolean = true ; var u = () (BOOLTHEN, Unit ako typ premennej) */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("b", false, type_get_basic(TY_BOOL), newBoolLiteral(true)));
    addCommand(body, newVarDef("u", true, NULL, newUnitLiteral()));
    CHECK(analyzeMainBody(body) == ERR_OK);
}

void testIntToDoubleInit(void) {
    /* val d: Double = 3 -> init je ConvertNode */
    BlockNode *body = newBlock();
    ASTNode *varDef = newVarDef("d", false, type_get_basic(TY_DOUBLE), newIntLiteral(3));
    addCommand(body, varDef);

    ProgramNode *program = newProgram();
    addMain(program, (ASTNode *)body);
    CHECK(runAnalyze(program) == ERR_OK);

    ASTNode *init = ((VarOrValNode *)varDef)->init;
    CHECK(init->kind == AST_CONVERT);
    CHECK(init->type == type_get_basic(TY_DOUBLE));
    CHECK(((ConvertNode *)init)->operand->kind == AST_LIT);
    CHECK(((ConvertNode *)init)->operand->type == type_get_basic(TY_INT));
    finishAnalyze(program);
}

void testAssignIntToDouble(void) {
    /* var d: Double = 1.5 ; d = 2 -> hodnota je ConvertNode */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("d", true, type_get_basic(TY_DOUBLE), newDoubleLiteral(1.5)));
    ASTNode *assign = newAssign("d", newIntLiteral(2));
    addCommand(body, assign);

    ProgramNode *program = newProgram();
    addMain(program, (ASTNode *)body);
    CHECK(runAnalyze(program) == ERR_OK);
    CHECK(((AssignNode *)assign)->value->kind == AST_CONVERT);
    finishAnalyze(program);
}

/* testy: nemodifikovateľné premenné (7) */

void testAssignVal(void) {
    /* val x = 1 ; x = 2 */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("x", false, NULL, newIntLiteral(1)));
    addCommand(body, newAssign("x", newIntLiteral(2)));
    CHECK(analyzeMainBody(body) == ERR_IMMUTABLE);
}

void testAssignParam(void) {
    /* def f(a: Int): Int = { a = 2 ; a } */
    BlockNode *body = newBlock();
    addCommand(body, newAssign("a", newIntLiteral(2)));
    addCommand(body, newExprStatement(newId("a")));

    ProgramNode *program = newProgram();
    DefFuncNode *function =
        newFunction(program, "f", false, type_get_basic(TY_INT), (ASTNode *)body);
    addParamList(function);
    addParam(function, "a", type_get_basic(TY_INT));
    addMain(program, (ASTNode *)newBlock());
    CHECK(analyze(program) == ERR_IMMUTABLE);
}

/* testy: vyplnené (S) polia */

void testLocalsFilled(void) {
    /* val a = 1 ; var b = "s" ; a */
    BlockNode *body = newBlock();
    ASTNode *first = newVarDef("a", false, NULL, newIntLiteral(1));
    ASTNode *second = newVarDef("b", true, NULL, newStringLiteral("s"));
    ASTNode *use = newId("a");
    addCommand(body, first);
    addCommand(body, second);
    addCommand(body, newExprStatement(use));

    ProgramNode *program = newProgram();
    DefFuncNode *mainFunction = addMain(program, (ASTNode *)body);
    CHECK(runAnalyze(program) == ERR_OK);

    CHECK(mainFunction->localsCount == 2);
    CHECK(mainFunction->locals[0] == ((VarOrValNode *)first)->symbol);
    CHECK(mainFunction->locals[1] == ((VarOrValNode *)second)->symbol);
    CHECK(((VarOrValNode *)second)->symbol->isMutable);
    CHECK(((IDNode *)use)->symbol == ((VarOrValNode *)first)->symbol);
    CHECK(((IDNode *)use)->idKind == ID_VAR);
    CHECK(use->type == type_get_basic(TY_INT));
    finishAnalyze(program);
}

void testParamSymbolFilled(void) {
    /* def f(a: Int): Int = a */
    ASTNode *use = newId("a");

    ProgramNode *program = newProgram();
    DefFuncNode *function = newFunction(program, "f", false, type_get_basic(TY_INT), use);
    addParamList(function);
    addParam(function, "a", type_get_basic(TY_INT));
    addMain(program, (ASTNode *)newBlock());
    CHECK(runAnalyze(program) == ERR_OK);

    Param *param = &function->paramList[0].params[0];
    CHECK(param->symbol != NULL);
    CHECK(param->symbol->kind == SYM_PARAMETER);
    CHECK(((IDNode *)use)->symbol == param->symbol);
    CHECK(function->localsCount == 0);
    finishAnalyze(program);
}

void testExtensionReceiverVisible(void) {
    /* extension (v: Int) def same: Int = v */
    ProgramNode *program = newProgram();
    ExtensionNode *extension = calloc(1, sizeof(ExtensionNode));
    extension->node.kind = AST_EXTENSION;
    extension->receiver.name = "v";
    extension->receiver.type = type_get_basic(TY_INT);
    addItem(program, (ASTNode *)extension);

    DefFuncNode *method = calloc(1, sizeof(DefFuncNode));
    method->node.kind = AST_FUNCTION_DEF;
    method->name = "same";
    method->returnType = type_get_basic(TY_INT);
    method->body = newId("v");
    extension->methods = malloc(sizeof(ASTNode *));
    extension->methods[0] = (ASTNode *)method;
    extension->methodsCount = 1;

    addMain(program, (ASTNode *)newBlock());
    CHECK(analyze(program) == ERR_OK);
}

void testBodyNotSeenByOtherFunction(void) {
    /* def f(): Unit = { val x = 1 } ; @main = x -> 3 */
    BlockNode *functionBody = newBlock();
    addCommand(functionBody, newVarDef("x", false, NULL, newIntLiteral(1)));

    ProgramNode *program = newProgram();
    DefFuncNode *function =
        newFunction(program, "f", false, type_get_basic(TY_UNIT), (ASTNode *)functionBody);
    addParamList(function);

    BlockNode *mainBody = newBlock();
    addCommand(mainBody, newExprStatement(newId("x")));
    addMain(program, (ASTNode *)mainBody);
    CHECK(analyze(program) == ERR_UNDEF);
}

int main(void) {
    testUndefinedVariable();
    testUseBeforeDefinition();
    testOutOfScope();
    testUndefinedLocalType();
    testAssignUndefined();

    testRedefineVariable();
    testRedefineVarVal();
    testShadowingInSubBlock();
    testParamShadowedInBody();

    testInitTypeMismatch();
    testAssignOtherType();
    testAssignDoubleToInt();
    testBooleanVariable();
    testIntToDoubleInit();
    testAssignIntToDouble();

    testAssignVal();
    testAssignParam();

    testLocalsFilled();
    testParamSymbolFilled();
    testExtensionReceiverVisible();
    testBodyNotSeenByOtherFunction();

    type_free_all();

    if (failureCount > 0) {
        fprintf(stderr, "test_semantic_body: %d chyb\n", failureCount);
        return 1;
    }

    printf("test_semantic_body: OK\n");
    return 0;
}

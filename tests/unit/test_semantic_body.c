/**
 * @file test_semantic_body.c
 * @brief Unit testy kontroly tiel funkcií v sémantike (premenné, rozsahy, priradenie, výrazy,
 *        podmienky, cykly, návratové typy)
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

ASTNode *newBinary(TokenType operator, ASTNode *left, ASTNode *right) {
    BinaryNode *binary = calloc(1, sizeof(BinaryNode));
    binary->node.kind = AST_BINARY;
    binary->operator = operator;
    binary->left = left;
    binary->right = right;
    return (ASTNode *)binary;
}

ASTNode *newUnary(TokenType operator, ASTNode *right) {
    UnaryNode *unary = calloc(1, sizeof(UnaryNode));
    unary->node.kind = AST_UNARY;
    unary->operator = operator;
    unary->right = right;
    return (ASTNode *)unary;
}

ASTNode *newIf(ASTNode *condition, ASTNode *thenBranch, ASTNode *elseBranch) {
    IfNode *ifNode = calloc(1, sizeof(IfNode));
    ifNode->node.kind = AST_IF;
    ifNode->condition = condition;
    ifNode->thenBranch = thenBranch;
    ifNode->elseBranch = elseBranch;
    return (ASTNode *)ifNode;
}

ASTNode *newWhile(ASTNode *condition, ASTNode *body) {
    WhileNode *whileNode = calloc(1, sizeof(WhileNode));
    whileNode->node.kind = AST_WHILE;
    whileNode->condition = condition;
    whileNode->body = body;
    return (ASTNode *)whileNode;
}

ASTNode *newReturn(ASTNode *returnValue) {
    ReturnNode *returnNode = calloc(1, sizeof(ReturnNode));
    returnNode->node.kind = AST_RETURN;
    returnNode->returnValue = returnValue;
    return (ASTNode *)returnNode;
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
    } else if (node->kind == AST_BINARY) {
        freeNode(((BinaryNode *)node)->left);
        freeNode(((BinaryNode *)node)->right);
    } else if (node->kind == AST_UNARY) {
        freeNode(((UnaryNode *)node)->right);
    } else if (node->kind == AST_IF) {
        freeNode(((IfNode *)node)->condition);
        freeNode(((IfNode *)node)->thenBranch);
        freeNode(((IfNode *)node)->elseBranch);
    } else if (node->kind == AST_WHILE) {
        freeNode(((WhileNode *)node)->condition);
        freeNode(((WhileNode *)node)->body);
    } else if (node->kind == AST_RETURN) {
        freeNode(((ReturnNode *)node)->returnValue);
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

/* testy: výrazy a operátory */

/* program s @main, ktorý má jediný príkaz val x = expr */
ErrorCode analyzeInitExpression(ASTNode *expr) {
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("x", false, NULL, expr));
    return analyzeMainBody(body);
}

void testAndOnInt(void) {
    /* val x = 1 && 2 */
    CHECK(analyzeInitExpression(newBinary(TOK_AND, newIntLiteral(1), newIntLiteral(2))) ==
          ERR_TYPE);
}

void testBoolArithmetic(void) {
    /* val x = true + 1 */
    CHECK(analyzeInitExpression(newBinary(TOK_PLUS, newBoolLiteral(true), newIntLiteral(1))) ==
          ERR_TYPE);
}

void testCompareStringInt(void) {
    /* val x = "a" < 1 */
    CHECK(analyzeInitExpression(newBinary(TOK_LESS, newStringLiteral("a"), newIntLiteral(1))) ==
          ERR_TYPE);
}

void testUnaryMinusString(void) {
    /* val x = -"a" */
    CHECK(analyzeInitExpression(newUnary(TOK_MINUS, newStringLiteral("a"))) == ERR_TYPE);
}

void testConcatStringInt(void) {
    /* val x = "a" + 1 (err_type_concat) */
    CHECK(analyzeInitExpression(newBinary(TOK_PLUS, newStringLiteral("a"), newIntLiteral(1))) ==
          ERR_TYPE);
}

void testEqualsDifferentTypes(void) {
    /* val x = "a" == 1 */
    CHECK(analyzeInitExpression(newBinary(TOK_EQUAL, newStringLiteral("a"), newIntLiteral(1))) ==
          ERR_TYPE);
}

void testNotOnInt(void) {
    /* val x = !1 */
    CHECK(analyzeInitExpression(newUnary(TOK_NOT, newIntLiteral(1))) == ERR_TYPE);
}

void testStringMinus(void) {
    /* val x = "a" - "b" (konkatenácia je len +) */
    CHECK(analyzeInitExpression(newBinary(TOK_MINUS, newStringLiteral("a"),
                                          newStringLiteral("b"))) == ERR_TYPE);
}

void testUndefinedInExpression(void) {
    /* val x = 1 + y */
    CHECK(analyzeInitExpression(newBinary(TOK_PLUS, newIntLiteral(1), newId("y"))) == ERR_UNDEF);
}

/* spustí analýzu val x = expr a vráti typ výrazu (AST ostane na kontrolu) */
Type *expressionType(ProgramNode **program, ASTNode **expr) {
    BlockNode *body = newBlock();
    ASTNode *varDef = newVarDef("x", false, NULL, *expr);
    addCommand(body, varDef);
    *program = newProgram();
    addMain(*program, (ASTNode *)body);

    if (runAnalyze(*program) != ERR_OK) {
        return NULL;
    }

    *expr = ((VarOrValNode *)varDef)->init;
    return (*expr)->type;
}

void testIntArithmetic(void) {
    /* 7 / 2 * 3 - 1 je Int */
    ProgramNode *program;
    ASTNode *expr = newBinary(
        TOK_MINUS,
        newBinary(TOK_MULTIPLY, newBinary(TOK_DIVIDE, newIntLiteral(7), newIntLiteral(2)),
                  newIntLiteral(3)),
        newIntLiteral(1));
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_INT));
    finishAnalyze(program);
}

void testIntPlusDouble(void) {
    /* 1 + 2.5 je Double, ľavý operand sa prevedie */
    ProgramNode *program;
    ASTNode *expr = newBinary(TOK_PLUS, newIntLiteral(1), newDoubleLiteral(2.5));
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_DOUBLE));
    CHECK(((BinaryNode *)expr)->left->kind == AST_CONVERT);
    CHECK(((BinaryNode *)expr)->right->kind == AST_LIT);
    finishAnalyze(program);
}

void testConvertInserted(void) {
    /* val d: Double = 3 ; val e = d + 1 -> pravý operand sa prevedie (ok_int_to_double) */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("d", false, type_get_basic(TY_DOUBLE), newIntLiteral(3)));
    ASTNode *sum = newBinary(TOK_PLUS, newId("d"), newIntLiteral(1));
    ASTNode *varDef = newVarDef("e", false, NULL, sum);
    addCommand(body, varDef);

    ProgramNode *program = newProgram();
    addMain(program, (ASTNode *)body);
    CHECK(runAnalyze(program) == ERR_OK);
    CHECK(sum->type == type_get_basic(TY_DOUBLE));
    CHECK(((BinaryNode *)sum)->left->kind == AST_ID);
    CHECK(((BinaryNode *)sum)->right->kind == AST_CONVERT);
    CHECK(((VarOrValNode *)varDef)->symbol->type == type_get_basic(TY_DOUBLE));
    finishAnalyze(program);
}

void testStringConcat(void) {
    /* "a" + "b" je String */
    ProgramNode *program;
    ASTNode *expr = newBinary(TOK_PLUS, newStringLiteral("a"), newStringLiteral("b"));
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_STRING));
    finishAnalyze(program);
}

void testRelationalOperators(void) {
    /* 1 <= 2.5 aj "a" > "b" sú Boolean */
    ProgramNode *program;
    ASTNode *expr = newBinary(TOK_LESS_EQUAL, newIntLiteral(1), newDoubleLiteral(2.5));
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_BOOL));
    CHECK(((BinaryNode *)expr)->left->kind == AST_CONVERT);
    finishAnalyze(program);

    expr = newBinary(TOK_GREATER, newStringLiteral("a"), newStringLiteral("b"));
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_BOOL));
    finishAnalyze(program);
}

void testEqualsMixedNumbers(void) {
    /* 1 != 1.0 je Boolean, Int sa prevedie */
    ProgramNode *program;
    ASTNode *expr = newBinary(TOK_NOT_EQUAL, newIntLiteral(1), newDoubleLiteral(1.0));
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_BOOL));
    CHECK(((BinaryNode *)expr)->left->kind == AST_CONVERT);
    finishAnalyze(program);
}

void testEqualsSameType(void) {
    /* "a" == "b" aj () == () sú Boolean */
    ProgramNode *program;
    ASTNode *expr = newBinary(TOK_EQUAL, newStringLiteral("a"), newStringLiteral("b"));
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_BOOL));
    finishAnalyze(program);

    expr = newBinary(TOK_EQUAL, newUnitLiteral(), newUnitLiteral());
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_BOOL));
    finishAnalyze(program);
}

void testBooleanOperators(void) {
    /* !true && 1 < 2 || false je Boolean (BOOLTHEN) */
    ProgramNode *program;
    ASTNode *expr = newBinary(
        TOK_OR,
        newBinary(TOK_AND, newUnary(TOK_NOT, newBoolLiteral(true)),
                  newBinary(TOK_LESS, newIntLiteral(1), newIntLiteral(2))),
        newBoolLiteral(false));
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_BOOL));
    finishAnalyze(program);
}

void testUnaryKeepsType(void) {
    /* -1.5 je Double, -1 je Int */
    ProgramNode *program;
    ASTNode *expr = newUnary(TOK_MINUS, newDoubleLiteral(1.5));
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_DOUBLE));
    finishAnalyze(program);

    expr = newUnary(TOK_MINUS, newIntLiteral(1));
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_INT));
    finishAnalyze(program);
}

void testExpressionWithVariables(void) {
    /* var x = 1 ; x = x * 2 + 1 */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("x", true, NULL, newIntLiteral(1)));
    addCommand(body,
               newAssign("x", newBinary(TOK_PLUS,
                                        newBinary(TOK_MULTIPLY, newId("x"), newIntLiteral(2)),
                                        newIntLiteral(1))));
    CHECK(analyzeMainBody(body) == ERR_OK);
}

void testAssignDoubleExpressionToInt(void) {
    /* var x = 1 ; x = x + 0.5 */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("x", true, NULL, newIntLiteral(1)));
    addCommand(body, newAssign("x", newBinary(TOK_PLUS, newId("x"), newDoubleLiteral(0.5))));
    CHECK(analyzeMainBody(body) == ERR_TYPE);
}

/* testy: podmienky a cykly */

/* blok s jediným výrazom */
ASTNode *blockOf(ASTNode *expr) {
    BlockNode *block = newBlock();
    addCommand(block, newExprStatement(expr));
    return (ASTNode *)block;
}

ASTNode *lessThan(long long left, long long right) {
    return newBinary(TOK_LESS, newIntLiteral(left), newIntLiteral(right));
}

void testIfConditionInt(void) {
    /* if 1 then 1 else 2 */
    BlockNode *body = newBlock();
    addCommand(body, newExprStatement(newIf(newIntLiteral(1), blockOf(newIntLiteral(1)),
                                            blockOf(newIntLiteral(2)))));
    CHECK(analyzeMainBody(body) == ERR_TYPE);
}

void testWhileConditionString(void) {
    /* while "a" do 1 */
    BlockNode *body = newBlock();
    addCommand(body, newWhile(newStringLiteral("a"), blockOf(newIntLiteral(1))));
    CHECK(analyzeMainBody(body) == ERR_TYPE);
}

void testIfConditionUndefined(void) {
    /* if y < 1 then 1 else 2 */
    BlockNode *body = newBlock();
    addCommand(body, newExprStatement(
                         newIf(newBinary(TOK_LESS, newId("y"), newIntLiteral(1)),
                               blockOf(newIntLiteral(1)), blockOf(newIntLiteral(2)))));
    CHECK(analyzeMainBody(body) == ERR_UNDEF);
}

void testIfBranchesMismatch(void) {
    /* val x = if 1 < 2 then 1 else "a" (err6_if_expression_branches) */
    CHECK(analyzeInitExpression(newIf(lessThan(1, 2), newIntLiteral(1),
                                      newStringLiteral("a"))) == ERR_TYPE);
}

void testIfBranchScope(void) {
    /* if 1 < 2 then { val y = 1 ; y } else () ; y (err3_out_of_scope) */
    BlockNode *thenBlock = newBlock();
    addCommand(thenBlock, newVarDef("y", false, NULL, newIntLiteral(1)));
    addCommand(thenBlock, newExprStatement(newId("y")));

    BlockNode *body = newBlock();
    addCommand(body, newExprStatement(newIf(lessThan(1, 2), (ASTNode *)thenBlock,
                                            blockOf(newUnitLiteral()))));
    addCommand(body, newExprStatement(newId("y")));
    CHECK(analyzeMainBody(body) == ERR_UNDEF);
}

void testSingleStatementBranchScope(void) {
    /* if 1 < 2 then val y = 1 else () ; y -> aj vetva bez bloku má vlastný rozsah */
    BlockNode *body = newBlock();
    addCommand(body, newExprStatement(newIf(lessThan(1, 2),
                                            newVarDef("y", false, NULL, newIntLiteral(1)),
                                            newUnitLiteral())));
    addCommand(body, newExprStatement(newId("y")));
    CHECK(analyzeMainBody(body) == ERR_UNDEF);
}

void testWhileBodyScope(void) {
    /* while 1 < 2 do { var i = 1 } ; i */
    BlockNode *loopBody = newBlock();
    addCommand(loopBody, newVarDef("i", true, NULL, newIntLiteral(1)));

    BlockNode *body = newBlock();
    addCommand(body, newWhile(lessThan(1, 2), (ASTNode *)loopBody));
    addCommand(body, newExprStatement(newId("i")));
    CHECK(analyzeMainBody(body) == ERR_UNDEF);
}

void testIfValueSameType(void) {
    /* val x = if 1 < 2 then "a" else "b" je String */
    ProgramNode *program;
    ASTNode *expr = newIf(lessThan(1, 2), newStringLiteral("a"), newStringLiteral("b"));
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_STRING));
    finishAnalyze(program);
}

void testIfValueIntDouble(void) {
    /* val x = if 1 < 2 then 1 else 2.5 je Double, then vetva sa prevedie */
    ProgramNode *program;
    ASTNode *expr = newIf(lessThan(1, 2), newIntLiteral(1), newDoubleLiteral(2.5));
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_DOUBLE));
    CHECK(((IfNode *)expr)->thenBranch->kind == AST_CONVERT);
    CHECK(((IfNode *)expr)->elseBranch->kind == AST_LIT);
    finishAnalyze(program);
}

void testIfValueWithoutElse(void) {
    /* val x = if 1 < 2 then 1 -> chýbajúce else je (), Int a Unit sa nezhodujú */
    CHECK(analyzeInitExpression(newIf(lessThan(1, 2), newIntLiteral(1), NULL)) == ERR_TYPE);
}

void testIfUnitValueWithoutElse(void) {
    /* val x = if 1 < 2 then () -> obe vetvy sú Unit */
    ProgramNode *program;
    ASTNode *expr = newIf(lessThan(1, 2), newUnitLiteral(), NULL);
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_UNIT));
    finishAnalyze(program);
}

void testElseIfChain(void) {
    /* val x = if 1 < 2 then 1 else if 2 < 3 then 2 else 3 je Int (BOOLTHEN) */
    ProgramNode *program;
    ASTNode *expr = newIf(lessThan(1, 2), newIntLiteral(1),
                          newIf(lessThan(2, 3), newIntLiteral(2), newIntLiteral(3)));
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_INT));
    finishAnalyze(program);
}

void testElseIfChainMismatch(void) {
    /* val x = if 1 < 2 then 1 else if 2 < 3 then "a" else 3 */
    CHECK(analyzeInitExpression(newIf(lessThan(1, 2), newIntLiteral(1),
                                      newIf(lessThan(2, 3), newStringLiteral("a"),
                                            newIntLiteral(3)))) == ERR_TYPE);
}

void testIfStatementMixedBranches(void) {
    /* if 1 < 2 then 1 else "a" ; () -> hodnota sa nepoužije, typy vetiev nevadia */
    BlockNode *body = newBlock();
    addCommand(body, newExprStatement(newIf(lessThan(1, 2), blockOf(newIntLiteral(1)),
                                            blockOf(newStringLiteral("a")))));
    addCommand(body, newExprStatement(newUnitLiteral()));
    CHECK(analyzeMainBody(body) == ERR_OK);
}

void testBooleanVariableCondition(void) {
    /* val b = true ; if b then () (BOOLTHEN: podmienka je ľubovoľný Boolean, bez else) */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("b", false, NULL, newBoolLiteral(true)));
    addCommand(body, newExprStatement(newIf(newId("b"), blockOf(newUnitLiteral()), NULL)));
    CHECK(analyzeMainBody(body) == ERR_OK);
}

void testWhileLoop(void) {
    /* var i = 0 ; while i < 10 do i = i + 1 */
    BlockNode *loopBody = newBlock();
    addCommand(loopBody, newAssign("i", newBinary(TOK_PLUS, newId("i"), newIntLiteral(1))));

    BlockNode *body = newBlock();
    addCommand(body, newVarDef("i", true, NULL, newIntLiteral(0)));
    ASTNode *loop = newWhile(newBinary(TOK_LESS, newId("i"), newIntLiteral(10)),
                             (ASTNode *)loopBody);
    addCommand(body, loop);

    ProgramNode *program = newProgram();
    addMain(program, (ASTNode *)body);
    CHECK(runAnalyze(program) == ERR_OK);
    CHECK(loop->type == type_get_basic(TY_UNIT));
    finishAnalyze(program);
}

void testWhileValueIsUnit(void) {
    /* val x = while 1 < 2 do "a" -> výsledok cyklu je Unit */
    ProgramNode *program;
    ASTNode *expr = newWhile(lessThan(1, 2), blockOf(newStringLiteral("a")));
    CHECK(expressionType(&program, &expr) == type_get_basic(TY_UNIT));
    finishAnalyze(program);
}

/* testy: návratové typy */

/* def name() [: returnType] = body, k tomu prázdny @main */
DefFuncNode *addFunctionWithMain(ProgramNode **program, const char *name, Type *returnType,
                                 ASTNode *body) {
    *program = newProgram();
    DefFuncNode *function = newFunction(*program, name, false, returnType, body);
    addParamList(function);
    addMain(*program, (ASTNode *)newBlock());
    return function;
}

/* skontroluje program s jednou funkciou a vráti chybu */
ErrorCode analyzeFunction(Type *returnType, ASTNode *body) {
    ProgramNode *program;
    addFunctionWithMain(&program, "f", returnType, body);
    return analyze(program);
}

/* def f(x: Int) [: returnType] = body */
ErrorCode analyzeFunctionWithParam(Type *returnType, ASTNode *body) {
    ProgramNode *program = newProgram();
    DefFuncNode *function = newFunction(program, "f", false, returnType, body);
    addParamList(function);
    addParam(function, "x", type_get_basic(TY_INT));
    addMain(program, (ASTNode *)newBlock());
    return analyze(program);
}

void testReturnWithoutReturnType(void) {
    /* def f(x: Int) = return x (err6_return_without_return_type) */
    BlockNode *body = newBlock();
    addCommand(body, newReturn(newId("x")));
    CHECK(analyzeFunctionWithParam(NULL, (ASTNode *)body) == ERR_TYPE);
}

void testReturnTypeMismatch(void) {
    /* def f(): Int = "a" (err6_return_type_mismatch) */
    CHECK(analyzeFunction(type_get_basic(TY_INT), newStringLiteral("a")) == ERR_TYPE);
}

void testReturnValueMismatch(void) {
    /* def f(): Int = { return "a" } */
    BlockNode *body = newBlock();
    addCommand(body, newReturn(newStringLiteral("a")));
    CHECK(analyzeFunction(type_get_basic(TY_INT), (ASTNode *)body) == ERR_TYPE);
}

void testReturnUndefinedValue(void) {
    /* def f(): Int = { return y } */
    BlockNode *body = newBlock();
    addCommand(body, newReturn(newId("y")));
    CHECK(analyzeFunction(type_get_basic(TY_INT), (ASTNode *)body) == ERR_UNDEF);
}

void testFunctionEndsWithIfWithoutElse(void) {
    /* def f(): Int = { if 1 < 2 then 1 } (err6_if_value_without_else) */
    BlockNode *body = newBlock();
    addCommand(body, newExprStatement(newIf(lessThan(1, 2), blockOf(newIntLiteral(1)), NULL)));
    CHECK(analyzeFunction(type_get_basic(TY_INT), (ASTNode *)body) == ERR_TYPE);
}

void testBlockEndingWithVarDef(void) {
    /* def f(): Int = { val a = 1 } -> výsledok bloku je () */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("a", false, NULL, newIntLiteral(1)));
    CHECK(analyzeFunction(type_get_basic(TY_INT), (ASTNode *)body) == ERR_TYPE);
}

void testBareReturnInIntFunction(void) {
    /* def f(): Int = { return } */
    BlockNode *body = newBlock();
    addCommand(body, newReturn(NULL));
    CHECK(analyzeFunction(type_get_basic(TY_INT), (ASTNode *)body) == ERR_TYPE);
}

void testBareReturnInInferredFunction(void) {
    /* def f() = { return } */
    BlockNode *body = newBlock();
    addCommand(body, newReturn(NULL));
    CHECK(analyzeFunction(NULL, (ASTNode *)body) == ERR_TYPE);
}

void testUnreachableCodeChecked(void) {
    /* def f(): Int = { return 1 ; y } -> kontroluje sa aj kód za return */
    BlockNode *body = newBlock();
    addCommand(body, newReturn(newIntLiteral(1)));
    addCommand(body, newExprStatement(newId("y")));
    CHECK(analyzeFunction(type_get_basic(TY_INT), (ASTNode *)body) == ERR_UNDEF);
}

void testAllBranchesReturn(void) {
    /* def sign(x: Int): Int = if x < 0 then { return -1 } else { return 1 }
       (ok_all_branches_return) */
    BlockNode *thenBlock = newBlock();
    addCommand(thenBlock, newReturn(newUnary(TOK_MINUS, newIntLiteral(1))));
    BlockNode *elseBlock = newBlock();
    addCommand(elseBlock, newReturn(newIntLiteral(1)));

    BlockNode *body = newBlock();
    addCommand(body, newExprStatement(newIf(newBinary(TOK_LESS, newId("x"), newIntLiteral(0)),
                                            (ASTNode *)thenBlock, (ASTNode *)elseBlock)));
    CHECK(analyzeFunctionWithParam(type_get_basic(TY_INT), (ASTNode *)body) == ERR_OK);
}

void testElseIfAllReturn(void) {
    /* def f(x: Int): Int = if x < 0 then return -1 else if x == 0 then return 0 else return 1 */
    ASTNode *innerIf = newIf(newBinary(TOK_EQUAL, newId("x"), newIntLiteral(0)),
                             newReturn(newIntLiteral(0)), newReturn(newIntLiteral(1)));
    ASTNode *outerIf = newIf(newBinary(TOK_LESS, newId("x"), newIntLiteral(0)),
                             newReturn(newUnary(TOK_MINUS, newIntLiteral(1))), innerIf);
    CHECK(analyzeFunctionWithParam(type_get_basic(TY_INT), outerIf) == ERR_OK);
}

void testOneBranchReturnsOtherValue(void) {
    /* def f(x: Int): Int = if x < 0 then { return 0 } else { x } */
    BlockNode *thenBlock = newBlock();
    addCommand(thenBlock, newReturn(newIntLiteral(0)));

    BlockNode *body = newBlock();
    addCommand(body, newExprStatement(newIf(newBinary(TOK_LESS, newId("x"), newIntLiteral(0)),
                                            (ASTNode *)thenBlock, blockOf(newId("x")))));
    CHECK(analyzeFunctionWithParam(type_get_basic(TY_INT), (ASTNode *)body) == ERR_OK);
}

void testReturnInsideWhile(void) {
    /* def f(): Int = { while 1 < 2 do { return 1 } ; 2 } */
    BlockNode *loopBody = newBlock();
    addCommand(loopBody, newReturn(newIntLiteral(1)));

    BlockNode *body = newBlock();
    addCommand(body, newWhile(lessThan(1, 2), (ASTNode *)loopBody));
    addCommand(body, newExprStatement(newIntLiteral(2)));
    CHECK(analyzeFunction(type_get_basic(TY_INT), (ASTNode *)body) == ERR_OK);
}

void testIfWithoutElseInMiddle(void) {
    /* def f(): Int = { if 1 < 2 then () ; 1 } */
    BlockNode *body = newBlock();
    addCommand(body, newExprStatement(newIf(lessThan(1, 2), blockOf(newUnitLiteral()), NULL)));
    addCommand(body, newExprStatement(newIntLiteral(1)));
    CHECK(analyzeFunction(type_get_basic(TY_INT), (ASTNode *)body) == ERR_OK);
}

void testUnitDiscardsValue(void) {
    /* def f(): Unit = 1 (ok_unit_discards_value) */
    CHECK(analyzeFunction(type_get_basic(TY_UNIT), newIntLiteral(1)) == ERR_OK);
}

void testUnitDiscardsReturnValue(void) {
    /* def f(): Unit = { return "a" } -> zahodí sa aj hodnota z return */
    BlockNode *body = newBlock();
    addCommand(body, newReturn(newStringLiteral("a")));
    CHECK(analyzeFunction(type_get_basic(TY_UNIT), (ASTNode *)body) == ERR_OK);
}

void testUnitReturnValueStillChecked(void) {
    /* def f(): Unit = { return y } -> hodnota sa zahodí, ale skontroluje */
    BlockNode *body = newBlock();
    addCommand(body, newReturn(newId("y")));
    CHECK(analyzeFunction(type_get_basic(TY_UNIT), (ASTNode *)body) == ERR_UNDEF);
}

void testBareReturnInUnitFunction(void) {
    /* def f(): Unit = { return } */
    BlockNode *body = newBlock();
    addCommand(body, newReturn(NULL));
    CHECK(analyzeFunction(type_get_basic(TY_UNIT), (ASTNode *)body) == ERR_OK);
}

void testUnitMixedIfBranches(void) {
    /* def f(): Unit = if 1 < 2 then 1 else "a" -> hodnota sa zahodí */
    CHECK(analyzeFunction(type_get_basic(TY_UNIT),
                          newIf(lessThan(1, 2), newIntLiteral(1), newStringLiteral("a"))) ==
          ERR_OK);
}

void testInferredUnitIf(void) {
    /* def f(x: Int) = if x < 0 then () else () -> odvodí sa Unit (ok_inferred_unit_if) */
    ProgramNode *program = newProgram();
    DefFuncNode *function =
        newFunction(program, "f", false, NULL,
                    newIf(newBinary(TOK_LESS, newId("x"), newIntLiteral(0)),
                          blockOf(newUnitLiteral()), blockOf(newUnitLiteral())));
    addParamList(function);
    addParam(function, "x", type_get_basic(TY_INT));
    addMain(program, (ASTNode *)newBlock());

    CHECK(runAnalyze(program) == ERR_OK);
    CHECK(function->returnType == NULL);
    CHECK(function->symbol->type->func.ret == type_get_basic(TY_UNIT));
    finishAnalyze(program);
}

void testInferredIntType(void) {
    /* def f() = { val a = 1 ; a + 1 } -> odvodí sa Int */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("a", false, NULL, newIntLiteral(1)));
    addCommand(body, newExprStatement(newBinary(TOK_PLUS, newId("a"), newIntLiteral(1))));

    ProgramNode *program;
    DefFuncNode *function = addFunctionWithMain(&program, "f", NULL, (ASTNode *)body);
    CHECK(runAnalyze(program) == ERR_OK);
    CHECK(function->symbol->type->func.ret == type_get_basic(TY_INT));
    finishAnalyze(program);
}

void testReturnConvert(void) {
    /* def f(): Double = 1 -> telo je ConvertNode */
    ProgramNode *program;
    DefFuncNode *function =
        addFunctionWithMain(&program, "f", type_get_basic(TY_DOUBLE), newIntLiteral(1));
    CHECK(runAnalyze(program) == ERR_OK);
    CHECK(function->body->kind == AST_CONVERT);
    finishAnalyze(program);
}

void testReturnValueConvert(void) {
    /* def f(): Double = { return 1 } -> hodnota v return je ConvertNode */
    BlockNode *body = newBlock();
    ASTNode *returnNode = newReturn(newIntLiteral(1));
    addCommand(body, returnNode);

    ProgramNode *program;
    addFunctionWithMain(&program, "f", type_get_basic(TY_DOUBLE), (ASTNode *)body);
    CHECK(runAnalyze(program) == ERR_OK);
    CHECK(((ReturnNode *)returnNode)->returnValue->kind == AST_CONVERT);
    finishAnalyze(program);
}

void testBlockResultConvert(void) {
    /* def f(): Double = { val a = 1 ; a } -> posledný výraz bloku je ConvertNode */
    BlockNode *body = newBlock();
    addCommand(body, newVarDef("a", false, NULL, newIntLiteral(1)));
    ASTNode *lastStatement = newExprStatement(newId("a"));
    addCommand(body, lastStatement);

    ProgramNode *program;
    addFunctionWithMain(&program, "f", type_get_basic(TY_DOUBLE), (ASTNode *)body);
    CHECK(runAnalyze(program) == ERR_OK);
    CHECK(((LonelyExprNode *)lastStatement)->expr->kind == AST_CONVERT);
    finishAnalyze(program);
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

    testAndOnInt();
    testBoolArithmetic();
    testCompareStringInt();
    testUnaryMinusString();
    testConcatStringInt();
    testEqualsDifferentTypes();
    testNotOnInt();
    testStringMinus();
    testUndefinedInExpression();

    testIntArithmetic();
    testIntPlusDouble();
    testConvertInserted();
    testStringConcat();
    testRelationalOperators();
    testEqualsMixedNumbers();
    testEqualsSameType();
    testBooleanOperators();
    testUnaryKeepsType();
    testExpressionWithVariables();
    testAssignDoubleExpressionToInt();

    testIfConditionInt();
    testWhileConditionString();
    testIfConditionUndefined();
    testIfBranchesMismatch();
    testIfBranchScope();
    testSingleStatementBranchScope();
    testWhileBodyScope();
    testIfValueSameType();
    testIfValueIntDouble();
    testIfValueWithoutElse();
    testIfUnitValueWithoutElse();
    testElseIfChain();
    testElseIfChainMismatch();
    testIfStatementMixedBranches();
    testBooleanVariableCondition();
    testWhileLoop();
    testWhileValueIsUnit();

    testReturnWithoutReturnType();
    testReturnTypeMismatch();
    testReturnValueMismatch();
    testReturnUndefinedValue();
    testFunctionEndsWithIfWithoutElse();
    testBlockEndingWithVarDef();
    testBareReturnInIntFunction();
    testBareReturnInInferredFunction();
    testUnreachableCodeChecked();
    testAllBranchesReturn();
    testElseIfAllReturn();
    testOneBranchReturnsOtherValue();
    testReturnInsideWhile();
    testIfWithoutElseInMiddle();
    testUnitDiscardsValue();
    testUnitDiscardsReturnValue();
    testUnitReturnValueStillChecked();
    testBareReturnInUnitFunction();
    testUnitMixedIfBranches();
    testInferredUnitIf();
    testInferredIntType();
    testReturnConvert();
    testReturnValueConvert();
    testBlockResultConvert();

    type_free_all();

    if (failureCount > 0) {
        fprintf(stderr, "test_semantic_body: %d chyb\n", failureCount);
        return 1;
    }

    printf("test_semantic_body: OK\n");
    return 0;
}

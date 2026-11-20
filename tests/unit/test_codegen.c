/**
 * @file test_codegen.c
 * @brief Unit testy generatora kodu IFJcode26 (zakladne zadanie, vsetky rozsirenia,
 *        behove chyby)
 *
 * Použitie AI: tento súbor vygenerovala AI (Claude Opus 5.5).
 * Prompt: "mame urobene unit testy na codegen? v tests/? ... nic necommituj, len ich vytvor
 *          zadanie je v .zadanie/ a pouzivame vsetky rozsirenia"
 *
 * Kazdy test poskladá AST rucne (parser ho zatial nevytvara), spusti semantiku a generator,
 * vygenerovany kod spusti v interprete ic26int a porovna vystup aj navratovy kod.
 * Zdrojovy program v IFJ26 je pri kazdom teste v komentari.
 * Interpret sa berie z premennej IC26INT (Makefile ju nastavi), inak sa testy preskocia.
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

#define _POSIX_C_SOURCE 200809L

#include "codegen.h"
#include "semantic.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int failureCount = 0;

/* pomocne funkcie na AST, kym parser AST nevytvara (uzly sa neuvolnuju) */

void *newNode(size_t size, ASTNodeKind kind) {
    ASTNode *node = calloc(1, size);
    node->kind = kind;
    return node;
}

char *copyText(const char *text) {
    char *copy = malloc(strlen(text) + 1);
    strcpy(copy, text);
    return copy;
}

ASTNode **collectNodes(int count, va_list args) {
    ASTNode **nodes = calloc((size_t)count + 1, sizeof(ASTNode *));
    for (int index = 0; index < count; index++) {
        nodes[index] = va_arg(args, ASTNode *);
    }
    return nodes;
}

Type *astUnion(int count, ...) {
    TypeList members = {NULL, 0, 0};
    va_list args;
    va_start(args, count);
    for (int index = 0; index < count; index++) {
        type_list_append(&members, va_arg(args, Type *));
    }
    va_end(args);
    return type_create_union(members);
}

Type *astFunctype(Type *returnType, int count, ...) {
    TypeList params = {NULL, 0, 0};
    va_list args;
    va_start(args, count);
    for (int index = 0; index < count; index++) {
        type_list_append(&params, va_arg(args, Type *));
    }
    va_end(args);
    return type_create_function(params, returnType);
}

ASTNode *astAlias(const char *name, Type *aliased) {
    TypeAliasNode *alias = newNode(sizeof(TypeAliasNode), AST_TYPE_ALIAS);
    alias->name = copyText(name);
    alias->aliased = aliased;
    return (ASTNode *)alias;
}

Param *astParam(const char *name, Type *type) {
    Param *param = calloc(1, sizeof(Param));
    param->name = copyText(name);
    param->type = type;
    param->line = 1;
    return param;
}

ParamList *astPlist(int count, ...) {
    ParamList *paramList = calloc(1, sizeof(ParamList));
    paramList->params = calloc((size_t)count + 1, sizeof(Param));
    paramList->params_count = (size_t)count;
    va_list args;
    va_start(args, count);
    for (int index = 0; index < count; index++) {
        paramList->params[index] = *va_arg(args, Param *);
    }
    va_end(args);
    return paramList;
}

ASTNode *astFunc(int line, const char *name, int isMain, Type *returnType, ASTNode *body,
                 int listCount, ...) {
    DefFuncNode *function = newNode(sizeof(DefFuncNode), AST_FUNCTION_DEF);
    function->node.line = line;
    function->name = copyText(name);
    function->isMain = isMain;
    function->returnType = returnType;
    function->body = body;
    function->paramListCount = (size_t)listCount;
    function->paramList = listCount > 0 ? calloc((size_t)listCount, sizeof(ParamList)) : NULL;
    va_list args;
    va_start(args, listCount);
    for (int index = 0; index < listCount; index++) {
        function->paramList[index] = *va_arg(args, ParamList *);
    }
    va_end(args);
    return (ASTNode *)function;
}

ASTNode *astClass(const char *name, ParamList *fields) {
    ClassDefNode *classDef = newNode(sizeof(ClassDefNode), AST_CLASS_DEF);
    classDef->name = copyText(name);
    classDef->fields = *fields;
    if (classDef->fields.params_count == 0) {
        classDef->fields.params = NULL;
    }
    return (ASTNode *)classDef;
}

ASTNode *astExtension(Param *receiver, int count, ...) {
    ExtensionNode *extension = newNode(sizeof(ExtensionNode), AST_EXTENSION);
    extension->receiver = *receiver;
    va_list args;
    va_start(args, count);
    extension->methods = collectNodes(count, args);
    va_end(args);
    extension->methodsCount = (size_t)count;
    return (ASTNode *)extension;
}

ASTNode *astProgram(int count, ...) {
    ProgramNode *program = newNode(sizeof(ProgramNode), AST_PROGRAM);
    va_list args;
    va_start(args, count);
    program->items = collectNodes(count, args);
    va_end(args);
    program->itemsCount = (size_t)count;
    return (ASTNode *)program;
}

ASTNode *astBlock(int count, ...) {
    BlockNode *block = newNode(sizeof(BlockNode), AST_BLOCK);
    va_list args;
    va_start(args, count);
    block->commandOrder = collectNodes(count, args);
    va_end(args);
    block->commandOrderCount = (size_t)count;
    return (ASTNode *)block;
}

ASTNode *astVardef(int line, const char *name, int isMutable, Type *declaredType, ASTNode *init) {
    VarOrValNode *varDef = newNode(sizeof(VarOrValNode), AST_VAR_DEF);
    varDef->node.line = line;
    varDef->name = copyText(name);
    varDef->isMutable = isMutable;
    varDef->declaredType = declaredType;
    varDef->init = init;
    return (ASTNode *)varDef;
}

ASTNode *astReturn(int line, ASTNode *value) {
    ReturnNode *returnNode = newNode(sizeof(ReturnNode), AST_RETURN);
    returnNode->node.line = line;
    returnNode->returnValue = value;
    return (ASTNode *)returnNode;
}

ASTNode *astWhile(int line, ASTNode *condition, ASTNode *body) {
    WhileNode *whileNode = newNode(sizeof(WhileNode), AST_WHILE);
    whileNode->node.line = line;
    whileNode->condition = condition;
    whileNode->body = body;
    return (ASTNode *)whileNode;
}

ASTNode *astFor(int line, const char *varName, ASTNode *source, ASTNode *body, int count, ...) {
    ForNode *forNode = newNode(sizeof(ForNode), AST_FOR);
    forNode->node.line = line;
    forNode->varName = copyText(varName);
    forNode->source = source;
    forNode->body = body;
    va_list args;
    va_start(args, count);
    forNode->guards = collectNodes(count, args);
    va_end(args);
    forNode->guardsCount = (size_t)count;
    return (ASTNode *)forNode;
}

ASTNode *astRange(ASTNode *from, ASTNode *to, ASTNode *step, int inclusive) {
    RangeNode *range = newNode(sizeof(RangeNode), AST_RANGE);
    range->from = from;
    range->to = to;
    range->step = step;
    range->inclusive = inclusive;
    return (ASTNode *)range;
}

ASTNode *astAssign(int line, const char *name, ASTNode *value) {
    AssignNode *assign = newNode(sizeof(AssignNode), AST_ASSIGN);
    assign->node.line = line;
    assign->name = copyText(name);
    assign->value = value;
    return (ASTNode *)assign;
}

ASTNode *astLonely(ASTNode *expr) {
    LonelyExprNode *lonely = newNode(sizeof(LonelyExprNode), AST_EXPR_STATEMENT);
    lonely->expr = expr;
    return (ASTNode *)lonely;
}

ASTNode *astIf(int line, ASTNode *condition, ASTNode *thenBranch, ASTNode *elseBranch) {
    IfNode *ifNode = newNode(sizeof(IfNode), AST_IF);
    ifNode->node.line = line;
    ifNode->condition = condition;
    ifNode->thenBranch = thenBranch;
    ifNode->elseBranch = elseBranch;
    return (ASTNode *)ifNode;
}

ASTNode *astBin(int line, TokenType operator, ASTNode *left, ASTNode *right) {
    BinaryNode *binary = newNode(sizeof(BinaryNode), AST_BINARY);
    binary->node.line = line;
    binary->operator = operator;
    binary->left = left;
    binary->right = right;
    return (ASTNode *)binary;
}

ASTNode *astUn(int line, TokenType operator, ASTNode *right) {
    UnaryNode *unary = newNode(sizeof(UnaryNode), AST_UNARY);
    unary->node.line = line;
    unary->operator = operator;
    unary->right = right;
    return (ASTNode *)unary;
}

ASTNode *astCall(int line, ASTNode *caller, int count, ...) {
    CallNode *call = newNode(sizeof(CallNode), AST_CALL);
    call->node.line = line;
    call->caller = caller;
    va_list args;
    va_start(args, count);
    call->args = collectNodes(count, args);
    va_end(args);
    call->argsCount = (size_t)count;
    return (ASTNode *)call;
}

ASTNode *astMember(ASTNode *object, const char *name) {
    MemberNode *member = newNode(sizeof(MemberNode), AST_MEMBER);
    member->object = object;
    member->name = copyText(name);
    return (ASTNode *)member;
}

ASTNode *astMatch(int line, ASTNode *subject, int count, ...) {
    MatchNode *match = newNode(sizeof(MatchNode), AST_MATCH);
    match->node.line = line;
    match->subject = subject;
    va_list args;
    va_start(args, count);
    match->cases = collectNodes(count, args);
    va_end(args);
    match->casesCount = (size_t)count;
    return (ASTNode *)match;
}

ASTNode *astCase(ASTNode *pattern, ASTNode *guard, ASTNode *body) {
    CaseNode *caseNode = (CaseNode *)pattern;
    caseNode->guard = guard;
    caseNode->body = body;
    return pattern;
}

ASTNode *astPatWild(void) {
    CaseNode *caseNode = newNode(sizeof(CaseNode), AST_CASE);
    caseNode->patternKind = PAT_WILDCARD;
    return (ASTNode *)caseNode;
}

ASTNode *astPatTyped(const char *name, Type *type) {
    CaseNode *caseNode = newNode(sizeof(CaseNode), AST_CASE);
    caseNode->patternKind = PAT_TYPED;
    caseNode->binder.name = copyText(name);
    caseNode->binder.type = type;
    return (ASTNode *)caseNode;
}

ASTNode *astPatCtor(const char *className, int count, ...) {
    CaseNode *caseNode = newNode(sizeof(CaseNode), AST_CASE);
    caseNode->patternKind = PAT_CONSTRUCTOR;
    caseNode->className = copyText(className);
    caseNode->ctorBinders.params = calloc((size_t)count + 1, sizeof(Param));
    caseNode->ctorBinders.params_count = (size_t)count;
    va_list args;
    va_start(args, count);
    for (int index = 0; index < count; index++) {
        caseNode->ctorBinders.params[index].name = copyText(va_arg(args, const char *));
    }
    va_end(args);
    return (ASTNode *)caseNode;
}

ASTNode *astPatLit(ASTNode *literal) {
    CaseNode *caseNode = newNode(sizeof(CaseNode), AST_CASE);
    caseNode->patternKind = PAT_LITERAL;
    caseNode->literal = literal;
    return (ASTNode *)caseNode;
}

LiteralNode *newLiteral(LiteralType litType) {
    LiteralNode *literal = newNode(sizeof(LiteralNode), AST_LIT);
    literal->litType = litType;
    return literal;
}

ASTNode *astInt(long long value) {
    LiteralNode *literal = newLiteral(LIT_INT);
    literal->litValue.int_value = value;
    return (ASTNode *)literal;
}

ASTNode *astDbl(double value) {
    LiteralNode *literal = newLiteral(LIT_DOUBLE);
    literal->litValue.float_value = value;
    return (ASTNode *)literal;
}

ASTNode *astStr(const char *value) {
    LiteralNode *literal = newLiteral(LIT_STRING);
    literal->litValue.string_value = copyText(value);
    return (ASTNode *)literal;
}

ASTNode *astBool(int value) {
    LiteralNode *literal = newLiteral(LIT_BOOLEAN);
    literal->litValue.bool_value = value;
    return (ASTNode *)literal;
}

ASTNode *astUnit(void) { return (ASTNode *)newLiteral(LIT_UNIT); }

ASTNode *astList(int count, ...) {
    ListLitNode *list = newNode(sizeof(ListLitNode), AST_LIST_LIT);
    va_list args;
    va_start(args, count);
    list->elems = collectNodes(count, args);
    va_end(args);
    list->elemsCount = (size_t)count;
    return (ASTNode *)list;
}

ASTNode *astId(int line, const char *name) {
    IDNode *id = newNode(sizeof(IDNode), AST_ID);
    id->node.line = line;
    id->name = copyText(name);
    return (ASTNode *)id;
}

ASTNode *astLambda(ParamList *params, ASTNode *body) {
    LambdaNode *lambda = newNode(sizeof(LambdaNode), AST_LAMBDA);
    lambda->params = *params;
    lambda->body = body;
    return (ASTNode *)lambda;
}

ASTNode *astInterp(int count, ...) {
    InterpNode *interp = newNode(sizeof(InterpNode), AST_INTERP);
    va_list args;
    va_start(args, count);
    interp->parts = collectNodes(count, args);
    va_end(args);
    interp->partsCount = (size_t)count;
    return (ASTNode *)interp;
}

/* testovane programy */

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = readInt()
 *     val b = readDouble()
 *     val c = readInt()
 *     val d = readDouble()
 *     val e = readInt()
 *     print(a)
 *     print(" ")
 *     print(b)
 *     print(" ")
 *     print(c)
 *     print(" ")
 *     print(d)
 *     print(" ")
 *     print(e)
 */
ASTNode *buildReadIntAndDouble(void) {
    return astProgram(
        1, astFunc(2, "main", 1, type_get_basic(TY_UNIT),
                   astBlock(14, astVardef(3, "a", 0, NULL, astCall(3, astId(3, "readInt"), 0)),
                            astVardef(4, "b", 0, NULL, astCall(4, astId(4, "readDouble"), 0)),
                            astVardef(5, "c", 0, NULL, astCall(5, astId(5, "readInt"), 0)),
                            astVardef(6, "d", 0, NULL, astCall(6, astId(6, "readDouble"), 0)),
                            astVardef(7, "e", 0, NULL, astCall(7, astId(7, "readInt"), 0)),
                            astLonely(astCall(8, astId(8, "print"), 1, astId(8, "a"))),
                            astLonely(astCall(9, astId(9, "print"), 1, astStr(" "))),
                            astLonely(astCall(10, astId(10, "print"), 1, astId(10, "b"))),
                            astLonely(astCall(11, astId(11, "print"), 1, astStr(" "))),
                            astLonely(astCall(12, astId(12, "print"), 1, astId(12, "c"))),
                            astLonely(astCall(13, astId(13, "print"), 1, astStr(" "))),
                            astLonely(astCall(14, astId(14, "print"), 1, astId(14, "d"))),
                            astLonely(astCall(15, astId(15, "print"), 1, astStr(" "))),
                            astLonely(astCall(16, astId(16, "print"), 1, astId(16, "e")))),
                   1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = toStr(42)
 *     val b = toStr(1.5)
 *     val c = toStr("x")
 *     val d = toStr(())
 *     val e = toStr(-7)
 *     val f = a + b + c + d + e
 *     print(f)
 */
ASTNode *buildToStr(void) {
    return astProgram(
        1, astFunc(
               2, "main", 1, type_get_basic(TY_UNIT),
               astBlock(
                   7, astVardef(3, "a", 0, NULL, astCall(3, astId(3, "toStr"), 1, astInt(42))),
                   astVardef(4, "b", 0, NULL,
                             astCall(4, astId(4, "toStr"), 1, astDbl(0x1.8000000000000p+0))),
                   astVardef(5, "c", 0, NULL, astCall(5, astId(5, "toStr"), 1, astStr("x"))),
                   astVardef(6, "d", 0, NULL, astCall(6, astId(6, "toStr"), 1, astUnit())),
                   astVardef(7, "e", 0, NULL,
                             astCall(7, astId(7, "toStr"), 1, astUn(7, TOK_MINUS, astInt(7)))),
                   astVardef(8, "f", 0, NULL,
                             astBin(8, TOK_PLUS,
                                    astBin(8, TOK_PLUS,
                                           astBin(8, TOK_PLUS,
                                                  astBin(8, TOK_PLUS, astId(8, "a"), astId(8, "b")),
                                                  astId(8, "c")),
                                           astId(8, "d")),
                                    astId(8, "e"))),
                   astLonely(astCall(9, astId(9, "print"), 1, astId(9, "f")))),
               1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = toInt(3.9)
 *     val b = toInt(-2.7)
 *     val c = strLength("hello")
 *     val d = strLength("")
 *     print(a)
 *     print(" ")
 *     print(b)
 *     print(" ")
 *     print(c)
 *     print(" ")
 *     print(d)
 */
ASTNode *buildToIntAndStrLength(void) {
    return astProgram(
        1, astFunc(2, "main", 1, type_get_basic(TY_UNIT),
                   astBlock(
                       11,
                       astVardef(3, "a", 0, NULL,
                                 astCall(3, astId(3, "toInt"), 1, astDbl(0x1.f333333333333p+1))),
                       astVardef(4, "b", 0, NULL,
                                 astCall(4, astId(4, "toInt"), 1,
                                         astUn(4, TOK_MINUS, astDbl(0x1.599999999999ap+1)))),
                       astVardef(5, "c", 0, NULL,
                                 astCall(5, astId(5, "strLength"), 1, astStr("hello"))),
                       astVardef(6, "d", 0, NULL, astCall(6, astId(6, "strLength"), 1, astStr(""))),
                       astLonely(astCall(7, astId(7, "print"), 1, astId(7, "a"))),
                       astLonely(astCall(8, astId(8, "print"), 1, astStr(" "))),
                       astLonely(astCall(9, astId(9, "print"), 1, astId(9, "b"))),
                       astLonely(astCall(10, astId(10, "print"), 1, astStr(" "))),
                       astLonely(astCall(11, astId(11, "print"), 1, astId(11, "c"))),
                       astLonely(astCall(12, astId(12, "print"), 1, astStr(" "))),
                       astLonely(astCall(13, astId(13, "print"), 1, astId(13, "d")))),
                   1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = ord("ABC", 0)
 *     val b = ord("ABC", 2)
 *     val c = ord("ABC", 3)
 *     val d = ord("ABC", -1)
 *     val e = ord("", 0)
 *     print(a)
 *     print(" ")
 *     print(b)
 *     print(" ")
 *     print(c)
 *     print(" ")
 *     print(d)
 *     print(" ")
 *     print(e)
 */
ASTNode *buildOrd(void) {
    return astProgram(
        1, astFunc(2, "main", 1, type_get_basic(TY_UNIT),
                   astBlock(14,
                            astVardef(3, "a", 0, NULL,
                                      astCall(3, astId(3, "ord"), 2, astStr("ABC"), astInt(0))),
                            astVardef(4, "b", 0, NULL,
                                      astCall(4, astId(4, "ord"), 2, astStr("ABC"), astInt(2))),
                            astVardef(5, "c", 0, NULL,
                                      astCall(5, astId(5, "ord"), 2, astStr("ABC"), astInt(3))),
                            astVardef(6, "d", 0, NULL,
                                      astCall(6, astId(6, "ord"), 2, astStr("ABC"),
                                              astUn(6, TOK_MINUS, astInt(1)))),
                            astVardef(7, "e", 0, NULL,
                                      astCall(7, astId(7, "ord"), 2, astStr(""), astInt(0))),
                            astLonely(astCall(8, astId(8, "print"), 1, astId(8, "a"))),
                            astLonely(astCall(9, astId(9, "print"), 1, astStr(" "))),
                            astLonely(astCall(10, astId(10, "print"), 1, astId(10, "b"))),
                            astLonely(astCall(11, astId(11, "print"), 1, astStr(" "))),
                            astLonely(astCall(12, astId(12, "print"), 1, astId(12, "c"))),
                            astLonely(astCall(13, astId(13, "print"), 1, astStr(" "))),
                            astLonely(astCall(14, astId(14, "print"), 1, astId(14, "d"))),
                            astLonely(astCall(15, astId(15, "print"), 1, astStr(" "))),
                            astLonely(astCall(16, astId(16, "print"), 1, astId(16, "e")))),
                   1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = chr(65)
 *     val b = chr(97)
 *     val c = a + b + chr(48)
 *     print(c)
 */
ASTNode *buildChr(void) {
    return astProgram(
        1,
        astFunc(2, "main", 1, type_get_basic(TY_UNIT),
                astBlock(
                    4, astVardef(3, "a", 0, NULL, astCall(3, astId(3, "chr"), 1, astInt(65))),
                    astVardef(4, "b", 0, NULL, astCall(4, astId(4, "chr"), 1, astInt(97))),
                    astVardef(5, "c", 0, NULL,
                              astBin(5, TOK_PLUS, astBin(5, TOK_PLUS, astId(5, "a"), astId(5, "b")),
                                     astCall(5, astId(5, "chr"), 1, astInt(48)))),
                    astLonely(astCall(6, astId(6, "print"), 1, astId(6, "c")))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = subStr("abcdef", 1, 3)
 *     val b = subStr("abcdef", 0, 6)
 *     val c = subStr("abcdef", 2, 2)
 *     print(a)
 *     print("|")
 *     print(b)
 *     print("|")
 *     print(c)
 *     print("|")
 */
ASTNode *buildSubStr(void) {
    return astProgram(
        1, astFunc(2, "main", 1, type_get_basic(TY_UNIT),
                   astBlock(9,
                            astVardef(3, "a", 0, NULL,
                                      astCall(3, astId(3, "subStr"), 3, astStr("abcdef"), astInt(1),
                                              astInt(3))),
                            astVardef(4, "b", 0, NULL,
                                      astCall(4, astId(4, "subStr"), 3, astStr("abcdef"), astInt(0),
                                              astInt(6))),
                            astVardef(5, "c", 0, NULL,
                                      astCall(5, astId(5, "subStr"), 3, astStr("abcdef"), astInt(2),
                                              astInt(2))),
                            astLonely(astCall(6, astId(6, "print"), 1, astId(6, "a"))),
                            astLonely(astCall(7, astId(7, "print"), 1, astStr("|"))),
                            astLonely(astCall(8, astId(8, "print"), 1, astId(8, "b"))),
                            astLonely(astCall(9, astId(9, "print"), 1, astStr("|"))),
                            astLonely(astCall(10, astId(10, "print"), 1, astId(10, "c"))),
                            astLonely(astCall(11, astId(11, "print"), 1, astStr("|")))),
                   1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val x = 10
 *     var i = 0
 *     while i < 3 do
 *         val x = i * 2
 *         val s = "a#b\\c d"
 *         print(x)
 *         print(s)
 *         i = i + 1
 *     print(x)
 *     print("\n")
 *     val d = 7 / 2
 *     val e = 7.0 / 2
 *     val f = -7 / 2
 *     val g = -(3 - 5)
 *     val h = 1 + 2 * 3 - 4 / 2
 *     print(d)
 *     print(" ")
 *     print(e)
 *     print(" ")
 *     print(f)
 *     print(" ")
 *     print(g)
 *     print(" ")
 *     print(h)
 *     print("\n")
 *     if "abc" < "abd" then
 *         print("lt")
 *     else
 *         print("ge")
 *     if 2 == 2.0 then
 *         print(" eq")
 *     else
 *         print(" ne")
 *     if "x" != "y" then
 *         print(" ne")
 *     else
 *         print(" eq")
 *     val u = ()
 *     print(u)
 *     val w = print(" w")
 *     print(w)
 *     var z = 1.5
 *     z = z * 2
 *     print(z)
 *     val big = 9223372036854775807
 *     print(big)
 *     print(" ")
 *     val neg = -2.5
 *     print(-neg)
 */
ASTNode *buildExpressionsAndVariables(void) {
    return astProgram(
        1,
        astFunc(
            2, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                35, astVardef(3, "x", 0, NULL, astInt(10)), astVardef(4, "i", 1, NULL, astInt(0)),
                astWhile(
                    5, astBin(5, TOK_LESS, astId(5, "i"), astInt(3)),
                    astBlock(5,
                             astVardef(6, "x", 0, NULL,
                                       astBin(6, TOK_MULTIPLY, astId(6, "i"), astInt(2))),
                             astVardef(7, "s", 0, NULL, astStr("a#b\134c d")),
                             astLonely(astCall(8, astId(8, "print"), 1, astId(8, "x"))),
                             astLonely(astCall(9, astId(9, "print"), 1, astId(9, "s"))),
                             astAssign(10, "i", astBin(10, TOK_PLUS, astId(10, "i"), astInt(1))))),
                astLonely(astCall(11, astId(11, "print"), 1, astId(11, "x"))),
                astLonely(astCall(12, astId(12, "print"), 1, astStr("\012"))),
                astVardef(13, "d", 0, NULL, astBin(13, TOK_DIVIDE, astInt(7), astInt(2))),
                astVardef(14, "e", 0, NULL,
                          astBin(14, TOK_DIVIDE, astDbl(0x1.c000000000000p+2), astInt(2))),
                astVardef(15, "f", 0, NULL,
                          astBin(15, TOK_DIVIDE, astUn(15, TOK_MINUS, astInt(7)), astInt(2))),
                astVardef(16, "g", 0, NULL,
                          astUn(16, TOK_MINUS, astBin(16, TOK_MINUS, astInt(3), astInt(5)))),
                astVardef(17, "h", 0, NULL,
                          astBin(17, TOK_MINUS,
                                 astBin(17, TOK_PLUS, astInt(1),
                                        astBin(17, TOK_MULTIPLY, astInt(2), astInt(3))),
                                 astBin(17, TOK_DIVIDE, astInt(4), astInt(2)))),
                astLonely(astCall(18, astId(18, "print"), 1, astId(18, "d"))),
                astLonely(astCall(19, astId(19, "print"), 1, astStr(" "))),
                astLonely(astCall(20, astId(20, "print"), 1, astId(20, "e"))),
                astLonely(astCall(21, astId(21, "print"), 1, astStr(" "))),
                astLonely(astCall(22, astId(22, "print"), 1, astId(22, "f"))),
                astLonely(astCall(23, astId(23, "print"), 1, astStr(" "))),
                astLonely(astCall(24, astId(24, "print"), 1, astId(24, "g"))),
                astLonely(astCall(25, astId(25, "print"), 1, astStr(" "))),
                astLonely(astCall(26, astId(26, "print"), 1, astId(26, "h"))),
                astLonely(astCall(27, astId(27, "print"), 1, astStr("\012"))),
                astLonely(astIf(
                    28, astBin(28, TOK_LESS, astStr("abc"), astStr("abd")),
                    astBlock(1, astLonely(astCall(29, astId(29, "print"), 1, astStr("lt")))),
                    astBlock(1, astLonely(astCall(31, astId(31, "print"), 1, astStr("ge")))))),
                astLonely(astIf(
                    32, astBin(32, TOK_EQUAL, astInt(2), astDbl(0x1.0000000000000p+1)),
                    astBlock(1, astLonely(astCall(33, astId(33, "print"), 1, astStr(" eq")))),
                    astBlock(1, astLonely(astCall(35, astId(35, "print"), 1, astStr(" ne")))))),
                astLonely(astIf(
                    36, astBin(36, TOK_NOT_EQUAL, astStr("x"), astStr("y")),
                    astBlock(1, astLonely(astCall(37, astId(37, "print"), 1, astStr(" ne")))),
                    astBlock(1, astLonely(astCall(39, astId(39, "print"), 1, astStr(" eq")))))),
                astVardef(40, "u", 0, NULL, astUnit()),
                astLonely(astCall(41, astId(41, "print"), 1, astId(41, "u"))),
                astVardef(42, "w", 0, NULL, astCall(42, astId(42, "print"), 1, astStr(" w"))),
                astLonely(astCall(43, astId(43, "print"), 1, astId(43, "w"))),
                astVardef(44, "z", 1, NULL, astDbl(0x1.8000000000000p+0)),
                astAssign(45, "z", astBin(45, TOK_MULTIPLY, astId(45, "z"), astInt(2))),
                astLonely(astCall(46, astId(46, "print"), 1, astId(46, "z"))),
                astVardef(47, "big", 0, NULL, astInt(9223372036854775807)),
                astLonely(astCall(48, astId(48, "print"), 1, astId(48, "big"))),
                astLonely(astCall(49, astId(49, "print"), 1, astStr(" "))),
                astVardef(50, "neg", 0, NULL, astUn(50, TOK_MINUS, astDbl(0x1.4000000000000p+1))),
                astLonely(
                    astCall(51, astId(51, "print"), 1, astUn(51, TOK_MINUS, astId(51, "neg"))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * def ord(s: String): Int = 7
 * def sub(a: Int, b: Int, c: Int): Int = a - b - c
 * def early(n: Int): Unit =
 *     if n > 5 then
 *         print("big")
 *         return
 *     else
 *         print("small")
 *     print("!")
 * def fib(n: Int): Int =
 *     if n < 2 then
 *         return n
 *     else
 *         val a = fib(n - 1)
 *         val b = fib(n - 2)
 *         return a + b
 * def inferred(x: Int) =
 *     val y = x + 1
 *     y * 2
 * def unitInferred() =
 *     print("u")
 * def emptyRet(): Unit =
 *     return 5
 * @main def run(): Unit =
 *     val a = ord("x")
 *     val b = sub(10, 3, 2)
 *     early(9)
 *     early(1)
 *     val c = fib(15)
 *     val d = inferred(4)
 *     val e = unitInferred()
 *     val f = emptyRet()
 *     print(a)
 *     print(" ")
 *     print(b)
 *     print(" ")
 *     print(c)
 *     print(" ")
 *     print(d)
 *     print(" ")
 *     print(e)
 *     print(f)
 *     val g = strLength(readLine())
 *     print(g)
 */
ASTNode *buildFunctions(void) {
    return astProgram(
        8,
        astFunc(2, "ord", 0, type_get_basic(TY_INT), astInt(7), 1,
                astPlist(1, astParam("s", type_get_basic(TY_STRING)))),
        astFunc(
            3, "sub", 0, type_get_basic(TY_INT),
            astBin(3, TOK_MINUS, astBin(3, TOK_MINUS, astId(3, "a"), astId(3, "b")), astId(3, "c")),
            1,
            astPlist(3, astParam("a", type_get_basic(TY_INT)),
                     astParam("b", type_get_basic(TY_INT)), astParam("c", type_get_basic(TY_INT)))),
        astFunc(4, "early", 0, type_get_basic(TY_UNIT),
                astBlock(
                    2,
                    astLonely(astIf(
                        5, astBin(5, TOK_GREATER, astId(5, "n"), astInt(5)),
                        astBlock(2, astLonely(astCall(6, astId(6, "print"), 1, astStr("big"))),
                                 astReturn(7, NULL)),
                        astBlock(1, astLonely(astCall(9, astId(9, "print"), 1, astStr("small")))))),
                    astLonely(astCall(10, astId(10, "print"), 1, astStr("!")))),
                1, astPlist(1, astParam("n", type_get_basic(TY_INT)))),
        astFunc(11, "fib", 0, type_get_basic(TY_INT),
                astBlock(
                    1,
                    astLonely(astIf(
                        12, astBin(12, TOK_LESS, astId(12, "n"), astInt(2)),
                        astBlock(1, astReturn(13, astId(13, "n"))),
                        astBlock(
                            3,
                            astVardef(15, "a", 0, NULL,
                                      astCall(15, astId(15, "fib"), 1,
                                              astBin(15, TOK_MINUS, astId(15, "n"), astInt(1)))),
                            astVardef(16, "b", 0, NULL,
                                      astCall(16, astId(16, "fib"), 1,
                                              astBin(16, TOK_MINUS, astId(16, "n"), astInt(2)))),
                            astReturn(17, astBin(17, TOK_PLUS, astId(17, "a"), astId(17, "b"))))))),
                1, astPlist(1, astParam("n", type_get_basic(TY_INT)))),
        astFunc(18, "inferred", 0, NULL,
                astBlock(
                    2, astVardef(19, "y", 0, NULL, astBin(19, TOK_PLUS, astId(19, "x"), astInt(1))),
                    astLonely(astBin(20, TOK_MULTIPLY, astId(20, "y"), astInt(2)))),
                1, astPlist(1, astParam("x", type_get_basic(TY_INT)))),
        astFunc(21, "unitInferred", 0, NULL,
                astBlock(1, astLonely(astCall(22, astId(22, "print"), 1, astStr("u")))), 1,
                astPlist(0)),
        astFunc(23, "emptyRet", 0, type_get_basic(TY_UNIT), astBlock(1, astReturn(24, astInt(5))),
                1, astPlist(0)),
        astFunc(25, "run", 1, type_get_basic(TY_UNIT),
                astBlock(
                    20, astVardef(26, "a", 0, NULL, astCall(26, astId(26, "ord"), 1, astStr("x"))),
                    astVardef(27, "b", 0, NULL,
                              astCall(27, astId(27, "sub"), 3, astInt(10), astInt(3), astInt(2))),
                    astLonely(astCall(28, astId(28, "early"), 1, astInt(9))),
                    astLonely(astCall(29, astId(29, "early"), 1, astInt(1))),
                    astVardef(30, "c", 0, NULL, astCall(30, astId(30, "fib"), 1, astInt(15))),
                    astVardef(31, "d", 0, NULL, astCall(31, astId(31, "inferred"), 1, astInt(4))),
                    astVardef(32, "e", 0, NULL, astCall(32, astId(32, "unitInferred"), 0)),
                    astVardef(33, "f", 0, NULL, astCall(33, astId(33, "emptyRet"), 0)),
                    astLonely(astCall(34, astId(34, "print"), 1, astId(34, "a"))),
                    astLonely(astCall(35, astId(35, "print"), 1, astStr(" "))),
                    astLonely(astCall(36, astId(36, "print"), 1, astId(36, "b"))),
                    astLonely(astCall(37, astId(37, "print"), 1, astStr(" "))),
                    astLonely(astCall(38, astId(38, "print"), 1, astId(38, "c"))),
                    astLonely(astCall(39, astId(39, "print"), 1, astStr(" "))),
                    astLonely(astCall(40, astId(40, "print"), 1, astId(40, "d"))),
                    astLonely(astCall(41, astId(41, "print"), 1, astStr(" "))),
                    astLonely(astCall(42, astId(42, "print"), 1, astId(42, "e"))),
                    astLonely(astCall(43, astId(43, "print"), 1, astId(43, "f"))),
                    astVardef(44, "g", 0, NULL,
                              astCall(44, astId(44, "strLength"), 1,
                                      astCall(44, astId(44, "readLine"), 0))),
                    astLonely(astCall(45, astId(45, "print"), 1, astId(45, "g")))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * def add(a: Int, b: Int): Int = a + b
 * def show(s: String): String =
 *     print(s)
 *     s
 * @main def main(): Unit =
 *     val r = add(add(1, 2), add(3, 4) * 2)
 *     print(r)
 *     print(toStr(r + strLength(subStr("abcd", 1, 3))))
 *     val x = add(strLength(show("a")), strLength(show("bc")))
 *     print(x)
 *     print(ord(subStr("xyz", 1, 2), 0))
 *     print(chr(ord("A", 0) + 1))
 */
ASTNode *buildFunexpArguments(void) {
    return astProgram(
        3,
        astFunc(2, "add", 0, type_get_basic(TY_INT),
                astBin(2, TOK_PLUS, astId(2, "a"), astId(2, "b")), 1,
                astPlist(2, astParam("a", type_get_basic(TY_INT)),
                         astParam("b", type_get_basic(TY_INT)))),
        astFunc(3, "show", 0, type_get_basic(TY_STRING),
                astBlock(2, astLonely(astCall(4, astId(4, "print"), 1, astId(4, "s"))),
                         astLonely(astId(5, "s"))),
                1, astPlist(1, astParam("s", type_get_basic(TY_STRING)))),
        astFunc(
            6, "main", 1, type_get_basic(TY_UNIT),
            astBlock(7,
                     astVardef(7, "r", 0, NULL,
                               astCall(7, astId(7, "add"), 2,
                                       astCall(7, astId(7, "add"), 2, astInt(1), astInt(2)),
                                       astBin(7, TOK_MULTIPLY,
                                              astCall(7, astId(7, "add"), 2, astInt(3), astInt(4)),
                                              astInt(2)))),
                     astLonely(astCall(8, astId(8, "print"), 1, astId(8, "r"))),
                     astLonely(astCall(
                         9, astId(9, "print"), 1,
                         astCall(9, astId(9, "toStr"), 1,
                                 astBin(9, TOK_PLUS, astId(9, "r"),
                                        astCall(9, astId(9, "strLength"), 1,
                                                astCall(9, astId(9, "subStr"), 3, astStr("abcd"),
                                                        astInt(1), astInt(3))))))),
                     astVardef(10, "x", 0, NULL,
                               astCall(10, astId(10, "add"), 2,
                                       astCall(10, astId(10, "strLength"), 1,
                                               astCall(10, astId(10, "show"), 1, astStr("a"))),
                                       astCall(10, astId(10, "strLength"), 1,
                                               astCall(10, astId(10, "show"), 1, astStr("bc"))))),
                     astLonely(astCall(11, astId(11, "print"), 1, astId(11, "x"))),
                     astLonely(astCall(12, astId(12, "print"), 1,
                                       astCall(12, astId(12, "ord"), 2,
                                               astCall(12, astId(12, "subStr"), 3, astStr("xyz"),
                                                       astInt(1), astInt(2)),
                                               astInt(0)))),
                     astLonely(astCall(
                         13, astId(13, "print"), 1,
                         astCall(13, astId(13, "chr"), 1,
                                 astBin(13, TOK_PLUS,
                                        astCall(13, astId(13, "ord"), 2, astStr("A"), astInt(0)),
                                        astInt(1)))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * def sq(a: Int): Int = a * a
 * def half(a: Double): Double = a / 2
 * @main def main(): Unit =
 *     val a = 2 * sq(3) + sq(sq(2)) - -sq(1)
 *     print(a)
 *     print(" ")
 *     if sq(2) < sq(3) then
 *         print("lt")
 *     else
 *         print("ge")
 *     print(" ")
 *     val b = half(sq(3)) + 1
 *     print(b)
 *     print(" ")
 *     val s = toStr(sq(5)) + "/" + toStr(half(1.0))
 *     print(s)
 *     var i = 0
 *     while i < sq(2) do
 *         i = i + 1
 *     print(i)
 */
ASTNode *buildFunexpOperand(void) {
    return astProgram(
        3,
        astFunc(2, "sq", 0, type_get_basic(TY_INT),
                astBin(2, TOK_MULTIPLY, astId(2, "a"), astId(2, "a")), 1,
                astPlist(1, astParam("a", type_get_basic(TY_INT)))),
        astFunc(3, "half", 0, type_get_basic(TY_DOUBLE),
                astBin(3, TOK_DIVIDE, astId(3, "a"), astInt(2)), 1,
                astPlist(1, astParam("a", type_get_basic(TY_DOUBLE)))),
        astFunc(
            4, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                13,
                astVardef(5, "a", 0, NULL,
                          astBin(5, TOK_MINUS,
                                 astBin(5, TOK_PLUS,
                                        astBin(5, TOK_MULTIPLY, astInt(2),
                                               astCall(5, astId(5, "sq"), 1, astInt(3))),
                                        astCall(5, astId(5, "sq"), 1,
                                                astCall(5, astId(5, "sq"), 1, astInt(2)))),
                                 astUn(5, TOK_MINUS, astCall(5, astId(5, "sq"), 1, astInt(1))))),
                astLonely(astCall(6, astId(6, "print"), 1, astId(6, "a"))),
                astLonely(astCall(7, astId(7, "print"), 1, astStr(" "))),
                astLonely(astIf(
                    8,
                    astBin(8, TOK_LESS, astCall(8, astId(8, "sq"), 1, astInt(2)),
                           astCall(8, astId(8, "sq"), 1, astInt(3))),
                    astBlock(1, astLonely(astCall(9, astId(9, "print"), 1, astStr("lt")))),
                    astBlock(1, astLonely(astCall(11, astId(11, "print"), 1, astStr("ge")))))),
                astLonely(astCall(12, astId(12, "print"), 1, astStr(" "))),
                astVardef(13, "b", 0, NULL,
                          astBin(13, TOK_PLUS,
                                 astCall(13, astId(13, "half"), 1,
                                         astCall(13, astId(13, "sq"), 1, astInt(3))),
                                 astInt(1))),
                astLonely(astCall(14, astId(14, "print"), 1, astId(14, "b"))),
                astLonely(astCall(15, astId(15, "print"), 1, astStr(" "))),
                astVardef(16, "s", 0, NULL,
                          astBin(16, TOK_PLUS,
                                 astBin(16, TOK_PLUS,
                                        astCall(16, astId(16, "toStr"), 1,
                                                astCall(16, astId(16, "sq"), 1, astInt(5))),
                                        astStr("/")),
                                 astCall(16, astId(16, "toStr"), 1,
                                         astCall(16, astId(16, "half"), 1,
                                                 astDbl(0x1.0000000000000p+0))))),
                astLonely(astCall(17, astId(17, "print"), 1, astId(17, "s"))),
                astVardef(18, "i", 1, NULL, astInt(0)),
                astWhile(19,
                         astBin(19, TOK_LESS, astId(19, "i"),
                                astCall(19, astId(19, "sq"), 1, astInt(2))),
                         astBlock(1, astAssign(20, "i",
                                               astBin(20, TOK_PLUS, astId(20, "i"), astInt(1))))),
                astLonely(astCall(21, astId(21, "print"), 1, astId(21, "i")))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * def flip(b: Boolean): Boolean =
 *     if b then
 *         return false
 *     else
 *         return true
 * @main def main(): Unit =
 *     val t = true
 *     var f = false
 *     f = flip(f)
 *     if t == f then
 *         print("same")
 *     else
 *         print("diff")
 *     if t != false then
 *         print(" ok")
 *     else
 *         print(" bad")
 *     val c = 1 < 2 && 3 > 4 || 2 == 2
 *     if c then
 *         print(" c")
 *     else
 *         print(" nc")
 */
ASTNode *buildBoolLiterals(void) {
    return astProgram(
        2,
        astFunc(2, "flip", 0, type_get_basic(TY_BOOL),
                astBlock(1, astLonely(astIf(3, astId(3, "b"), astBlock(1, astReturn(4, astBool(0))),
                                            astBlock(1, astReturn(6, astBool(1)))))),
                1, astPlist(1, astParam("b", type_get_basic(TY_BOOL)))),
        astFunc(
            7, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                7, astVardef(8, "t", 0, NULL, astBool(1)), astVardef(9, "f", 1, NULL, astBool(0)),
                astAssign(10, "f", astCall(10, astId(10, "flip"), 1, astId(10, "f"))),
                astLonely(astIf(
                    11, astBin(11, TOK_EQUAL, astId(11, "t"), astId(11, "f")),
                    astBlock(1, astLonely(astCall(12, astId(12, "print"), 1, astStr("same")))),
                    astBlock(1, astLonely(astCall(14, astId(14, "print"), 1, astStr("diff")))))),
                astLonely(astIf(
                    15, astBin(15, TOK_NOT_EQUAL, astId(15, "t"), astBool(0)),
                    astBlock(1, astLonely(astCall(16, astId(16, "print"), 1, astStr(" ok")))),
                    astBlock(1, astLonely(astCall(18, astId(18, "print"), 1, astStr(" bad")))))),
                astVardef(19, "c", 0, NULL,
                          astBin(19, TOK_OR,
                                 astBin(19, TOK_AND, astBin(19, TOK_LESS, astInt(1), astInt(2)),
                                        astBin(19, TOK_GREATER, astInt(3), astInt(4))),
                                 astBin(19, TOK_EQUAL, astInt(2), astInt(2)))),
                astLonely(astIf(
                    20, astId(20, "c"),
                    astBlock(1, astLonely(astCall(21, astId(21, "print"), 1, astStr(" c")))),
                    astBlock(1, astLonely(astCall(23, astId(23, "print"), 1, astStr(" nc"))))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = toStr(true) + "," + toStr(1 > 2)
 *     print(a)
 *     print(" ")
 *     print(false)
 *     print(3 == 3)
 */
ASTNode *buildBoolToStr(void) {
    return astProgram(
        1,
        astFunc(2, "main", 1, type_get_basic(TY_UNIT),
                astBlock(5,
                         astVardef(3, "a", 0, NULL,
                                   astBin(3, TOK_PLUS,
                                          astBin(3, TOK_PLUS,
                                                 astCall(3, astId(3, "toStr"), 1, astBool(1)),
                                                 astStr(",")),
                                          astCall(3, astId(3, "toStr"), 1,
                                                  astBin(3, TOK_GREATER, astInt(1), astInt(2))))),
                         astLonely(astCall(4, astId(4, "print"), 1, astId(4, "a"))),
                         astLonely(astCall(5, astId(5, "print"), 1, astStr(" "))),
                         astLonely(astCall(6, astId(6, "print"), 1, astBool(0))),
                         astLonely(astCall(7, astId(7, "print"), 1,
                                           astBin(7, TOK_EQUAL, astInt(3), astInt(3))))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * def side(v: Boolean): Boolean =
 *     print("s")
 *     v
 * @main def main(): Unit =
 *     val a = false && side(true)
 *     val b = true && side(false)
 *     val c = true && side(true) && side(true)
 *     print(a)
 *     print(b)
 *     print(c)
 */
ASTNode *buildShortCircuitAnd(void) {
    return astProgram(
        2,
        astFunc(2, "side", 0, type_get_basic(TY_BOOL),
                astBlock(2, astLonely(astCall(3, astId(3, "print"), 1, astStr("s"))),
                         astLonely(astId(4, "v"))),
                1, astPlist(1, astParam("v", type_get_basic(TY_BOOL)))),
        astFunc(5, "main", 1, type_get_basic(TY_UNIT),
                astBlock(6,
                         astVardef(6, "a", 0, NULL,
                                   astBin(6, TOK_AND, astBool(0),
                                          astCall(6, astId(6, "side"), 1, astBool(1)))),
                         astVardef(7, "b", 0, NULL,
                                   astBin(7, TOK_AND, astBool(1),
                                          astCall(7, astId(7, "side"), 1, astBool(0)))),
                         astVardef(8, "c", 0, NULL,
                                   astBin(8, TOK_AND,
                                          astBin(8, TOK_AND, astBool(1),
                                                 astCall(8, astId(8, "side"), 1, astBool(1))),
                                          astCall(8, astId(8, "side"), 1, astBool(1)))),
                         astLonely(astCall(9, astId(9, "print"), 1, astId(9, "a"))),
                         astLonely(astCall(10, astId(10, "print"), 1, astId(10, "b"))),
                         astLonely(astCall(11, astId(11, "print"), 1, astId(11, "c")))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * def side(v: Boolean): Boolean =
 *     print("s")
 *     v
 * @main def main(): Unit =
 *     val a = true || side(true)
 *     val b = false || side(false)
 *     val c = false || side(false) || side(true)
 *     val d = false && side(true) || side(true) && true
 *     print(a)
 *     print(b)
 *     print(c)
 *     print(d)
 */
ASTNode *buildShortCircuitOr(void) {
    return astProgram(
        2,
        astFunc(2, "side", 0, type_get_basic(TY_BOOL),
                astBlock(2, astLonely(astCall(3, astId(3, "print"), 1, astStr("s"))),
                         astLonely(astId(4, "v"))),
                1, astPlist(1, astParam("v", type_get_basic(TY_BOOL)))),
        astFunc(5, "main", 1, type_get_basic(TY_UNIT),
                astBlock(
                    8,
                    astVardef(
                        6, "a", 0, NULL,
                        astBin(6, TOK_OR, astBool(1), astCall(6, astId(6, "side"), 1, astBool(1)))),
                    astVardef(
                        7, "b", 0, NULL,
                        astBin(7, TOK_OR, astBool(0), astCall(7, astId(7, "side"), 1, astBool(0)))),
                    astVardef(8, "c", 0, NULL,
                              astBin(8, TOK_OR,
                                     astBin(8, TOK_OR, astBool(0),
                                            astCall(8, astId(8, "side"), 1, astBool(0))),
                                     astCall(8, astId(8, "side"), 1, astBool(1)))),
                    astVardef(9, "d", 0, NULL,
                              astBin(9, TOK_OR,
                                     astBin(9, TOK_AND, astBool(0),
                                            astCall(9, astId(9, "side"), 1, astBool(1))),
                                     astBin(9, TOK_AND, astCall(9, astId(9, "side"), 1, astBool(1)),
                                            astBool(1)))),
                    astLonely(astCall(10, astId(10, "print"), 1, astId(10, "a"))),
                    astLonely(astCall(11, astId(11, "print"), 1, astId(11, "b"))),
                    astLonely(astCall(12, astId(12, "print"), 1, astId(12, "c"))),
                    astLonely(astCall(13, astId(13, "print"), 1, astId(13, "d")))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * def f(n: Int): Int =
 *     if n > 3 then
 *         return 1
 *     if n > 1 then
 *         print("mid")
 *     0
 * @main def main(): Unit =
 *     var i = 0
 *     while i < 5 do
 *         if i == 2 then
 *             print("two")
 *         print(i)
 *         i = i + 1
 *     print(f(5))
 *     print(f(2))
 *     print(f(0))
 */
ASTNode *buildIfWithoutElse(void) {
    return astProgram(
        2,
        astFunc(2, "f", 0, type_get_basic(TY_INT),
                astBlock(3,
                         astLonely(astIf(3, astBin(3, TOK_GREATER, astId(3, "n"), astInt(3)),
                                         astBlock(1, astReturn(4, astInt(1))), NULL)),
                         astLonely(astIf(5, astBin(5, TOK_GREATER, astId(5, "n"), astInt(1)),
                                         astBlock(1, astLonely(astCall(6, astId(6, "print"), 1,
                                                                       astStr("mid")))),
                                         NULL)),
                         astLonely(astInt(0))),
                1, astPlist(1, astParam("n", type_get_basic(TY_INT)))),
        astFunc(
            8, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                5, astVardef(9, "i", 1, NULL, astInt(0)),
                astWhile(
                    10, astBin(10, TOK_LESS, astId(10, "i"), astInt(5)),
                    astBlock(3,
                             astLonely(astIf(11, astBin(11, TOK_EQUAL, astId(11, "i"), astInt(2)),
                                             astBlock(1, astLonely(astCall(12, astId(12, "print"),
                                                                           1, astStr("two")))),
                                             NULL)),
                             astLonely(astCall(13, astId(13, "print"), 1, astId(13, "i"))),
                             astAssign(14, "i", astBin(14, TOK_PLUS, astId(14, "i"), astInt(1))))),
                astLonely(
                    astCall(15, astId(15, "print"), 1, astCall(15, astId(15, "f"), 1, astInt(5)))),
                astLonely(
                    astCall(16, astId(16, "print"), 1, astCall(16, astId(16, "f"), 1, astInt(2)))),
                astLonely(
                    astCall(17, astId(17, "print"), 1, astCall(17, astId(17, "f"), 1, astInt(0))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * def sign(x: Int): Unit =
 *     if x < 0 then
 *         print("negative")
 *     else if x == 0 then
 *         print("zero")
 *     else if x < 10 then
 *         print("small")
 *     else
 *         print("big")
 *     print(";")
 * def maybe(x: Int): Unit =
 *     if x < 0 then
 *         print("n")
 *     else if x == 0 then
 *         print("z")
 *     print(".")
 * @main def main(): Unit =
 *     sign(-5)
 *     sign(0)
 *     sign(3)
 *     sign(30)
 *     maybe(-1)
 *     maybe(0)
 *     maybe(1)
 */
ASTNode *buildElseIf(void) {
    return astProgram(
        3,
        astFunc(
            2, "sign", 0, type_get_basic(TY_UNIT),
            astBlock(
                2,
                astLonely(astIf(
                    3, astBin(3, TOK_LESS, astId(3, "x"), astInt(0)),
                    astBlock(1, astLonely(astCall(4, astId(4, "print"), 1, astStr("negative")))),
                    astIf(5, astBin(5, TOK_EQUAL, astId(5, "x"), astInt(0)),
                          astBlock(1, astLonely(astCall(6, astId(6, "print"), 1, astStr("zero")))),
                          astIf(7, astBin(7, TOK_LESS, astId(7, "x"), astInt(10)),
                                astBlock(1, astLonely(
                                                astCall(8, astId(8, "print"), 1, astStr("small")))),
                                astBlock(1, astLonely(astCall(10, astId(10, "print"), 1,
                                                              astStr("big")))))))),
                astLonely(astCall(11, astId(11, "print"), 1, astStr(";")))),
            1, astPlist(1, astParam("x", type_get_basic(TY_INT)))),
        astFunc(12, "maybe", 0, type_get_basic(TY_UNIT),
                astBlock(2,
                         astLonely(astIf(13, astBin(13, TOK_LESS, astId(13, "x"), astInt(0)),
                                         astBlock(1, astLonely(astCall(14, astId(14, "print"), 1,
                                                                       astStr("n")))),
                                         astIf(15, astBin(15, TOK_EQUAL, astId(15, "x"), astInt(0)),
                                               astBlock(1, astLonely(astCall(16, astId(16, "print"),
                                                                             1, astStr("z")))),
                                               NULL))),
                         astLonely(astCall(17, astId(17, "print"), 1, astStr(".")))),
                1, astPlist(1, astParam("x", type_get_basic(TY_INT)))),
        astFunc(18, "main", 1, type_get_basic(TY_UNIT),
                astBlock(
                    7,
                    astLonely(astCall(19, astId(19, "sign"), 1, astUn(19, TOK_MINUS, astInt(5)))),
                    astLonely(astCall(20, astId(20, "sign"), 1, astInt(0))),
                    astLonely(astCall(21, astId(21, "sign"), 1, astInt(3))),
                    astLonely(astCall(22, astId(22, "sign"), 1, astInt(30))),
                    astLonely(astCall(23, astId(23, "maybe"), 1, astUn(23, TOK_MINUS, astInt(1)))),
                    astLonely(astCall(24, astId(24, "maybe"), 1, astInt(0))),
                    astLonely(astCall(25, astId(25, "maybe"), 1, astInt(1)))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = readBoolean()
 *     val b = readBoolean()
 *     val c = readBoolean()
 *     print(a)
 *     print(b)
 *     print(c)
 */
ASTNode *buildReadBoolean(void) {
    return astProgram(
        1, astFunc(2, "main", 1, type_get_basic(TY_UNIT),
                   astBlock(6, astVardef(3, "a", 0, NULL, astCall(3, astId(3, "readBoolean"), 0)),
                            astVardef(4, "b", 0, NULL, astCall(4, astId(4, "readBoolean"), 0)),
                            astVardef(5, "c", 0, NULL, astCall(5, astId(5, "readBoolean"), 0)),
                            astLonely(astCall(6, astId(6, "print"), 1, astId(6, "a"))),
                            astLonely(astCall(7, astId(7, "print"), 1, astId(7, "b"))),
                            astLonely(astCall(8, astId(8, "print"), 1, astId(8, "c")))),
                   1, astPlist(0)));
}

/*
 * import ifj26.*
 * def abs(x: Int): Int = if x < 0 then 0 - x else x
 * def pick(n: Int) =
 *     if n == 0 then "zero" else if n == 1 then "one" else "many"
 * @main def main(): Unit =
 *     var x = -5
 *     val absolute = if x < 0 then 0 - x else x
 *     print(absolute)
 *     print(abs(7))
 *     print(pick(0))
 *     print(pick(1))
 *     print(pick(5))
 *     val d = if x < 0 then 1 else 2.5
 *     print(d)
 *     print(if x > 0 then "p" else "n")
 *     val s = toStr(if true then 3 else 4) + "!"
 *     print(s)
 */
ASTNode *buildIfAsExpression(void) {
    return astProgram(
        3,
        astFunc(2, "abs", 0, type_get_basic(TY_INT),
                astIf(2, astBin(2, TOK_LESS, astId(2, "x"), astInt(0)),
                      astBin(2, TOK_MINUS, astInt(0), astId(2, "x")), astId(2, "x")),
                1, astPlist(1, astParam("x", type_get_basic(TY_INT)))),
        astFunc(3, "pick", 0, NULL,
                astBlock(1, astLonely(astIf(4, astBin(4, TOK_EQUAL, astId(4, "n"), astInt(0)),
                                            astStr("zero"),
                                            astIf(4, astBin(4, TOK_EQUAL, astId(4, "n"), astInt(1)),
                                                  astStr("one"), astStr("many"))))),
                1, astPlist(1, astParam("n", type_get_basic(TY_INT)))),
        astFunc(5, "main", 1, type_get_basic(TY_UNIT),
                astBlock(
                    12, astVardef(6, "x", 1, NULL, astUn(6, TOK_MINUS, astInt(5))),
                    astVardef(7, "absolute", 0, NULL,
                              astIf(7, astBin(7, TOK_LESS, astId(7, "x"), astInt(0)),
                                    astBin(7, TOK_MINUS, astInt(0), astId(7, "x")), astId(7, "x"))),
                    astLonely(astCall(8, astId(8, "print"), 1, astId(8, "absolute"))),
                    astLonely(astCall(9, astId(9, "print"), 1,
                                      astCall(9, astId(9, "abs"), 1, astInt(7)))),
                    astLonely(astCall(10, astId(10, "print"), 1,
                                      astCall(10, astId(10, "pick"), 1, astInt(0)))),
                    astLonely(astCall(11, astId(11, "print"), 1,
                                      astCall(11, astId(11, "pick"), 1, astInt(1)))),
                    astLonely(astCall(12, astId(12, "print"), 1,
                                      astCall(12, astId(12, "pick"), 1, astInt(5)))),
                    astVardef(13, "d", 0, NULL,
                              astIf(13, astBin(13, TOK_LESS, astId(13, "x"), astInt(0)), astInt(1),
                                    astDbl(0x1.4000000000000p+1))),
                    astLonely(astCall(14, astId(14, "print"), 1, astId(14, "d"))),
                    astLonely(astCall(15, astId(15, "print"), 1,
                                      astIf(15, astBin(15, TOK_GREATER, astId(15, "x"), astInt(0)),
                                            astStr("p"), astStr("n")))),
                    astVardef(16, "s", 0, NULL,
                              astBin(16, TOK_PLUS,
                                     astCall(16, astId(16, "toStr"), 1,
                                             astIf(16, astBool(1), astInt(3), astInt(4))),
                                     astStr("!"))),
                    astLonely(astCall(17, astId(17, "print"), 1, astId(17, "s")))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     var x = -5
 *     val abs2 = if x < 0 then
 *         0 - x
 *     else
 *         x
 *     val abs3 = if x < 0 then 0 - x else
 *         x
 *     print(abs2)
 *     print(abs3)
 *     val d = if x < 0 then
 *         val t = 1
 *         t + 1
 *     else 2.5
 *     print(d)
 *     if x < 0 then x = 1 else x = 2
 *     print(x)
 *     if x == 1 then print("one") else print("other")
 *     val y = if x > 100 then
 *         print("big")
 *         x
 *     else
 *         print("small")
 *         x + 1
 *     print(y)
 */
ASTNode *buildOneLineBranches(void) {
    return astProgram(
        1, astFunc(
               2, "main", 1, type_get_basic(TY_UNIT),
               astBlock(
                   12, astVardef(3, "x", 1, NULL, astUn(3, TOK_MINUS, astInt(5))),
                   astVardef(
                       4, "abs2", 0, NULL,
                       astIf(4, astBin(4, TOK_LESS, astId(4, "x"), astInt(0)),
                             astBlock(1, astLonely(astBin(5, TOK_MINUS, astInt(0), astId(5, "x")))),
                             astBlock(1, astLonely(astId(7, "x"))))),
                   astVardef(8, "abs3", 0, NULL,
                             astIf(8, astBin(8, TOK_LESS, astId(8, "x"), astInt(0)),
                                   astBin(8, TOK_MINUS, astInt(0), astId(8, "x")),
                                   astBlock(1, astLonely(astId(9, "x"))))),
                   astLonely(astCall(10, astId(10, "print"), 1, astId(10, "abs2"))),
                   astLonely(astCall(11, astId(11, "print"), 1, astId(11, "abs3"))),
                   astVardef(12, "d", 0, NULL,
                             astIf(12, astBin(12, TOK_LESS, astId(12, "x"), astInt(0)),
                                   astBlock(
                                       2, astVardef(13, "t", 0, NULL, astInt(1)),
                                       astLonely(astBin(14, TOK_PLUS, astId(14, "t"), astInt(1)))),
                                   astDbl(0x1.4000000000000p+1))),
                   astLonely(astCall(16, astId(16, "print"), 1, astId(16, "d"))),
                   astLonely(astIf(17, astBin(17, TOK_LESS, astId(17, "x"), astInt(0)),
                                   astAssign(17, "x", astInt(1)), astAssign(17, "x", astInt(2)))),
                   astLonely(astCall(18, astId(18, "print"), 1, astId(18, "x"))),
                   astLonely(astIf(19, astBin(19, TOK_EQUAL, astId(19, "x"), astInt(1)),
                                   astCall(19, astId(19, "print"), 1, astStr("one")),
                                   astCall(19, astId(19, "print"), 1, astStr("other")))),
                   astVardef(
                       20, "y", 0, NULL,
                       astIf(
                           20, astBin(20, TOK_GREATER, astId(20, "x"), astInt(100)),
                           astBlock(2, astLonely(astCall(21, astId(21, "print"), 1, astStr("big"))),
                                    astLonely(astId(22, "x"))),
                           astBlock(2,
                                    astLonely(astCall(24, astId(24, "print"), 1, astStr("small"))),
                                    astLonely(astBin(25, TOK_PLUS, astId(25, "x"), astInt(1)))))),
                   astLonely(astCall(26, astId(26, "print"), 1, astId(26, "y")))),
               1, astPlist(0)));
}

/*
 * import ifj26.*
 * def last(n: Int): Int =
 *     for i <- 1 to n do
 *         if i == 3 then
 *             return i
 *     0
 * @main def main(): Unit =
 *     for i <- 1 to 3 do print(i)
 *     print(" ")
 *     for i <- 5 to 1 do print(i)
 *     var n = 3
 *     for i <- 1 to n do
 *         n = n + 1
 *         val sq = i * i
 *         print(sq)
 *     print(" ")
 *     print(n)
 *     print(last(5))
 *     print(last(2))
 *     for i <- 1 to 2 do
 *         for j <- i to 2 do
 *             print(j)
 */
ASTNode *buildForTo(void) {
    return astProgram(
        2,
        astFunc(2, "last", 0, type_get_basic(TY_INT),
                astBlock(2,
                         astFor(3, "i", astRange(astInt(1), astId(3, "n"), NULL, 1),
                                astBlock(1, astLonely(astIf(
                                                4, astBin(4, TOK_EQUAL, astId(4, "i"), astInt(3)),
                                                astBlock(1, astReturn(5, astId(5, "i"))), NULL))),
                                0),
                         astLonely(astInt(0))),
                1, astPlist(1, astParam("n", type_get_basic(TY_INT)))),
        astFunc(
            7, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                10,
                astFor(8, "i", astRange(astInt(1), astInt(3), NULL, 1),
                       astCall(8, astId(8, "print"), 1, astId(8, "i")), 0),
                astLonely(astCall(9, astId(9, "print"), 1, astStr(" "))),
                astFor(10, "i", astRange(astInt(5), astInt(1), NULL, 1),
                       astCall(10, astId(10, "print"), 1, astId(10, "i")), 0),
                astVardef(11, "n", 1, NULL, astInt(3)),
                astFor(12, "i", astRange(astInt(1), astId(12, "n"), NULL, 1),
                       astBlock(3,
                                astAssign(13, "n", astBin(13, TOK_PLUS, astId(13, "n"), astInt(1))),
                                astVardef(14, "sq", 0, NULL,
                                          astBin(14, TOK_MULTIPLY, astId(14, "i"), astId(14, "i"))),
                                astLonely(astCall(15, astId(15, "print"), 1, astId(15, "sq")))),
                       0),
                astLonely(astCall(16, astId(16, "print"), 1, astStr(" "))),
                astLonely(astCall(17, astId(17, "print"), 1, astId(17, "n"))),
                astLonely(astCall(18, astId(18, "print"), 1,
                                  astCall(18, astId(18, "last"), 1, astInt(5)))),
                astLonely(astCall(19, astId(19, "print"), 1,
                                  astCall(19, astId(19, "last"), 1, astInt(2)))),
                astFor(20, "i", astRange(astInt(1), astInt(2), NULL, 1),
                       astBlock(1, astFor(21, "j", astRange(astId(21, "i"), astInt(2), NULL, 1),
                                          astBlock(1, astLonely(astCall(22, astId(22, "print"), 1,
                                                                        astId(22, "j")))),
                                          0)),
                       0)),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     for i <- 0 until 3 do print(i)
 *     print(" ")
 *     for i <- 3 until 3 do print(i)
 *     val s = "abc"
 *     for i <- 0 until strLength(s) do
 *         print(subStr(s, i, i + 1))
 */
ASTNode *buildForUntil(void) {
    return astProgram(
        1,
        astFunc(
            2, "main", 1, type_get_basic(TY_UNIT),
            astBlock(5,
                     astFor(3, "i", astRange(astInt(0), astInt(3), NULL, 0),
                            astCall(3, astId(3, "print"), 1, astId(3, "i")), 0),
                     astLonely(astCall(4, astId(4, "print"), 1, astStr(" "))),
                     astFor(5, "i", astRange(astInt(3), astInt(3), NULL, 0),
                            astCall(5, astId(5, "print"), 1, astId(5, "i")), 0),
                     astVardef(6, "s", 0, NULL, astStr("abc")),
                     astFor(7, "i",
                            astRange(astInt(0), astCall(7, astId(7, "strLength"), 1, astId(7, "s")),
                                     NULL, 0),
                            astBlock(1, astLonely(astCall(8, astId(8, "print"), 1,
                                                          astCall(8, astId(8, "subStr"), 3,
                                                                  astId(8, "s"), astId(8, "i"),
                                                                  astBin(8, TOK_PLUS, astId(8, "i"),
                                                                         astInt(1)))))),
                            0)),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     for i <- 1 to 10 by 2 do print(i)
 *     print(" ")
 *     for i <- 10 to 1 by -3 do print(i)
 *     print(" ")
 *     for i <- 10 until 1 by -3 do print(i)
 *     print(" ")
 *     for i <- 0 until 6 by 3 do print(i)
 *     print(" ")
 *     var st = -1
 *     for i <- 3 to 1 by st do
 *         st = 5
 *         print(i)
 *     print(" ")
 *     for i <- 1 to 3 by -1 do print(i)
 */
ASTNode *buildForBy(void) {
    return astProgram(
        1,
        astFunc(
            2, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                12,
                astFor(3, "i", astRange(astInt(1), astInt(10), astInt(2), 1),
                       astCall(3, astId(3, "print"), 1, astId(3, "i")), 0),
                astLonely(astCall(4, astId(4, "print"), 1, astStr(" "))),
                astFor(5, "i", astRange(astInt(10), astInt(1), astUn(5, TOK_MINUS, astInt(3)), 1),
                       astCall(5, astId(5, "print"), 1, astId(5, "i")), 0),
                astLonely(astCall(6, astId(6, "print"), 1, astStr(" "))),
                astFor(7, "i", astRange(astInt(10), astInt(1), astUn(7, TOK_MINUS, astInt(3)), 0),
                       astCall(7, astId(7, "print"), 1, astId(7, "i")), 0),
                astLonely(astCall(8, astId(8, "print"), 1, astStr(" "))),
                astFor(9, "i", astRange(astInt(0), astInt(6), astInt(3), 0),
                       astCall(9, astId(9, "print"), 1, astId(9, "i")), 0),
                astLonely(astCall(10, astId(10, "print"), 1, astStr(" "))),
                astVardef(11, "st", 1, NULL, astUn(11, TOK_MINUS, astInt(1))),
                astFor(12, "i", astRange(astInt(3), astInt(1), astId(12, "st"), 1),
                       astBlock(2, astAssign(13, "st", astInt(5)),
                                astLonely(astCall(14, astId(14, "print"), 1, astId(14, "i")))),
                       0),
                astLonely(astCall(15, astId(15, "print"), 1, astStr(" "))),
                astFor(16, "i", astRange(astInt(1), astInt(3), astUn(16, TOK_MINUS, astInt(1)), 1),
                       astCall(16, astId(16, "print"), 1, astId(16, "i")), 0)),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * def ok(i: Int): Boolean =
 *     print(".")
 *     i != 2
 * @main def main(): Unit =
 *     val limit = 10
 *     for
 *         i <- 0 until limit
 *         if i != 3
 *         if i < 8
 *     do
 *         print(i)
 *     print(" ")
 *     for i <- 1 to 4 if ok(i) do print(i)
 */
ASTNode *buildForGuards(void) {
    return astProgram(
        2,
        astFunc(2, "ok", 0, type_get_basic(TY_BOOL),
                astBlock(2, astLonely(astCall(3, astId(3, "print"), 1, astStr("."))),
                         astLonely(astBin(4, TOK_NOT_EQUAL, astId(4, "i"), astInt(2)))),
                1, astPlist(1, astParam("i", type_get_basic(TY_INT)))),
        astFunc(5, "main", 1, type_get_basic(TY_UNIT),
                astBlock(4, astVardef(6, "limit", 0, NULL, astInt(10)),
                         astFor(7, "i", astRange(astInt(0), astId(8, "limit"), NULL, 0),
                                astBlock(1, astLonely(astCall(12, astId(12, "print"), 1,
                                                              astId(12, "i")))),
                                2, astBin(9, TOK_NOT_EQUAL, astId(9, "i"), astInt(3)),
                                astBin(10, TOK_LESS, astId(10, "i"), astInt(8))),
                         astLonely(astCall(13, astId(13, "print"), 1, astStr(" "))),
                         astFor(14, "i", astRange(astInt(1), astInt(4), NULL, 1),
                                astCall(14, astId(14, "print"), 1, astId(14, "i")), 1,
                                astCall(14, astId(14, "ok"), 1, astId(14, "i")))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val k = 42
 *     val e = k.toString + "!"
 *     print(e)
 *     print(1.2345.toString)
 *     print(true.toString)
 *     print("s".toString)
 *     print(().toString)
 */
ASTNode *buildValmemToString(void) {
    return astProgram(
        1,
        astFunc(
            2, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                7, astVardef(3, "k", 0, NULL, astInt(42)),
                astVardef(4, "e", 0, NULL,
                          astBin(4, TOK_PLUS, astMember(astId(4, "k"), "toString"), astStr("!"))),
                astLonely(astCall(5, astId(5, "print"), 1, astId(5, "e"))),
                astLonely(astCall(6, astId(6, "print"), 1,
                                  astMember(astDbl(0x1.3c083126e978dp+0), "toString"))),
                astLonely(astCall(7, astId(7, "print"), 1, astMember(astBool(1), "toString"))),
                astLonely(astCall(8, astId(8, "print"), 1, astMember(astStr("s"), "toString"))),
                astLonely(astCall(9, astId(9, "print"), 1, astMember(astUnit(), "toString")))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val d = 3.7
 *     print(d.toInt)
 *     print(" ")
 *     print(-2.5.toInt)
 *     print(" ")
 *     print("abc".length)
 *     print(" ")
 *     val s = "hello" + "!"
 *     print(s.length + 1)
 */
ASTNode *buildValmemToIntLength(void) {
    return astProgram(
        1,
        astFunc(2, "main", 1, type_get_basic(TY_UNIT),
                astBlock(
                    9, astVardef(3, "d", 0, NULL, astDbl(0x1.d99999999999ap+1)),
                    astLonely(astCall(4, astId(4, "print"), 1, astMember(astId(4, "d"), "toInt"))),
                    astLonely(astCall(5, astId(5, "print"), 1, astStr(" "))),
                    astLonely(astCall(
                        6, astId(6, "print"), 1,
                        astUn(6, TOK_MINUS, astMember(astDbl(0x1.4000000000000p+1), "toInt")))),
                    astLonely(astCall(7, astId(7, "print"), 1, astStr(" "))),
                    astLonely(astCall(8, astId(8, "print"), 1, astMember(astStr("abc"), "length"))),
                    astLonely(astCall(9, astId(9, "print"), 1, astStr(" "))),
                    astVardef(10, "s", 0, NULL, astBin(10, TOK_PLUS, astStr("hello"), astStr("!"))),
                    astLonely(astCall(
                        11, astId(11, "print"), 1,
                        astBin(11, TOK_PLUS, astMember(astId(11, "s"), "length"), astInt(1))))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = 3.7
 *     val b = -2.5
 *     val c = 4.0
 *     val d = -0.5
 *     print(a.floor)
 *     print(" ")
 *     print(b.floor)
 *     print(" ")
 *     print(c.floor)
 *     print(" ")
 *     print(d.floor)
 */
ASTNode *buildValmemFloor(void) {
    return astProgram(
        1,
        astFunc(
            2, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                11, astVardef(3, "a", 0, NULL, astDbl(0x1.d99999999999ap+1)),
                astVardef(4, "b", 0, NULL, astUn(4, TOK_MINUS, astDbl(0x1.4000000000000p+1))),
                astVardef(5, "c", 0, NULL, astDbl(0x1.0000000000000p+2)),
                astVardef(6, "d", 0, NULL, astUn(6, TOK_MINUS, astDbl(0x1.0000000000000p-1))),
                astLonely(astCall(7, astId(7, "print"), 1, astMember(astId(7, "a"), "floor"))),
                astLonely(astCall(8, astId(8, "print"), 1, astStr(" "))),
                astLonely(astCall(9, astId(9, "print"), 1, astMember(astId(9, "b"), "floor"))),
                astLonely(astCall(10, astId(10, "print"), 1, astStr(" "))),
                astLonely(astCall(11, astId(11, "print"), 1, astMember(astId(11, "c"), "floor"))),
                astLonely(astCall(12, astId(12, "print"), 1, astStr(" "))),
                astLonely(astCall(13, astId(13, "print"), 1, astMember(astId(13, "d"), "floor")))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * def half(x: Double): Double = x / 2
 * @main def main(): Unit =
 *     val n = -3.5
 *     print(n.floor.toInt)
 *     print(" ")
 *     print(n.floor.toInt.toString.length)
 *     print(" ")
 *     print(half(7.0).floor.toString)
 *     print(" ")
 *     print("abcd".length.toString + "x")
 */
ASTNode *buildValmemChain(void) {
    return astProgram(
        2,
        astFunc(2, "half", 0, type_get_basic(TY_DOUBLE),
                astBin(2, TOK_DIVIDE, astId(2, "x"), astInt(2)), 1,
                astPlist(1, astParam("x", type_get_basic(TY_DOUBLE)))),
        astFunc(
            3, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                8, astVardef(4, "n", 0, NULL, astUn(4, TOK_MINUS, astDbl(0x1.c000000000000p+1))),
                astLonely(astCall(5, astId(5, "print"), 1,
                                  astMember(astMember(astId(5, "n"), "floor"), "toInt"))),
                astLonely(astCall(6, astId(6, "print"), 1, astStr(" "))),
                astLonely(astCall(
                    7, astId(7, "print"), 1,
                    astMember(astMember(astMember(astMember(astId(7, "n"), "floor"), "toInt"),
                                        "toString"),
                              "length"))),
                astLonely(astCall(8, astId(8, "print"), 1, astStr(" "))),
                astLonely(astCall(9, astId(9, "print"), 1,
                                  astMember(astMember(astCall(9, astId(9, "half"), 1,
                                                              astDbl(0x1.c000000000000p+2)),
                                                      "floor"),
                                            "toString"))),
                astLonely(astCall(10, astId(10, "print"), 1, astStr(" "))),
                astLonely(astCall(11, astId(11, "print"), 1,
                                  astBin(11, TOK_PLUS,
                                         astMember(astMember(astStr("abcd"), "length"), "toString"),
                                         astStr("x"))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val name = "IFJ"
 *     val n = 26
 *     val d = 1.2345
 *     val b = true
 *     print(s"$name${n + 1} costs $$0 $d $b ${()}|")
 *     print(s"")
 *     print(s"${toStr(n)}")
 */
ASTNode *buildInterpolation(void) {
    return astProgram(
        1,
        astFunc(
            2, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                7, astVardef(3, "name", 0, NULL, astStr("IFJ")),
                astVardef(4, "n", 0, NULL, astInt(26)),
                astVardef(5, "d", 0, NULL, astDbl(0x1.3c083126e978dp+0)),
                astVardef(6, "b", 0, NULL, astBool(1)),
                astLonely(astCall(7, astId(7, "print"), 1,
                                  astInterp(9, astId(0, "name"),
                                            astBin(1, TOK_PLUS, astId(1, "n"), astInt(1)),
                                            astStr(" costs $0 "), astId(0, "d"), astStr(" "),
                                            astId(0, "b"), astStr(" "), astUnit(), astStr("|")))),
                astLonely(astCall(8, astId(8, "print"), 1, astInterp(0))),
                astLonely(astCall(9, astId(9, "print"), 1,
                                  astInterp(1, astCall(1, astId(1, "toStr"), 1, astId(1, "n")))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * def answer: Int = 42
 * def answerWithParens(): Int = 42
 * def noisy: Int =
 *     print("!")
 *     1
 * def hello: Unit = print("hello ")
 * @main def run(): Unit =
 *     val x = answer
 *     val y = answerWithParens()
 *     print(x + y)
 *     val s = noisy + noisy
 *     print(s)
 *     hello
 *     print(toStr(answer))
 */
ASTNode *buildNoParamList(void) {
    return astProgram(
        5, astFunc(2, "answer", 0, type_get_basic(TY_INT), astInt(42), 0),
        astFunc(3, "answerWithParens", 0, type_get_basic(TY_INT), astInt(42), 1, astPlist(0)),
        astFunc(4, "noisy", 0, type_get_basic(TY_INT),
                astBlock(2, astLonely(astCall(5, astId(5, "print"), 1, astStr("!"))),
                         astLonely(astInt(1))),
                0),
        astFunc(7, "hello", 0, type_get_basic(TY_UNIT),
                astCall(7, astId(7, "print"), 1, astStr("hello ")), 0),
        astFunc(
            8, "run", 1, type_get_basic(TY_UNIT),
            astBlock(7, astVardef(9, "x", 0, NULL, astId(9, "answer")),
                     astVardef(10, "y", 0, NULL, astCall(10, astId(10, "answerWithParens"), 0)),
                     astLonely(astCall(11, astId(11, "print"), 1,
                                       astBin(11, TOK_PLUS, astId(11, "x"), astId(11, "y")))),
                     astVardef(12, "s", 0, NULL,
                               astBin(12, TOK_PLUS, astId(12, "noisy"), astId(12, "noisy"))),
                     astLonely(astCall(13, astId(13, "print"), 1, astId(13, "s"))),
                     astLonely(astId(14, "hello")),
                     astLonely(astCall(15, astId(15, "print"), 1,
                                       astCall(15, astId(15, "toStr"), 1, astId(15, "answer"))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * def add(a: Int)(b: Int)(c: Int): Int = a - b - c
 * def offset()(value: Int): Int = value + 1
 * def show(s: String): Int =
 *     print(s)
 *     1
 * def three(a: Int, b: Int)()(c: Int): Int = a * 100 + b * 10 + c
 * @main def run(): Unit =
 *     val sum = add(10)(2)(1)
 *     val result = offset()(sum)
 *     print(result)
 *     print(" ")
 *     print(add(show("a"))(show("b"))(show("c")))
 *     print(" ")
 *     print(three(1, 2)()(3))
 */
ASTNode *buildMultipleParamLists(void) {
    return astProgram(
        5,
        astFunc(
            2, "add", 0, type_get_basic(TY_INT),
            astBin(2, TOK_MINUS, astBin(2, TOK_MINUS, astId(2, "a"), astId(2, "b")), astId(2, "c")),
            3, astPlist(1, astParam("a", type_get_basic(TY_INT))),
            astPlist(1, astParam("b", type_get_basic(TY_INT))),
            astPlist(1, astParam("c", type_get_basic(TY_INT)))),
        astFunc(3, "offset", 0, type_get_basic(TY_INT),
                astBin(3, TOK_PLUS, astId(3, "value"), astInt(1)), 2, astPlist(0),
                astPlist(1, astParam("value", type_get_basic(TY_INT)))),
        astFunc(4, "show", 0, type_get_basic(TY_INT),
                astBlock(2, astLonely(astCall(5, astId(5, "print"), 1, astId(5, "s"))),
                         astLonely(astInt(1))),
                1, astPlist(1, astParam("s", type_get_basic(TY_STRING)))),
        astFunc(7, "three", 0, type_get_basic(TY_INT),
                astBin(7, TOK_PLUS,
                       astBin(7, TOK_PLUS, astBin(7, TOK_MULTIPLY, astId(7, "a"), astInt(100)),
                              astBin(7, TOK_MULTIPLY, astId(7, "b"), astInt(10))),
                       astId(7, "c")),
                3,
                astPlist(2, astParam("a", type_get_basic(TY_INT)),
                         astParam("b", type_get_basic(TY_INT))),
                astPlist(0), astPlist(1, astParam("c", type_get_basic(TY_INT)))),
        astFunc(
            8, "run", 1, type_get_basic(TY_UNIT),
            astBlock(
                7,
                astVardef(
                    9, "sum", 0, NULL,
                    astCall(9, astCall(9, astCall(9, astId(9, "add"), 1, astInt(10)), 1, astInt(2)),
                            1, astInt(1))),
                astVardef(10, "result", 0, NULL,
                          astCall(10, astCall(10, astId(10, "offset"), 0), 1, astId(10, "sum"))),
                astLonely(astCall(11, astId(11, "print"), 1, astId(11, "result"))),
                astLonely(astCall(12, astId(12, "print"), 1, astStr(" "))),
                astLonely(
                    astCall(13, astId(13, "print"), 1,
                            astCall(13,
                                    astCall(13,
                                            astCall(13, astId(13, "add"), 1,
                                                    astCall(13, astId(13, "show"), 1, astStr("a"))),
                                            1, astCall(13, astId(13, "show"), 1, astStr("b"))),
                                    1, astCall(13, astId(13, "show"), 1, astStr("c"))))),
                astLonely(astCall(14, astId(14, "print"), 1, astStr(" "))),
                astLonely(astCall(
                    15, astId(15, "print"), 1,
                    astCall(
                        15,
                        astCall(15, astCall(15, astId(15, "three"), 2, astInt(1), astInt(2)), 0), 1,
                        astInt(3))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * extension (value: Int)
 *     def twice: Int = value * 2
 *     def foo(): String = "foo" + value.toString
 *     def plus(other: Int): Int = value + other
 *     def addAll(a: Int)(b: Int): Int = value + a + b
 *     def again: Int = value.twice.twice
 * extension (value: String)
 *     def twice: String = value + value
 * @main def main(): Unit =
 *     val x: Int = 6
 *     print(x.twice)
 *     print(" ")
 *     print(x.foo())
 *     print(" ")
 *     print(x.plus(12))
 *     print(" ")
 *     print(x.addAll(1)(2))
 *     print(" ")
 *     print(x.again)
 *     print(" ")
 *     print("ab".twice)
 *     print(" ")
 *     print((x + 1).plus(1).twice)
 */
ASTNode *buildExtensionCalls(void) {
    return astProgram(
        3,
        astExtension(
            astParam("value", type_get_basic(TY_INT)), 5,
            astFunc(3, "twice", 0, type_get_basic(TY_INT),
                    astBin(3, TOK_MULTIPLY, astId(3, "value"), astInt(2)), 0),
            astFunc(4, "foo", 0, type_get_basic(TY_STRING),
                    astBin(4, TOK_PLUS, astStr("foo"), astMember(astId(4, "value"), "toString")), 1,
                    astPlist(0)),
            astFunc(5, "plus", 0, type_get_basic(TY_INT),
                    astBin(5, TOK_PLUS, astId(5, "value"), astId(5, "other")), 1,
                    astPlist(1, astParam("other", type_get_basic(TY_INT)))),
            astFunc(6, "addAll", 0, type_get_basic(TY_INT),
                    astBin(6, TOK_PLUS, astBin(6, TOK_PLUS, astId(6, "value"), astId(6, "a")),
                           astId(6, "b")),
                    2, astPlist(1, astParam("a", type_get_basic(TY_INT))),
                    astPlist(1, astParam("b", type_get_basic(TY_INT)))),
            astFunc(7, "again", 0, type_get_basic(TY_INT),
                    astMember(astMember(astId(7, "value"), "twice"), "twice"), 0)),
        astExtension(astParam("value", type_get_basic(TY_STRING)), 1,
                     astFunc(9, "twice", 0, type_get_basic(TY_STRING),
                             astBin(9, TOK_PLUS, astId(9, "value"), astId(9, "value")), 0)),
        astFunc(
            10, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                14, astVardef(11, "x", 0, type_get_basic(TY_INT), astInt(6)),
                astLonely(astCall(12, astId(12, "print"), 1, astMember(astId(12, "x"), "twice"))),
                astLonely(astCall(13, astId(13, "print"), 1, astStr(" "))),
                astLonely(astCall(14, astId(14, "print"), 1,
                                  astCall(14, astMember(astId(14, "x"), "foo"), 0))),
                astLonely(astCall(15, astId(15, "print"), 1, astStr(" "))),
                astLonely(astCall(16, astId(16, "print"), 1,
                                  astCall(16, astMember(astId(16, "x"), "plus"), 1, astInt(12)))),
                astLonely(astCall(17, astId(17, "print"), 1, astStr(" "))),
                astLonely(astCall(
                    18, astId(18, "print"), 1,
                    astCall(18, astCall(18, astMember(astId(18, "x"), "addAll"), 1, astInt(1)), 1,
                            astInt(2)))),
                astLonely(astCall(19, astId(19, "print"), 1, astStr(" "))),
                astLonely(astCall(20, astId(20, "print"), 1, astMember(astId(20, "x"), "again"))),
                astLonely(astCall(21, astId(21, "print"), 1, astStr(" "))),
                astLonely(astCall(22, astId(22, "print"), 1, astMember(astStr("ab"), "twice"))),
                astLonely(astCall(23, astId(23, "print"), 1, astStr(" "))),
                astLonely(astCall(
                    24, astId(24, "print"), 1,
                    astMember(
                        astCall(24,
                                astMember(astBin(24, TOK_PLUS, astId(24, "x"), astInt(1)), "plus"),
                                1, astInt(1)),
                        "twice")))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * type Scalar = Int | Double | String | Boolean | Unit
 * def show(x: Scalar): Unit =
 *     print(x)
 *     print(" ")
 * @main def main(): Unit =
 *     var anything: Scalar = 5
 *     show(anything)
 *     anything = "hello"
 *     show(anything)
 *     show(1.5)
 *     show(true)
 *     show(())
 */
ASTNode *buildPrintUnion(void) {
    return astProgram(
        3,
        astAlias("Scalar", astUnion(5, type_get_basic(TY_INT), type_get_basic(TY_DOUBLE),
                                    type_get_basic(TY_STRING), type_get_basic(TY_BOOL),
                                    type_get_basic(TY_UNIT))),
        astFunc(3, "show", 0, type_get_basic(TY_UNIT),
                astBlock(2, astLonely(astCall(4, astId(4, "print"), 1, astId(4, "x"))),
                         astLonely(astCall(5, astId(5, "print"), 1, astStr(" ")))),
                1, astPlist(1, astParam("x", type_create_named("Scalar")))),
        astFunc(6, "main", 1, type_get_basic(TY_UNIT),
                astBlock(7, astVardef(7, "anything", 1, type_create_named("Scalar"), astInt(5)),
                         astLonely(astCall(8, astId(8, "show"), 1, astId(8, "anything"))),
                         astAssign(9, "anything", astStr("hello")),
                         astLonely(astCall(10, astId(10, "show"), 1, astId(10, "anything"))),
                         astLonely(astCall(11, astId(11, "show"), 1, astDbl(0x1.8000000000000p+0))),
                         astLonely(astCall(12, astId(12, "show"), 1, astBool(1))),
                         astLonely(astCall(13, astId(13, "show"), 1, astUnit()))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * type Scalar = Int | Double | String | Boolean | Unit
 * def foo(x: Double | String): String =
 *     return toStr(x)
 * def show(x: Scalar): String = toStr(x) + "|" + x.toString + "|" + s"$x"
 * @main def main(): Unit =
 *     print(foo(5))
 *     print(" ")
 *     print(foo("a"))
 *     print(" ")
 *     print(show(7))
 *     print(" ")
 *     print(show(2.5))
 *     print(" ")
 *     print(show(false))
 *     print(" ")
 *     print(show(()))
 */
ASTNode *buildToStrUnion(void) {
    return astProgram(
        4,
        astAlias("Scalar", astUnion(5, type_get_basic(TY_INT), type_get_basic(TY_DOUBLE),
                                    type_get_basic(TY_STRING), type_get_basic(TY_BOOL),
                                    type_get_basic(TY_UNIT))),
        astFunc(3, "foo", 0, type_get_basic(TY_STRING),
                astBlock(1, astReturn(4, astCall(4, astId(4, "toStr"), 1, astId(4, "x")))), 1,
                astPlist(1, astParam("x", astUnion(2, type_get_basic(TY_DOUBLE),
                                                   type_get_basic(TY_STRING))))),
        astFunc(5, "show", 0, type_get_basic(TY_STRING),
                astBin(5, TOK_PLUS,
                       astBin(5, TOK_PLUS,
                              astBin(5, TOK_PLUS,
                                     astBin(5, TOK_PLUS,
                                            astCall(5, astId(5, "toStr"), 1, astId(5, "x")),
                                            astStr("|")),
                                     astMember(astId(5, "x"), "toString")),
                              astStr("|")),
                       astInterp(1, astId(0, "x"))),
                1, astPlist(1, astParam("x", type_create_named("Scalar")))),
        astFunc(6, "main", 1, type_get_basic(TY_UNIT),
                astBlock(11,
                         astLonely(astCall(7, astId(7, "print"), 1,
                                           astCall(7, astId(7, "foo"), 1, astInt(5)))),
                         astLonely(astCall(8, astId(8, "print"), 1, astStr(" "))),
                         astLonely(astCall(9, astId(9, "print"), 1,
                                           astCall(9, astId(9, "foo"), 1, astStr("a")))),
                         astLonely(astCall(10, astId(10, "print"), 1, astStr(" "))),
                         astLonely(astCall(11, astId(11, "print"), 1,
                                           astCall(11, astId(11, "show"), 1, astInt(7)))),
                         astLonely(astCall(12, astId(12, "print"), 1, astStr(" "))),
                         astLonely(astCall(
                             13, astId(13, "print"), 1,
                             astCall(13, astId(13, "show"), 1, astDbl(0x1.4000000000000p+1)))),
                         astLonely(astCall(14, astId(14, "print"), 1, astStr(" "))),
                         astLonely(astCall(15, astId(15, "print"), 1,
                                           astCall(15, astId(15, "show"), 1, astBool(0)))),
                         astLonely(astCall(16, astId(16, "print"), 1, astStr(" "))),
                         astLonely(astCall(17, astId(17, "print"), 1,
                                           astCall(17, astId(17, "show"), 1, astUnit())))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * type Scalar = Int | String | Unit
 * def same(a: Scalar, b: Scalar): Boolean = a == b
 * @main def main(): Unit =
 *     val x: Scalar = 5
 *     val y: Scalar = "5"
 *     val z: Scalar = ()
 *     print(same(x, y))
 *     print(same(x, 5))
 *     print(same(y, "5"))
 *     print(same(z, ()))
 *     print(same(z, x))
 *     print(x != y)
 */
ASTNode *buildUnionEquals(void) {
    return astProgram(
        3,
        astAlias("Scalar", astUnion(3, type_get_basic(TY_INT), type_get_basic(TY_STRING),
                                    type_get_basic(TY_UNIT))),
        astFunc(3, "same", 0, type_get_basic(TY_BOOL),
                astBin(3, TOK_EQUAL, astId(3, "a"), astId(3, "b")), 1,
                astPlist(2, astParam("a", type_create_named("Scalar")),
                         astParam("b", type_create_named("Scalar")))),
        astFunc(
            4, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                9, astVardef(5, "x", 0, type_create_named("Scalar"), astInt(5)),
                astVardef(6, "y", 0, type_create_named("Scalar"), astStr("5")),
                astVardef(7, "z", 0, type_create_named("Scalar"), astUnit()),
                astLonely(astCall(8, astId(8, "print"), 1,
                                  astCall(8, astId(8, "same"), 2, astId(8, "x"), astId(8, "y")))),
                astLonely(astCall(9, astId(9, "print"), 1,
                                  astCall(9, astId(9, "same"), 2, astId(9, "x"), astInt(5)))),
                astLonely(astCall(10, astId(10, "print"), 1,
                                  astCall(10, astId(10, "same"), 2, astId(10, "y"), astStr("5")))),
                astLonely(astCall(11, astId(11, "print"), 1,
                                  astCall(11, astId(11, "same"), 2, astId(11, "z"), astUnit()))),
                astLonely(
                    astCall(12, astId(12, "print"), 1,
                            astCall(12, astId(12, "same"), 2, astId(12, "z"), astId(12, "x")))),
                astLonely(astCall(13, astId(13, "print"), 1,
                                  astBin(13, TOK_NOT_EQUAL, astId(13, "x"), astId(13, "y"))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * case class Point(x: Int, y: Int)
 * case class Line(a: Point, b: Point, name: String)
 * def len2(l: Line): Int =
 *     val dx = l.b.x - l.a.x
 *     val dy = l.b.y - l.a.y
 *     dx * dx + dy * dy
 * @main def main(): Unit =
 *     val p = Point(1, 2)
 *     val l = Line(Point(4, 6), p, "L")
 *     print(p.x + p.y)
 *     print(" ")
 *     print(len2(l))
 *     print(" ")
 *     print(l.name + l.a.y.toString)
 */
ASTNode *buildClassFields(void) {
    return astProgram(
        4,
        astClass("Point", astPlist(2, astParam("x", type_get_basic(TY_INT)),
                                   astParam("y", type_get_basic(TY_INT)))),
        astClass("Line", astPlist(3, astParam("a", type_create_named("Point")),
                                  astParam("b", type_create_named("Point")),
                                  astParam("name", type_get_basic(TY_STRING)))),
        astFunc(
            4, "len2", 0, type_get_basic(TY_INT),
            astBlock(3,
                     astVardef(5, "dx", 0, NULL,
                               astBin(5, TOK_MINUS, astMember(astMember(astId(5, "l"), "b"), "x"),
                                      astMember(astMember(astId(5, "l"), "a"), "x"))),
                     astVardef(6, "dy", 0, NULL,
                               astBin(6, TOK_MINUS, astMember(astMember(astId(6, "l"), "b"), "y"),
                                      astMember(astMember(astId(6, "l"), "a"), "y"))),
                     astLonely(astBin(7, TOK_PLUS,
                                      astBin(7, TOK_MULTIPLY, astId(7, "dx"), astId(7, "dx")),
                                      astBin(7, TOK_MULTIPLY, astId(7, "dy"), astId(7, "dy"))))),
            1, astPlist(1, astParam("l", type_create_named("Line")))),
        astFunc(
            8, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                7,
                astVardef(9, "p", 0, NULL, astCall(9, astId(9, "Point"), 2, astInt(1), astInt(2))),
                astVardef(10, "l", 0, NULL,
                          astCall(10, astId(10, "Line"), 3,
                                  astCall(10, astId(10, "Point"), 2, astInt(4), astInt(6)),
                                  astId(10, "p"), astStr("L"))),
                astLonely(astCall(11, astId(11, "print"), 1,
                                  astBin(11, TOK_PLUS, astMember(astId(11, "p"), "x"),
                                         astMember(astId(11, "p"), "y")))),
                astLonely(astCall(12, astId(12, "print"), 1, astStr(" "))),
                astLonely(astCall(13, astId(13, "print"), 1,
                                  astCall(13, astId(13, "len2"), 1, astId(13, "l")))),
                astLonely(astCall(14, astId(14, "print"), 1, astStr(" "))),
                astLonely(astCall(15, astId(15, "print"), 1,
                                  astBin(15, TOK_PLUS, astMember(astId(15, "l"), "name"),
                                         astMember(astMember(astMember(astId(15, "l"), "a"), "y"),
                                                   "toString"))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * def f(n: Int): String =
 *     n match
 *         case _ => "any" + toStr(n)
 * def g(n: Int): Unit =
 *     n match
 *         case _ =>
 *             print("block")
 *             print(n)
 * @main def main(): Unit =
 *     val x = 3 match
 *         case _ => 10
 *     print(x)
 *     print(f(4))
 *     g(5)
 *     val y = 7 match
 *         case _ => 1
 *     print(y + 1)
 */
ASTNode *buildMatchExpression(void) {
    return astProgram(
        3,
        astFunc(2, "f", 0, type_get_basic(TY_STRING),
                astBlock(1, astLonely(astMatch(
                                3, astId(3, "n"), 1,
                                astCase(astPatWild(), NULL,
                                        astBin(4, TOK_PLUS, astStr("any"),
                                               astCall(4, astId(4, "toStr"), 1, astId(4, "n"))))))),
                1, astPlist(1, astParam("n", type_get_basic(TY_INT)))),
        astFunc(
            5, "g", 0, type_get_basic(TY_UNIT),
            astBlock(
                1, astLonely(astMatch(
                       6, astId(6, "n"), 1,
                       astCase(astPatWild(), NULL,
                               astBlock(
                                   2, astLonely(astCall(8, astId(8, "print"), 1, astStr("block"))),
                                   astLonely(astCall(9, astId(9, "print"), 1, astId(9, "n")))))))),
            1, astPlist(1, astParam("n", type_get_basic(TY_INT)))),
        astFunc(10, "main", 1, type_get_basic(TY_UNIT),
                astBlock(
                    6,
                    astVardef(11, "x", 0, NULL,
                              astMatch(11, astInt(3), 1, astCase(astPatWild(), NULL, astInt(10)))),
                    astLonely(astCall(13, astId(13, "print"), 1, astId(13, "x"))),
                    astLonely(astCall(14, astId(14, "print"), 1,
                                      astCall(14, astId(14, "f"), 1, astInt(4)))),
                    astLonely(astCall(15, astId(15, "g"), 1, astInt(5))),
                    astVardef(16, "y", 0, NULL,
                              astMatch(16, astInt(7), 1, astCase(astPatWild(), NULL, astInt(1)))),
                    astLonely(astCall(18, astId(18, "print"), 1,
                                      astBin(18, TOK_PLUS, astId(18, "y"), astInt(1))))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * def d(v: Int | String | Double | Unit): String =
 *     v match
 *         case 0 => "zero"
 *         case -1 => "minus one"
 *         case "a" => "letter a"
 *         case 2.5 => "two and half"
 *         case () => "unit"
 *         case _ => "other"
 * def b(x: Boolean): Int =
 *     x match
 *         case true => 1
 *         case _ => 0
 * @main def main(): Unit =
 *     print(d(0))
 *     print(",")
 *     print(d(-1))
 *     print(",")
 *     print(d("a"))
 *     print(",")
 *     print(d(2.5))
 *     print(",")
 *     print(d(()))
 *     print(",")
 *     print(d(7))
 *     print(",")
 *     print(d("b"))
 *     print(",")
 *     print(b(true))
 *     print(b(false))
 */
ASTNode *buildLiteralPattern(void) {
    return astProgram(
        3,
        astFunc(
            2, "d", 0, type_get_basic(TY_STRING),
            astBlock(
                1,
                astLonely(astMatch(
                    3, astId(3, "v"), 6, astCase(astPatLit(astInt(0)), NULL, astStr("zero")),
                    astCase(astPatLit(astUn(5, TOK_MINUS, astInt(1))), NULL, astStr("minus one")),
                    astCase(astPatLit(astStr("a")), NULL, astStr("letter a")),
                    astCase(astPatLit(astDbl(0x1.4000000000000p+1)), NULL, astStr("two and half")),
                    astCase(astPatLit(astUnit()), NULL, astStr("unit")),
                    astCase(astPatWild(), NULL, astStr("other"))))),
            1,
            astPlist(1,
                     astParam("v", astUnion(4, type_get_basic(TY_INT), type_get_basic(TY_STRING),
                                            type_get_basic(TY_DOUBLE), type_get_basic(TY_UNIT))))),
        astFunc(10, "b", 0, type_get_basic(TY_INT),
                astBlock(1, astLonely(astMatch(11, astId(11, "x"), 2,
                                               astCase(astPatLit(astBool(1)), NULL, astInt(1)),
                                               astCase(astPatWild(), NULL, astInt(0))))),
                1, astPlist(1, astParam("x", type_get_basic(TY_BOOL)))),
        astFunc(
            14,
            "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                16,
                astLonely(
                    astCall(15, astId(15, "print"), 1, astCall(15, astId(15, "d"), 1, astInt(0)))),
                astLonely(astCall(16, astId(16, "print"), 1, astStr(","))),
                astLonely(astCall(17, astId(17, "print"), 1,
                                  astCall(17, astId(17, "d"), 1, astUn(17, TOK_MINUS, astInt(1))))),
                astLonely(astCall(18, astId(18, "print"), 1, astStr(","))),
                astLonely(astCall(19, astId(19, "print"), 1,
                                  astCall(19, astId(19, "d"), 1, astStr("a")))),
                astLonely(astCall(20, astId(20, "print"), 1, astStr(","))),
                astLonely(astCall(21, astId(21, "print"), 1,
                                  astCall(21, astId(21, "d"), 1, astDbl(0x1.4000000000000p+1)))),
                astLonely(astCall(22, astId(22, "print"), 1, astStr(","))),
                astLonely(
                    astCall(23, astId(23, "print"), 1, astCall(23, astId(23, "d"), 1, astUnit()))),
                astLonely(astCall(24, astId(24, "print"), 1, astStr(","))),
                astLonely(
                    astCall(25, astId(25, "print"), 1, astCall(25, astId(25, "d"), 1, astInt(7)))),
                astLonely(astCall(26, astId(26, "print"), 1, astStr(","))),
                astLonely(astCall(27, astId(27, "print"), 1,
                                  astCall(27, astId(27, "d"), 1, astStr("b")))),
                astLonely(astCall(28, astId(28, "print"), 1, astStr(","))),
                astLonely(
                    astCall(29, astId(29, "print"), 1, astCall(29, astId(29, "b"), 1, astBool(1)))),
                astLonely(astCall(30, astId(30, "print"), 1,
                                  astCall(30, astId(30, "b"), 1, astBool(0))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * type Num = Int | Double
 * def d(v: Int | String | Double | Unit): String =
 *     v match
 *         case n: Int => "int " + toStr(n + 1)
 *         case s: String => "str " + s
 *         case x: Double => "dbl " + toStr(x)
 *         case _ => "unit"
 * def e(v: Int | String | Double): String =
 *     v match
 *         case n: Num => "num"
 *         case _ => "other"
 * @main def main(): Unit =
 *     print(d(1))
 *     print(",")
 *     print(d("q"))
 *     print(",")
 *     print(d(2.0))
 *     print(",")
 *     print(d(()))
 *     print(",")
 *     print(e(1))
 *     print(e(1.5))
 *     print(e("x"))
 */
ASTNode *buildTypedPattern(void) {
    return astProgram(
        4, astAlias("Num", astUnion(2, type_get_basic(TY_INT), type_get_basic(TY_DOUBLE))),
        astFunc(
            3, "d", 0, type_get_basic(TY_STRING),
            astBlock(1, astLonely(astMatch(
                            4, astId(4, "v"), 4,
                            astCase(astPatTyped("n", type_get_basic(TY_INT)), NULL,
                                    astBin(5, TOK_PLUS, astStr("int "),
                                           astCall(5, astId(5, "toStr"), 1,
                                                   astBin(5, TOK_PLUS, astId(5, "n"), astInt(1))))),
                            astCase(astPatTyped("s", type_get_basic(TY_STRING)), NULL,
                                    astBin(6, TOK_PLUS, astStr("str "), astId(6, "s"))),
                            astCase(astPatTyped("x", type_get_basic(TY_DOUBLE)), NULL,
                                    astBin(7, TOK_PLUS, astStr("dbl "),
                                           astCall(7, astId(7, "toStr"), 1, astId(7, "x")))),
                            astCase(astPatWild(), NULL, astStr("unit"))))),
            1,
            astPlist(1,
                     astParam("v", astUnion(4, type_get_basic(TY_INT), type_get_basic(TY_STRING),
                                            type_get_basic(TY_DOUBLE), type_get_basic(TY_UNIT))))),
        astFunc(
            9, "e", 0, type_get_basic(TY_STRING),
            astBlock(1, astLonely(astMatch(10, astId(10, "v"), 2,
                                           astCase(astPatTyped("n", type_create_named("Num")), NULL,
                                                   astStr("num")),
                                           astCase(astPatWild(), NULL, astStr("other"))))),
            1,
            astPlist(1, astParam("v", astUnion(3, type_get_basic(TY_INT), type_get_basic(TY_STRING),
                                               type_get_basic(TY_DOUBLE))))),
        astFunc(
            13,
            "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                11,
                astLonely(
                    astCall(14, astId(14, "print"), 1, astCall(14, astId(14, "d"), 1, astInt(1)))),
                astLonely(astCall(15, astId(15, "print"), 1, astStr(","))),
                astLonely(astCall(16, astId(16, "print"), 1,
                                  astCall(16, astId(16, "d"), 1, astStr("q")))),
                astLonely(astCall(17, astId(17, "print"), 1, astStr(","))),
                astLonely(astCall(18, astId(18, "print"), 1,
                                  astCall(18, astId(18, "d"), 1, astDbl(0x1.0000000000000p+1)))),
                astLonely(astCall(19, astId(19, "print"), 1, astStr(","))),
                astLonely(
                    astCall(20, astId(20, "print"), 1, astCall(20, astId(20, "d"), 1, astUnit()))),
                astLonely(astCall(21, astId(21, "print"), 1, astStr(","))),
                astLonely(
                    astCall(22, astId(22, "print"), 1, astCall(22, astId(22, "e"), 1, astInt(1)))),
                astLonely(astCall(23, astId(23, "print"), 1,
                                  astCall(23, astId(23, "e"), 1, astDbl(0x1.8000000000000p+0)))),
                astLonely(astCall(24, astId(24, "print"), 1,
                                  astCall(24, astId(24, "e"), 1, astStr("x"))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * case class Point(x: Int, y: Int)
 * case class Size(x: Int, y: Int)
 * type Shape = Point | Size | Int
 * def d(v: Shape): String =
 *     v match
 *         case Point(a, _) => "point " + toStr(a)
 *         case s: Size => "size " + toStr(s.x * s.y)
 *         case _ => "int"
 * @main def main(): Unit =
 *     print(d(Point(2, 3)))
 *     print(",")
 *     print(d(Size(2, 3)))
 *     print(",")
 *     print(d(4))
 */
ASTNode *buildConstructorPattern(void) {
    return astProgram(
        5,
        astClass("Point", astPlist(2, astParam("x", type_get_basic(TY_INT)),
                                   astParam("y", type_get_basic(TY_INT)))),
        astClass("Size", astPlist(2, astParam("x", type_get_basic(TY_INT)),
                                  astParam("y", type_get_basic(TY_INT)))),
        astAlias("Shape", astUnion(3, type_create_named("Point"), type_create_named("Size"),
                                   type_get_basic(TY_INT))),
        astFunc(5, "d", 0, type_get_basic(TY_STRING),
                astBlock(1, astLonely(astMatch(
                                6, astId(6, "v"), 3,
                                astCase(astPatCtor("Point", 2, "a", "_"), NULL,
                                        astBin(7, TOK_PLUS, astStr("point "),
                                               astCall(7, astId(7, "toStr"), 1, astId(7, "a")))),
                                astCase(astPatTyped("s", type_create_named("Size")), NULL,
                                        astBin(8, TOK_PLUS, astStr("size "),
                                               astCall(8, astId(8, "toStr"), 1,
                                                       astBin(8, TOK_MULTIPLY,
                                                              astMember(astId(8, "s"), "x"),
                                                              astMember(astId(8, "s"), "y"))))),
                                astCase(astPatWild(), NULL, astStr("int"))))),
                1, astPlist(1, astParam("v", type_create_named("Shape")))),
        astFunc(10, "main", 1, type_get_basic(TY_UNIT),
                astBlock(5,
                         astLonely(astCall(
                             11, astId(11, "print"), 1,
                             astCall(11, astId(11, "d"), 1,
                                     astCall(11, astId(11, "Point"), 2, astInt(2), astInt(3))))),
                         astLonely(astCall(12, astId(12, "print"), 1, astStr(","))),
                         astLonely(astCall(
                             13, astId(13, "print"), 1,
                             astCall(13, astId(13, "d"), 1,
                                     astCall(13, astId(13, "Size"), 2, astInt(2), astInt(3))))),
                         astLonely(astCall(14, astId(14, "print"), 1, astStr(","))),
                         astLonely(astCall(15, astId(15, "print"), 1,
                                           astCall(15, astId(15, "d"), 1, astInt(4))))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * case class Point(x: Int, y: Int)
 * def d(v: Int | String | Point): String =
 *     v match
 *         case 0 => "zero"
 *         case n: Int if n > 0 => "positive"
 *         case text: String if text.length > 2 => "long"
 *         case Point(a, b) if a == b => "diag"
 *         case Point(a, b) => "point " + toStr(a + b)
 *         case _ => "other"
 * @main def main(): Unit =
 *     print(d(0))
 *     print(",")
 *     print(d(5))
 *     print(",")
 *     print(d(-5))
 *     print(",")
 *     print(d("abc"))
 *     print(",")
 *     print(d("ab"))
 *     print(",")
 *     print(d(Point(1, 1)))
 *     print(",")
 *     print(d(Point(1, 2)))
 */
ASTNode *buildCaseGuards(void) {
    return astProgram(
        3,
        astClass("Point", astPlist(2, astParam("x", type_get_basic(TY_INT)),
                                   astParam("y", type_get_basic(TY_INT)))),
        astFunc(
            3, "d", 0, type_get_basic(TY_STRING),
            astBlock(
                1,
                astLonely(astMatch(
                    4, astId(4, "v"), 6, astCase(astPatLit(astInt(0)), NULL, astStr("zero")),
                    astCase(astPatTyped("n", type_get_basic(TY_INT)),
                            astBin(6, TOK_GREATER, astId(6, "n"), astInt(0)), astStr("positive")),
                    astCase(
                        astPatTyped("text", type_get_basic(TY_STRING)),
                        astBin(7, TOK_GREATER, astMember(astId(7, "text"), "length"), astInt(2)),
                        astStr("long")),
                    astCase(astPatCtor("Point", 2, "a", "b"),
                            astBin(8, TOK_EQUAL, astId(8, "a"), astId(8, "b")), astStr("diag")),
                    astCase(astPatCtor("Point", 2, "a", "b"), NULL,
                            astBin(9, TOK_PLUS, astStr("point "),
                                   astCall(9, astId(9, "toStr"), 1,
                                           astBin(9, TOK_PLUS, astId(9, "a"), astId(9, "b"))))),
                    astCase(astPatWild(), NULL, astStr("other"))))),
            1,
            astPlist(1, astParam("v", astUnion(3, type_get_basic(TY_INT), type_get_basic(TY_STRING),
                                               type_create_named("Point"))))),
        astFunc(11, "main", 1, type_get_basic(TY_UNIT),
                astBlock(13,
                         astLonely(astCall(12, astId(12, "print"), 1,
                                           astCall(12, astId(12, "d"), 1, astInt(0)))),
                         astLonely(astCall(13, astId(13, "print"), 1, astStr(","))),
                         astLonely(astCall(14, astId(14, "print"), 1,
                                           astCall(14, astId(14, "d"), 1, astInt(5)))),
                         astLonely(astCall(15, astId(15, "print"), 1, astStr(","))),
                         astLonely(astCall(
                             16, astId(16, "print"), 1,
                             astCall(16, astId(16, "d"), 1, astUn(16, TOK_MINUS, astInt(5))))),
                         astLonely(astCall(17, astId(17, "print"), 1, astStr(","))),
                         astLonely(astCall(18, astId(18, "print"), 1,
                                           astCall(18, astId(18, "d"), 1, astStr("abc")))),
                         astLonely(astCall(19, astId(19, "print"), 1, astStr(","))),
                         astLonely(astCall(20, astId(20, "print"), 1,
                                           astCall(20, astId(20, "d"), 1, astStr("ab")))),
                         astLonely(astCall(21, astId(21, "print"), 1, astStr(","))),
                         astLonely(astCall(
                             22, astId(22, "print"), 1,
                             astCall(22, astId(22, "d"), 1,
                                     astCall(22, astId(22, "Point"), 2, astInt(1), astInt(1))))),
                         astLonely(astCall(23, astId(23, "print"), 1, astStr(","))),
                         astLonely(astCall(
                             24, astId(24, "print"), 1,
                             astCall(24, astId(24, "d"), 1,
                                     astCall(24, astId(24, "Point"), 2, astInt(1), astInt(2)))))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = readIntMaybe()
 *     val b = readIntMaybe()
 *     val c = readDoubleMaybe()
 *     val d = readDoubleMaybe()
 *     print(a)
 *     print(" ")
 *     print(b)
 *     print(" ")
 *     print(c)
 *     print(" ")
 *     print(d)
 *     print(" ")
 *     print(toStr(c))
 */
ASTNode *buildReadMaybe(void) {
    return astProgram(
        1, astFunc(2, "main", 1, type_get_basic(TY_UNIT),
                   astBlock(13, astVardef(3, "a", 0, NULL, astCall(3, astId(3, "readIntMaybe"), 0)),
                            astVardef(4, "b", 0, NULL, astCall(4, astId(4, "readIntMaybe"), 0)),
                            astVardef(5, "c", 0, NULL, astCall(5, astId(5, "readDoubleMaybe"), 0)),
                            astVardef(6, "d", 0, NULL, astCall(6, astId(6, "readDoubleMaybe"), 0)),
                            astLonely(astCall(7, astId(7, "print"), 1, astId(7, "a"))),
                            astLonely(astCall(8, astId(8, "print"), 1, astStr(" "))),
                            astLonely(astCall(9, astId(9, "print"), 1, astId(9, "b"))),
                            astLonely(astCall(10, astId(10, "print"), 1, astStr(" "))),
                            astLonely(astCall(11, astId(11, "print"), 1, astId(11, "c"))),
                            astLonely(astCall(12, astId(12, "print"), 1, astStr(" "))),
                            astLonely(astCall(13, astId(13, "print"), 1, astId(13, "d"))),
                            astLonely(astCall(14, astId(14, "print"), 1, astStr(" "))),
                            astLonely(astCall(15, astId(15, "print"), 1,
                                              astCall(15, astId(15, "toStr"), 1, astId(15, "c"))))),
                   1, astPlist(0)));
}

/*
 * import ifj26.*
 * def inc(a: Int): Int = a + 1
 * def twice(a: Int): Int = a * 2
 * def applyTo(f: (Int) => Int, x: Int): Int = f(x)
 * def pick(b: Boolean): (Int) => Int =
 *     if b then
 *         return inc
 *     else
 *         return twice
 * @main def main(): Unit =
 *     val g: (Int) => Int = inc
 *     print(g(5))
 *     print(" ")
 *     print(applyTo(twice, 5))
 *     print(" ")
 *     print(applyTo(g, g(1)))
 *     print(" ")
 *     val p = pick(false)
 *     print(p(10))
 */
ASTNode *buildFunctionValueCall(void) {
    return astProgram(
        5,
        astFunc(2, "inc", 0, type_get_basic(TY_INT), astBin(2, TOK_PLUS, astId(2, "a"), astInt(1)),
                1, astPlist(1, astParam("a", type_get_basic(TY_INT)))),
        astFunc(3, "twice", 0, type_get_basic(TY_INT),
                astBin(3, TOK_MULTIPLY, astId(3, "a"), astInt(2)), 1,
                astPlist(1, astParam("a", type_get_basic(TY_INT)))),
        astFunc(
            4, "applyTo", 0, type_get_basic(TY_INT), astCall(4, astId(4, "f"), 1, astId(4, "x")), 1,
            astPlist(2,
                     astParam("f", astFunctype(type_get_basic(TY_INT), 1, type_get_basic(TY_INT))),
                     astParam("x", type_get_basic(TY_INT)))),
        astFunc(5, "pick", 0, astFunctype(type_get_basic(TY_INT), 1, type_get_basic(TY_INT)),
                astBlock(
                    1, astLonely(astIf(6, astId(6, "b"), astBlock(1, astReturn(7, astId(7, "inc"))),
                                       astBlock(1, astReturn(9, astId(9, "twice")))))),
                1, astPlist(1, astParam("b", type_get_basic(TY_BOOL)))),
        astFunc(10, "main", 1, type_get_basic(TY_UNIT),
                astBlock(9,
                         astVardef(11, "g", 0,
                                   astFunctype(type_get_basic(TY_INT), 1, type_get_basic(TY_INT)),
                                   astId(11, "inc")),
                         astLonely(astCall(12, astId(12, "print"), 1,
                                           astCall(12, astId(12, "g"), 1, astInt(5)))),
                         astLonely(astCall(13, astId(13, "print"), 1, astStr(" "))),
                         astLonely(astCall(
                             14, astId(14, "print"), 1,
                             astCall(14, astId(14, "applyTo"), 2, astId(14, "twice"), astInt(5)))),
                         astLonely(astCall(15, astId(15, "print"), 1, astStr(" "))),
                         astLonely(astCall(16, astId(16, "print"), 1,
                                           astCall(16, astId(16, "applyTo"), 2, astId(16, "g"),
                                                   astCall(16, astId(16, "g"), 1, astInt(1))))),
                         astLonely(astCall(17, astId(17, "print"), 1, astStr(" "))),
                         astVardef(18, "p", 0, NULL, astCall(18, astId(18, "pick"), 1, astBool(0))),
                         astLonely(astCall(19, astId(19, "print"), 1,
                                           astCall(19, astId(19, "p"), 1, astInt(10))))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * def applyTo(f: (Int) => Int, x: Int): Int = f(x)
 * def makeAdder(): (Int, Int) => Int = (a: Int, b: Int) => a + b
 * @main def main(): Unit =
 *     val sq = (a: Int) => a * a
 *     val z = () => 42
 *     print(sq(5))
 *     print(" ")
 *     print(applyTo((a: Int) => a - 1, 0))
 *     print(" ")
 *     print(z())
 *     print(" ")
 *     val add = makeAdder()
 *     print(add(10, 8))
 *     print(" ")
 *     val positive = (value: Int) => value > 0
 *     print(positive(3))
 *     val nested = (a: Int) => applyTo((b: Int) => b * 10, a)
 *     print(nested(4))
 */
ASTNode *buildLambdas(void) {
    return astProgram(
        3,
        astFunc(
            2, "applyTo", 0, type_get_basic(TY_INT), astCall(2, astId(2, "f"), 1, astId(2, "x")), 1,
            astPlist(2,
                     astParam("f", astFunctype(type_get_basic(TY_INT), 1, type_get_basic(TY_INT))),
                     astParam("x", type_get_basic(TY_INT)))),
        astFunc(
            3, "makeAdder", 0,
            astFunctype(type_get_basic(TY_INT), 2, type_get_basic(TY_INT), type_get_basic(TY_INT)),
            astLambda(astPlist(2, astParam("a", type_get_basic(TY_INT)),
                               astParam("b", type_get_basic(TY_INT))),
                      astBin(3, TOK_PLUS, astId(3, "a"), astId(3, "b"))),
            1, astPlist(0)),
        astFunc(4, "main", 1, type_get_basic(TY_UNIT),
                astBlock(
                    15,
                    astVardef(5, "sq", 0, NULL,
                              astLambda(astPlist(1, astParam("a", type_get_basic(TY_INT))),
                                        astBin(5, TOK_MULTIPLY, astId(5, "a"), astId(5, "a")))),
                    astVardef(6, "z", 0, NULL, astLambda(astPlist(0), astInt(42))),
                    astLonely(
                        astCall(7, astId(7, "print"), 1, astCall(7, astId(7, "sq"), 1, astInt(5)))),
                    astLonely(astCall(8, astId(8, "print"), 1, astStr(" "))),
                    astLonely(astCall(
                        9, astId(9, "print"), 1,
                        astCall(9, astId(9, "applyTo"), 2,
                                astLambda(astPlist(1, astParam("a", type_get_basic(TY_INT))),
                                          astBin(9, TOK_MINUS, astId(9, "a"), astInt(1))),
                                astInt(0)))),
                    astLonely(astCall(10, astId(10, "print"), 1, astStr(" "))),
                    astLonely(astCall(11, astId(11, "print"), 1, astCall(11, astId(11, "z"), 0))),
                    astLonely(astCall(12, astId(12, "print"), 1, astStr(" "))),
                    astVardef(13, "add", 0, NULL, astCall(13, astId(13, "makeAdder"), 0)),
                    astLonely(astCall(14, astId(14, "print"), 1,
                                      astCall(14, astId(14, "add"), 2, astInt(10), astInt(8)))),
                    astLonely(astCall(15, astId(15, "print"), 1, astStr(" "))),
                    astVardef(16, "positive", 0, NULL,
                              astLambda(astPlist(1, astParam("value", type_get_basic(TY_INT))),
                                        astBin(16, TOK_GREATER, astId(16, "value"), astInt(0)))),
                    astLonely(astCall(17, astId(17, "print"), 1,
                                      astCall(17, astId(17, "positive"), 1, astInt(3)))),
                    astVardef(
                        18, "nested", 0, NULL,
                        astLambda(
                            astPlist(1, astParam("a", type_get_basic(TY_INT))),
                            astCall(18, astId(18, "applyTo"), 2,
                                    astLambda(astPlist(1, astParam("b", type_get_basic(TY_INT))),
                                              astBin(18, TOK_MULTIPLY, astId(18, "b"), astInt(10))),
                                    astId(18, "a")))),
                    astLonely(astCall(19, astId(19, "print"), 1,
                                      astCall(19, astId(19, "nested"), 1, astInt(4))))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * def goo(): (Int) => String =
 *     return (a: Int) =>
 *         val strA = toStr(a)
 *         "cislo" + strA
 * def bar(a: Int, f: (Int) => Boolean) =
 *     val result = f(a)
 *     if result then
 *         print("f uspelo")
 *     else
 *         print("f neuspelo")
 * @main def main(): Unit =
 *     val sumTo = (n: Int) =>
 *         var s = 0
 *         var i = 1
 *         while i <= n do
 *             val x = i
 *             s = s + x
 *             i = i + 1
 *         s
 *     print(sumTo(4))
 *     print(" ")
 *     val g = goo()
 *     print(g(7))
 *     print(" ")
 *     val sayHi = () =>
 *         print("hi")
 *     sayHi()
 *     print(" ")
 *     bar(5, (v: Int) => v > 3)
 *     val m = (v: Int) =>
 *         v match
 *             case 0 => "zero"
 *             case _ => "nonzero"
 *     print(m(0))
 */
ASTNode *buildLambdaBlockBody(void) {
    return astProgram(
        3,
        astFunc(2, "goo", 0, astFunctype(type_get_basic(TY_STRING), 1, type_get_basic(TY_INT)),
                astBlock(1, astReturn(
                                3, astLambda(astPlist(1, astParam("a", type_get_basic(TY_INT))),
                                             astBlock(2,
                                                      astVardef(4, "strA", 0, NULL,
                                                                astCall(4, astId(4, "toStr"), 1,
                                                                        astId(4, "a"))),
                                                      astLonely(astBin(5, TOK_PLUS, astStr("cislo"),
                                                                       astId(5, "strA"))))))),
                1, astPlist(0)),
        astFunc(6, "bar", 0, NULL,
                astBlock(
                    2, astVardef(7, "result", 0, NULL, astCall(7, astId(7, "f"), 1, astId(7, "a"))),
                    astLonely(astIf(8, astId(8, "result"),
                                    astBlock(1, astLonely(astCall(9, astId(9, "print"), 1,
                                                                  astStr("f uspelo")))),
                                    astBlock(1, astLonely(astCall(11, astId(11, "print"), 1,
                                                                  astStr("f neuspelo"))))))),
                1,
                astPlist(2, astParam("a", type_get_basic(TY_INT)),
                         astParam("f", astFunctype(type_get_basic(TY_BOOL), 1,
                                                   type_get_basic(TY_INT))))),
        astFunc(
            12,
            "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                12,
                astVardef(
                    13,
                    "sumTo",
                    0, NULL,
                    astLambda(
                        astPlist(1, astParam("n", type_get_basic(TY_INT))),
                        astBlock(
                            4, astVardef(14, "s", 1, NULL, astInt(0)),
                            astVardef(15, "i", 1, NULL, astInt(1)),
                            astWhile(16, astBin(16, TOK_LESS_EQUAL, astId(16, "i"), astId(16, "n")),
                                     astBlock(3, astVardef(17, "x", 0, NULL, astId(17, "i")),
                                              astAssign(18, "s",
                                                        astBin(18, TOK_PLUS, astId(18, "s"),
                                                               astId(18, "x"))),
                                              astAssign(19, "i",
                                                        astBin(19, TOK_PLUS, astId(19, "i"),
                                                               astInt(1))))),
                            astLonely(astId(20, "s"))))),
                astLonely(astCall(21, astId(21, "print"), 1,
                                  astCall(21, astId(21, "sumTo"), 1, astInt(4)))),
                astLonely(astCall(22, astId(22, "print"), 1, astStr(" "))),
                astVardef(23, "g", 0, NULL, astCall(23, astId(23, "goo"), 0)),
                astLonely(
                    astCall(24, astId(24, "print"), 1, astCall(24, astId(24, "g"), 1, astInt(7)))),
                astLonely(astCall(25, astId(25, "print"), 1, astStr(" "))),
                astVardef(26, "sayHi", 0, NULL,
                          astLambda(astPlist(0),
                                    astBlock(1, astLonely(astCall(27, astId(27, "print"), 1,
                                                                  astStr("hi")))))),
                astLonely(astCall(28, astId(28, "sayHi"), 0)),
                astLonely(astCall(29, astId(29, "print"), 1, astStr(" "))),
                astLonely(astCall(30, astId(30, "bar"), 2, astInt(5),
                                  astLambda(astPlist(1, astParam("v", type_get_basic(TY_INT))),
                                            astBin(30, TOK_GREATER, astId(30, "v"), astInt(3))))),
                astVardef(31, "m", 0, NULL,
                          astLambda(astPlist(1, astParam("v", type_get_basic(TY_INT))),
                                    astBlock(1, astLonely(astMatch(32, astId(32, "v"), 2,
                                                                   astCase(astPatLit(astInt(0)),
                                                                           NULL, astStr("zero")),
                                                                   astCase(astPatWild(), NULL,
                                                                           astStr("nonzero"))))))),
                astLonely(
                    astCall(35, astId(35, "print"), 1, astCall(35, astId(35, "m"), 1, astInt(0))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * def makeMul(k: Int): (Int) => Int =
 *     if k == 2 then
 *         return (a: Int) => a * 2
 *     else
 *         return (a: Int) => a * 3
 * def compose(f: (Int) => Int, g: (Int) => Int): (Int) => Int = (x: Int) => x + 1
 * def curry(): () => (Int) => Int = () => (a: Int) => a + 100
 * @main def main(): Unit =
 *     print(makeMul(2)(5))
 *     print(" ")
 *     print(makeMul(3)(5))
 *     print(" ")
 *     val c = curry()
 *     print(c()(1))
 *     print(" ")
 *     print(curry()()(2))
 */
ASTNode *buildReturnFunction(void) {
    return astProgram(
        4,
        astFunc(
            2, "makeMul", 0, astFunctype(type_get_basic(TY_INT), 1, type_get_basic(TY_INT)),
            astBlock(
                1,
                astLonely(astIf(
                    3, astBin(3, TOK_EQUAL, astId(3, "k"), astInt(2)),
                    astBlock(
                        1, astReturn(
                               4, astLambda(astPlist(1, astParam("a", type_get_basic(TY_INT))),
                                            astBin(4, TOK_MULTIPLY, astId(4, "a"), astInt(2))))),
                    astBlock(
                        1, astReturn(
                               6, astLambda(astPlist(1, astParam("a", type_get_basic(TY_INT))),
                                            astBin(6, TOK_MULTIPLY, astId(6, "a"), astInt(3)))))))),
            1, astPlist(1, astParam("k", type_get_basic(TY_INT)))),
        astFunc(7, "compose", 0, astFunctype(type_get_basic(TY_INT), 1, type_get_basic(TY_INT)),
                astLambda(astPlist(1, astParam("x", type_get_basic(TY_INT))),
                          astBin(7, TOK_PLUS, astId(7, "x"), astInt(1))),
                1,
                astPlist(
                    2,
                    astParam("f", astFunctype(type_get_basic(TY_INT), 1, type_get_basic(TY_INT))),
                    astParam("g", astFunctype(type_get_basic(TY_INT), 1, type_get_basic(TY_INT))))),
        astFunc(8, "curry", 0,
                astFunctype(astFunctype(type_get_basic(TY_INT), 1, type_get_basic(TY_INT)), 0),
                astLambda(astPlist(0), astLambda(astPlist(1, astParam("a", type_get_basic(TY_INT))),
                                                 astBin(8, TOK_PLUS, astId(8, "a"), astInt(100)))),
                1, astPlist(0)),
        astFunc(
            9, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                8,
                astLonely(astCall(
                    10, astId(10, "print"), 1,
                    astCall(10, astCall(10, astId(10, "makeMul"), 1, astInt(2)), 1, astInt(5)))),
                astLonely(astCall(11, astId(11, "print"), 1, astStr(" "))),
                astLonely(astCall(
                    12, astId(12, "print"), 1,
                    astCall(12, astCall(12, astId(12, "makeMul"), 1, astInt(3)), 1, astInt(5)))),
                astLonely(astCall(13, astId(13, "print"), 1, astStr(" "))),
                astVardef(14, "c", 0, NULL, astCall(14, astId(14, "curry"), 0)),
                astLonely(astCall(15, astId(15, "print"), 1,
                                  astCall(15, astCall(15, astId(15, "c"), 0), 1, astInt(1)))),
                astLonely(astCall(16, astId(16, "print"), 1, astStr(" "))),
                astLonely(astCall(17, astId(17, "print"), 1,
                                  astCall(17, astCall(17, astCall(17, astId(17, "curry"), 0), 0), 1,
                                          astInt(2))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = List(10, 20, 30)
 *     print(a(0) + a(2))
 *     print(" ")
 *     val b = List(List("x"), List("y", "z"))
 *     print(b(1)(1))
 *     print(" ")
 *     print(List(1.5)(0))
 */
ASTNode *buildListIndex(void) {
    return astProgram(
        1,
        astFunc(
            2, "main", 1, type_get_basic(TY_UNIT),
            astBlock(7, astVardef(3, "a", 0, NULL, astList(3, astInt(10), astInt(20), astInt(30))),
                     astLonely(astCall(4, astId(4, "print"), 1,
                                       astBin(4, TOK_PLUS, astCall(4, astId(4, "a"), 1, astInt(0)),
                                              astCall(4, astId(4, "a"), 1, astInt(2))))),
                     astLonely(astCall(5, astId(5, "print"), 1, astStr(" "))),
                     astVardef(
                         6, "b", 0, NULL,
                         astList(2, astList(1, astStr("x")), astList(2, astStr("y"), astStr("z")))),
                     astLonely(astCall(
                         7, astId(7, "print"), 1,
                         astCall(7, astCall(7, astId(7, "b"), 1, astInt(1)), 1, astInt(1)))),
                     astLonely(astCall(8, astId(8, "print"), 1, astStr(" "))),
                     astLonely(astCall(
                         9, astId(9, "print"), 1,
                         astCall(9, astList(1, astDbl(0x1.8000000000000p+0)), 1, astInt(0))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = List(10, 20, 30)
 *     print(a.head)
 *     print(" ")
 *     val t = a.tail
 *     print(t(0) + t(1))
 *     print(" ")
 *     print(a.tail.tail.head)
 *     val e = List(5).tail
 *     print(" ok")
 */
ASTNode *buildListHeadTail(void) {
    return astProgram(
        1,
        astFunc(
            2, "main", 1, type_get_basic(TY_UNIT),
            astBlock(9, astVardef(3, "a", 0, NULL, astList(3, astInt(10), astInt(20), astInt(30))),
                     astLonely(astCall(4, astId(4, "print"), 1, astMember(astId(4, "a"), "head"))),
                     astLonely(astCall(5, astId(5, "print"), 1, astStr(" "))),
                     astVardef(6, "t", 0, NULL, astMember(astId(6, "a"), "tail")),
                     astLonely(astCall(7, astId(7, "print"), 1,
                                       astBin(7, TOK_PLUS, astCall(7, astId(7, "t"), 1, astInt(0)),
                                              astCall(7, astId(7, "t"), 1, astInt(1))))),
                     astLonely(astCall(8, astId(8, "print"), 1, astStr(" "))),
                     astLonely(astCall(
                         9, astId(9, "print"), 1,
                         astMember(astMember(astMember(astId(9, "a"), "tail"), "tail"), "head"))),
                     astVardef(10, "e", 0, NULL, astMember(astList(1, astInt(5)), "tail")),
                     astLonely(astCall(11, astId(11, "print"), 1, astStr(" ok")))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * def sum(l: List[Int]): Int =
 *     var s = 0
 *     var r = l
 *     var i = 0
 *     while i < 100 do
 *         i = i + 1
 *     s
 * @main def main(): Unit =
 *     val a = List(1, 2, 3, 4, 5)
 *     val d = a.drop(2)
 *     print(d.head)
 *     print(d(2))
 *     val t = a.take(2)
 *     print(t(1))
 *     val x = a.take(10).drop(-3)
 *     print(x(4))
 *     val e = a.drop(10)
 *     val f = a.take(-1)
 *     print(" ok")
 */
ASTNode *buildListDropTake(void) {
    return astProgram(
        2,
        astFunc(
            2, "sum", 0, type_get_basic(TY_INT),
            astBlock(
                5, astVardef(3, "s", 1, NULL, astInt(0)), astVardef(4, "r", 1, NULL, astId(4, "l")),
                astVardef(5, "i", 1, NULL, astInt(0)),
                astWhile(
                    6, astBin(6, TOK_LESS, astId(6, "i"), astInt(100)),
                    astBlock(1, astAssign(7, "i", astBin(7, TOK_PLUS, astId(7, "i"), astInt(1))))),
                astLonely(astId(8, "s"))),
            1, astPlist(1, astParam("l", type_create_list(type_get_basic(TY_INT))))),
        astFunc(
            9, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                11,
                astVardef(10, "a", 0, NULL,
                          astList(5, astInt(1), astInt(2), astInt(3), astInt(4), astInt(5))),
                astVardef(11, "d", 0, NULL,
                          astCall(11, astMember(astId(11, "a"), "drop"), 1, astInt(2))),
                astLonely(astCall(12, astId(12, "print"), 1, astMember(astId(12, "d"), "head"))),
                astLonely(
                    astCall(13, astId(13, "print"), 1, astCall(13, astId(13, "d"), 1, astInt(2)))),
                astVardef(14, "t", 0, NULL,
                          astCall(14, astMember(astId(14, "a"), "take"), 1, astInt(2))),
                astLonely(
                    astCall(15, astId(15, "print"), 1, astCall(15, astId(15, "t"), 1, astInt(1)))),
                astVardef(
                    16, "x", 0, NULL,
                    astCall(16,
                            astMember(astCall(16, astMember(astId(16, "a"), "take"), 1, astInt(10)),
                                      "drop"),
                            1, astUn(16, TOK_MINUS, astInt(3)))),
                astLonely(
                    astCall(17, astId(17, "print"), 1, astCall(17, astId(17, "x"), 1, astInt(4)))),
                astVardef(18, "e", 0, NULL,
                          astCall(18, astMember(astId(18, "a"), "drop"), 1, astInt(10))),
                astVardef(19, "f", 0, NULL,
                          astCall(19, astMember(astId(19, "a"), "take"), 1,
                                  astUn(19, TOK_MINUS, astInt(1)))),
                astLonely(astCall(20, astId(20, "print"), 1, astStr(" ok")))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = List(1, 2, 3, 4, 5)
 *     val s = a.slice(1, 3)
 *     print(s(0))
 *     print(s(1))
 *     val t = a.slice(3, 100)
 *     print(t(1))
 *     val u = a.slice(4, 2)
 *     val v = a.slice(-2, 1)
 *     print(v.head)
 *     val m = a.drop(1).take(3).slice(1, 2)
 *     print(m.head)
 */
ASTNode *buildListSlice(void) {
    return astProgram(
        1,
        astFunc(
            2, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                11,
                astVardef(3, "a", 0, NULL,
                          astList(5, astInt(1), astInt(2), astInt(3), astInt(4), astInt(5))),
                astVardef(4, "s", 0, NULL,
                          astCall(4, astMember(astId(4, "a"), "slice"), 2, astInt(1), astInt(3))),
                astLonely(
                    astCall(5, astId(5, "print"), 1, astCall(5, astId(5, "s"), 1, astInt(0)))),
                astLonely(
                    astCall(6, astId(6, "print"), 1, astCall(6, astId(6, "s"), 1, astInt(1)))),
                astVardef(7, "t", 0, NULL,
                          astCall(7, astMember(astId(7, "a"), "slice"), 2, astInt(3), astInt(100))),
                astLonely(
                    astCall(8, astId(8, "print"), 1, astCall(8, astId(8, "t"), 1, astInt(1)))),
                astVardef(9, "u", 0, NULL,
                          astCall(9, astMember(astId(9, "a"), "slice"), 2, astInt(4), astInt(2))),
                astVardef(10, "v", 0, NULL,
                          astCall(10, astMember(astId(10, "a"), "slice"), 2,
                                  astUn(10, TOK_MINUS, astInt(2)), astInt(1))),
                astLonely(astCall(11, astId(11, "print"), 1, astMember(astId(11, "v"), "head"))),
                astVardef(
                    12, "m", 0, NULL,
                    astCall(
                        12,
                        astMember(astCall(12,
                                          astMember(astCall(12, astMember(astId(12, "a"), "drop"),
                                                            1, astInt(1)),
                                                    "take"),
                                          1, astInt(3)),
                                  "slice"),
                        2, astInt(1), astInt(2))),
                astLonely(astCall(13, astId(13, "print"), 1, astMember(astId(13, "m"), "head")))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = List(1, 2) ++ List(3)
 *     print(a(0))
 *     print(a(1))
 *     print(a(2))
 *     val e: List[Int] = List()
 *     val b = e ++ a ++ e ++ List(4, 5)
 *     print(b(4))
 *     print(b.drop(2).head)
 */
ASTNode *buildListConcat(void) {
    return astProgram(
        1, astFunc(
               2, "main", 1, type_get_basic(TY_UNIT),
               astBlock(8,
                        astVardef(3, "a", 0, NULL,
                                  astBin(3, TOK_CONCAT, astList(2, astInt(1), astInt(2)),
                                         astList(1, astInt(3)))),
                        astLonely(astCall(4, astId(4, "print"), 1,
                                          astCall(4, astId(4, "a"), 1, astInt(0)))),
                        astLonely(astCall(5, astId(5, "print"), 1,
                                          astCall(5, astId(5, "a"), 1, astInt(1)))),
                        astLonely(astCall(6, astId(6, "print"), 1,
                                          astCall(6, astId(6, "a"), 1, astInt(2)))),
                        astVardef(7, "e", 0, type_create_list(type_get_basic(TY_INT)), astList(0)),
                        astVardef(8, "b", 0, NULL,
                                  astBin(8, TOK_CONCAT,
                                         astBin(8, TOK_CONCAT,
                                                astBin(8, TOK_CONCAT, astId(8, "e"), astId(8, "a")),
                                                astId(8, "e")),
                                         astList(2, astInt(4), astInt(5)))),
                        astLonely(astCall(9, astId(9, "print"), 1,
                                          astCall(9, astId(9, "b"), 1, astInt(4)))),
                        astLonely(astCall(
                            10, astId(10, "print"), 1,
                            astMember(astCall(10, astMember(astId(10, "b"), "drop"), 1, astInt(2)),
                                      "head")))),
               1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = List.range(1, 10)
 *     print(a(0))
 *     print(a(8))
 *     print(" ")
 *     val b = List.range(5, 0, -2)
 *     print(b(0))
 *     print(b(1))
 *     print(b(2))
 *     print(" ")
 *     val c = List.range(0, 10, 3)
 *     print(c(3))
 *     print(" ")
 *     val e = List.range(5, 1)
 *     val f = List.range(1, 5, -1)
 *     print("ok")
 */
ASTNode *buildListRange(void) {
    return astProgram(
        1,
        astFunc(2, "main", 1, type_get_basic(TY_UNIT),
                astBlock(15,
                         astVardef(3, "a", 0, NULL,
                                   astCall(3, astMember(astId(3, "List"), "range"), 2, astInt(1),
                                           astInt(10))),
                         astLonely(astCall(4, astId(4, "print"), 1,
                                           astCall(4, astId(4, "a"), 1, astInt(0)))),
                         astLonely(astCall(5, astId(5, "print"), 1,
                                           astCall(5, astId(5, "a"), 1, astInt(8)))),
                         astLonely(astCall(6, astId(6, "print"), 1, astStr(" "))),
                         astVardef(7, "b", 0, NULL,
                                   astCall(7, astMember(astId(7, "List"), "range"), 3, astInt(5),
                                           astInt(0), astUn(7, TOK_MINUS, astInt(2)))),
                         astLonely(astCall(8, astId(8, "print"), 1,
                                           astCall(8, astId(8, "b"), 1, astInt(0)))),
                         astLonely(astCall(9, astId(9, "print"), 1,
                                           astCall(9, astId(9, "b"), 1, astInt(1)))),
                         astLonely(astCall(10, astId(10, "print"), 1,
                                           astCall(10, astId(10, "b"), 1, astInt(2)))),
                         astLonely(astCall(11, astId(11, "print"), 1, astStr(" "))),
                         astVardef(12, "c", 0, NULL,
                                   astCall(12, astMember(astId(12, "List"), "range"), 3, astInt(0),
                                           astInt(10), astInt(3))),
                         astLonely(astCall(13, astId(13, "print"), 1,
                                           astCall(13, astId(13, "c"), 1, astInt(3)))),
                         astLonely(astCall(14, astId(14, "print"), 1, astStr(" "))),
                         astVardef(15, "e", 0, NULL,
                                   astCall(15, astMember(astId(15, "List"), "range"), 2, astInt(5),
                                           astInt(1))),
                         astVardef(16, "f", 0, NULL,
                                   astCall(16, astMember(astId(16, "List"), "range"), 3, astInt(1),
                                           astInt(5), astUn(16, TOK_MINUS, astInt(1)))),
                         astLonely(astCall(17, astId(17, "print"), 1, astStr("ok")))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = (1 to 5).toList
 *     print(a(4))
 *     print(a.drop(4).head)
 *     val b = (0 until 3).toList
 *     print(b(2))
 *     val c = (6 to 2 by -2).toList
 *     print(c(2))
 *     val d = (1 to 10 by 4).toList
 *     print(d(2))
 *     val e = (10 until 0 by -5).toList
 *     print(e(1))
 */
ASTNode *buildRangeToList(void) {
    return astProgram(
        1,
        astFunc(
            2, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                11,
                astVardef(3, "a", 0, NULL,
                          astMember(astRange(astInt(1), astInt(5), NULL, 1), "toList")),
                astLonely(
                    astCall(4, astId(4, "print"), 1, astCall(4, astId(4, "a"), 1, astInt(4)))),
                astLonely(astCall(
                    5, astId(5, "print"), 1,
                    astMember(astCall(5, astMember(astId(5, "a"), "drop"), 1, astInt(4)), "head"))),
                astVardef(6, "b", 0, NULL,
                          astMember(astRange(astInt(0), astInt(3), NULL, 0), "toList")),
                astLonely(
                    astCall(7, astId(7, "print"), 1, astCall(7, astId(7, "b"), 1, astInt(2)))),
                astVardef(
                    8, "c", 0, NULL,
                    astMember(astRange(astInt(6), astInt(2), astUn(8, TOK_MINUS, astInt(2)), 1),
                              "toList")),
                astLonely(
                    astCall(9, astId(9, "print"), 1, astCall(9, astId(9, "c"), 1, astInt(2)))),
                astVardef(10, "d", 0, NULL,
                          astMember(astRange(astInt(1), astInt(10), astInt(4), 1), "toList")),
                astLonely(
                    astCall(11, astId(11, "print"), 1, astCall(11, astId(11, "d"), 1, astInt(2)))),
                astVardef(
                    12, "e", 0, NULL,
                    astMember(astRange(astInt(10), astInt(0), astUn(12, TOK_MINUS, astInt(5)), 0),
                              "toList")),
                astLonely(
                    astCall(13, astId(13, "print"), 1, astCall(13, astId(13, "e"), 1, astInt(1))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val a = List(1, 2, 3)
 *     print(toStr(a))
 *     print(" ")
 *     print(List("a", "b").toString)
 *     print(" ")
 *     print(s"${List(1.5, 2.0)}")
 *     print(" ")
 *     val e: List[Int] = List()
 *     print(e.toString)
 *     print(" ")
 *     print(List(true, false).toString)
 */
ASTNode *buildListToStr(void) {
    return astProgram(
        1,
        astFunc(2, "main", 1, type_get_basic(TY_UNIT),
                astBlock(
                    11, astVardef(3, "a", 0, NULL, astList(3, astInt(1), astInt(2), astInt(3))),
                    astLonely(astCall(4, astId(4, "print"), 1,
                                      astCall(4, astId(4, "toStr"), 1, astId(4, "a")))),
                    astLonely(astCall(5, astId(5, "print"), 1, astStr(" "))),
                    astLonely(astCall(6, astId(6, "print"), 1,
                                      astMember(astList(2, astStr("a"), astStr("b")), "toString"))),
                    astLonely(astCall(7, astId(7, "print"), 1, astStr(" "))),
                    astLonely(astCall(8, astId(8, "print"), 1,
                                      astInterp(1, astList(2, astDbl(0x1.8000000000000p+0),
                                                           astDbl(0x1.0000000000000p+1))))),
                    astLonely(astCall(9, astId(9, "print"), 1, astStr(" "))),
                    astVardef(10, "e", 0, type_create_list(type_get_basic(TY_INT)), astList(0)),
                    astLonely(
                        astCall(11, astId(11, "print"), 1, astMember(astId(11, "e"), "toString"))),
                    astLonely(astCall(12, astId(12, "print"), 1, astStr(" "))),
                    astLonely(astCall(13, astId(13, "print"), 1,
                                      astMember(astList(2, astBool(1), astBool(0)), "toString")))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 * def show(x: List[Int] | Unit): Unit = print(x)
 * @main def main(): Unit =
 *     print(List(1, 2))
 *     print(" ")
 *     print(List(List(1), List(2, 3)))
 *     print(" ")
 *     print(toStr(List(List("a"))))
 *     print(" ")
 *     show(List(5))
 *     show(())
 *     print(" ")
 *     val e: List[Int] = List()
 *     print(e)
 */
ASTNode *buildPrintList(void) {
    return astProgram(
        2,
        astFunc(2, "show", 0, type_get_basic(TY_UNIT),
                astCall(2, astId(2, "print"), 1, astId(2, "x")), 1,
                astPlist(1, astParam("x", astUnion(2, type_create_list(type_get_basic(TY_INT)),
                                                   type_get_basic(TY_UNIT))))),
        astFunc(
            3, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                11, astLonely(astCall(4, astId(4, "print"), 1, astList(2, astInt(1), astInt(2)))),
                astLonely(astCall(5, astId(5, "print"), 1, astStr(" "))),
                astLonely(
                    astCall(6, astId(6, "print"), 1,
                            astList(2, astList(1, astInt(1)), astList(2, astInt(2), astInt(3))))),
                astLonely(astCall(7, astId(7, "print"), 1, astStr(" "))),
                astLonely(
                    astCall(8, astId(8, "print"), 1,
                            astCall(8, astId(8, "toStr"), 1, astList(1, astList(1, astStr("a")))))),
                astLonely(astCall(9, astId(9, "print"), 1, astStr(" "))),
                astLonely(astCall(10, astId(10, "show"), 1, astList(1, astInt(5)))),
                astLonely(astCall(11, astId(11, "show"), 1, astUnit())),
                astLonely(astCall(12, astId(12, "print"), 1, astStr(" "))),
                astVardef(13, "e", 0, type_create_list(type_get_basic(TY_INT)), astList(0)),
                astLonely(astCall(14, astId(14, "print"), 1, astId(14, "e")))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * def even(x: Int): Boolean = x / 2 * 2 == x
 * @main def main(): Unit =
 *     val a = List(1, 2, 3, 4, 5, 6)
 *     print(a.filter((v: Int) => v > 2))
 *     print(" ")
 *     print(a.filter(even))
 *     print(" ")
 *     print(a.filter((v: Int) => v > 10))
 *     print(" ")
 *     print(a.filter((v: Int) => v >= 3 && v <= 5).tail)
 *     print(" ")
 *     var calls = 0
 *     print(List("x", "yy").filter((s: String) => s.length == 2))
 */
ASTNode *buildListFilter(void) {
    return astProgram(
        2,
        astFunc(2, "even", 0, type_get_basic(TY_BOOL),
                astBin(2, TOK_EQUAL,
                       astBin(2, TOK_MULTIPLY, astBin(2, TOK_DIVIDE, astId(2, "x"), astInt(2)),
                              astInt(2)),
                       astId(2, "x")),
                1, astPlist(1, astParam("x", type_get_basic(TY_INT)))),
        astFunc(
            3, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                11,
                astVardef(
                    4, "a", 0, NULL,
                    astList(6, astInt(1), astInt(2), astInt(3), astInt(4), astInt(5), astInt(6))),
                astLonely(
                    astCall(5, astId(5, "print"), 1,
                            astCall(5, astMember(astId(5, "a"), "filter"), 1,
                                    astLambda(astPlist(1, astParam("v", type_get_basic(TY_INT))),
                                              astBin(5, TOK_GREATER, astId(5, "v"), astInt(2)))))),
                astLonely(astCall(6, astId(6, "print"), 1, astStr(" "))),
                astLonely(
                    astCall(7, astId(7, "print"), 1,
                            astCall(7, astMember(astId(7, "a"), "filter"), 1, astId(7, "even")))),
                astLonely(astCall(8, astId(8, "print"), 1, astStr(" "))),
                astLonely(astCall(
                    9, astId(9, "print"), 1,
                    astCall(9, astMember(astId(9, "a"), "filter"), 1,
                            astLambda(astPlist(1, astParam("v", type_get_basic(TY_INT))),
                                      astBin(9, TOK_GREATER, astId(9, "v"), astInt(10)))))),
                astLonely(astCall(10, astId(10, "print"), 1, astStr(" "))),
                astLonely(astCall(
                    11, astId(11, "print"), 1,
                    astMember(
                        astCall(11, astMember(astId(11, "a"), "filter"), 1,
                                astLambda(
                                    astPlist(1, astParam("v", type_get_basic(TY_INT))),
                                    astBin(11, TOK_AND,
                                           astBin(11, TOK_GREATER_EQUAL, astId(11, "v"), astInt(3)),
                                           astBin(11, TOK_LESS_EQUAL, astId(11, "v"), astInt(5))))),
                        "tail"))),
                astLonely(astCall(12, astId(12, "print"), 1, astStr(" "))),
                astVardef(13, "calls", 1, NULL, astInt(0)),
                astLonely(astCall(
                    14, astId(14, "print"), 1,
                    astCall(14, astMember(astList(2, astStr("x"), astStr("yy")), "filter"), 1,
                            astLambda(astPlist(1, astParam("s", type_get_basic(TY_STRING))),
                                      astBin(14, TOK_EQUAL, astMember(astId(14, "s"), "length"),
                                             astInt(2))))))),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val foo = List(1, 2, 6, 89, 20, 1)
 *     print(foo.takeWhile((x: Int) => x < 10))
 *     print(" ")
 *     print(foo.dropWhile((x: Int) => x < 10))
 *     print(" ")
 *     print(foo.takeWhile((x: Int) => x > 100))
 *     print(" ")
 *     print(foo.dropWhile((x: Int) => x < 100))
 *     print(" ")
 *     print(foo.takeWhile((x: Int) => x < 100))
 */
ASTNode *buildListTakeDropWhile(void) {
    return astProgram(
        1, astFunc(2, "main", 1, type_get_basic(TY_UNIT),
                   astBlock(
                       10,
                       astVardef(3, "foo", 0, NULL,
                                 astList(6, astInt(1), astInt(2), astInt(6), astInt(89), astInt(20),
                                         astInt(1))),
                       astLonely(astCall(
                           4, astId(4, "print"), 1,
                           astCall(4, astMember(astId(4, "foo"), "takeWhile"), 1,
                                   astLambda(astPlist(1, astParam("x", type_get_basic(TY_INT))),
                                             astBin(4, TOK_LESS, astId(4, "x"), astInt(10)))))),
                       astLonely(astCall(5, astId(5, "print"), 1, astStr(" "))),
                       astLonely(astCall(
                           6, astId(6, "print"), 1,
                           astCall(6, astMember(astId(6, "foo"), "dropWhile"), 1,
                                   astLambda(astPlist(1, astParam("x", type_get_basic(TY_INT))),
                                             astBin(6, TOK_LESS, astId(6, "x"), astInt(10)))))),
                       astLonely(astCall(7, astId(7, "print"), 1, astStr(" "))),
                       astLonely(astCall(
                           8, astId(8, "print"), 1,
                           astCall(8, astMember(astId(8, "foo"), "takeWhile"), 1,
                                   astLambda(astPlist(1, astParam("x", type_get_basic(TY_INT))),
                                             astBin(8, TOK_GREATER, astId(8, "x"), astInt(100)))))),
                       astLonely(astCall(9, astId(9, "print"), 1, astStr(" "))),
                       astLonely(astCall(
                           10, astId(10, "print"), 1,
                           astCall(10, astMember(astId(10, "foo"), "dropWhile"), 1,
                                   astLambda(astPlist(1, astParam("x", type_get_basic(TY_INT))),
                                             astBin(10, TOK_LESS, astId(10, "x"), astInt(100)))))),
                       astLonely(astCall(11, astId(11, "print"), 1, astStr(" "))),
                       astLonely(astCall(
                           12, astId(12, "print"), 1,
                           astCall(12, astMember(astId(12, "foo"), "takeWhile"), 1,
                                   astLambda(astPlist(1, astParam("x", type_get_basic(TY_INT))),
                                             astBin(12, TOK_LESS, astId(12, "x"), astInt(100))))))),
                   1, astPlist(0)));
}

/*
 * import ifj26.*
 * @main def main(): Unit =
 *     val lst = List(1, 2, 3, 4, 5)
 *     var sum = 0
 *     for v <- lst do sum = sum + v
 *     print(sum)
 *     print(" ")
 *     for
 *         v <- lst
 *         if v > 3
 *     do
 *         print(v)
 *     print(" ")
 *     for s <- List("a", "b") do
 *         for t <- List("x", "y") do
 *             print(s + t)
 *     print(" ")
 *     val e: List[Int] = List()
 *     for v <- e do print(v)
 *     for v <- (1 to 3).toList do print(v)
 */
ASTNode *buildForOverList(void) {
    return astProgram(
        1,
        astFunc(
            2, "main", 1, type_get_basic(TY_UNIT),
            astBlock(
                12,
                astVardef(3, "lst", 0, NULL,
                          astList(5, astInt(1), astInt(2), astInt(3), astInt(4), astInt(5))),
                astVardef(4, "sum", 1, NULL, astInt(0)),
                astFor(5, "v", astId(5, "lst"),
                       astAssign(5, "sum", astBin(5, TOK_PLUS, astId(5, "sum"), astId(5, "v"))), 0),
                astLonely(astCall(6, astId(6, "print"), 1, astId(6, "sum"))),
                astLonely(astCall(7, astId(7, "print"), 1, astStr(" "))),
                astFor(8, "v", astId(9, "lst"),
                       astBlock(1, astLonely(astCall(12, astId(12, "print"), 1, astId(12, "v")))),
                       1, astBin(10, TOK_GREATER, astId(10, "v"), astInt(3))),
                astLonely(astCall(13, astId(13, "print"), 1, astStr(" "))),
                astFor(14, "s", astList(2, astStr("a"), astStr("b")),
                       astBlock(1, astFor(15, "t", astList(2, astStr("x"), astStr("y")),
                                          astBlock(1, astLonely(astCall(16, astId(16, "print"), 1,
                                                                        astBin(16, TOK_PLUS,
                                                                               astId(16, "s"),
                                                                               astId(16, "t"))))),
                                          0)),
                       0),
                astLonely(astCall(17, astId(17, "print"), 1, astStr(" "))),
                astVardef(18, "e", 0, type_create_list(type_get_basic(TY_INT)), astList(0)),
                astFor(19, "v", astId(19, "e"), astCall(19, astId(19, "print"), 1, astId(19, "v")),
                       0),
                astFor(20, "v", astMember(astRange(astInt(1), astInt(3), NULL, 1), "toList"),
                       astCall(20, astId(20, "print"), 1, astId(20, "v")), 0)),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 *
 * @main def main(): Unit =
 *     print("ok")
 *     val s = subStr("abc", 3, 3)
 *     print(s)
 */
ASTNode *buildErrorSubStrStartEqLength(void) {
    return astProgram(1,
                      astFunc(4, "main", 1, type_get_basic(TY_UNIT),
                              astBlock(3, astLonely(astCall(5, astId(5, "print"), 1, astStr("ok"))),
                                       astVardef(6, "s", 0, NULL,
                                                 astCall(6, astId(6, "subStr"), 3, astStr("abc"),
                                                         astInt(3), astInt(3))),
                                       astLonely(astCall(7, astId(7, "print"), 1, astId(7, "s")))),
                              1, astPlist(0)));
}

/*
 * import ifj26.*
 *
 * @main def main(): Unit =
 *     print("ok")
 *     val s = subStr("abc", 1, 5)
 *     print(s)
 */
ASTNode *buildErrorSubStrEnd(void) {
    return astProgram(1,
                      astFunc(4, "main", 1, type_get_basic(TY_UNIT),
                              astBlock(3, astLonely(astCall(5, astId(5, "print"), 1, astStr("ok"))),
                                       astVardef(6, "s", 0, NULL,
                                                 astCall(6, astId(6, "subStr"), 3, astStr("abc"),
                                                         astInt(1), astInt(5))),
                                       astLonely(astCall(7, astId(7, "print"), 1, astId(7, "s")))),
                              1, astPlist(0)));
}

/*
 * import ifj26.*
 *
 * @main def main(): Unit =
 *     print("ok")
 *     val c = chr(200)
 *     print(c)
 */
ASTNode *buildErrorChrOutOfRange(void) {
    return astProgram(
        1, astFunc(4, "main", 1, type_get_basic(TY_UNIT),
                   astBlock(3, astLonely(astCall(5, astId(5, "print"), 1, astStr("ok"))),
                            astVardef(6, "c", 0, NULL, astCall(6, astId(6, "chr"), 1, astInt(200))),
                            astLonely(astCall(7, astId(7, "print"), 1, astId(7, "c")))),
                   1, astPlist(0)));
}

/*
 * import ifj26.*
 *
 * @main def main(): Unit =
 *     print("ok")
 *     val z = 0
 *     val r = 5 / z
 *     print(r)
 */
ASTNode *buildErrorIntDivZero(void) {
    return astProgram(
        1, astFunc(
               4, "main", 1, type_get_basic(TY_UNIT),
               astBlock(4, astLonely(astCall(5, astId(5, "print"), 1, astStr("ok"))),
                        astVardef(6, "z", 0, NULL, astInt(0)),
                        astVardef(7, "r", 0, NULL, astBin(7, TOK_DIVIDE, astInt(5), astId(7, "z"))),
                        astLonely(astCall(8, astId(8, "print"), 1, astId(8, "r")))),
               1, astPlist(0)));
}

/*
 * import ifj26.*
 *
 * @main def main(): Unit =
 *     print("ok")
 *     var s = 0
 *     for i <- 1 to 3 by s do
 *         print(i)
 */
ASTNode *buildErrorZeroStepVariable(void) {
    return astProgram(
        1,
        astFunc(
            4, "main", 1, type_get_basic(TY_UNIT),
            astBlock(3, astLonely(astCall(5, astId(5, "print"), 1, astStr("ok"))),
                     astVardef(6, "s", 1, NULL, astInt(0)),
                     astFor(7, "i", astRange(astInt(1), astInt(3), astId(7, "s"), 1),
                            astBlock(1, astLonely(astCall(8, astId(8, "print"), 1, astId(8, "i")))),
                            0)),
            1, astPlist(0)));
}

/*
 * import ifj26.*
 *
 * @main def main(): Unit =
 *     print("ok")
 *     val e: List[Int] = List()
 *     val x = e.head
 *     print(x)
 */
ASTNode *buildErrorHeadOfEmpty(void) {
    return astProgram(
        1,
        astFunc(4, "main", 1, type_get_basic(TY_UNIT),
                astBlock(4, astLonely(astCall(5, astId(5, "print"), 1, astStr("ok"))),
                         astVardef(6, "e", 0, type_create_list(type_get_basic(TY_INT)), astList(0)),
                         astVardef(7, "x", 0, NULL, astMember(astId(7, "e"), "head")),
                         astLonely(astCall(8, astId(8, "print"), 1, astId(8, "x")))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 *
 * @main def main(): Unit =
 *     print("ok")
 *     val e: List[Int] = List()
 *     val x = e.tail
 *     print(x)
 */
ASTNode *buildErrorTailOfEmpty(void) {
    return astProgram(
        1,
        astFunc(4, "main", 1, type_get_basic(TY_UNIT),
                astBlock(4, astLonely(astCall(5, astId(5, "print"), 1, astStr("ok"))),
                         astVardef(6, "e", 0, type_create_list(type_get_basic(TY_INT)), astList(0)),
                         astVardef(7, "x", 0, NULL, astMember(astId(7, "e"), "tail")),
                         astLonely(astCall(8, astId(8, "print"), 1, astId(8, "x")))),
                1, astPlist(0)));
}

/*
 * import ifj26.*
 *
 * @main def main(): Unit =
 *     print("ok")
 *     val l = List(1, 2)
 *     val x = l(5)
 *     print(x)
 */
ASTNode *buildErrorListIndex(void) {
    return astProgram(
        1, astFunc(4, "main", 1, type_get_basic(TY_UNIT),
                   astBlock(4, astLonely(astCall(5, astId(5, "print"), 1, astStr("ok"))),
                            astVardef(6, "l", 0, NULL, astList(2, astInt(1), astInt(2))),
                            astVardef(7, "x", 0, NULL, astCall(7, astId(7, "l"), 1, astInt(5))),
                            astLonely(astCall(8, astId(8, "print"), 1, astId(8, "x")))),
                   1, astPlist(0)));
}

/*
 * import ifj26.*
 *
 * @main def main(): Unit =
 *     print("ok")
 *     var s = 0
 *     val l = List.range(0, 5, s)
 *     print(l)
 */
ASTNode *buildErrorListRangeZeroStep(void) {
    return astProgram(1,
                      astFunc(4, "main", 1, type_get_basic(TY_UNIT),
                              astBlock(4, astLonely(astCall(5, astId(5, "print"), 1, astStr("ok"))),
                                       astVardef(6, "s", 1, NULL, astInt(0)),
                                       astVardef(7, "l", 0, NULL,
                                                 astCall(7, astMember(astId(7, "List"), "range"), 3,
                                                         astInt(0), astInt(5), astId(7, "s"))),
                                       astLonely(astCall(8, astId(8, "print"), 1, astId(8, "l")))),
                              1, astPlist(0)));
}

/* spustanie testov */

/**
 * @brief jeden test: program, vstup, ocakavany vystup a navratovy kod interpretu
 */
typedef struct {
    const char *name;
    ASTNode *(*build)(void);
    const char *input;          // standardny vstup, NULL = prazdny
    const char *expectedOutput; // ocakavany vystup interpretu
    int expectedExitCode;       // 0, pri behovej chybe napr. 40
} CodegenCase;

CodegenCase cases[] = {
    {"ReadIntAndDouble", buildReadIntAndDouble, "42\0122.5\012abc\012\012",
     "42 0x1.4p+1 0 0x0p+0 0", 0},
    {"ToStr", buildToStr, NULL, "421.50x()-7", 0},
    {"ToIntAndStrLength", buildToIntAndStrLength, NULL, "3 -2 5 0", 0},
    {"Ord", buildOrd, NULL, "65 67 -1 -1 -1", 0},
    {"Chr", buildChr, NULL, "Aa0", 0},
    {"SubStr", buildSubStr, NULL, "bc|abcdef||", 0},
    {"ExpressionsAndVariables", buildExpressionsAndVariables, NULL,
     "0a#b\134c d2a#b\134c d4a#b\134c d10\0123 0x1.cp+1 -3 2 5\012lt eq ne() "
     "w()0x1.8p+19223372036854775807 0x1.4p+1",
     0},
    {"Functions", buildFunctions, "abc\012", "bigsmall!u7 5 610 10 ()()4", 0},
    {"FunexpArguments", buildFunexpArguments, NULL, "1719abc3121B", 0},
    {"FunexpOperand", buildFunexpOperand, NULL, "35 lt 0x1.6p+2 25/0.504", 0},
    {"BoolLiterals", buildBoolLiterals, NULL, "same ok c", 0},
    {"BoolToStr", buildBoolToStr, NULL, "true,false falsetrue", 0},
    {"ShortCircuitAnd", buildShortCircuitAnd, NULL, "sssfalsefalsetrue", 0},
    {"ShortCircuitOr", buildShortCircuitOr, NULL, "sssstruefalsetruetrue", 0},
    {"IfWithoutElse", buildIfWithoutElse, NULL, "01two2341mid00", 0},
    {"ElseIf", buildElseIf, NULL, "negative;zero;small;big;n.z..", 0},
    {"ReadBoolean", buildReadBoolean, "true\012false\012", "truefalsefalse", 0},
    {"IfAsExpression", buildIfAsExpression, NULL, "57zeroonemany0x1p+0n3!", 0},
    {"OneLineBranches", buildOneLineBranches, NULL, "550x1p+11onesmall2", 0},
    {"ForTo", buildForTo, NULL, "123 149 630122", 0},
    {"ForUntil", buildForUntil, NULL, "012 abc", 0},
    {"ForBy", buildForBy, NULL, "13579 10741 1074 03 321 ", 0},
    {"ForGuards", buildForGuards, NULL, "0124567 .1..3.4", 0},
    {"ValmemToString", buildValmemToString, NULL, "42!1.23trues()", 0},
    {"ValmemToIntLength", buildValmemToIntLength, NULL, "3 -2 3 7", 0},
    {"ValmemFloor", buildValmemFloor, NULL, "0x1.8p+1 -0x1.8p+1 0x1p+2 -0x1p+0", 0},
    {"ValmemChain", buildValmemChain, NULL, "-4 2 3.00 4x", 0},
    {"Interpolation", buildInterpolation, NULL, "IFJ27 costs $0 1.23 true ()|26", 0},
    {"NoParamList", buildNoParamList, NULL, "84!!2hello 42", 0},
    {"MultipleParamLists", buildMultipleParamLists, NULL, "8 abc-1 123", 0},
    {"ExtensionCalls", buildExtensionCalls, NULL, "12 foo6 18 9 24 abab 16", 0},
    {"PrintUnion", buildPrintUnion, NULL, "5 hello 0x1.8p+0 true () ", 0},
    {"ToStrUnion", buildToStrUnion, NULL, "5.00 a 7|7|7 2.50|2.50|2.50 false|false|false ()|()|()",
     0},
    {"UnionEquals", buildUnionEquals, NULL, "falsetruetruetruefalsetrue", 0},
    {"ClassFields", buildClassFields, NULL, "3 25 L6", 0},
    {"MatchExpression", buildMatchExpression, NULL, "10any4block52", 0},
    {"LiteralPattern", buildLiteralPattern, NULL,
     "zero,minus one,letter a,two and half,unit,other,other,10", 0},
    {"TypedPattern", buildTypedPattern, NULL, "int 2,str q,dbl 2.00,unit,numnumother", 0},
    {"ConstructorPattern", buildConstructorPattern, NULL, "point 2,size 6,int", 0},
    {"CaseGuards", buildCaseGuards, NULL, "zero,positive,other,long,other,diag,point 3", 0},
    {"ReadMaybe", buildReadMaybe, "12\012x\0121.5\012", "12 () 0x1.8p+0 () 1.50", 0},
    {"FunctionValueCall", buildFunctionValueCall, NULL, "6 10 3 20", 0},
    {"Lambdas", buildLambdas, NULL, "25 -1 42 18 true40", 0},
    {"LambdaBlockBody", buildLambdaBlockBody, NULL, "10 cislo7 hi f uspelozero", 0},
    {"ReturnFunction", buildReturnFunction, NULL, "10 15 101 102", 0},
    {"ListIndex", buildListIndex, NULL, "40 z 0x1.8p+0", 0},
    {"ListHeadTail", buildListHeadTail, NULL, "10 50 30 ok", 0},
    {"ListDropTake", buildListDropTake, NULL, "3525 ok", 0},
    {"ListSlice", buildListSlice, NULL, "23513", 0},
    {"ListConcat", buildListConcat, NULL, "12353", 0},
    {"ListRange", buildListRange, NULL, "19 531 9 ok", 0},
    {"RangeToList", buildRangeToList, NULL, "552295", 0},
    {"ListToStr", buildListToStr, NULL,
     "List(1, 2, 3) List(a, b) List(1.50, 2.00) List() List(true, false)", 0},
    {"PrintList", buildPrintList, NULL,
     "List(1, 2) List(List(1), List(2, 3)) List(List(a)) List(5)() List()", 0},
    {"ListFilter", buildListFilter, NULL,
     "List(3, 4, 5, 6) List(2, 4, 6) List() List(4, 5) List(yy)", 0},
    {"ListTakeDropWhile", buildListTakeDropWhile, NULL,
     "List(1, 2, 6) List(89, 20, 1) List() List() List(1, 2, 6, 89, 20, 1)", 0},
    {"ForOverList", buildForOverList, NULL, "15 45 axaybxby 123", 0},
    {"ErrorSubStrStartEqLength", buildErrorSubStrStartEqLength, NULL, "ok", 40},
    {"ErrorSubStrEnd", buildErrorSubStrEnd, NULL, "ok", 40},
    {"ErrorChrOutOfRange", buildErrorChrOutOfRange, NULL, "ok", 58},
    {"ErrorIntDivZero", buildErrorIntDivZero, NULL, "ok", 57},
    {"ErrorZeroStepVariable", buildErrorZeroStepVariable, NULL, "ok", 40},
    {"ErrorHeadOfEmpty", buildErrorHeadOfEmpty, NULL, "ok", 40},
    {"ErrorTailOfEmpty", buildErrorTailOfEmpty, NULL, "ok", 40},
    {"ErrorListIndex", buildErrorListIndex, NULL, "ok", 40},
    {"ErrorListRangeZeroStep", buildErrorListRangeZeroStep, NULL, "ok", 40},
};

/**
 * @brief funkcia na zapis retazca do suboru
 * @return true pri uspechu
 */
bool writeFile(const char *path, const char *text) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return false;
    }
    fputs(text, file);
    fclose(file);
    return true;
}

/**
 * @brief funkcia na nacitanie celeho suboru, vysledok treba uvolnit
 * @return obsah suboru alebo NULL
 */
char *readFile(const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return NULL;
    }

    size_t capacity = 256;
    size_t length = 0;
    char *text = malloc(capacity);
    int character;

    while ((character = fgetc(file)) != EOF) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            text = realloc(text, capacity);
        }
        text[length++] = (char)character;
    }

    text[length] = '\0';
    fclose(file);
    return text;
}

/**
 * @brief funkcia na spustenie jedneho testu
 * @param testCase je test
 * @param interpreter je cesta k ic26int
 */
void runCase(const CodegenCase *testCase, const char *interpreter) {
    char codePath[256];
    char inputPath[256];
    char outputPath[256];
    char command[1024];

    snprintf(codePath, sizeof(codePath), "tests/tmp/codegen_%s.code", testCase->name);
    snprintf(inputPath, sizeof(inputPath), "tests/tmp/codegen_%s.in", testCase->name);
    snprintf(outputPath, sizeof(outputPath), "tests/tmp/codegen_%s.out", testCase->name);

    // semantika a generator
    ASTNode *program = testCase->build();
    Symtable table;
    ErrorCode error = ERR_OK;
    symtableInit(&table);
    semanticAnalyze(program, &table, &error);

    if (error != ERR_OK) {
        fprintf(stderr, "%s: semantika vratila %d\n", testCase->name, error);
        failureCount++;
        symtableFree(&table);
        return;
    }

    FILE *code = fopen(codePath, "w");
    if (code == NULL) {
        fprintf(stderr, "%s: neda sa vytvorit %s\n", testCase->name, codePath);
        failureCount++;
        symtableFree(&table);
        return;
    }

    int result = codegenGenerate((ProgramNode *)program, code);
    fclose(code);
    symtableFree(&table);

    if (result != ERR_OK) {
        fprintf(stderr, "%s: codegenGenerate vratil %d\n", testCase->name, result);
        failureCount++;
        return;
    }

    // interpret
    writeFile(inputPath, testCase->input != NULL ? testCase->input : "");
    snprintf(command, sizeof(command), "\"%s\" %s < %s > %s 2>/dev/null", interpreter, codePath,
             inputPath, outputPath);

    int status = system(command);
    int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    char *output = readFile(outputPath);

    if (exitCode != testCase->expectedExitCode) {
        fprintf(stderr, "%s: interpret skoncil s %d, ocakavane %d (kod v %s)\n", testCase->name,
                exitCode, testCase->expectedExitCode, codePath);
        failureCount++;
    } else if (output == NULL || strcmp(output, testCase->expectedOutput) != 0) {
        fprintf(stderr, "%s: zly vystup\n  ocakavane: \"%s\"\n  dostali:   \"%s\"\n",
                testCase->name, testCase->expectedOutput, output != NULL ? output : "(nic)");
        failureCount++;
    }

    free(output);
}

int main(void) {
    const char *interpreter = getenv("IC26INT");

    if (interpreter == NULL) {
        interpreter = "tests/bin/ic26int-linux-x86_64";
    }

    // bez interpretu sa vystup overit neda
    if (access(interpreter, X_OK) != 0) {
        printf("test_codegen: PRESKOCENE (interpret %s nie je k dispozicii)\n", interpreter);
        return 0;
    }

    system("mkdir -p tests/tmp");

    size_t caseCount = sizeof(cases) / sizeof(cases[0]);
    for (size_t caseIndex = 0; caseIndex < caseCount; caseIndex++) {
        runCase(&cases[caseIndex], interpreter);
    }

    type_free_all();

    if (failureCount > 0) {
        fprintf(stderr, "test_codegen: %d chyb z %zu testov\n", failureCount, caseCount);
        return 1;
    }

    printf("test_codegen: OK (%zu testov)\n", caseCount);
    return 0;
}

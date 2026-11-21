/**
 * @file test_ast.c
 * @brief Unit testy AST uzlov (src/parser/ast.c)
 *
 * Použitie AI: tento súbor vygenerovala AI (Claude Opus 5.5).
 * Prompt: "Vytvor alebo aktualizujunit testy pre AST uzly v src/parser/ast.c podľa ast.h"
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

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failureCount = 0;

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "%s:%d: CHECK zlyhal: %s\n", __FILE__, __LINE__, #condition);          \
            failureCount++;                                                                        \
        }                                                                                          \
    } while (0)

static void testBinaryTree(void) {
    // a + b * 2
    ASTNode *a = astNewId("a", 1);
    ASTNode *b = astNewId("b", 1);
    ASTNode *two = astNewIntLit(2, 1);
    ASTNode *mul = astNewBinary(TOK_MULTIPLY, b, two, 1);
    ASTNode *add = astNewBinary(TOK_PLUS, a, mul, 1);

    CHECK(add != NULL && mul != NULL && a != NULL && b != NULL && two != NULL);
    if (add == NULL) {
        astFree(mul);
        astFree(a);
        return;
    }

    CHECK(add->kind == AST_BINARY);
    CHECK(add->line == 1);
    CHECK(add->type == NULL);

    BinaryNode *addBin = (BinaryNode *)add;
    CHECK(addBin->operator == TOK_PLUS);
    CHECK(addBin->left == a);
    CHECK(addBin->right == mul);

    CHECK(a->kind == AST_ID);
    CHECK(strcmp(((IDNode *)a)->name, "a") == 0);
    CHECK(((IDNode *)a)->idKind == ID_UNRESOLVED);
    CHECK(((IDNode *)a)->symbol == NULL);

    BinaryNode *mulBin = (BinaryNode *)mul;
    CHECK(mul->kind == AST_BINARY);
    CHECK(mulBin->operator == TOK_MULTIPLY);
    CHECK(mulBin->left == b);
    CHECK(mulBin->right == two);

    CHECK(b->kind == AST_ID);
    CHECK(strcmp(((IDNode *)b)->name, "b") == 0);

    CHECK(two->kind == AST_LIT);
    LiteralNode *twoLit = (LiteralNode *)two;
    CHECK(twoLit->litType == LIT_INT);
    CHECK(twoLit->litValue.int_value == 2);

    astFree(add);
}

static void testLiteralsAndUnary(void) {
    char nameBuf[16] = "x";
    char strBuf[16] = "ahoj";

    ASTNode *id = astNewId(nameBuf, 5);
    ASTNode *str = astNewStringLit(strBuf, 6);
    ASTNode *dbl = astNewDoubleLit(3.5, 7);
    ASTNode *flag = astNewBoolLit(true, 8);
    ASTNode *unit = astNewUnitLit(9);

    nameBuf[0] = 'z';
    strBuf[0] = 'b';

    CHECK(id != NULL && str != NULL && dbl != NULL && flag != NULL && unit != NULL);

    if (id != NULL) {
        CHECK(id->kind == AST_ID);
        CHECK(id->line == 5);
        CHECK(strcmp(((IDNode *)id)->name, "x") == 0);
    }
    if (str != NULL) {
        LiteralNode *lit = (LiteralNode *)str;
        CHECK(lit->litType == LIT_STRING);
        CHECK(strcmp(lit->litValue.string_value, "ahoj") == 0);
    }
    if (dbl != NULL) {
        LiteralNode *lit = (LiteralNode *)dbl;
        CHECK(lit->litType == LIT_DOUBLE);
        CHECK(lit->litValue.float_value == 3.5);
    }
    if (flag != NULL) {
        LiteralNode *lit = (LiteralNode *)flag;
        CHECK(lit->litType == LIT_BOOLEAN);
        CHECK(lit->litValue.bool_value == true);
    }
    if (unit != NULL) {
        LiteralNode *lit = (LiteralNode *)unit;
        CHECK(lit->litType == LIT_UNIT);
        CHECK(unit->line == 9);
    }

    ASTNode *neg = astNewUnary(TOK_MINUS, dbl, 10);
    ASTNode *notNode = astNewUnary(TOK_NOT, flag, 11);
    CHECK(neg != NULL && notNode != NULL);

    if (neg != NULL) {
        CHECK(neg->kind == AST_UNARY);
        CHECK(neg->line == 10);
        CHECK(((UnaryNode *)neg)->operator == TOK_MINUS);
        CHECK(((UnaryNode *)neg)->right == dbl);
    }
    if (notNode != NULL) {
        CHECK(notNode->kind == AST_UNARY);
        CHECK(notNode->line == 11);
        CHECK(((UnaryNode *)notNode)->operator == TOK_NOT);
        CHECK(((UnaryNode *)notNode)->right == flag);
    }

    astFree(NULL);
    astFree(id);
    astFree(str);
    astFree(neg);
    astFree(notNode);
    astFree(unit);
}

static void testAstPrint(void) {
    ASTNode *a = astNewId("a", 1);
    ASTNode *b = astNewId("b", 1);
    ASTNode *two = astNewIntLit(2, 1);
    ASTNode *mul = astNewBinary(TOK_MULTIPLY, b, two, 1);
    ASTNode *add = astNewBinary(TOK_PLUS, a, mul, 1);
    CHECK(add != NULL && mul != NULL && a != NULL && b != NULL && two != NULL);
    if (add == NULL) {
        astFree(mul);
        astFree(a);
        return;
    }

    FILE *out = tmpfile();
    CHECK(out != NULL);
    if (out != NULL) {
        astPrint(add, 0, out);
        astPrint(NULL, 1, out);
        rewind(out);

        char buf[512];
        size_t n = fread(buf, 1, sizeof(buf) - 1, out);
        buf[n] = '\0';
        fclose(out);

        CHECK(n > 0);
        CHECK(strstr(buf, "Binary(+)") != NULL);
        CHECK(strstr(buf, "  Id(a)") != NULL);
        CHECK(strstr(buf, "  Binary(*)") != NULL);
        CHECK(strstr(buf, "    Id(b)") != NULL);
        CHECK(strstr(buf, "    Int(2)") != NULL);
        CHECK(strstr(buf, "  (null)") != NULL);
    }

    astPrint(add, 0, NULL);
    astFree(add);
}

static void testBlockWithIfAndStatements(void) {
    char varName[16] = "x";
    char assignName[16] = "x";

    ASTNode *block = astNewBlock(10);
    ASTNode *varDef = astNewVarDef(varName, true, NULL, astNewIntLit(1, 10), 10);
    varName[0] = 'q';

    ASTNode *cond = astNewBinary(TOK_LESS, astNewId("x", 11), astNewIntLit(10, 11), 11);
    ASTNode *thenBlk = astNewBlock(11);
    ASTNode *assign = astNewAssign(
        assignName, astNewBinary(TOK_PLUS, astNewId("x", 12), astNewIntLit(1, 12), 12), 12);
    assignName[0] = 'w';
    ASTNode *elseBlk = astNewBlock(13);
    ASTNode *retVal = astNewReturn(astNewId("x", 14), 14);
    ASTNode *ifNode = astNewIf(cond, thenBlk, elseBlk, 11);

    ASTNode *whileBlk = astNewBlock(15);
    ASTNode *exprStmt = astNewExprStatement(astNewIntLit(42, 16), 16);
    ASTNode *whileNode = astNewWhile(astNewBoolLit(false, 15), whileBlk, 15);
    ASTNode *ifNoElse = astNewIf(astNewBoolLit(true, 17), astNewReturn(NULL, 17), NULL, 17);

    CHECK(block != NULL && varDef != NULL && thenBlk != NULL && elseBlk != NULL && ifNode != NULL);
    CHECK(whileBlk != NULL && exprStmt != NULL && whileNode != NULL && ifNoElse != NULL);
    if (!block || !varDef || !thenBlk || !elseBlk || !ifNode || !whileBlk || !exprStmt ||
        !whileNode || !ifNoElse) {
        return;
    }

    CHECK(astAppend(&((BlockNode *)thenBlk)->commandOrder,
                    &((BlockNode *)thenBlk)->commandOrderCount, assign));
    CHECK(astAppend(&((BlockNode *)elseBlk)->commandOrder,
                    &((BlockNode *)elseBlk)->commandOrderCount, retVal));
    CHECK(astAppend(&((BlockNode *)whileBlk)->commandOrder,
                    &((BlockNode *)whileBlk)->commandOrderCount, exprStmt));

    BlockNode *blk = (BlockNode *)block;
    CHECK(astAppend(&blk->commandOrder, &blk->commandOrderCount, varDef));
    CHECK(astAppend(&blk->commandOrder, &blk->commandOrderCount, ifNode));
    CHECK(astAppend(&blk->commandOrder, &blk->commandOrderCount, whileNode));
    CHECK(astAppend(&blk->commandOrder, &blk->commandOrderCount, ifNoElse));
    CHECK(blk->commandOrderCount == 4);

    CHECK(strcmp(((VarOrValNode *)varDef)->name, "x") == 0);
    CHECK(((VarOrValNode *)varDef)->isMutable == true);
    CHECK(strcmp(((AssignNode *)assign)->name, "x") == 0);
    CHECK(((IfNode *)ifNoElse)->elseBranch == NULL);

    FILE *out = tmpfile();
    CHECK(out != NULL);
    if (out != NULL) {
        astPrint(block, 0, out);
        rewind(out);
        char buf[1024];
        size_t n = fread(buf, 1, sizeof(buf) - 1, out);
        buf[n] = '\0';
        fclose(out);
        CHECK(strstr(buf, "Block") != NULL);
        CHECK(strstr(buf, "Var(x)") != NULL);
        CHECK(strstr(buf, "If") != NULL);
        CHECK(strstr(buf, "Assign(x)") != NULL);
        CHECK(strstr(buf, "Return") != NULL);
        CHECK(strstr(buf, "While") != NULL);
        CHECK(strstr(buf, "ExprStmt") != NULL);
    }

    astFree(block);
}

static void testNestedCall(void) {
    // f(1)(2) sa sklada z dvoch vnorenych CallNode cez caller
    ASTNode *call1 = astNewCall(astNewId("f", 20), 20);
    CHECK(call1 != NULL);
    if (call1 == NULL) {
        return;
    }
    CHECK(astAppend(&((CallNode *)call1)->args, &((CallNode *)call1)->argsCount,
                    astNewIntLit(1, 20)));

    ASTNode *call2 = astNewCall(call1, 20);
    CHECK(call2 != NULL);
    if (call2 == NULL) {
        astFree(call1);
        return;
    }
    CHECK(astAppend(&((CallNode *)call2)->args, &((CallNode *)call2)->argsCount,
                    astNewIntLit(2, 20)));

    CallNode *outer = (CallNode *)call2;
    CHECK(outer->node.kind == AST_CALL);
    CHECK(outer->caller == call1);
    CHECK(outer->argsCount == 1);
    CHECK(((CallNode *)call1)->argsCount == 1);

    FILE *out = tmpfile();
    CHECK(out != NULL);
    if (out != NULL) {
        astPrint(call2, 0, out);
        rewind(out);
        char buf[512];
        size_t n = fread(buf, 1, sizeof(buf) - 1, out);
        buf[n] = '\0';
        fclose(out);
        CHECK(strstr(buf, "Call") != NULL);
        CHECK(strstr(buf, "Id(f)") != NULL);
        CHECK(strstr(buf, "Int(1)") != NULL);
        CHECK(strstr(buf, "Int(2)") != NULL);
    }

    astFree(call2);
}

static void testProgramAndFunction(void) {
    Type *intType = type_get_basic(TY_INT);
    ASTNode *prog = astNewProgram(1);
    CHECK(prog != NULL && intType != NULL);
    if (prog == NULL) {
        type_free_all();
        return;
    }

    // 1) @main def main() = { return } -> jeden prazdny zoznam parametrov
    ASTNode *mainFnNode = astNewFuncDef("main", true, 2);
    CHECK(mainFnNode != NULL);
    if (mainFnNode != NULL) {
        DefFuncNode *mainFn = (DefFuncNode *)mainFnNode;
        ParamList *emptyList = astFuncAddParamList(mainFn);
        CHECK(emptyList != NULL);
        CHECK(mainFn->isMain == true);
        CHECK(mainFn->paramListCount == 1);
        CHECK(emptyList != NULL && emptyList->params_count == 0);
        ASTNode *blk = astNewBlock(2);
        CHECK(blk != NULL);
        if (blk != NULL) {
            CHECK(astAppend(&((BlockNode *)blk)->commandOrder,
                            &((BlockNode *)blk)->commandOrderCount, astNewReturn(NULL, 2)));
        }
        mainFn->body = blk;
        CHECK(astAppend(&((ProgramNode *)prog)->items, &((ProgramNode *)prog)->itemsCount,
                        mainFnNode));
    }

    // 2) def add(a: Int)(b: Int): Int = { return a + b } -> dva zoznamy parametrov
    ASTNode *addFnNode = astNewFuncDef("add", false, 5);
    CHECK(addFnNode != NULL);
    if (addFnNode != NULL) {
        DefFuncNode *addFn = (DefFuncNode *)addFnNode;
        addFn->returnType = intType;
        char p1[8] = "a";
        char p2[8] = "b";
        ParamList *list1 = astFuncAddParamList(addFn);
        CHECK(list1 != NULL);
        if (list1 != NULL) {
            CHECK(astParamListAdd(list1, p1, intType, 5));
        }
        ParamList *list2 = astFuncAddParamList(addFn);
        CHECK(list2 != NULL);
        if (list2 != NULL) {
            CHECK(astParamListAdd(list2, p2, intType, 5));
        }
        p1[0] = 'x';
        p2[0] = 'y';
        if (addFn->paramListCount == 2) {
            CHECK(strcmp(addFn->paramList[0].params[0].name, "a") == 0);
            CHECK(strcmp(addFn->paramList[1].params[0].name, "b") == 0);
            CHECK(addFn->paramList[0].params[0].symbol == NULL);
            CHECK(addFn->paramList[1].params[0].type == intType);
        }
        CHECK(addFn->paramListCount == 2);
        ASTNode *sum = astNewBinary(TOK_PLUS, astNewId("a", 5), astNewId("b", 5), 5);
        addFn->body = astNewReturn(sum, 5);
        CHECK(astAppend(&((ProgramNode *)prog)->items, &((ProgramNode *)prog)->itemsCount,
                        addFnNode));
    }

    // 3) def answer: Int = 42 -> 0 zoznamov parametrov
    ASTNode *ansFnNode = astNewFuncDef("answer", false, 8);
    CHECK(ansFnNode != NULL);
    if (ansFnNode != NULL) {
        DefFuncNode *ansFn = (DefFuncNode *)ansFnNode;
        ansFn->returnType = intType;
        ansFn->body = astNewIntLit(42, 8);
        CHECK(ansFn->paramListCount == 0);
        CHECK(astAppend(&((ProgramNode *)prog)->items, &((ProgramNode *)prog)->itemsCount,
                        ansFnNode));
    }

    CHECK(((ProgramNode *)prog)->itemsCount == 3);

    FILE *out = tmpfile();
    CHECK(out != NULL);
    if (out != NULL) {
        astPrint(prog, 0, out);
        rewind(out);
        char buf[1024];
        size_t n = fread(buf, 1, sizeof(buf) - 1, out);
        buf[n] = '\0';
        fclose(out);
        CHECK(strstr(buf, "Program") != NULL);
        CHECK(strstr(buf, "Func(@main main)") != NULL);
        CHECK(strstr(buf, "Params()") != NULL);
        CHECK(strstr(buf, "Func(add)") != NULL);
        CHECK(strstr(buf, "Params(a)") != NULL);
        CHECK(strstr(buf, "Params(b)") != NULL);
        CHECK(strstr(buf, "Func(answer)") != NULL);
    }

    astFree(prog);
    type_free_all();
}

static void testForRangeAndLists(void) {
    // for (i <- 1 to 10 by 2 if i > 3) ...
    ASTNode *range = astNewRange(astNewIntLit(1, 20), astNewIntLit(10, 20), astNewIntLit(2, 20),
                                 true, 20);
    ASTNode *guard = astNewBinary(TOK_GREATER, astNewId("i", 20), astNewIntLit(3, 20), 20);
    ASTNode *forNode = astNewFor("i", range, astNewBlock(21), 20);
    CHECK(range != NULL && guard != NULL && forNode != NULL);
    if (forNode != NULL) {
        ForNode *f = (ForNode *)forNode;
        CHECK(f->node.kind == AST_FOR);
        CHECK(strcmp(f->varName, "i") == 0);
        CHECK(((RangeNode *)f->source)->inclusive == true);
        CHECK(astAppend(&f->guards, &f->guardsCount, guard));
    }

    // List(1, 2) a s"x=${i}"
    ASTNode *list = astNewListLit(22);
    ASTNode *interp = astNewInterp(23);
    CHECK(list != NULL && interp != NULL);
    if (list != NULL) {
        CHECK(astAppend(&((ListLitNode *)list)->elems, &((ListLitNode *)list)->elemsCount,
                        astNewIntLit(1, 22)));
        CHECK(astAppend(&((ListLitNode *)list)->elems, &((ListLitNode *)list)->elemsCount,
                        astNewIntLit(2, 22)));
        CHECK(((ListLitNode *)list)->elemsCount == 2);
    }
    if (interp != NULL) {
        CHECK(astAppend(&((InterpNode *)interp)->parts, &((InterpNode *)interp)->partsCount,
                        astNewStringLit("x=", 23)));
        CHECK(astAppend(&((InterpNode *)interp)->parts, &((InterpNode *)interp)->partsCount,
                        astNewId("i", 23)));
        CHECK(((InterpNode *)interp)->partsCount == 2);
    }

    FILE *out = tmpfile();
    CHECK(out != NULL);
    if (out != NULL) {
        astPrint(forNode, 0, out);
        astPrint(list, 0, out);
        astPrint(interp, 0, out);
        rewind(out);
        char buf[1024];
        size_t n = fread(buf, 1, sizeof(buf) - 1, out);
        buf[n] = '\0';
        fclose(out);
        CHECK(strstr(buf, "For(i)") != NULL);
        CHECK(strstr(buf, "Range(to)") != NULL);
        CHECK(strstr(buf, "List") != NULL);
        CHECK(strstr(buf, "Interp") != NULL);
    }

    astFree(forNode);
    astFree(list);
    astFree(interp);
}

static void testMemberLambdaAndConvert(void) {
    Type *intType = type_get_basic(TY_INT);
    ASTNode *mem = astNewMember(astNewId("p", 30), "x", 30);
    ASTNode *lamBody = astNewBinary(TOK_PLUS, astNewId("a", 31), astNewIntLit(1, 31), 31);
    ASTNode *lamNode = astNewLambda(lamBody, 31);
    ASTNode *conv = astAlloc(sizeof(ConvertNode), AST_CONVERT, 32);
    ASTNode *fnNode = astNewFuncDef("withLocals", false, 33);

    CHECK(mem != NULL && lamNode != NULL && conv != NULL && fnNode != NULL);
    if (mem != NULL) {
        CHECK(mem->kind == AST_MEMBER);
        CHECK(strcmp(((MemberNode *)mem)->name, "x") == 0);
    }
    if (lamNode != NULL) {
        LambdaNode *lam = (LambdaNode *)lamNode;
        CHECK(astParamListAdd(&lam->params, "a", intType, 31));
        lam->locals = calloc(1, sizeof(struct Symbol *));
        lam->localsCount = 1;
    }
    if (conv != NULL) {
        ((ConvertNode *)conv)->operand = astNewIntLit(5, 32);
    }
    if (fnNode != NULL) {
        DefFuncNode *fn = (DefFuncNode *)fnNode;
        fn->locals = calloc(2, sizeof(struct Symbol *));
        fn->localsCount = 2;
        fn->body = astNewUnitLit(33);
    }

    FILE *out = tmpfile();
    CHECK(out != NULL);
    if (out != NULL) {
        astPrint(mem, 0, out);
        astPrint(lamNode, 0, out);
        astPrint(conv, 0, out);
        rewind(out);
        char buf[1024];
        size_t n = fread(buf, 1, sizeof(buf) - 1, out);
        buf[n] = '\0';
        fclose(out);
        CHECK(strstr(buf, "Member(.x)") != NULL);
        CHECK(strstr(buf, "Lambda(a)") != NULL);
        CHECK(strstr(buf, "Convert") != NULL);
    }

    astFree(mem);
    astFree(lamNode);
    astFree(conv);
    astFree(fnNode);
    type_free_all();
}

static void testAdtAndExtension(void) {
    Type *intType = type_get_basic(TY_INT);
    Type *pointType = type_create_named("Point");

    ASTNode *cls = astNewClassDef("Point", 40);
    ASTNode *alias = astNewTypeAlias("Num", intType, 41);
    ASTNode *ext = astNewExtension("p", pointType, 42);
    CHECK(cls != NULL && alias != NULL && ext != NULL);
    if (cls != NULL) {
        ClassDefNode *c = (ClassDefNode *)cls;
        CHECK(astParamListAdd(&c->fields, "x", intType, 40));
        CHECK(astParamListAdd(&c->fields, "y", intType, 40));
    }
    if (ext != NULL) {
        ASTNode *method = astNewFuncDef("norm", false, 43);
        ((DefFuncNode *)method)->body = astNewIntLit(0, 43);
        CHECK(astAppend(&((ExtensionNode *)ext)->methods, &((ExtensionNode *)ext)->methodsCount,
                        method));
    }

    ASTNode *match = astNewMatch(astNewId("p", 45), 45);
    ASTNode *cWild = astNewCase(PAT_WILDCARD, 46);
    ASTNode *cLit = astNewCase(PAT_LITERAL, 47);
    ASTNode *cTyped = astNewCase(PAT_TYPED, 48);
    ASTNode *cCtor = astNewCase(PAT_CONSTRUCTOR, 49);
    CHECK(match != NULL && cWild && cLit && cTyped && cCtor);
    if (match && cWild && cLit && cTyped && cCtor) {
        ((CaseNode *)cWild)->body = astNewIntLit(0, 46);

        ((CaseNode *)cLit)->literal = astNewIntLit(1, 47);
        ((CaseNode *)cLit)->body = astNewIntLit(10, 47);

        CaseNode *typed = (CaseNode *)cTyped;
        typed->binder.name = astCopyString("n");
        typed->binder.type = intType;
        typed->binder.line = 48;
        typed->body = astNewId("n", 48);

        CaseNode *ctor = (CaseNode *)cCtor;
        ctor->className = astCopyString("Point");
        CHECK(astParamListAdd(&ctor->ctorBinders, "x", NULL, 49));
        CHECK(astParamListAdd(&ctor->ctorBinders, "y", NULL, 49));
        ctor->guard = astNewBinary(TOK_GREATER, astNewId("x", 49), astNewIntLit(0, 49), 49);
        ctor->body = astNewId("x", 49);

        MatchNode *m = (MatchNode *)match;
        CHECK(astAppend(&m->cases, &m->casesCount, cWild));
        CHECK(astAppend(&m->cases, &m->casesCount, cLit));
        CHECK(astAppend(&m->cases, &m->casesCount, cTyped));
        CHECK(astAppend(&m->cases, &m->casesCount, cCtor));
    }

    FILE *out = tmpfile();
    CHECK(out != NULL);
    if (out != NULL) {
        astPrint(cls, 0, out);
        astPrint(alias, 0, out);
        astPrint(ext, 0, out);
        astPrint(match, 0, out);
        rewind(out);
        char buf[1024];
        size_t n = fread(buf, 1, sizeof(buf) - 1, out);
        buf[n] = '\0';
        fclose(out);
        CHECK(strstr(buf, "Class(Point)") != NULL);
        CHECK(strstr(buf, "Fields(x, y)") != NULL);
        CHECK(strstr(buf, "TypeAlias(Num)") != NULL);
        CHECK(strstr(buf, "Extension(p)") != NULL);
        CHECK(strstr(buf, "Match") != NULL);
        CHECK(strstr(buf, "Case(Point(x, y))") != NULL);
        CHECK(strstr(buf, "Case(_)") != NULL);
    }

    astFree(cls);
    astFree(alias);
    astFree(ext);
    astFree(match);
    type_free_all();
}

int main(void) {
    testBinaryTree();
    testLiteralsAndUnary();
    testAstPrint();
    testBlockWithIfAndStatements();
    testNestedCall();
    testProgramAndFunction();
    testForRangeAndLists();
    testMemberLambdaAndConvert();
    testAdtAndExtension();

    if (failureCount) {
        fprintf(stderr, "test_ast: %d chyb\n", failureCount);
        return 1;
    }
    printf("test_ast: OK\n");
    return 0;
}

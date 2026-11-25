/**
 * @file test_expr.c
 * @brief Unit testy precedenčnej analýzy výrazov (src/parser/expr.c)
 *
 * Použitie AI: tento súbor vygenerovala AI (Claude Opus 5.5).
 * Prompt: "Vytvor alebo aktualizuj unit testy pre precedenčnú analýzu v src/parser/expr.c"
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
#include "error.h"
#include "expr.h"
#include "scanner.h"
#include "types.h"

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

static bool doubleNear(double a, double b, double eps) {
    double diff = a - b;
    return diff < eps && diff > -eps;
}

static int parseExprWithFirst(const char *src, ASTNode *first, ASTNode **out, TokenType *nextTok) {
    *out = NULL;
    FILE *in = tmpfile();
    if (in == NULL) {
        astFree(first);
        return ERR_INTERNAL;
    }
    fputs(src, in);
    rewind(in);

    scanner_init(in);
    Token tok = {0};
    int err = scanner_get_token(&tok);
    if (err == ERR_OK) {
        err = expr_parse(&tok, first, out);
        if (nextTok != NULL) {
            *nextTok = tok.type;
        }
    } else {
        astFree(first);
    }
    dynstr_free(&tok.lexeme);
    scanner_free();
    fclose(in);
    return err;
}

static int parseExprString(const char *src, ASTNode **out, TokenType *nextTok) {
    return parseExprWithFirst(src, NULL, out, nextTok);
}

static void testPrecedenceAndAssoc(void) {
    ASTNode *root = NULL;

    // a + b * 2 -> +(a, *(b, 2))
    CHECK(parseExprString("a + b * 2", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_BINARY);
    if (root != NULL && root->kind == AST_BINARY) {
        BinaryNode *add = (BinaryNode *)root;
        CHECK(add->operator == TOK_PLUS);
        CHECK(add->left != NULL && add->left->kind == AST_ID);
        CHECK(strcmp(((IDNode *)add->left)->name, "a") == 0);
        CHECK(add->right != NULL && add->right->kind == AST_BINARY);
        if (add->right != NULL && add->right->kind == AST_BINARY) {
            BinaryNode *mul = (BinaryNode *)add->right;
            CHECK(mul->operator == TOK_MULTIPLY);
            CHECK(mul->left != NULL && strcmp(((IDNode *)mul->left)->name, "b") == 0);
            CHECK(mul->right != NULL && mul->right->kind == AST_LIT);
            CHECK(((LiteralNode *)mul->right)->litValue.int_value == 2);
        }
    }
    astFree(root);

    // 10 - 3 - 2 -> -(-(10, 3), 2)
    CHECK(parseExprString("10 - 3 - 2", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_BINARY);
    if (root != NULL && root->kind == AST_BINARY) {
        BinaryNode *top = (BinaryNode *)root;
        CHECK(top->operator == TOK_MINUS);
        CHECK(top->right != NULL && ((LiteralNode *)top->right)->litValue.int_value == 2);
        CHECK(top->left != NULL && top->left->kind == AST_BINARY);
        if (top->left != NULL && top->left->kind == AST_BINARY) {
            BinaryNode *inner = (BinaryNode *)top->left;
            CHECK(inner->operator == TOK_MINUS);
            CHECK(((LiteralNode *)inner->left)->litValue.int_value == 10);
            CHECK(((LiteralNode *)inner->right)->litValue.int_value == 3);
        }
    }
    astFree(root);

    // (a + b) * 2
    CHECK(parseExprString("(a + b) * 2", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_BINARY);
    if (root != NULL && root->kind == AST_BINARY) {
        BinaryNode *mul = (BinaryNode *)root;
        CHECK(mul->operator == TOK_MULTIPLY);
        CHECK(mul->left != NULL && mul->left->kind == AST_BINARY);
        CHECK(((BinaryNode *)mul->left)->operator == TOK_PLUS);
    }
    astFree(root);

    // a < b && c == d || !flag -> ||(&&(<(a, b), ==(c, d)), !(flag))
    CHECK(parseExprString("a < b && c == d || !flag", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_BINARY);
    if (root != NULL && root->kind == AST_BINARY) {
        BinaryNode *orNode = (BinaryNode *)root;
        CHECK(orNode->operator == TOK_OR);
        CHECK(orNode->left != NULL && orNode->left->kind == AST_BINARY);
        CHECK(((BinaryNode *)orNode->left)->operator == TOK_AND);
        CHECK(orNode->right != NULL && orNode->right->kind == AST_UNARY);
        CHECK(((UnaryNode *)orNode->right)->operator == TOK_NOT);
    }
    astFree(root);
}

static void testUnaryOps(void) {
    ASTNode *root = NULL;

    // -5
    CHECK(parseExprString("-5", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_UNARY);
    if (root != NULL && root->kind == AST_UNARY) {
        UnaryNode *un = (UnaryNode *)root;
        CHECK(un->operator == TOK_MINUS);
        CHECK(un->right != NULL && un->right->kind == AST_LIT);
        CHECK(((LiteralNode *)un->right)->litValue.int_value == 5);
    }
    astFree(root);

    // +42
    CHECK(parseExprString("+42", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_UNARY);
    if (root != NULL && root->kind == AST_UNARY) {
        UnaryNode *un = (UnaryNode *)root;
        CHECK(un->operator == TOK_PLUS);
        CHECK(un->right != NULL && un->right->kind == AST_LIT);
        CHECK(((LiteralNode *)un->right)->litValue.int_value == 42);
    }
    astFree(root);

    // --x -> -(-(x))
    CHECK(parseExprString("--x", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_UNARY);
    if (root != NULL && root->kind == AST_UNARY) {
        UnaryNode *outer = (UnaryNode *)root;
        CHECK(outer->operator == TOK_MINUS);
        CHECK(outer->right != NULL && outer->right->kind == AST_UNARY);
        if (outer->right != NULL && outer->right->kind == AST_UNARY) {
            UnaryNode *inner = (UnaryNode *)outer->right;
            CHECK(inner->operator == TOK_MINUS);
            CHECK(inner->right != NULL && strcmp(((IDNode *)inner->right)->name, "x") == 0);
        }
    }
    astFree(root);

    // !true
    CHECK(parseExprString("!true", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_UNARY);
    if (root != NULL && root->kind == AST_UNARY) {
        UnaryNode *un = (UnaryNode *)root;
        CHECK(un->operator == TOK_NOT);
        CHECK(un->right != NULL && ((LiteralNode *)un->right)->litType == LIT_BOOLEAN);
        CHECK(((LiteralNode *)un->right)->litValue.bool_value == true);
    }
    astFree(root);

    // -(a + b)
    CHECK(parseExprString("-(a + b)", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_UNARY);
    if (root != NULL && root->kind == AST_UNARY) {
        CHECK(((UnaryNode *)root)->operator == TOK_MINUS);
        CHECK(((UnaryNode *)root)->right != NULL);
        CHECK(((UnaryNode *)root)->right->kind == AST_BINARY);
    }
    astFree(root);
}

static void testNumbersAndStrings(void) {
    ASTNode *root = NULL;

    CHECK(parseExprString("1_000_000", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LIT);
    if (root != NULL && root->kind == AST_LIT) {
        CHECK(((LiteralNode *)root)->litType == LIT_INT);
        CHECK(((LiteralNode *)root)->litValue.int_value == 1000000);
    }
    astFree(root);

    CHECK(parseExprString("1_0.0_1", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LIT);
    if (root != NULL && root->kind == AST_LIT) {
        CHECK(((LiteralNode *)root)->litType == LIT_DOUBLE);
        CHECK(doubleNear(((LiteralNode *)root)->litValue.float_value, 10.01, 1e-9));
    }
    astFree(root);

    CHECK(parseExprString(".5", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LIT);
    if (root != NULL && root->kind == AST_LIT) {
        CHECK(((LiteralNode *)root)->litType == LIT_DOUBLE);
        CHECK(doubleNear(((LiteralNode *)root)->litValue.float_value, 0.5, 1e-9));
    }
    astFree(root);

    CHECK(parseExprString("3.0d", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LIT);
    if (root != NULL && root->kind == AST_LIT) {
        CHECK(((LiteralNode *)root)->litType == LIT_DOUBLE);
        CHECK(doubleNear(((LiteralNode *)root)->litValue.float_value, 3.0, 1e-9));
    }
    astFree(root);

    CHECK(parseExprString("4e+1D", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LIT);
    if (root != NULL && root->kind == AST_LIT) {
        CHECK(((LiteralNode *)root)->litType == LIT_DOUBLE);
        CHECK(doubleNear(((LiteralNode *)root)->litValue.float_value, 40.0, 1e-9));
    }
    astFree(root);

    CHECK(parseExprString("6.02e2_3", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LIT);
    if (root != NULL && root->kind == AST_LIT) {
        CHECK(((LiteralNode *)root)->litType == LIT_DOUBLE);
        CHECK(((LiteralNode *)root)->litValue.float_value > 6.0e23);
        CHECK(((LiteralNode *)root)->litValue.float_value < 6.04e23);
    }
    astFree(root);

    // bezny retazec dekoduje escape sekvencie
    CHECK(parseExprString("\"a\\tb\\\\c\\\"d\\n\"", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LIT);
    if (root != NULL && root->kind == AST_LIT) {
        CHECK(((LiteralNode *)root)->litType == LIT_STRING);
        CHECK(strcmp(((LiteralNode *)root)->litValue.string_value, "a\tb\\c\"d\n") == 0);
    }
    astFree(root);

    // viacriadkovy """...""" je raw a escape sekvencie nedekoduje
    CHECK(parseExprString("\"\"\"x\\ny\n z\"\"\"", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LIT);
    if (root != NULL && root->kind == AST_LIT) {
        CHECK(((LiteralNode *)root)->litType == LIT_STRING);
        CHECK(strcmp(((LiteralNode *)root)->litValue.string_value, "x\\ny\n z") == 0);
    }
    astFree(root);

    // () je Unit literal
    CHECK(parseExprString("()", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LIT);
    if (root != NULL && root->kind == AST_LIT) {
        CHECK(((LiteralNode *)root)->litType == LIT_UNIT);
    }
    astFree(root);
}

static void testCallsMembersListsRanges(void) {
    ASTNode *root = NULL;

    // f(1)(2)
    CHECK(parseExprString("f(1)(2)", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_CALL);
    if (root != NULL && root->kind == AST_CALL) {
        CallNode *outer = (CallNode *)root;
        CHECK(outer->argsCount == 1);
        CHECK(outer->caller != NULL && outer->caller->kind == AST_CALL);
        if (outer->caller != NULL && outer->caller->kind == AST_CALL) {
            CallNode *inner = (CallNode *)outer->caller;
            CHECK(inner->argsCount == 1);
            CHECK(inner->caller != NULL && strcmp(((IDNode *)inner->caller)->name, "f") == 0);
        }
    }
    astFree(root);

    // p.x
    CHECK(parseExprString("p.x", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_MEMBER);
    if (root != NULL && root->kind == AST_MEMBER) {
        MemberNode *mem = (MemberNode *)root;
        CHECK(strcmp(mem->name, "x") == 0);
        CHECK(mem->object != NULL && strcmp(((IDNode *)mem->object)->name, "p") == 0);
    }
    astFree(root);

    // "IFJ26".length
    CHECK(parseExprString("\"IFJ26\".length", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_MEMBER);
    if (root != NULL && root->kind == AST_MEMBER) {
        MemberNode *mem = (MemberNode *)root;
        CHECK(strcmp(mem->name, "length") == 0);
        CHECK(mem->object != NULL && mem->object->kind == AST_LIT);
        CHECK(strcmp(((LiteralNode *)mem->object)->litValue.string_value, "IFJ26") == 0);
    }
    astFree(root);

    // joined.drop(1).take(5)
    CHECK(parseExprString("joined.drop(1).take(5)", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_CALL);
    if (root != NULL && root->kind == AST_CALL) {
        CallNode *takeCall = (CallNode *)root;
        CHECK(takeCall->argsCount == 1);
        CHECK(takeCall->caller != NULL && takeCall->caller->kind == AST_MEMBER);
        if (takeCall->caller != NULL && takeCall->caller->kind == AST_MEMBER) {
            MemberNode *takeMem = (MemberNode *)takeCall->caller;
            CHECK(strcmp(takeMem->name, "take") == 0);
            CHECK(takeMem->object != NULL && takeMem->object->kind == AST_CALL);
        }
    }
    astFree(root);

    // List(1, 2, 3) a List()
    CHECK(parseExprString("List(1, 2, 3)", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LIST_LIT);
    if (root != NULL && root->kind == AST_LIST_LIT) {
        CHECK(((ListLitNode *)root)->elemsCount == 3);
    }
    astFree(root);

    CHECK(parseExprString("List()", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LIST_LIT);
    if (root != NULL && root->kind == AST_LIST_LIT) {
        CHECK(((ListLitNode *)root)->elemsCount == 0);
    }
    astFree(root);

    // List.range(5, 0, -2)
    CHECK(parseExprString("List.range(5, 0, -2)", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_CALL);
    if (root != NULL && root->kind == AST_CALL) {
        CallNode *call = (CallNode *)root;
        CHECK(call->argsCount == 3);
        CHECK(call->caller != NULL && call->caller->kind == AST_MEMBER);
        if (call->caller != NULL && call->caller->kind == AST_MEMBER) {
            MemberNode *mem = (MemberNode *)call->caller;
            CHECK(strcmp(mem->name, "range") == 0);
            CHECK(mem->object != NULL && strcmp(((IDNode *)mem->object)->name, "List") == 0);
        }
    }
    astFree(root);

    // (1 to 6).toList
    CHECK(parseExprString("(1 to 6).toList", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_MEMBER);
    if (root != NULL && root->kind == AST_MEMBER) {
        MemberNode *mem = (MemberNode *)root;
        CHECK(strcmp(mem->name, "toList") == 0);
        CHECK(mem->object != NULL && mem->object->kind == AST_RANGE);
        if (mem->object != NULL && mem->object->kind == AST_RANGE) {
            RangeNode *r = (RangeNode *)mem->object;
            CHECK(r->inclusive == true);
            CHECK(r->step == NULL);
        }
    }
    astFree(root);

    // 1 until 10 by 2
    CHECK(parseExprString("1 until 10 by 2", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_RANGE);
    if (root != NULL && root->kind == AST_RANGE) {
        RangeNode *r = (RangeNode *)root;
        CHECK(r->inclusive == false);
        CHECK(((LiteralNode *)r->from)->litValue.int_value == 1);
        CHECK(((LiteralNode *)r->to)->litValue.int_value == 10);
        CHECK(r->step != NULL && ((LiteralNode *)r->step)->litValue.int_value == 2);
    }
    astFree(root);
}

static void testInterpolatedStrings(void) {
    ASTNode *root = NULL;

    // s"" -> jeden prazdny retazec
    CHECK(parseExprString("s\"\"", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_INTERP);
    if (root != NULL && root->kind == AST_INTERP) {
        InterpNode *in = (InterpNode *)root;
        CHECK(in->partsCount == 1);
        CHECK(in->parts[0]->kind == AST_LIT);
        CHECK(strcmp(((LiteralNode *)in->parts[0])->litValue.string_value, "") == 0);
    }
    astFree(root);

    // s"plain"
    CHECK(parseExprString("s\"plain\"", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_INTERP);
    if (root != NULL && root->kind == AST_INTERP) {
        InterpNode *in = (InterpNode *)root;
        CHECK(in->partsCount == 1);
        CHECK(strcmp(((LiteralNode *)in->parts[0])->litValue.string_value, "plain") == 0);
    }
    astFree(root);

    // s"$course$year" -> 2 identifikatory bez prazdnych stringov medzi nimi
    CHECK(parseExprString("s\"$course$year\"", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_INTERP);
    if (root != NULL && root->kind == AST_INTERP) {
        InterpNode *in = (InterpNode *)root;
        CHECK(in->partsCount == 2);
        CHECK(in->parts[0]->kind == AST_ID &&
              strcmp(((IDNode *)in->parts[0])->name, "course") == 0);
        CHECK(in->parts[1]->kind == AST_ID && strcmp(((IDNode *)in->parts[1])->name, "year") == 0);
    }
    astFree(root);

    // s"$name!"
    CHECK(parseExprString("s\"$name!\"", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_INTERP);
    if (root != NULL && root->kind == AST_INTERP) {
        InterpNode *in = (InterpNode *)root;
        CHECK(in->partsCount == 2);
        CHECK(in->parts[0]->kind == AST_ID && strcmp(((IDNode *)in->parts[0])->name, "name") == 0);
        CHECK(in->parts[1]->kind == AST_LIT);
        CHECK(strcmp(((LiteralNode *)in->parts[1])->litValue.string_value, "!") == 0);
    }
    astFree(root);

    // s"\"q\"\n"
    CHECK(parseExprString("s\"\\\"q\\\"\\n\"", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_INTERP);
    if (root != NULL && root->kind == AST_INTERP) {
        InterpNode *in = (InterpNode *)root;
        CHECK(in->partsCount == 1);
        CHECK(strcmp(((LiteralNode *)in->parts[0])->litValue.string_value, "\"q\"\n") == 0);
    }
    astFree(root);

    // s"cost $$0" -> escapovany dolar
    CHECK(parseExprString("s\"cost $$0\"", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_INTERP);
    if (root != NULL && root->kind == AST_INTERP) {
        InterpNode *in = (InterpNode *)root;
        CHECK(in->partsCount == 1);
        CHECK(strcmp(((LiteralNode *)in->parts[0])->litValue.string_value, "cost $0") == 0);
    }
    astFree(root);

    // s"$name${n + 1} costs $$0"
    CHECK(parseExprString("s\"$name${n + 1} costs $$0\"", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_INTERP);
    if (root != NULL && root->kind == AST_INTERP) {
        InterpNode *in = (InterpNode *)root;
        CHECK(in->partsCount == 3);
        CHECK(in->parts[0]->kind == AST_ID && strcmp(((IDNode *)in->parts[0])->name, "name") == 0);
        CHECK(in->parts[1]->kind == AST_BINARY);
        CHECK(((BinaryNode *)in->parts[1])->operator == TOK_PLUS);
        CHECK(in->parts[2]->kind == AST_LIT);
        CHECK(strcmp(((LiteralNode *)in->parts[2])->litValue.string_value, " costs $0") == 0);
    }
    astFree(root);

    // s"""He said "hi", \n$name.""" -> raw viacriadkovy bez dekodovania \n
    CHECK(parseExprString("s\"\"\"He said \"hi\", \\n$name.\"\"\"", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_INTERP);
    if (root != NULL && root->kind == AST_INTERP) {
        InterpNode *in = (InterpNode *)root;
        CHECK(in->partsCount == 3);
        CHECK(strcmp(((LiteralNode *)in->parts[0])->litValue.string_value,
                     "He said \"hi\", \\n") == 0);
        CHECK(in->parts[1]->kind == AST_ID && strcmp(((IDNode *)in->parts[1])->name, "name") == 0);
        CHECK(strcmp(((LiteralNode *)in->parts[2])->litValue.string_value, ".") == 0);
    }
    astFree(root);

    // s"""raw ${a + 1}."""
    CHECK(parseExprString("s\"\"\"raw ${a + 1}.\"\"\"", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_INTERP);
    if (root != NULL && root->kind == AST_INTERP) {
        InterpNode *in = (InterpNode *)root;
        CHECK(in->partsCount == 3);
        CHECK(strcmp(((LiteralNode *)in->parts[0])->litValue.string_value, "raw ") == 0);
        CHECK(in->parts[1]->kind == AST_BINARY);
        CHECK(strcmp(((LiteralNode *)in->parts[2])->litValue.string_value, ".") == 0);
    }
    astFree(root);

    // vnorena interpolacia s"a ${s"b $x"}"
    CHECK(parseExprString("s\"a ${s\"b $x\"}\"", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_INTERP);
    if (root != NULL && root->kind == AST_INTERP) {
        InterpNode *outer = (InterpNode *)root;
        CHECK(outer->partsCount == 2);
        CHECK(strcmp(((LiteralNode *)outer->parts[0])->litValue.string_value, "a ") == 0);
        CHECK(outer->parts[1]->kind == AST_INTERP);
        if (outer->parts[1]->kind == AST_INTERP) {
            InterpNode *inner = (InterpNode *)outer->parts[1];
            CHECK(inner->partsCount == 2);
            CHECK(strcmp(((LiteralNode *)inner->parts[0])->litValue.string_value, "b ") == 0);
            CHECK(inner->parts[1]->kind == AST_ID &&
                  strcmp(((IDNode *)inner->parts[1])->name, "x") == 0);
        }
    }
    astFree(root);

    // pristup cez bodku na interpolovanom retazci: s"hi $x".length
    CHECK(parseExprString("s\"hi $x\".length", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_MEMBER);
    if (root != NULL && root->kind == AST_MEMBER) {
        MemberNode *mem = (MemberNode *)root;
        CHECK(strcmp(mem->name, "length") == 0);
        CHECK(mem->object != NULL && mem->object->kind == AST_INTERP);
    }
    astFree(root);
}

static void testLambdas(void) {
    ASTNode *root = NULL;

    // () => 42
    CHECK(parseExprString("() => 42", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LAMBDA);
    if (root != NULL && root->kind == AST_LAMBDA) {
        LambdaNode *lam = (LambdaNode *)root;
        CHECK(lam->params.params_count == 0);
        CHECK(lam->body != NULL && lam->body->kind == AST_LIT);
        if (lam->body != NULL && lam->body->kind == AST_LIT) {
            LiteralNode *lit = (LiteralNode *)lam->body;
            CHECK(lit->litType == LIT_INT && lit->litValue.int_value == 42);
        }
    }
    astFree(root);

    // (a: Int) => a * a
    CHECK(parseExprString("(a: Int) => a * a", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LAMBDA);
    if (root != NULL && root->kind == AST_LAMBDA) {
        LambdaNode *lam = (LambdaNode *)root;
        CHECK(lam->params.params_count == 1);
        CHECK(strcmp(lam->params.params[0].name, "a") == 0);
        CHECK(lam->params.params[0].type != NULL && lam->params.params[0].type->kind == TY_INT);
        CHECK(lam->body != NULL && lam->body->kind == AST_BINARY);
        if (lam->body != NULL && lam->body->kind == AST_BINARY) {
            CHECK(((BinaryNode *)lam->body)->operator == TOK_MULTIPLY);
        }
    }
    astFree(root);

    // (left: Int, right: Int) => left + right
    CHECK(parseExprString("(left: Int, right: Int) => left + right", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LAMBDA);
    if (root != NULL && root->kind == AST_LAMBDA) {
        LambdaNode *lam = (LambdaNode *)root;
        CHECK(lam->params.params_count == 2);
        CHECK(strcmp(lam->params.params[0].name, "left") == 0);
        CHECK(lam->params.params[0].type->kind == TY_INT);
        CHECK(strcmp(lam->params.params[1].name, "right") == 0);
        CHECK(lam->params.params[1].type->kind == TY_INT);
        CHECK(lam->body != NULL && lam->body->kind == AST_BINARY);
        if (lam->body != NULL && lam->body->kind == AST_BINARY) {
            CHECK(((BinaryNode *)lam->body)->operator == TOK_PLUS);
        }
    }
    astFree(root);

    // (f: (Int) => Int, xs: List[Int]) => f(xs(0))
    CHECK(parseExprString("(f: (Int) => Int, xs: List[Int]) => f(xs(0))", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LAMBDA);
    if (root != NULL && root->kind == AST_LAMBDA) {
        LambdaNode *lam = (LambdaNode *)root;
        CHECK(lam->params.params_count == 2);
        Type *t0 = lam->params.params[0].type;
        CHECK(t0 != NULL && t0->kind == TY_FUNC);
        if (t0 != NULL && t0->kind == TY_FUNC) {
            CHECK(t0->func.params.len == 1);
            CHECK(t0->func.params.items[0]->kind == TY_INT);
            CHECK(t0->func.ret != NULL && t0->func.ret->kind == TY_INT);
        }
        Type *t1 = lam->params.params[1].type;
        CHECK(t1 != NULL && t1->kind == TY_LIST);
        if (t1 != NULL && t1->kind == TY_LIST) {
            CHECK(t1->elem != NULL && t1->elem->kind == TY_INT);
        }
        CHECK(lam->body != NULL && lam->body->kind == AST_CALL);
    }
    astFree(root);

    // (v: Int | String | Point) => v
    CHECK(parseExprString("(v: Int | String | Point) => v", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LAMBDA);
    if (root != NULL && root->kind == AST_LAMBDA) {
        LambdaNode *lam = (LambdaNode *)root;
        CHECK(lam->params.params_count == 1);
        Type *ut = lam->params.params[0].type;
        CHECK(ut != NULL && ut->kind == TY_UNION);
        if (ut != NULL && ut->kind == TY_UNION) {
            CHECK(ut->members.len == 3);
            CHECK(ut->members.items[0]->kind == TY_INT);
            CHECK(ut->members.items[1]->kind == TY_STRING);
            CHECK(ut->members.items[2]->kind == TY_NAMED);
            if (ut->members.items[2]->kind == TY_NAMED) {
                CHECK(strcmp(ut->members.items[2]->named.name, "Point") == 0);
            }
        }
    }
    astFree(root);

    // (a: Int) => (b: Int) => a + b
    CHECK(parseExprString("(a: Int) => (b: Int) => a + b", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LAMBDA);
    if (root != NULL && root->kind == AST_LAMBDA) {
        LambdaNode *outer = (LambdaNode *)root;
        CHECK(outer->params.params_count == 1);
        CHECK(outer->body != NULL && outer->body->kind == AST_LAMBDA);
        if (outer->body != NULL && outer->body->kind == AST_LAMBDA) {
            LambdaNode *inner = (LambdaNode *)outer->body;
            CHECK(inner->params.params_count == 1);
            CHECK(strcmp(inner->params.params[0].name, "b") == 0);
            CHECK(inner->body != NULL && inner->body->kind == AST_BINARY);
        }
    }
    astFree(root);

    // applyTo((a: Int) => a - 1, 0)
    CHECK(parseExprString("applyTo((a: Int) => a - 1, 0)", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_CALL);
    if (root != NULL && root->kind == AST_CALL) {
        CallNode *call = (CallNode *)root;
        CHECK(call->argsCount == 2);
        CHECK(call->args[0]->kind == AST_LAMBDA);
        CHECK(call->args[1]->kind == AST_LIT);
        if (call->args[1]->kind == AST_LIT) {
            CHECK(((LiteralNode *)call->args[1])->litValue.int_value == 0);
        }
    }
    astFree(root);

    // joined.filter((v: Int) => v >= 3 && v <= 6)
    CHECK(parseExprString("joined.filter((v: Int) => v >= 3 && v <= 6)", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_CALL);
    if (root != NULL && root->kind == AST_CALL) {
        CallNode *call = (CallNode *)root;
        CHECK(call->caller != NULL && call->caller->kind == AST_MEMBER);
        if (call->caller != NULL && call->caller->kind == AST_MEMBER) {
            MemberNode *mem = (MemberNode *)call->caller;
            CHECK(strcmp(mem->name, "filter") == 0);
            CHECK(mem->object != NULL && mem->object->kind == AST_ID);
        }
        CHECK(call->argsCount == 1);
        CHECK(call->args[0]->kind == AST_LAMBDA);
    }
    astFree(root);

    // ((a: Int) => a + 1)(5)
    CHECK(parseExprString("((a: Int) => a + 1)(5)", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_CALL);
    if (root != NULL && root->kind == AST_CALL) {
        CallNode *call = (CallNode *)root;
        CHECK(call->caller != NULL && call->caller->kind == AST_LAMBDA);
        CHECK(call->argsCount == 1);
        CHECK(call->args[0]->kind == AST_LIT);
        if (call->args[0]->kind == AST_LIT) {
            CHECK(((LiteralNode *)call->args[0])->litValue.int_value == 5);
        }
    }
    astFree(root);

    // f(() => 42)
    CHECK(parseExprString("f(() => 42)", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_CALL);
    if (root != NULL && root->kind == AST_CALL) {
        CallNode *call = (CallNode *)root;
        CHECK(call->argsCount == 1);
        CHECK(call->args[0]->kind == AST_LAMBDA);
        if (call->args[0]->kind == AST_LAMBDA) {
            CHECK(((LambdaNode *)call->args[0])->params.params_count == 0);
        }
    }
    astFree(root);

    // () je stale Unit literal a (a + 1) zatvorkovany vyraz
    CHECK(parseExprString("()", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_LIT && ((LiteralNode *)root)->litType == LIT_UNIT);
    astFree(root);

    CHECK(parseExprString("(a + 1)", &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_BINARY);
    if (root != NULL && root->kind == AST_BINARY) {
        CHECK(((BinaryNode *)root)->operator == TOK_PLUS);
    }
    astFree(root);

    type_free_all();
}

static void testSyntaxErrorsAndBoundaries(void) {
    ASTNode *root = NULL;
    TokenType nextTok = TOK_EOF;

    // neplatne vyrazy musia vratit ERR_SYNTAX a *output == NULL
    const char *invalidExprs[] = {
        "",        "1 +",          "(1 + 2",     "1 < 2 < 3",   "1 == 2 == 3",
        "1 to 5 by 2 by 3",        "1 by 2",     "x.5",         "f(1, )",
        "s\"a ${}\"", "(a: ) => 1", "(a: Int) 1", "(a: Int, ) => 1", "a b",
    };
    size_t invalidCount = sizeof(invalidExprs) / sizeof(invalidExprs[0]);
    for (size_t i = 0; i < invalidCount; i++) {
        root = (ASTNode *)0x1;
        CHECK(parseExprString(invalidExprs[i], &root, NULL) == ERR_SYNTAX);
        CHECK(root == NULL);
    }

    // hranice vyrazov: PSA nesmie pohltit oddelovac za vyrazom
    CHECK(parseExprString("x < 10 then y", &root, &nextTok) == ERR_OK);
    CHECK(nextTok == TOK_KW_THEN);
    CHECK(root != NULL && root->kind == AST_BINARY);
    astFree(root);

    CHECK(parseExprString("x > 0 do y", &root, &nextTok) == ERR_OK);
    CHECK(nextTok == TOK_KW_DO);
    CHECK(root != NULL && root->kind == AST_BINARY);
    astFree(root);

    CHECK(parseExprString("a + b)", &root, &nextTok) == ERR_OK);
    CHECK(nextTok == TOK_RPAREN);
    CHECK(root != NULL && root->kind == AST_BINARY);
    astFree(root);

    CHECK(parseExprString("a + b, c", &root, &nextTok) == ERR_OK);
    CHECK(nextTok == TOK_COMMA);
    CHECK(root != NULL && root->kind == AST_BINARY);
    astFree(root);

    CHECK(parseExprString("a + b\nc", &root, &nextTok) == ERR_OK);
    CHECK(nextTok == TOK_EOL);
    CHECK(root != NULL && root->kind == AST_BINARY);
    astFree(root);

    CHECK(parseExprString("()\nx", &root, &nextTok) == ERR_OK);
    CHECK(nextTok == TOK_EOL);
    CHECK(root != NULL && root->kind == AST_LIT && ((LiteralNode *)root)->litType == LIT_UNIT);
    astFree(root);

    CHECK(parseExprString("() => 42\nx", &root, &nextTok) == ERR_OK);
    CHECK(nextTok == TOK_EOL);
    CHECK(root != NULL && root->kind == AST_LAMBDA);
    astFree(root);

    // pokracovanie vyrazu s vopred nacitanym prvym operandom first
    ASTNode *first = astNewId("f", 1);
    CHECK(first != NULL);
    CHECK(parseExprWithFirst("(1, 2)", first, &root, &nextTok) == ERR_OK);
    CHECK(nextTok == TOK_EOL);
    CHECK(root != NULL && root->kind == AST_CALL);
    if (root != NULL && root->kind == AST_CALL) {
        CallNode *call = (CallNode *)root;
        CHECK(call->caller != NULL && call->caller->kind == AST_ID);
        CHECK(strcmp(((IDNode *)call->caller)->name, "f") == 0);
        CHECK(call->argsCount == 2);
    }
    astFree(root);

    first = astNewId("x", 1);
    CHECK(first != NULL);
    CHECK(parseExprWithFirst("+ 3", first, &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_BINARY);
    if (root != NULL && root->kind == AST_BINARY) {
        BinaryNode *bin = (BinaryNode *)root;
        CHECK(bin->operator == TOK_PLUS);
        CHECK(bin->left != NULL && bin->left->kind == AST_ID);
        CHECK(bin->right != NULL && bin->right->kind == AST_LIT);
    }
    astFree(root);

    first = astNewId("obj", 1);
    CHECK(first != NULL);
    CHECK(parseExprWithFirst(".prop", first, &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_MEMBER);
    if (root != NULL && root->kind == AST_MEMBER) {
        MemberNode *mem = (MemberNode *)root;
        CHECK(strcmp(mem->name, "prop") == 0);
        CHECK(mem->object != NULL && mem->object->kind == AST_ID);
    }
    astFree(root);

    first = astNewId("obj", 1);
    CHECK(first != NULL);
    CHECK(parseExprWithFirst(".step(1)", first, &root, NULL) == ERR_OK);
    CHECK(root != NULL && root->kind == AST_CALL);
    if (root != NULL && root->kind == AST_CALL) {
        CallNode *call = (CallNode *)root;
        CHECK(call->caller != NULL && call->caller->kind == AST_MEMBER);
        CHECK(call->argsCount == 1);
    }
    astFree(root);

    first = astNewId("x", 1);
    CHECK(first != NULL);
    CHECK(parseExprWithFirst("then y", first, &root, &nextTok) == ERR_OK);
    CHECK(nextTok == TOK_KW_THEN);
    CHECK(root != NULL && root->kind == AST_ID);
    if (root != NULL && root->kind == AST_ID) {
        CHECK(strcmp(((IDNode *)root)->name, "x") == 0);
    }
    astFree(root);

    // priamy prechod z LL parsera na "x\ny": x uz je nacitane a dalsi token je EOL
    FILE *in = tmpfile();
    CHECK(in != NULL);
    if (in != NULL) {
        fputs("x\ny", in);
        rewind(in);
        scanner_init(in);
        Token tok = {0};
        CHECK(scanner_get_token(&tok) == ERR_OK);
        CHECK(tok.type == TOK_IDENTIFIER);
        ASTNode *preRead = astNewId(tok.lexeme.data, tok.line);
        CHECK(preRead != NULL);
        CHECK(scanner_get_token(&tok) == ERR_OK);
        CHECK(tok.type == TOK_EOL);
        CHECK(expr_parse(&tok, preRead, &root) == ERR_OK);
        CHECK(tok.type == TOK_EOL);
        CHECK(root != NULL && root->kind == AST_ID);
        if (root != NULL && root->kind == AST_ID) {
            CHECK(strcmp(((IDNode *)root)->name, "x") == 0);
        }
        astFree(root);
        dynstr_free(&tok.lexeme);
        scanner_free();
        fclose(in);
    }

    type_free_all();
}

int main(void) {
    testPrecedenceAndAssoc();
    testUnaryOps();
    testNumbersAndStrings();
    testCallsMembersListsRanges();
    testInterpolatedStrings();
    testLambdas();
    testSyntaxErrorsAndBoundaries();

    if (failureCount) {
        fprintf(stderr, "test_expr: %d chyb\n", failureCount);
        return 1;
    }
    printf("test_expr: OK\n");
    return 0;
}

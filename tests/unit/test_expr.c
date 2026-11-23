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

static int parseExprString(const char *src, ASTNode **out, TokenType *nextTok) {
    *out = NULL;
    FILE *in = tmpfile();
    if (in == NULL) {
        return ERR_INTERNAL;
    }
    fputs(src, in);
    rewind(in);

    scanner_init(in);
    Token tok = {0};
    int err = scanner_get_token(&tok);
    if (err == ERR_OK) {
        err = expr_parse(&tok, NULL, out);
        if (nextTok != NULL) {
            *nextTok = tok.type;
        }
    }
    dynstr_free(&tok.lexeme);
    scanner_free();
    fclose(in);
    return err;
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

int main(void) {
    testPrecedenceAndAssoc();
    testUnaryOps();
    testNumbersAndStrings();
    testCallsMembersListsRanges();

    if (failureCount) {
        fprintf(stderr, "test_expr: %d chyb\n", failureCount);
        return 1;
    }
    printf("test_expr: OK\n");
    return 0;
}

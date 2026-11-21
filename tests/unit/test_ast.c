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

int main(void) {
    testBinaryTree();
    testLiteralsAndUnary();
    testAstPrint();

    if (failureCount) {
        fprintf(stderr, "test_ast: %d chyb\n", failureCount);
        return 1;
    }
    printf("test_ast: OK\n");
    return 0;
}

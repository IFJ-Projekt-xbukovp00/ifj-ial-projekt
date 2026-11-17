/**
 * @file test_bst.c
 * @brief Unit testy AVL stromu (src/symtable/bst.c)
 *
 * Po každom vložení kontroluje usporiadanie BVS, správne uložené výšky
 * a vyváženosť (rozdiel výšok podstromov najviac 1).
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

#include "bst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Strom potrebuje len ukazovateľ, test si Symbol nadefinuje po svojom. */
struct Symbol {
    int id;
};

static int failureCount = 0;

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "%s:%d: CHECK zlyhal: %s\n", __FILE__, __LINE__, #condition);          \
            failureCount++;                                                                        \
        }                                                                                          \
    } while (0)

/**
 * Overí invarianty AVL v podstrome. Vráti skutočnú výšku, alebo -1 pri chybe.
 * lowerBound/upperBound sú hranice pre kľúče (NULL = bez hranice).
 */
static int checkAvl(const Node *node, const char *lowerBound, const char *upperBound) {
    if (node == NULL) {
        return 0;
    }
    if ((lowerBound && strcmp(node->name, lowerBound) <= 0) ||
        (upperBound && strcmp(node->name, upperBound) >= 0)) {
        fprintf(stderr, "porusene usporiadanie pri kluci %s\n", node->name);
        return -1;
    }
    int leftHeight = checkAvl(node->left, lowerBound, node->name);
    int rightHeight = checkAvl(node->right, node->name, upperBound);
    if (leftHeight < 0 || rightHeight < 0) {
        return -1;
    }
    int actualHeight = 1 + rightHeight;
    if (leftHeight > rightHeight) {
        actualHeight = 1 + leftHeight;
    }
    if (node->height != actualHeight) {
        fprintf(stderr, "zla vyska v uzle %s: %d, ma byt %d\n", node->name, node->height,
                actualHeight);
        return -1;
    }
    if (leftHeight - rightHeight > 1 || rightHeight - leftHeight > 1) {
        fprintf(stderr, "nevyvazeny uzol %s (%d vs %d)\n", node->name, leftHeight, rightHeight);
        return -1;
    }
    return actualHeight;
}

/* Vloží a overí, že nenastala chyba. */
static Node *insertChecked(Node *root, const char *name, struct Symbol *symbol) {
    ErrorCode error = ERR_OK;
    root = insertNode(root, name, symbol, &error);
    CHECK(error == ERR_OK);
    return root;
}

static struct Symbol *findSymbol(Node *root, const char *name) {
    ErrorCode error = ERR_OK;
    Node *foundNode = findNode(root, name, &error);
    if (foundNode == NULL) {
        return NULL;
    }
    return foundNode->symbol;
}

static void testEmpty(void) {
    CHECK(findSymbol(NULL, "x") == NULL);
    destroyTree(NULL);
}

static void testInsertFind(void) {
    Node *root = NULL;
    struct Symbol symbolA = {1}, symbolB = {2}, symbolC = {3};
    root = insertChecked(root, "b", &symbolB);
    root = insertChecked(root, "a", &symbolA);
    root = insertChecked(root, "c", &symbolC);
    CHECK(findSymbol(root, "a") == &symbolA);
    CHECK(findSymbol(root, "b") == &symbolB);
    CHECK(findSymbol(root, "c") == &symbolC);
    CHECK(findSymbol(root, "d") == NULL);
    CHECK(findSymbol(root, "") == NULL);
    CHECK(checkAvl(root, NULL, NULL) == 2);
    destroyTree(root);
}

static void testDuplicate(void) {
    Node *root = NULL;
    struct Symbol firstSymbol = {1}, secondSymbol = {2};
    root = insertChecked(root, "x", &firstSymbol);
    root = insertChecked(root, "y", &firstSymbol);
    ErrorCode error = ERR_OK;
    Node *returnedRoot = insertNode(root, "x", &secondSymbol, &error);
    CHECK(error == ERR_REDEF);
    CHECK(returnedRoot == root);
    CHECK(findSymbol(root, "x") == &firstSymbol);
    CHECK(checkAvl(root, NULL, NULL) == 2);
    destroyTree(root);
}

static void testKeyIsCopied(void) {
    Node *root = NULL;
    struct Symbol symbol = {1};
    char nameBuffer[8] = "meno";
    root = insertChecked(root, nameBuffer, &symbol);
    strcpy(nameBuffer, "iny");
    CHECK(findSymbol(root, "meno") == &symbol);
    CHECK(findSymbol(root, "iny") == NULL);
    destroyTree(root);
}

/* Všetky štyri prípady rotácií: LL, RR, LR, RL. */
static void testRotations(void) {
    const char *insertOrders[4][3] = {
        {"c", "b", "a"}, /* LL -> pravá rotácia */
        {"a", "b", "c"}, /* RR -> ľavá rotácia */
        {"c", "a", "b"}, /* LR */
        {"a", "c", "b"}, /* RL */
    };
    struct Symbol symbol = {0};
    for (int caseIndex = 0; caseIndex < 4; caseIndex++) {
        Node *root = NULL;
        for (int keyIndex = 0; keyIndex < 3; keyIndex++) {
            root = insertChecked(root, insertOrders[caseIndex][keyIndex], &symbol);
        }
        CHECK(checkAvl(root, NULL, NULL) == 2);
        CHECK(root != NULL && strcmp(root->name, "b") == 0);
        destroyTree(root);
    }
}

/* Veľa kľúčov v zoradenom aj zamiešanom poradí, invarianty po každom vložení. */
static void testManyKeys(int shuffled) {
    enum { KEY_COUNT = 2000 };
    static struct Symbol symbols[KEY_COUNT];
    static char keys[KEY_COUNT][16];
    int insertOrder[KEY_COUNT];
    for (int keyIndex = 0; keyIndex < KEY_COUNT; keyIndex++) {
        symbols[keyIndex].id = keyIndex;
        snprintf(keys[keyIndex], sizeof keys[keyIndex], "k%05d", keyIndex);
        insertOrder[keyIndex] = keyIndex;
    }
    if (shuffled) {
        srand(42);
        for (int keyIndex = KEY_COUNT - 1; keyIndex > 0; keyIndex--) {
            int swapIndex = rand() % (keyIndex + 1);
            int swapValue = insertOrder[keyIndex];
            insertOrder[keyIndex] = insertOrder[swapIndex];
            insertOrder[swapIndex] = swapValue;
        }
    }

    Node *root = NULL;
    int allValid = 1;
    for (int keyIndex = 0; keyIndex < KEY_COUNT && allValid; keyIndex++) {
        int position = insertOrder[keyIndex];
        root = insertChecked(root, keys[position], &symbols[position]);
        if (checkAvl(root, NULL, NULL) < 0) {
            allValid = 0;
        }
    }
    CHECK(allValid);
    for (int keyIndex = 0; keyIndex < KEY_COUNT; keyIndex++) {
        CHECK(findSymbol(root, keys[keyIndex]) == &symbols[keyIndex]);
    }
    /* AVL s 2000 uzlami má výšku najviac 1.44 * log2(2002) ≈ 15.8 */
    CHECK(root != NULL && root->height <= 15);
    destroyTree(root);
}

int main(void) {
    testEmpty();
    testInsertFind();
    testDuplicate();
    testKeyIsCopied();
    testRotations();
    testManyKeys(0);
    testManyKeys(1);

    if (failureCount) {
        fprintf(stderr, "test_bst: %d chyb\n", failureCount);
        return 1;
    }
    printf("test_bst: OK\n");
    return 0;
}

/**
 * @file bst.h
 * @brief Výškovo vyvážený binárny vyhľadávací strom (vv-BVS)
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
#include "token.h"
#include "error.h"

#pragma once

struct Symbol; // definicia v symtable.h

typedef struct Node{
    char *name;
    struct Symbol *symbol;
    int height;
    struct Node *left;
    struct Node *right;
}Node;

int height(Node *node);
void updateHeight(Node *node);
Node *rotateRight(Node *root);
Node *rotateLeft(Node *root);
Node *balance(Node *root);
Node *alocNode(const char *name, struct Symbol *symbol, ErrorCode *error);
Node *insertNode(Node *root, const char *name, struct Symbol *symbol, ErrorCode *error);
Node *findNode(Node *root, const char *name, ErrorCode *error);
void destroyTree(Node *root);


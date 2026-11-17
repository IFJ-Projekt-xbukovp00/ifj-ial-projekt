/**
 * @file symtable.c
 * @brief Tabuľka symbolov (ADT)
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

#include "symtable.h"
#include <stdlib.h>
#include <string.h>

IDs *initNodeID(Token *Node){
    IDs *newNode = malloc(sizeof(IDs));

    if (newNode == NULL){
        return NULL;
    }

    newNode->Node = Node;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

IDs *addNodeID(IDs *root, Token *Node){
    if (root == NULL){
        return initNodeID(Node);
    }

    int cmp = strcmp(Node->lexeme.data, root->Node->lexeme.data);

    if (cmp < 0){
        root->left = addNodeID(root->left, Node);
    }
    else if (cmp > 0){
        root->right = addNodeID(root->right, Node);
    }

    return root;
}

bool findAndControlNodeID(IDs *root, Token *Node){
    if (root == NULL){
        return false;
    }

    int cmp = strcmp(Node->lexeme.data, root->Node->lexeme.data);

    if (cmp == 0){
        return true;
    }
    else if (cmp < 0){
        return findAndControlNodeID(root->left, Node);
    }
    else{
        return findAndControlNodeID(root->right, Node);
    }
}

void clearTreeID(IDs *root){
    if (root == NULL){
        return;
    }
    
    clearTreeID(root->left);
    clearTreeID(root->right);
    free(root);
}


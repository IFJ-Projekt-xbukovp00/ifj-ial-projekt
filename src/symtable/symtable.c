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
    int cmp = strcmp(root->Node->lexeme, Node->lexeme);
}

KeyWordTree *initNodeKW(char *word){
    KeyWordTree *newNode = malloc(sizeof(KeyWordTree));
    if (newNode == NULL){
        return NULL;
    }
    newNode->word = malloc(strlen(word)+1);
    if (newNode->word == NULL){
        free(newNode);
        return NULL;
    }
    strcpy(newNode->word, word);
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}
KeyWordTree *addNodeKW(KeyWordTree *root, char *word){
    if (root == NULL){
        return initNode(word);
    }
    int cmp = strcmp(word, root->word);
    if (cmp < 0){
        addNodeKW(root->left, word);
    }
    else{
        addNodeKW(root->right, word);
    }
    return root;
}
bool findNodeKW(KeyWordTree *root, char *word){
    if (root == NULL){
        return false;
    }
    int cmp = strcmp(word, root->word);
    if (cmp == 0){
        return true;
    }
    else if (cmp < 0){
        return findNodeKW(root->left, word);
    }
    else{
        return findNodeKW(root->right, word);
    }
}
void clearTreeKW(KeyWordTree *root){
    if (root == NULL){
        return;
    }
    clearTreeKW(root->left);
    clearTreeKW(root->right);
    free(root->word);
    free(root);
}

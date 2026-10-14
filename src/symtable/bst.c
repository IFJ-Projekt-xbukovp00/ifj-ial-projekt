/**
 * @file bst.c
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

#include "bst.h"
#include "token.h"
#include "error.h"
/** 
 * @brief funkcia na pridanie do BVS
 * @param root je koreň/list, na ktorom sa nachádzam
 * @param name je hodnota, ktorú pridávam
 * @param type je typ hodnoty
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie
 * @returns updatnutý strom
*/
Node *insertNode(Node *root, const char *name, TokenType type, ErrorCode *error){
    if (root == NULL){
        Node *node = malloc(sizeof(Node));
        root->name = malloc(strlen(name)+1);
        if (root->name == NULL){
            *error = ERR_INTERNAL;
            return NULL;
        }
        strcpy(root->name, name);
        root->type = type;
        root->left = NULL;
        root->right = NULL;
        return root;
    }
    int cmp = strcmp(name, root->name);
    if (cmp < 0){
        root->left = insertNode(root->left, name, type, error);
    }
    else if (cmp > 0){
        root->right = insertNode(root->right, name, type, error);
    }
    return root;
}
/**
 * @brief funkcia na hľadanie Node v BVS
 * @param root je koreň/list, na ktorom sa nachádzam
 * @param name je hodnota, ktorej list hľadám
 * @param error je pointer, cez ktorý si vrátim chybovú hlášku
 * @returns list s hodnotou
 */
Node *findNode(Node *root, const char *name, ErrorCode *error){
    if (root == NULL){
        return NULL;
    }
    int cmp = strcmp(name, root->name);
    if (cmp == 0){
        return root;
    }
    if (cmp < 0){
        return findNode(root->left, name, error);
    }
    else if (cmp > 0){
        return findNode(root->right, name, error);
    }
}
 
/**
 * @brief funkcia na uvoľnenie stromu z pamäte
 * @param root je koreň/list, na ktorom sa nachádzam
 * @returns ..
 */
void destroyTree(Node *root){
    if (root == NULL){
        return; 
    }
    destroyTree(root->left);
    destroyTree(root->right);
    free(root->name);
    free(root);
}
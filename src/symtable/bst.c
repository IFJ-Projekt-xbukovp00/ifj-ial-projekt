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

#include <stdlib.h>
#include <string.h>

/**
 * @brief výška podstromu (prázdny = 0)
 * @param node je uzol, ktorého výšku potrebujem
 * @returns výška uzla
 */
int height(Node *node){
    if (node == NULL){
        return 0;
    }
    return node->height;
}

/**
 * @brief prepočíta výšku uzla z výšok jeho detí
 * @param node je uzol, ktorého výšku aktualizujem
 * @returns ..
 */
void updateHeight(Node *node){
    int leftHeight = height(node->left);
    int rightHeight = height(node->right);
    if (leftHeight > rightHeight){
        node->height = 1 + leftHeight;
    }
    else {
        node->height = 1 + rightHeight;
    }
}

/**
 * @brief pravá rotácia okolo root
 * @param root je koreň, cez ktorý robím rotáciu
 * @returns nový koreň podstromu
 */
Node *rotateRight(Node *root){
    Node *newRoot = root->left;
    root->left = newRoot->right;
    newRoot->right = root;
    updateHeight(root);
    updateHeight(newRoot);
    return newRoot;
}

/**
 * @brief ľavá rotácia okolo root
 * @param root je koreň, cez ktorý robím rotáciu
 * @returns nový koreň podstromu
 */
Node *rotateLeft(Node *root){
    Node *newRoot = root->right;
    root->right = newRoot->left;
    newRoot->left = root;
    updateHeight(root);
    updateHeight(newRoot);
    return newRoot;
}

/**
 * @brief obnoví vyváženie uzla po vložení (prípady LL, LR, RR, RL)
 * @param root je koreň, ktorý balansujem
 * @returns nový koreň podstromu
 */
Node *balance(Node *root){
    updateHeight(root);
    int balanceFactor = height(root->left) - height(root->right);
    if (balanceFactor > 1){
        if (height(root->left->left) < height(root->left->right)){
            root->left = rotateLeft(root->left); // LR
        }
        return rotateRight(root);
    }
    if (balanceFactor < -1){
        if (height(root->right->right) < height(root->right->left)){
            root->right = rotateRight(root->right); // RL
        }
        return rotateLeft(root);
    }
    return root;
}
/**
 * @brief funkcia na alokovanie uzla AVL stromu
 * @param name je hodnota 
 * @param symbol je symbol 
 * @param error je pointer, cez ktorý si vrátim chybu (ERR_INTERNAL 
 *              ak zlyhá alokácia)
 * @returns nový uzol
 */
Node *alocNode(const char *name, struct Symbol *symbol, ErrorCode *error){
    Node *node = malloc(sizeof(Node));
    if (node == NULL){
        *error = ERR_INTERNAL;
        return NULL;
    }
    node->name = malloc(strlen(name)+1);
    if (node->name == NULL){
        free(node);
        *error = ERR_INTERNAL;
        return NULL;
    }
    strcpy(node->name, name);
    node->symbol = symbol;
    node->height = 1;
    node->left = NULL;
    node->right = NULL;
    return node;
}

/**
 * @brief funkcia na pridanie do BVS
 * @param root je koreň/list, na ktorom sa nachádzam
 * @param name je hodnota, ktorú pridávam
 * @param symbol je symbol priradený k menu
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie
 *        (ERR_REDEF ak meno v strome už je, ERR_INTERNAL pri chybe alokácie, no
 *        pri úspechu sa nemení)
 * @returns updatnutý strom
*/
Node *insertNode(Node *root, const char *name, struct Symbol *symbol, ErrorCode *error){
    if (root == NULL){
        Node *node = malloc(sizeof(Node));
        if (node == NULL){
            *error = ERR_INTERNAL;
            return NULL;
        }
        node->name = malloc(strlen(name)+1);
        if (node->name == NULL){
            free(node);
            *error = ERR_INTERNAL;
            return NULL;
        }
        strcpy(node->name, name);
        node->symbol = symbol;
        node->height = 1;
        node->left = NULL;
        node->right = NULL;
        return node;
    }
    int cmp = strcmp(name, root->name);
    if (cmp < 0){
        root->left = insertNode(root->left, name, symbol, error);
    }
    else if (cmp > 0){
        root->right = insertNode(root->right, name, symbol, error);
    }
    else {
        *error = ERR_REDEF;
        return root;
    }
    return balance(root);
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
    return findNode(root->right, name, error);
}
 
/**
 * @brief Uvoľnenie stromu z pamäte
 * @param root je koreň/list, na ktorom sa nachádzam
 * @returns nič
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
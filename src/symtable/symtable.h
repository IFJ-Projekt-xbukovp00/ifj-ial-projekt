/**
 * @file symtable.h
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

#pragma once

#include "bst.h"

/**
 * @brief strom pre identifikátory
 */
typedef struct IDs{
    Token *Node;
    struct IDs *left;
    struct IDs *right;
}IDs;

/**
 * @brief funkcia na vytvorenie listu do stromu identifikátorov
 * @param Node je štruktúra, ktorá má v sebe informáciu o tokene
 * @returns list
 */
IDs *initNodeID(Token *Node);

/**
 * @brief funkcia na pridanie listu do stromu identifikátorov
 * @param root je miesto, kde sa má pridať
 * @param Node je štruktúra, ktorá má v sebe informáciu o tokene
 * @returns root
 */
IDs *addNodeID(IDs *root, Token *Node);

/**
 * @brief funkcia na hľadanie a kontrolovanie identikátora v strome
 * @param root je kontrolovaný list
 * @param Node je štruktúra, ktorá má v sebe informáciu o tokene
 * @returns pravdivostnú hodnotu
 */
bool findAndControlNodeID(IDs *root, Token *Node);

/**
 * @brief funkcia na mazanie stromu identifikátorov
 * @param root je list, ktorý odstraňujem
 * @returns ..
 */
void clearTreeID(IDs *root);

/**
 * @brief binárny strom pre keyWords
 */
typedef struct KeyWordTree{
    char *word;
    struct KeyWordTree *left;
    struct KeyWordTree *right;
}KeyWordTree;

/**
 * @brief funkcia na vytvorenie listu do stromu kľúčových slov
 * @param word je kľúčové slovo
 * @returns list
 */
KeyWordTree *initNodeKW(char *word);

/**
 * @brief funkcia na pridanie listu do stromu kľúčových slov
 * @param root je miesto, kde sa má pridať
 * @param word je kľúčové slovo
 * @returns root
 */
KeyWordTree *addNodeKW(KeyWordTree *root, char *word);

/** 
 * @brief funkcia na hľadanie kľúčového slova v strome
 * @param root je kontrolovaný list
 * @param word je kľúčové slovo, ktoré hľadám
 * @returns pravdivostnú hodnotu
*/
bool findNodeKW(KeyWordTree *root, char *word);

/**
 * @brief funkcia na mazanie stromu kľúčových slov
 * @param root je list, ktorý odstraňujem
 * @returns ..
 */
void clearTreeKW(KeyWordTree *root);



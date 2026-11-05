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


typedef struct Values{
    char *val_dataType;
    char *val_name;
    char *val_value;
    struct Values *left;
    struct Values *right;   
}Values;

/** 
 * @brief funkcie pre vytvorenie štruktúry
 * @returns ukazateľ na novo vytvorenú štruktúru
*/

Values *valCtor(void);

/** 
 * @brief funkcie pre pridávanie štruktúry na koniec
 * @param *tail ukazuje na list stromu, kde sa bude pridávať
*/

void *AddValue (Values *tail);

/** 
 *@brief odstraňuje list zo stromu
 *@param *cur ukazuje na list, ktorý sa odstraňuje
*/

void ValueDtor(Values *cur);

/**
 * @brief binárny strom pre keyWords
 */
typedef struct KeyWordTree{
    char *word;
    struct KeyWordTree *left;
    struct KeyWordTree *right   
}KeyWordTree;

/**
 * @brief funkcia na vytvorenie listu do stromu
 * @param word je kľúčové slovo
 * @returns list
 */
KeyWordTree *initNode(char *word);

/**
 * @brief funkcia na pridanie listu do stromu
 * @param root je miesto, kde sa má pridať
 * @param word je kľúčové slovo
 * @returns root
 */
KeyWordTree *addNode(KeyWordTree *root, char *word);

/** 
 * @brief funkcia na hľadanie kľúčového slova v strome
 * @param root je koreň, resp. kontrolovaný list
 * @param word je kľúčové slovo, ktoré hľadám
 * @returns pravdivostnú hodnotu
*/
bool findNode(KeyWordTree *root, char *word);

/**
 * @brief funkcia na mazanie stromu
 * @param root je list, ktorý odstraňujem
 * @returns ..
 */
void clearTree(KeyWordTree *root);



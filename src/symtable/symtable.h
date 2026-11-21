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

#include "ast.h"
#include "bst.h"
#include "types.h"

#include <stdbool.h>

/**
 * @brief druh symbolu
 */
typedef enum {
    SYM_VARIABLE,  // premenná (val / var)
    SYM_PARAMETER, // formálny parameter funkcie
    SYM_FUNCTION,  // používateľská funkcia
    SYM_BUILTIN,   // vstavaná funkcia
    SYM_CLASS,     // case class (ALGEBDT)
    SYM_ALIAS,     // alias typu (ALGEBDT)
    SYM_EXTENSION  // rozširujúca metóda (EXTKW)
} SymbolKind;

/**
 * @brief stav kontroly tela funkcie (pri aliase stav rozlisenia typu)
 */
typedef enum {
    FUNC_NOT_STARTED, // ešte nekontrolovaná
    FUNC_IN_PROGRESS, // práve sa kontroluje (rekurzia)
    FUNC_DONE         // hotová
} FunctionCheckState;

/**
 * @brief informácie o jednom mene v programe
 */
typedef struct Symbol {
    SymbolKind kind;
    Type *type;                    // typ symbolu, NULL = zatiaľ neodvodený
    bool isMutable;                // true pre var, inak false
    ASTNode *definition;           // uzol definície funkcie, inak NULL
    FunctionCheckState checkState; // stav kontroly tela
    Param *receiver;               // príjemca rozširujúcej metódy, inak NULL
    struct Symbol *nextAllocated;  // zoznam všetkých symbolov, aby sa dali na konci uvoľniť
    int cgId;                      // cislo premennej v generatore (meno x$N), 0 = nepridelene
} Symbol;

/**
 * @brief jeden rozsah platnosti (blok), rozsahy tvoria zásobník
 */
typedef struct Scope {
    Node *tree;           // AVL strom: meno -> symbol
    struct Scope *parent; // nadradený rozsah, NULL pre globálny
} Scope;

/**
 * @brief tabuľka symbolov
 */
typedef struct Symtable {
    Scope *top;          // aktuálny (najvnútornejší) rozsah
    Symbol *allSymbols;  // všetky vytvorené symboly
    Node *typeTree;      // mená typov (type, case class)
    Node *extensionTree; // rozširujúce metody, kľúč "Typ.meno"
} Symtable;

/**
 * @brief funkcia na inicializáciu prázdnej tabuľky (bez rozsahov)
 * @param table je inicializovaná tabuľka
 */
void symtableInit(Symtable *table);

/**
 * @brief funkcia na uvoľnenie všetkých rozsahov aj symbolov
 * @param table je uvoľňovaná tabuľka
 */
void symtableFree(Symtable *table);

/**
 * @brief funkcia na vnorenie do nového rozsahu (prvý vytvorený je globálny)
 * @param table je tabuľka symbolov
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (ERR_INTERNAL)
 */
void pushScope(Symtable *table, ErrorCode *error);

/**
 * @brief funkcia na opustenie aktuálneho rozsahu
 * @param table je tabuľka symbolov
 * Symboly z rozsahu sa neuvoľnia, môže na ne ukazovať AST (uvoľní ich symtableFree).
 */
void popScope(Symtable *table);

/**
 * @brief funkcia na definovanie mena v aktuálnom rozsahu
 * @param table je tabuľka symbolov
 * @param name je definované meno
 * @param kind je druh symbolu
 * @param type je typ symbolu (môže byť NULL)
 * @param isMutable je true pre var
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie
 *        (ERR_REDEF ak meno v aktuálnom rozsahu už je, ERR_INTERNAL pri chybe alokácie)
 * @returns nový symbol, pri chybe NULL
 */
Symbol *defineSymbol(Symtable *table, const char *name, SymbolKind kind, Type *type, bool isMutable,
                     ErrorCode *error);

/**
 * @brief funkcia na definovanie mena v zadanom strome (napr. typeTree)
 * @param table je tabuľka symbolov
 * @param tree je strom, do ktorého meno vkladám
 * @param name je definované meno
 * @param kind je druh symbolu
 * @param type je typ symbolu (moze byt NULL)
 * @param isMutable je true pre var
 * @param error je pointer, cez ktory si vrátim chybove hlasenie (ERR_REDEF, ERR_INTERNAL)
 * @returns nový symbol, pri chybe NULL
 */
Symbol *defineSymbolInTree(Symtable *table, Node **tree, const char *name, SymbolKind kind,
                           Type *type, bool isMutable, ErrorCode *error);

/**
 * @brief funkcia na hľadanie mena v jednom strome
 * @param tree je prehľadávaný strom
 * @param name je hladane meno
 * @returns symbol, alebo NULL ak tam meno nie je
 */
Symbol *lookupSymbolInTree(Node *tree, const char *name);

/**
 * @brief funkcia na hľadanie mena od aktuálneho rozsahu po globálny
 * @param table je tabuľka symbolov
 * @param name je hľadané meno
 * @returns najbližší symbol s daným menom, alebo NULL ak meno neexistuje
 */
Symbol *lookupSymbol(Symtable *table, const char *name);

/**
 * @file semantic.h
 * @brief Sémantická analýza (typové kontroly, rozsahy platnosti)
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

#include "symtable.h"

/**
 * @brief funkcia na vloženie vstavaných funkcií do aktuálneho (globálneho) rozsahu
 *
 * Vloží vstavané funkcie zo základného zadania aj z rozšírení BOOLTHEN
 * (readBoolean), ALGEBDT (readIntMaybe, readDoubleMaybe) a LISTS (List.range).
 * Typ každej funkcie je TY_FUNC s typmi parametrov a návratovým typom.
 * Členy hodnôt (VALMEM, metódy zoznamov) tu nie sú, nie sú to mená v rozsahu.
 *
 * @param table je tabuľka symbolov, musí mať aspoň jeden rozsah
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie
 *        (pred volaním musí byť ERR_OK, pri úspechu sa nemení)
 */
void addBuiltins(Symtable *table, ErrorCode *error);

/**
 * @brief funkcia na vytvorenie typu funkcie z jej definície
 * @param function je definícia funkcie
 * @returns typ TY_FUNC, parametre všetkých zoznamov sú za sebou
 */
Type *createFunctionType(DefFuncNode *function);

/**
 * @brief funkcia na kontrolu, či sa neopakujú mená parametrov
 * @param table je tabuľka symbolov
 * @param function je definícia funkcie
 * @param receiver je prijemca rozširujúcej metódy, inak NULL
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (4)
 * @returns true, ak má funkcia aspoň jeden parameter
 */
bool checkParamNames(Symtable *table, DefFuncNode *function, Param *receiver, ErrorCode *error);

/**
 * @brief funkcia na vloženie funkcie do tabuľky a kontrolu parametrov a @main
 * @param table je tabuľka symbolov
 * @param function je definícia funkcie
 * @param mainCount je počet doteraz nájdených @main
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (4, 5, 6)
 */
void collectFunction(Symtable *table, DefFuncNode *function, int *mainCount, ErrorCode *error);

/**
 * @brief funkcia na vloženie case class do tabulky typov aj ako konštruktor
 * @param table je tabuľka symbolov
 * @param classDef je definícia triedy
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (4)
 */
void collectClass(Symtable *table, ClassDefNode *classDef, ErrorCode *error);

/**
 * @brief funkcia na vloženie aliasu typu do tabuľky typov
 * @param table je tabulka symbolov
 * @param alias je definícia aliasu
 * @param error je pointer, cez ktory si vrátim chybové hlasenie (4)
 */
void collectTypeAlias(Symtable *table, TypeAliasNode *alias, ErrorCode *error);

/**
 * @brief funkcia na prvý prechod, pozbiera globálne definicie
 * @param table je tabuľka symbolov
 * @param program je koreň AST
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (3 ak chýba @main)
 */
void collectGlobalDefinitions(Symtable *table, ProgramNode *program, ErrorCode *error);

/**
 * @brief funkcia na preskocenie mien typov (TY_NAMED) az po skutočny typ
 * @param type je typ, môze byť NULL
 * @returns skutocný typ, alebo type ak ešte nie je rozliseny
 */
Type *getRealType(Type *type);

/**
 * @brief funkcia na rozlisenie aliasu typu
 * @param table je tabuľka symbolov
 * @param aliasSymbol je symbol aliasu
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (3, 6 ak je alias cyklický)
 */
void resolveAlias(Symtable *table, Symbol *aliasSymbol, ErrorCode *error);

/**
 * @brief funkcia na rozlíšenie vsetkých mien typov (TY_NAMED) vnutri typu
 * @param table je tabuľka symbolov
 * @param type je typ, môže byť NULL
 * @param error je pointer, cez ktory si vratim chybové hlásenie (3 ak typ neexistuje)
 */
void resolveType(Symtable *table, Type *type, ErrorCode *error);

/**
 * @brief funkcia na rozlísenie typov v zozname parametrov
 * @param table je tabuľka symbolov
 * @param paramList je zoznam parametrov
 * @param error je pointer, cez ktory si vrátim chybové hlasenie
 */
void resolveParamList(Symtable *table, ParamList *paramList, ErrorCode *error);

/**
 * @brief funkcia na rozlisenie typov parametrov a navratového typu funkcie
 * @param table je tabuľka symbolov
 * @param function je definícia funkcie
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie
 */
void resolveFunctionTypes(Symtable *table, DefFuncNode *function, ErrorCode *error);

/**
 * @brief funkcia na druhý prechod, rozlíši typy v globálnych definíciách
 * @param table je tabuľka symbolov
 * @param program je koreň AST
 * @param error je pointer, cez ktory si vrátim chybove hlásenie
 */
void resolveGlobalTypes(Symtable *table, ProgramNode *program, ErrorCode *error);

/**
 * @brief funkcia na zistenie mena typu pre kluc rozširujúcej metódy
 * @param type je typ príjemcu
 * @returns meno typu (Int, String, meno triedy, ...)
 */
const char *getTypeKeyName(Type *type);

/**
 * @brief funkcia na vlozenie rozširujúcej metódy pod klúčom "Typ.meno"
 * @param table je tabuľka symbolov
 * @param receiver je prijemca
 * @param method je definícia metódy
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (4)
 */
void collectExtensionMethod(Symtable *table, Param *receiver, DefFuncNode *method,
                            ErrorCode *error);

/**
 * @brief funkcia na vlozenie všetkých rozširujúcich metod (až po rozliseni typov)
 * @param table je tabulka symbolov
 * @param program je koreň AST
 * @param error je pointer, cez ktory si vrátim chybové hlasenie
 */
void collectExtensions(Symtable *table, ProgramNode *program, ErrorCode *error);

/**
 * @brief funkcia na kontrolu, že @main s aliasom vracia Unit
 * @param program je koreň AST
 * @param error je pointer, cez ktory si vrátim chybove hlásenie (6)
 */
void checkMainReturnType(ProgramNode *program, ErrorCode *error);

/**
 * @brief funkcia na sémantickú analýzu programu
 * @param program je koreň AST
 * @param table je prázdna tabuľka symbolov (init aj free robí volajúci)
 * @param error je pointer, cez ktorý si vrátim prvú chybu
 */
void semanticAnalyze(ASTNode *program, Symtable *table, ErrorCode *error);

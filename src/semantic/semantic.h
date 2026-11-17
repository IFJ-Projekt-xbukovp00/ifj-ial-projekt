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
 * @brief funkcia na vloženie funkcie do tabuľky a kontrolu parametrov a @main
 * @param table je tabuľka symbolov
 * @param function je definícia funkcie
 * @param mainCount je počet doteraz nájdených @main
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (4, 5, 6)
 */
void collectFunction(Symtable *table, DefFuncNode *function, int *mainCount, ErrorCode *error);

/**
 * @brief funkcia na prvý prechod, pozbiera globálne definície (zatiaľ len funkcie)
 * @param table je tabuľka symbolov
 * @param program je koreň AST
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (3 ak chýba @main)
 */
void collectGlobalDefinitions(Symtable *table, ProgramNode *program, ErrorCode *error);

/**
 * @brief funkcia na sémantickú analýzu programu
 * @param program je koreň AST
 * @param table je prázdna tabuľka symbolov (init aj free robí volajúci)
 * @param error je pointer, cez ktorý si vrátim prvú chybu
 */
void semanticAnalyze(ASTNode *program, Symtable *table, ErrorCode *error);

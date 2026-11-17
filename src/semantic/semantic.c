/**
 * @file semantic.c
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

#include "semantic.h"

#include <stdarg.h>

/**
 * @brief funkcia na vloženie jednej vstavanej funkcie
 * @param table je tabuľka symbolov
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie
 * @param name je meno funkcie
 * @param returnType je návratový typ
 * @param paramCount je počet parametrov, za ním nasledujú ich typy (Type *)
 */
void addBuiltin(Symtable *table, ErrorCode *error, const char *name, Type *returnType,
                       int paramCount, ...){
    if (*error != ERR_OK){
        return;
    }

    TypeList params = {NULL, 0, 0};
    va_list paramTypes;
    
    va_start(paramTypes, paramCount);

    for (int paramIndex = 0; paramIndex < paramCount; paramIndex++){
        type_list_append(&params, va_arg(paramTypes, Type *));
    }

    va_end(paramTypes);
    defineSymbol(table, name, SYM_BUILTIN, type_create_function(params, returnType), false, error);
}

void addBuiltins(Symtable *table, ErrorCode *error){
    Type *intType = type_get_basic(TY_INT);
    Type *doubleType = type_get_basic(TY_DOUBLE);
    Type *stringType = type_get_basic(TY_STRING);
    Type *unitType = type_get_basic(TY_UNIT);
    Type *boolType = type_get_basic(TY_BOOL);
    Type *anyType = type_get_basic(TY_ANY);

    // základné zadanie
    addBuiltin(table, error, "print", unitType, 1, anyType);
    addBuiltin(table, error, "readLine", stringType, 0);
    addBuiltin(table, error, "readInt", intType, 0);
    addBuiltin(table, error, "readDouble", doubleType, 0);
    addBuiltin(table, error, "toStr", stringType, 1, anyType);
    addBuiltin(table, error, "toInt", intType, 1, doubleType);
    addBuiltin(table, error, "strLength", intType, 1, stringType);
    addBuiltin(table, error, "subStr", stringType, 3, stringType, intType, intType);
    addBuiltin(table, error, "ord", intType, 2, stringType, intType);
    addBuiltin(table, error, "chr", stringType, 1, intType);

    // BOOLTHEN
    addBuiltin(table, error, "readBoolean", boolType, 0);

    // ALGEBDT: pri chybnom vstupe vracajú ()
    TypeList intOrUnit = {NULL, 0, 0};
    type_list_append(&intOrUnit, intType);
    type_list_append(&intOrUnit, unitType);
    addBuiltin(table, error, "readIntMaybe", type_create_union(intOrUnit), 0);

    TypeList doubleOrUnit = {NULL, 0, 0};
    type_list_append(&doubleOrUnit, doubleType);
    type_list_append(&doubleOrUnit, unitType);
    addBuiltin(table, error, "readDoubleMaybe", type_create_union(doubleOrUnit), 0);

    // LISTS: List.range(start, end) aj List.range(start, end, step), tretí parameter
    // je nepovinný – kontrola volania musí povoliť 2 aj 3 argumenty
    addBuiltin(table, error, "List.range", type_create_list(intType), 3, intType, intType,
               intType);
}

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

void collectFunction(Symtable *table, DefFuncNode *function, int *mainCount, ErrorCode *error){
    if (*error != ERR_OK){
        return;
    }

    // parametre zo všetkých zoznamov dáme za sebou
    TypeList paramTypes = {NULL, 0, 0};

    for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++){
        ParamList *paramList = &function->paramList[listIndex];

        for (size_t paramIndex = 0; paramIndex < paramList->params_count; paramIndex++){
            type_list_append(&paramTypes, paramList->params[paramIndex].type);
        }
    }

    Type *functionType = type_create_function(paramTypes, function->returnType);
    Symbol *functionSymbol = defineSymbol(table, function->name, SYM_FUNCTION, functionType,
                                          false, error);
    if (functionSymbol == NULL){
        return;
    }

    functionSymbol->definition = (ASTNode *)function;
    function->symbol = functionSymbol;

    // rovnaké meno parametra aj v inom zozname je chyba 4
    pushScope(table, error);
    if (*error != ERR_OK){
        return;
    }

    bool hasParameter = false;

    for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++){
        ParamList *paramList = &function->paramList[listIndex];

        for (size_t paramIndex = 0; paramIndex < paramList->params_count; paramIndex++){
            Param *param = &paramList->params[paramIndex];
            hasParameter = true;

            if (defineSymbol(table, param->name, SYM_PARAMETER, param->type, false, error) == NULL){
                popScope(table);
                return;
            }
        }
    }

    popScope(table);

    if (!function->isMain){
        return;
    }

    (*mainCount)++;

    if (*mainCount > 1){
        *error = ERR_REDEF;
        return;
    }

    if (hasParameter){
        *error = ERR_PARAMS;
        return;
    }

    // neuvedený typ a alias skontrolujeme až neskôr
    if (function->returnType != NULL && function->returnType->kind != TY_UNIT
        && function->returnType->kind != TY_NAMED){
        *error = ERR_TYPE;
    }
}

void collectGlobalDefinitions(Symtable *table, ProgramNode *program, ErrorCode *error){
    int mainCount = 0;

    for (size_t itemIndex = 0; itemIndex < program->itemsCount; itemIndex++){
        if (*error != ERR_OK){
            return;
        }

        ASTNode *item = program->items[itemIndex];

        if (item->kind == AST_FUNCTION_DEF){
            collectFunction(table, (DefFuncNode *)item, &mainCount, error);
        }
    }

    if (*error == ERR_OK && mainCount == 0){
        *error = ERR_UNDEF;
    }
}

void semanticAnalyze(ASTNode *program, Symtable *table, ErrorCode *error){
    if (program == NULL || program->kind != AST_PROGRAM){
        *error = ERR_INTERNAL;
        return;
    }

    // vstavané funkcie majú vlastný rozsah, aby sa dali prekryť
    pushScope(table, error);
    addBuiltins(table, error);

    // globálny rozsah
    if (*error == ERR_OK){
        pushScope(table, error);
    }

    if (*error != ERR_OK){
        return;
    }

    collectGlobalDefinitions(table, (ProgramNode *)program, error);
}

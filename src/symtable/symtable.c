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
#include <stdlib.h>
#include <string.h>

void symtableInit(Symtable *table){
    table->top = NULL;
    table->allSymbols = NULL;
}

void symtableFree(Symtable *table){
    while (table->top != NULL){
        popScope(table);
    }

    Symbol *symbol = table->allSymbols;

    while (symbol != NULL){
        Symbol *nextSymbol = symbol->nextAllocated;
        free(symbol);
        symbol = nextSymbol;
    }

    table->allSymbols = NULL;
}

void pushScope(Symtable *table, ErrorCode *error){
    Scope *newScope = malloc(sizeof(Scope));

    if (newScope == NULL){
        *error = ERR_INTERNAL;
        return;
    }

    newScope->tree = NULL;
    newScope->parent = table->top;
    table->top = newScope;
}

void popScope(Symtable *table){

    Scope *oldScope = table->top;
    if (oldScope == NULL){
        return;
    }

    table->top = oldScope->parent;
    destroyTree(oldScope->tree);
    free(oldScope);
}

Symbol *defineSymbol(Symtable *table, const char *name, SymbolKind kind, Type *type,
                     bool isMutable, ErrorCode *error){
    if (table->top == NULL){
        *error = ERR_INTERNAL;
        return NULL;
    }

    Symbol *symbol = malloc(sizeof(Symbol));
    if (symbol == NULL){
        *error = ERR_INTERNAL;
        return NULL;
    }

    symbol->kind = kind;
    symbol->type = type;
    symbol->isMutable = isMutable;
    symbol->definition = NULL;
    symbol->checkState = FUNC_NOT_STARTED;

    ErrorCode insertError = ERR_OK;

    table->top->tree = insertNode(table->top->tree, name, symbol, &insertError);

    if (insertError != ERR_OK){
        free(symbol);
        *error = insertError;
        return NULL;
    }

    symbol->nextAllocated = table->allSymbols;
    table->allSymbols = symbol;
    return symbol;
}

Symbol *lookupSymbol(Symtable *table, const char *name){
    ErrorCode findError = ERR_OK;

    for (Scope *scope = table->top; scope != NULL; scope = scope->parent){
        Node *foundNode = findNode(scope->tree, name, &findError);

        if (foundNode != NULL){
            return foundNode->symbol;
        }
    }
    
    return NULL;
}

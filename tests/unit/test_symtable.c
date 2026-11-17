/**
 * @file test_symtable.c
 * @brief Unit testy tabuľky symbolov (src/symtable/symtable.c)
 *
 * Kontroluje definíciu a hľadanie mien, redefiníciu v rovnakom rozsahu,
 * prekrytie mena vo vnorenom rozsahu a návrat po opustení rozsahu.
 *
 * Použitie AI: tento súbor vygenerovala AI (Claude Opus 5.5).
 * Prompt: "Vytvor nám prosím unit testy na testovanie symtable pre jazyk ifj26, zadanie nájdeš
 *   v .zadanie, po spravnosti mame vsetky exntensiony"
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

#include <stdio.h>

int failureCount = 0;

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "%s:%d: CHECK zlyhal: %s\n", __FILE__, __LINE__, #condition);          \
            failureCount++;                                                                        \
        }                                                                                          \
    } while (0)

/* Definuje meno a overí, že nenastala chyba. */
Symbol *defineChecked(Symtable *table, const char *name, SymbolKind kind, bool isMutable) {
    ErrorCode error = ERR_OK;
    Symbol *symbol = defineSymbol(table, name, kind, NULL, isMutable, &error);
    CHECK(error == ERR_OK);
    CHECK(symbol != NULL);
    return symbol;
}

void testEmpty(void) {
    Symtable table;
    symtableInit(&table);
    CHECK(lookupSymbol(&table, "x") == NULL);

    ErrorCode error = ERR_OK;
    CHECK(defineSymbol(&table, "x", SYM_VARIABLE, NULL, false, &error) == NULL);
    CHECK(error == ERR_INTERNAL);

    popScope(&table);
    symtableFree(&table);
}

void testDefineLookup(void) {
    Symtable table;
    symtableInit(&table);
    ErrorCode error = ERR_OK;
    pushScope(&table, &error);
    CHECK(error == ERR_OK);

    Symbol *variable = defineChecked(&table, "x", SYM_VARIABLE, true);
    Symbol *function = defineChecked(&table, "main", SYM_FUNCTION, false);
    CHECK(lookupSymbol(&table, "x") == variable);
    CHECK(lookupSymbol(&table, "main") == function);
    CHECK(lookupSymbol(&table, "y") == NULL);
    CHECK(variable->kind == SYM_VARIABLE && variable->isMutable);
    CHECK(function->kind == SYM_FUNCTION && !function->isMutable);
    CHECK(variable->type == NULL);

    symtableFree(&table);
    CHECK(table.top == NULL && table.allSymbols == NULL);
}

void testRedefinition(void) {
    Symtable table;
    symtableInit(&table);
    ErrorCode error = ERR_OK;
    pushScope(&table, &error);

    Symbol *firstSymbol = defineChecked(&table, "x", SYM_VARIABLE, false);
    error = ERR_OK;
    CHECK(defineSymbol(&table, "x", SYM_VARIABLE, NULL, true, &error) == NULL);
    CHECK(error == ERR_REDEF);
    CHECK(lookupSymbol(&table, "x") == firstSymbol);

    symtableFree(&table);
}

void testShadowingAndPop(void) {
    Symtable table;
    symtableInit(&table);
    ErrorCode error = ERR_OK;
    pushScope(&table, &error);
    Symbol *outerSymbol = defineChecked(&table, "x", SYM_VARIABLE, false);
    Symbol *globalOnly = defineChecked(&table, "g", SYM_FUNCTION, false);

    pushScope(&table, &error);
    CHECK(error == ERR_OK);
    CHECK(lookupSymbol(&table, "x") == outerSymbol);
    Symbol *innerSymbol = defineChecked(&table, "x", SYM_VARIABLE, true);
    Symbol *innerOnly = defineChecked(&table, "y", SYM_VARIABLE, false);
    CHECK(innerSymbol != outerSymbol);
    CHECK(lookupSymbol(&table, "x") == innerSymbol);
    CHECK(lookupSymbol(&table, "y") == innerOnly);
    CHECK(lookupSymbol(&table, "g") == globalOnly);

    popScope(&table);
    CHECK(lookupSymbol(&table, "x") == outerSymbol);
    CHECK(lookupSymbol(&table, "y") == NULL);
    /* symbol z opusteného rozsahu ostáva platný (môže naň ukazovať AST) */
    CHECK(innerOnly->kind == SYM_VARIABLE);

    symtableFree(&table);
}

int main(void) {
    testEmpty();
    testDefineLookup();
    testRedefinition();
    testShadowingAndPop();

    if (failureCount) {
        fprintf(stderr, "test_symtable: %d chyb\n", failureCount);
        return 1;
    }
    printf("test_symtable: OK\n");
    return 0;
}

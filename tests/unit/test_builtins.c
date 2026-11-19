/**
 * @file test_builtins.c
 * @brief Unit testy vstavaných funkcií (addBuiltins v src/semantic/semantic.c)
 *
 * Kontroluje, že sú v globálnom rozsahu všetky vstavané funkcie zo zadania
 * (aj z rozšírení BOOLTHEN, ALGEBDT a LISTS) so správnymi typmi parametrov
 * a návratovým typom, a že ich nemožno znova definovať.
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

#include <stdio.h>

int failureCount = 0;

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "%s:%d: CHECK zlyhal: %s\n", __FILE__, __LINE__, #condition);          \
            failureCount++;                                                                        \
        }                                                                                          \
    } while (0)

/**
 * Overí vstavanú funkciu: druh symbolu, počet a druhy parametrov a druh návratového typu.
 * Druhy parametrov sú v poli paramKinds, ktoré má paramCount prvkov.
 */
Symbol *checkBuiltin(Symtable *table, const char *name, TypeKind returnKind,
                            size_t paramCount, const TypeKind *paramKinds) {
    Symbol *symbol = lookupSymbol(table, name);
    CHECK(symbol != NULL);
    if (symbol == NULL) {
        fprintf(stderr, "  chyba vstavana funkcia %s\n", name);
        return NULL;
    }
    CHECK(symbol->kind == SYM_BUILTIN);
    CHECK(!symbol->isMutable);
    CHECK(symbol->type != NULL && symbol->type->kind == TY_FUNC);
    if (symbol->type == NULL || symbol->type->kind != TY_FUNC) {
        return symbol;
    }
    CHECK(symbol->type->func.ret != NULL && symbol->type->func.ret->kind == returnKind);
    CHECK(symbol->type->func.params.len == paramCount);
    if (symbol->type->func.params.len == paramCount) {
        for (size_t paramIndex = 0; paramIndex < paramCount; paramIndex++) {
            CHECK(symbol->type->func.params.items[paramIndex]->kind == paramKinds[paramIndex]);
        }
    }
    return symbol;
}

/* Overí, že typ je zjednotenie práve dvoch zadaných druhov. */
void checkUnionOfTwo(const Type *type, TypeKind firstKind, TypeKind secondKind) {
    CHECK(type != NULL && type->kind == TY_UNION);
    if (type == NULL || type->kind != TY_UNION) {
        return;
    }
    CHECK(type->members.len == 2);
    if (type->members.len == 2) {
        CHECK(type->members.items[0]->kind == firstKind);
        CHECK(type->members.items[1]->kind == secondKind);
    }
}

void testBaseBuiltins(Symtable *table) {
    const TypeKind anyParam[] = {TY_ANY};
    const TypeKind doubleParam[] = {TY_DOUBLE};
    const TypeKind stringParam[] = {TY_STRING};
    const TypeKind subStrParams[] = {TY_STRING, TY_INT, TY_INT};
    const TypeKind ordParams[] = {TY_STRING, TY_INT};
    const TypeKind intParam[] = {TY_INT};

    checkBuiltin(table, "print", TY_UNIT, 1, anyParam);
    checkBuiltin(table, "readLine", TY_STRING, 0, NULL);
    checkBuiltin(table, "readInt", TY_INT, 0, NULL);
    checkBuiltin(table, "readDouble", TY_DOUBLE, 0, NULL);
    checkBuiltin(table, "toStr", TY_STRING, 1, anyParam);
    checkBuiltin(table, "toInt", TY_INT, 1, doubleParam);
    checkBuiltin(table, "strLength", TY_INT, 1, stringParam);
    checkBuiltin(table, "subStr", TY_STRING, 3, subStrParams);
    checkBuiltin(table, "ord", TY_INT, 2, ordParams);
    checkBuiltin(table, "chr", TY_STRING, 1, intParam);
}

void testExtensionBuiltins(Symtable *table) {
    checkBuiltin(table, "readBoolean", TY_BOOL, 0, NULL);

    Symbol *readIntMaybe = checkBuiltin(table, "readIntMaybe", TY_UNION, 0, NULL);
    if (readIntMaybe != NULL && readIntMaybe->type->kind == TY_FUNC) {
        checkUnionOfTwo(readIntMaybe->type->func.ret, TY_INT, TY_UNIT);
    }
    Symbol *readDoubleMaybe = checkBuiltin(table, "readDoubleMaybe", TY_UNION, 0, NULL);
    if (readDoubleMaybe != NULL && readDoubleMaybe->type->kind == TY_FUNC) {
        checkUnionOfTwo(readDoubleMaybe->type->func.ret, TY_DOUBLE, TY_UNIT);
    }

    const TypeKind rangeParams[] = {TY_INT, TY_INT, TY_INT};
    Symbol *listRange = checkBuiltin(table, "List.range", TY_LIST, 3, rangeParams);
    if (listRange != NULL && listRange->type->kind == TY_FUNC) {
        const Type *listType = listRange->type->func.ret;
        CHECK(listType->elem != NULL && listType->elem->kind == TY_INT);
    }
}

void testNotBuiltins(Symtable *table) {
    /* členy hodnôt a metódy zoznamov nie sú mená v rozsahu */
    CHECK(lookupSymbol(table, "length") == NULL);
    CHECK(lookupSymbol(table, "head") == NULL);
    CHECK(lookupSymbol(table, "main") == NULL);
}

void testRedefinition(Symtable *table) {
    ErrorCode error = ERR_OK;
    CHECK(defineSymbol(table, "print", SYM_FUNCTION, NULL, false, &error) == NULL);
    CHECK(error == ERR_REDEF);
    CHECK(lookupSymbol(table, "print")->kind == SYM_BUILTIN);

    /* druhé vloženie vstavaných funkcií do toho istého rozsahu je redefinícia */
    error = ERR_OK;
    addBuiltins(table, &error);
    CHECK(error == ERR_REDEF);
}

void testShadowingInNestedScope(Symtable *table) {
    /* lokálna premenná vo vnorenom rozsahu vstavanú funkciu prekryje */
    ErrorCode error = ERR_OK;
    pushScope(table, &error);
    Symbol *local = defineSymbol(table, "print", SYM_VARIABLE, NULL, false, &error);
    CHECK(error == ERR_OK);
    CHECK(lookupSymbol(table, "print") == local);
    popScope(table);
    CHECK(lookupSymbol(table, "print")->kind == SYM_BUILTIN);
}

int main(void) {
    Symtable table;
    symtableInit(&table);
    ErrorCode error = ERR_OK;
    pushScope(&table, &error);
    addBuiltins(&table, &error);
    CHECK(error == ERR_OK);

    testBaseBuiltins(&table);
    testExtensionBuiltins(&table);
    testNotBuiltins(&table);
    testRedefinition(&table);
    testShadowingInNestedScope(&table);

    symtableFree(&table);
    type_free_all();

    if (failureCount) {
        fprintf(stderr, "test_builtins: %d chyb\n", failureCount);
        return 1;
    }
    printf("test_builtins: OK\n");
    return 0;
}

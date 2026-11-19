/**
 * @file test_types.c
 * @brief Unit testy porovnávania typov (type_equals, type_accepts, type_print)
 *
 * Použitie AI: tento súbor vygenerovala AI (Claude Opus 5.5).
 * Prompt: "Vyrob testy ako pre zvyšok semntickeho analyzatora, tentokrát ale na ich telá,
 *   podľa zadania .zadanie, robíme so všetkými rožšíreniami."
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

#include "types.h"

#include <stdio.h>
#include <string.h>

int failureCount = 0;

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "%s:%d: CHECK zlyhal: %s\n", __FILE__, __LINE__, #condition);          \
            failureCount++;                                                                        \
        }                                                                                          \
    } while (0)

/* pomocné funkcie */

Type *basic(TypeKind kind) {
    return type_get_basic(kind);
}

Type *unionOfTwo(Type *first, Type *second) {
    TypeList members = {0};
    type_list_append(&members, first);
    type_list_append(&members, second);
    return type_create_union(members);
}

Type *functionOfOne(Type *param, Type *ret) {
    TypeList params = {0};
    type_list_append(&params, param);
    return type_create_function(params, ret);
}

Type *namedResolvedTo(const char *name, Type *resolved) {
    Type *named = type_create_named(name);
    named->named.resolved = resolved;
    return named;
}

/**
 * @brief funkcia na vypísanie typu do reťazca cez dočasný súbor
 * @param type je vypisovaný typ
 * @param buffer je reťazec, do ktorého sa výpis uloží
 * @param size je veľkosť reťazca
 */
void printToBuffer(const Type *type, char *buffer, size_t size) {
    FILE *file = tmpfile();
    buffer[0] = '\0';

    if (file == NULL) {
        return;
    }

    type_print(type, file);
    rewind(file);

    size_t length = fread(buffer, 1, size - 1, file);
    buffer[length] = '\0';
    fclose(file);
}

/* testy */

void testEquals(void) {
    CHECK(type_equals(basic(TY_INT), basic(TY_INT)));
    CHECK(!type_equals(basic(TY_INT), basic(TY_DOUBLE)));
    CHECK(!type_equals(basic(TY_INT), NULL));

    // dva rôzne ukazovatele, rovnaký obsah
    CHECK(type_equals(type_create_list(basic(TY_INT)), type_create_list(basic(TY_INT))));
    CHECK(!type_equals(type_create_list(basic(TY_INT)), type_create_list(basic(TY_STRING))));

    // meno typu sa porovnáva podľa toho, na čo ukazuje
    CHECK(type_equals(namedResolvedTo("Number", basic(TY_INT)), basic(TY_INT)));
    CHECK(type_equals(basic(TY_INT), namedResolvedTo("Number", basic(TY_INT))));
    CHECK(!type_equals(namedResolvedTo("Number", basic(TY_INT)), basic(TY_STRING)));

    // funkcie po zložkách
    CHECK(type_equals(functionOfOne(basic(TY_INT), basic(TY_STRING)),
                      functionOfOne(basic(TY_INT), basic(TY_STRING))));
    CHECK(!type_equals(functionOfOne(basic(TY_INT), basic(TY_STRING)),
                       functionOfOne(basic(TY_DOUBLE), basic(TY_STRING))));
    CHECK(!type_equals(functionOfOne(basic(TY_INT), basic(TY_STRING)),
                       functionOfOne(basic(TY_INT), basic(TY_UNIT))));

    // zjednotenia
    CHECK(type_equals(unionOfTwo(basic(TY_INT), basic(TY_STRING)),
                      unionOfTwo(basic(TY_INT), basic(TY_STRING))));
    CHECK(!type_equals(unionOfTwo(basic(TY_INT), basic(TY_STRING)),
                       unionOfTwo(basic(TY_INT), basic(TY_UNIT))));
    CHECK(!type_equals(unionOfTwo(basic(TY_INT), basic(TY_STRING)), basic(TY_INT)));
}

void testAccepts(void) {
    // jediná implicitná konverzia Int -> Double
    CHECK(type_accepts(basic(TY_DOUBLE), basic(TY_INT)));
    CHECK(!type_accepts(basic(TY_INT), basic(TY_DOUBLE)));
    CHECK(!type_accepts(basic(TY_STRING), basic(TY_INT)));
    CHECK(type_accepts(basic(TY_STRING), basic(TY_STRING)));

    // parameter print a toStr
    CHECK(type_accepts(basic(TY_ANY), basic(TY_STRING)));
    CHECK(type_accepts(basic(TY_ANY), type_create_list(basic(TY_INT))));

    // zjednotenie prijme svoj člen (aj s konverziou, ok_union_accepts_int)
    CHECK(type_accepts(unionOfTwo(basic(TY_DOUBLE), basic(TY_STRING)), basic(TY_INT)));
    CHECK(type_accepts(unionOfTwo(basic(TY_INT), basic(TY_UNIT)), basic(TY_UNIT)));
    CHECK(!type_accepts(unionOfTwo(basic(TY_INT), basic(TY_UNIT)), basic(TY_STRING)));

    // zjednotenie ako hodnota: každý člen musí byť prijatý
    CHECK(!type_accepts(basic(TY_INT), unionOfTwo(basic(TY_INT), basic(TY_UNIT))));
    CHECK(type_accepts(unionOfTwo(basic(TY_INT), basic(TY_UNIT)),
                       unionOfTwo(basic(TY_UNIT), basic(TY_INT))));

    // zjednotenie ako hodnota sa neprevádza (ALGEBDT: foo(anything) je chyba)
    Type *scalar = unionOfTwo(unionOfTwo(basic(TY_INT), basic(TY_DOUBLE)), basic(TY_STRING));
    CHECK(!type_accepts(unionOfTwo(basic(TY_DOUBLE), basic(TY_STRING)), scalar));
    CHECK(type_accepts(scalar, unionOfTwo(basic(TY_INT), basic(TY_STRING))));
    CHECK(type_accepts(scalar, basic(TY_INT)));
    CHECK(type_accepts(basic(TY_ANY), scalar));
    CHECK(!type_accepts(basic(TY_DOUBLE), unionOfTwo(basic(TY_INT), basic(TY_DOUBLE))));

    // meno typu na oboch stranách
    CHECK(type_accepts(basic(TY_INT), namedResolvedTo("Number", basic(TY_INT))));
    CHECK(type_accepts(namedResolvedTo("Real", basic(TY_DOUBLE)), basic(TY_INT)));

    // prázdny zoznam List() prijme každý typ zoznamu
    CHECK(type_accepts(type_create_list(basic(TY_INT)), type_create_list(NULL)));
    CHECK(!type_accepts(basic(TY_INT), type_create_list(NULL)));

    // zoznamy sa neprevádzajú
    CHECK(!type_accepts(type_create_list(basic(TY_DOUBLE)), type_create_list(basic(TY_INT))));

    CHECK(!type_accepts(basic(TY_INT), NULL));
    CHECK(!type_accepts(NULL, basic(TY_INT)));
}

void testPrint(void) {
    char buffer[128];

    printToBuffer(basic(TY_INT), buffer, sizeof(buffer));
    CHECK(strcmp(buffer, "Int") == 0);

    printToBuffer(basic(TY_BOOL), buffer, sizeof(buffer));
    CHECK(strcmp(buffer, "Boolean") == 0);

    printToBuffer(type_create_list(basic(TY_INT)), buffer, sizeof(buffer));
    CHECK(strcmp(buffer, "List[Int]") == 0);

    printToBuffer(unionOfTwo(basic(TY_INT), basic(TY_UNIT)), buffer, sizeof(buffer));
    CHECK(strcmp(buffer, "Int | Unit") == 0);

    printToBuffer(functionOfOne(basic(TY_INT), basic(TY_BOOL)), buffer, sizeof(buffer));
    CHECK(strcmp(buffer, "(Int) => Boolean") == 0);

    printToBuffer(type_create_named("Point"), buffer, sizeof(buffer));
    CHECK(strcmp(buffer, "Point") == 0);
}

int main(void) {
    testEquals();
    testAccepts();
    testPrint();

    type_free_all();

    if (failureCount > 0) {
        fprintf(stderr, "test_types: %d chyb\n", failureCount);
        return 1;
    }

    printf("test_types: OK\n");
    return 0;
}

/**
 * @file types.c
 * @brief Typy jazyka IFJ26 (vytváranie a uvoľňovanie)
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
#include "error.h"

#include <stdlib.h>
#include <string.h>

Type basicTypes[TY_ANY + 1]; // jednoduché typy, nealokujú sa

Type **allTypes = NULL;      // všetky alokované typy, uvoľní ich type_free_all
size_t allTypesCount = 0;
size_t allTypesCapacity = 0;

/**
 * @brief funkcia na alokovanie nového typu a jeho zapamätanie pre type_free_all
 * @param kind je druh typu
 * @returns nový typ, NULL ak zlyhá alokácia
 */
Type *newType(TypeKind kind){
    if (allTypesCount == allTypesCapacity){
        size_t newCapacity = 16;

        if (allTypesCapacity > 0){
            newCapacity = allTypesCapacity * 2;
        }

        Type **newArray = realloc(allTypes, newCapacity * sizeof(Type *));

        if (newArray == NULL){
            error_exit(ERR_INTERNAL, "nedostatok pamate pre typy");
        }

        allTypes = newArray;
        allTypesCapacity = newCapacity;
    }

    Type *type = calloc(1, sizeof(Type));

    if (type == NULL){
        error_exit(ERR_INTERNAL, "nedostatok pamate pre typ");
    }

    type->kind = kind;
    allTypes[allTypesCount] = type;
    allTypesCount++;
    return type;
}

Type *type_get_basic(TypeKind kind){
    basicTypes[kind].kind = kind;
    return &basicTypes[kind];
}

Type *type_create_list(Type *elem){
    Type *type = newType(TY_LIST);
    type->elem = elem;
    return type;
}

Type *type_create_function(TypeList params, Type *ret){
    Type *type = newType(TY_FUNC);
    type->func.params = params;
    type->func.ret = ret;
    return type;
}

Type *type_create_union(TypeList members){
    Type *type = newType(TY_UNION);
    type->members = members;
    return type;
}

Type *type_create_named(const char *name){
    Type *type = newType(TY_NAMED);
    type->named.name = malloc(strlen(name) + 1);

    if (type->named.name == NULL){
        error_exit(ERR_INTERNAL, "nedostatok pamate pre meno typu");
    }

    strcpy(type->named.name, name);
    type->named.resolved = NULL;
    return type;
}

Type *type_create_class(struct Symbol *cls){
    Type *type = newType(TY_CLASS);
    type->cls = cls;
    return type;
}

void type_list_append(TypeList *list, Type *t){
    if (list->len == list->cap){
        size_t newCapacity = 4;
        if (list->cap > 0){
            newCapacity = list->cap * 2;
        }
        Type **newItems = realloc(list->items, newCapacity * sizeof(Type *));
        if (newItems == NULL){
            error_exit(ERR_INTERNAL, "nedostatok pamate pre zoznam typov");
        }
        list->items = newItems;
        list->cap = newCapacity;
    }
    
    list->items[list->len] = t;
    list->len++;
}

/**
 * @brief funkcia na preskocenie mien typov (TY_NAMED) až po skutocny typ
 * @param type je typ, ktory môže byt meno
 * @returns skutočný typ, alebo samotne meno, ak este nie je rozlíšené
 */
const Type *skipNamedType(const Type *type){
    while (type != NULL && type->kind == TY_NAMED && type->named.resolved != NULL){
        type = type->named.resolved;
    }

    return type;
}

/**
 * @brief funkcia na zistenie, ci zjednotenie obsahuje rovnaky typ
 * @param members je zoznam členov zjednotenia
 * @param type je hladaný typ
 * @returns true, ak niektory člen je rovnaky ako type
 */
bool typeListContains(const TypeList *members, const Type *type){
    for (size_t memberIndex = 0; memberIndex < members->len; memberIndex++){
        if (type_equals(members->items[memberIndex], type)){
            return true;
        }
    }

    return false;
}

bool type_equals(const Type *a, const Type *b){
    a = skipNamedType(a);
    b = skipNamedType(b);

    if (a == NULL || b == NULL){
        return false;
    }
    if (a == b){
        return true;
    }
    if (a->kind != b->kind){
        return false;
    }

    if (a->kind == TY_LIST){
        return type_equals(a->elem, b->elem);
    }
    if (a->kind == TY_FUNC){
        if (a->func.params.len != b->func.params.len){
            return false;
        }
        for (size_t paramIndex = 0; paramIndex < a->func.params.len; paramIndex++){
            if (!type_equals(a->func.params.items[paramIndex], b->func.params.items[paramIndex])){
                return false;
            }
        }
        return type_equals(a->func.ret, b->func.ret);
    }
    if (a->kind == TY_UNION){
        if (a->members.len != b->members.len){
            return false;
        }
        // poradie clenov nerozhoduje
        for (size_t memberIndex = 0; memberIndex < a->members.len; memberIndex++){
            if (!typeListContains(&b->members, a->members.items[memberIndex])){
                return false;
            }
        }
        return true;
    }
    if (a->kind == TY_CLASS){
        return a->cls == b->cls;
    }
    if (a->kind == TY_NAMED){
        // obe mena ešte nerozlisene
        return strcmp(a->named.name, b->named.name) == 0;
    }

    // jednoduché typy
    return true;
}

/**
 * @brief funkcia na zistenie, ci typ prijme hodnotu bez prevodu Int na Double
 *
 * Hodnotu zjednotenia nevieme pri preklade previest, lebo nevieme, ktory člen v nej je.
 * @param expected je typ, ktorý na danom mieste cakame
 * @param actual je typ hodnoty
 * @returns true, ak je to v poriadku aj bez prevodu
 */
bool acceptsWithoutConversion(const Type *expected, const Type *actual){
    expected = skipNamedType(expected);
    actual = skipNamedType(actual);

    if (expected == NULL || actual == NULL){
        return false;
    }
    if (expected->kind == TY_ANY || type_equals(expected, actual)){
        return true;
    }

    if (actual->kind == TY_UNION){
        for (size_t memberIndex = 0; memberIndex < actual->members.len; memberIndex++){
            if (!acceptsWithoutConversion(expected, actual->members.items[memberIndex])){
                return false;
            }
        }
        return true;
    }

    if (expected->kind == TY_UNION){
        for (size_t memberIndex = 0; memberIndex < expected->members.len; memberIndex++){
            if (acceptsWithoutConversion(expected->members.items[memberIndex], actual)){
                return true;
            }
        }
    }

    return false;
}

bool type_accepts(const Type *expected, const Type *actual){
    expected = skipNamedType(expected);
    actual = skipNamedType(actual);

    if (expected == NULL || actual == NULL){
        return false;
    }
    if (expected->kind == TY_ANY){
        return true;
    }
    if (type_equals(expected, actual)){
        return true;
    }
    // jedina implicitná konverzia
    if (expected->kind == TY_DOUBLE && actual->kind == TY_INT){
        return true;
    }

    // prázdny zoznam List() nema typ prvkov, prijme ho kazdy zoznam
    if (expected->kind == TY_LIST && actual->kind == TY_LIST && actual->elem == NULL){
        return true;
    }

    // hodnota typu zjednotenie: musi sediet každý jej clen, a to bez prevodu
    if (actual->kind == TY_UNION){
        return acceptsWithoutConversion(expected, actual);
    }

    // ocakavane zjednotenie: stačí jeden člen
    if (expected->kind == TY_UNION){
        for (size_t memberIndex = 0; memberIndex < expected->members.len; memberIndex++){
            if (type_accepts(expected->members.items[memberIndex], actual)){
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief funkcia na vypisanie zoznamu typov oddelenych oddeľovačom
 * @param list je zoznam typov
 * @param separator je oddelovac medzi typmi
 * @param out je subor, do ktoreho sa vypisuje
 */
void printTypeList(const TypeList *list, const char *separator, FILE *out){
    for (size_t typeIndex = 0; typeIndex < list->len; typeIndex++){
        if (typeIndex > 0){
            fputs(separator, out);
        }
        type_print(list->items[typeIndex], out);
    }
}

void type_print(const Type *t, FILE *out){
    if (t == NULL){
        fputs("?", out);
        return;
    }

    if (t->kind == TY_INT){
        fputs("Int", out);
    }
    else if (t->kind == TY_DOUBLE){
        fputs("Double", out);
    }
    else if (t->kind == TY_STRING){
        fputs("String", out);
    }
    else if (t->kind == TY_UNIT){
        fputs("Unit", out);
    }
    else if (t->kind == TY_BOOL){
        fputs("Boolean", out);
    }
    else if (t->kind == TY_LIST){
        fputs("List[", out);
        type_print(t->elem, out);
        fputs("]", out);
    }
    else if (t->kind == TY_FUNC){
        fputs("(", out);
        printTypeList(&t->func.params, ", ", out);
        fputs(") => ", out);
        type_print(t->func.ret, out);
    }
    else if (t->kind == TY_UNION){
        printTypeList(&t->members, " | ", out);
    }
    else if (t->kind == TY_CLASS){
        fputs("case class", out);
    }
    else if (t->kind == TY_NAMED){
        fputs(t->named.name, out);
    }
    else if (t->kind == TY_RANGE){
        fputs("Range", out);
    }
    else{
        fputs("Any", out);
    }
}

void type_free_all(void){
    for (size_t index = 0; index < allTypesCount; index++){
        Type *type = allTypes[index];

        if (type->kind == TY_FUNC){
            free(type->func.params.items);
        }
        else if (type->kind == TY_UNION){
            free(type->members.items);
        }
        else if (type->kind == TY_NAMED){
            free(type->named.name);
        }
        free(type);
    }

    free(allTypes);
    allTypes = NULL;
    allTypesCount = 0;
    allTypesCapacity = 0;
}

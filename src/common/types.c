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

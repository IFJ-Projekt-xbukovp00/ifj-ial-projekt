/**
 * @file dynstr.c
 * @brief Dynamický reťazec (buffer pre lexémy a generovaný kód)
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

#include "dynstr.h"

#include <stdlib.h>
#include <string.h>

#define DYNSTR_INIT_CAP 16 // Počiatočná kapacita dyn. reťazca

/**
 * @brief Zabezpečí, aby sa do reťazca zmestilo ďalších @p extra znakov + '\0'.
 * @return true pri úspechu, false pri chybe alokácie (reťazec ostane nezmenený).
 */
static bool dynstr_reserve(DynStr *s, size_t extra) {
    size_t needed = s->len + extra + 1;
    if (needed <= s->cap) {
        return true;
    }

    size_t new_cap;
    if (s->cap) {
        new_cap = s->cap;
    } else {
        new_cap = DYNSTR_INIT_CAP;
    }
    
    while (new_cap < needed) {
        new_cap *= 2;
    }

    char *tmp = realloc(s->data, new_cap);
    if (tmp == NULL) {
        return false;
    }
    s->data = tmp;
    s->cap = new_cap;
    return true;
}

bool dynstr_init(DynStr *s) {
    s->data = malloc(DYNSTR_INIT_CAP);
    if (s->data == NULL) {
        s->len = 0;
        s->cap = 0;
        return false;
    }
    s->data[0] = '\0';
    s->len = 0;
    s->cap = DYNSTR_INIT_CAP;
    return true;
}

void dynstr_free(DynStr *s) {
    if (s == NULL) {
        return;
    }
    free(s->data);
    s->data = NULL;
    s->len = 0;
    s->cap = 0;
}

void dynstr_clear(DynStr *s) {
    s->len = 0;
    if (s->data != NULL) {
        s->data[0] = '\0';
    }
}

bool dynstr_append_char(DynStr *s, char c) {
    if (!dynstr_reserve(s, 1)) {
        return false;
    }
    s->data[s->len++] = c;
    s->data[s->len] = '\0';
    return true;
}

bool dynstr_append(DynStr *s, const char *str) {
    size_t n = strlen(str);
    if (!dynstr_reserve(s, n)) {
        return false;
    }
    memcpy(s->data + s->len, str, n + 1);
    s->len += n;
    return true;
}

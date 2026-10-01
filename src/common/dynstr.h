/**
 * @file dynstr.h
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

#pragma once

#include <stdbool.h>
#include <stddef.h>

/** @brief Dynamicky rastúci reťazec. */
typedef struct {
    char *data; /**< Obsah reťazca ukončený '\\0'. */
    size_t len; /**< Aktuálna dĺžka bez '\\0'. */
    size_t cap; /**< Alokovaná kapacita. */
} DynStr;

/**
 * @brief Inicializuje prázdny reťazec.
 * @return true pri úspechu, false pri chybe alokácie.
 */
bool dynstr_init(DynStr *s);
/**
 * @brief Uvoľní pamäť reťazca.
 */
void dynstr_free(DynStr *s);
/**
 * @brief Vyprázdni reťazec bez uvoľnenia pamäte.
 */
void dynstr_clear(DynStr *s);
/**
 * @brief Pripojí jeden znak.
 * @return true pri úspechu, false pri chybe alokácie.
 */
bool dynstr_append_char(DynStr *s, char c);
/**
 * @brief Pripojí reťazec.
 * @return true pri úspechu, false pri chybe alokácie.
 */
bool dynstr_append(DynStr *s, const char *str);

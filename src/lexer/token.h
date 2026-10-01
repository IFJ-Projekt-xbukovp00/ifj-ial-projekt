/**
 * @file token.h
 * @brief Definícia tokenov lexikálneho analyzátora
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

#include "dynstr.h"

/** @brief Typ tokenu. */
typedef enum {
    TOK_EOF,
    TOK_EOL,
    TOK_INDENT,
    TOK_DEDENT,
    TOK_IDENTIFIER,
    TOK_KEYWORD,
    TOK_INT_LIT,
    TOK_DOUBLE_LIT,
    TOK_STRING_LIT,
    /* TODO: operátory, zátvorky, interpunkcia */
} TokenType;

/** @brief Token vrátený lexikálnym analyzátorom. */
typedef struct {
    TokenType type; /**< Typ tokenu. */
    DynStr lexeme;  /**< Text lexému. */
    int line;       /**< Riadok vo zdrojovom súbore. */
} Token;

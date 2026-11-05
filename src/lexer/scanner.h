/**
 * @file scanner.h
 * @brief Lexikálny analyzátor (konečný automat)
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

#include "error.h"
#include "token.h"

#include <stdio.h>

/**
 * @brief Inicializuje skener nad vstupným súborom.
 * @param input Zdrojový súbor.
 */
void scanner_init(FILE *input);

/**
 * @brief Načíta ďalší token.
 *
 * Komentáre sa preskakujú. Na začiatku každého logického riadku (okrem prvého) príde
 * TOK_EOL a podľa odsadenia TOK_INDENT alebo jeden či viac TOK_DEDENT; na konci vstupu
 * TOK_EOL, TOK_DEDENT až po nulové odsadenie a TOK_EOF (potom už stále TOK_EOF).
 * Vnútri zátvoriek sa koniec riadku nevracia.
 *
 * Interpolovaný reťazec prichádza po častiach ako TOK_INTERP_STRING; časť sa rozlíši podľa
 * lexému: začína s" alebo } a končí " (koniec reťazca) alebo ${ (nasleduje výraz, ktorý
 * končí časťou začínajúcou }).
 *
 * @param[out] token Načítaný token; token->lexeme musí byť inicializovaný (dynstr_init)
 *                   alebo vynulovaný, obsah sa prepíše.
 * @return ERR_OK, pri lexikálnej chybe ERR_LEX, pri chybnom odsadení ERR_SYNTAX,
 *         pri nedostatku pamäte ERR_INTERNAL.
 */
int scanner_get_token(Token *token);

/**
 * @brief Uvoľní pamäť skenera (zásobníky, rozpracované tokeny).
 */
void scanner_free(void);

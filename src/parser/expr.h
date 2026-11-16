/**
 * @file expr.h
 * @brief Precedenčná syntaktická analýza výrazov
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

#include "ast.h"
#include "expr_stack.h"
#include "token.h"

/**
 * @brief precedencna analyza jedneho vyrazu
 * @param token prvy token vyrazu, na vystupe prvy token za vyrazom
 * @param output koren AST vyrazu (nastavi sa pri ERR_OK)
 * @return ERR_OK, ERR_SYNTAX, ERR_LEX alebo ERR_INTERNAL
 */
int expr_parse(Token *token, ASTNode **output);

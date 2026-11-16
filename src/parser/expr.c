/**
 * @file expr.c
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

#include "expr.h"
#include "error.h"
#include "scanner.h"

#include <stdlib.h>
#include <string.h>

typedef enum {
    R_ERR = 0, // syntakticka chyba
    R_LT,      // < shift so znackou
    R_EQ,      // = shift bez znacky
    R_GT,      // > redukcia
    R_END      // $ $ koniec
} PtRel;

// precedencna tabulka [vrchny terminal][vstupny token]
// clang-format off
static const PtRel precTable[PT_COUNT][PT_COUNT] = {
    /*             .      u-     !      *      /      +      -      ++     <      <=     >      >=     ==     !=     &&     ||     to     until  by     f      ,      (      )      i      $ */
    [PT_DOT]    = {R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_EQ , R_ERR, R_ERR, R_ERR, R_EQ , R_ERR},
    [PT_UMINUS] = {R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_NOT]    = {R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_MUL]    = {R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_DIV]    = {R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_ADD]    = {R_LT , R_LT , R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_SUB]    = {R_LT , R_LT , R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_CONCAT] = {R_LT , R_LT , R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_LT]     = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_LE]     = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_GT]     = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_GE]     = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_EQ]     = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_ERR, R_ERR, R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_NEQ]    = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_ERR, R_ERR, R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_AND]    = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_OR]     = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_TO]     = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_UNTIL]  = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_BY]     = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_FUNC]   = {R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR, R_ERR , R_EQ , R_ERR, R_ERR, R_ERR},
    [PT_COMMA]  = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_EQ , R_LT , R_EQ , R_LT , R_ERR},
    [PT_LPAR]   = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_EQ , R_LT , R_EQ , R_LT , R_ERR},
    [PT_RPAR]   = {R_GT , R_ERR, R_ERR, R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_ERR, R_GT , R_ERR, R_GT , R_ERR, R_GT},
    [PT_ID]     = {R_GT , R_ERR, R_ERR, R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_ERR, R_GT , R_ERR, R_GT , R_ERR, R_GT},
    [PT_DOLLAR] = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_ERR, R_LT , R_ERR, R_LT , R_END},
};
// clang-format on

typedef struct {
    Token *token;
} PsaState;

static int nextToken(PsaState *state) { return scanner_get_token(state->token); }

// zaradi token do stlpca tabulky, vsetko mimo vyrazu je $
static int classifyToken(PsaState *state, PtIndex *column) {
    const Token *token = state->token;
    switch (token->type) {
    case TOK_IDENTIFIER:
    case TOK_INT_LIT:
    case TOK_DOUBLE_LIT:
    case TOK_STRING_LIT:
    case TOK_BOOL_LIT:
        *column = PT_ID;
        break;
    case TOK_MINUS:
        *column = PT_SUB;
        break;
    case TOK_PLUS:
        *column = PT_ADD;
        break;
    case TOK_MULTIPLY:
        *column = PT_MUL;
        break;
    case TOK_DIVIDE:
        *column = PT_DIV;
        break;
    case TOK_CONCAT:
        *column = PT_CONCAT;
        break;
    case TOK_LESS:
        *column = PT_LT;
        break;
    case TOK_LESS_EQUAL:
        *column = PT_LE;
        break;
    case TOK_GREATER:
        *column = PT_GT;
        break;
    case TOK_GREATER_EQUAL:
        *column = PT_GE;
        break;
    case TOK_EQUAL:
        *column = PT_EQ;
        break;
    case TOK_NOT_EQUAL:
        *column = PT_NEQ;
        break;
    case TOK_NOT:
        *column = PT_NOT;
        break;
    case TOK_AND:
        *column = PT_AND;
        break;
    case TOK_OR:
        *column = PT_OR;
        break;
    case TOK_DOT:
        *column = PT_DOT;
        break;
    case TOK_LPAREN:
        *column = PT_LPAR;
        break;
    case TOK_RPAREN:
        *column = PT_RPAR;
        break;
    case TOK_COMMA:
        *column = PT_COMMA;
        break;
    default:
        *column = PT_DOLLAR;
        break;
    }
    return ERR_OK;
}

/**
 * @file expr_stack.h
 * @brief Zásobník pre precedenčnú syntaktickú analýzu výrazov
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

#include "ast.h"
#include "token.h"

// index riadku a stlpca precedencnej tabulky (poradie musi sediet s precTable v expr.c)
typedef enum {
    PT_DOT,    // . (VALMEM/LISTS/EXTKW)
    PT_UMINUS, // unarne - a +
    PT_NOT,    // !
    PT_MUL,    // *
    PT_DIV,    // /
    PT_ADD,    // +
    PT_SUB,    // binarne -
    PT_CONCAT, // ++ (LISTS)
    PT_LT,     // <
    PT_LE,     // <=
    PT_GT,     // >
    PT_GE,     // >=
    PT_EQ,     // ==
    PT_NEQ,    // !=
    PT_AND,    // && (BOOLTHEN)
    PT_OR,     // || (BOOLTHEN)
    PT_TO,     // to (CONTROL)
    PT_UNTIL,  // until (CONTROL)
    PT_BY,     // by (CONTROL)
    PT_FUNC,   // f, identifikator pred (
    PT_COMMA,  // ,
    PT_LPAR,   // (
    PT_RPAR,   // )
    PT_ID,     // i, identifikator alebo literal
    PT_DOLLAR, // $, koniec vyrazu
    PT_COUNT
} PtIndex;

typedef enum {
    SYM_TERM, // terminal
    SYM_EXPR, // neterminal E
    SYM_MARK  // znacka <
} SymKind;

typedef struct {
    SymKind kind;
    PtIndex ptIndex;     // pri SYM_TERM index do tabulky
    TokenType tokenType; // pri SYM_TERM povodny token pre AST
    ASTNode *node;       // pri SYM_EXPR, pri i sa robi hned pri shifte
    int line;
} StackItem;

#define SMAX 1000

// zasobnik implementovany polom
typedef struct tastack {
    StackItem dataArray[SMAX];
    int top;
} TAStack;

/** @brief inicializuje prazdny zasobnik */
void A_InitStack(TAStack *stack);

/** @brief vlozi prvok na vrchol zasobnika (volajuci musi overit !A_IsFull) */
void A_Push(TAStack *stack, StackItem item);

/** @brief odstrani prvok z vrcholu zasobnika */
void A_Pop(TAStack *stack);

/** @brief vrati prvok na vrchole (zasobnik nesmie byt prazdny) */
StackItem A_Top(TAStack *stack);

/** @brief vrati true ak je zasobnik prazdny */
bool A_IsEmpty(TAStack *stack);

/** @brief vrati true ak je zasobnik plny */
bool A_IsFull(TAStack *stack);

/**
 * @brief najde najvrchnejsi terminal na zasobniku (E a < preskakuje)
 * @return pointer na polozku, alebo NULL ak tam ziadny terminal nie je
 */
StackItem *A_TopTerminal(TAStack *stack);

/**
 * @brief vlozi znacku < hned za najvrchnejsi terminal
 * @return false ak je zasobnik plny alebo v nom nie je terminal
 */
bool A_InsertMark(TAStack *stack);

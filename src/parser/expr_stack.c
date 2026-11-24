/**
 * @file expr_stack.c
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

#include "expr_stack.h"

#include <stddef.h>

void A_InitStack(TAStack *stack) { stack->top = 0; }

void A_Push(TAStack *stack, StackItem item) {
    if (stack->top < SMAX) {
        stack->dataArray[stack->top] = item;
        stack->top++;
    }
}

void A_Pop(TAStack *stack) {
    if (stack->top > 0) {
        stack->top--;
    }
}

StackItem A_Top(TAStack *stack) {
    // podmienkou je neprazdny zasobnik
    return stack->dataArray[stack->top - 1];
}

bool A_IsEmpty(const TAStack *stack) { return stack->top == 0; }

bool A_IsFull(const TAStack *stack) { return stack->top == SMAX; }

static int topTerminalIndex(const TAStack *stack) {
    for (int i = stack->top - 1; i >= 0; i--) {
        if (stack->dataArray[i].kind == SYM_TERM) {
            return i;
        }
    }
    return -1;
}

StackItem *A_TopTerminal(TAStack *stack) {
    int idx = topTerminalIndex(stack);
    return (idx < 0) ? NULL : &stack->dataArray[idx];
}

bool A_InsertMark(TAStack *stack) {
    int idx = topTerminalIndex(stack);
    if (idx < 0 || A_IsFull(stack)) {
        return false;
    }

    // posun prvkov nad terminalom o jedno miesto hore
    for (int i = stack->top; i > idx + 1; i--) {
        stack->dataArray[i] = stack->dataArray[i - 1];
    }

    StackItem mark = {.kind = SYM_MARK, .node = NULL};
    stack->dataArray[idx + 1] = mark;
    stack->top++;
    return true;
}

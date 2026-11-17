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

bool A_IsEmpty(TAStack *stack) { return stack->top == 0; }

bool A_IsFull(TAStack *stack) { return stack->top == SMAX; }

static int topTerminalIndex(TAStack *stack) {
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

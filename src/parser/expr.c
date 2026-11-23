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
    [PT_LT]     = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_ERR, R_ERR, R_ERR, R_ERR, R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_LE]     = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_ERR, R_ERR, R_ERR, R_ERR, R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_GT]     = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_ERR, R_ERR, R_ERR, R_ERR, R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
    [PT_GE]     = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_ERR, R_ERR, R_ERR, R_ERR, R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_LT , R_GT , R_LT , R_GT , R_LT , R_GT},
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
    [PT_RPAR]   = {R_GT , R_ERR, R_ERR, R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_ERR, R_GT , R_GT, R_GT , R_ERR, R_GT},
    [PT_ID]     = {R_GT , R_ERR, R_ERR, R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_GT , R_ERR, R_GT , R_ERR, R_GT , R_ERR, R_GT},
    [PT_DOLLAR] = {R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_LT , R_ERR, R_LT , R_ERR, R_LT , R_END},
};
// clang-format on

// dekoduje escape sekvencie v retazcovom literali
static char *decodeStringLiteral(const char *src, size_t len) {
    char *out = malloc(len + 1);
    if (out == NULL) {
        return NULL;
    }
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (src[i] == '\\' && i + 1 < len) {
            i++;
            switch (src[i]) {
            case 'n':
                out[j++] = '\n';
                break;
            case 'r':
                out[j++] = '\r';
                break;
            case 't':
                out[j++] = '\t';
                break;
            case '\\':
                out[j++] = '\\';
                break;
            case '"':
                out[j++] = '"';
                break;
            default:
                out[j++] = src[i];
                break;
            }
        } else {
            out[j++] = src[i];
        }
    }
    out[j] = '\0';
    return out;
}

// odstrani oddelovacie podciarkovniky z ciselneho literalu pre strtoll/strtod
static char *stripUnderscores(const char *src, size_t len) {
    char *out = malloc(len + 1);
    if (out == NULL) {
        return NULL;
    }
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (src[i] != '_') {
            out[j++] = src[i];
        }
    }
    out[j] = '\0';
    return out;
}

// pravidlo E -> i, vytvori LiteralNode alebo IDNode z tokenu
static ASTNode *nodeFromToken(const Token *token) {
    const char *lexeme = token->lexeme.data;
    size_t len = token->lexeme.len;

    if (token->type == TOK_IDENTIFIER || token->type == TOK_KW_LIST) {
        return astNewId(lexeme, token->line);
    }

    switch (token->type) {
    case TOK_INT_LIT: {
        char *clean = stripUnderscores(lexeme, len);
        if (clean == NULL) {
            return NULL;
        }
        long long val = strtoll(clean, NULL, 10);
        free(clean);
        return astNewIntLit(val, token->line);
    }
    case TOK_DOUBLE_LIT: {
        char *clean = stripUnderscores(lexeme, len);
        if (clean == NULL) {
            return NULL;
        }
        // strtod sa sam zastavi na pripone d/D
        double val = strtod(clean, NULL);
        free(clean);
        return astNewDoubleLit(val, token->line);
    }
    case TOK_BOOL_LIT:
        return astNewBoolLit(strcmp(lexeme, "true") == 0, token->line);
    case TOK_STRING_LIT: {
        size_t quotes = (len >= 6 && strncmp(lexeme, "\"\"\"", 3) == 0) ? 3 : 1;
        if (len < 2 * quotes) {
            return NULL;
        }
        char *decoded = decodeStringLiteral(lexeme + quotes, len - 2 * quotes);
        if (decoded == NULL) {
            return NULL;
        }
        ASTNode *lit = astNewStringLit(decoded, token->line);
        free(decoded);
        return lit;
    }
    default:
        return NULL;
    }
}

typedef struct {
    Token *token;
    Token lookahead;   // lookahead na rozlisenie f( od i
    bool hasLookahead;
    int parenDepth;    // pocet otvorenych ( v ramci vyrazu
    bool afterOperand; // posledny bol i alebo ), takze - je binarne
} PsaState;

static int nextToken(PsaState *state) {
    if (state->hasLookahead) {
        dynstr_free(&state->token->lexeme);
        *state->token = state->lookahead;
        memset(&state->lookahead, 0, sizeof(Token));
        state->hasLookahead = false;
        return ERR_OK;
    }
    return scanner_get_token(state->token);
}

// nacita dalsi token bez posunu aktualneho
static int peekToken(PsaState *state, Token **next) {
    if (!state->hasLookahead) {
        memset(&state->lookahead, 0, sizeof(Token));
        if (!dynstr_init(&state->lookahead.lexeme)) {
            return ERR_INTERNAL;
        }
        int err = scanner_get_token(&state->lookahead);
        if (err != ERR_OK) {
            dynstr_free(&state->lookahead.lexeme);
            return err;
        }
        state->hasLookahead = true;
    }
    *next = &state->lookahead;
    return ERR_OK;
}

// zaradi token do stlpca tabulky, vsetko mimo vyrazu je $
static int classifyToken(PsaState *state, PtIndex *column) {
    const Token *token = state->token;
    switch (token->type) {
    case TOK_KW_LIST: // List(...) a List.range(...) ako identifikator
    case TOK_IDENTIFIER: {
        // to/until/by chodia z lexera ako bezny identifikator
        if (strcmp(token->lexeme.data, "to") == 0) {
            *column = PT_TO;
            return ERR_OK;
        }
        if (strcmp(token->lexeme.data, "until") == 0) {
            *column = PT_UNTIL;
            return ERR_OK;
        }
        if (strcmp(token->lexeme.data, "by") == 0) {
            *column = PT_BY;
            return ERR_OK;
        }
        Token *next;
        int err = peekToken(state, &next);
        if (err != ERR_OK) {
            return err;
        }
        *column = (next->type == TOK_LPAREN) ? PT_FUNC : PT_ID;
        return ERR_OK;
    }
    case TOK_INT_LIT:
    case TOK_DOUBLE_LIT:
    case TOK_STRING_LIT:
    case TOK_BOOL_LIT:
        *column = PT_ID;
        break;
    case TOK_MINUS:
        *column = state->afterOperand ? PT_SUB : PT_UMINUS;
        break;
    case TOK_PLUS:
        *column = state->afterOperand ? PT_ADD : PT_UMINUS;
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
    // ) a , mimo nasich zatvoriek uz patria parseru
    case TOK_RPAREN:
        *column = state->parenDepth > 0 ? PT_RPAR : PT_DOLLAR;
        break;
    case TOK_COMMA:
        *column = state->parenDepth > 0 ? PT_COMMA : PT_DOLLAR;
        break;
    default:
        *column = PT_DOLLAR;
        break;
    }
    return ERR_OK;
}

static bool isTerminal(const StackItem *item, PtIndex ptIndex) {
    return item->kind == SYM_TERM && item->ptIndex == ptIndex;
}

static bool isExpr(const StackItem *item) { return item->kind == SYM_EXPR; }

static bool isBinaryOperator(PtIndex ptIndex) {
    switch (ptIndex) {
    case PT_MUL:
    case PT_DIV:
    case PT_ADD:
    case PT_SUB:
    case PT_CONCAT:
    case PT_LT:
    case PT_LE:
    case PT_GT:
    case PT_GE:
    case PT_EQ:
    case PT_NEQ:
    case PT_AND:
    case PT_OR:
    case PT_TO:
    case PT_UNTIL:
    case PT_BY:
        return true;
    default:
        return false;
    }
}

// rozparsuje ( ) alebo ( E , E , ... ) do pola argumentov
static int takeArguments(StackItem *items, int count, ASTNode ***args, size_t *argsCount) {
    if (count < 2 || !isTerminal(&items[0], PT_LPAR) || !isTerminal(&items[count - 1], PT_RPAR)) {
        return ERR_SYNTAX;
    }
    if (count == 2) {
        return ERR_OK; // f()
    }

    int inner = count - 2;
    if (inner % 2 == 0) {
        return ERR_SYNTAX;
    }
    for (int i = 1; i <= inner; i++) {
        if (i % 2 == 1) {
            if (!isExpr(&items[i])) {
                return ERR_SYNTAX;
            }
        } else if (!isTerminal(&items[i], PT_COMMA)) {
            return ERR_SYNTAX;
        }
    }

    for (int i = 1; i <= inner; i += 2) {
        if (!astAppend(args, argsCount, items[i].node)) {
            return ERR_INTERNAL;
        }
        items[i].node = NULL; // uz patri volaniu alebo zoznamu
    }
    return ERR_OK;
}

// zredukuje handle nad znackou < na AST uzol (pouzite uzly vynuluje proti double-free)
static int reduceHandle(StackItem *handle, int count, ASTNode **result) {
    int line = handle[0].line;

    // E -> i
    if (count == 1 && isTerminal(&handle[0], PT_ID)) {
        *result = handle[0].node;
        handle[0].node = NULL;
        return ERR_OK;
    }

    // E -> -E | !E
    if (count == 2 && handle[0].kind == SYM_TERM &&
        (handle[0].ptIndex == PT_UMINUS || handle[0].ptIndex == PT_NOT) && isExpr(&handle[1])) {
        ASTNode *un = astNewUnary(handle[0].tokenType, handle[1].node, line);
        if (un == NULL) {
            return ERR_INTERNAL;
        }
        handle[1].node = NULL;
        *result = un;
        return ERR_OK;
    }

    // E -> ( ), hodnota typu Unit
    if (count == 2 && isTerminal(&handle[0], PT_LPAR) && isTerminal(&handle[1], PT_RPAR)) {
        ASTNode *lit = astNewUnitLit(line);
        if (lit == NULL) {
            return ERR_INTERNAL;
        }
        *result = lit;
        return ERR_OK;
    }

    // E -> ( E )
    if (count == 3 && isTerminal(&handle[0], PT_LPAR) && isExpr(&handle[1]) &&
        isTerminal(&handle[2], PT_RPAR)) {
        *result = handle[1].node;
        handle[1].node = NULL;
        return ERR_OK;
    }

    // E -> E . i
    if (count == 3 && isExpr(&handle[0]) && isTerminal(&handle[1], PT_DOT) &&
        isTerminal(&handle[2], PT_ID)) {
        if (handle[2].node == NULL || handle[2].node->kind != AST_ID) {
            return ERR_SYNTAX; // x.5 nie je platny clen
        }
        ASTNode *mem =
            astNewMember(handle[0].node, ((IDNode *)handle[2].node)->name, handle[1].line);
        if (mem == NULL) {
            return ERR_INTERNAL;
        }
        astFree(handle[2].node);
        handle[0].node = handle[2].node = NULL;
        *result = mem;
        return ERR_OK;
    }

    // E -> E op E
    if (count == 3 && isExpr(&handle[0]) && handle[1].kind == SYM_TERM &&
        isBinaryOperator(handle[1].ptIndex) && isExpr(&handle[2])) {
        PtIndex op = handle[1].ptIndex;

        if (op == PT_TO || op == PT_UNTIL) {
            ASTNode *range = astNewRange(handle[0].node, handle[2].node, NULL, op == PT_TO,
                                         handle[1].line);
            if (range == NULL) {
                return ERR_INTERNAL;
            }
            *result = range;
        } else if (op == PT_BY) {
            // by moze ist len za to/until, ktore este nema krok
            if (handle[0].node == NULL || handle[0].node->kind != AST_RANGE) {
                return ERR_SYNTAX;
            }
            RangeNode *range = (RangeNode *)handle[0].node;
            if (range->step != NULL) {
                return ERR_SYNTAX;
            }
            range->step = handle[2].node;
            *result = handle[0].node;
        } else {
            ASTNode *bin = astNewBinary(handle[1].tokenType, handle[0].node, handle[2].node,
                                        handle[1].line);
            if (bin == NULL) {
                return ERR_INTERNAL;
            }
            *result = bin;
        }
        handle[0].node = handle[2].node = NULL;
        return ERR_OK;
    }

    // E -> List ( prvky ), zoznam nie je volanie funkcie
    if (count >= 3 && isTerminal(&handle[0], PT_FUNC) && handle[0].tokenType == TOK_KW_LIST) {
        ASTNode *listNode = astNewListLit(line);
        if (listNode == NULL) {
            return ERR_INTERNAL;
        }
        ListLitNode *list = (ListLitNode *)listNode;
        int err = takeArguments(&handle[1], count - 1, &list->elems, &list->elemsCount);
        if (err != ERR_OK) {
            astFree(listNode);
            return err;
        }
        *result = listNode;
        return ERR_OK;
    }

    // E -> f ( args )
    if (count >= 3 && isTerminal(&handle[0], PT_FUNC)) {
        ASTNode *callNode = astNewCall(NULL, line);
        if (callNode == NULL) {
            return ERR_INTERNAL;
        }
        CallNode *call = (CallNode *)callNode;
        int err = takeArguments(&handle[1], count - 1, &call->args, &call->argsCount);
        if (err != ERR_OK) {
            astFree(callNode);
            return err;
        }
        call->caller = handle[0].node;
        handle[0].node = NULL;
        *result = callNode;
        return ERR_OK;
    }

    // E -> E . f ( args )
    if (count >= 5 && isExpr(&handle[0]) && isTerminal(&handle[1], PT_DOT) &&
        isTerminal(&handle[2], PT_FUNC)) {
        if (handle[2].node == NULL || handle[2].node->kind != AST_ID) {
            return ERR_SYNTAX;
        }
        ASTNode *memNode = astNewMember(NULL, ((IDNode *)handle[2].node)->name, handle[1].line);
        if (memNode == NULL) {
            return ERR_INTERNAL;
        }
        ASTNode *callNode = astNewCall(memNode, line);
        if (callNode == NULL) {
            astFree(memNode);
            return ERR_INTERNAL;
        }
        CallNode *call = (CallNode *)callNode;
        int err = takeArguments(&handle[3], count - 3, &call->args, &call->argsCount);
        if (err != ERR_OK) {
            astFree(callNode);
            return err;
        }
        ((MemberNode *)memNode)->object = handle[0].node;
        astFree(handle[2].node);
        handle[0].node = handle[2].node = NULL;
        *result = callNode;
        return ERR_OK;
    }

    // E -> E ( args ), napr. add(a)(b) alebo List(1, 2)(0)
    if (count >= 3 && isExpr(&handle[0]) && isTerminal(&handle[1], PT_LPAR)) {
        ASTNode *callNode = astNewCall(NULL, handle[1].line);
        if (callNode == NULL) {
            return ERR_INTERNAL;
        }
        CallNode *call = (CallNode *)callNode;
        int err = takeArguments(&handle[1], count - 1, &call->args, &call->argsCount);
        if (err != ERR_OK) {
            astFree(callNode);
            return err;
        }
        call->caller = handle[0].node;
        handle[0].node = NULL;
        *result = callNode;
        return ERR_OK;
    }

    return ERR_SYNTAX;
}

static int reduce(TAStack *stack) {
    int mark = stack->top - 1;
    while (mark >= 0 && stack->dataArray[mark].kind != SYM_MARK) {
        mark--;
    }
    if (mark < 0) {
        return ERR_SYNTAX;
    }

    ASTNode *result = NULL;
    int err = reduceHandle(&stack->dataArray[mark + 1], stack->top - mark - 1, &result);
    if (err != ERR_OK) {
        return err;
    }

    int line = result->line;
    stack->top = mark; // zahodi <handle
    StackItem expr = {.kind = SYM_EXPR, .node = result, .line = line};
    A_Push(stack, expr);
    return ERR_OK;
}

static void freeStack(TAStack *stack, PsaState *state) {
    for (int i = 0; i < stack->top; i++) {
        astFree(stack->dataArray[i].node);
    }
    stack->top = 0;
    if (state->hasLookahead) {
        dynstr_free(&state->lookahead.lexeme);
        state->hasLookahead = false;
    }
}

int expr_parse(Token *token, ASTNode *first, ASTNode **output) {
    if (token == NULL || output == NULL) {
        astFree(first);
        return ERR_INTERNAL;
    }

    // kazde volanie ma vlastny zasobnik, aby slo volat expr_parse aj rekurzivne
    TAStack *stack = malloc(sizeof(TAStack));
    if (stack == NULL) {
        astFree(first);
        return ERR_INTERNAL;
    }
    PsaState state = {.token = token};
    int err;

    A_InitStack(stack);
    StackItem dollar = {.kind = SYM_TERM, .ptIndex = PT_DOLLAR, .line = token->line};
    A_Push(stack, dollar);

    if (first != NULL) { // LL parser uz nacital prvy operand
        StackItem expr = {.kind = SYM_EXPR, .node = first, .line = first->line};
        A_Push(stack, expr);
        state.afterOperand = true;
    }

    PtIndex input;
    if ((err = classifyToken(&state, &input)) != ERR_OK) {
        goto fail;
    }

    for (;;) {
        const StackItem *topTerm = A_TopTerminal(stack);
        if (topTerm == NULL) {
            err = ERR_INTERNAL;
            goto fail;
        }
        PtRel rel = precTable[topTerm->ptIndex][input];

        if (rel == R_LT || rel == R_EQ) {
            if (A_IsFull(stack) || (rel == R_LT && !A_InsertMark(stack)) || A_IsFull(stack)) {
                err = ERR_INTERNAL;
                goto fail;
            }
            StackItem term = {
                .kind = SYM_TERM, .ptIndex = input, .tokenType = token->type, .line = token->line};
            // uzol robime hned pri shifte, kym mame lexem
            if (input == PT_ID || input == PT_FUNC) {
                term.node = nodeFromToken(token);
                if (term.node == NULL) {
                    err = ERR_INTERNAL;
                    goto fail;
                }
            }
            A_Push(stack, term);

            if (input == PT_LPAR) {
                state.parenDepth++;
            }
            if (input == PT_RPAR) {
                state.parenDepth--;
            }
            state.afterOperand = (input == PT_ID || input == PT_RPAR);

            if ((err = nextToken(&state)) != ERR_OK) {
                goto fail;
            }
            if ((err = classifyToken(&state, &input)) != ERR_OK) {
                goto fail;
            }
        } else if (rel == R_GT) {
            if ((err = reduce(stack)) != ERR_OK) {
                goto fail;
            }
        } else if (rel == R_END) {
            // na zasobniku musi ostat len $ E
            if (stack->top != 2 || !isExpr(&stack->dataArray[1])) {
                err = ERR_SYNTAX;
                goto fail;
            }
            *output = stack->dataArray[1].node;
            stack->top = 0;
            if (state.hasLookahead) {
                dynstr_free(&state.lookahead.lexeme);
                state.hasLookahead = false;
            }
            free(stack);
            return ERR_OK;
        } else {
            err = ERR_SYNTAX;
            goto fail;
        }
    }

fail:
    freeStack(stack, &state);
    free(stack);
    return err;
}

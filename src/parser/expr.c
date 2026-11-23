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

static char decodeEscapeChar(char c) {
    switch (c) {
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case '\\':
        return '\\';
    case '"':
        return '"';
    default:
        return c;
    }
}

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
            out[j++] = decodeEscapeChar(src[i]);
        } else {
            out[j++] = src[i];
        }
    }
    out[j] = '\0';
    return out;
}

static bool isIdentStart(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isIdentChar(char c) { return isIdentStart(c) || (c >= '0' && c <= '9'); }

static bool appendInterpText(InterpNode *interp, const char *text, size_t len, int line) {
    if (len == 0) {
        return true;
    }
    char *buf = malloc(len + 1);
    if (buf == NULL) {
        return false;
    }
    memcpy(buf, text, len);
    buf[len] = '\0';
    ASTNode *lit = astNewStringLit(buf, line);
    free(buf);
    if (lit == NULL || !astAppend(&interp->parts, &interp->partsCount, lit)) {
        astFree(lit);
        return false;
    }
    return true;
}

static bool appendInterpChunk(InterpNode *interp, const char *src, size_t len, bool isMulti,
                              int line) {
    char *buf = malloc(len + 1);
    if (buf == NULL) {
        return false;
    }
    size_t bufLen = 0;
    for (size_t i = 0; i < len; i++) {
        if (!isMulti && src[i] == '\\' && i + 1 < len) {
            i++;
            buf[bufLen++] = decodeEscapeChar(src[i]);
        } else if (src[i] == '$' && i + 1 < len && isIdentStart(src[i + 1])) {
            if (!appendInterpText(interp, buf, bufLen, line)) {
                free(buf);
                return false;
            }
            bufLen = 0;
            size_t start = i + 1;
            size_t end = start;
            while (end < len && isIdentChar(src[end])) {
                end++;
            }
            char *idBuf = malloc(end - start + 1);
            if (idBuf == NULL) {
                free(buf);
                return false;
            }
            memcpy(idBuf, src + start, end - start);
            idBuf[end - start] = '\0';
            ASTNode *id = astNewId(idBuf, line);
            free(idBuf);
            if (id == NULL || !astAppend(&interp->parts, &interp->partsCount, id)) {
                astFree(id);
                free(buf);
                return false;
            }
            i = end - 1;
        } else if (src[i] == '$' && i + 1 < len && src[i + 1] == '$') {
            buf[bufLen++] = '$';
            i++;
        } else {
            buf[bufLen++] = src[i];
        }
    }
    bool ok = appendInterpText(interp, buf, bufLen, line);
    free(buf);
    return ok;
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
        size_t innerLen = len - 2 * quotes;
        char *buf = NULL;
        if (quotes == 3) {
            // viacriadkovy """...""" je raw, escape sekvencie sa nedekoduju
            buf = malloc(innerLen + 1);
            if (buf != NULL) {
                memcpy(buf, lexeme + 3, innerLen);
                buf[innerLen] = '\0';
            }
        } else {
            buf = decodeStringLiteral(lexeme + 1, innerLen);
        }
        if (buf == NULL) {
            return NULL;
        }
        ASTNode *lit = astNewStringLit(buf, token->line);
        free(buf);
        return lit;
    }
    default:
        return NULL;
    }
}

typedef struct {
    Token *token;
    Token lookahead[2]; // 2-tokenovy lookahead pre f( a lambdu () => / (a: T)
    int lookaheadCount;
    int parenDepth;    // pocet otvorenych ( v ramci vyrazu
    bool afterOperand; // posledny bol i alebo ), takze -/+ je binarne
} PsaState;

static void freeLookahead(PsaState *state) {
    for (int i = 0; i < state->lookaheadCount; i++) {
        dynstr_free(&state->lookahead[i].lexeme);
    }
    state->lookaheadCount = 0;
}

static int nextToken(PsaState *state) {
    if (state->lookaheadCount > 0) {
        dynstr_free(&state->token->lexeme);
        *state->token = state->lookahead[0];
        if (state->lookaheadCount == 2) {
            state->lookahead[0] = state->lookahead[1];
        }
        state->lookaheadCount--;
        memset(&state->lookahead[state->lookaheadCount], 0, sizeof(Token));
        return ERR_OK;
    }
    return scanner_get_token(state->token);
}

// nacita token s offsetom 0 alebo 1 bez posunu aktualneho
static int peekTokenAt(PsaState *state, int offset, Token **next) {
    if (offset < 0 || offset >= 2) {
        return ERR_INTERNAL;
    }
    while (state->lookaheadCount <= offset) {
        Token *slot = &state->lookahead[state->lookaheadCount];
        memset(slot, 0, sizeof(Token));
        if (!dynstr_init(&slot->lexeme)) {
            return ERR_INTERNAL;
        }
        int err = scanner_get_token(slot);
        if (err != ERR_OK) {
            dynstr_free(&slot->lexeme);
            return err;
        }
        state->lookaheadCount++;
    }
    *next = &state->lookahead[offset];
    return ERR_OK;
}

static int parseLambdaType(PsaState *state, Type **out);

static int parseLambdaPrimType(PsaState *state, Type **out) {
    Type *type = NULL;
    switch (state->token->type) {
    case TOK_KW_INT:
        type = type_get_basic(TY_INT);
        break;
    case TOK_KW_DOUBLE:
        type = type_get_basic(TY_DOUBLE);
        break;
    case TOK_KW_STRING:
        type = type_get_basic(TY_STRING);
        break;
    case TOK_KW_UNIT:
        type = type_get_basic(TY_UNIT);
        break;
    case TOK_KW_BOOLEAN:
        type = type_get_basic(TY_BOOL);
        break;
    case TOK_IDENTIFIER: {
        const char *name = state->token->lexeme.data != NULL ? state->token->lexeme.data : "";
        type = type_create_named(name);
        break;
    }
    case TOK_KW_LIST: {
        int err = nextToken(state);
        if (err != ERR_OK) {
            return err;
        }
        if (state->token->type != TOK_LBRACKET) {
            return ERR_SYNTAX;
        }
        err = nextToken(state);
        if (err != ERR_OK) {
            return err;
        }
        Type *elem = NULL;
        err = parseLambdaType(state, &elem);
        if (err != ERR_OK) {
            return err;
        }
        if (state->token->type != TOK_RBRACKET) {
            return ERR_SYNTAX;
        }
        err = nextToken(state);
        if (err != ERR_OK) {
            return err;
        }
        *out = type_create_list(elem);
        return ERR_OK;
    }
    default:
        return ERR_SYNTAX;
    }

    int err = nextToken(state);
    if (err != ERR_OK) {
        return err;
    }
    *out = type;
    return ERR_OK;
}

static int parseLambdaType(PsaState *state, Type **out) {
    if (state->token->type == TOK_LPAREN) {
        TypeList params = {0};
        int err = nextToken(state);
        if (err != ERR_OK) {
            return err;
        }
        if (state->token->type != TOK_RPAREN) {
            for (;;) {
                Type *paramType = NULL;
                err = parseLambdaType(state, &paramType);
                if (err != ERR_OK) {
                    free(params.items);
                    return err;
                }
                type_list_append(&params, paramType);
                if (state->token->type == TOK_COMMA) {
                    err = nextToken(state);
                    if (err != ERR_OK) {
                        free(params.items);
                        return err;
                    }
                } else {
                    break;
                }
            }
        }
        if (state->token->type != TOK_RPAREN) {
            free(params.items);
            return ERR_SYNTAX;
        }
        err = nextToken(state);
        if (err != ERR_OK) {
            free(params.items);
            return err;
        }
        if (state->token->type != TOK_ARROW) {
            free(params.items);
            return ERR_SYNTAX;
        }
        err = nextToken(state);
        if (err != ERR_OK) {
            free(params.items);
            return err;
        }
        Type *ret = NULL;
        err = parseLambdaType(state, &ret);
        if (err != ERR_OK) {
            free(params.items);
            return err;
        }
        *out = type_create_function(params, ret);
        return ERR_OK;
    }

    Type *first = NULL;
    int err = parseLambdaPrimType(state, &first);
    if (err != ERR_OK) {
        return err;
    }
    if (state->token->type != TOK_PIPE) {
        *out = first;
        return ERR_OK;
    }

    TypeList members = {0};
    type_list_append(&members, first);
    while (state->token->type == TOK_PIPE) {
        err = nextToken(state);
        if (err != ERR_OK) {
            free(members.items);
            return err;
        }
        Type *member = NULL;
        err = parseLambdaPrimType(state, &member);
        if (err != ERR_OK) {
            free(members.items);
            return err;
        }
        type_list_append(&members, member);
    }
    *out = type_create_union(members);
    return ERR_OK;
}

static int isLambdaStart(PsaState *state, bool *isLambda) {
    *isLambda = false;
    Token *t1 = NULL;
    int err = peekTokenAt(state, 0, &t1);
    if (err != ERR_OK) {
        return err;
    }
    if (t1->type == TOK_RPAREN) {
        Token *t2 = NULL;
        err = peekTokenAt(state, 1, &t2);
        if (err != ERR_OK) {
            return err;
        }
        *isLambda = (t2->type == TOK_ARROW);
        return ERR_OK;
    }
    if (t1->type == TOK_IDENTIFIER) {
        Token *t2 = NULL;
        err = peekTokenAt(state, 1, &t2);
        if (err != ERR_OK) {
            return err;
        }
        *isLambda = (t2->type == TOK_COLON);
        return ERR_OK;
    }
    return ERR_OK;
}

static int parseLambda(PsaState *state, ASTNode **out) {
    int line = state->token->line;
    ASTNode *node = astNewLambda(NULL, line);
    if (node == NULL) {
        return ERR_INTERNAL;
    }
    LambdaNode *lam = (LambdaNode *)node;

    int err = nextToken(state);
    if (err != ERR_OK) {
        astFree(node);
        return err;
    }
    if (state->token->type != TOK_RPAREN) {
        for (;;) {
            if (state->token->type != TOK_IDENTIFIER) {
                astFree(node);
                return ERR_SYNTAX;
            }
            int paramLine = state->token->line;
            const char *lexeme = state->token->lexeme.data != NULL ? state->token->lexeme.data : "";
            char *paramName = astCopyString(lexeme);
            if (paramName == NULL) {
                astFree(node);
                return ERR_INTERNAL;
            }
            err = nextToken(state);
            if (err != ERR_OK) {
                free(paramName);
                astFree(node);
                return err;
            }
            if (state->token->type != TOK_COLON) {
                free(paramName);
                astFree(node);
                return ERR_SYNTAX;
            }
            err = nextToken(state);
            if (err != ERR_OK) {
                free(paramName);
                astFree(node);
                return err;
            }
            Type *paramType = NULL;
            err = parseLambdaType(state, &paramType);
            if (err != ERR_OK) {
                free(paramName);
                astFree(node);
                return err;
            }
            bool ok = astParamListAdd(&lam->params, paramName, paramType, paramLine);
            free(paramName);
            if (!ok) {
                astFree(node);
                return ERR_INTERNAL;
            }
            if (state->token->type == TOK_COMMA) {
                err = nextToken(state);
                if (err != ERR_OK) {
                    astFree(node);
                    return err;
                }
            } else {
                break;
            }
        }
    }

    if (state->token->type != TOK_RPAREN) {
        astFree(node);
        return ERR_SYNTAX;
    }
    err = nextToken(state);
    if (err != ERR_OK) {
        astFree(node);
        return err;
    }
    if (state->token->type != TOK_ARROW) {
        astFree(node);
        return ERR_SYNTAX;
    }
    err = nextToken(state);
    if (err != ERR_OK) {
        astFree(node);
        return err;
    }

    ASTNode *body = NULL;
    err = expr_parse(state->token, NULL, &body);
    if (err != ERR_OK) {
        astFree(node);
        return err;
    }
    lam->body = body;
    *out = node;
    return ERR_OK;
}

static int parseInterpString(PsaState *state, ASTNode **out) {
    int line = state->token->line;
    const char *startLexeme = state->token->lexeme.data;
    size_t startLen = state->token->lexeme.len;
    bool isMulti = (startLen >= 4 && strncmp(startLexeme, "s\"\"\"", 4) == 0);

    ASTNode *node = astNewInterp(line);
    if (node == NULL) {
        return ERR_INTERNAL;
    }
    InterpNode *interp = (InterpNode *)node;

    bool first = true;
    for (;;) {
        const char *lexeme = state->token->lexeme.data;
        size_t len = state->token->lexeme.len;
        size_t prefix = first ? (isMulti ? 4 : 2) : 1;
        bool hasExpr = (len >= prefix + 2 && lexeme[len - 2] == '$' && lexeme[len - 1] == '{');
        size_t suffix = hasExpr ? 2 : (isMulti ? 3 : 1);

        if (len < prefix + suffix) {
            astFree(node);
            return ERR_SYNTAX;
        }
        if (!appendInterpChunk(interp, lexeme + prefix, len - prefix - suffix, isMulti,
                               state->token->line)) {
            astFree(node);
            return ERR_INTERNAL;
        }
        if (!hasExpr) {
            break;
        }

        // za ${ nasleduje vnoreny vyraz ukonceny castou zacinajucou }
        int err = nextToken(state);
        if (err != ERR_OK) {
            astFree(node);
            return err;
        }
        ASTNode *subExpr = NULL;
        err = expr_parse(state->token, NULL, &subExpr);
        if (err != ERR_OK) {
            astFree(node);
            return err;
        }
        if (!astAppend(&interp->parts, &interp->partsCount, subExpr)) {
            astFree(subExpr);
            astFree(node);
            return ERR_INTERNAL;
        }
        if (state->token->type != TOK_INTERP_STRING || state->token->lexeme.data == NULL ||
            state->token->lexeme.data[0] != '}') {
            astFree(node);
            return ERR_SYNTAX;
        }
        first = false;
    }

    if (interp->partsCount == 0) {
        ASTNode *empty = astNewStringLit("", line);
        if (empty == NULL || !astAppend(&interp->parts, &interp->partsCount, empty)) {
            astFree(empty);
            astFree(node);
            return ERR_INTERNAL;
        }
    }
    *out = node;
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
        if (state->afterOperand) {
            *column = PT_ID;
            return ERR_OK;
        }
        Token *next;
        int err = peekTokenAt(state, 0, &next);
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
    case TOK_INTERP_STRING:
        *column = (token->lexeme.data != NULL && token->lexeme.data[0] == 's') ? PT_ID : PT_DOLLAR;
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
        if (!state->afterOperand) {
            bool isLambda = false;
            int err = isLambdaStart(state, &isLambda);
            if (err != ERR_OK) {
                return err;
            }
            if (isLambda) {
                *column = PT_ID;
                return ERR_OK;
            }
        }
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
        astFree(handle[0].node);
        handle[0].node = NULL;
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
    freeLookahead(state);
}

int expr_parse(Token *token, ASTNode *first, ASTNode **output) {
    if (token == NULL || output == NULL) {
        astFree(first);
        return ERR_INTERNAL;
    }
    *output = NULL;

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
            bool consumedNext = false;
            // uzol robime hned pri shifte, kym mame lexem
            if (input == PT_ID && token->type == TOK_INTERP_STRING) {
                if ((err = parseInterpString(&state, &term.node)) != ERR_OK) {
                    goto fail;
                }
            } else if (input == PT_ID && token->type == TOK_LPAREN) {
                if ((err = parseLambda(&state, &term.node)) != ERR_OK) {
                    goto fail;
                }
                consumedNext = true;
            } else if (input == PT_ID || input == PT_FUNC) {
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

            if (!consumedNext && (err = nextToken(&state)) != ERR_OK) {
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
            freeLookahead(&state);
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

int expr_parse_type(Token *token, Type **output) {
    if (token == NULL || output == NULL) {
        return ERR_INTERNAL;
    }
    *output = NULL;
    PsaState state = {.token = token};
    return parseLambdaType(&state, output);
}

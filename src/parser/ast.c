/**
 * @file ast.c
 * @brief Abstraktný syntaktický strom
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

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASTNode *astAlloc(size_t size, ASTNodeKind kind, int line) {
    ASTNode *node = calloc(1, size);
    if (node == NULL) {
        return NULL;
    }
    node->kind = kind;
    node->line = line;
    node->type = NULL;
    return node;
}

char *astCopyString(const char *text) {
    if (text == NULL) {
        return NULL;
    }
    size_t len = strlen(text);
    char *copy = malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, text, len + 1);
    return copy;
}

ASTNode *astNewIntLit(long long value, int line) {
    LiteralNode *lit = (LiteralNode *)astAlloc(sizeof(LiteralNode), AST_LIT, line);
    if (lit == NULL) {
        return NULL;
    }
    lit->litType = LIT_INT;
    lit->litValue.int_value = value;
    return (ASTNode *)lit;
}

ASTNode *astNewDoubleLit(double value, int line) {
    LiteralNode *lit = (LiteralNode *)astAlloc(sizeof(LiteralNode), AST_LIT, line);
    if (lit == NULL) {
        return NULL;
    }
    lit->litType = LIT_DOUBLE;
    lit->litValue.float_value = value;
    return (ASTNode *)lit;
}

ASTNode *astNewStringLit(const char *value, int line) {
    LiteralNode *lit = (LiteralNode *)astAlloc(sizeof(LiteralNode), AST_LIT, line);
    if (lit == NULL) {
        return NULL;
    }
    lit->litType = LIT_STRING;
    lit->litValue.string_value = astCopyString(value);
    if (lit->litValue.string_value == NULL) {
        free(lit);
        return NULL;
    }
    return (ASTNode *)lit;
}

ASTNode *astNewBoolLit(bool value, int line) {
    LiteralNode *lit = (LiteralNode *)astAlloc(sizeof(LiteralNode), AST_LIT, line);
    if (lit == NULL) {
        return NULL;
    }
    lit->litType = LIT_BOOLEAN;
    lit->litValue.bool_value = value;
    return (ASTNode *)lit;
}

ASTNode *astNewUnitLit(int line) {
    LiteralNode *lit = (LiteralNode *)astAlloc(sizeof(LiteralNode), AST_LIT, line);
    if (lit == NULL) {
        return NULL;
    }
    lit->litType = LIT_UNIT;
    return (ASTNode *)lit;
}

ASTNode *astNewId(const char *name, int line) {
    IDNode *idNode = (IDNode *)astAlloc(sizeof(IDNode), AST_ID, line);
    if (idNode == NULL) {
        return NULL;
    }
    idNode->name = astCopyString(name);
    if (idNode->name == NULL) {
        free(idNode);
        return NULL;
    }
    return (ASTNode *)idNode;
}

ASTNode *astNewBinary(TokenType op, ASTNode *left, ASTNode *right, int line) {
    BinaryNode *bin = (BinaryNode *)astAlloc(sizeof(BinaryNode), AST_BINARY, line);
    if (bin == NULL) {
        return NULL;
    }
    bin->operator = op;
    bin->left = left;
    bin->right = right;
    return (ASTNode *)bin;
}

ASTNode *astNewUnary(TokenType op, ASTNode *operand, int line) {
    UnaryNode *un = (UnaryNode *)astAlloc(sizeof(UnaryNode), AST_UNARY, line);
    if (un == NULL) {
        return NULL;
    }
    un->operator = op;
    un->right = operand;
    return (ASTNode *)un;
}

void astFree(ASTNode *node) {
    if (node == NULL) {
        return;
    }
    switch (node->kind) {
    case AST_LIT: {
        LiteralNode *lit = (LiteralNode *)node;
        if (lit->litType == LIT_STRING) {
            free(lit->litValue.string_value);
        }
        break;
    }
    case AST_ID:
        free(((IDNode *)node)->name);
        break;
    case AST_BINARY:
        astFree(((BinaryNode *)node)->left);
        astFree(((BinaryNode *)node)->right);
        break;
    case AST_UNARY:
        astFree(((UnaryNode *)node)->right);
        break;
    default:
        break;
    }
    free(node);
}

/**
 * Ladiaci výpis AST stromu (printIndent, tokenOpName, astPrint).
 *
 * Použitie AI: tieto ladiace funkcie vygenerovala AI (Claude Opus 5.5).
 * Prompt 1: "Napíš pomocnú ladiacu funkciu astPrint(const ASTNode *node, int depth, FILE *out)
 *   do src/parser/ast.c pre odsadený výpis výrazových uzlov podľa ast.h."
 */

/* Vypíše odsadenie zodpovedajúce hĺbke vnorenia v strome. */
static void printIndent(int depth, FILE *out) {
    for (int i = 0; i < depth; i++) {
        fprintf(out, "  ");
    }
}

/* Prevedie typ tokenu operátora na textovú reprezentáciu. */
static const char *tokenOpName(TokenType op) {
    switch (op) {
    case TOK_PLUS:
        return "+";
    case TOK_MINUS:
        return "-";
    case TOK_MULTIPLY:
        return "*";
    case TOK_DIVIDE:
        return "/";
    case TOK_MODULO:
        return "%";
    case TOK_CONCAT:
        return "++";
    case TOK_REMOVE:
        return "--";
    case TOK_EQUAL:
        return "==";
    case TOK_NOT_EQUAL:
        return "!=";
    case TOK_LESS:
        return "<";
    case TOK_LESS_EQUAL:
        return "<=";
    case TOK_GREATER:
        return ">";
    case TOK_GREATER_EQUAL:
        return ">=";
    case TOK_NOT:
        return "!";
    case TOK_AND:
        return "&&";
    case TOK_OR:
        return "||";
    default:
        return "?";
    }
}

void astPrint(const ASTNode *node, int depth, FILE *out) {
    // Ochrana pred neplatným výstupným prúdom
    if (out == NULL) {
        return;
    }
    printIndent(depth, out);
    // Prázdny ukazovateľ vypíšeme čitateľne ako (null)
    if (node == NULL) {
        fprintf(out, "(null)\n");
        return;
    }

    switch (node->kind) {
    case AST_LIT: {
        const LiteralNode *lit = (const LiteralNode *)node;
        switch (lit->litType) {
        case LIT_INT:
            fprintf(out, "Int(%lld)\n", lit->litValue.int_value);
            break;
        case LIT_DOUBLE:
            fprintf(out, "Double(%g)\n", lit->litValue.float_value);
            break;
        case LIT_STRING:
            // Ošetrenie chýbajúceho reťazca pri literáli
            fprintf(out, "String(\"%s\")\n",
                    lit->litValue.string_value != NULL ? lit->litValue.string_value : "");
            break;
        case LIT_BOOLEAN:
            fprintf(out, "Bool(%s)\n", lit->litValue.bool_value ? "true" : "false");
            break;
        case LIT_UNIT:
            fprintf(out, "Unit()\n");
            break;
        }
        break;
    }
    case AST_ID: {
        const IDNode *idNode = (const IDNode *)node;
        fprintf(out, "Id(%s)\n", idNode->name != NULL ? idNode->name : "");
        break;
    }
    case AST_BINARY: {
        const BinaryNode *bin = (const BinaryNode *)node;
        fprintf(out, "Binary(%s)\n", tokenOpName(bin->operator));
        // Rekurzívny výpis oboch operandov na ďalšej úrovni
        astPrint(bin->left, depth + 1, out);
        astPrint(bin->right, depth + 1, out);
        break;
    }
    case AST_UNARY: {
        const UnaryNode *un = (const UnaryNode *)node;
        fprintf(out, "Unary(%s)\n", tokenOpName(un->operator));
        astPrint(un->right, depth + 1, out);
        break;
    }
    default:
        fprintf(out, "?\n");
        break;
    }
}

bool astAppend(ASTNode ***items, size_t *count, ASTNode *node) {
    if (items == NULL || count == NULL) {
        return false;
    }
    // zvacsi pole o jeden prvok
    *items = realloc(*items, (*count + 1) * sizeof(ASTNode *));
    (*count)++;
    if (*items == NULL) {
        return false;
    }
    (*items)[*count - 1] = node;
    return true;
}

ASTNode *astNewBlock(int line) {
    return astAlloc(sizeof(BlockNode), AST_BLOCK, line);
}

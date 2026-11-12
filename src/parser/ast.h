/**
 * @file ast.h
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

#pragma once
#include "token.h"
#include "types.h"

/**
 * @brief štruktúra pre informáciu o pozícií výrazu v kóde
 */
typedef struct{
    int line;
    int col;
    int offset;
}Position;

/**
 * @brief štruktúra, ktorá hovorí, kde začína a končí výraz
 */
typedef struct{
    Position start;
    Position end;
}Span;

/**
 * @brief štruktúra očíslovania typov uzlov
 */
typedef enum{
    AST_LIT, /** literál */
    AST_ID, /** identifikátor */
    AST_BINARY, /** binárna operácia ('+', '-',...) */
    AST_UNARY, /** unárna operácia (-x, negácia,...) */
    AST_CALL, /** volanie funkcie */
    AST_IF, /** podmienka IF */
    AST_WHILE, /** podmienka WHILE */
    AST_ /*TO DO: pridať ďalšie mega pls*/
}ASTNodeKind;

/**
 * @brief štruktúra očíslovania typov literálov
 */
typedef enum{
    LIT_INT, /** int */
    LIT_DOUBLE, /** double */
    LIT_STRING, /** char/char * */
    LIT_BOOLEAN, /** bool */
    LIT_UNIT /** void */
}LiteralType;
/**
 * @brief štruktúra, ako bude vyzerať samotný uzol v AST strome
 */
typedef struct{
    ASTNodeKind kind; 
    Span span;
    Type *type;
}ASTNode;

/**
 * @brief uzol literálov
 */
typedef struct{
    ASTNode node;
    LiteralType type;
    union {
        long long int_value;
        double float_value;
        char *string_value;
        bool bool_value;
    }litValue;
}ASTLiteralNode;

/**
 * @brief uzol identifikátorov
 */
typedef struct{
    ASTNode node;
    char *name;
}IDNode;

/**
 * @brief uzol binárnych operácíi
 */
typedef struct {
    ASTNode node;
    TokenType operator;
    ASTNode *left;
    ASTNode *right;
}BinaryNode;

/**
 * @brief uzol unárnych operácíi
 */
typedef struct{
    ASTNode node;
    TokenType operator;
    ASTNode *right;
}UnaryNode;

/**
 * @brief uzol volaní funkcíi
 */
typedef struct{
    ASTNode node;
    ASTNode *caller; /** ten, kto funkciu volá */
    ASTNode **args; /** argumenty funkcie */
    size_t argsCount; /** počet argumentov */
}CallNode;


/**
 * @brief uzol if-podmienok
 */
typedef struct{
    ASTNode node;
    ASTNode *condition; /** podmienka */
    ASTNode *conditionTrue; /** ak je splnená */
    ASTNode *conditionFalse; /** ak nie je splnená */
}IfNode;

/**
 * @brief uzol while-podmienok
 */
typedef struct{
    ASTNode node;
    ASTNode *condition;
    ASTNode *conditionTrue;
}WhileNode;




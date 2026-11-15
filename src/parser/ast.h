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
#include <stdbool.h>
#include <stddef.h>

#include "token.h"
#include "types.h"

struct Symbol; // definicia v symtable.h

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
    AST_RETURN, /** return; */
    AST_BLOCK, /** blok */
    AST_ASSIGN, /** priradenie */
    AST_VAR_DEF, /** definícia premenných */
    AST_EXPR_STATEMENT, /** údaj o výraze */
    AST_FUNCTION_DEF, /** definícia funkcie */
    AST_PROGRAM /** program */
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
    int line; // riadok
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
}LiteralNode;

/**
 * @brief uzol identifikátorov
 */
typedef struct{
    ASTNode node;
    char *name;
    struct Symbol *symbol;
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

/**
 * @brief uzol bloku
 */
typedef struct{
    ASTNode node;
    ASTNode **commandOrder; /** pole príkazov v poradí bloku*/
    size_t commandOrderCount; /** počet príkazov */
}BlockNode;

/**
 * @brief štruktúra parametrov
 */
typedef struct{
    char *name;
    Type *type;
    int line;
    struct Symbol *symbol;
}Param;

/**
 * @brief štruktúra zoznamu parametrov
 */
typedef struct{
    Param *params;
    size_t params_count;
}ParamList;

/**
 * @brief uzol definícii premenných var & val
 */
typedef struct{
    ASTNode node;
    ASTNode *init;
    char *name;
    bool isMutable; /** var->true, val->false */
    Type *declaredType;
    struct Symbol *symbol;
}VarOrValNode;

/**
 * @brief uzol priradenia k premennej
 */
typedef struct{
    ASTNode node;
    char *name;
    ASTNode *value;
    struct Symbol *symbol;
}AssignNode;

/**
 * @brief uzol returnov
 */
typedef struct{
    ASTNode node;
    ASTNode *returnValue;
}ReturnNode;

/**
 * @brief uzol samostatných príkazov
 */
typedef struct{
    ASTNode node;
    ASTNode *expr;
}LonelyExprNode;

/**
 * @brief uzol deficícíi funkcíi
 */
typedef struct{
    ASTNode node;
    char *name;
    ParamList *paramList; /** zoznam parametrov */
    size_t paramListCount;
    Type *returnValue;
    bool isMain; /** či je funkcia @main */
    ASTNode *body; /** telo funkcie */
    struct Symbol *symbol;
}DefFuncNode;

/**
 * @brief uzol koreňa celého programu
 */
typedef struct{
    ASTNode node;
    ASTNode **funcDefinitions;
    size_t funcDefinitionsCount;
}ProgramNode;
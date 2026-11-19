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

// (S) - vyplni semantika, parser necha NULL

/**
 * @brief štruktúra očíslovania typov uzlov
 */
typedef enum {
    AST_LIT,            /** literál */
    AST_ID,             /** identifikátor */
    AST_BINARY,         /** binárna operácia ('+', '-',...) */
    AST_UNARY,          /** unárna operácia (-x, negácia,...) */
    AST_CALL,           /** volanie funkcie */
    AST_IF,             /** podmienka IF */
    AST_WHILE,          /** podmienka WHILE */
    AST_RETURN,         /** return; */
    AST_BLOCK,          /** blok */
    AST_ASSIGN,         /** priradenie */
    AST_VAR_DEF,        /** definícia premenných */
    AST_EXPR_STATEMENT, /** údaj o výraze */
    AST_FUNCTION_DEF,   /** definícia funkcie */
    AST_PROGRAM,        /** program */
    AST_EXTENSION,      // extension
    AST_CLASS_DEF,      // case class
    AST_TYPE_ALIAS,     // alias typu
    AST_FOR,            // cyklus FOR
    AST_MEMBER,         // pristup cez bodku
    AST_MATCH,          // match
    AST_CASE,           // vetva case
    AST_LAMBDA,         // lambda
    AST_RANGE,          // rozsah
    AST_LIST_LIT,       // zoznam
    AST_INTERP,         // interpolovany retazec
    AST_CONVERT         // prevod Int na Double
} ASTNodeKind;

/**
 * @brief štruktúra očíslovania typov literálov
 */
typedef enum {
    LIT_INT,     /** int */
    LIT_DOUBLE,  /** double */
    LIT_STRING,  /** char/char * */
    LIT_BOOLEAN, /** bool */
    LIT_UNIT     /** void */
} LiteralType;

/**
 * @brief druh volania
 */
typedef enum {
    CALL_UNRESOLVED,  // nevyriesene
    CALL_FUNC,        // nasa funkcia
    CALL_BUILTIN,     // vstavana funkcia
    CALL_VALUE,       // lambda v premennej
    CALL_CONSTRUCTOR, // konstruktor
    CALL_LIST_INDEX,  // index zoznamu
    CALL_EXT_METHOD,  // rozsirujuca metoda
    CALL_LIST_METHOD  // metoda zoznamu
} CallKind;

/**
 * @brief druh identifikatora
 */
typedef enum {
    ID_UNRESOLVED, // nevyriesene
    ID_VAR,        // premenna alebo parameter
    ID_FUNC_CALL,  // volanie funkcie bez zatvoriek
    ID_FUNC_REF    // funkcia ako hodnota
} IdKind;

/**
 * @brief druh pristupu cez bodku
 */
typedef enum {
    MEM_UNRESOLVED, // nevyriesene
    MEM_BUILTIN,    // vstavany clen
    MEM_FIELD,      // atribut triedy
    MEM_EXT_METHOD, // rozsirujuca metoda
    MEM_CALLEE      // volana metoda
} MemberKind;

/**
 * @brief druh vzoru vo vetve case
 */
typedef enum {
    PAT_WILDCARD,   // case _
    PAT_LITERAL,    // literal
    PAT_TYPED,      // typ
    PAT_CONSTRUCTOR // konstruktor
} PatternKind;
/**
 * @brief štruktúra, ako bude vyzerať samotný uzol v AST strome
 */
typedef struct {
    ASTNodeKind kind;
    int line; // riadok
    Type *type;
} ASTNode;

/**
 * @brief uzol literálov
 */
typedef struct {
    ASTNode node;
    LiteralType litType;
    union {
        long long int_value;
        double float_value;
        char *string_value;
        bool bool_value;
    } litValue;
} LiteralNode;

/**
 * @brief uzol identifikátorov
 */
typedef struct {
    ASTNode node;
    char *name;
    IdKind idKind; // (S)
    struct Symbol *symbol;
} IDNode;

/**
 * @brief uzol binárnych operácíi
 */
typedef struct {
    ASTNode node;
    TokenType operator;
    ASTNode *left;
    ASTNode *right;
} BinaryNode;

/**
 * @brief uzol unárnych operácíi
 */
typedef struct {
    ASTNode node;
    TokenType operator;
    ASTNode *right;
} UnaryNode;

/**
 * @brief uzol volaní funkcíi
 */
typedef struct {
    ASTNode node;
    ASTNode *caller;       /** ten, kto funkciu volá */
    ASTNode **args;        /** argumenty funkcie */
    size_t argsCount;      /** počet argumentov */
    CallKind callKind;     // (S)
    struct Symbol *symbol; // (S)
} CallNode;

/**
 * @brief uzol if-podmienok
 */
typedef struct {
    ASTNode node;
    ASTNode *condition;  /** podmienka */
    ASTNode *thenBranch; /** ak je splnená */
    ASTNode *elseBranch; /** ak nie je splnená */
} IfNode;

/**
 * @brief uzol while-podmienok
 */
typedef struct {
    ASTNode node;
    ASTNode *condition;
    ASTNode *body;
} WhileNode;

/**
 * @brief uzol bloku
 */
typedef struct {
    ASTNode node;
    ASTNode **commandOrder;   /** pole príkazov v poradí bloku*/
    size_t commandOrderCount; /** počet príkazov */
} BlockNode;

/**
 * @brief štruktúra parametrov
 */
typedef struct {
    char *name;
    Type *type;
    int line;
    struct Symbol *symbol;
} Param;

/**
 * @brief štruktúra zoznamu parametrov
 */
typedef struct {
    Param *params;
    size_t params_count;
} ParamList;

/**
 * @brief uzol definícii premenných var & val
 */
typedef struct {
    ASTNode node;
    ASTNode *init;
    char *name;
    bool isMutable; /** var->true, val->false */
    Type *declaredType;
    struct Symbol *symbol;
} VarOrValNode;

/**
 * @brief uzol priradenia k premennej
 */
typedef struct {
    ASTNode node;
    char *name;
    ASTNode *value;
    struct Symbol *symbol;
} AssignNode;

/**
 * @brief uzol returnov
 */
typedef struct {
    ASTNode node;
    ASTNode *returnValue; // NULL pri return bez hodnoty
} ReturnNode;

/**
 * @brief uzol samostatných príkazov
 */
typedef struct {
    ASTNode node;
    ASTNode *expr;
} LonelyExprNode;

/**
 * @brief uzol deficícíi funkcíi
 */
typedef struct {
    ASTNode node;
    char *name;
    ParamList *paramList;  /** zoznam parametrov */
    size_t paramListCount; // 0 = def f: Int, 1 = def f()
    Type *returnType;
    bool isMain;   /** či je funkcia @main */
    ASTNode *body; /** telo funkcie */
    struct Symbol *symbol;
    struct Symbol **locals; // (S) lokalne premenne
    size_t localsCount;     // (S)
} DefFuncNode;

/**
 * @brief uzol koreňa celého programu
 */
typedef struct {
    ASTNode node;
    ASTNode **items; // definicie v poradi zo zdrojaku
    size_t itemsCount;
} ProgramNode;

/**
 * @brief uzol extension
 */
typedef struct {
    ASTNode node;
    Param receiver;    // prijemca
    ASTNode **methods; // metody
    size_t methodsCount;
} ExtensionNode;

/**
 * @brief uzol case class
 */
typedef struct {
    ASTNode node;
    char *name;
    ParamList fields;      // atributy
    struct Symbol *symbol; // (S)
} ClassDefNode;

/**
 * @brief uzol aliasu typu
 */
typedef struct {
    ASTNode node;
    char *name;
    Type *aliased;         // typ aliasu
    struct Symbol *symbol; // (S)
} TypeAliasNode;

/**
 * @brief uzol for cyklu
 */
typedef struct {
    ASTNode node;
    char *varName;    // riadiaca premenna
    ASTNode *source;  // rozsah alebo zoznam
    ASTNode **guards; // straze
    size_t guardsCount;
    ASTNode *body;
    struct Symbol *symbol; // (S)
} ForNode;

/**
 * @brief uzol pristupu cez bodku
 */
typedef struct {
    ASTNode node;
    ASTNode *object;       // vyraz pred bodkou
    char *name;            // meno za bodkou
    MemberKind memberKind; // (S)
    struct Symbol *symbol; // (S)
} MemberNode;

/**
 * @brief uzol match
 */
typedef struct {
    ASTNode node;
    ASTNode *subject; // porovnavany vyraz
    ASTNode **cases;  // vetvy
    size_t casesCount;
} MatchNode;

/**
 * @brief uzol vetvy case
 */
typedef struct {
    ASTNode node;
    PatternKind patternKind;
    ASTNode *literal;           // PAT_LITERAL
    Param binder;               // PAT_TYPED
    char *className;            // PAT_CONSTRUCTOR
    ParamList ctorBinders;      // PAT_CONSTRUCTOR
    struct Symbol *classSymbol; // (S)
    ASTNode *guard;             // straz alebo NULL
    ASTNode *body;              // telo
} CaseNode;

/**
 * @brief uzol lambdy
 */
typedef struct {
    ASTNode node;
    ParamList params;       // parametre
    ASTNode *body;          // telo
    struct Symbol *symbol;  // (S)
    struct Symbol **locals; // (S) lokalne premenne
    size_t localsCount;     // (S)
} LambdaNode;

/**
 * @brief uzol rozsahu
 */
typedef struct {
    ASTNode node;
    ASTNode *from;
    ASTNode *to;
    ASTNode *step;  // NULL = 1
    bool inclusive; // to->true, until->false
} RangeNode;

/**
 * @brief uzol zoznamu
 */
typedef struct {
    ASTNode node;
    ASTNode **elems; // prvky
    size_t elemsCount;
} ListLitNode;

/**
 * @brief uzol interpolovaneho retazca
 */
typedef struct {
    ASTNode node;
    ASTNode **parts; // casti retazca
    size_t partsCount;
} InterpNode;

/**
 * @brief uzol prevodu Int na Double
 */
typedef struct {
    ASTNode node;
    ASTNode *operand; // vyraz Int
} ConvertNode;
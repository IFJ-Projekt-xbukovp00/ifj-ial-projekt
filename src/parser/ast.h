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
#include <stdio.h>

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

/**
 * @brief alokuje a vynuluje AST uzol daneho druhu
 * @param size velkost struktury uzla v bajtoch
 * @param kind druh uzla AST
 * @param line riadok vo zdrojovom kode
 * @return ukazovatel na novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astAlloc(size_t size, ASTNodeKind kind, int line);

/**
 * @brief vytvori dynamicky alokovanu kopiu retazca
 * @param text zdrojovy retazec
 * @return kopia retazca, alebo NULL ak text je NULL alebo pri chybe alokacie
 */
char *astCopyString(const char *text);

/**
 * @brief vytvori uzol celociselneho literalu
 * @param value hodnota literalu
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewIntLit(long long value, int line);

/**
 * @brief vytvori uzol desatinneho literalu
 * @param value hodnota literalu
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewDoubleLit(double value, int line);

/**
 * @brief vytvori uzol retazcoveho literalu s kopiou textu
 * @param value text literalu
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewStringLit(const char *value, int line);

/**
 * @brief vytvori uzol logickeho literalu
 * @param value pravdivostna hodnota
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewBoolLit(bool value, int line);

/**
 * @brief vytvori uzol unit literalu ()
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewUnitLit(int line);

/**
 * @brief vytvori uzol identifikatora s kopiou mena
 * @param name meno identifikatora
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewId(const char *name, int line);

/**
 * @brief vytvori uzol binarnej operacie
 * @param op operator (TOK_PLUS, TOK_LESS, ...)
 * @param left lavy operand (pri chybe alokacie sa neuvolni)
 * @param right pravy operand (pri chybe alokacie sa neuvolni)
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewBinary(TokenType op, ASTNode *left, ASTNode *right, int line);

/**
 * @brief vytvori uzol unarnej operacie
 * @param op operator (TOK_MINUS, TOK_NOT, ...)
 * @param operand operand (pri chybe alokacie sa neuvolni)
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewUnary(TokenType op, ASTNode *operand, int line);

/**
 * @brief rekurzivne uvolni AST uzol a jeho poduzly (Type * a Symbol * neuvolnuje)
 * @param node uzol na uvolnenie (NULL je povoleny)
 */
void astFree(ASTNode *node);

/**
 * @brief ladiaci odsadeny vypis AST stromu do suboru alebo streamu
 * @param node korenovy uzol na vypis (NULL vypise "(null)")
 * @param depth aktualna hlbka odsadenia
 * @param out vystupny stream
 */
void astPrint(const ASTNode *node, int depth, FILE *out);

/**
 * @brief prida uzol na koniec dynamickeho pola ukazovatelov
 * @param items adresa pola ukazovatelov na AST uzly
 * @param count adresa pocitadla prvkov v poli
 * @param node vkladany uzol
 * @return true pri uspechu, false pri neplatnych parametroch alebo chybe alokacie
 */
bool astAppend(ASTNode ***items, size_t *count, ASTNode *node);

/**
 * @brief vytvori uzol bloku (prikazy sa pridavaju cez astAppend)
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewBlock(int line);

/**
 * @brief vytvori uzol definicie premennej var alebo val
 * @param name meno premennej
 * @param isMutable true pre var, false pre val
 * @param declaredType deklarovany typ alebo NULL pri odvodeni
 * @param init inicializacny vyraz
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewVarDef(const char *name, bool isMutable, Type *declaredType, ASTNode *init,
                      int line);

/**
 * @brief vytvori uzol priradenia do premennej
 * @param name meno premennej
 * @param value priradovany vyraz
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewAssign(const char *name, ASTNode *value, int line);

/**
 * @brief vytvori uzol prikazu return
 * @param value navratovy vyraz alebo NULL pre return bez hodnoty
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewReturn(ASTNode *value, int line);

/**
 * @brief vytvori uzol samostatneho vyrazoveho prikazu
 * @param expr vyraz
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewExprStatement(ASTNode *expr, int line);

/**
 * @brief vytvori uzol podmienky if
 * @param cond podmienka
 * @param thenB vetva then
 * @param elseB vetva else alebo NULL
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewIf(ASTNode *cond, ASTNode *thenB, ASTNode *elseB, int line);

/**
 * @brief vytvori uzol cyklu while
 * @param cond podmienka
 * @param body telo cyklu
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewWhile(ASTNode *cond, ASTNode *body, int line);

/**
 * @brief vytvori uzol volania funkcie (argumenty cez astAppend, vnorene volania cez caller)
 * @param caller volany vyraz alebo predchadzajuce volanie pri f(1)(2)
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewCall(ASTNode *caller, int line);

/**
 * @brief prida parameter do zoznamu parametrov (spravi kopiu mena)
 * @param list zoznam parametrov
 * @param name meno parametra
 * @param type typ parametra (neuvolnuje sa v AST)
 * @param line riadok vo zdrojovom kode
 * @return true pri uspechu, false pri chybe alokacie alebo neplatnych parametroch
 */
bool astParamListAdd(ParamList *list, const char *name, Type *type, int line);

/** @brief uvolni mena a pole parametrov v zozname (Type * a Symbol * neuvolnuje) */
void astParamListFree(ParamList *list);

/**
 * @brief vytvori uzol definicie funkcie (parametre cez astFuncAddParamList)
 * @param name meno funkcie
 * @param isMain true pre @main funkciu
 * @param line riadok vo zdrojovom kode
 * @return novy uzol, alebo NULL pri chybe alokacie
 */
ASTNode *astNewFuncDef(const char *name, bool isMain, int line);

/**
 * @brief prida prazdny zoznam parametrov k funkcii (0 pre def f, 1 pre def f())
 * @param func uzol funkcie
 * @return ukazovatel na novy ParamList, alebo NULL pri chybe alokacie
 */
ParamList *astFuncAddParamList(DefFuncNode *func);

/** @brief vytvori korenovy uzol programu (definicie sa pridavaju cez astAppend) */
ASTNode *astNewProgram(int line);

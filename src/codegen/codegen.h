/**
 * @file codegen.h
 * @brief Generátor cieľového kódu IFJcode26
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

#include "ast.h"
#include "dynstr.h"
#include "symtable.h"

#include <stdarg.h>
#include <stdio.h>

/**
 * @brief pomocne funkcie za behu, napisane priamo v IFJcode26
 *
 * Vypisu sa raz na konci programu, ale len tie, ktore sa niekde pouzili.
 */
typedef enum {
    HELPER_SUBSTR, // $subStr
    HELPER_ORD,    // $ord
    HELPER_COUNT   // pocet pomocnych funkcii, musi byt posledny
} HelperKind;

/**
 * @brief kontext generatora, prenasa sa medzi vsetkymi funkciami generatora
 */
typedef struct {
    FILE *out;                     // vystupny subor
    DynStr declarations;           // DEFVAR vsetkych premennych, vypisu sa pred kodom
    DynStr code;                   // vygenerovany kod, vypise sa az na konci
    int labelCounter;              // pocitadlo navesti L$N
    int varCounter;                // pocitadlo premennych, cislo N v mene x$N
    int hiddenCounter;             // pocitadlo skrytych premennych %hN
    DefFuncNode *function;         // prave generovana funkcia
    int error;                     // ERR_INTERNAL ak zlyhala alokacia
    bool helperUsed[HELPER_COUNT]; // ktore pomocne funkcie sa maju vypisat
} CodeGen;

/**
 * @brief funkcia na oznacenie pomocnej funkcie ako pouzitej, vypise sa na konci programu
 * @param gen je kontext generatora
 * @param helper je pouzita pomocna funkcia
 */
void useHelper(CodeGen *gen, HelperKind helper);

/**
 * @brief funkcia na vypis jednej pomocnej funkcie
 * @param gen je kontext generatora
 * @param helper je vypisovana pomocna funkcia
 */
void writeHelper(CodeGen *gen, HelperKind helper);

/**
 * @brief funkcia na vypis vsetkych pouzitych pomocnych funkcii na koniec programu
 * @param gen je kontext generatora
 */
void writeHelpers(CodeGen *gen);

/**
 * @brief funkcia na vygenerovanie programu v jazyku IFJcode26
 * @param prog je koren AST, ktory uz presiel semantickou analyzou
 * @param out je vystupny subor, kam sa zapisuje kod
 * @return 0 ak generovanie prebehlo v poriadku, inak kod chyby
 */
int codegenGenerate(ProgramNode *prog, FILE *out);

/**
 * @brief funkcia na zapis jednej casti kodu do bufferu, formatuje sa ako pri printf
 * @param gen je kontext generatora
 * @param format je formatovaci retazec
 */
void emit(CodeGen *gen, const char *format, ...);

/**
 * @brief funkcia na zapis formatovaneho textu na koniec bufferu
 * @param gen je kontext generatora
 * @param target je buffer, do ktoreho sa zapisuje
 * @param format je formatovaci retazec
 * @param args su argumenty formatu
 */
void appendFormat(CodeGen *gen, DynStr *target, const char *format, va_list args);

/**
 * @brief funkcia na zapis definicie premennej (DEFVAR), ktora sa vypise pred kodom
 * @param gen je kontext generatora
 * @param format je formatovaci retazec
 */
void emitDeclaration(CodeGen *gen, const char *format, ...);

/**
 * @brief funkcia na vypis uvodneho riadku programu priamo do vystupu
 * @param gen je kontext generatora
 */
void emitHeader(CodeGen *gen);

/**
 * @brief funkcia na definovanie pomocnych premennych %t0 az %t4 a %trash
 * @param gen je kontext generatora
 */
void emitHelperVars(CodeGen *gen);

/**
 * @brief funkcia na vypis retazcovej konstanty, specialne znaky sa escapuju ako \ddd
 * @param gen je kontext generatora
 * @param text je vypisovany retazec
 */
void emitString(CodeGen *gen, const char *text);

/**
 * @brief funkcia na vypis premennej v tvare LF@meno$N
 * @param gen je kontext generatora
 * @param name je meno premennej zo zdrojaku
 * @param symbol je symbol premennej, v nom je jej cislo
 */
void emitVar(CodeGen *gen, const char *name, Symbol *symbol);

/**
 * @brief funkcia na vypis literalu ako konstanty (int@5, float@0x1p+0, ...)
 * @param gen je kontext generatora
 * @param literal je vypisovany literal
 */
void emitLiteral(CodeGen *gen, LiteralNode *literal);

/**
 * @brief funkcia na inicializaciu kontextu generatora
 * @param gen je inicializovany kontext
 * @param out je vystupny subor
 */
void initCodeGen(CodeGen *gen, FILE *out);

/**
 * @brief funkcia na ziskanie noveho cisla navestia, navestie sa potom pise ako L$N
 * @param gen je kontext generatora
 * @return nove cislo navestia
 */
int newLabel(CodeGen *gen);

/**
 * @brief funkcia na uvolnenie pamate kontextu generatora
 * @param gen je uvolnovany kontext
 */
void freeCodeGen(CodeGen *gen);

/**
 * @brief funkcia na zistenie skutocneho druhu typu uzla (alias sa preskoci)
 * @param node je uzol vyrazu, ktory uz presiel semantikou
 * @return druh typu, pri chybajucom type TY_UNIT
 */
TypeKind getKind(ASTNode *node);

/**
 * @brief funkcia na najdenie funkcie @main medzi definiciami programu
 * @param prog je koren AST
 * @return uzol funkcie main, alebo NULL ak v programe nie je
 */
DefFuncNode *findMain(ProgramNode *prog);

/**
 * @brief funkcia na vygenerovanie vstavanej funkcie print
 * @param gen je kontext generatora
 * @param call je uzol volania print
 */
void genPrint(CodeGen *gen, CallNode *call);

/**
 * @brief funkcia na vygenerovanie vstavanej funkcie readLine
 * @param gen je kontext generatora
 */
void genReadLine(CodeGen *gen);

/**
 * @brief funkcia na vygenerovanie vstavanej funkcie readInt, readDouble alebo readBoolean
 * @param gen je kontext generatora
 * @param typeName je typ pre instrukciu READ (int, float alebo bool)
 * @param zero je konstanta, ktora sa vrati pri chybnom vstupe (0 alebo false)
 */
void genReadNumber(CodeGen *gen, const char *typeName, const char *zero);

/**
 * @brief funkcia na prevod hodnoty na vrchu zasobnika na retazec podla jej statickeho typu
 * @param gen je kontext generatora
 * @param type je staticky typ hodnoty
 */
void genValueToStr(CodeGen *gen, Type *type);

/**
 * @brief funkcia na vygenerovanie vstavanej funkcie toStr
 * @param gen je kontext generatora
 * @param call je uzol volania toStr
 */
void genToStr(CodeGen *gen, CallNode *call);

/**
 * @brief funkcia na vygenerovanie volania vstavanej funkcie ord (pomocna funkcia $ord)
 * @param gen je kontext generatora
 * @param call je uzol volania ord
 */
void genOrd(CodeGen *gen, CallNode *call);

/**
 * @brief funkcia na vygenerovanie tela pomocnej funkcie $ord
 * @param gen je kontext generatora
 */
void genOrdHelper(CodeGen *gen);

/**
 * @brief funkcia na podmieneny skok, ak plati porovnanie dvoch hodnot
 * @param gen je kontext generatora
 * @param left je lavy operand (premenna alebo konstanta)
 * @param operator je zasobnikova instrukcia porovnania (LTS, GTS, EQS)
 * @param right je pravy operand
 * @param label je cislo navestia, kam sa skace
 */
void genJumpIfTrue(CodeGen *gen, const char *left, const char *operator, const char * right,
                   int label);

/**
 * @brief funkcia na vygenerovanie volania vstavanej funkcie subStr (pomocna funkcia $subStr)
 * @param gen je kontext generatora
 * @param call je uzol volania subStr
 */
void genSubStr(CodeGen *gen, CallNode *call);

/**
 * @brief funkcia na vygenerovanie tela pomocnej funkcie $subStr
 * @param gen je kontext generatora
 */
void genSubStrHelper(CodeGen *gen);

/**
 * @brief funkcia na vyhodnotenie argumentov volania zlava doprava na zasobnik
 * @param gen je kontext generatora
 * @param call je uzol volania
 */
void genArgs(CodeGen *gen, CallNode *call);

/**
 * @brief funkcia na vygenerovanie volania funkcie definovanej v programe
 * @param gen je kontext generatora
 * @param call je uzol volania
 */
void genFunctionCall(CodeGen *gen, CallNode *call);

/**
 * @brief funkcia na vygenerovanie volania funkcie, vysledok ostane na zasobniku
 * @param gen je kontext generatora
 * @param call je uzol volania
 */
void genCall(CodeGen *gen, CallNode *call);

/**
 * @brief funkcia na vygenerovanie volania vstavanej funkcie podla mena
 * @param gen je kontext generatora
 * @param call je uzol volania
 * @param name je meno vstavanej funkcie
 * @return true ak sa vstavana funkcia nasla a vygenerovala
 */
bool genBuiltinCall(CodeGen *gen, CallNode *call, const char *name);

/**
 * @brief funkcia na vygenerovanie skratoveho &&
 * @param gen je kontext generatora
 * @param binary je uzol operacie &&
 */
void genAnd(CodeGen *gen, BinaryNode *binary);

/**
 * @brief funkcia na vygenerovanie skratoveho ||
 * @param gen je kontext generatora
 * @param binary je uzol operacie ||
 */
void genOr(CodeGen *gen, BinaryNode *binary);

/**
 * @brief funkcia na vygenerovanie pristupu cez bodku (x.toString, ...)
 * @param gen je kontext generatora
 * @param member je uzol pristupu cez bodku
 */
void genMember(CodeGen *gen, MemberNode *member);

/**
 * @brief funkcia na vygenerovanie binarnej operacie
 * @param gen je kontext generatora
 * @param binary je uzol binarnej operacie
 */
void genBinary(CodeGen *gen, BinaryNode *binary);

/**
 * @brief funkcia na vygenerovanie unarnej operacie
 * @param gen je kontext generatora
 * @param unary je uzol unarnej operacie
 */
void genUnary(CodeGen *gen, UnaryNode *unary);

/**
 * @brief funkcia na vygenerovanie vyrazu, vysledok vyrazu ostane na datovom zasobniku
 * @param gen je kontext generatora
 * @param node je generovany vyraz
 */
void genExpr(CodeGen *gen, ASTNode *node);

/**
 * @brief funkcia na pridelenie cisla lokalnej premennej a jej DEFVAR na zaciatok funkcie
 * @param gen je kontext generatora
 * @param name je meno premennej zo zdrojaku
 * @param symbol je symbol premennej
 */
void defineLocal(CodeGen *gen, const char *name, Symbol *symbol);

/**
 * @brief funkcia na vytvorenie skrytej lokalnej premennej LF@%hN (napr. mez cyklu for)
 * @param gen je kontext generatora
 * @return cislo N novej skrytej premennej
 */
int newHiddenVar(CodeGen *gen);

/**
 * @brief funkcia na vygenerovanie definicie premennej (val aj var)
 * @param gen je kontext generatora
 * @param varDef je uzol definicie
 */
void genVarDef(CodeGen *gen, VarOrValNode *varDef);

/**
 * @brief funkcia na vygenerovanie priradenia do premennej
 * @param gen je kontext generatora
 * @param assign je uzol priradenia
 */
void genAssign(CodeGen *gen, AssignNode *assign);

/**
 * @brief funkcia na vyhodnotenie podmienky a skok, ak je nepravdiva
 * @param gen je kontext generatora
 * @param condition je podmienka (vyraz typu Boolean)
 * @param label je cislo navestia, kam sa skace pri false
 */
void genJumpIfFalse(CodeGen *gen, ASTNode *condition, int label);

/**
 * @brief funkcia na vygenerovanie vetvy podmienky
 * @param gen je kontext generatora
 * @param branch je vetva (blok alebo jeden prikaz)
 * @param valueUsed je true, ak ma vetva nechat hodnotu na zasobniku
 */
void genBranch(CodeGen *gen, ASTNode *branch, bool valueUsed);

/**
 * @brief funkcia na vygenerovanie jedneho if v retazci else if
 * @param gen je kontext generatora
 * @param ifNode je uzol podmienky
 * @param endLabel je cislo spolocneho navestia za celym retazcom
 * @param valueUsed je true, ak sa hodnota if pouzije
 */
void genIfChain(CodeGen *gen, IfNode *ifNode, int endLabel, bool valueUsed);

/**
 * @brief funkcia na vygenerovanie podmienky if (prikaz aj vyraz)
 * @param gen je kontext generatora
 * @param ifNode je uzol podmienky
 * @param valueUsed je true, ak ma if nechat hodnotu na zasobniku
 */
void genIf(CodeGen *gen, IfNode *ifNode, bool valueUsed);

/**
 * @brief funkcia na vygenerovanie cyklu while
 * @param gen je kontext generatora
 * @param whileNode je uzol cyklu
 */
void genWhile(CodeGen *gen, WhileNode *whileNode);

/**
 * @brief funkcia na vygenerovanie kontroly konca cyklu for pre jeden smer
 * @param gen je kontext generatora
 * @param forNode je uzol cyklu
 * @param endVar je cislo skrytej premennej s hornou mezou
 * @param upward je true pre kladny krok
 * @param endLabel je cislo navestia za cyklom
 */
void genForCheck(CodeGen *gen, ForNode *forNode, int endVar, bool upward, int endLabel);

/**
 * @brief funkcia na vygenerovanie cyklu for cez rozsah
 * @param gen je kontext generatora
 * @param forNode je uzol cyklu
 */
void genFor(CodeGen *gen, ForNode *forNode);

/**
 * @brief funkcia na vygenerovanie prikazu return, skace na koniec funkcie f$meno$end
 * @param gen je kontext generatora
 * @param returnNode je uzol return
 */
void genReturn(CodeGen *gen, ReturnNode *returnNode);

/**
 * @brief funkcia na vygenerovanie vsetkych prikazov bloku
 * @param gen je kontext generatora
 * @param block je generovany blok
 */
void genBlock(CodeGen *gen, BlockNode *block);

/**
 * @brief funkcia na vygenerovanie prikazu, ktoreho hodnota sa pouzije (napr. telo funkcie)
 *
 * Hodnota bloku je hodnota jeho posledneho prikazu, ostane na datovom zasobniku.
 * @param gen je kontext generatora
 * @param node je generovany prikaz
 */
void genValue(CodeGen *gen, ASTNode *node);

/**
 * @brief funkcia na vygenerovanie jedneho prikazu
 * @param gen je kontext generatora
 * @param node je generovany prikaz (moze byt NULL)
 */
void genStatement(CodeGen *gen, ASTNode *node);

/**
 * @brief funkcia na vypis definicii premennych a kodu z bufferov, buffre sa potom vyprazdnia
 * @param gen je kontext generatora
 */
void writeBuffers(CodeGen *gen);

/**
 * @brief funkcia na definovanie parametrov funkcie a ich naplnenie z datoveho zasobnika
 * @param gen je kontext generatora
 * @param function je uzol definicie funkcie
 */
void genParams(CodeGen *gen, DefFuncNode *function);

/**
 * @brief funkcia na vygenerovanie definicie funkcie (navestie f$meno)
 * @param gen je kontext generatora
 * @param function je uzol definicie funkcie
 */
void genFunction(CodeGen *gen, DefFuncNode *function);

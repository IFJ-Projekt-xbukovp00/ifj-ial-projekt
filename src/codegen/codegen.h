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

#include <stdio.h>

/**
 * @brief kontext generatora, prenasa sa medzi vsetkymi funkciami generatora
 */
typedef struct {
    FILE *out;        // vystupny subor
    int labelCounter; // pocitadlo navesti L$N
} CodeGen;

/**
 * @brief funkcia na vygenerovanie programu v jazyku IFJcode26
 * @param prog je koren AST, ktory uz presiel semantickou analyzou
 * @param out je vystupny subor, kam sa zapisuje kod
 * @return 0 ak generovanie prebehlo v poriadku, inak kod chyby
 */
int codegenGenerate(ProgramNode *prog, FILE *out);

/**
 * @brief funkcia na vypis jednej casti kodu, formatuje sa ako pri printf
 * @param gen je kontext generatora
 * @param format je formatovaci retazec
 */
void emit(CodeGen *gen, const char *format, ...);

/**
 * @brief funkcia na vypis uvodneho riadku programu
 * @param gen je kontext generatora
 */
void emitHeader(CodeGen *gen);

/**
 * @brief funkcia na definovanie pomocnych premennych %t0 a %t1
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
 * @brief funkcia na vygenerovanie volania funkcie
 * @param gen je kontext generatora
 * @param call je uzol volania
 */
void genCall(CodeGen *gen, CallNode *call);

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
 * @brief funkcia na vygenerovanie vsetkych prikazov bloku
 * @param gen je kontext generatora
 * @param block je generovany blok
 */
void genBlock(CodeGen *gen, BlockNode *block);

/**
 * @brief funkcia na vygenerovanie jedneho prikazu
 * @param gen je kontext generatora
 * @param node je generovany prikaz (moze byt NULL)
 */
void genStatement(CodeGen *gen, ASTNode *node);

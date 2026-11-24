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
    HELPER_SUBSTR,     // $subStr
    HELPER_ORD,        // $ord
    HELPER_TOSTR_ANY,  // $toStr_any, toStr pre hodnotu, ktorej typ sa zisti az za behu
    HELPER_LIST_ALLOC, // $list_alloc, novy zoznam danej dlzky
    HELPER_COUNT       // pocet pomocnych funkcii, musi byt posledny
} HelperKind;

/** @brief pocet vstavanych funkcii, ktore mozu mat obal $wrap$meno */
#define BUILTIN_COUNT 13

/**
 * @brief kontext generatora, prenasa sa medzi vsetkymi funkciami generatora
 */
typedef struct {
    FILE *out;                             // vystupny subor
    DynStr declarations;                   // DEFVAR vsetkych premennych, vypisu sa pred kodom
    DynStr code;                           // vygenerovany kod, vypise sa az na konci
    int labelCounter;                      // pocitadlo navesti L$N
    int varCounter;                        // pocitadlo premennych, cislo N v mene x$N
    int hiddenCounter;                     // pocitadlo skrytych premennych %hN
    LambdaNode **lambdas;                  // lambdy, ktore sa vypisu za funkciami (lambda$N)
    size_t lambdasCount;                   // pocet lambd
    size_t lambdasCapacity;                // velkost pola lambd
    DefFuncNode *function;                 // prave generovana funkcia
    int error;                             // ERR_INTERNAL ak zlyhala alokacia
    bool helperUsed[HELPER_COUNT];         // ktore pomocne funkcie sa maju vypisat
    Symbol *wrapperSymbols[BUILTIN_COUNT]; // vstavane funkcie pouzite ako hodnota, inak NULL
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
 * @brief funkcia na najdenie poradia vstavanej funkcie v tabulke obalov
 * @param name je meno vstavanej funkcie
 * @return index v tabulke, alebo -1 ak tam nie je
 */
int findBuiltin(const char *name);

/**
 * @brief funkcia na vygenerovanie obalu $wrap$meno pre vstavanu funkciu pouzitu ako hodnota
 * @param gen je kontext generatora
 * @param builtinIndex je index vstavanej funkcie v tabulke obalov
 */
void genWrapper(CodeGen *gen, int builtinIndex);

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
 * @brief funkcia na vypis navestia funkcie (f$meno, pri rozsirujucej metode ext$Typ$meno)
 * @param gen je kontext generatora
 * @param function je uzol definicie funkcie alebo metody
 */
void emitFunctionLabel(CodeGen *gen, DefFuncNode *function);

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
 * @brief funkcia na vyhodnotenie argumentov zo vsetkych zoznamov f(a)(b) na zasobnik
 * @param gen je kontext generatora
 * @param call je uzol volania s poslednym zoznamom argumentov
 */
void genArgLists(CodeGen *gen, CallNode *call);

/**
 * @brief funkcia na vygenerovanie volania funkcie definovanej v programe
 * @param gen je kontext generatora
 * @param call je uzol volania
 */
void genFunctionCall(CodeGen *gen, CallNode *call);

/**
 * @brief funkcia na vygenerovanie volania rozsirujucej metody x.meno(...)
 * @param gen je kontext generatora
 * @param call je uzol volania (posledny zoznam argumentov)
 */
void genExtMethodCall(CodeGen *gen, CallNode *call);

/**
 * @brief funkcia na vygenerovanie tela pomocnej funkcie $toStr_any (rozhoduje podla TYPE)
 * @param gen je kontext generatora
 */
void genToStrAnyHelper(CodeGen *gen);

/**
 * @brief funkcia na vygenerovanie konstruktora case class, vytvori pole ["Meno", ...]
 * @param gen je kontext generatora
 * @param call je uzol volania konstruktora
 */
void genConstructor(CodeGen *gen, CallNode *call);

/**
 * @brief funkcia na volanie funkcnej hodnoty (premenna, parameter, vysledok volania)
 * @param gen je kontext generatora
 * @param call je uzol volania
 */
void genValueCall(CodeGen *gen, CallNode *call);

/**
 * @brief funkcia na zistenie mena volanej vstavanej funkcie
 * @param call je uzol volania vstavanej funkcie
 * @return meno funkcie (aj "List.range"), inak prazdny retazec
 */
const char *getBuiltinName(CallNode *call);

/**
 * @brief funkcia na vygenerovanie tela pomocnej funkcie $list_alloc
 *
 * Zoznam je pole ["List", dlzka, prvok0, prvok1, ...].
 * @param gen je kontext generatora
 */
void genListAllocHelper(CodeGen *gen);

/**
 * @brief funkcia na vytvorenie zoznamu, dlzka je na vrchu zasobnika
 * @param gen je kontext generatora
 */
void genListAlloc(CodeGen *gen);

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
 * @brief funkcia na zaokruhlenie Double na vrchu zasobnika nadol (clen floor)
 * @param gen je kontext generatora
 */
void genFloor(CodeGen *gen);

/**
 * @brief funkcia na najdenie poradia atributu v case class
 * @param classDef je definicia triedy
 * @param name je meno atributu
 * @return index atributu od nuly
 */
size_t getFieldIndex(const ClassDefNode *classDef, const char *name);

/**
 * @brief funkcia na pristup k atributu case class (ARRAYGET), objekt je na zasobniku
 * @param gen je kontext generatora
 * @param member je uzol pristupu k atributu
 */
void genFieldAccess(CodeGen *gen, MemberNode *member);

/**
 * @brief funkcia na vygenerovanie pristupu cez bodku (x.toString, ...)
 * @param gen je kontext generatora
 * @param member je uzol pristupu cez bodku
 */
void genMember(CodeGen *gen, MemberNode *member);

/**
 * @brief funkcia na spojenie dvoch retazcov z vrchu zasobnika
 * @param gen je kontext generatora
 */
void genConcat(CodeGen *gen);

/**
 * @brief funkcia na vygenerovanie interpolovaneho retazca s"..."
 * @param gen je kontext generatora
 * @param interp je uzol interpolovaneho retazca
 */
void genInterp(CodeGen *gen, InterpNode *interp);

/**
 * @brief funkcia na vygenerovanie identifikatora (premenna, volanie bez zatvoriek, funkcia ako
 * hodnota)
 * @param gen je kontext generatora
 * @param id je uzol identifikatora
 */
void genId(CodeGen *gen, IDNode *id);

/**
 * @brief funkcia na porovnanie == dvoch hodnot, z ktorych aspon jedna je zjednotenie
 * @param gen je kontext generatora
 */
void genUnionEquals(CodeGen *gen);

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
 * @brief funkcia na zistenie mena typu za behu (ako ho vracia instrukcia TYPE)
 * @param type je staticky typ
 * @return int, float, string, bool, nil alebo array
 */
const char *getRuntimeTypeName(Type *type);

/**
 * @brief funkcia na test literaloveho vzoru (najprv typ, potom hodnota)
 * @param gen je kontext generatora
 * @param caseNode je vetva case
 * @param subjectVar je cislo skrytej premennej s porovnavanou hodnotou
 * @param nextLabel je cislo navestia dalsej vetvy
 */
void genLiteralPattern(CodeGen *gen, CaseNode *caseNode, int subjectVar, int nextLabel);

/**
 * @brief funkcia na kontrolu, ze hodnota je pole s danym menom na indexe 0 (trieda)
 * @param gen je kontext generatora
 * @param tag je ocakavane meno (meno triedy)
 * @param subjectVar je cislo skrytej premennej s hodnotou
 * @param failLabel je cislo navestia pri nezhode
 */
void genTagTest(CodeGen *gen, const char *tag, int subjectVar, int failLabel);

/**
 * @brief funkcia na test vzoru konstruktora Point(x, y) a naviazanie atributov
 * @param gen je kontext generatora
 * @param caseNode je vetva case
 * @param subjectVar je cislo skrytej premennej s porovnavanou hodnotou
 * @param nextLabel je cislo navestia dalsej vetvy
 */
void genConstructorPattern(CodeGen *gen, CaseNode *caseNode, int subjectVar, int nextLabel);

/**
 * @brief funkcia na kontrolu typu hodnoty za behu, pri inom type skoci na failLabel
 * @param gen je kontext generatora
 * @param type je ocakavany typ (moze byt aj zjednotenie)
 * @param subjectVar je cislo skrytej premennej s hodnotou
 * @param failLabel je cislo navestia pri nezhode
 */
void genTypeTest(CodeGen *gen, Type *type, int subjectVar, int failLabel);

/**
 * @brief funkcia na ulozenie porovnavanej hodnoty do premennej zo vzoru
 * @param gen je kontext generatora
 * @param binder je premenna zo vzoru
 * @param subjectVar je cislo skrytej premennej s hodnotou
 */
void genBindSubject(CodeGen *gen, Param *binder, int subjectVar);

/**
 * @brief funkcia na test vzoru vetvy case, pri nezhode skoci na dalsiu vetvu
 * @param gen je kontext generatora
 * @param caseNode je vetva case
 * @param subjectVar je cislo skrytej premennej s porovnavanou hodnotou
 * @param nextLabel je cislo navestia dalsej vetvy
 */
void genPattern(CodeGen *gen, CaseNode *caseNode, int subjectVar, int nextLabel);

/**
 * @brief funkcia na vygenerovanie vyrazu match
 * @param gen je kontext generatora
 * @param match je uzol match
 * @param valueUsed je true, ak ma match nechat hodnotu na zasobniku
 */
void genMatch(CodeGen *gen, MatchNode *match, bool valueUsed);

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
 * @brief funkcia na zapamatanie lambdy, ktora sa vygeneruje az za funkciami
 * @param gen je kontext generatora
 * @param lambda je uzol lambdy
 * @return cislo N lambdy (navestie lambda$N)
 */
int addLambda(CodeGen *gen, LambdaNode *lambda);

/**
 * @brief funkcia na vygenerovanie lambdy ako samostatnej funkcie lambda$N
 * @param gen je kontext generatora
 * @param lambda je uzol lambdy
 * @param lambdaId je cislo lambdy
 */
void genLambda(CodeGen *gen, LambdaNode *lambda, int lambdaId);

/**
 * @brief funkcia na vygenerovanie definicie funkcie alebo rozsirujucej metody
 * @param gen je kontext generatora
 * @param function je uzol definicie funkcie
 */
void genFunction(CodeGen *gen, DefFuncNode *function);

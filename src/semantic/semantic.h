/**
 * @file semantic.h
 * @brief Sémantická analýza (typové kontroly, rozsahy platnosti)
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

#include "symtable.h"

/**
 * @brief kontext kontroly tela jednej funkcie
 *
 * Telo funkcie sa môže kontrolovat aj na poziadanie (z volania v inom tele),
 * vtedy parent ukazuje na kontext volajúcej funkcie.
 */
typedef struct BodyContext{
    DefFuncNode *function;      // funkcia, ktorej telo sa kontroluje
    struct BodyContext *parent; // kontext, z ktoreho sa kontrola spustila, inak NULL
}BodyContext;

/**
 * @brief funkcia na vloženie vstavaných funkcií do aktuálneho (globálneho) rozsahu
 *
 * Vloží vstavané funkcie zo základného zadania aj z rozšírení BOOLTHEN
 * (readBoolean), ALGEBDT (readIntMaybe, readDoubleMaybe) a LISTS (List.range).
 * Typ každej funkcie je TY_FUNC s typmi parametrov a návratovým typom.
 * Členy hodnôt (VALMEM, metódy zoznamov) tu nie sú, nie sú to mená v rozsahu.
 *
 * @param table je tabuľka symbolov, musí mať aspoň jeden rozsah
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie
 *        (pred volaním musí byť ERR_OK, pri úspechu sa nemení)
 */
void addBuiltins(Symtable *table, ErrorCode *error);

/**
 * @brief funkcia na vytvorenie typu funkcie z jej definície
 * @param function je definícia funkcie
 * @returns typ TY_FUNC, parametre všetkých zoznamov sú za sebou
 */
Type *createFunctionType(DefFuncNode *function);

/**
 * @brief funkcia na kontrolu, či sa neopakujú mená parametrov
 * @param table je tabuľka symbolov
 * @param function je definícia funkcie
 * @param receiver je prijemca rozširujúcej metódy, inak NULL
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (4)
 * @returns true, ak má funkcia aspoň jeden parameter
 */
bool checkParamNames(Symtable *table, DefFuncNode *function, Param *receiver, ErrorCode *error);

/**
 * @brief funkcia na vloženie funkcie do tabuľky a kontrolu parametrov a @main
 * @param table je tabuľka symbolov
 * @param function je definícia funkcie
 * @param mainCount je počet doteraz nájdených @main
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (4, 5, 6)
 */
void collectFunction(Symtable *table, DefFuncNode *function, int *mainCount, ErrorCode *error);

/**
 * @brief funkcia na vloženie case class do tabulky typov aj ako konštruktor
 * @param table je tabuľka symbolov
 * @param classDef je definícia triedy
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (4)
 */
void collectClass(Symtable *table, ClassDefNode *classDef, ErrorCode *error);

/**
 * @brief funkcia na vloženie aliasu typu do tabuľky typov
 * @param table je tabulka symbolov
 * @param alias je definícia aliasu
 * @param error je pointer, cez ktory si vrátim chybové hlasenie (4)
 */
void collectTypeAlias(Symtable *table, TypeAliasNode *alias, ErrorCode *error);

/**
 * @brief funkcia na prvý prechod, pozbiera globálne definicie
 * @param table je tabuľka symbolov
 * @param program je koreň AST
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (3 ak chýba @main)
 */
void collectGlobalDefinitions(Symtable *table, ProgramNode *program, ErrorCode *error);

/**
 * @brief funkcia na preskocenie mien typov (TY_NAMED) az po skutočny typ
 * @param type je typ, môze byť NULL
 * @returns skutocný typ, alebo type ak ešte nie je rozliseny
 */
Type *getRealType(Type *type);

/**
 * @brief funkcia na rozlisenie aliasu typu
 * @param table je tabuľka symbolov
 * @param aliasSymbol je symbol aliasu
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (3, 6 ak je alias cyklický)
 */
void resolveAlias(Symtable *table, Symbol *aliasSymbol, ErrorCode *error);

/**
 * @brief funkcia na rozlíšenie vsetkých mien typov (TY_NAMED) vnutri typu
 * @param table je tabuľka symbolov
 * @param type je typ, môže byť NULL
 * @param error je pointer, cez ktory si vratim chybové hlásenie (3 ak typ neexistuje)
 */
void resolveType(Symtable *table, Type *type, ErrorCode *error);

/**
 * @brief funkcia na rozlísenie typov v zozname parametrov
 * @param table je tabuľka symbolov
 * @param paramList je zoznam parametrov
 * @param error je pointer, cez ktory si vrátim chybové hlasenie
 */
void resolveParamList(Symtable *table, ParamList *paramList, ErrorCode *error);

/**
 * @brief funkcia na rozlisenie typov parametrov a navratového typu funkcie
 * @param table je tabuľka symbolov
 * @param function je definícia funkcie
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie
 */
void resolveFunctionTypes(Symtable *table, DefFuncNode *function, ErrorCode *error);

/**
 * @brief funkcia na druhý prechod, rozlíši typy v globálnych definíciách
 * @param table je tabuľka symbolov
 * @param program je koreň AST
 * @param error je pointer, cez ktory si vrátim chybove hlásenie
 */
void resolveGlobalTypes(Symtable *table, ProgramNode *program, ErrorCode *error);

/**
 * @brief funkcia na zistenie mena typu pre kluc rozširujúcej metódy
 * @param type je typ príjemcu
 * @returns meno typu (Int, String, meno triedy, ...)
 */
const char *getTypeKeyName(Type *type);

/**
 * @brief funkcia na vlozenie rozširujúcej metódy pod klúčom "Typ.meno"
 * @param table je tabuľka symbolov
 * @param receiver je prijemca
 * @param method je definícia metódy
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (4)
 */
void collectExtensionMethod(Symtable *table, Param *receiver, DefFuncNode *method,
                            ErrorCode *error);

/**
 * @brief funkcia na vlozenie všetkých rozširujúcich metod (až po rozliseni typov)
 * @param table je tabulka symbolov
 * @param program je koreň AST
 * @param error je pointer, cez ktory si vrátim chybové hlasenie
 */
void collectExtensions(Symtable *table, ProgramNode *program, ErrorCode *error);

/**
 * @brief funkcia na kontrolu, že @main s aliasom vracia Unit
 * @param program je koreň AST
 * @param error je pointer, cez ktory si vrátim chybove hlásenie (6)
 */
void checkMainReturnType(ProgramNode *program, ErrorCode *error);

/**
 * @brief funkcia na sémantickú analýzu programu
 * @param program je koreň AST
 * @param table je prázdna tabuľka symbolov (init aj free robí volajúci)
 * @param error je pointer, cez ktorý si vrátim prvú chybu
 */
void semanticAnalyze(ASTNode *program, Symtable *table, ErrorCode *error);

/**
 * @brief funkcia na zistenie typu literálu
 * @param literal je uzol literalu
 * @returns typ literálu
 */
Type *getLiteralType(LiteralNode *literal);

/**
 * @brief funkcia na zistenie, ci treba hodnotu previest z Int na Double
 * @param expected je typ, ktory na danom mieste čakáme
 * @param actual je typ hodnoty
 * @returns true, ak hodnota je Int a miesto čaká Double (aj vo zjednoteni bez Int)
 */
bool needsIntToDouble(Type *expected, Type *actual);

/**
 * @brief funkcia na vytvorenie uzla prevodu Int na Double
 * @param operand je výraz typu Int
 * @param error je pointer, cez ktorý si vrátim chybové hlasenie (ERR_INTERNAL)
 * @returns nový uzol, pri chybe NULL
 */
ASTNode *createConvertNode(ASTNode *operand, ErrorCode *error);

/**
 * @brief funkcia na kontrolu, či hodnota môže byť na mieste s daným typom
 *
 * Ak treba, nahradí hodnotu v AST uzlom prevodu Int na Double.
 * @param expected je typ, ktory na danom mieste čakáme
 * @param value je pointer na miesto v AST, kde je skontrolovaná hodnota
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (len ERR_INTERNAL)
 * @returns true, ak typ hodnoty sedí (chybu 5 alebo 6 nastavuje volajúci)
 */
bool acceptValue(Type *expected, ASTNode **value, ErrorCode *error);

/**
 * @brief funkcia na pridanie lokálnej premennej do zoznamu premenných funkcie
 * @param function je funkcia, v ktorej tele je premenná
 * @param local je symbol premennej
 * @param error je pointer, cez ktorý si vrátim chybove hlásenie (ERR_INTERNAL)
 */
void addLocal(DefFuncNode *function, Symbol *local, ErrorCode *error);

/**
 * @brief funkcia na nájdenie globálneho rozsahu (pod rozsahom vstavaných funkcií)
 * @param table je tabuľka symbolov
 * @returns globálny rozsah
 */
Scope *getGlobalScope(Symtable *table);

/**
 * @brief funkcia na kontrolu identifikátora vo výraze
 * @param table je tabuľka symbolov
 * @param id je uzol identifikatora
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (3)
 * @returns typ identifikátora, pri chybe NULL
 */
Type *checkId(Symtable *table, IDNode *id, ErrorCode *error);

/**
 * @brief funkcia na kontrolu bloku, blok je vlastny rozsah platnosti
 * @param table je tabuľka symbolov
 * @param context je kontext kontrolovanej funkcie
 * @param block je uzol bloku
 * @param isValueUsed je true, ak sa výsledok bloku dalej použije
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie
 * @returns typ posledného príkazu (prazdny blok Unit)
 */
Type *checkBlock(Symtable *table, BodyContext *context, BlockNode *block, bool isValueUsed,
                 ErrorCode *error);

/**
 * @brief funkcia na kontrolu definície premennej (val / var)
 * @param table je tabuľka symbolov
 * @param context je kontext kontrolovanej funkcie
 * @param varDef je uzol definicie
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (3, 4, 6)
 * @returns Unit, pri chybe NULL
 */
Type *checkVarDef(Symtable *table, BodyContext *context, VarOrValNode *varDef, ErrorCode *error);

/**
 * @brief funkcia na kontrolu priradenia do premennej
 * @param table je tabuľka symbolov
 * @param context je kontext kontrolovanej funkcie
 * @param assign je uzol priradenia
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (3, 6, 7)
 * @returns Unit, pri chybe NULL
 */
Type *checkAssign(Symtable *table, BodyContext *context, AssignNode *assign, ErrorCode *error);

/**
 * @brief funkcia na kontrolu jedného uzla v tele funkcie (príkazu alebo výrazu)
 *
 * Nastaví typ uzla a vráti ho. Typ NULL bez chyby znamená, že uzol končí
 * príkazom return (nemá výsledok).
 * @param table je tabuľka symbolov
 * @param context je kontext kontrolovanej funkcie
 * @param node je pointer na miesto v AST, uzol sa môže nahradiť (prevod Int na Double)
 * @param isValueUsed je true, ak sa výsledok uzla dalej použije
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie
 * @returns typ uzla
 */
Type *checkNode(Symtable *table, BodyContext *context, ASTNode **node, bool isValueUsed,
                ErrorCode *error);

/**
 * @brief funkcia na definovanie príjemcu a parametrov funkcie v aktuálnom rozsahu
 * @param table je tabuľka symbolov
 * @param functionSymbol je symbol funkcie
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (ERR_INTERNAL)
 */
void defineParameters(Symtable *table, Symbol *functionSymbol, ErrorCode *error);

/**
 * @brief funkcia na kontrolu tela jednej funkcie
 *
 * Telo sa kontroluje v rozsahu globálny + parametre, takze nevidí lokálne
 * premenné volajúcej funkcie. Hotová funkcia sa znova nekontroluje.
 * @param table je tabuľka symbolov
 * @param parent je kontext funkcie, z ktorej sa kontrola spustila, inak NULL
 * @param functionSymbol je symbol kontrolovanej funkcie
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie
 */
void checkFunctionBody(Symtable *table, BodyContext *parent, Symbol *functionSymbol,
                       ErrorCode *error);

/**
 * @brief funkcia na tretí prechod, skontroluje telá všetkých funkcií a metod
 * @param table je tabuľka symbolov
 * @param program je koreň AST
 * @param error je pointer, cez ktorý si vrátim prvú chybu
 */
void checkFunctionBodies(Symtable *table, ProgramNode *program, ErrorCode *error);

/**
 * @brief funkcia na zistenie, či je typ číselný (Int alebo Double)
 * @param type je typ, uz bez mien typov
 * @returns true pre Int a Double
 */
bool isNumericType(Type *type);

/**
 * @brief funkcia na prevod Int operandu na Double, ak je druhý operand Double
 * @param binary je uzol binárnej operácie
 * @param leftType je typ ľavého operandu
 * @param rightType je typ pravého operandu
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (ERR_INTERNAL)
 * @returns Double, ak je aspoň jeden operand Double, inak Int
 */
Type *convertNumericOperands(BinaryNode *binary, Type *leftType, Type *rightType,
                             ErrorCode *error);

/**
 * @brief funkcia na kontrolu binárnej operácie
 * @param table je tabuľka symbolov
 * @param context je kontext kontrolovanej funkcie
 * @param binary je uzol binarnej operacie
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (6)
 * @returns typ výsledku, pri chybe NULL
 */
Type *checkBinary(Symtable *table, BodyContext *context, BinaryNode *binary, ErrorCode *error);

/**
 * @brief funkcia na kontrolu unárnej operácie (-, +, !)
 * @param table je tabuľka symbolov
 * @param context je kontext kontrolovanej funkcie
 * @param unary je uzol unarnej operacie
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (6)
 * @returns typ výsledku, pri chybe NULL
 */
Type *checkUnary(Symtable *table, BodyContext *context, UnaryNode *unary, ErrorCode *error);

/**
 * @brief funkcia na kontrolu vetvy podmienky alebo tela cyklu vo vlastnom rozsahu
 * @param table je tabuľka symbolov
 * @param context je kontext kontrolovanej funkcie
 * @param branch je pointer na miesto v AST, kde je vetva
 * @param isValueUsed je true, ak sa výsledok vetvy dalej použije
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie
 * @returns typ vetvy (NULL, ak konci príkazom return)
 */
Type *checkBranch(Symtable *table, BodyContext *context, ASTNode **branch, bool isValueUsed,
                  ErrorCode *error);

/**
 * @brief funkcia na kontrolu, že podmienka je typu Boolean
 * @param table je tabuľka symbolov
 * @param context je kontext kontrolovanej funkcie
 * @param condition je pointer na miesto v AST, kde je podmienka
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (6)
 */
void checkCondition(Symtable *table, BodyContext *context, ASTNode **condition,
                    ErrorCode *error);

/**
 * @brief funkcia na kontrolu podmienky if (prikaz aj vyraz)
 *
 * Ak sa hodnota použije, vetvy musia mať rovnaký typ (príp. Int a Double),
 * chýbajúce else sa berie ako (). Ak sa nepouzije, typy vetiev sa neporovnávajú.
 * @param table je tabuľka symbolov
 * @param context je kontext kontrolovanej funkcie
 * @param ifNode je uzol podmienky
 * @param isValueUsed je true, ak sa výsledok podmienky dalej použije
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (6)
 * @returns typ výsledku (NULL, ak obe vetvy konci príkazom return)
 */
Type *checkIf(Symtable *table, BodyContext *context, IfNode *ifNode, bool isValueUsed,
              ErrorCode *error);

/**
 * @brief funkcia na kontrolu cyklu while
 * @param table je tabuľka symbolov
 * @param context je kontext kontrolovanej funkcie
 * @param whileNode je uzol cyklu
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie (6)
 * @returns Unit, pri chybe NULL
 */
Type *checkWhile(Symtable *table, BodyContext *context, WhileNode *whileNode, ErrorCode *error);

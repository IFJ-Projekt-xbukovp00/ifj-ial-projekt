/**
 * @file test_semantic_global.c
 * @brief Unit testy prvého prechodu sémantiky (funkcie, parametre, @main)
 *
 * Použitie AI: tento súbor vygenerovala AI (Claude Opus 5.5).
 * Prompt: "Prosím vytvor nám unit testy podľa predlohy na sémantický analyzátor, zadanie
 *   nájdeš v .zadanie/"
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

#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>

int failureCount = 0;

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "%s:%d: CHECK zlyhal: %s\n", __FILE__, __LINE__, #condition);          \
            failureCount++;                                                                        \
        }                                                                                          \
    } while (0)

/* pomocné funkcie na AST, kým nie je ast.c */

ProgramNode *newProgram(void) {
    ProgramNode *program = calloc(1, sizeof(ProgramNode));
    program->node.kind = AST_PROGRAM;
    return program;
}

void addItem(ProgramNode *program, ASTNode *item) {
    program->items = realloc(program->items, (program->itemsCount + 1) * sizeof(ASTNode *));
    program->items[program->itemsCount++] = item;
}

/* funkcia zatiaľ bez zoznamov parametrov */
DefFuncNode *newFunction(ProgramNode *program, const char *name, bool isMain, Type *returnType) {
    DefFuncNode *function = calloc(1, sizeof(DefFuncNode));
    function->node.kind = AST_FUNCTION_DEF;
    function->name = (char *)name;
    function->isMain = isMain;
    function->returnType = returnType;
    addItem(program, (ASTNode *)function);
    return function;
}

void addParamList(DefFuncNode *function) {
    function->paramList =
        realloc(function->paramList, (function->paramListCount + 1) * sizeof(ParamList));
    function->paramList[function->paramListCount].params = NULL;
    function->paramList[function->paramListCount].params_count = 0;
    function->paramListCount++;
}

/* parameter ide do posledného zoznamu */
void addParam(DefFuncNode *function, const char *name, Type *type) {
    ParamList *paramList = &function->paramList[function->paramListCount - 1];
    paramList->params =
        realloc(paramList->params, (paramList->params_count + 1) * sizeof(Param));
    Param *param = &paramList->params[paramList->params_count++];
    param->name = (char *)name;
    param->type = type;
    param->line = 0;
    param->symbol = NULL;
}

/* @main def main(): Unit */
DefFuncNode *addMain(ProgramNode *program) {
    DefFuncNode *mainFunction = newFunction(program, "main", true, type_get_basic(TY_UNIT));
    addParamList(mainFunction);
    return mainFunction;
}

void freeProgram(ProgramNode *program) {
    for (size_t itemIndex = 0; itemIndex < program->itemsCount; itemIndex++) {
        if (program->items[itemIndex]->kind == AST_FUNCTION_DEF) {
            DefFuncNode *function = (DefFuncNode *)program->items[itemIndex];
            for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++) {
                free(function->paramList[listIndex].params);
            }
            free(function->paramList);
        }
        free(program->items[itemIndex]);
    }
    free(program->items);
    free(program);
}

/* spustí analýzu, všetko uvoľní a vráti chybu */
ErrorCode analyze(ProgramNode *program) {
    Symtable table;
    symtableInit(&table);
    ErrorCode error = ERR_OK;
    semanticAnalyze((ASTNode *)program, &table, &error);
    symtableFree(&table);
    freeProgram(program);
    return error;
}

void testValidProgram(void) {
    /* def add(a: Int)(b: Double): Int ; def answer ; @main def main(): Unit */
    ProgramNode *program = newProgram();
    DefFuncNode *add = newFunction(program, "add", false, type_get_basic(TY_INT));
    addParamList(add);
    addParam(add, "a", type_get_basic(TY_INT));
    addParamList(add);
    addParam(add, "b", type_get_basic(TY_DOUBLE));
    DefFuncNode *answer = newFunction(program, "answer", false, NULL);
    DefFuncNode *mainFunction = addMain(program);

    Symtable table;
    symtableInit(&table);
    ErrorCode error = ERR_OK;
    semanticAnalyze((ASTNode *)program, &table, &error);
    CHECK(error == ERR_OK);

    Symbol *addSymbol = lookupSymbol(&table, "add");
    CHECK(addSymbol != NULL && addSymbol == add->symbol);
    if (addSymbol != NULL) {
        CHECK(addSymbol->kind == SYM_FUNCTION);
        CHECK(addSymbol->definition == (ASTNode *)add);
        CHECK(addSymbol->checkState == FUNC_NOT_STARTED);
        CHECK(addSymbol->type->kind == TY_FUNC);
        CHECK(addSymbol->type->func.params.len == 2);
        CHECK(addSymbol->type->func.params.items[0]->kind == TY_INT);
        CHECK(addSymbol->type->func.params.items[1]->kind == TY_DOUBLE);
        CHECK(addSymbol->type->func.ret->kind == TY_INT);
    }

    /* neuvedený typ ostane NULL */
    CHECK(answer->symbol != NULL && answer->symbol->type->func.ret == NULL);
    CHECK(answer->symbol != NULL && answer->symbol->type->func.params.len == 0);
    CHECK(mainFunction->symbol == lookupSymbol(&table, "main"));

    /* parametre globálne nevidno, vstavané funkcie áno */
    CHECK(lookupSymbol(&table, "a") == NULL);
    CHECK(lookupSymbol(&table, "print") != NULL);

    symtableFree(&table);
    freeProgram(program);
}

void testMainRules(void) {
    /* chýbajúci @main -> 3 */
    ProgramNode *program = newProgram();
    addParamList(newFunction(program, "f", false, type_get_basic(TY_UNIT)));
    CHECK(analyze(program) == ERR_UNDEF);

    /* prázdny program -> 3 */
    CHECK(analyze(newProgram()) == ERR_UNDEF);

    /* dva @main -> 4 */
    program = newProgram();
    addMain(program);
    addParamList(newFunction(program, "b", true, type_get_basic(TY_UNIT)));
    CHECK(analyze(program) == ERR_REDEF);

    /* @main s parametrom -> 5 */
    program = newProgram();
    addParam(addMain(program), "a", type_get_basic(TY_INT));
    CHECK(analyze(program) == ERR_PARAMS);

    /* @main s parametrom v druhom zozname -> 5 */
    program = newProgram();
    DefFuncNode *mainFunction = addMain(program);
    addParamList(mainFunction);
    addParam(mainFunction, "a", type_get_basic(TY_INT));
    CHECK(analyze(program) == ERR_PARAMS);

    /* @main s návratovým typom Int -> 6 */
    program = newProgram();
    addParamList(newFunction(program, "main", true, type_get_basic(TY_INT)));
    CHECK(analyze(program) == ERR_TYPE);

    /* @main bez typu a bez zátvoriek je ok */
    program = newProgram();
    newFunction(program, "run", true, NULL);
    CHECK(analyze(program) == ERR_OK);

    /* @main môže mať hocijaké meno */
    program = newProgram();
    addParamList(newFunction(program, "runFoo", true, type_get_basic(TY_UNIT)));
    addParamList(newFunction(program, "f", false, type_get_basic(TY_INT)));
    CHECK(analyze(program) == ERR_OK);
}

void testRedefinitions(void) {
    /* dve funkcie s rovnakým menom -> 4 */
    ProgramNode *program = newProgram();
    addParamList(newFunction(program, "f", false, type_get_basic(TY_INT)));
    addParamList(newFunction(program, "f", false, type_get_basic(TY_INT)));
    addMain(program);
    CHECK(analyze(program) == ERR_REDEF);

    /* def f(a: Int, a: Int) -> 4 */
    program = newProgram();
    DefFuncNode *function = newFunction(program, "f", false, type_get_basic(TY_INT));
    addParamList(function);
    addParam(function, "a", type_get_basic(TY_INT));
    addParam(function, "a", type_get_basic(TY_INT));
    addMain(program);
    CHECK(analyze(program) == ERR_REDEF);

    /* def f(a: Int)(a: Int) -> 4 */
    program = newProgram();
    function = newFunction(program, "f", false, type_get_basic(TY_INT));
    addParamList(function);
    addParam(function, "a", type_get_basic(TY_INT));
    addParamList(function);
    addParam(function, "a", type_get_basic(TY_INT));
    addMain(program);
    CHECK(analyze(program) == ERR_REDEF);

    /* parameter s menom funkcie je ok */
    program = newProgram();
    function = newFunction(program, "f", false, type_get_basic(TY_INT));
    addParamList(function);
    addParam(function, "f", type_get_basic(TY_INT));
    addParam(function, "print", type_get_basic(TY_INT));
    addMain(program);
    CHECK(analyze(program) == ERR_OK);

    /* rovnaký parameter v iných funkciách je ok */
    program = newProgram();
    function = newFunction(program, "f", false, type_get_basic(TY_INT));
    addParamList(function);
    addParam(function, "a", type_get_basic(TY_INT));
    function = newFunction(program, "g", false, type_get_basic(TY_INT));
    addParamList(function);
    addParam(function, "a", type_get_basic(TY_INT));
    addMain(program);
    CHECK(analyze(program) == ERR_OK);

    /* vlastná funkcia môže prekryť vstavanú */
    program = newProgram();
    addParamList(newFunction(program, "print", false, type_get_basic(TY_UNIT)));
    addMain(program);
    CHECK(analyze(program) == ERR_OK);
}

int main(void) {
    testValidProgram();
    testMainRules();
    testRedefinitions();

    type_free_all();

    if (failureCount) {
        fprintf(stderr, "test_semantic_global: %d chyb\n", failureCount);
        return 1;
    }
    printf("test_semantic_global: OK\n");
    return 0;
}

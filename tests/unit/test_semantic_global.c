/**
 * @file test_semantic_global.c
 * @brief Unit testy globálnych definícií v sémantike (funkcie, @main, typy, extension)
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

/* metóda rozširenia, do programu sa nepridáva */
DefFuncNode *newMethod(ExtensionNode *extension, const char *name, Type *returnType) {
    DefFuncNode *method = calloc(1, sizeof(DefFuncNode));
    method->node.kind = AST_FUNCTION_DEF;
    method->name = (char *)name;
    method->returnType = returnType;
    extension->methods = realloc(extension->methods,
                                 (extension->methodsCount + 1) * sizeof(ASTNode *));
    extension->methods[extension->methodsCount++] = (ASTNode *)method;
    return method;
}

ExtensionNode *newExtension(ProgramNode *program, const char *receiverName, Type *receiverType) {
    ExtensionNode *extension = calloc(1, sizeof(ExtensionNode));
    extension->node.kind = AST_EXTENSION;
    extension->receiver.name = (char *)receiverName;
    extension->receiver.type = receiverType;
    addItem(program, (ASTNode *)extension);
    return extension;
}

ClassDefNode *newClass(ProgramNode *program, const char *name) {
    ClassDefNode *classDef = calloc(1, sizeof(ClassDefNode));
    classDef->node.kind = AST_CLASS_DEF;
    classDef->name = (char *)name;
    addItem(program, (ASTNode *)classDef);
    return classDef;
}

void addField(ClassDefNode *classDef, const char *name, Type *type) {
    ParamList *fields = &classDef->fields;
    fields->params = realloc(fields->params, (fields->params_count + 1) * sizeof(Param));
    Param *field = &fields->params[fields->params_count++];
    field->name = (char *)name;
    field->type = type;
    field->line = 0;
    field->symbol = NULL;
}

TypeAliasNode *newAlias(ProgramNode *program, const char *name, Type *aliased) {
    TypeAliasNode *alias = calloc(1, sizeof(TypeAliasNode));
    alias->node.kind = AST_TYPE_ALIAS;
    alias->name = (char *)name;
    alias->aliased = aliased;
    addItem(program, (ASTNode *)alias);
    return alias;
}

Type *unionOfTwo(Type *first, Type *second) {
    TypeList members = {NULL, 0, 0};
    type_list_append(&members, first);
    type_list_append(&members, second);
    return type_create_union(members);
}

/* @main def main(): Unit */
DefFuncNode *addMain(ProgramNode *program) {
    DefFuncNode *mainFunction = newFunction(program, "main", true, type_get_basic(TY_UNIT));
    addParamList(mainFunction);
    return mainFunction;
}

void freeFunction(DefFuncNode *function) {
    for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++) {
        free(function->paramList[listIndex].params);
    }
    free(function->paramList);
}

void freeProgram(ProgramNode *program) {
    for (size_t itemIndex = 0; itemIndex < program->itemsCount; itemIndex++) {
        ASTNode *item = program->items[itemIndex];
        if (item->kind == AST_FUNCTION_DEF) {
            freeFunction((DefFuncNode *)item);
        } else if (item->kind == AST_CLASS_DEF) {
            free(((ClassDefNode *)item)->fields.params);
        } else if (item->kind == AST_EXTENSION) {
            ExtensionNode *extension = (ExtensionNode *)item;
            for (size_t methodIndex = 0; methodIndex < extension->methodsCount; methodIndex++) {
                freeFunction((DefFuncNode *)extension->methods[methodIndex]);
                free(extension->methods[methodIndex]);
            }
            free(extension->methods);
        }
        free(item);
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

void testValidTypes(void) {
    /* def f(p: Point, s: Scalar): Scalar ; case class Point(x: Int, y: Int)
       type Scalar = Int | Point ; type Number = Scalar ; @main */
    ProgramNode *program = newProgram();
    DefFuncNode *function = newFunction(program, "f", false, type_create_named("Scalar"));
    addParamList(function);
    addParam(function, "p", type_create_named("Point"));
    addParam(function, "s", type_create_named("Number"));
    ClassDefNode *point = newClass(program, "Point");
    addField(point, "x", type_get_basic(TY_INT));
    addField(point, "y", type_get_basic(TY_INT));
    newAlias(program, "Scalar", unionOfTwo(type_get_basic(TY_INT), type_create_named("Point")));
    newAlias(program, "Number", type_create_named("Scalar"));
    addMain(program);

    Symtable table;
    symtableInit(&table);
    ErrorCode error = ERR_OK;
    semanticAnalyze((ASTNode *)program, &table, &error);
    CHECK(error == ERR_OK);

    /* trieda je v tabuľke typov aj ako konštruktor */
    Symbol *pointSymbol = lookupSymbolInTree(table.typeTree, "Point");
    CHECK(pointSymbol != NULL && pointSymbol == point->symbol);
    CHECK(lookupSymbol(&table, "Point") == pointSymbol);
    if (pointSymbol != NULL) {
        CHECK(pointSymbol->kind == SYM_CLASS);
        CHECK(pointSymbol->type->kind == TY_CLASS && pointSymbol->type->cls == pointSymbol);
    }

    /* alias nie je v rozsahu hodnôt */
    CHECK(lookupSymbol(&table, "Scalar") == NULL);
    CHECK(lookupSymbolInTree(table.typeTree, "Scalar") != NULL);

    /* mená typov sú rozlíšené */
    Type *pointType = getRealType(function->paramList[0].params[0].type);
    CHECK(pointType != NULL && pointType->kind == TY_CLASS && pointType->cls == pointSymbol);
    Type *numberType = getRealType(function->paramList[0].params[1].type);
    CHECK(numberType != NULL && numberType->kind == TY_UNION);
    if (numberType != NULL && numberType->kind == TY_UNION) {
        CHECK(getRealType(numberType->members.items[1])->kind == TY_CLASS);
    }
    CHECK(getRealType(function->returnType) == numberType);

    symtableFree(&table);
    freeProgram(program);
}

void testTypeErrors(void) {
    /* neznamy typ parametra -> 3 */
    ProgramNode *program = newProgram();
    DefFuncNode *function = newFunction(program, "f", false, type_get_basic(TY_INT));
    addParamList(function);
    addParam(function, "x", type_create_named("Foo"));
    addMain(program);
    CHECK(analyze(program) == ERR_UNDEF);

    /* neznamy navratový typ -> 3 */
    program = newProgram();
    addParamList(newFunction(program, "f", false, type_create_list(type_create_named("Foo"))));
    addMain(program);
    CHECK(analyze(program) == ERR_UNDEF);

    /* neznámy typ atributu a v aliase -> 3 */
    program = newProgram();
    addField(newClass(program, "P"), "x", type_create_named("Foo"));
    addMain(program);
    CHECK(analyze(program) == ERR_UNDEF);

    program = newProgram();
    newAlias(program, "A", unionOfTwo(type_get_basic(TY_INT), type_create_named("Foo")));
    addMain(program);
    CHECK(analyze(program) == ERR_UNDEF);

    /* dve case class P -> 4 */
    program = newProgram();
    addField(newClass(program, "P"), "x", type_get_basic(TY_INT));
    addField(newClass(program, "P"), "y", type_get_basic(TY_INT));
    addMain(program);
    CHECK(analyze(program) == ERR_REDEF);

    /* trieda a alias s rovnakým menom -> 4 */
    program = newProgram();
    newClass(program, "P");
    newAlias(program, "P", type_get_basic(TY_INT));
    addMain(program);
    CHECK(analyze(program) == ERR_REDEF);

    /* trieda a funkcia s rovnakým menom -> 4 (v oboch poradiach) */
    program = newProgram();
    newClass(program, "P");
    addParamList(newFunction(program, "P", false, type_get_basic(TY_INT)));
    addMain(program);
    CHECK(analyze(program) == ERR_REDEF);

    program = newProgram();
    addParamList(newFunction(program, "P", false, type_get_basic(TY_INT)));
    newClass(program, "P");
    addMain(program);
    CHECK(analyze(program) == ERR_REDEF);

    /* alias a funkcia s rovnakym menom je ok (iny menny priestor) */
    program = newProgram();
    newAlias(program, "A", type_get_basic(TY_INT));
    addParamList(newFunction(program, "A", false, type_get_basic(TY_INT)));
    addMain(program);
    CHECK(analyze(program) == ERR_OK);

    /* rovnaký atribút -> 4 */
    program = newProgram();
    ClassDefNode *classDef = newClass(program, "P");
    addField(classDef, "x", type_get_basic(TY_INT));
    addField(classDef, "x", type_get_basic(TY_DOUBLE));
    addMain(program);
    CHECK(analyze(program) == ERR_REDEF);

    /* trieda môže mať atribút svojho typu */
    program = newProgram();
    addField(newClass(program, "Node"), "next", type_create_named("Node"));
    addMain(program);
    CHECK(analyze(program) == ERR_OK);

    /* cyklicke aliasy -> 6 */
    program = newProgram();
    newAlias(program, "A", type_create_named("B"));
    newAlias(program, "B", type_create_named("A"));
    addMain(program);
    CHECK(analyze(program) == ERR_TYPE);

    program = newProgram();
    newAlias(program, "A", type_create_list(type_create_named("A")));
    addMain(program);
    CHECK(analyze(program) == ERR_TYPE);

    /* @main s aliasom na Unit je ok, na Int -> 6 */
    program = newProgram();
    newAlias(program, "U", type_get_basic(TY_UNIT));
    addParamList(newFunction(program, "main", true, type_create_named("U")));
    CHECK(analyze(program) == ERR_OK);

    program = newProgram();
    newAlias(program, "I", type_get_basic(TY_INT));
    addParamList(newFunction(program, "main", true, type_create_named("I")));
    CHECK(analyze(program) == ERR_TYPE);
}

void testExtensions(void) {
    /* extension (v: Int) twice ; extension (s: String) twice ; def twice */
    ProgramNode *program = newProgram();
    ExtensionNode *intExtension = newExtension(program, "v", type_get_basic(TY_INT));
    DefFuncNode *intTwice = newMethod(intExtension, "twice", type_get_basic(TY_INT));
    ExtensionNode *stringExtension = newExtension(program, "s", type_get_basic(TY_STRING));
    DefFuncNode *plus = newMethod(stringExtension, "twice", type_get_basic(TY_STRING));
    addParamList(plus);
    addParam(plus, "other", type_get_basic(TY_INT));
    addParamList(newFunction(program, "twice", false, type_get_basic(TY_INT)));
    addMain(program);

    Symtable table;
    symtableInit(&table);
    ErrorCode error = ERR_OK;
    semanticAnalyze((ASTNode *)program, &table, &error);
    CHECK(error == ERR_OK);

    Symbol *method = lookupSymbolInTree(table.extensionTree, "Int.twice");
    CHECK(method != NULL && method == intTwice->symbol);
    if (method != NULL) {
        CHECK(method->kind == SYM_EXTENSION);
        CHECK(method->receiver == &intExtension->receiver);
        CHECK(method->definition == (ASTNode *)intTwice);
        CHECK(method->type->kind == TY_FUNC && method->type->func.params.len == 0);
    }
    method = lookupSymbolInTree(table.extensionTree, "String.twice");
    CHECK(method != NULL && method->type->func.params.len == 1);
    CHECK(lookupSymbol(&table, "twice")->kind == SYM_FUNCTION);

    symtableFree(&table);
    freeProgram(program);

    /* dvakrat twice pre Int -> 4 */
    program = newProgram();
    newMethod(newExtension(program, "v", type_get_basic(TY_INT)), "twice", NULL);
    newMethod(newExtension(program, "w", type_get_basic(TY_INT)), "twice", NULL);
    addMain(program);
    CHECK(analyze(program) == ERR_REDEF);

    /* Int a alias na Int su ten istý typ -> 4 */
    program = newProgram();
    newAlias(program, "MyInt", type_get_basic(TY_INT));
    newMethod(newExtension(program, "v", type_get_basic(TY_INT)), "foo", NULL);
    newMethod(newExtension(program, "w", type_create_named("MyInt")), "foo", NULL);
    addMain(program);
    CHECK(analyze(program) == ERR_REDEF);

    /* parameter s menom prijemcu -> 4 */
    program = newProgram();
    DefFuncNode *methodNode =
        newMethod(newExtension(program, "v", type_get_basic(TY_INT)), "plus", NULL);
    addParamList(methodNode);
    addParam(methodNode, "v", type_get_basic(TY_INT));
    addMain(program);
    CHECK(analyze(program) == ERR_REDEF);

    /* rozsírenie triedy, kľuc je meno triedy */
    program = newProgram();
    newMethod(newExtension(program, "p", type_create_named("P")), "show", NULL);
    newClass(program, "P");
    addMain(program);
    symtableInit(&table);
    error = ERR_OK;
    semanticAnalyze((ASTNode *)program, &table, &error);
    CHECK(error == ERR_OK);
    CHECK(lookupSymbolInTree(table.extensionTree, "P.show") != NULL);
    symtableFree(&table);
    freeProgram(program);

    /* rozšírenie neznámeho typu -> 3 */
    program = newProgram();
    newMethod(newExtension(program, "f", type_create_named("Foo")), "show", NULL);
    addMain(program);
    CHECK(analyze(program) == ERR_UNDEF);
}

int main(void) {
    testValidProgram();
    testMainRules();
    testRedefinitions();
    testValidTypes();
    testTypeErrors();
    testExtensions();

    type_free_all();

    if (failureCount) {
        fprintf(stderr, "test_semantic_global: %d chyb\n", failureCount);
        return 1;
    }
    printf("test_semantic_global: OK\n");
    return 0;
}

/**
 * @file semantic.c
 * @brief Sémantická analýza (typové kontroly, rozsahy platnosti)
 *
 * Použitie AI: návrh a logika (pseudokód) sú moje, prepis do C robila po častiach
 * AI (Claude Opus 5.5). Prompt (približné znenie, presný text sa nezachoval):
 *   "Pregeneruj môj pseudokód na C kód, premenné a funkcie sú štýlom variableName."
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/*
 * Použitie AI: vstavané funkcie (addBuiltin, addBuiltins)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
/**
 * @brief funkcia na vloženie jednej vstavanej funkcie
 * @param table je tabuľka symbolov
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie
 * @param name je meno funkcie
 * @param returnType je návratový typ
 * @param paramCount je počet parametrov, za ním nasledujú ich typy (Type *)
 */
void addBuiltin(Symtable *table, ErrorCode *error, const char *name, Type *returnType,
                int paramCount, ...) {
    if (*error != ERR_OK) {
        return;
    }

    TypeList params = {NULL, 0, 0};
    va_list paramTypes;

    va_start(paramTypes, paramCount);

    for (int paramIndex = 0; paramIndex < paramCount; paramIndex++) {
        type_list_append(&params, va_arg(paramTypes, Type *));
    }

    va_end(paramTypes);
    defineSymbol(table, name, SYM_BUILTIN, type_create_function(params, returnType), false, error);
}

void addBuiltins(Symtable *table, ErrorCode *error) {
    Type *intType = type_get_basic(TY_INT);
    Type *doubleType = type_get_basic(TY_DOUBLE);
    Type *stringType = type_get_basic(TY_STRING);
    Type *unitType = type_get_basic(TY_UNIT);
    Type *boolType = type_get_basic(TY_BOOL);
    Type *anyType = type_get_basic(TY_ANY);

    // základné zadanie
    addBuiltin(table, error, "print", unitType, 1, anyType);
    addBuiltin(table, error, "readLine", stringType, 0);
    addBuiltin(table, error, "readInt", intType, 0);
    addBuiltin(table, error, "readDouble", doubleType, 0);
    addBuiltin(table, error, "toStr", stringType, 1, anyType);
    addBuiltin(table, error, "toInt", intType, 1, doubleType);
    addBuiltin(table, error, "strLength", intType, 1, stringType);
    addBuiltin(table, error, "subStr", stringType, 3, stringType, intType, intType);
    addBuiltin(table, error, "ord", intType, 2, stringType, intType);
    addBuiltin(table, error, "chr", stringType, 1, intType);

    // BOOLTHEN
    addBuiltin(table, error, "readBoolean", boolType, 0);

    // ALGEBDT: pri chybnom vstupe vracajú ()
    TypeList intOrUnit = {NULL, 0, 0};
    type_list_append(&intOrUnit, intType);
    type_list_append(&intOrUnit, unitType);
    addBuiltin(table, error, "readIntMaybe", type_create_union(intOrUnit), 0);

    TypeList doubleOrUnit = {NULL, 0, 0};
    type_list_append(&doubleOrUnit, doubleType);
    type_list_append(&doubleOrUnit, unitType);
    addBuiltin(table, error, "readDoubleMaybe", type_create_union(doubleOrUnit), 0);

    // LISTS: List.range(start, end) aj List.range(start, end, step), tretí parameter
    // je nepovinný – kontrola volania musí povoliť 2 aj 3 argumenty
    addBuiltin(table, error, "List.range", type_create_list(intType), 3, intType, intType, intType);
}

/*
 * Použitie AI: typy a parametre (createFunctionType, checkParamNames)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
Type *createFunctionType(DefFuncNode *function) {
    // parametre zo všetkých zoznamov dáme za sebou
    TypeList paramTypes = {NULL, 0, 0};

    for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++) {
        ParamList *paramList = &function->paramList[listIndex];

        for (size_t paramIndex = 0; paramIndex < paramList->params_count; paramIndex++) {
            type_list_append(&paramTypes, paramList->params[paramIndex].type);
        }
    }

    return type_create_function(paramTypes, function->returnType);
}

bool checkParamNames(Symtable *table, DefFuncNode *function, Param *receiver, ErrorCode *error) {
    bool hasParameter = false;

    // rovnaké meno parametra aj v inom zozname je chyba 4
    pushScope(table, error);
    if (*error != ERR_OK) {
        return false;
    }

    if (receiver != NULL) {
        defineSymbol(table, receiver->name, SYM_PARAMETER, receiver->type, false, error);
    }

    for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++) {
        ParamList *paramList = &function->paramList[listIndex];

        for (size_t paramIndex = 0; paramIndex < paramList->params_count; paramIndex++) {
            Param *param = &paramList->params[paramIndex];
            hasParameter = true;

            if (*error == ERR_OK) {
                defineSymbol(table, param->name, SYM_PARAMETER, param->type, false, error);
            }
        }
    }

    popScope(table);
    return hasParameter;
}

/*
 * Použitie AI: globálne funkcie a @main (collectFunction)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
void collectFunction(Symtable *table, DefFuncNode *function, int *mainCount, ErrorCode *error) {
    if (*error != ERR_OK) {
        return;
    }

    Symbol *functionSymbol = defineSymbol(table, function->name, SYM_FUNCTION,
                                          createFunctionType(function), false, error);
    if (functionSymbol == NULL) {
        return;
    }

    functionSymbol->definition = (ASTNode *)function;
    function->symbol = functionSymbol;

    bool hasParameter = checkParamNames(table, function, NULL, error);

    if (*error != ERR_OK || !function->isMain) {
        return;
    }

    (*mainCount)++;

    if (*mainCount > 1) {
        *error = ERR_REDEF;
        return;
    }

    if (hasParameter) {
        *error = ERR_PARAMS;
        return;
    }

    // neuvedený typ a alias skontrolujeme až neskôr
    if (function->returnType != NULL && function->returnType->kind != TY_UNIT &&
        function->returnType->kind != TY_NAMED) {
        *error = ERR_TYPE;
    }
}

/*
 * Použitie AI: triedy a aliasy typov (collectClass, collectTypeAlias)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
void collectClass(Symtable *table, ClassDefNode *classDef, ErrorCode *error) {
    if (*error != ERR_OK) {
        return;
    }

    Symbol *classSymbol =
        defineSymbolInTree(table, &table->typeTree, classDef->name, SYM_CLASS, NULL, false, error);
    if (classSymbol == NULL) {
        return;
    }

    classSymbol->type = type_create_class(classSymbol);
    classSymbol->definition = (ASTNode *)classDef;
    classDef->symbol = classSymbol;

    // meno triedy je aj konstruktor, takže nesmie byť rovnaké ako funkcia
    table->top->tree = insertNode(table->top->tree, classDef->name, classSymbol, error);
    if (*error != ERR_OK) {
        return;
    }

    // rovnake meno atributu je chyba 4
    pushScope(table, error);
    if (*error != ERR_OK) {
        return;
    }

    for (size_t fieldIndex = 0; fieldIndex < classDef->fields.params_count; fieldIndex++) {
        Param *field = &classDef->fields.params[fieldIndex];

        if (*error == ERR_OK) {
            defineSymbol(table, field->name, SYM_PARAMETER, field->type, false, error);
        }
    }

    popScope(table);
}

void collectTypeAlias(Symtable *table, TypeAliasNode *alias, ErrorCode *error) {
    if (*error != ERR_OK) {
        return;
    }

    Symbol *aliasSymbol = defineSymbolInTree(table, &table->typeTree, alias->name, SYM_ALIAS,
                                             alias->aliased, false, error);
    if (aliasSymbol == NULL) {
        return;
    }

    aliasSymbol->definition = (ASTNode *)alias;
    alias->symbol = aliasSymbol;
}

/*
 * Použitie AI: globálne definície (collectGlobalDefinitions)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
void collectGlobalDefinitions(Symtable *table, ProgramNode *program, ErrorCode *error) {
    int mainCount = 0;

    for (size_t itemIndex = 0; itemIndex < program->itemsCount; itemIndex++) {
        if (*error != ERR_OK) {
            return;
        }

        ASTNode *item = program->items[itemIndex];

        if (item->kind == AST_FUNCTION_DEF) {
            collectFunction(table, (DefFuncNode *)item, &mainCount, error);
        } else if (item->kind == AST_CLASS_DEF) {
            collectClass(table, (ClassDefNode *)item, error);
        } else if (item->kind == AST_TYPE_ALIAS) {
            collectTypeAlias(table, (TypeAliasNode *)item, error);
        } else if (item->kind == AST_EXTENSION) {
            ExtensionNode *extension = (ExtensionNode *)item;

            for (size_t methodIndex = 0; methodIndex < extension->methodsCount; methodIndex++) {
                if (*error == ERR_OK) {
                    checkParamNames(table, (DefFuncNode *)extension->methods[methodIndex],
                                    &extension->receiver, error);
                }
            }
        }
    }

    if (*error == ERR_OK && mainCount == 0) {
        *error = ERR_UNDEF;
    }
}

/*
 * Použitie AI: rozlíšenie typov a extension (getRealType … checkMainReturnType)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
Type *getRealType(Type *type) {
    while (type != NULL && type->kind == TY_NAMED && type->named.resolved != NULL) {
        type = type->named.resolved;
    }

    return type;
}

void resolveAlias(Symtable *table, Symbol *aliasSymbol, ErrorCode *error) {
    if (aliasSymbol->checkState == FUNC_DONE) {
        return;
    }

    // alias, ktory sa odkazuje sam na seba
    if (aliasSymbol->checkState == FUNC_IN_PROGRESS) {
        *error = ERR_TYPE;
        return;
    }

    aliasSymbol->checkState = FUNC_IN_PROGRESS;
    resolveType(table, aliasSymbol->type, error);
    aliasSymbol->checkState = FUNC_DONE;
}

void resolveType(Symtable *table, Type *type, ErrorCode *error) {
    if (*error != ERR_OK || type == NULL) {
        return;
    }

    if (type->kind == TY_LIST) {
        resolveType(table, type->elem, error);
    } else if (type->kind == TY_FUNC) {
        for (size_t paramIndex = 0; paramIndex < type->func.params.len; paramIndex++) {
            resolveType(table, type->func.params.items[paramIndex], error);
        }
        resolveType(table, type->func.ret, error);
    } else if (type->kind == TY_UNION) {
        for (size_t memberIndex = 0; memberIndex < type->members.len; memberIndex++) {
            resolveType(table, type->members.items[memberIndex], error);
        }
    } else if (type->kind == TY_NAMED && type->named.resolved == NULL) {
        Symbol *typeSymbol = lookupSymbolInTree(table->typeTree, type->named.name);

        if (typeSymbol == NULL) {
            *error = ERR_UNDEF;
            return;
        }

        if (typeSymbol->kind == SYM_ALIAS) {
            resolveAlias(table, typeSymbol, error);
        }

        type->named.resolved = getRealType(typeSymbol->type);
    }
}

void resolveParamList(Symtable *table, ParamList *paramList, ErrorCode *error) {
    for (size_t paramIndex = 0; paramIndex < paramList->params_count; paramIndex++) {
        resolveType(table, paramList->params[paramIndex].type, error);
    }
}

void resolveFunctionTypes(Symtable *table, DefFuncNode *function, ErrorCode *error) {
    for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++) {
        resolveParamList(table, &function->paramList[listIndex], error);
    }

    resolveType(table, function->returnType, error);
}

void resolveGlobalTypes(Symtable *table, ProgramNode *program, ErrorCode *error) {
    for (size_t itemIndex = 0; itemIndex < program->itemsCount; itemIndex++) {
        ASTNode *item = program->items[itemIndex];

        if (item->kind == AST_FUNCTION_DEF) {
            resolveFunctionTypes(table, (DefFuncNode *)item, error);
        } else if (item->kind == AST_CLASS_DEF) {
            resolveParamList(table, &((ClassDefNode *)item)->fields, error);
        } else if (item->kind == AST_TYPE_ALIAS) {
            resolveAlias(table, ((TypeAliasNode *)item)->symbol, error);
        } else if (item->kind == AST_EXTENSION) {
            ExtensionNode *extension = (ExtensionNode *)item;
            resolveType(table, extension->receiver.type, error);

            for (size_t methodIndex = 0; methodIndex < extension->methodsCount; methodIndex++) {
                resolveFunctionTypes(table, (DefFuncNode *)extension->methods[methodIndex], error);
            }
        }

        if (*error != ERR_OK) {
            return;
        }
    }
}

const char *getTypeKeyName(Type *type) {
    type = getRealType(type);

    if (type == NULL) {
        return "?";
    }
    if (type->kind == TY_INT) {
        return "Int";
    }
    if (type->kind == TY_DOUBLE) {
        return "Double";
    }
    if (type->kind == TY_STRING) {
        return "String";
    }
    if (type->kind == TY_BOOL) {
        return "Boolean";
    }
    if (type->kind == TY_UNIT) {
        return "Unit";
    }
    if (type->kind == TY_CLASS) {
        return ((ClassDefNode *)type->cls->definition)->name;
    }
    if (type->kind == TY_LIST) {
        return "List";
    }
    // zjednotenia a funkcie ako príjemcu nepodporujeme presne
    return "?";
}

void collectExtensionMethod(Symtable *table, Param *receiver, DefFuncNode *method,
                            ErrorCode *error) {
    const char *typeName = getTypeKeyName(receiver->type);
    char *key = malloc(strlen(typeName) + strlen(method->name) + 2);

    if (key == NULL) {
        *error = ERR_INTERNAL;
        return;
    }

    strcpy(key, typeName);
    strcat(key, ".");
    strcat(key, method->name);

    Symbol *methodSymbol = defineSymbolInTree(table, &table->extensionTree, key, SYM_EXTENSION,
                                              createFunctionType(method), false, error);
    free(key);

    if (methodSymbol == NULL) {
        return;
    }

    methodSymbol->definition = (ASTNode *)method;
    methodSymbol->receiver = receiver;
    method->symbol = methodSymbol;
}

void collectExtensions(Symtable *table, ProgramNode *program, ErrorCode *error) {
    for (size_t itemIndex = 0; itemIndex < program->itemsCount; itemIndex++) {
        if (program->items[itemIndex]->kind != AST_EXTENSION) {
            continue;
        }

        ExtensionNode *extension = (ExtensionNode *)program->items[itemIndex];

        for (size_t methodIndex = 0; methodIndex < extension->methodsCount; methodIndex++) {
            if (*error != ERR_OK) {
                return;
            }

            collectExtensionMethod(table, &extension->receiver,
                                   (DefFuncNode *)extension->methods[methodIndex], error);
        }
    }
}

void checkMainReturnType(ProgramNode *program, ErrorCode *error) {
    for (size_t itemIndex = 0; itemIndex < program->itemsCount; itemIndex++) {
        if (program->items[itemIndex]->kind != AST_FUNCTION_DEF) {
            continue;
        }

        DefFuncNode *function = (DefFuncNode *)program->items[itemIndex];
        const Type *returnType = getRealType(function->returnType);

        // @main s aliasom, ktorý nie je Unit
        if (function->isMain && returnType != NULL && returnType->kind != TY_UNIT) {
            *error = ERR_TYPE;
            return;
        }
    }
}

/*
 * Použitie AI: hlavná funkcia analýzy (semanticAnalyze)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
void semanticAnalyze(ASTNode *program, Symtable *table, ErrorCode *error) {
    if (program == NULL || program->kind != AST_PROGRAM) {
        *error = ERR_INTERNAL;
        return;
    }

    // vstavané funkcie majú vlastný rozsah, aby sa dali prekryť
    pushScope(table, error);
    addBuiltins(table, error);

    // globálny rozsah
    if (*error == ERR_OK) {
        pushScope(table, error);
    }

    if (*error != ERR_OK) {
        return;
    }

    ProgramNode *programNode = (ProgramNode *)program;

    collectGlobalDefinitions(table, programNode, error);

    if (*error == ERR_OK) {
        resolveGlobalTypes(table, programNode, error);
    }
    if (*error == ERR_OK) {
        collectExtensions(table, programNode, error);
    }
    if (*error == ERR_OK) {
        checkMainReturnType(programNode, error);
    }
    if (*error == ERR_OK) {
        checkFunctionBodies(table, programNode, error);
    }
}

/*
 * Použitie AI: premenné a rozsahy (getLiteralType … checkFunctionBodies)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
Type *getLiteralType(const LiteralNode *literal) {
    if (literal->litType == LIT_INT) {
        return type_get_basic(TY_INT);
    }
    if (literal->litType == LIT_DOUBLE) {
        return type_get_basic(TY_DOUBLE);
    }
    if (literal->litType == LIT_STRING) {
        return type_get_basic(TY_STRING);
    }
    if (literal->litType == LIT_BOOLEAN) {
        return type_get_basic(TY_BOOL);
    }

    return type_get_basic(TY_UNIT);
}

bool needsIntToDouble(Type *expected, Type *actual) {
    expected = getRealType(expected);
    actual = getRealType(actual);

    if (expected == NULL || actual == NULL || actual->kind != TY_INT) {
        return false;
    }
    if (expected->kind == TY_DOUBLE) {
        return true;
    }
    if (expected->kind != TY_UNION) {
        return false;
    }

    // zjednotenie, ktore Int obsahuje, hodnotu prijme bez prevodu
    bool hasDouble = false;

    for (size_t memberIndex = 0; memberIndex < expected->members.len; memberIndex++) {
        const Type *member = getRealType(expected->members.items[memberIndex]);

        if (member->kind == TY_INT) {
            return false;
        }
        if (member->kind == TY_DOUBLE) {
            hasDouble = true;
        }
    }

    return hasDouble;
}

ASTNode *createConvertNode(ASTNode *operand, ErrorCode *error) {
    ConvertNode *convert = calloc(1, sizeof(ConvertNode));

    if (convert == NULL) {
        *error = ERR_INTERNAL;
        return NULL;
    }

    convert->node.kind = AST_CONVERT;
    convert->node.line = operand->line;
    convert->node.type = type_get_basic(TY_DOUBLE);
    convert->operand = operand;
    return (ASTNode *)convert;
}

bool acceptValue(Type *expected, ASTNode **value, ErrorCode *error) {
    if (!type_accepts(expected, (*value)->type)) {
        return false;
    }

    if (needsIntToDouble(expected, (*value)->type)) {
        ASTNode *convert = createConvertNode(*value, error);

        if (convert != NULL) {
            *value = convert;
        }
    }

    return true;
}

void addLocal(DefFuncNode *function, Symbol *local, ErrorCode *error) {
    Symbol **newLocals = realloc(function->locals, (function->localsCount + 1) * sizeof(Symbol *));

    if (newLocals == NULL) {
        *error = ERR_INTERNAL;
        return;
    }

    function->locals = newLocals;
    function->locals[function->localsCount] = local;
    function->localsCount++;
}

Scope *getGlobalScope(Symtable *table) {
    Scope *scope = table->top;

    // najspodnejsi rozsah su vstavane funkcie, nad nim je globalny
    while (scope != NULL && scope->parent != NULL && scope->parent->parent != NULL) {
        scope = scope->parent;
    }

    return scope;
}

Type *checkId(Symtable *table, BodyContext *context, IDNode *id, ErrorCode *error) {
    Symbol *symbol = lookupSymbol(table, id->name);

    if (symbol == NULL) {
        *error = ERR_UNDEF;
        return NULL;
    }

    // funkcia bez zoznamu parametrov sa vola bez zatvoriek
    if (symbol->kind == SYM_FUNCTION && ((DefFuncNode *)symbol->definition)->paramListCount == 0) {
        id->idKind = ID_FUNC_CALL;
        id->symbol = symbol;
        return getFunctionReturnType(table, context, symbol, error);
    }

    // funkcia s jedným zoznamom parametrov ako hodnota (referencia na funkciu)
    if (symbol->kind == SYM_FUNCTION && ((DefFuncNode *)symbol->definition)->paramListCount == 1) {
        getFunctionReturnType(table, context, symbol, error);
        if (*error != ERR_OK) {
            return NULL;
        }

        id->idKind = ID_FUNC_REF;
        id->symbol = symbol;
        return symbol->type;
    }

    // ine funkcie a konštruktory ako hodnotu nepodporujeme
    if (symbol->kind != SYM_VARIABLE && symbol->kind != SYM_PARAMETER) {
        *error = ERR_TYPE;
        return NULL;
    }

    id->idKind = ID_VAR;
    id->symbol = symbol;
    return symbol->type;
}

Type *checkBlock(Symtable *table, BodyContext *context, BlockNode *block, bool isValueUsed,
                 ErrorCode *error) {
    pushScope(table, error);
    if (*error != ERR_OK) {
        return NULL;
    }

    Type *blockType = type_get_basic(TY_UNIT);

    for (size_t commandIndex = 0; commandIndex < block->commandOrderCount; commandIndex++) {
        // hodnota sa pouziva len z posledneho prikazu bloku
        bool isLastCommand = commandIndex + 1 == block->commandOrderCount;
        blockType = checkNode(table, context, &block->commandOrder[commandIndex],
                              isLastCommand && isValueUsed, error);

        if (*error != ERR_OK) {
            break;
        }
    }

    // rozsah sa zrusi aj pri chybe, aby sa dal uvolnit cez symtableFree
    popScope(table);
    return blockType;
}

Type *checkVarDef(Symtable *table, BodyContext *context, VarOrValNode *varDef, ErrorCode *error) {
    // inicializacia sa kontroluje pred definiciou, premenna v nej este nie je vidno
    Type *variableType = checkNode(table, context, &varDef->init, true, error);
    if (*error != ERR_OK) {
        return NULL;
    }

    if (varDef->declaredType != NULL) {
        resolveType(table, varDef->declaredType, error);
        if (*error != ERR_OK) {
            return NULL;
        }

        if (!acceptValue(varDef->declaredType, &varDef->init, error)) {
            *error = ERR_TYPE;
        }
        if (*error != ERR_OK) {
            return NULL;
        }

        variableType = varDef->declaredType;
    } else if (variableType == NULL) {
        // typ sa neda odvodit
        *error = ERR_TYPE;
        return NULL;
    } else if (varDef->init->kind == AST_ID && ((IDNode *)varDef->init)->idKind == ID_FUNC_REF) {
        // bežnú funkciu mozno ulozit len do premennej s uvedeným typom
        *error = ERR_TYPE;
        return NULL;
    } else if (getRealType(variableType)->kind == TY_LIST &&
               getRealType(variableType)->elem == NULL) {
        // prázdny zoznam vyzaduje očakávaný typ
        *error = ERR_TYPE;
        return NULL;
    }

    Symbol *variableSymbol =
        defineSymbol(table, varDef->name, SYM_VARIABLE, variableType, varDef->isMutable, error);
    if (variableSymbol == NULL) {
        return NULL;
    }

    varDef->symbol = variableSymbol;
    addContextLocal(context, variableSymbol, error);
    return type_get_basic(TY_UNIT);
}

Type *checkAssign(Symtable *table, BodyContext *context, AssignNode *assign, ErrorCode *error) {
    Symbol *targetSymbol = lookupSymbol(table, assign->name);

    if (targetSymbol == NULL) {
        *error = ERR_UNDEF;
        return NULL;
    }

    // do val, parametra ani funkcie sa priradit neda
    if (targetSymbol->kind != SYM_VARIABLE || !targetSymbol->isMutable) {
        *error = ERR_IMMUTABLE;
        return NULL;
    }

    assign->symbol = targetSymbol;

    checkNode(table, context, &assign->value, true, error);
    if (*error != ERR_OK) {
        return NULL;
    }

    if (!acceptValue(targetSymbol->type, &assign->value, error)) {
        *error = ERR_TYPE;
    }
    if (*error != ERR_OK) {
        return NULL;
    }

    return type_get_basic(TY_UNIT);
}

Type *checkNode(Symtable *table, BodyContext *context, ASTNode **node, bool isValueUsed,
                ErrorCode *error) {
    if (*error != ERR_OK) {
        return NULL;
    }
    if (*node == NULL) {
        return type_get_basic(TY_UNIT);
    }

    ASTNode *current = *node;
    Type *nodeType = type_get_basic(TY_UNIT);

    if (current->kind == AST_LIT) {
        nodeType = getLiteralType((LiteralNode *)current);
    } else if (current->kind == AST_ID) {
        nodeType = checkId(table, context, (IDNode *)current, error);
    } else if (current->kind == AST_BLOCK) {
        nodeType = checkBlock(table, context, (BlockNode *)current, isValueUsed, error);
    } else if (current->kind == AST_VAR_DEF) {
        nodeType = checkVarDef(table, context, (VarOrValNode *)current, error);
    } else if (current->kind == AST_ASSIGN) {
        nodeType = checkAssign(table, context, (AssignNode *)current, error);
    } else if (current->kind == AST_BINARY) {
        nodeType = checkBinary(table, context, (BinaryNode *)current, error);
    } else if (current->kind == AST_UNARY) {
        nodeType = checkUnary(table, context, (UnaryNode *)current, error);
    } else if (current->kind == AST_IF) {
        nodeType = checkIf(table, context, (IfNode *)current, isValueUsed, error);
    } else if (current->kind == AST_WHILE) {
        nodeType = checkWhile(table, context, (WhileNode *)current, error);
    } else if (current->kind == AST_RANGE) {
        nodeType = checkRange(table, context, (RangeNode *)current, error);
    } else if (current->kind == AST_FOR) {
        nodeType = checkFor(table, context, (ForNode *)current, error);
    } else if (current->kind == AST_LIST_LIT) {
        nodeType = checkListLiteral(table, context, (ListLitNode *)current, error);
    } else if (current->kind == AST_LAMBDA) {
        nodeType = checkLambda(table, context, (LambdaNode *)current, error);
    } else if (current->kind == AST_INTERP) {
        nodeType = checkInterpolation(table, context, (InterpNode *)current, error);
    } else if (current->kind == AST_MATCH) {
        nodeType = checkMatch(table, context, (MatchNode *)current, isValueUsed, error);
    } else if (current->kind == AST_MEMBER) {
        nodeType = checkMember(table, context, (MemberNode *)current, error);
    } else if (current->kind == AST_CALL) {
        nodeType = checkCall(table, context, (CallNode *)current, error);
    } else if (current->kind == AST_RETURN) {
        nodeType = checkReturn(table, context, (ReturnNode *)current, error);
    } else if (current->kind == AST_EXPR_STATEMENT) {
        nodeType =
            checkNode(table, context, &((LonelyExprNode *)current)->expr, isValueUsed, error);
    }

    current->type = nodeType;
    return nodeType;
}

void defineParameters(Symtable *table, Symbol *functionSymbol, ErrorCode *error) {
    DefFuncNode *function = (DefFuncNode *)functionSymbol->definition;
    Param *receiver = functionSymbol->receiver;

    if (receiver != NULL) {
        receiver->symbol =
            defineSymbol(table, receiver->name, SYM_PARAMETER, receiver->type, false, error);
    }

    for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++) {
        ParamList *paramList = &function->paramList[listIndex];

        for (size_t paramIndex = 0; paramIndex < paramList->params_count; paramIndex++) {
            Param *param = &paramList->params[paramIndex];

            if (*error == ERR_OK) {
                param->symbol =
                    defineSymbol(table, param->name, SYM_PARAMETER, param->type, false, error);
            }
        }
    }
}

void checkFunctionBody(Symtable *table, BodyContext *parent, Symbol *functionSymbol,
                       ErrorCode *error) {
    if (*error != ERR_OK || functionSymbol->checkState == FUNC_DONE) {
        return;
    }

    DefFuncNode *function = (DefFuncNode *)functionSymbol->definition;

    // funkcia bez tela je len v rucne poskladanych AST
    if (function->body == NULL) {
        functionSymbol->checkState = FUNC_DONE;
        return;
    }

    functionSymbol->checkState = FUNC_IN_PROGRESS;

    // telo nesmie vidiet lokalne premenne volajucej funkcie
    Scope *savedTop = table->top;
    table->top = getGlobalScope(table);

    pushScope(table, error);
    if (*error != ERR_OK) {
        table->top = savedTop;
        return;
    }

    BodyContext context = {function, parent, NULL};

    // pri explicitnom Unit sa vysledok tela zahodi
    bool isUnitDeclared = isUnitType(function->returnType);

    defineParameters(table, functionSymbol, error);
    Type *bodyType = checkNode(table, &context, &function->body, !isUnitDeclared, error);
    checkFunctionResult(functionSymbol, bodyType, error);

    popScope(table);
    table->top = savedTop;
    functionSymbol->checkState = FUNC_DONE;
}

void checkFunctionBodies(Symtable *table, ProgramNode *program, ErrorCode *error) {
    for (size_t itemIndex = 0; itemIndex < program->itemsCount; itemIndex++) {
        ASTNode *item = program->items[itemIndex];

        if (item->kind == AST_FUNCTION_DEF) {
            checkFunctionBody(table, NULL, ((DefFuncNode *)item)->symbol, error);
        } else if (item->kind == AST_EXTENSION) {
            ExtensionNode *extension = (ExtensionNode *)item;

            for (size_t methodIndex = 0; methodIndex < extension->methodsCount; methodIndex++) {
                checkFunctionBody(table, NULL,
                                  ((DefFuncNode *)extension->methods[methodIndex])->symbol, error);
            }
        }

        if (*error != ERR_OK) {
            return;
        }
    }
}

/*
 * Použitie AI: výrazy a operátory (isNumericType … checkUnary)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
bool isNumericType(const Type *type) {
    return type != NULL && (type->kind == TY_INT || type->kind == TY_DOUBLE);
}

Type *convertNumericOperands(BinaryNode *binary, const Type *leftType, const Type *rightType,
                             ErrorCode *error) {
    if (leftType->kind == TY_INT && rightType->kind == TY_INT) {
        return type_get_basic(TY_INT);
    }

    // jediná implicitná konverzia, prevedie sa ta strana, ktora je Int
    if (leftType->kind == TY_INT) {
        ASTNode *convert = createConvertNode(binary->left, error);
        if (convert != NULL) {
            binary->left = convert;
        }
    } else if (rightType->kind == TY_INT) {
        ASTNode *convert = createConvertNode(binary->right, error);
        if (convert != NULL) {
            binary->right = convert;
        }
    }

    return type_get_basic(TY_DOUBLE);
}

Type *checkBinary(Symtable *table, BodyContext *context, BinaryNode *binary, ErrorCode *error) {
    // operandy sa vyhodnocuju zlava doprava
    Type *leftType = getRealType(checkNode(table, context, &binary->left, true, error));
    Type *rightType = getRealType(checkNode(table, context, &binary->right, true, error));

    if (*error != ERR_OK) {
        return NULL;
    }

    // operand bez hodnoty (konci returnom) nemoze byt vo vyraze
    if (leftType == NULL || rightType == NULL) {
        *error = ERR_TYPE;
        return NULL;
    }

    TokenType operator= binary->operator;
    bool bothNumeric = isNumericType(leftType) && isNumericType(rightType);
    bool bothString = leftType->kind == TY_STRING && rightType->kind == TY_STRING;

    if (operator== TOK_PLUS || operator== TOK_MINUS || operator== TOK_MULTIPLY || operator==
        TOK_DIVIDE) {
        if (bothNumeric) {
            return convertNumericOperands(binary, leftType, rightType, error);
        }
        // konkatenacia, ine typy sa na retazec automaticky neprevadzaju
        if (operator== TOK_PLUS && bothString) {
            return type_get_basic(TY_STRING);
        }
    } else if (operator== TOK_LESS || operator== TOK_LESS_EQUAL || operator==
               TOK_GREATER || operator== TOK_GREATER_EQUAL) {
        if (bothNumeric) {
            convertNumericOperands(binary, leftType, rightType, error);
            return type_get_basic(TY_BOOL);
        }
        if (bothString) {
            return type_get_basic(TY_BOOL);
        }
    } else if (operator== TOK_EQUAL || operator== TOK_NOT_EQUAL) {
        if (bothNumeric) {
            convertNumericOperands(binary, leftType, rightType, error);
            return type_get_basic(TY_BOOL);
        }
        if (type_equals(leftType, rightType)) {
            return type_get_basic(TY_BOOL);
        }
    } else if (operator== TOK_AND || operator== TOK_OR) {
        if (leftType->kind == TY_BOOL && rightType->kind == TY_BOOL) {
            return type_get_basic(TY_BOOL);
        }
    }

    else if (operator== TOK_CONCAT && leftType->kind == TY_LIST && rightType->kind == TY_LIST) {
        // prazdny zoznam sa prispôsobí druhému
        if (leftType->elem == NULL) {
            return rightType;
        }
        if (rightType->elem == NULL || type_equals(leftType, rightType)) {
            return leftType;
        }
    }

    // nesulad typov alebo nepodporovany operator
    *error = ERR_TYPE;
    return NULL;
}

Type *checkUnary(Symtable *table, BodyContext *context, UnaryNode *unary, ErrorCode *error) {
    Type *operandType = getRealType(checkNode(table, context, &unary->right, true, error));

    if (*error != ERR_OK) {
        return NULL;
    }

    // unarne - a + nemenia typ cisla
    if ((unary->operator== TOK_MINUS || unary->operator== TOK_PLUS) && isNumericType(operandType)) {
        return operandType;
    }
    if (unary->operator== TOK_NOT && operandType != NULL && operandType->kind == TY_BOOL) {
        return operandType;
    }

    *error = ERR_TYPE;
    return NULL;
}

/*
 * Použitie AI: podmienky a cykly (checkBranch … checkWhile)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
Type *checkBranch(Symtable *table, BodyContext *context, ASTNode **branch, bool isValueUsed,
                  ErrorCode *error) {
    // aj vetva bez bloku je samostatny rozsah platnosti
    pushScope(table, error);
    if (*error != ERR_OK) {
        return NULL;
    }

    Type *branchType = checkNode(table, context, branch, isValueUsed, error);

    popScope(table);
    return branchType;
}

void checkCondition(Symtable *table, BodyContext *context, ASTNode **condition, ErrorCode *error) {
    const Type *conditionType = getRealType(checkNode(table, context, condition, true, error));

    if (*error != ERR_OK) {
        return;
    }

    if (conditionType == NULL || conditionType->kind != TY_BOOL) {
        *error = ERR_TYPE;
    }
}

Type *checkIf(Symtable *table, BodyContext *context, IfNode *ifNode, bool isValueUsed,
              ErrorCode *error) {
    checkCondition(table, context, &ifNode->condition, error);

    Type *thenType = checkBranch(table, context, &ifNode->thenBranch, isValueUsed, error);
    Type *elseType = type_get_basic(TY_UNIT);

    // chýbajúce else sa správa ako vetva s ()
    if (ifNode->elseBranch != NULL) {
        elseType = checkBranch(table, context, &ifNode->elseBranch, isValueUsed, error);
    }

    if (*error != ERR_OK) {
        return NULL;
    }

    if (!isValueUsed) {
        if (thenType == NULL && elseType == NULL) {
            return NULL;
        }
        return type_get_basic(TY_UNIT);
    }

    // vetva, ktora konci returnom, typ neurcuje
    if (thenType == NULL) {
        return elseType;
    }
    if (elseType == NULL) {
        return thenType;
    }

    if (type_equals(thenType, elseType)) {
        return thenType;
    }

    const Type *realThenType = getRealType(thenType);
    const Type *realElseType = getRealType(elseType);

    if (isNumericType(realThenType) && isNumericType(realElseType)) {
        // jedna vetva je Int, druha Double, Int vetva sa prevedie
        Type *doubleType = type_get_basic(TY_DOUBLE);

        if (realThenType->kind == TY_INT) {
            acceptValue(doubleType, &ifNode->thenBranch, error);
        } else {
            acceptValue(doubleType, &ifNode->elseBranch, error);
        }
        return doubleType;
    }

    *error = ERR_TYPE;
    return NULL;
}

Type *checkWhile(Symtable *table, BodyContext *context, WhileNode *whileNode, ErrorCode *error) {
    checkCondition(table, context, &whileNode->condition, error);

    // hodnoty z tela cyklu sa neprenasaju dalej
    checkBranch(table, context, &whileNode->body, false, error);

    if (*error != ERR_OK) {
        return NULL;
    }

    return type_get_basic(TY_UNIT);
}

/*
 * Použitie AI: návratové typy (isUnitType … checkFunctionResult)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
bool isUnitType(Type *type) {
    type = getRealType(type);
    return type != NULL && type->kind == TY_UNIT;
}

Type *checkReturn(Symtable *table, BodyContext *context, ReturnNode *returnNode, ErrorCode *error) {
    // v tele lambdy return nesmie byť
    if (context->lambda != NULL) {
        *error = ERR_SEM_OTHER;
        return NULL;
    }

    Type *returnType = context->function->returnType;

    // return vyzaduje uvedeny navratovy typ
    if (returnType == NULL) {
        *error = ERR_TYPE;
        return NULL;
    }

    bool isUnitDeclared = isUnitType(returnType);

    // holy return vracia () a smie byt len pri Unit
    if (returnNode->returnValue == NULL) {
        if (!isUnitDeclared) {
            *error = ERR_TYPE;
        }
        return NULL;
    }

    checkNode(table, context, &returnNode->returnValue, true, error);
    if (*error != ERR_OK) {
        return NULL;
    }

    if (!isUnitDeclared && !acceptValue(returnType, &returnNode->returnValue, error)) {
        *error = ERR_TYPE;
    }

    return NULL;
}

bool acceptResult(Type *expected, ASTNode **node, ErrorCode *error) {
    ASTNode *current = *node;

    // vysledok, ktory konci returnom, sa kontroloval v return
    if (current->type == NULL) {
        return true;
    }

    if (current->kind == AST_BLOCK) {
        BlockNode *block = (BlockNode *)current;

        if (block->commandOrderCount == 0) {
            return type_accepts(expected, type_get_basic(TY_UNIT));
        }
        return acceptResult(expected, &block->commandOrder[block->commandOrderCount - 1], error);
    }
    if (current->kind == AST_EXPR_STATEMENT) {
        return acceptResult(expected, &((LonelyExprNode *)current)->expr, error);
    }
    if (current->kind == AST_IF) {
        IfNode *ifNode = (IfNode *)current;
        bool isThenAccepted = acceptResult(expected, &ifNode->thenBranch, error);

        // chýbajúce else je vetva s ()
        if (ifNode->elseBranch == NULL) {
            return isThenAccepted && type_accepts(expected, type_get_basic(TY_UNIT));
        }
        return isThenAccepted && acceptResult(expected, &ifNode->elseBranch, error);
    }
    if (current->kind == AST_MATCH) {
        MatchNode *match = (MatchNode *)current;
        bool areCasesAccepted = true;

        for (size_t caseIndex = 0; caseIndex < match->casesCount; caseIndex++) {
            CaseNode *caseNode = (CaseNode *)match->cases[caseIndex];

            if (!acceptResult(expected, &caseNode->body, error)) {
                areCasesAccepted = false;
            }
        }
        return areCasesAccepted;
    }

    return acceptValue(expected, node, error);
}

void checkFunctionResult(Symbol *functionSymbol, Type *bodyType, ErrorCode *error) {
    if (*error != ERR_OK) {
        return;
    }

    DefFuncNode *function = (DefFuncNode *)functionSymbol->definition;

    // neuvedeny typ sa odvodi z vysledku tela
    if (function->returnType == NULL) {
        functionSymbol->type->func.ret = bodyType;
        return;
    }

    if (isUnitType(function->returnType)) {
        return;
    }

    if (!acceptResult(function->returnType, &function->body, error) && *error == ERR_OK) {
        *error = ERR_TYPE;
    }
}

/*
 * Použitie AI: volania funkcií (getFunctionReturnType, checkArguments, checkCall)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
Type *getFunctionReturnType(Symtable *table, BodyContext *context, Symbol *functionSymbol,
                            ErrorCode *error) {
    if (functionSymbol->kind == SYM_BUILTIN) {
        return functionSymbol->type->func.ret;
    }

    DefFuncNode *function = (DefFuncNode *)functionSymbol->definition;

    if (functionSymbol->checkState == FUNC_IN_PROGRESS) {
        // rekurzia, kazda funkcia v kruhu volani musi mat uvedeny typ
        for (BodyContext *current = context; current != NULL; current = current->parent) {
            if (current->function->returnType == NULL) {
                *error = ERR_TYPE;
                return NULL;
            }
            if (current->function == function) {
                break;
            }
        }
    } else {
        // telo sa skontroluje hned, aj ked typ je uvedeny (kvoli nepriamej rekurzii)
        checkFunctionBody(table, context, functionSymbol, error);
    }

    if (*error != ERR_OK) {
        return NULL;
    }

    if (function->returnType != NULL) {
        return function->returnType;
    }
    return functionSymbol->type->func.ret;
}

void checkArguments(Symtable *table, BodyContext *context, CallNode *call, Type **paramTypes,
                    size_t paramCount, ErrorCode *error) {
    if (call->argsCount != paramCount) {
        *error = ERR_PARAMS;
        return;
    }

    for (size_t argIndex = 0; argIndex < call->argsCount; argIndex++) {
        checkNode(table, context, &call->args[argIndex], true, error);
        if (*error != ERR_OK) {
            return;
        }

        if (!acceptValue(paramTypes[argIndex], &call->args[argIndex], error)) {
            *error = ERR_PARAMS;
        }
        if (*error != ERR_OK) {
            return;
        }
    }
}

Type *checkCall(Symtable *table, BodyContext *context, CallNode *call, ErrorCode *error) {
    // pocet zoznamov argumentov f(a)(b)
    size_t listCount = 1;
    ASTNode *caller = call->caller;

    while (caller->kind == AST_CALL) {
        listCount++;
        caller = ((CallNode *)caller)->caller;
    }

    // premenná alebo parameter s funkciou sa vola ako hodnota
    if (caller->kind == AST_ID) {
        const Symbol *callerSymbol = lookupSymbol(table, ((IDNode *)caller)->name);

        if (callerSymbol != NULL &&
            (callerSymbol->kind == SYM_VARIABLE || callerSymbol->kind == SYM_PARAMETER)) {
            return checkValueCall(table, context, call, error);
        }
    }

    Symbol *callee = NULL;
    bool isListStatic = false;

    if (caller->kind == AST_ID) {
        callee = checkFunctionCallee(table, (IDNode *)caller, error);
    } else if (caller->kind == AST_MEMBER) {
        MemberNode *member = (MemberNode *)caller;

        if (isListStaticAccess(table, member)) {
            callee = findListStaticFunction(table, member, error);
            isListStatic = true;
        } else {
            Type *objectType = checkMemberObject(table, context, member, error);

            if (objectType == NULL) {
                return NULL;
            }
            if (objectType->kind == TY_LIST && isListMethodName(member->name)) {
                return checkListMethodCall(table, context, call, member, objectType, listCount,
                                           error);
            }
            callee = checkMethodCallee(table, member, objectType, error);
        }
    } else {
        // ina hodnota (zoznam, lambda) sa vola ako hodnota
        return checkValueCall(table, context, call, error);
    }

    if (callee == NULL) {
        return NULL;
    }

    // funkcia, ktora vracia funkciu: zoznam navyše patrí volaniu výsledku, goo()(1)
    if (callee->kind == SYM_FUNCTION &&
        listCount > ((DefFuncNode *)callee->definition)->paramListCount) {
        const Type *returnType = getRealType(getFunctionReturnType(table, context, callee, error));

        if (*error != ERR_OK) {
            return NULL;
        }
        if (returnType != NULL && returnType->kind == TY_FUNC) {
            return checkValueCall(table, context, call, error);
        }
    }

    // zoznamy volani od najvnutornejsieho, ten patri k prvemu zoznamu parametrov
    CallNode **calls = malloc(listCount * sizeof(CallNode *));
    if (calls == NULL) {
        *error = ERR_INTERNAL;
        return NULL;
    }

    CallNode *current = call;
    for (size_t listIndex = listCount; listIndex > 0; listIndex--) {
        calls[listIndex - 1] = current;
        current->symbol = callee;
        current->callKind = CALL_FUNC;
        if (callee->kind == SYM_BUILTIN) {
            current->callKind = CALL_BUILTIN;
        }
        if (callee->kind == SYM_EXTENSION) {
            current->callKind = CALL_EXT_METHOD;
        }
        if (callee->kind == SYM_CLASS) {
            current->callKind = CALL_CONSTRUCTOR;
        }
        if (listIndex > 1) {
            current = (CallNode *)current->caller;
        }
    }

    if (callee->kind == SYM_BUILTIN) {
        // vstavane funkcie maju jeden zoznam parametrov
        if (listCount != 1) {
            *error = ERR_PARAMS;
        } else {
            size_t paramCount = callee->type->func.params.len;

            // List.range ma krok nepovinný
            if (isListStatic && call->argsCount + 1 == paramCount) {
                paramCount--;
            }
            checkArguments(table, context, call, callee->type->func.params.items, paramCount,
                           error);
        }
    } else if (callee->kind == SYM_CLASS) {
        // konštruktor ma jeden zoznam, argumenty su atributy triedy
        if (listCount != 1) {
            *error = ERR_PARAMS;
        } else {
            checkConstructorArguments(table, context, call, (ClassDefNode *)callee->definition,
                                      error);
        }
    } else {
        DefFuncNode *function = (DefFuncNode *)callee->definition;

        if (listCount != function->paramListCount) {
            *error = ERR_PARAMS;
        }

        // typy parametrov su v type funkcie za sebou, zoznam po zozname
        size_t firstParamIndex = 0;

        for (size_t listIndex = 0; listIndex < listCount && *error == ERR_OK; listIndex++) {
            size_t paramCount = function->paramList[listIndex].params_count;
            checkArguments(table, context, calls[listIndex],
                           callee->type->func.params.items + firstParamIndex, paramCount, error);
            firstParamIndex += paramCount;
        }
    }

    free(calls);

    if (*error != ERR_OK) {
        return NULL;
    }
    if (callee->kind == SYM_CLASS) {
        return callee->type;
    }

    return getFunctionReturnType(table, context, callee, error);
}

/*
 * Použitie AI: cyklus for (isZeroLiteral … checkFor)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
bool isZeroLiteral(ASTNode *step) {
    // -0 a +0 su unarne operacie nad literalom
    while (step->kind == AST_UNARY) {
        step = ((UnaryNode *)step)->right;
    }

    if (step->kind != AST_LIT) {
        return false;
    }

    LiteralNode *literal = (LiteralNode *)step;
    return literal->litType == LIT_INT && literal->litValue.int_value == 0;
}

void checkRangeBound(Symtable *table, BodyContext *context, ASTNode **bound, ErrorCode *error) {
    const Type *boundType = getRealType(checkNode(table, context, bound, true, error));

    if (*error != ERR_OK) {
        return;
    }

    if (boundType == NULL || boundType->kind != TY_INT) {
        *error = ERR_TYPE;
    }
}

Type *checkRange(Symtable *table, BodyContext *context, RangeNode *range, ErrorCode *error) {
    checkRangeBound(table, context, &range->from, error);
    checkRangeBound(table, context, &range->to, error);

    if (range->step != NULL) {
        checkRangeBound(table, context, &range->step, error);

        // nenulovost kroku sa da overit len pri literali, inak az za behu
        if (*error == ERR_OK && isZeroLiteral(range->step)) {
            *error = ERR_SEM_OTHER;
        }
    }

    if (*error != ERR_OK) {
        return NULL;
    }

    return type_get_basic(TY_RANGE);
}

Type *checkFor(Symtable *table, BodyContext *context, ForNode *forNode, ErrorCode *error) {
    Type *sourceType = getRealType(checkNode(table, context, &forNode->source, true, error));

    if (*error != ERR_OK) {
        return NULL;
    }

    // rozsah dáva Int, zoznam typ svojich prvkov
    Type *loopType = type_get_basic(TY_INT);

    if (sourceType != NULL && sourceType->kind == TY_LIST) {
        loopType = sourceType->elem;
    } else if (sourceType == NULL || sourceType->kind != TY_RANGE) {
        loopType = NULL;
    }

    if (loopType == NULL) {
        *error = ERR_TYPE;
        return NULL;
    }

    // riadiaca premenna plati v strazach aj v tele, nie za cyklom
    pushScope(table, error);
    if (*error != ERR_OK) {
        return NULL;
    }

    Symbol *loopSymbol =
        defineSymbol(table, forNode->varName, SYM_VARIABLE, loopType, false, error);
    if (loopSymbol != NULL) {
        forNode->symbol = loopSymbol;
        addContextLocal(context, loopSymbol, error);
    }

    for (size_t guardIndex = 0; guardIndex < forNode->guardsCount; guardIndex++) {
        if (*error != ERR_OK) {
            break;
        }
        checkCondition(table, context, &forNode->guards[guardIndex], error);
    }

    checkBranch(table, context, &forNode->body, false, error);

    popScope(table);

    if (*error != ERR_OK) {
        return NULL;
    }

    return type_get_basic(TY_UNIT);
}

/*
 * Použitie AI: členy hodnôt (getBuiltinMemberType, checkMember)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
Type *getBuiltinMemberType(const Type *objectType, const char *name) {
    // toString má kazda hodnota
    if (strcmp(name, "toString") == 0) {
        return type_get_basic(TY_STRING);
    }
    if (objectType->kind == TY_DOUBLE && strcmp(name, "toInt") == 0) {
        return type_get_basic(TY_INT);
    }
    if (objectType->kind == TY_DOUBLE && strcmp(name, "floor") == 0) {
        return type_get_basic(TY_DOUBLE);
    }
    if (objectType->kind == TY_STRING && strcmp(name, "length") == 0) {
        return type_get_basic(TY_INT);
    }

    return NULL;
}

Type *checkMember(Symtable *table, BodyContext *context, MemberNode *member, ErrorCode *error) {
    // List.range sa da len zavolat
    if (isListStaticAccess(table, member)) {
        if (findListStaticFunction(table, member, error) != NULL) {
            *error = ERR_PARAMS;
        }
        return NULL;
    }

    Type *objectType = checkMemberObject(table, context, member, error);

    if (objectType == NULL) {
        return NULL;
    }

    Type *memberType = getBuiltinMemberType(objectType, member->name);

    if (memberType != NULL) {
        member->memberKind = MEM_BUILTIN;
        return memberType;
    }

    if (objectType->kind == TY_CLASS) {
        Type *fieldType = getFieldType(objectType->cls, member->name);

        // symbol je trieda, atribut sa v nej hlada podla mena
        if (fieldType != NULL) {
            member->memberKind = MEM_FIELD;
            member->symbol = objectType->cls;
            return fieldType;
        }
    }

    Type *listMemberType = getListMemberType(objectType, member->name);

    if (listMemberType != NULL) {
        member->memberKind = MEM_BUILTIN;
        return listMemberType;
    }

    // metóda zoznamu s argumentmi sa bez zatvoriek volat neda
    if (objectType->kind == TY_LIST && isListMethodName(member->name)) {
        *error = ERR_PARAMS;
        return NULL;
    }

    Symbol *method = findExtensionMethod(table, objectType, member->name, error);

    // neznámy clen je ako nedefinovaná funkcia
    if (method == NULL) {
        if (*error == ERR_OK) {
            *error = ERR_UNDEF;
        }
        return NULL;
    }

    // bez zátvoriek sa da volat len metoda bez zoznamu parametrov
    if (((DefFuncNode *)method->definition)->paramListCount != 0) {
        *error = ERR_PARAMS;
        return NULL;
    }

    member->memberKind = MEM_EXT_METHOD;
    member->symbol = method;
    return getFunctionReturnType(table, context, method, error);
}

/*
 * Použitie AI: rozširujúce metódy (findExtensionMethod … checkMethodCallee)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
Symbol *findExtensionMethod(Symtable *table, Type *receiverType, const char *name,
                            ErrorCode *error) {
    const char *typeName = getTypeKeyName(receiverType);
    char *key = malloc(strlen(typeName) + strlen(name) + 2);

    if (key == NULL) {
        *error = ERR_INTERNAL;
        return NULL;
    }

    // rovnaký klúč ako pri vkladaní metody, "Typ.meno"
    strcpy(key, typeName);
    strcat(key, ".");
    strcat(key, name);

    Symbol *method = lookupSymbolInTree(table->extensionTree, key);
    free(key);
    return method;
}

Symbol *checkFunctionCallee(Symtable *table, IDNode *calleeId, ErrorCode *error) {
    Symbol *callee = lookupSymbol(table, calleeId->name);

    if (callee == NULL) {
        *error = ERR_UNDEF;
        return NULL;
    }
    if (callee->kind != SYM_FUNCTION && callee->kind != SYM_BUILTIN && callee->kind != SYM_CLASS) {
        *error = ERR_TYPE;
        return NULL;
    }

    calleeId->symbol = callee;
    calleeId->node.type = callee->type;
    return callee;
}

Symbol *checkMethodCallee(Symtable *table, MemberNode *member, Type *objectType, ErrorCode *error) {
    Symbol *method = findExtensionMethod(table, objectType, member->name, error);

    if (method == NULL) {
        if (*error != ERR_OK) {
            return NULL;
        }

        // vstavany člen sa vola bez zatvoriek, neznamy člen je nedefinovaný
        if (getBuiltinMemberType(objectType, member->name) != NULL ||
            getListMemberType(objectType, member->name) != NULL) {
            *error = ERR_PARAMS;
        } else {
            *error = ERR_UNDEF;
        }
        return NULL;
    }

    member->memberKind = MEM_CALLEE;
    member->symbol = method;
    member->node.type = method->type;
    return method;
}

/*
 * Použitie AI: case class a match (checkConstructorArguments … checkMatch)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
void checkConstructorArguments(Symtable *table, BodyContext *context, CallNode *call,
                               ClassDefNode *classDef, ErrorCode *error) {
    size_t fieldCount = classDef->fields.params_count;
    Type **fieldTypes = malloc((fieldCount + 1) * sizeof(Type *));

    if (fieldTypes == NULL) {
        *error = ERR_INTERNAL;
        return;
    }

    for (size_t fieldIndex = 0; fieldIndex < fieldCount; fieldIndex++) {
        fieldTypes[fieldIndex] = classDef->fields.params[fieldIndex].type;
    }

    checkArguments(table, context, call, fieldTypes, fieldCount, error);
    free(fieldTypes);
}

Type *getFieldType(Symbol *classSymbol, const char *name) {
    ClassDefNode *classDef = (ClassDefNode *)classSymbol->definition;

    for (size_t fieldIndex = 0; fieldIndex < classDef->fields.params_count; fieldIndex++) {
        if (strcmp(classDef->fields.params[fieldIndex].name, name) == 0) {
            return classDef->fields.params[fieldIndex].type;
        }
    }

    return NULL;
}

void defineCaseBinder(Symtable *table, BodyContext *context, Param *binder, Type *type,
                      ErrorCode *error) {
    // väzba _ hodnotu ignoruje, premenna nevznikne
    if (strcmp(binder->name, "_") == 0) {
        return;
    }

    // premenna zo vzoru je nemodifikovatelna, plati len vo vetve
    binder->symbol = defineSymbol(table, binder->name, SYM_VARIABLE, type, false, error);

    if (binder->symbol != NULL) {
        addContextLocal(context, binder->symbol, error);
    }
}

void checkCasePattern(Symtable *table, BodyContext *context, CaseNode *caseNode, Type *subjectType,
                      ErrorCode *error) {
    if (caseNode->patternKind == PAT_LITERAL) {
        checkNode(table, context, &caseNode->literal, true, error);

        if (*error == ERR_OK && !acceptValue(subjectType, &caseNode->literal, error)) {
            *error = ERR_TYPE;
        }
    } else if (caseNode->patternKind == PAT_TYPED) {
        resolveType(table, caseNode->binder.type, error);
        if (*error != ERR_OK) {
            return;
        }

        // typ vo vzore musí byť jeden z možných typov hodnoty
        if (!type_accepts(subjectType, caseNode->binder.type)) {
            *error = ERR_TYPE;
            return;
        }

        defineCaseBinder(table, context, &caseNode->binder, caseNode->binder.type, error);
    } else if (caseNode->patternKind == PAT_CONSTRUCTOR) {
        Symbol *classSymbol = lookupSymbolInTree(table->typeTree, caseNode->className);

        if (classSymbol == NULL || classSymbol->kind != SYM_CLASS) {
            *error = ERR_UNDEF;
            return;
        }

        caseNode->classSymbol = classSymbol;

        if (!type_accepts(subjectType, classSymbol->type)) {
            *error = ERR_TYPE;
            return;
        }

        ClassDefNode *classDef = (ClassDefNode *)classSymbol->definition;
        ParamList *binders = &caseNode->ctorBinders;

        // kazdy atribút ma práve jednu väzbu, ako pri volaní konštruktora
        if (binders->params_count != classDef->fields.params_count) {
            *error = ERR_PARAMS;
            return;
        }

        for (size_t binderIndex = 0; binderIndex < binders->params_count; binderIndex++) {
            if (*error != ERR_OK) {
                return;
            }
            defineCaseBinder(table, context, &binders->params[binderIndex],
                             classDef->fields.params[binderIndex].type, error);
        }
    }
}

Type *combineCaseTypes(MatchNode *match, Type **caseTypes, ErrorCode *error) {
    Type *resultType = NULL;
    bool areAllEqual = true;
    bool areAllNumeric = true;
    bool hasDouble = false;

    for (size_t caseIndex = 0; caseIndex < match->casesCount; caseIndex++) {
        Type *caseType = caseTypes[caseIndex];

        // vetva, ktorá konci returnom, typ neurčuje
        if (caseType == NULL) {
            continue;
        }

        if (resultType == NULL) {
            resultType = caseType;
        } else if (!type_equals(resultType, caseType)) {
            areAllEqual = false;
        }

        const Type *realType = getRealType(caseType);

        if (!isNumericType(realType)) {
            areAllNumeric = false;
        } else if (realType->kind == TY_DOUBLE) {
            hasDouble = true;
        }
    }

    if (resultType == NULL || areAllEqual) {
        return resultType;
    }

    if (areAllNumeric && hasDouble) {
        Type *doubleType = type_get_basic(TY_DOUBLE);

        for (size_t caseIndex = 0; caseIndex < match->casesCount; caseIndex++) {
            if (caseTypes[caseIndex] != NULL) {
                acceptValue(doubleType, &((CaseNode *)match->cases[caseIndex])->body, error);
            }
        }
        return doubleType;
    }

    *error = ERR_TYPE;
    return NULL;
}

Type *checkMatch(Symtable *table, BodyContext *context, MatchNode *match, bool isValueUsed,
                 ErrorCode *error) {
    // match musi byt vycerpávajúci, staci vyzadovat case _
    bool hasWildcard = false;

    for (size_t caseIndex = 0; caseIndex < match->casesCount; caseIndex++) {
        if (((CaseNode *)match->cases[caseIndex])->patternKind == PAT_WILDCARD) {
            hasWildcard = true;
        }
    }

    if (!hasWildcard) {
        *error = ERR_SEM_OTHER;
        return NULL;
    }

    Type *subjectType = checkNode(table, context, &match->subject, true, error);

    if (*error != ERR_OK) {
        return NULL;
    }
    if (subjectType == NULL) {
        *error = ERR_TYPE;
        return NULL;
    }

    Type **caseTypes = malloc(match->casesCount * sizeof(Type *));

    if (caseTypes == NULL) {
        *error = ERR_INTERNAL;
        return NULL;
    }

    bool areAllReturning = true;

    for (size_t caseIndex = 0; caseIndex < match->casesCount && *error == ERR_OK; caseIndex++) {
        CaseNode *caseNode = (CaseNode *)match->cases[caseIndex];

        // premenne zo vzoru platia v strazi aj v tele vetvy
        pushScope(table, error);
        if (*error != ERR_OK) {
            break;
        }

        checkCasePattern(table, context, caseNode, subjectType, error);

        if (*error == ERR_OK && caseNode->guard != NULL) {
            checkCondition(table, context, &caseNode->guard, error);
        }

        caseTypes[caseIndex] = checkNode(table, context, &caseNode->body, isValueUsed, error);

        if (caseTypes[caseIndex] != NULL) {
            areAllReturning = false;
        }

        popScope(table);
    }

    Type *resultType = NULL;

    if (*error == ERR_OK) {
        if (!isValueUsed) {
            if (!areAllReturning) {
                resultType = type_get_basic(TY_UNIT);
            }
        } else {
            resultType = combineCaseTypes(match, caseTypes, error);
        }
    }

    free(caseTypes);
    return resultType;
}

/*
 * Použitie AI: interpolované reťazce (checkInterpolation)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
Type *checkInterpolation(Symtable *table, BodyContext *context, InterpNode *interp,
                         ErrorCode *error) {
    for (size_t partIndex = 0; partIndex < interp->partsCount; partIndex++) {
        const Type *partType = checkNode(table, context, &interp->parts[partIndex], true, error);

        if (*error != ERR_OK) {
            return NULL;
        }

        // cast, ktorá končí returnom, nemá hodnotu na vloženie
        if (partType == NULL) {
            *error = ERR_TYPE;
            return NULL;
        }
    }

    return type_get_basic(TY_STRING);
}

/*
 * Použitie AI: lambdy (addContextLocal, checkLambda, checkValueCall)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
void addContextLocal(BodyContext *context, Symbol *local, ErrorCode *error) {
    if (context->lambda == NULL) {
        addLocal(context->function, local, error);
        return;
    }

    LambdaNode *lambda = context->lambda;
    Symbol **newLocals = realloc(lambda->locals, (lambda->localsCount + 1) * sizeof(Symbol *));

    if (newLocals == NULL) {
        *error = ERR_INTERNAL;
        return;
    }

    lambda->locals = newLocals;
    lambda->locals[lambda->localsCount] = local;
    lambda->localsCount++;
}

Type *checkLambda(Symtable *table, BodyContext *context, LambdaNode *lambda, ErrorCode *error) {
    resolveParamList(table, &lambda->params, error);
    if (*error != ERR_OK) {
        return NULL;
    }

    // lambda nevidi lokalne premenné okolitej funkcie
    Scope *savedTop = table->top;
    table->top = getGlobalScope(table);

    pushScope(table, error);
    if (*error != ERR_OK) {
        table->top = savedTop;
        return NULL;
    }

    TypeList paramTypes = {NULL, 0, 0};

    for (size_t paramIndex = 0; paramIndex < lambda->params.params_count; paramIndex++) {
        Param *param = &lambda->params.params[paramIndex];
        type_list_append(&paramTypes, param->type);

        if (*error == ERR_OK) {
            param->symbol =
                defineSymbol(table, param->name, SYM_PARAMETER, param->type, false, error);
        }
    }

    BodyContext lambdaContext = {context->function, context, lambda};
    Type *bodyType = checkNode(table, &lambdaContext, &lambda->body, true, error);

    popScope(table);
    table->top = savedTop;

    if (*error != ERR_OK) {
        free(paramTypes.items);
        return NULL;
    }

    return type_create_function(paramTypes, bodyType);
}

Type *checkValueCall(Symtable *table, BodyContext *context, CallNode *call, ErrorCode *error) {
    Type *calleeType = getRealType(checkNode(table, context, &call->caller, true, error));

    if (*error != ERR_OK) {
        return NULL;
    }

    // zoznam sa indexuje syntaxou volania, lst(i)
    if (calleeType != NULL && calleeType->kind == TY_LIST) {
        Type *indexType = type_get_basic(TY_INT);

        checkArguments(table, context, call, &indexType, 1, error);
        if (*error != ERR_OK) {
            return NULL;
        }
        if (calleeType->elem == NULL) {
            *error = ERR_TYPE;
            return NULL;
        }

        call->callKind = CALL_LIST_INDEX;
        return calleeType->elem;
    }

    // volat sa da len hodnota typu funkcia
    if (calleeType == NULL || calleeType->kind != TY_FUNC) {
        *error = ERR_TYPE;
        return NULL;
    }

    checkArguments(table, context, call, calleeType->func.params.items, calleeType->func.params.len,
                   error);
    if (*error != ERR_OK) {
        return NULL;
    }

    call->callKind = CALL_VALUE;
    return calleeType->func.ret;
}

/*
 * Použitie AI: zoznamy (checkMemberObject … checkListLiteral)
 *   vygenerovala AI z môjho pseudokódu, kód som skontroloval (prompt viď hlavičku súboru).
 */
Type *checkMemberObject(Symtable *table, BodyContext *context, MemberNode *member,
                        ErrorCode *error) {
    Type *objectType = getRealType(checkNode(table, context, &member->object, true, error));

    if (*error != ERR_OK) {
        return NULL;
    }

    // hodnota, ktorá konci returnom, nemá cleny
    if (objectType == NULL) {
        *error = ERR_TYPE;
    }

    return objectType;
}

bool isListStaticAccess(Symtable *table, MemberNode *member) {
    if (member->object->kind != AST_ID) {
        return false;
    }

    const IDNode *objectId = (const IDNode *)member->object;
    return strcmp(objectId->name, "List") == 0 && lookupSymbol(table, "List") == NULL;
}

Symbol *findListStaticFunction(Symtable *table, MemberNode *member, ErrorCode *error) {
    char *key = malloc(strlen("List.") + strlen(member->name) + 1);

    if (key == NULL) {
        *error = ERR_INTERNAL;
        return NULL;
    }

    // vstavana funkcia je v tabuľke pod menom "List.range"
    strcpy(key, "List.");
    strcat(key, member->name);
    Symbol *function = lookupSymbol(table, key);
    free(key);

    if (function == NULL || function->kind != SYM_BUILTIN) {
        *error = ERR_UNDEF;
        return NULL;
    }

    member->memberKind = MEM_CALLEE;
    member->symbol = function;
    return function;
}

bool isListMethodName(const char *name) {
    return strcmp(name, "drop") == 0 || strcmp(name, "take") == 0 || strcmp(name, "slice") == 0 ||
           strcmp(name, "filter") == 0 || strcmp(name, "takeWhile") == 0 ||
           strcmp(name, "dropWhile") == 0;
}

Type *getListMemberType(Type *objectType, const char *name) {
    if (objectType->kind == TY_LIST && strcmp(name, "head") == 0) {
        return objectType->elem;
    }
    if (objectType->kind == TY_LIST && strcmp(name, "tail") == 0) {
        return objectType;
    }
    // rozsah 1 to 5 sa da previest na zoznam
    if (objectType->kind == TY_RANGE && strcmp(name, "toList") == 0) {
        return type_create_list(type_get_basic(TY_INT));
    }

    return NULL;
}

Type *checkListMethodCall(Symtable *table, BodyContext *context, CallNode *call, MemberNode *member,
                          Type *listType, size_t listCount, ErrorCode *error) {
    if (listCount != 1) {
        *error = ERR_PARAMS;
        return NULL;
    }

    Type *paramTypes[2];
    size_t paramCount = 1;

    if (strcmp(member->name, "drop") == 0 || strcmp(member->name, "take") == 0) {
        paramTypes[0] = type_get_basic(TY_INT);
    } else if (strcmp(member->name, "slice") == 0) {
        paramTypes[0] = type_get_basic(TY_INT);
        paramTypes[1] = type_get_basic(TY_INT);
        paramCount = 2;
    } else {
        // filter, takeWhile a dropWhile berú funkciu (T) => Boolean
        TypeList predicateParams = {NULL, 0, 0};
        type_list_append(&predicateParams, listType->elem);
        paramTypes[0] = type_create_function(predicateParams, type_get_basic(TY_BOOL));
    }

    checkArguments(table, context, call, paramTypes, paramCount, error);
    if (*error != ERR_OK) {
        return NULL;
    }

    call->callKind = CALL_LIST_METHOD;
    member->memberKind = MEM_CALLEE;
    return listType;
}

Type *checkListLiteral(Symtable *table, BodyContext *context, ListLitNode *list, ErrorCode *error) {
    if (list->elemsCount == 0) {
        return type_create_list(NULL);
    }

    Type *elemType = NULL;
    bool areAllEqual = true;
    bool areAllNumeric = true;
    bool hasDouble = false;

    for (size_t elemIndex = 0; elemIndex < list->elemsCount; elemIndex++) {
        Type *currentType = checkNode(table, context, &list->elems[elemIndex], true, error);

        if (*error != ERR_OK) {
            return NULL;
        }
        if (currentType == NULL) {
            *error = ERR_TYPE;
            return NULL;
        }

        if (elemType == NULL) {
            elemType = currentType;
        } else if (!type_equals(elemType, currentType)) {
            areAllEqual = false;
        }

        const Type *realType = getRealType(currentType);

        if (!isNumericType(realType)) {
            areAllNumeric = false;
        } else if (realType->kind == TY_DOUBLE) {
            hasDouble = true;
        }
    }

    if (areAllEqual) {
        return type_create_list(elemType);
    }

    // zoznam je homogénny, jedine Int prvky sa da previest na Double
    if (areAllNumeric && hasDouble) {
        Type *doubleType = type_get_basic(TY_DOUBLE);

        for (size_t elemIndex = 0; elemIndex < list->elemsCount; elemIndex++) {
            acceptValue(doubleType, &list->elems[elemIndex], error);
        }
        return type_create_list(doubleType);
    }

    *error = ERR_TYPE;
    return NULL;
}

/**
 * @file semantic.c
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

#include "semantic.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief funkcia na vloženie jednej vstavanej funkcie
 * @param table je tabuľka symbolov
 * @param error je pointer, cez ktorý si vrátim chybové hlásenie
 * @param name je meno funkcie
 * @param returnType je návratový typ
 * @param paramCount je počet parametrov, za ním nasledujú ich typy (Type *)
 */
void addBuiltin(Symtable *table, ErrorCode *error, const char *name, Type *returnType,
                       int paramCount, ...){
    if (*error != ERR_OK){
        return;
    }

    TypeList params = {NULL, 0, 0};
    va_list paramTypes;
    
    va_start(paramTypes, paramCount);

    for (int paramIndex = 0; paramIndex < paramCount; paramIndex++){
        type_list_append(&params, va_arg(paramTypes, Type *));
    }

    va_end(paramTypes);
    defineSymbol(table, name, SYM_BUILTIN, type_create_function(params, returnType), false, error);
}

void addBuiltins(Symtable *table, ErrorCode *error){
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
    addBuiltin(table, error, "List.range", type_create_list(intType), 3, intType, intType,
               intType);
}

Type *createFunctionType(DefFuncNode *function){
    // parametre zo všetkých zoznamov dáme za sebou
    TypeList paramTypes = {NULL, 0, 0};

    for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++){
        ParamList *paramList = &function->paramList[listIndex];

        for (size_t paramIndex = 0; paramIndex < paramList->params_count; paramIndex++){
            type_list_append(&paramTypes, paramList->params[paramIndex].type);
        }
    }

    return type_create_function(paramTypes, function->returnType);
}

bool checkParamNames(Symtable *table, DefFuncNode *function, Param *receiver, ErrorCode *error){
    bool hasParameter = false;

    // rovnaké meno parametra aj v inom zozname je chyba 4
    pushScope(table, error);
    if (*error != ERR_OK){
        return false;
    }

    if (receiver != NULL){
        defineSymbol(table, receiver->name, SYM_PARAMETER, receiver->type, false, error);
    }

    for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++){
        ParamList *paramList = &function->paramList[listIndex];

        for (size_t paramIndex = 0; paramIndex < paramList->params_count; paramIndex++){
            Param *param = &paramList->params[paramIndex];
            hasParameter = true;

            if (*error == ERR_OK){
                defineSymbol(table, param->name, SYM_PARAMETER, param->type, false, error);
            }
        }
    }

    popScope(table);
    return hasParameter;
}

void collectFunction(Symtable *table, DefFuncNode *function, int *mainCount, ErrorCode *error){
    if (*error != ERR_OK){
        return;
    }

    Symbol *functionSymbol = defineSymbol(table, function->name, SYM_FUNCTION,
                                          createFunctionType(function), false, error);
    if (functionSymbol == NULL){
        return;
    }

    functionSymbol->definition = (ASTNode *)function;
    function->symbol = functionSymbol;

    bool hasParameter = checkParamNames(table, function, NULL, error);

    if (*error != ERR_OK || !function->isMain){
        return;
    }

    (*mainCount)++;

    if (*mainCount > 1){
        *error = ERR_REDEF;
        return;
    }

    if (hasParameter){
        *error = ERR_PARAMS;
        return;
    }

    // neuvedený typ a alias skontrolujeme až neskôr
    if (function->returnType != NULL && function->returnType->kind != TY_UNIT
        && function->returnType->kind != TY_NAMED){
        *error = ERR_TYPE;
    }
}

void collectClass(Symtable *table, ClassDefNode *classDef, ErrorCode *error){
    if (*error != ERR_OK){
        return;
    }

    Symbol *classSymbol = defineSymbolInTree(table, &table->typeTree, classDef->name, SYM_CLASS,
                                             NULL, false, error);
    if (classSymbol == NULL){
        return;
    }

    classSymbol->type = type_create_class(classSymbol);
    classSymbol->definition = (ASTNode *)classDef;
    classDef->symbol = classSymbol;

    // meno triedy je aj konstruktor, takže nesmie byť rovnaké ako funkcia
    table->top->tree = insertNode(table->top->tree, classDef->name, classSymbol, error);
    if (*error != ERR_OK){
        return;
    }

    // rovnake meno atributu je chyba 4
    pushScope(table, error);
    if (*error != ERR_OK){
        return;
    }

    for (size_t fieldIndex = 0; fieldIndex < classDef->fields.params_count; fieldIndex++){
        Param *field = &classDef->fields.params[fieldIndex];

        if (*error == ERR_OK){
            defineSymbol(table, field->name, SYM_PARAMETER, field->type, false, error);
        }
    }

    popScope(table);
}

void collectTypeAlias(Symtable *table, TypeAliasNode *alias, ErrorCode *error){
    if (*error != ERR_OK){
        return;
    }

    Symbol *aliasSymbol = defineSymbolInTree(table, &table->typeTree, alias->name, SYM_ALIAS,
                                             alias->aliased, false, error);
    if (aliasSymbol == NULL){
        return;
    }

    aliasSymbol->definition = (ASTNode *)alias;
    alias->symbol = aliasSymbol;
}

void collectGlobalDefinitions(Symtable *table, ProgramNode *program, ErrorCode *error){
    int mainCount = 0;

    for (size_t itemIndex = 0; itemIndex < program->itemsCount; itemIndex++){
        if (*error != ERR_OK){
            return;
        }

        ASTNode *item = program->items[itemIndex];

        if (item->kind == AST_FUNCTION_DEF){
            collectFunction(table, (DefFuncNode *)item, &mainCount, error);
        }
        else if (item->kind == AST_CLASS_DEF){
            collectClass(table, (ClassDefNode *)item, error);
        }
        else if (item->kind == AST_TYPE_ALIAS){
            collectTypeAlias(table, (TypeAliasNode *)item, error);
        }
        else if (item->kind == AST_EXTENSION){
            // metódy zaregistrujeme až po rozlísení typov, tu len parametre
            ExtensionNode *extension = (ExtensionNode *)item;

            for (size_t methodIndex = 0; methodIndex < extension->methodsCount; methodIndex++){
                if (*error == ERR_OK){
                    checkParamNames(table, (DefFuncNode *)extension->methods[methodIndex],
                                    &extension->receiver, error);
                }
            }
        }
    }

    if (*error == ERR_OK && mainCount == 0){
        *error = ERR_UNDEF;
    }
}

Type *getRealType(Type *type){
    while (type != NULL && type->kind == TY_NAMED && type->named.resolved != NULL){
        type = type->named.resolved;
    }

    return type;
}

void resolveAlias(Symtable *table, Symbol *aliasSymbol, ErrorCode *error){
    if (aliasSymbol->checkState == FUNC_DONE){
        return;
    }

    // alias, ktory sa odkazuje sam na seba
    if (aliasSymbol->checkState == FUNC_IN_PROGRESS){
        *error = ERR_TYPE;
        return;
    }

    aliasSymbol->checkState = FUNC_IN_PROGRESS;
    resolveType(table, aliasSymbol->type, error);
    aliasSymbol->checkState = FUNC_DONE;
}

void resolveType(Symtable *table, Type *type, ErrorCode *error){
    if (*error != ERR_OK || type == NULL){
        return;
    }

    if (type->kind == TY_LIST){
        resolveType(table, type->elem, error);
    }
    else if (type->kind == TY_FUNC){
        for (size_t paramIndex = 0; paramIndex < type->func.params.len; paramIndex++){
            resolveType(table, type->func.params.items[paramIndex], error);
        }
        resolveType(table, type->func.ret, error);
    }
    else if (type->kind == TY_UNION){
        for (size_t memberIndex = 0; memberIndex < type->members.len; memberIndex++){
            resolveType(table, type->members.items[memberIndex], error);
        }
    }
    else if (type->kind == TY_NAMED && type->named.resolved == NULL){
        Symbol *typeSymbol = lookupSymbolInTree(table->typeTree, type->named.name);

        if (typeSymbol == NULL){
            *error = ERR_UNDEF;
            return;
        }

        if (typeSymbol->kind == SYM_ALIAS){
            resolveAlias(table, typeSymbol, error);
        }

        type->named.resolved = getRealType(typeSymbol->type);
    }
}

void resolveParamList(Symtable *table, ParamList *paramList, ErrorCode *error){
    for (size_t paramIndex = 0; paramIndex < paramList->params_count; paramIndex++){
        resolveType(table, paramList->params[paramIndex].type, error);
    }
}

void resolveFunctionTypes(Symtable *table, DefFuncNode *function, ErrorCode *error){
    for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++){
        resolveParamList(table, &function->paramList[listIndex], error);
    }

    resolveType(table, function->returnType, error);
}

void resolveGlobalTypes(Symtable *table, ProgramNode *program, ErrorCode *error){
    for (size_t itemIndex = 0; itemIndex < program->itemsCount; itemIndex++){
        ASTNode *item = program->items[itemIndex];

        if (item->kind == AST_FUNCTION_DEF){
            resolveFunctionTypes(table, (DefFuncNode *)item, error);
        }
        else if (item->kind == AST_CLASS_DEF){
            resolveParamList(table, &((ClassDefNode *)item)->fields, error);
        }
        else if (item->kind == AST_TYPE_ALIAS){
            resolveAlias(table, ((TypeAliasNode *)item)->symbol, error);
        }
        else if (item->kind == AST_EXTENSION){
            ExtensionNode *extension = (ExtensionNode *)item;
            resolveType(table, extension->receiver.type, error);

            for (size_t methodIndex = 0; methodIndex < extension->methodsCount; methodIndex++){
                resolveFunctionTypes(table, (DefFuncNode *)extension->methods[methodIndex], error);
            }
        }

        if (*error != ERR_OK){
            return;
        }
    }
}

const char *getTypeKeyName(Type *type){
    type = getRealType(type);

    if (type == NULL){
        return "?";
    }
    if (type->kind == TY_INT){
        return "Int";
    }
    if (type->kind == TY_DOUBLE){
        return "Double";
    }
    if (type->kind == TY_STRING){
        return "String";
    }
    if (type->kind == TY_BOOL){
        return "Boolean";
    }
    if (type->kind == TY_UNIT){
        return "Unit";
    }
    if (type->kind == TY_CLASS){
        return ((ClassDefNode *)type->cls->definition)->name;
    }
    if (type->kind == TY_LIST){
        return "List";
    }
    // zjednotenia a funkcie ako príjemcu nepodporujeme presne
    return "?";
}

void collectExtensionMethod(Symtable *table, Param *receiver, DefFuncNode *method,
                            ErrorCode *error){
    const char *typeName = getTypeKeyName(receiver->type);
    char *key = malloc(strlen(typeName) + strlen(method->name) + 2);

    if (key == NULL){
        *error = ERR_INTERNAL;
        return;
    }

    strcpy(key, typeName);
    strcat(key, ".");
    strcat(key, method->name);

    Symbol *methodSymbol = defineSymbolInTree(table, &table->extensionTree, key, SYM_EXTENSION,
                                              createFunctionType(method), false, error);
    free(key);

    if (methodSymbol == NULL){
        return;
    }

    methodSymbol->definition = (ASTNode *)method;
    methodSymbol->receiver = receiver;
    method->symbol = methodSymbol;
}

void collectExtensions(Symtable *table, ProgramNode *program, ErrorCode *error){
    for (size_t itemIndex = 0; itemIndex < program->itemsCount; itemIndex++){
        if (program->items[itemIndex]->kind != AST_EXTENSION){
            continue;
        }

        ExtensionNode *extension = (ExtensionNode *)program->items[itemIndex];

        for (size_t methodIndex = 0; methodIndex < extension->methodsCount; methodIndex++){
            if (*error != ERR_OK){
                return;
            }

            collectExtensionMethod(table, &extension->receiver,
                                   (DefFuncNode *)extension->methods[methodIndex], error);
        }
    }
}

void checkMainReturnType(ProgramNode *program, ErrorCode *error){
    for (size_t itemIndex = 0; itemIndex < program->itemsCount; itemIndex++){
        if (program->items[itemIndex]->kind != AST_FUNCTION_DEF){
            continue;
        }

        DefFuncNode *function = (DefFuncNode *)program->items[itemIndex];
        Type *returnType = getRealType(function->returnType);

        // @main s aliasom, ktorý nie je Unit
        if (function->isMain && returnType != NULL && returnType->kind != TY_UNIT){
            *error = ERR_TYPE;
            return;
        }
    }
}

void semanticAnalyze(ASTNode *program, Symtable *table, ErrorCode *error){
    if (program == NULL || program->kind != AST_PROGRAM){
        *error = ERR_INTERNAL;
        return;
    }

    // vstavané funkcie majú vlastný rozsah, aby sa dali prekryť
    pushScope(table, error);
    addBuiltins(table, error);

    // globálny rozsah
    if (*error == ERR_OK){
        pushScope(table, error);
    }

    if (*error != ERR_OK){
        return;
    }

    ProgramNode *programNode = (ProgramNode *)program;

    collectGlobalDefinitions(table, programNode, error);

    if (*error == ERR_OK){
        resolveGlobalTypes(table, programNode, error);
    }
    if (*error == ERR_OK){
        collectExtensions(table, programNode, error);
    }
    if (*error == ERR_OK){
        checkMainReturnType(programNode, error);
    }
}

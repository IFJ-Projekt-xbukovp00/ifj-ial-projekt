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
    if (*error == ERR_OK){
        checkFunctionBodies(table, programNode, error);
    }
}

Type *getLiteralType(LiteralNode *literal){
    if (literal->litType == LIT_INT){
        return type_get_basic(TY_INT);
    }
    if (literal->litType == LIT_DOUBLE){
        return type_get_basic(TY_DOUBLE);
    }
    if (literal->litType == LIT_STRING){
        return type_get_basic(TY_STRING);
    }
    if (literal->litType == LIT_BOOLEAN){
        return type_get_basic(TY_BOOL);
    }

    return type_get_basic(TY_UNIT);
}

bool needsIntToDouble(Type *expected, Type *actual){
    expected = getRealType(expected);
    actual = getRealType(actual);

    if (expected == NULL || actual == NULL || actual->kind != TY_INT){
        return false;
    }
    if (expected->kind == TY_DOUBLE){
        return true;
    }
    if (expected->kind != TY_UNION){
        return false;
    }

    // zjednotenie, ktore Int obsahuje, hodnotu prijme bez prevodu
    bool hasDouble = false;

    for (size_t memberIndex = 0; memberIndex < expected->members.len; memberIndex++){
        Type *member = getRealType(expected->members.items[memberIndex]);

        if (member->kind == TY_INT){
            return false;
        }
        if (member->kind == TY_DOUBLE){
            hasDouble = true;
        }
    }

    return hasDouble;
}

ASTNode *createConvertNode(ASTNode *operand, ErrorCode *error){
    ConvertNode *convert = calloc(1, sizeof(ConvertNode));

    if (convert == NULL){
        *error = ERR_INTERNAL;
        return NULL;
    }

    convert->node.kind = AST_CONVERT;
    convert->node.line = operand->line;
    convert->node.type = type_get_basic(TY_DOUBLE);
    convert->operand = operand;
    return (ASTNode *)convert;
}

bool acceptValue(Type *expected, ASTNode **value, ErrorCode *error){
    if (!type_accepts(expected, (*value)->type)){
        return false;
    }

    if (needsIntToDouble(expected, (*value)->type)){
        ASTNode *convert = createConvertNode(*value, error);

        if (convert != NULL){
            *value = convert;
        }
    }

    return true;
}

void addLocal(DefFuncNode *function, Symbol *local, ErrorCode *error){
    Symbol **newLocals = realloc(function->locals, (function->localsCount + 1) * sizeof(Symbol *));

    if (newLocals == NULL){
        *error = ERR_INTERNAL;
        return;
    }

    function->locals = newLocals;
    function->locals[function->localsCount] = local;
    function->localsCount++;
}

Scope *getGlobalScope(Symtable *table){
    Scope *scope = table->top;

    // najspodnejsi rozsah su vstavane funkcie, nad nim je globalny
    while (scope != NULL && scope->parent != NULL && scope->parent->parent != NULL){
        scope = scope->parent;
    }

    return scope;
}

Type *checkId(Symtable *table, IDNode *id, ErrorCode *error){
    Symbol *symbol = lookupSymbol(table, id->name);

    if (symbol == NULL){
        *error = ERR_UNDEF;
        return NULL;
    }

    // funkcie a konštruktory ako hodnotu zatial nepodporujeme
    if (symbol->kind != SYM_VARIABLE && symbol->kind != SYM_PARAMETER){
        *error = ERR_TYPE;
        return NULL;
    }

    id->idKind = ID_VAR;
    id->symbol = symbol;
    return symbol->type;
}

Type *checkBlock(Symtable *table, BodyContext *context, BlockNode *block, bool isValueUsed,
                 ErrorCode *error){
    pushScope(table, error);
    if (*error != ERR_OK){
        return NULL;
    }

    Type *blockType = type_get_basic(TY_UNIT);

    for (size_t commandIndex = 0; commandIndex < block->commandOrderCount; commandIndex++){
        // hodnota sa pouziva len z posledneho prikazu bloku
        bool isLastCommand = commandIndex + 1 == block->commandOrderCount;
        blockType = checkNode(table, context, &block->commandOrder[commandIndex],
                              isLastCommand && isValueUsed, error);

        if (*error != ERR_OK){
            break;
        }
    }

    // rozsah sa zrusi aj pri chybe, aby sa dal uvolnit cez symtableFree
    popScope(table);
    return blockType;
}

Type *checkVarDef(Symtable *table, BodyContext *context, VarOrValNode *varDef, ErrorCode *error){
    // inicializacia sa kontroluje pred definiciou, premenna v nej este nie je vidno
    Type *variableType = checkNode(table, context, &varDef->init, true, error);
    if (*error != ERR_OK){
        return NULL;
    }

    if (varDef->declaredType != NULL){
        resolveType(table, varDef->declaredType, error);
        if (*error != ERR_OK){
            return NULL;
        }

        if (!acceptValue(varDef->declaredType, &varDef->init, error)){
            *error = ERR_TYPE;
        }
        if (*error != ERR_OK){
            return NULL;
        }

        variableType = varDef->declaredType;
    }
    else if (variableType == NULL){
        // typ sa neda odvodit
        *error = ERR_TYPE;
        return NULL;
    }

    Symbol *variableSymbol = defineSymbol(table, varDef->name, SYM_VARIABLE, variableType,
                                          varDef->isMutable, error);
    if (variableSymbol == NULL){
        return NULL;
    }

    varDef->symbol = variableSymbol;
    addLocal(context->function, variableSymbol, error);
    return type_get_basic(TY_UNIT);
}

Type *checkAssign(Symtable *table, BodyContext *context, AssignNode *assign, ErrorCode *error){
    Symbol *targetSymbol = lookupSymbol(table, assign->name);

    if (targetSymbol == NULL){
        *error = ERR_UNDEF;
        return NULL;
    }

    // do val, parametra ani funkcie sa priradit neda
    if (targetSymbol->kind != SYM_VARIABLE || !targetSymbol->isMutable){
        *error = ERR_IMMUTABLE;
        return NULL;
    }

    assign->symbol = targetSymbol;

    checkNode(table, context, &assign->value, true, error);
    if (*error != ERR_OK){
        return NULL;
    }

    if (!acceptValue(targetSymbol->type, &assign->value, error)){
        *error = ERR_TYPE;
    }
    if (*error != ERR_OK){
        return NULL;
    }

    return type_get_basic(TY_UNIT);
}

Type *checkNode(Symtable *table, BodyContext *context, ASTNode **node, bool isValueUsed,
                ErrorCode *error){
    if (*error != ERR_OK){
        return NULL;
    }
    if (*node == NULL){
        return type_get_basic(TY_UNIT);
    }

    ASTNode *current = *node;
    Type *nodeType = type_get_basic(TY_UNIT);

    if (current->kind == AST_LIT){
        nodeType = getLiteralType((LiteralNode *)current);
    }
    else if (current->kind == AST_ID){
        nodeType = checkId(table, (IDNode *)current, error);
    }
    else if (current->kind == AST_BLOCK){
        nodeType = checkBlock(table, context, (BlockNode *)current, isValueUsed, error);
    }
    else if (current->kind == AST_VAR_DEF){
        nodeType = checkVarDef(table, context, (VarOrValNode *)current, error);
    }
    else if (current->kind == AST_ASSIGN){
        nodeType = checkAssign(table, context, (AssignNode *)current, error);
    }
    else if (current->kind == AST_BINARY){
        nodeType = checkBinary(table, context, (BinaryNode *)current, error);
    }
    else if (current->kind == AST_UNARY){
        nodeType = checkUnary(table, context, (UnaryNode *)current, error);
    }
    else if (current->kind == AST_IF){
        nodeType = checkIf(table, context, (IfNode *)current, isValueUsed, error);
    }
    else if (current->kind == AST_WHILE){
        nodeType = checkWhile(table, context, (WhileNode *)current, error);
    }
    else if (current->kind == AST_RETURN){
        nodeType = checkReturn(table, context, (ReturnNode *)current, error);
    }
    else if (current->kind == AST_EXPR_STATEMENT){
        nodeType = checkNode(table, context, &((LonelyExprNode *)current)->expr, isValueUsed,
                             error);
    }

    current->type = nodeType;
    return nodeType;
}

void defineParameters(Symtable *table, Symbol *functionSymbol, ErrorCode *error){
    DefFuncNode *function = (DefFuncNode *)functionSymbol->definition;
    Param *receiver = functionSymbol->receiver;

    if (receiver != NULL){
        receiver->symbol = defineSymbol(table, receiver->name, SYM_PARAMETER, receiver->type,
                                        false, error);
    }

    for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++){
        ParamList *paramList = &function->paramList[listIndex];

        for (size_t paramIndex = 0; paramIndex < paramList->params_count; paramIndex++){
            Param *param = &paramList->params[paramIndex];

            if (*error == ERR_OK){
                param->symbol = defineSymbol(table, param->name, SYM_PARAMETER, param->type,
                                             false, error);
            }
        }
    }
}

void checkFunctionBody(Symtable *table, BodyContext *parent, Symbol *functionSymbol,
                       ErrorCode *error){
    if (*error != ERR_OK || functionSymbol->checkState == FUNC_DONE){
        return;
    }

    DefFuncNode *function = (DefFuncNode *)functionSymbol->definition;

    // funkcia bez tela je len v rucne poskladanych AST
    if (function->body == NULL){
        functionSymbol->checkState = FUNC_DONE;
        return;
    }

    functionSymbol->checkState = FUNC_IN_PROGRESS;

    // telo nesmie vidiet lokalne premenne volajucej funkcie
    Scope *savedTop = table->top;
    table->top = getGlobalScope(table);

    pushScope(table, error);
    if (*error != ERR_OK){
        table->top = savedTop;
        return;
    }

    BodyContext context = {function, parent};

    // pri explicitnom Unit sa vysledok tela zahodi
    bool isUnitDeclared = isUnitType(function->returnType);

    defineParameters(table, functionSymbol, error);
    Type *bodyType = checkNode(table, &context, &function->body, !isUnitDeclared, error);
    checkFunctionResult(functionSymbol, bodyType, error);

    popScope(table);
    table->top = savedTop;
    functionSymbol->checkState = FUNC_DONE;
}

void checkFunctionBodies(Symtable *table, ProgramNode *program, ErrorCode *error){
    for (size_t itemIndex = 0; itemIndex < program->itemsCount; itemIndex++){
        ASTNode *item = program->items[itemIndex];

        if (item->kind == AST_FUNCTION_DEF){
            checkFunctionBody(table, NULL, ((DefFuncNode *)item)->symbol, error);
        }
        else if (item->kind == AST_EXTENSION){
            ExtensionNode *extension = (ExtensionNode *)item;

            for (size_t methodIndex = 0; methodIndex < extension->methodsCount; methodIndex++){
                checkFunctionBody(table, NULL,
                                  ((DefFuncNode *)extension->methods[methodIndex])->symbol, error);
            }
        }

        if (*error != ERR_OK){
            return;
        }
    }
}

bool isNumericType(Type *type){
    return type != NULL && (type->kind == TY_INT || type->kind == TY_DOUBLE);
}

Type *convertNumericOperands(BinaryNode *binary, Type *leftType, Type *rightType,
                             ErrorCode *error){
    if (leftType->kind == TY_INT && rightType->kind == TY_INT){
        return type_get_basic(TY_INT);
    }

    // jediná implicitná konverzia, prevedie sa ta strana, ktora je Int
    if (leftType->kind == TY_INT){
        ASTNode *convert = createConvertNode(binary->left, error);
        if (convert != NULL){
            binary->left = convert;
        }
    }
    else if (rightType->kind == TY_INT){
        ASTNode *convert = createConvertNode(binary->right, error);
        if (convert != NULL){
            binary->right = convert;
        }
    }

    return type_get_basic(TY_DOUBLE);
}

Type *checkBinary(Symtable *table, BodyContext *context, BinaryNode *binary, ErrorCode *error){
    // operandy sa vyhodnocuju zlava doprava
    Type *leftType = getRealType(checkNode(table, context, &binary->left, true, error));
    Type *rightType = getRealType(checkNode(table, context, &binary->right, true, error));

    if (*error != ERR_OK){
        return NULL;
    }

    // operand bez hodnoty (konci returnom) nemoze byt vo vyraze
    if (leftType == NULL || rightType == NULL){
        *error = ERR_TYPE;
        return NULL;
    }

    TokenType operator = binary->operator;
    bool bothNumeric = isNumericType(leftType) && isNumericType(rightType);
    bool bothString = leftType->kind == TY_STRING && rightType->kind == TY_STRING;

    if (operator == TOK_PLUS || operator == TOK_MINUS || operator == TOK_MULTIPLY
        || operator == TOK_DIVIDE){
        if (bothNumeric){
            return convertNumericOperands(binary, leftType, rightType, error);
        }
        // konkatenacia, ine typy sa na retazec automaticky neprevadzaju
        if (operator == TOK_PLUS && bothString){
            return type_get_basic(TY_STRING);
        }
    }
    else if (operator == TOK_LESS || operator == TOK_LESS_EQUAL || operator == TOK_GREATER
             || operator == TOK_GREATER_EQUAL){
        if (bothNumeric){
            convertNumericOperands(binary, leftType, rightType, error);
            return type_get_basic(TY_BOOL);
        }
        if (bothString){
            return type_get_basic(TY_BOOL);
        }
    }
    else if (operator == TOK_EQUAL || operator == TOK_NOT_EQUAL){
        if (bothNumeric){
            convertNumericOperands(binary, leftType, rightType, error);
            return type_get_basic(TY_BOOL);
        }
        if (type_equals(leftType, rightType)){
            return type_get_basic(TY_BOOL);
        }
    }
    else if (operator == TOK_AND || operator == TOK_OR){
        if (leftType->kind == TY_BOOL && rightType->kind == TY_BOOL){
            return type_get_basic(TY_BOOL);
        }
    }

    // nesulad typov alebo nepodporovany operator
    *error = ERR_TYPE;
    return NULL;
}

Type *checkUnary(Symtable *table, BodyContext *context, UnaryNode *unary, ErrorCode *error){
    Type *operandType = getRealType(checkNode(table, context, &unary->right, true, error));

    if (*error != ERR_OK){
        return NULL;
    }

    // unarne - a + nemenia typ cisla
    if ((unary->operator == TOK_MINUS || unary->operator == TOK_PLUS)
        && isNumericType(operandType)){
        return operandType;
    }
    if (unary->operator == TOK_NOT && operandType != NULL && operandType->kind == TY_BOOL){
        return operandType;
    }

    *error = ERR_TYPE;
    return NULL;
}

Type *checkBranch(Symtable *table, BodyContext *context, ASTNode **branch, bool isValueUsed,
                  ErrorCode *error){
    // aj vetva bez bloku je samostatny rozsah platnosti
    pushScope(table, error);
    if (*error != ERR_OK){
        return NULL;
    }

    Type *branchType = checkNode(table, context, branch, isValueUsed, error);

    popScope(table);
    return branchType;
}

void checkCondition(Symtable *table, BodyContext *context, ASTNode **condition,
                    ErrorCode *error){
    Type *conditionType = getRealType(checkNode(table, context, condition, true, error));

    if (*error != ERR_OK){
        return;
    }

    if (conditionType == NULL || conditionType->kind != TY_BOOL){
        *error = ERR_TYPE;
    }
}

Type *checkIf(Symtable *table, BodyContext *context, IfNode *ifNode, bool isValueUsed,
              ErrorCode *error){
    checkCondition(table, context, &ifNode->condition, error);

    Type *thenType = checkBranch(table, context, &ifNode->thenBranch, isValueUsed, error);
    Type *elseType = type_get_basic(TY_UNIT);

    // chýbajúce else sa správa ako vetva s ()
    if (ifNode->elseBranch != NULL){
        elseType = checkBranch(table, context, &ifNode->elseBranch, isValueUsed, error);
    }

    if (*error != ERR_OK){
        return NULL;
    }

    if (!isValueUsed){
        if (thenType == NULL && elseType == NULL){
            return NULL;
        }
        return type_get_basic(TY_UNIT);
    }

    // vetva, ktora konci returnom, typ neurcuje
    if (thenType == NULL){
        return elseType;
    }
    if (elseType == NULL){
        return thenType;
    }

    if (type_equals(thenType, elseType)){
        return thenType;
    }

    Type *realThenType = getRealType(thenType);
    Type *realElseType = getRealType(elseType);

    if (isNumericType(realThenType) && isNumericType(realElseType)){
        // jedna vetva je Int, druha Double, Int vetva sa prevedie
        Type *doubleType = type_get_basic(TY_DOUBLE);

        if (realThenType->kind == TY_INT){
            acceptValue(doubleType, &ifNode->thenBranch, error);
        }
        else{
            acceptValue(doubleType, &ifNode->elseBranch, error);
        }
        return doubleType;
    }

    *error = ERR_TYPE;
    return NULL;
}

Type *checkWhile(Symtable *table, BodyContext *context, WhileNode *whileNode, ErrorCode *error){
    checkCondition(table, context, &whileNode->condition, error);

    // hodnoty z tela cyklu sa neprenasaju dalej
    checkBranch(table, context, &whileNode->body, false, error);

    if (*error != ERR_OK){
        return NULL;
    }

    return type_get_basic(TY_UNIT);
}

bool isUnitType(Type *type){
    type = getRealType(type);
    return type != NULL && type->kind == TY_UNIT;
}

Type *checkReturn(Symtable *table, BodyContext *context, ReturnNode *returnNode,
                  ErrorCode *error){
    Type *returnType = context->function->returnType;

    // return vyzaduje uvedeny navratovy typ
    if (returnType == NULL){
        *error = ERR_TYPE;
        return NULL;
    }

    bool isUnitDeclared = isUnitType(returnType);

    // holy return vracia () a smie byt len pri Unit
    if (returnNode->returnValue == NULL){
        if (!isUnitDeclared){
            *error = ERR_TYPE;
        }
        return NULL;
    }

    checkNode(table, context, &returnNode->returnValue, true, error);
    if (*error != ERR_OK){
        return NULL;
    }

    if (!isUnitDeclared && !acceptValue(returnType, &returnNode->returnValue, error)){
        *error = ERR_TYPE;
    }

    return NULL;
}

bool acceptResult(Type *expected, ASTNode **node, ErrorCode *error){
    ASTNode *current = *node;

    // vysledok, ktory konci returnom, sa kontroloval v return
    if (current->type == NULL){
        return true;
    }

    if (current->kind == AST_BLOCK){
        BlockNode *block = (BlockNode *)current;

        if (block->commandOrderCount == 0){
            return type_accepts(expected, type_get_basic(TY_UNIT));
        }
        return acceptResult(expected, &block->commandOrder[block->commandOrderCount - 1], error);
    }
    if (current->kind == AST_EXPR_STATEMENT){
        return acceptResult(expected, &((LonelyExprNode *)current)->expr, error);
    }
    if (current->kind == AST_IF){
        IfNode *ifNode = (IfNode *)current;
        bool isThenAccepted = acceptResult(expected, &ifNode->thenBranch, error);

        // chýbajúce else je vetva s ()
        if (ifNode->elseBranch == NULL){
            return isThenAccepted && type_accepts(expected, type_get_basic(TY_UNIT));
        }
        return isThenAccepted && acceptResult(expected, &ifNode->elseBranch, error);
    }

    return acceptValue(expected, node, error);
}

void checkFunctionResult(Symbol *functionSymbol, Type *bodyType, ErrorCode *error){
    if (*error != ERR_OK){
        return;
    }

    DefFuncNode *function = (DefFuncNode *)functionSymbol->definition;

    // neuvedeny typ sa odvodi z vysledku tela
    if (function->returnType == NULL){
        functionSymbol->type->func.ret = bodyType;
        return;
    }

    if (isUnitType(function->returnType)){
        return;
    }

    if (!acceptResult(function->returnType, &function->body, error) && *error == ERR_OK){
        *error = ERR_TYPE;
    }
}

/**
 * @file ast.c
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

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASTNode *astAlloc(size_t size, ASTNodeKind kind, int line) {
    ASTNode *node = calloc(1, size);
    if (node == NULL) {
        return NULL;
    }
    node->kind = kind;
    node->line = line;
    node->type = NULL;
    return node;
}

char *astCopyString(const char *text) {
    if (text == NULL) {
        return NULL;
    }
    size_t len = strlen(text);
    char *copy = malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, text, len + 1);
    return copy;
}

ASTNode *astNewIntLit(long long value, int line) {
    LiteralNode *lit = (LiteralNode *)astAlloc(sizeof(LiteralNode), AST_LIT, line);
    if (lit == NULL) {
        return NULL;
    }
    lit->litType = LIT_INT;
    lit->litValue.int_value = value;
    return (ASTNode *)lit;
}

ASTNode *astNewDoubleLit(double value, int line) {
    LiteralNode *lit = (LiteralNode *)astAlloc(sizeof(LiteralNode), AST_LIT, line);
    if (lit == NULL) {
        return NULL;
    }
    lit->litType = LIT_DOUBLE;
    lit->litValue.float_value = value;
    return (ASTNode *)lit;
}

ASTNode *astNewStringLit(const char *value, int line) {
    LiteralNode *lit = (LiteralNode *)astAlloc(sizeof(LiteralNode), AST_LIT, line);
    if (lit == NULL) {
        return NULL;
    }
    lit->litType = LIT_STRING;
    lit->litValue.string_value = astCopyString(value);
    if (lit->litValue.string_value == NULL) {
        free(lit);
        return NULL;
    }
    return (ASTNode *)lit;
}

ASTNode *astNewBoolLit(bool value, int line) {
    LiteralNode *lit = (LiteralNode *)astAlloc(sizeof(LiteralNode), AST_LIT, line);
    if (lit == NULL) {
        return NULL;
    }
    lit->litType = LIT_BOOLEAN;
    lit->litValue.bool_value = value;
    return (ASTNode *)lit;
}

ASTNode *astNewUnitLit(int line) {
    LiteralNode *lit = (LiteralNode *)astAlloc(sizeof(LiteralNode), AST_LIT, line);
    if (lit == NULL) {
        return NULL;
    }
    lit->litType = LIT_UNIT;
    return (ASTNode *)lit;
}

ASTNode *astNewId(const char *name, int line) {
    IDNode *idNode = (IDNode *)astAlloc(sizeof(IDNode), AST_ID, line);
    if (idNode == NULL) {
        return NULL;
    }
    idNode->name = astCopyString(name);
    if (idNode->name == NULL) {
        free(idNode);
        return NULL;
    }
    return (ASTNode *)idNode;
}

ASTNode *astNewBinary(TokenType op, ASTNode *left, ASTNode *right, int line) {
    BinaryNode *bin = (BinaryNode *)astAlloc(sizeof(BinaryNode), AST_BINARY, line);
    if (bin == NULL) {
        return NULL;
    }
    bin->operator= op;
    bin->left = left;
    bin->right = right;
    return (ASTNode *)bin;
}

ASTNode *astNewUnary(TokenType op, ASTNode *operand, int line) {
    UnaryNode *un = (UnaryNode *)astAlloc(sizeof(UnaryNode), AST_UNARY, line);
    if (un == NULL) {
        return NULL;
    }
    un->operator= op;
    un->right = operand;
    return (ASTNode *)un;
}

void astFree(ASTNode *node) {
    if (node == NULL) {
        return;
    }
    switch (node->kind) {
    case AST_LIT: {
        LiteralNode *lit = (LiteralNode *)node;
        if (lit->litType == LIT_STRING) {
            free(lit->litValue.string_value);
        }
        break;
    }
    case AST_ID:
        free(((IDNode *)node)->name);
        break;
    case AST_BINARY:
        astFree(((BinaryNode *)node)->left);
        astFree(((BinaryNode *)node)->right);
        break;
    case AST_UNARY:
        astFree(((UnaryNode *)node)->right);
        break;
    case AST_BLOCK: {
        BlockNode *block = (BlockNode *)node;
        // najprv prikazy, az potom pole
        for (size_t i = 0; i < block->commandOrderCount; i++) {
            astFree(block->commandOrder[i]);
        }
        free(block->commandOrder);
        break;
    }
    case AST_VAR_DEF: {
        VarOrValNode *varNode = (VarOrValNode *)node;
        free(varNode->name);
        astFree(varNode->init);
        break;
    }
    case AST_ASSIGN: {
        AssignNode *assign = (AssignNode *)node;
        free(assign->name);
        astFree(assign->value);
        break;
    }
    case AST_RETURN:
        astFree(((ReturnNode *)node)->returnValue);
        break;
    case AST_EXPR_STATEMENT:
        astFree(((LonelyExprNode *)node)->expr);
        break;
    case AST_IF: {
        IfNode *ifNode = (IfNode *)node;
        astFree(ifNode->condition);
        astFree(ifNode->thenBranch);
        astFree(ifNode->elseBranch);
        break;
    }
    case AST_WHILE: {
        WhileNode *whileNode = (WhileNode *)node;
        astFree(whileNode->condition);
        astFree(whileNode->body);
        break;
    }
    case AST_CALL: {
        CallNode *call = (CallNode *)node;
        astFree(call->caller);
        for (size_t i = 0; i < call->argsCount; i++) {
            astFree(call->args[i]);
        }
        free(call->args);
        break;
    }
    case AST_FUNCTION_DEF: {
        DefFuncNode *fn = (DefFuncNode *)node;
        free(fn->name);
        for (size_t i = 0; i < fn->paramListCount; i++) {
            astParamListFree(&fn->paramList[i]);
        }
        free(fn->paramList);
        astFree(fn->body);
        free(fn->locals);
        break;
    }
    case AST_PROGRAM: {
        ProgramNode *prog = (ProgramNode *)node;
        for (size_t i = 0; i < prog->itemsCount; i++) {
            astFree(prog->items[i]);
        }
        free(prog->items);
        break;
    }
    case AST_EXTENSION: {
        ExtensionNode *ext = (ExtensionNode *)node;
        free(ext->receiver.name);
        for (size_t i = 0; i < ext->methodsCount; i++) {
            astFree(ext->methods[i]);
        }
        free(ext->methods);
        break;
    }
    case AST_CLASS_DEF: {
        ClassDefNode *cls = (ClassDefNode *)node;
        free(cls->name);
        astParamListFree(&cls->fields);
        break;
    }
    case AST_TYPE_ALIAS:
        free(((TypeAliasNode *)node)->name);
        break;
    case AST_FOR: {
        ForNode *forNode = (ForNode *)node;
        free(forNode->varName);
        astFree(forNode->source);
        for (size_t i = 0; i < forNode->guardsCount; i++) {
            astFree(forNode->guards[i]);
        }
        free(forNode->guards);
        astFree(forNode->body);
        break;
    }
    case AST_MEMBER: {
        MemberNode *mem = (MemberNode *)node;
        astFree(mem->object);
        free(mem->name);
        break;
    }
    case AST_MATCH: {
        MatchNode *match = (MatchNode *)node;
        astFree(match->subject);
        for (size_t i = 0; i < match->casesCount; i++) {
            astFree(match->cases[i]);
        }
        free(match->cases);
        break;
    }
    case AST_CASE: {
        CaseNode *caseNode = (CaseNode *)node;
        astFree(caseNode->literal);
        free(caseNode->binder.name);
        free(caseNode->className);
        astParamListFree(&caseNode->ctorBinders);
        astFree(caseNode->guard);
        astFree(caseNode->body);
        break;
    }
    case AST_LAMBDA: {
        LambdaNode *lam = (LambdaNode *)node;
        astParamListFree(&lam->params);
        astFree(lam->body);
        free(lam->locals);
        break;
    }
    case AST_RANGE: {
        RangeNode *range = (RangeNode *)node;
        astFree(range->from);
        astFree(range->to);
        astFree(range->step);
        break;
    }
    case AST_LIST_LIT: {
        ListLitNode *list = (ListLitNode *)node;
        for (size_t i = 0; i < list->elemsCount; i++) {
            astFree(list->elems[i]);
        }
        free(list->elems);
        break;
    }
    case AST_INTERP: {
        InterpNode *interp = (InterpNode *)node;
        for (size_t i = 0; i < interp->partsCount; i++) {
            astFree(interp->parts[i]);
        }
        free(interp->parts);
        break;
    }
    case AST_CONVERT:
        astFree(((ConvertNode *)node)->operand);
        break;
    }
    free(node);
}

/**
 * Ladiaci výpis AST stromu (printIndent, tokenOpName, astPrint).
 *
 * Použitie AI: tieto ladiace funkcie vygenerovala AI (Claude Opus 5.5).
 * Prompt 1: "Napíš pomocnú ladiacu funkciu astPrint(const ASTNode *node, int depth, FILE *out)
 *   do src/parser/ast.c pre odsadený výpis výrazových uzlov podľa ast.h."
 * Prompt 2: "Aktualizuj funkciu astPrint v src/parser/ast.c o nové druhy uzlov podľa ast.h."
 * Prompt 3: "Doplň do funkcie astPrint v src/parser/ast.c všetky zostávajúce uzly rozšírení a
 *   AST_CONVERT a odstráň vetvu default."
 */

/* Vypíše odsadenie zodpovedajúce hĺbke vnorenia v strome. */
static void printIndent(int depth, FILE *out) {
    for (int i = 0; i < depth; i++) {
        fprintf(out, "  ");
    }
}

/* Vypíše mená parametrov v zozname oddelené čiarkou. */
static void printParamNames(const ParamList *list, FILE *out) {
    for (size_t i = 0; i < list->params_count; i++) {
        if (i > 0) {
            fprintf(out, ", ");
        }
        const char *name = list->params[i].name;
        fprintf(out, "%s", name != NULL ? name : "");
    }
}

/* Prevedie typ tokenu operátora na textovú reprezentáciu. */
static const char *tokenOpName(TokenType op) {
    switch (op) {
    case TOK_PLUS:
        return "+";
    case TOK_MINUS:
        return "-";
    case TOK_MULTIPLY:
        return "*";
    case TOK_DIVIDE:
        return "/";
    case TOK_MODULO:
        return "%";
    case TOK_CONCAT:
        return "++";
    case TOK_REMOVE:
        return "--";
    case TOK_EQUAL:
        return "==";
    case TOK_NOT_EQUAL:
        return "!=";
    case TOK_LESS:
        return "<";
    case TOK_LESS_EQUAL:
        return "<=";
    case TOK_GREATER:
        return ">";
    case TOK_GREATER_EQUAL:
        return ">=";
    case TOK_NOT:
        return "!";
    case TOK_AND:
        return "&&";
    case TOK_OR:
        return "||";
    default:
        return "?";
    }
}

void astPrint(const ASTNode *node, int depth, FILE *out) {
    // Ochrana pred neplatným výstupným prúdom
    if (out == NULL) {
        return;
    }
    printIndent(depth, out);
    // Prázdny ukazovateľ vypíšeme čitateľne ako (null)
    if (node == NULL) {
        fprintf(out, "(null)\n");
        return;
    }

    switch (node->kind) {
    case AST_LIT: {
        const LiteralNode *lit = (const LiteralNode *)node;
        switch (lit->litType) {
        case LIT_INT:
            fprintf(out, "Int(%lld)\n", lit->litValue.int_value);
            break;
        case LIT_DOUBLE:
            fprintf(out, "Double(%g)\n", lit->litValue.float_value);
            break;
        case LIT_STRING:
            // Ošetrenie chýbajúceho reťazca pri literáli
            fprintf(out, "String(\"%s\")\n",
                    lit->litValue.string_value != NULL ? lit->litValue.string_value : "");
            break;
        case LIT_BOOLEAN:
            fprintf(out, "Bool(%s)\n", lit->litValue.bool_value ? "true" : "false");
            break;
        case LIT_UNIT:
            fprintf(out, "Unit()\n");
            break;
        }
        break;
    }
    case AST_ID: {
        const IDNode *idNode = (const IDNode *)node;
        fprintf(out, "Id(%s)\n", idNode->name != NULL ? idNode->name : "");
        break;
    }
    case AST_BINARY: {
        const BinaryNode *bin = (const BinaryNode *)node;
        fprintf(out, "Binary(%s)\n", tokenOpName(bin->operator));
        // Rekurzívny výpis oboch operandov na ďalšej úrovni
        astPrint(bin->left, depth + 1, out);
        astPrint(bin->right, depth + 1, out);
        break;
    }
    case AST_UNARY: {
        const UnaryNode *un = (const UnaryNode *)node;
        fprintf(out, "Unary(%s)\n", tokenOpName(un->operator));
        astPrint(un->right, depth + 1, out);
        break;
    }
    case AST_BLOCK: {
        const BlockNode *block = (const BlockNode *)node;
        fprintf(out, "Block\n");
        // Všetky príkazy bloku v poradí
        for (size_t i = 0; i < block->commandOrderCount; i++) {
            astPrint(block->commandOrder[i], depth + 1, out);
        }
        break;
    }
    case AST_VAR_DEF: {
        const VarOrValNode *varNode = (const VarOrValNode *)node;
        fprintf(out, "%s(%s)\n", varNode->isMutable ? "Var" : "Val",
                varNode->name != NULL ? varNode->name : "");
        astPrint(varNode->init, depth + 1, out);
        break;
    }
    case AST_ASSIGN: {
        const AssignNode *assign = (const AssignNode *)node;
        fprintf(out, "Assign(%s)\n", assign->name != NULL ? assign->name : "");
        astPrint(assign->value, depth + 1, out);
        break;
    }
    case AST_RETURN: {
        const ReturnNode *ret = (const ReturnNode *)node;
        fprintf(out, "Return\n");
        // Voliteľný návratový výraz
        if (ret->returnValue != NULL) {
            astPrint(ret->returnValue, depth + 1, out);
        }
        break;
    }
    case AST_EXPR_STATEMENT:
        fprintf(out, "ExprStmt\n");
        astPrint(((const LonelyExprNode *)node)->expr, depth + 1, out);
        break;
    case AST_IF: {
        const IfNode *ifNode = (const IfNode *)node;
        fprintf(out, "If\n");
        astPrint(ifNode->condition, depth + 1, out);
        astPrint(ifNode->thenBranch, depth + 1, out);
        // Vetva else je nepovinná
        if (ifNode->elseBranch != NULL) {
            astPrint(ifNode->elseBranch, depth + 1, out);
        }
        break;
    }
    case AST_WHILE: {
        const WhileNode *whileNode = (const WhileNode *)node;
        fprintf(out, "While\n");
        astPrint(whileNode->condition, depth + 1, out);
        astPrint(whileNode->body, depth + 1, out);
        break;
    }
    case AST_CALL: {
        const CallNode *call = (const CallNode *)node;
        fprintf(out, "Call\n");
        // Volaný výraz a zoznam argumentov
        astPrint(call->caller, depth + 1, out);
        for (size_t i = 0; i < call->argsCount; i++) {
            astPrint(call->args[i], depth + 1, out);
        }
        break;
    }
    case AST_FUNCTION_DEF: {
        const DefFuncNode *fn = (const DefFuncNode *)node;
        fprintf(out, "Func(%s%s)\n", fn->isMain ? "@main " : "", fn->name != NULL ? fn->name : "");
        // Zoznamy parametrov vypíšeme prehľadne po riadkoch
        for (size_t i = 0; i < fn->paramListCount; i++) {
            printIndent(depth + 1, out);
            fprintf(out, "Params(");
            printParamNames(&fn->paramList[i], out);
            fprintf(out, ")\n");
        }
        astPrint(fn->body, depth + 1, out);
        break;
    }
    case AST_PROGRAM: {
        const ProgramNode *prog = (const ProgramNode *)node;
        fprintf(out, "Program\n");
        // Všetky definície na najvyššej úrovni
        for (size_t i = 0; i < prog->itemsCount; i++) {
            astPrint(prog->items[i], depth + 1, out);
        }
        break;
    }
    case AST_EXTENSION: {
        const ExtensionNode *ext = (const ExtensionNode *)node;
        fprintf(out, "Extension(%s)\n", ext->receiver.name != NULL ? ext->receiver.name : "");
        // Rozširujúce metódy v poradí definície
        for (size_t i = 0; i < ext->methodsCount; i++) {
            astPrint(ext->methods[i], depth + 1, out);
        }
        break;
    }
    case AST_CLASS_DEF: {
        const ClassDefNode *cls = (const ClassDefNode *)node;
        fprintf(out, "Class(%s)\n", cls->name != NULL ? cls->name : "");
        printIndent(depth + 1, out);
        fprintf(out, "Fields(");
        printParamNames(&cls->fields, out);
        fprintf(out, ")\n");
        break;
    }
    case AST_TYPE_ALIAS: {
        const TypeAliasNode *alias = (const TypeAliasNode *)node;
        fprintf(out, "TypeAlias(%s)\n", alias->name != NULL ? alias->name : "");
        break;
    }
    case AST_FOR: {
        const ForNode *forNode = (const ForNode *)node;
        fprintf(out, "For(%s)\n", forNode->varName != NULL ? forNode->varName : "");
        astPrint(forNode->source, depth + 1, out);
        // Stráže cyklu if a následne telo
        for (size_t i = 0; i < forNode->guardsCount; i++) {
            astPrint(forNode->guards[i], depth + 1, out);
        }
        astPrint(forNode->body, depth + 1, out);
        break;
    }
    case AST_MEMBER: {
        const MemberNode *mem = (const MemberNode *)node;
        fprintf(out, "Member(.%s)\n", mem->name != NULL ? mem->name : "");
        astPrint(mem->object, depth + 1, out);
        break;
    }
    case AST_MATCH: {
        const MatchNode *match = (const MatchNode *)node;
        fprintf(out, "Match\n");
        astPrint(match->subject, depth + 1, out);
        for (size_t i = 0; i < match->casesCount; i++) {
            astPrint(match->cases[i], depth + 1, out);
        }
        break;
    }
    case AST_CASE: {
        const CaseNode *caseNode = (const CaseNode *)node;
        // Hlavička vetvy podľa použitého vzoru
        switch (caseNode->patternKind) {
        case PAT_WILDCARD:
            fprintf(out, "Case(_)\n");
            break;
        case PAT_LITERAL:
            fprintf(out, "Case(lit)\n");
            astPrint(caseNode->literal, depth + 1, out);
            break;
        case PAT_TYPED:
            fprintf(out, "Case(%s)\n", caseNode->binder.name != NULL ? caseNode->binder.name : "");
            break;
        case PAT_CONSTRUCTOR:
            fprintf(out, "Case(%s(", caseNode->className != NULL ? caseNode->className : "");
            printParamNames(&caseNode->ctorBinders, out);
            fprintf(out, "))\n");
            break;
        }
        if (caseNode->guard != NULL) {
            astPrint(caseNode->guard, depth + 1, out);
        }
        astPrint(caseNode->body, depth + 1, out);
        break;
    }
    case AST_LAMBDA: {
        const LambdaNode *lam = (const LambdaNode *)node;
        fprintf(out, "Lambda(");
        printParamNames(&lam->params, out);
        fprintf(out, ")\n");
        astPrint(lam->body, depth + 1, out);
        break;
    }
    case AST_RANGE: {
        const RangeNode *range = (const RangeNode *)node;
        fprintf(out, "Range(%s)\n", range->inclusive ? "to" : "until");
        astPrint(range->from, depth + 1, out);
        astPrint(range->to, depth + 1, out);
        // Nepovinný krok rozsahu (by)
        if (range->step != NULL) {
            astPrint(range->step, depth + 1, out);
        }
        break;
    }
    case AST_LIST_LIT: {
        const ListLitNode *list = (const ListLitNode *)node;
        fprintf(out, "List\n");
        for (size_t i = 0; i < list->elemsCount; i++) {
            astPrint(list->elems[i], depth + 1, out);
        }
        break;
    }
    case AST_INTERP: {
        const InterpNode *interp = (const InterpNode *)node;
        fprintf(out, "Interp\n");
        for (size_t i = 0; i < interp->partsCount; i++) {
            astPrint(interp->parts[i], depth + 1, out);
        }
        break;
    }
    case AST_CONVERT:
        fprintf(out, "Convert\n");
        astPrint(((const ConvertNode *)node)->operand, depth + 1, out);
        break;
    }
}

bool astAppend(ASTNode ***items, size_t *count, ASTNode *node) {
    if (items == NULL || count == NULL) {
        return false;
    }
    // zvacsi pole o jeden prvok cez docasny ukazovatel
    ASTNode **tmp = realloc(*items, (*count + 1) * sizeof(ASTNode *));
    if (tmp == NULL) {
        return false;
    }
    *items = tmp;
    (*items)[*count] = node;
    (*count)++;
    return true;
}

ASTNode *astNewBlock(int line) { return astAlloc(sizeof(BlockNode), AST_BLOCK, line); }

ASTNode *astNewVarDef(const char *name, bool isMutable, Type *declaredType, ASTNode *init,
                      int line) {
    VarOrValNode *varNode = (VarOrValNode *)astAlloc(sizeof(VarOrValNode), AST_VAR_DEF, line);
    if (varNode == NULL) {
        return NULL;
    }
    varNode->name = astCopyString(name);
    if (varNode->name == NULL) {
        free(varNode);
        return NULL;
    }
    varNode->isMutable = isMutable;
    varNode->declaredType = declaredType;
    varNode->init = init;
    return (ASTNode *)varNode;
}

ASTNode *astNewAssign(const char *name, ASTNode *value, int line) {
    AssignNode *assign = (AssignNode *)astAlloc(sizeof(AssignNode), AST_ASSIGN, line);
    if (assign == NULL) {
        return NULL;
    }
    assign->name = astCopyString(name);
    if (assign->name == NULL) {
        free(assign);
        return NULL;
    }
    assign->value = value;
    return (ASTNode *)assign;
}

ASTNode *astNewReturn(ASTNode *value, int line) {
    ReturnNode *ret = (ReturnNode *)astAlloc(sizeof(ReturnNode), AST_RETURN, line);
    if (ret == NULL) {
        return NULL;
    }
    // return bez hodnoty ma NULL
    ret->returnValue = value;
    return (ASTNode *)ret;
}

ASTNode *astNewExprStatement(ASTNode *expr, int line) {
    LonelyExprNode *stmt =
        (LonelyExprNode *)astAlloc(sizeof(LonelyExprNode), AST_EXPR_STATEMENT, line);
    if (stmt == NULL) {
        return NULL;
    }
    stmt->expr = expr;
    return (ASTNode *)stmt;
}

ASTNode *astNewIf(ASTNode *cond, ASTNode *thenB, ASTNode *elseB, int line) {
    IfNode *ifNode = (IfNode *)astAlloc(sizeof(IfNode), AST_IF, line);
    if (ifNode == NULL) {
        return NULL;
    }
    ifNode->condition = cond;
    ifNode->thenBranch = thenB;
    // else vetva moze byt NULL
    ifNode->elseBranch = elseB;
    return (ASTNode *)ifNode;
}

ASTNode *astNewWhile(ASTNode *cond, ASTNode *body, int line) {
    WhileNode *whileNode = (WhileNode *)astAlloc(sizeof(WhileNode), AST_WHILE, line);
    if (whileNode == NULL) {
        return NULL;
    }
    whileNode->condition = cond;
    whileNode->body = body;
    return (ASTNode *)whileNode;
}

ASTNode *astNewCall(ASTNode *caller, int line) {
    CallNode *call = (CallNode *)astAlloc(sizeof(CallNode), AST_CALL, line);
    if (call == NULL) {
        return NULL;
    }
    call->caller = caller;
    return (ASTNode *)call;
}

bool astParamListAdd(ParamList *list, const char *name, Type *type, int line) {
    if (list == NULL || name == NULL) {
        return false;
    }
    char *nameCopy = astCopyString(name);
    if (nameCopy == NULL) {
        return false;
    }
    Param *tmp = realloc(list->params, (list->params_count + 1) * sizeof(Param));
    if (tmp == NULL) {
        free(nameCopy);
        return false;
    }
    list->params = tmp;
    Param *param = &list->params[list->params_count];
    param->name = nameCopy;
    param->type = type;
    param->line = line;
    // symbol doplni az semantika
    param->symbol = NULL;
    list->params_count++;
    return true;
}

void astParamListFree(ParamList *list) {
    if (list == NULL) {
        return;
    }
    for (size_t i = 0; i < list->params_count; i++) {
        free(list->params[i].name);
    }
    free(list->params);
    list->params = NULL;
    list->params_count = 0;
}

ASTNode *astNewFuncDef(const char *name, bool isMain, int line) {
    DefFuncNode *fn = (DefFuncNode *)astAlloc(sizeof(DefFuncNode), AST_FUNCTION_DEF, line);
    if (fn == NULL) {
        return NULL;
    }
    fn->name = astCopyString(name);
    if (fn->name == NULL) {
        free(fn);
        return NULL;
    }
    fn->isMain = isMain;
    return (ASTNode *)fn;
}

ParamList *astFuncAddParamList(DefFuncNode *func) {
    if (func == NULL) {
        return NULL;
    }
    ParamList *tmp = realloc(func->paramList, (func->paramListCount + 1) * sizeof(ParamList));
    if (tmp == NULL) {
        return NULL;
    }
    func->paramList = tmp;
    ParamList *added = &func->paramList[func->paramListCount];
    added->params = NULL;
    added->params_count = 0;
    func->paramListCount++;
    return added;
}

ASTNode *astNewProgram(int line) { return astAlloc(sizeof(ProgramNode), AST_PROGRAM, line); }

ASTNode *astNewRange(ASTNode *from, ASTNode *to, ASTNode *step, bool inclusive, int line) {
    RangeNode *range = (RangeNode *)astAlloc(sizeof(RangeNode), AST_RANGE, line);
    if (range == NULL) {
        return NULL;
    }
    range->from = from;
    range->to = to;
    range->step = step;
    range->inclusive = inclusive;
    return (ASTNode *)range;
}

ASTNode *astNewFor(const char *varName, ASTNode *source, ASTNode *body, int line) {
    ForNode *forNode = (ForNode *)astAlloc(sizeof(ForNode), AST_FOR, line);
    if (forNode == NULL) {
        return NULL;
    }
    forNode->varName = astCopyString(varName);
    if (forNode->varName == NULL) {
        free(forNode);
        return NULL;
    }
    forNode->source = source;
    forNode->body = body;
    return (ASTNode *)forNode;
}

ASTNode *astNewListLit(int line) { return astAlloc(sizeof(ListLitNode), AST_LIST_LIT, line); }

ASTNode *astNewInterp(int line) { return astAlloc(sizeof(InterpNode), AST_INTERP, line); }

ASTNode *astNewMember(ASTNode *object, const char *name, int line) {
    MemberNode *mem = (MemberNode *)astAlloc(sizeof(MemberNode), AST_MEMBER, line);
    if (mem == NULL) {
        return NULL;
    }
    mem->name = astCopyString(name);
    if (mem->name == NULL) {
        free(mem);
        return NULL;
    }
    mem->object = object;
    return (ASTNode *)mem;
}

ASTNode *astNewLambda(ASTNode *body, int line) {
    LambdaNode *lam = (LambdaNode *)astAlloc(sizeof(LambdaNode), AST_LAMBDA, line);
    if (lam == NULL) {
        return NULL;
    }
    lam->body = body;
    return (ASTNode *)lam;
}

ASTNode *astNewClassDef(const char *name, int line) {
    ClassDefNode *cls = (ClassDefNode *)astAlloc(sizeof(ClassDefNode), AST_CLASS_DEF, line);
    if (cls == NULL) {
        return NULL;
    }
    cls->name = astCopyString(name);
    if (cls->name == NULL) {
        free(cls);
        return NULL;
    }
    return (ASTNode *)cls;
}

ASTNode *astNewTypeAlias(const char *name, Type *aliased, int line) {
    TypeAliasNode *alias = (TypeAliasNode *)astAlloc(sizeof(TypeAliasNode), AST_TYPE_ALIAS, line);
    if (alias == NULL) {
        return NULL;
    }
    alias->name = astCopyString(name);
    if (alias->name == NULL) {
        free(alias);
        return NULL;
    }
    alias->aliased = aliased;
    return (ASTNode *)alias;
}

ASTNode *astNewExtension(const char *recvName, Type *recvType, int line) {
    ExtensionNode *ext = (ExtensionNode *)astAlloc(sizeof(ExtensionNode), AST_EXTENSION, line);
    if (ext == NULL) {
        return NULL;
    }
    ext->receiver.name = astCopyString(recvName);
    if (ext->receiver.name == NULL) {
        free(ext);
        return NULL;
    }
    ext->receiver.type = recvType;
    ext->receiver.line = line;
    return (ASTNode *)ext;
}

ASTNode *astNewMatch(ASTNode *subject, int line) {
    MatchNode *match = (MatchNode *)astAlloc(sizeof(MatchNode), AST_MATCH, line);
    if (match == NULL) {
        return NULL;
    }
    match->subject = subject;
    return (ASTNode *)match;
}

ASTNode *astNewCase(PatternKind kind, int line) {
    CaseNode *caseNode = (CaseNode *)astAlloc(sizeof(CaseNode), AST_CASE, line);
    if (caseNode == NULL) {
        return NULL;
    }
    caseNode->patternKind = kind;
    return (ASTNode *)caseNode;
}

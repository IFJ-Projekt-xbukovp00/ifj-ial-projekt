/**
 * @file codegen.c
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

#include "codegen.h"
#include "dynstr.h"
#include "error.h"
#include "semantic.h"

#include <stdarg.h>
#include <string.h>

void appendFormat(CodeGen *gen, DynStr *target, const char *format, va_list args) {
    if (gen->error != ERR_OK) {
        return;
    }

    va_list argsCopy;

    // najprv sa zisti dlzka vysledku, potom sa zapise na koniec bufferu
    va_copy(argsCopy, args);
    int length = vsnprintf(NULL, 0, format, argsCopy);
    va_end(argsCopy);

    if (length < 0 || !dynstr_reserve(target, (size_t)length)) {
        gen->error = ERR_INTERNAL;
        return;
    }

    vsnprintf(target->data + target->len, (size_t)length + 1, format, args);
    target->len += (size_t)length;
}

void emit(CodeGen *gen, const char *format, ...) {
    va_list args;

    va_start(args, format);
    appendFormat(gen, &gen->code, format, args);
    va_end(args);
}

void emitDeclaration(CodeGen *gen, const char *format, ...) {
    va_list args;

    va_start(args, format);
    appendFormat(gen, &gen->declarations, format, args);
    va_end(args);
}

void emitHeader(CodeGen *gen) { fprintf(gen->out, ".IFJcode26\n"); }

void emitHelperVars(CodeGen *gen) {
    emitDeclaration(gen, "DEFVAR LF@%%t0\n");
    emitDeclaration(gen, "DEFVAR LF@%%t1\n");
    emitDeclaration(gen, "DEFVAR LF@%%t2\n");
    emitDeclaration(gen, "DEFVAR LF@%%t3\n");
    emitDeclaration(gen, "DEFVAR LF@%%t4\n");
    emitDeclaration(gen, "DEFVAR LF@%%trash\n");
}

void emitString(CodeGen *gen, const char *text) {
    emit(gen, "string@");

    for (size_t charIndex = 0; text[charIndex] != '\0'; charIndex++) {
        unsigned char currentChar = (unsigned char)text[charIndex];

        // biele znaky, # a \ sa musia zapisat ako \ddd
        if (currentChar <= 32 || currentChar == 35 || currentChar == 92) {
            emit(gen, "\\%03d", currentChar);
        } else {
            emit(gen, "%c", currentChar);
        }
    }
}

void emitVar(CodeGen *gen, const char *name, Symbol *symbol) {
    // prijemca rozsirujucej metody cislo nedostane (semantika mu pri kazdej metode
    // vytvori novy symbol), preto je vzdy pod menom meno$0
    emit(gen, "LF@%s$%d", name, symbol->cgId);
}

void emitFunctionLabel(CodeGen *gen, DefFuncNode *function) {
    if (function->symbol->kind == SYM_EXTENSION) {
        // rozsirujuca metoda moze mat rovnake meno pre rozne typy
        Param *receiver = function->symbol->receiver;
        emit(gen, "ext$%s$%s", getTypeKeyName(receiver->type), function->name);
    } else {
        emit(gen, "f$%s", function->name);
    }
}

void emitLiteral(CodeGen *gen, LiteralNode *literal) {
    if (literal->litType == LIT_INT) {
        emit(gen, "int@%lld", literal->litValue.int_value);
    } else if (literal->litType == LIT_DOUBLE) {
        // desatinne cislo sa zapisuje vo formate %a
        emit(gen, "float@%a", literal->litValue.float_value);
    } else if (literal->litType == LIT_STRING) {
        emitString(gen, literal->litValue.string_value);
    } else if (literal->litType == LIT_BOOLEAN) {
        // true a false su priamo konstanty bool
        emit(gen, "bool@%s", literal->litValue.bool_value ? "true" : "false");
    } else if (literal->litType == LIT_UNIT) {
        // hodnota () je za behu nil
        emit(gen, "nil@nil");
    }
}

void initCodeGen(CodeGen *gen, FILE *out) {
    gen->out = out;
    gen->labelCounter = 0;
    gen->varCounter = 0;
    gen->hiddenCounter = 0;
    gen->function = NULL;
    gen->error = ERR_OK;

    for (int helper = 0; helper < HELPER_COUNT; helper++) {
        gen->helperUsed[helper] = false;
    }

    // oba buffre sa inicializuju vzdy, aby sa dali uvolnit aj pri chybe
    bool isCodeReady = dynstr_init(&gen->code);
    bool isDeclarationsReady = dynstr_init(&gen->declarations);

    if (!isCodeReady || !isDeclarationsReady) {
        gen->error = ERR_INTERNAL;
    }
}

void freeCodeGen(CodeGen *gen) {
    dynstr_free(&gen->code);
    dynstr_free(&gen->declarations);
}

int newLabel(CodeGen *gen) {
    gen->labelCounter++;
    return gen->labelCounter;
}

TypeKind getKind(ASTNode *node) {
    // typ moze byt aj alias, rozhoduje az skutocny typ
    Type *type = getRealType(node->type);

    if (type == NULL) {
        return TY_UNIT;
    }
    return type->kind;
}

DefFuncNode *findMain(ProgramNode *prog) {
    for (size_t itemIndex = 0; itemIndex < prog->itemsCount; itemIndex++) {
        ASTNode *item = prog->items[itemIndex];

        if (item->kind == AST_FUNCTION_DEF && ((DefFuncNode *)item)->isMain) {
            return (DefFuncNode *)item;
        }
    }

    return NULL;
}

void genPrint(CodeGen *gen, CallNode *call) {
    ASTNode *arg = call->args[0];

    // WRITE nema zasobnikovu verziu
    genExpr(gen, arg);
    emit(gen, "POPS LF@%%t0\n");

    TypeKind kind = getKind(arg);

    if (kind == TY_UNIT) {
        // hodnota Unit je za behu nil, vypisuje sa ako "()"
        emit(gen, "WRITE string@()\n");
    } else if (kind == TY_UNION) {
        // pri zjednoteni sa az za behu zisti, ci to nie je () (nil)
        int writeLabel = newLabel(gen);

        emit(gen, "TYPE LF@%%t1 LF@%%t0\n");
        emit(gen, "JUMPIFNEQ L$%d LF@%%t1 string@nil\n", writeLabel);
        emit(gen, "MOVE LF@%%t0 string@()\n");
        emit(gen, "LABEL L$%d\n", writeLabel);
        emit(gen, "WRITE LF@%%t0\n");
    } else {
        // Int, Double, String aj Boolean vypise WRITE rovnako ako print
        // (bool ako true alebo false)
        emit(gen, "WRITE LF@%%t0\n");
    }

    // print vracia ()
    emit(gen, "PUSHS nil@nil\n");
}

void genArgs(CodeGen *gen, CallNode *call) {
    // argumentom moze byt lubovolny vyraz, aj dalsie volanie
    // vyhodnocuju sa zlava doprava, kazdy necha na zasobniku jednu hodnotu
    for (size_t argIndex = 0; argIndex < call->argsCount; argIndex++) {
        genExpr(gen, call->args[argIndex]);
    }
}

void genArgLists(CodeGen *gen, CallNode *call) {
    // f(a)(b) je vnorene volanie, vnutorne volanie ma prvy zoznam argumentov,
    // argumenty vsetkych zoznamov sa daju za sebou ako jeden zoznam
    if (call->caller->kind == AST_CALL && ((CallNode *)call->caller)->symbol == call->symbol) {
        genArgLists(gen, (CallNode *)call->caller);
    }

    genArgs(gen, call);
}

void genFunctionCall(CodeGen *gen, CallNode *call) {
    DefFuncNode *function = (DefFuncNode *)call->symbol->definition;

    // funkcia si argumenty vyberie zo zasobnika sama
    genArgLists(gen, call);
    emit(gen, "CALL f$%s\n", function->name);
}

void genReadLine(CodeGen *gen) {
    // READ vrati riadok aj s koncom riadku, pri EOF prazdny retazec
    emit(gen, "READ LF@%%t0 string\n");
    emit(gen, "PUSHS LF@%%t0\n");
}

void genReadNumber(CodeGen *gen, const char *typeName, const char *zero) {
    int okLabel = newLabel(gen);

    // READ vrati pri chybnom vstupe alebo EOF nil, vtedy sa vracia 0
    emit(gen, "READ LF@%%t0 %s\n", typeName);
    emit(gen, "JUMPIFNEQ L$%d LF@%%t0 nil@nil\n", okLabel);
    emit(gen, "MOVE LF@%%t0 %s\n", zero);
    emit(gen, "LABEL L$%d\n", okLabel);
    emit(gen, "PUSHS LF@%%t0\n");
}

void genValueToStr(CodeGen *gen, Type *type) {
    type = getRealType(type);

    if (type == NULL) {
        return;
    }

    if (type->kind == TY_INT) {
        emit(gen, "INT2STRS\n");
    } else if (type->kind == TY_DOUBLE) {
        // FLOAT2STR pise vo formate %.2f, presne ako toStr
        emit(gen, "FLOAT2STRS\n");
    } else if (type->kind == TY_UNIT) {
        // hodnota () je nil, ako retazec je "()"
        emit(gen, "POPS LF@%%trash\n");
        emit(gen, "PUSHS string@()\n");
    } else if (type->kind == TY_BOOL) {
        // pre bool nie je prevodna instrukcia, rozhodne sa skokom
        int trueLabel = newLabel(gen);
        int endLabel = newLabel(gen);

        emit(gen, "PUSHS bool@true\n");
        emit(gen, "JUMPIFEQS L$%d\n", trueLabel);
        emit(gen, "PUSHS string@false\n");
        emit(gen, "JUMP L$%d\n", endLabel);
        emit(gen, "LABEL L$%d\n", trueLabel);
        emit(gen, "PUSHS string@true\n");
        emit(gen, "LABEL L$%d\n", endLabel);
    } else if (type->kind == TY_UNION) {
        // typ hodnoty zjednotenia sa zisti az za behu
        useHelper(gen, HELPER_TOSTR_ANY);
        emit(gen, "CALL $toStr_any\n");
    }
    // retazec uz je retazec, nic sa nerobi
}

void genToStr(CodeGen *gen, CallNode *call) {
    genExpr(gen, call->args[0]);
    genValueToStr(gen, call->args[0]->type);
}

void genOrd(CodeGen *gen, CallNode *call) {
    genArgs(gen, call);

    useHelper(gen, HELPER_ORD);
    emit(gen, "CALL $ord\n");
}

void genOrdHelper(CodeGen *gen) {
    int badLabel = newLabel(gen);

    emit(gen, "POPS LF@%%t1\n");
    emit(gen, "POPS LF@%%t0\n");

    // index mimo retazec (aj prazdny retazec) vracia -1
    emit(gen, "LT LF@%%t2 LF@%%t1 int@0\n");
    emit(gen, "JUMPIFEQ L$%d LF@%%t2 bool@true\n", badLabel);
    emit(gen, "STRLEN LF@%%t2 LF@%%t0\n");
    emit(gen, "LT LF@%%t2 LF@%%t1 LF@%%t2\n");
    emit(gen, "JUMPIFEQ L$%d LF@%%t2 bool@false\n", badLabel);

    emit(gen, "STRI2INT LF@%%t0 LF@%%t0 LF@%%t1\n");
    emit(gen, "PUSHS LF@%%t0\n");
    emit(gen, "POPFRAME\n");
    emit(gen, "RETURN\n");

    emit(gen, "LABEL L$%d\n", badLabel);
    emit(gen, "PUSHS int@-1\n");
}

void genJumpIfTrue(CodeGen *gen, const char *left, const char *operator, const char *right,
                   int label) {
    // vysledok porovnania ide na zasobnik a pri true sa skace
    emit(gen, "PUSHS %s\n", left);
    emit(gen, "PUSHS %s\n", right);
    emit(gen, "%s\n", operator);
    emit(gen, "PUSHS bool@true\n");
    emit(gen, "JUMPIFEQS L$%d\n", label);
}

void genSubStr(CodeGen *gen, CallNode *call) {
    // argumenty idu na zasobnik ako pri beznej funkcii
    genArgs(gen, call);

    useHelper(gen, HELPER_SUBSTR);
    emit(gen, "CALL $subStr\n");
}

void genSubStrHelper(CodeGen *gen) {
    int errorLabel = newLabel(gen);
    int okLabel = newLabel(gen);

    // retazec do %t0, zaciatok do %t1, koniec do %t2
    emit(gen, "POPS LF@%%t2\n");
    emit(gen, "POPS LF@%%t1\n");
    emit(gen, "POPS LF@%%t0\n");
    emit(gen, "STRLEN LF@%%t3 LF@%%t0\n");

    // start < 0, end < 0, start > end, start >= dlzka, end > dlzka
    genJumpIfTrue(gen, "LF@%t1", "LTS", "int@0", errorLabel);
    genJumpIfTrue(gen, "LF@%t2", "LTS", "int@0", errorLabel);
    genJumpIfTrue(gen, "LF@%t1", "GTS", "LF@%t2", errorLabel);
    genJumpIfTrue(gen, "LF@%t1", "LTS", "LF@%t3", okLabel);
    emit(gen, "JUMP L$%d\n", errorLabel);
    emit(gen, "LABEL L$%d\n", okLabel);
    genJumpIfTrue(gen, "LF@%t2", "GTS", "LF@%t3", errorLabel);

    int loopLabel = newLabel(gen);
    int doneLabel = newLabel(gen);

    // znaky od start po end - 1 sa postupne pripajaju do %t3
    emit(gen, "MOVE LF@%%t3 string@\n");
    emit(gen, "LABEL L$%d\n", loopLabel);
    emit(gen, "PUSHS LF@%%t1\n");
    emit(gen, "PUSHS LF@%%t2\n");
    emit(gen, "LTS\n");
    emit(gen, "PUSHS bool@false\n");
    emit(gen, "JUMPIFEQS L$%d\n", doneLabel);
    emit(gen, "GETCHAR LF@%%t4 LF@%%t0 LF@%%t1\n");
    emit(gen, "CONCAT LF@%%t3 LF@%%t3 LF@%%t4\n");
    emit(gen, "ADD LF@%%t1 LF@%%t1 int@1\n");
    emit(gen, "JUMP L$%d\n", loopLabel);

    emit(gen, "LABEL L$%d\n", errorLabel);
    emit(gen, "EXIT int@40\n");

    emit(gen, "LABEL L$%d\n", doneLabel);
    emit(gen, "PUSHS LF@%%t3\n");
}

void genExtMethodCall(CodeGen *gen, CallNode *call) {
    CallNode *firstCall = call;

    // x.plus(1)(2): najvnutornejsie volanie ma ako caller clen x.plus
    while (firstCall->caller->kind == AST_CALL) {
        firstCall = (CallNode *)firstCall->caller;
    }

    // prijemca ide na zasobnik ako prvy argument, za nim argumenty vsetkych zoznamov
    genExpr(gen, ((MemberNode *)firstCall->caller)->object);
    genArgLists(gen, call);

    emit(gen, "CALL ");
    emitFunctionLabel(gen, (DefFuncNode *)call->symbol->definition);
    emit(gen, "\n");
}

void genToStrAnyHelper(CodeGen *gen) {
    int intLabel = newLabel(gen);
    int floatLabel = newLabel(gen);
    int boolLabel = newLabel(gen);
    int trueLabel = newLabel(gen);
    int nilLabel = newLabel(gen);
    int endLabel = newLabel(gen);

    // hodnota zjednotenia nema staticky typ, rozhoduje sa podla TYPE za behu
    emit(gen, "POPS LF@%%t0\n");
    emit(gen, "TYPE LF@%%t1 LF@%%t0\n");
    emit(gen, "JUMPIFEQ L$%d LF@%%t1 string@int\n", intLabel);
    emit(gen, "JUMPIFEQ L$%d LF@%%t1 string@float\n", floatLabel);
    emit(gen, "JUMPIFEQ L$%d LF@%%t1 string@bool\n", boolLabel);
    emit(gen, "JUMPIFEQ L$%d LF@%%t1 string@nil\n", nilLabel);

    // retazec sa vrati tak, ako je
    emit(gen, "PUSHS LF@%%t0\n");
    emit(gen, "JUMP L$%d\n", endLabel);

    emit(gen, "LABEL L$%d\n", intLabel);
    emit(gen, "INT2STR LF@%%t0 LF@%%t0\n");
    emit(gen, "PUSHS LF@%%t0\n");
    emit(gen, "JUMP L$%d\n", endLabel);

    emit(gen, "LABEL L$%d\n", floatLabel);
    emit(gen, "FLOAT2STR LF@%%t0 LF@%%t0\n");
    emit(gen, "PUSHS LF@%%t0\n");
    emit(gen, "JUMP L$%d\n", endLabel);

    emit(gen, "LABEL L$%d\n", boolLabel);
    emit(gen, "JUMPIFEQ L$%d LF@%%t0 bool@true\n", trueLabel);
    emit(gen, "PUSHS string@false\n");
    emit(gen, "JUMP L$%d\n", endLabel);
    emit(gen, "LABEL L$%d\n", trueLabel);
    emit(gen, "PUSHS string@true\n");
    emit(gen, "JUMP L$%d\n", endLabel);

    // hodnota () typu Unit je za behu nil
    emit(gen, "LABEL L$%d\n", nilLabel);
    emit(gen, "PUSHS string@()\n");

    emit(gen, "LABEL L$%d\n", endLabel);
}

void genConstructor(CodeGen *gen, CallNode *call) {
    const ClassDefNode *classDef = (const ClassDefNode *)call->symbol->definition;
    size_t fieldCount = classDef->fields.params_count;

    // atributy sa vyhodnotia najprv vsetky, aby si vnorene konstruktory
    // neprepisali pomocne premenne
    genArgs(gen, call);

    // hodnota triedy je pole ["Meno", atribut1, atribut2, ...]
    emit(gen, "NEWARRAY LF@%%t0 int@%zu\n", fieldCount + 1);
    emit(gen, "ARRAYSET LF@%%t0 int@0 ");
    emitString(gen, classDef->name);
    emit(gen, "\n");

    // posledny atribut je na vrchu zasobnika
    for (size_t fieldIndex = fieldCount; fieldIndex > 0; fieldIndex--) {
        emit(gen, "POPS LF@%%t1\n");
        emit(gen, "ARRAYSET LF@%%t0 int@%zu LF@%%t1\n", fieldIndex);
    }

    emit(gen, "PUSHS LF@%%t0\n");
}

void genCall(CodeGen *gen, CallNode *call) {
    if (call->callKind == CALL_CONSTRUCTOR) {
        genConstructor(gen, call);
    } else if (call->callKind == CALL_FUNC) {
        genFunctionCall(gen, call);
    } else if (call->callKind == CALL_EXT_METHOD) {
        genExtMethodCall(gen, call);
    } else if (call->callKind == CALL_BUILTIN && call->caller->kind == AST_ID &&
               genBuiltinCall(gen, call, ((IDNode *)call->caller)->name)) {
        // vstavana funkcia uz nechala vysledok na zasobniku
    } else {
        // volanie je aj operand vyrazu, na zasobniku musi byt vzdy jedna hodnota
        emit(gen, "PUSHS nil@nil\n");
    }
}

bool genBuiltinCall(CodeGen *gen, CallNode *call, const char *name) {
    if (strcmp(name, "print") == 0) {
        genPrint(gen, call);
    } else if (strcmp(name, "readLine") == 0) {
        genReadLine(gen);
    } else if (strcmp(name, "readInt") == 0) {
        genReadNumber(gen, "int", "int@0");
    } else if (strcmp(name, "readDouble") == 0) {
        genReadNumber(gen, "float", "float@0x0p+0");
    } else if (strcmp(name, "readIntMaybe") == 0 || strcmp(name, "readDoubleMaybe") == 0) {
        // pri chybnom vstupe READ vrati nil, co je presne hodnota () typu Unit
        bool isInt = strcmp(name, "readIntMaybe") == 0;
        emit(gen, "READ LF@%%t0 %s\n", isInt ? "int" : "float");
        emit(gen, "PUSHS LF@%%t0\n");
    } else if (strcmp(name, "readBoolean") == 0) {
        // READ bool vrati true len pre "true", inak nil, to sa berie ako false
        genReadNumber(gen, "bool", "bool@false");
    } else if (strcmp(name, "toStr") == 0) {
        genToStr(gen, call);
    } else if (strcmp(name, "toInt") == 0) {
        // FLOAT2INT oseka desatinnu cast
        genExpr(gen, call->args[0]);
        emit(gen, "FLOAT2INTS\n");
    } else if (strcmp(name, "strLength") == 0) {
        // STRLEN nema zasobnikovu verziu
        genExpr(gen, call->args[0]);
        emit(gen, "POPS LF@%%t0\n");
        emit(gen, "STRLEN LF@%%t0 LF@%%t0\n");
        emit(gen, "PUSHS LF@%%t0\n");
    } else if (strcmp(name, "ord") == 0) {
        genOrd(gen, call);
    } else if (strcmp(name, "subStr") == 0) {
        genSubStr(gen, call);
    } else if (strcmp(name, "chr") == 0) {
        // hodnota mimo 0 az 127 skonci chybou interpretu 58
        genExpr(gen, call->args[0]);
        emit(gen, "INT2CHARS\n");
    } else {
        return false;
    }

    return true;
}

void genAnd(CodeGen *gen, BinaryNode *binary) {
    int falseLabel = newLabel(gen);
    int endLabel = newLabel(gen);

    // skratove vyhodnotenie, pri false vlavo sa pravy operand nevyhodnoti
    genExpr(gen, binary->left);
    emit(gen, "PUSHS bool@false\n");
    emit(gen, "JUMPIFEQS L$%d\n", falseLabel);
    genExpr(gen, binary->right);
    emit(gen, "JUMP L$%d\n", endLabel);

    emit(gen, "LABEL L$%d\n", falseLabel);
    emit(gen, "PUSHS bool@false\n");
    emit(gen, "LABEL L$%d\n", endLabel);
}

void genOr(CodeGen *gen, BinaryNode *binary) {
    int trueLabel = newLabel(gen);
    int endLabel = newLabel(gen);

    // pri true vlavo je vysledok true a pravy operand sa nevyhodnoti
    genExpr(gen, binary->left);
    emit(gen, "PUSHS bool@true\n");
    emit(gen, "JUMPIFEQS L$%d\n", trueLabel);
    genExpr(gen, binary->right);
    emit(gen, "JUMP L$%d\n", endLabel);

    emit(gen, "LABEL L$%d\n", trueLabel);
    emit(gen, "PUSHS bool@true\n");
    emit(gen, "LABEL L$%d\n", endLabel);
}

void genFloor(CodeGen *gen) {
    int doneLabel = newLabel(gen);
    int notLessLabel = newLabel(gen);

    // cele cislo (aj -0.0) ostava tak, ako je
    emit(gen, "POPS LF@%%t0\n");
    emit(gen, "ISINT LF@%%t1 LF@%%t0\n");
    emit(gen, "JUMPIFEQ L$%d LF@%%t1 bool@true\n", doneLabel);

    // orezanie desatinnej casti ide k nule, pri zapornom cisle treba este odcitat 1
    emit(gen, "FLOAT2INT LF@%%t1 LF@%%t0\n");
    emit(gen, "INT2FLOAT LF@%%t1 LF@%%t1\n");
    emit(gen, "LT LF@%%t2 LF@%%t0 LF@%%t1\n");
    emit(gen, "JUMPIFEQ L$%d LF@%%t2 bool@false\n", notLessLabel);
    emit(gen, "SUB LF@%%t1 LF@%%t1 float@%a\n", 1.0);
    emit(gen, "LABEL L$%d\n", notLessLabel);
    emit(gen, "MOVE LF@%%t0 LF@%%t1\n");

    emit(gen, "LABEL L$%d\n", doneLabel);
    emit(gen, "PUSHS LF@%%t0\n");
}

size_t getFieldIndex(const ClassDefNode *classDef, const char *name) {
    for (size_t fieldIndex = 0; fieldIndex < classDef->fields.params_count; fieldIndex++) {
        if (strcmp(classDef->fields.params[fieldIndex].name, name) == 0) {
            return fieldIndex;
        }
    }

    // sem sa po semantike neda dostat
    return 0;
}

void genFieldAccess(CodeGen *gen, MemberNode *member) {
    const ClassDefNode *classDef = (const ClassDefNode *)member->symbol->definition;

    // na indexe 0 je meno triedy, atributy idu az za nim
    size_t index = getFieldIndex(classDef, member->name) + 1;

    emit(gen, "POPS LF@%%t0\n");
    emit(gen, "ARRAYGET LF@%%t0 LF@%%t0 int@%zu\n", index);
    emit(gen, "PUSHS LF@%%t0\n");
}

void genMember(CodeGen *gen, MemberNode *member) {
    // clen sa pocita z hodnoty pred bodkou, pri retazeni (x.floor.toInt)
    // je objektom dalsi clen, ten uz necha na zasobniku svoju hodnotu
    ASTNode *object = member->object;
    genExpr(gen, object);

    if (member->memberKind == MEM_EXT_METHOD) {
        // metoda bez zatvoriek, hodnota pred bodkou je prijemca
        emit(gen, "CALL ");
        emitFunctionLabel(gen, (DefFuncNode *)member->symbol->definition);
        emit(gen, "\n");
        return;
    }

    if (member->memberKind == MEM_FIELD) {
        genFieldAccess(gen, member);
        return;
    }

    if (member->memberKind != MEM_BUILTIN) {
        return;
    }

    if (strcmp(member->name, "toString") == 0) {
        // x.toString je to iste ako toStr(x), typ je typ objektu (aj ked je to clen)
        genValueToStr(gen, object->type);
    } else if (strcmp(member->name, "toInt") == 0) {
        // d.toInt je to iste ako toInt(d)
        emit(gen, "FLOAT2INTS\n");
    } else if (strcmp(member->name, "floor") == 0) {
        genFloor(gen);
    } else if (strcmp(member->name, "length") == 0) {
        // s.length je to iste ako strLength(s)
        emit(gen, "POPS LF@%%t0\n");
        emit(gen, "STRLEN LF@%%t0 LF@%%t0\n");
        emit(gen, "PUSHS LF@%%t0\n");
    }
}

void genConcat(CodeGen *gen) {
    // CONCAT nema zasobnikovu verziu, operandy sa vyberu do pomocnych premennych
    emit(gen, "POPS LF@%%t1\n");
    emit(gen, "POPS LF@%%t0\n");
    emit(gen, "CONCAT LF@%%t0 LF@%%t0 LF@%%t1\n");
    emit(gen, "PUSHS LF@%%t0\n");
}

void genInterp(CodeGen *gen, InterpNode *interp) {
    if (interp->partsCount == 0) {
        emit(gen, "PUSHS string@\n");
        return;
    }

    // kazda cast sa prevedie ako cez toStr a pripoji sa k doterajsiemu retazcu
    for (size_t partIndex = 0; partIndex < interp->partsCount; partIndex++) {
        ASTNode *part = interp->parts[partIndex];

        genExpr(gen, part);
        genValueToStr(gen, part->type);

        if (partIndex > 0) {
            genConcat(gen);
        }
    }
}

void genId(CodeGen *gen, IDNode *id) {
    if (id->idKind == ID_FUNC_CALL) {
        // funkcia bez zoznamu parametrov sa vola len menom, bez zatvoriek,
        // telo sa vyhodnoti pri kazdom pouziti znova
        const DefFuncNode *function = (const DefFuncNode *)id->symbol->definition;
        emit(gen, "CALL f$%s\n", function->name);
        return;
    }

    if (id->idKind == ID_FUNC_REF) {
        // funkcia ako hodnota je retazec s jej navestim, vola sa cez CALL LF@x
        emit(gen, "PUSHS string@");
        emitFunctionLabel(gen, (DefFuncNode *)id->symbol->definition);
        emit(gen, "\n");
        return;
    }

    emit(gen, "PUSHS ");
    emitVar(gen, id->name, id->symbol);
    emit(gen, "\n");
}

void genUnionEquals(CodeGen *gen) {
    int sameTypeLabel = newLabel(gen);
    int endLabel = newLabel(gen);

    // EQ na roznych typoch skonci chybou 53, preto sa najprv porovnaju typy
    emit(gen, "POPS LF@%%t1\n");
    emit(gen, "POPS LF@%%t0\n");
    emit(gen, "TYPE LF@%%t2 LF@%%t0\n");
    emit(gen, "TYPE LF@%%t3 LF@%%t1\n");
    emit(gen, "JUMPIFEQ L$%d LF@%%t2 LF@%%t3\n", sameTypeLabel);

    // rozne typy sa nikdy nerovnaju
    emit(gen, "PUSHS bool@false\n");
    emit(gen, "JUMP L$%d\n", endLabel);

    emit(gen, "LABEL L$%d\n", sameTypeLabel);
    emit(gen, "PUSHS LF@%%t0\n");
    emit(gen, "PUSHS LF@%%t1\n");
    emit(gen, "EQS\n");
    emit(gen, "LABEL L$%d\n", endLabel);
}

void genBinary(CodeGen *gen, BinaryNode *binary) {
    if (binary->operator == TOK_AND) {
        genAnd(gen, binary);
        return;
    }
    if (binary->operator == TOK_OR) {
        genOr(gen, binary);
        return;
    }

    // operandy sa vyhodnocuju zlava doprava, pravy je na vrchu zasobnika
    genExpr(gen, binary->left);
    genExpr(gen, binary->right);

    bool isUnion = getKind(binary->left) == TY_UNION || getKind(binary->right) == TY_UNION;

    if (binary->operator == TOK_PLUS && getKind((ASTNode *)binary) == TY_STRING) {
        genConcat(gen);
    } else if (isUnion && binary->operator == TOK_EQUAL) {
        genUnionEquals(gen);
    } else if (isUnion && binary->operator == TOK_NOT_EQUAL) {
        genUnionEquals(gen);
        emit(gen, "NOTS\n");
    } else if (binary->operator == TOK_PLUS) {
        emit(gen, "ADDS\n");
    } else if (binary->operator == TOK_MINUS) {
        emit(gen, "SUBS\n");
    } else if (binary->operator == TOK_MULTIPLY) {
        emit(gen, "MULS\n");
    } else if (binary->operator == TOK_DIVIDE) {
        // delenie Double je desatinne, delenie Int celociselne
        if (getKind((ASTNode *)binary) == TY_DOUBLE) {
            emit(gen, "DIVS\n");
        } else {
            emit(gen, "IDIVS\n");
        }
    } else if (binary->operator == TOK_LESS) {
        emit(gen, "LTS\n");
    } else if (binary->operator == TOK_GREATER) {
        emit(gen, "GTS\n");
    } else if (binary->operator == TOK_EQUAL) {
        emit(gen, "EQS\n");
    } else if (binary->operator == TOK_LESS_EQUAL) {
        // a <= b je to iste ako !(a > b)
        emit(gen, "GTS\n");
        emit(gen, "NOTS\n");
    } else if (binary->operator == TOK_GREATER_EQUAL) {
        // a >= b je to iste ako !(a < b)
        emit(gen, "LTS\n");
        emit(gen, "NOTS\n");
    } else if (binary->operator == TOK_NOT_EQUAL) {
        emit(gen, "EQS\n");
        emit(gen, "NOTS\n");
    }
}

void genUnary(CodeGen *gen, UnaryNode *unary) {
    genExpr(gen, unary->right);

    // -x sa pocita ako x * -1
    if (unary->operator == TOK_MINUS) {
        if (getKind((ASTNode *)unary) == TY_DOUBLE) {
            emit(gen, "PUSHS float@%a\n", -1.0);
        } else {
            emit(gen, "PUSHS int@-1\n");
        }
        emit(gen, "MULS\n");
    } else if (unary->operator == TOK_NOT) {
        emit(gen, "NOTS\n");
    }
}

void genExpr(CodeGen *gen, ASTNode *node) {
    // kazdy vyraz necha na zasobniku presne jednu hodnotu
    if (node->kind == AST_LIT) {
        emit(gen, "PUSHS ");
        emitLiteral(gen, (LiteralNode *)node);
        emit(gen, "\n");
    } else if (node->kind == AST_ID) {
        genId(gen, (IDNode *)node);
    } else if (node->kind == AST_BINARY) {
        genBinary(gen, (BinaryNode *)node);
    } else if (node->kind == AST_UNARY) {
        genUnary(gen, (UnaryNode *)node);
    } else if (node->kind == AST_CONVERT) {
        // implicitny prevod Int na Double, ktory vlozila semantika
        genExpr(gen, ((ConvertNode *)node)->operand);
        emit(gen, "INT2FLOATS\n");
    } else if (node->kind == AST_CALL) {
        // volanie funkcie moze byt operandom ako hocijaky iny vyraz
        genCall(gen, (CallNode *)node);
    } else if (node->kind == AST_MEMBER) {
        genMember(gen, (MemberNode *)node);
    } else if (node->kind == AST_INTERP) {
        genInterp(gen, (InterpNode *)node);
    } else if (node->kind == AST_IF) {
        // if ako vyraz, kazda vetva necha na zasobniku svoju hodnotu
        genIf(gen, (IfNode *)node, true);
    } else if (node->kind == AST_MATCH) {
        // match ako vyraz, hodnotu necha vykonana vetva
        genMatch(gen, (MatchNode *)node, true);
    } else if (node->kind == AST_BLOCK || node->kind == AST_EXPR_STATEMENT ||
               node->kind == AST_VAR_DEF || node->kind == AST_ASSIGN || node->kind == AST_WHILE ||
               node->kind == AST_FOR || node->kind == AST_RETURN) {
        // blok alebo prikaz na mieste hodnoty (napr. vetva if prevedena na Double)
        genValue(gen, node);
    } else {
        emit(gen, "PUSHS nil@nil\n");
    }
}

void defineLocal(CodeGen *gen, const char *name, Symbol *symbol) {
    // kazda premenna dostane vlastne cislo, takze prekryte premenne maju rozne mena
    gen->varCounter++;
    symbol->cgId = gen->varCounter;

    // DEFVAR ide na zaciatok, aby sa v cykle nevykonal viackrat
    emitDeclaration(gen, "DEFVAR LF@%s$%d\n", name, symbol->cgId);
}

int newHiddenVar(CodeGen *gen) {
    // skryta premenna nema meno zo zdrojaku, %h sa s premennymi programu nepomiesa
    gen->hiddenCounter++;
    emitDeclaration(gen, "DEFVAR LF@%%h%d\n", gen->hiddenCounter);
    return gen->hiddenCounter;
}

void genVarDef(CodeGen *gen, VarOrValNode *varDef) {
    defineLocal(gen, varDef->name, varDef->symbol);
    genExpr(gen, varDef->init);
    emit(gen, "POPS ");
    emitVar(gen, varDef->name, varDef->symbol);
    emit(gen, "\n");
}

void genAssign(CodeGen *gen, AssignNode *assign) {
    genExpr(gen, assign->value);
    emit(gen, "POPS ");
    emitVar(gen, assign->name, assign->symbol);
    emit(gen, "\n");
}

void genJumpIfFalse(CodeGen *gen, ASTNode *condition, int label) {
    // podmienka je lubovolny vyraz typu Boolean, jeho hodnota je na zasobniku
    genExpr(gen, condition);
    emit(gen, "PUSHS bool@false\n");
    emit(gen, "JUMPIFEQS L$%d\n", label);
}

void genBranch(CodeGen *gen, ASTNode *branch, bool valueUsed) {
    if (valueUsed) {
        genValue(gen, branch);
    } else {
        genStatement(gen, branch);
    }
}

void genIfChain(CodeGen *gen, IfNode *ifNode, int endLabel, bool valueUsed) {
    int elseLabel = newLabel(gen);

    // pri nepravdivej podmienke sa skace na else vetvu
    genJumpIfFalse(gen, ifNode->condition, elseLabel);
    genBranch(gen, ifNode->thenBranch, valueUsed);
    emit(gen, "JUMP L$%d\n", endLabel);
    emit(gen, "LABEL L$%d\n", elseLabel);

    if (ifNode->elseBranch == NULL) {
        // if bez else, pri false sa then vetva len preskoci, hodnota je ()
        if (valueUsed) {
            emit(gen, "PUSHS nil@nil\n");
        }
        return;
    }

    if (ifNode->elseBranch->kind == AST_IF) {
        // else if pokracuje v tom istom retazci, koniec je spolocny
        genIfChain(gen, (IfNode *)ifNode->elseBranch, endLabel, valueUsed);
    } else {
        genBranch(gen, ifNode->elseBranch, valueUsed);
    }
}

void genIf(CodeGen *gen, IfNode *ifNode, bool valueUsed) {
    int endLabel = newLabel(gen);

    genIfChain(gen, ifNode, endLabel, valueUsed);
    emit(gen, "LABEL L$%d\n", endLabel);
}

void genWhile(CodeGen *gen, WhileNode *whileNode) {
    int startLabel = newLabel(gen);
    int endLabel = newLabel(gen);

    emit(gen, "LABEL L$%d\n", startLabel);
    genJumpIfFalse(gen, whileNode->condition, endLabel);

    genStatement(gen, whileNode->body);
    emit(gen, "JUMP L$%d\n", startLabel);
    emit(gen, "LABEL L$%d\n", endLabel);
}

void genForCheck(CodeGen *gen, ForNode *forNode, int endVar, bool upward, int endLabel) {
    RangeNode *range = (RangeNode *)forNode->source;

    emit(gen, "PUSHS ");
    emitVar(gen, forNode->varName, forNode->symbol);
    emit(gen, "\n");
    emit(gen, "PUSHS LF@%%h%d\n", endVar);

    // smerom hore: to konci pri i > end, until pri !(i < end)
    // smerom dole: to konci pri i < end, until pri !(i > end)
    if (range->inclusive) {
        emit(gen, upward ? "GTS\n" : "LTS\n");
        emit(gen, "PUSHS bool@true\n");
    } else {
        emit(gen, upward ? "LTS\n" : "GTS\n");
        emit(gen, "PUSHS bool@false\n");
    }
    emit(gen, "JUMPIFEQS L$%d\n", endLabel);
}

void genFor(CodeGen *gen, ForNode *forNode) {
    RangeNode *range = (RangeNode *)forNode->source;
    int endVar = newHiddenVar(gen);
    int stepVar = newHiddenVar(gen);
    int startLabel = newLabel(gen);
    int downLabel = newLabel(gen);
    int bodyLabel = newLabel(gen);
    int endLabel = newLabel(gen);

    // riadiaca premenna je zaroven pocitadlo, v tele sa menit neda
    defineLocal(gen, forNode->varName, forNode->symbol);

    // meze aj krok sa vyhodnotia len raz pred prvou iteraciou, zlava doprava
    genExpr(gen, range->from);
    emit(gen, "POPS ");
    emitVar(gen, forNode->varName, forNode->symbol);
    emit(gen, "\n");
    genExpr(gen, range->to);
    emit(gen, "POPS LF@%%h%d\n", endVar);

    if (range->step != NULL) {
        int stepOkLabel = newLabel(gen);

        genExpr(gen, range->step);
        emit(gen, "POPS LF@%%h%d\n", stepVar);

        // nulovy krok sa pri literali chyti uz v semantike, inak az tu za behu
        emit(gen, "JUMPIFNEQ L$%d LF@%%h%d int@0\n", stepOkLabel, stepVar);
        emit(gen, "EXIT int@40\n");
        emit(gen, "LABEL L$%d\n", stepOkLabel);
    } else {
        emit(gen, "MOVE LF@%%h%d int@1\n", stepVar);
    }

    // smer porovnania sa urci az za behu podla znamienka kroku
    emit(gen, "LABEL L$%d\n", startLabel);
    emit(gen, "LT LF@%%t0 LF@%%h%d int@0\n", stepVar);
    emit(gen, "JUMPIFEQ L$%d LF@%%t0 bool@true\n", downLabel);
    genForCheck(gen, forNode, endVar, true, endLabel);
    emit(gen, "JUMP L$%d\n", bodyLabel);
    emit(gen, "LABEL L$%d\n", downLabel);
    genForCheck(gen, forNode, endVar, false, endLabel);
    emit(gen, "LABEL L$%d\n", bodyLabel);

    // ked niektora straz neplati, telo sa preskoci a ide sa na dalsiu iteraciu
    int nextLabel = newLabel(gen);

    for (size_t guardIndex = 0; guardIndex < forNode->guardsCount; guardIndex++) {
        genJumpIfFalse(gen, forNode->guards[guardIndex], nextLabel);
    }

    genStatement(gen, forNode->body);

    emit(gen, "LABEL L$%d\n", nextLabel);
    emit(gen, "ADD ");
    emitVar(gen, forNode->varName, forNode->symbol);
    emit(gen, " ");
    emitVar(gen, forNode->varName, forNode->symbol);
    emit(gen, " LF@%%h%d\n", stepVar);
    emit(gen, "JUMP L$%d\n", startLabel);
    emit(gen, "LABEL L$%d\n", endLabel);
}

const char *getRuntimeTypeName(Type *type) {
    type = getRealType(type);

    if (type == NULL) {
        return "";
    }

    // rovnake mena, ake vracia instrukcia TYPE
    switch (type->kind) {
    case TY_INT:
        return "int";
    case TY_DOUBLE:
        return "float";
    case TY_STRING:
        return "string";
    case TY_BOOL:
        return "bool";
    case TY_UNIT:
        return "nil";
    case TY_CLASS:
    case TY_LIST:
        return "array";
    default:
        return "";
    }
}

void genLiteralPattern(CodeGen *gen, CaseNode *caseNode, int subjectVar, int nextLabel) {
    // hodnota zjednotenia moze mat iny typ ako literal, EQ by skoncil chybou 53
    emit(gen, "TYPE LF@%%t0 LF@%%h%d\n", subjectVar);
    emit(gen, "JUMPIFNEQ L$%d LF@%%t0 string@%s\n", nextLabel,
         getRuntimeTypeName(caseNode->literal->type));

    // literal moze byt aj -1, preto sa vyhodnoti ako vyraz
    genExpr(gen, caseNode->literal);
    emit(gen, "POPS LF@%%t1\n");
    emit(gen, "JUMPIFNEQ L$%d LF@%%h%d LF@%%t1\n", nextLabel, subjectVar);
}

void genTagTest(CodeGen *gen, const char *tag, int subjectVar, int failLabel) {
    // trieda je pole, na indexe 0 je jej meno
    emit(gen, "TYPE LF@%%t0 LF@%%h%d\n", subjectVar);
    emit(gen, "JUMPIFNEQ L$%d LF@%%t0 string@array\n", failLabel);
    emit(gen, "ARRAYGET LF@%%t0 LF@%%h%d int@0\n", subjectVar);
    emit(gen, "JUMPIFNEQ L$%d LF@%%t0 ", failLabel);
    emitString(gen, tag);
    emit(gen, "\n");
}

void genTypeTest(CodeGen *gen, Type *type, int subjectVar, int failLabel) {
    type = getRealType(type);

    if (type->kind == TY_UNION) {
        // hodnota patri do zjednotenia, ak patri do niektoreho jeho clena
        int okLabel = newLabel(gen);

        for (size_t memberIndex = 0; memberIndex < type->members.len; memberIndex++) {
            int memberFailLabel = newLabel(gen);

            genTypeTest(gen, type->members.items[memberIndex], subjectVar, memberFailLabel);
            emit(gen, "JUMP L$%d\n", okLabel);
            emit(gen, "LABEL L$%d\n", memberFailLabel);
        }

        emit(gen, "JUMP L$%d\n", failLabel);
        emit(gen, "LABEL L$%d\n", okLabel);
        return;
    }

    if (type->kind == TY_CLASS) {
        genTagTest(gen, ((ClassDefNode *)type->cls->definition)->name, subjectVar, failLabel);
        return;
    }

    emit(gen, "TYPE LF@%%t0 LF@%%h%d\n", subjectVar);
    emit(gen, "JUMPIFNEQ L$%d LF@%%t0 string@%s\n", failLabel, getRuntimeTypeName(type));
}

void genConstructorPattern(CodeGen *gen, CaseNode *caseNode, int subjectVar, int nextLabel) {
    genTagTest(gen, caseNode->className, subjectVar, nextLabel);

    // kazda vazba dostane hodnotu atributu na rovnakom mieste
    for (size_t binderIndex = 0; binderIndex < caseNode->ctorBinders.params_count; binderIndex++) {
        Param *binder = &caseNode->ctorBinders.params[binderIndex];

        if (binder->symbol == NULL) {
            continue;
        }

        defineLocal(gen, binder->name, binder->symbol);
        emit(gen, "ARRAYGET ");
        emitVar(gen, binder->name, binder->symbol);
        emit(gen, " LF@%%h%d int@%zu\n", subjectVar, binderIndex + 1);
    }
}

void genBindSubject(CodeGen *gen, Param *binder, int subjectVar) {
    // vazba _ premennu nevytvara
    if (binder->symbol == NULL) {
        return;
    }

    defineLocal(gen, binder->name, binder->symbol);
    emit(gen, "MOVE ");
    emitVar(gen, binder->name, binder->symbol);
    emit(gen, " LF@%%h%d\n", subjectVar);
}

void genPattern(CodeGen *gen, CaseNode *caseNode, int subjectVar, int nextLabel) {
    if (caseNode->patternKind == PAT_WILDCARD) {
        // vzor _ uspeje vzdy
        return;
    }

    if (caseNode->patternKind == PAT_LITERAL) {
        genLiteralPattern(gen, caseNode, subjectVar, nextLabel);
        return;
    }

    if (caseNode->patternKind == PAT_TYPED) {
        // n: Int, hodnota sa po kontrole typu ulozi do novej premennej
        genTypeTest(gen, caseNode->binder.type, subjectVar, nextLabel);
        genBindSubject(gen, &caseNode->binder, subjectVar);
        return;
    }

    // Point(x, y)
    genConstructorPattern(gen, caseNode, subjectVar, nextLabel);
}

void genMatch(CodeGen *gen, MatchNode *match, bool valueUsed) {
    int subjectVar = newHiddenVar(gen);
    int endLabel = newLabel(gen);

    // porovnavana hodnota sa vyhodnoti raz a ulozi do skrytej premennej
    genExpr(gen, match->subject);
    emit(gen, "POPS LF@%%h%d\n", subjectVar);

    // vetvy sa skusaju postupne, vykona sa prva, ktorej vzor sedi
    for (size_t caseIndex = 0; caseIndex < match->casesCount; caseIndex++) {
        CaseNode *caseNode = (CaseNode *)match->cases[caseIndex];
        int nextLabel = newLabel(gen);

        genPattern(gen, caseNode, subjectVar, nextLabel);

        // straz sa vyhodnoti az po vzore, uz vidi jeho premenne
        if (caseNode->guard != NULL) {
            genJumpIfFalse(gen, caseNode->guard, nextLabel);
        }

        genBranch(gen, caseNode->body, valueUsed);
        emit(gen, "JUMP L$%d\n", endLabel);
        emit(gen, "LABEL L$%d\n", nextLabel);
    }

    emit(gen, "LABEL L$%d\n", endLabel);
}

void genReturn(CodeGen *gen, ReturnNode *returnNode) {
    if (isUnitType(gen->function->returnType)) {
        // funkcia typu Unit vrati () az na konci, hodnota za return sa len vyhodnoti
        if (returnNode->returnValue != NULL) {
            genExpr(gen, returnNode->returnValue);
            emit(gen, "POPS LF@%%trash\n");
        }
    } else if (returnNode->returnValue == NULL) {
        // prazdny return vracia ()
        emit(gen, "PUSHS nil@nil\n");
    } else {
        genExpr(gen, returnNode->returnValue);
    }

    emit(gen, "JUMP ");
    emitFunctionLabel(gen, gen->function);
    emit(gen, "$end\n");
}

void genBlock(CodeGen *gen, BlockNode *block) {
    for (size_t commandIndex = 0; commandIndex < block->commandOrderCount; commandIndex++) {
        genStatement(gen, block->commandOrder[commandIndex]);
    }
}

void genValue(CodeGen *gen, ASTNode *node) {
    if (node->kind == AST_BLOCK && ((BlockNode *)node)->commandOrderCount == 0) {
        // prazdny blok ma hodnotu ()
        emit(gen, "PUSHS nil@nil\n");
    } else if (node->kind == AST_BLOCK) {
        BlockNode *block = (BlockNode *)node;

        // hodnotu bloku urcuje jeho posledny prikaz
        for (size_t commandIndex = 0; commandIndex + 1 < block->commandOrderCount; commandIndex++) {
            genStatement(gen, block->commandOrder[commandIndex]);
        }
        genValue(gen, block->commandOrder[block->commandOrderCount - 1]);
    } else if (node->kind == AST_EXPR_STATEMENT) {
        genValue(gen, ((LonelyExprNode *)node)->expr);
    } else if (node->kind == AST_VAR_DEF || node->kind == AST_ASSIGN || node->kind == AST_WHILE ||
               node->kind == AST_FOR || node->kind == AST_RETURN) {
        // definicia, priradenie a cykly nemaju hodnotu, vysledok je ()
        // za returnom sa sem program nedostane
        genStatement(gen, node);
        emit(gen, "PUSHS nil@nil\n");
    } else {
        genExpr(gen, node);
    }
}

void genStatement(CodeGen *gen, ASTNode *node) {
    if (node == NULL) {
        return;
    }

    if (node->kind == AST_BLOCK) {
        genBlock(gen, (BlockNode *)node);
    } else if (node->kind == AST_EXPR_STATEMENT) {
        genStatement(gen, ((LonelyExprNode *)node)->expr);
    } else if (node->kind == AST_VAR_DEF) {
        genVarDef(gen, (VarOrValNode *)node);
    } else if (node->kind == AST_ASSIGN) {
        genAssign(gen, (AssignNode *)node);
    } else if (node->kind == AST_IF) {
        genIf(gen, (IfNode *)node, false);
    } else if (node->kind == AST_WHILE) {
        genWhile(gen, (WhileNode *)node);
    } else if (node->kind == AST_FOR) {
        genFor(gen, (ForNode *)node);
    } else if (node->kind == AST_MATCH) {
        genMatch(gen, (MatchNode *)node, false);
    } else if (node->kind == AST_RETURN) {
        genReturn(gen, (ReturnNode *)node);
    } else {
        // vyraz ako prikaz (aj volanie) sa vyhodnoti a jeho hodnota sa zahodi
        genExpr(gen, node);
        emit(gen, "POPS LF@%%trash\n");
    }
}

void writeBuffers(CodeGen *gen) {
    fputs(gen->declarations.data, gen->out);
    fputs(gen->code.data, gen->out);

    // buffre sa vyprazdnia pre dalsiu funkciu
    dynstr_clear(&gen->declarations);
    dynstr_clear(&gen->code);
}

void genParams(CodeGen *gen, DefFuncNode *function) {
    for (size_t listIndex = 0; listIndex < function->paramListCount; listIndex++) {
        ParamList *paramList = &function->paramList[listIndex];

        for (size_t paramIndex = 0; paramIndex < paramList->params_count; paramIndex++) {
            Param *param = &paramList->params[paramIndex];
            defineLocal(gen, param->name, param->symbol);
        }
    }

    Param *receiver = function->symbol->receiver;

    if (receiver != NULL) {
        emitDeclaration(gen, "DEFVAR LF@%s$0\n", receiver->name);
    }

    // argumenty su na zasobniku zlava doprava, takze sa vyberaju od posledneho
    for (size_t listIndex = function->paramListCount; listIndex > 0; listIndex--) {
        ParamList *paramList = &function->paramList[listIndex - 1];

        for (size_t paramIndex = paramList->params_count; paramIndex > 0; paramIndex--) {
            Param *param = &paramList->params[paramIndex - 1];

            emit(gen, "POPS ");
            emitVar(gen, param->name, param->symbol);
            emit(gen, "\n");
        }
    }

    // prijemca metody je na zasobniku pred vsetkymi argumentmi
    if (receiver != NULL) {
        emit(gen, "POPS LF@%s$0\n", receiver->name);
    }
}

void genFunction(CodeGen *gen, DefFuncNode *function) {
    // navestie funkcie ide na zaciatok kodu, este pred DEFVAR
    emit(gen, "LABEL ");
    emitFunctionLabel(gen, function);
    emit(gen, "\n");
    emit(gen, "CREATEFRAME\n");
    emit(gen, "PUSHFRAME\n");
    fputs(gen->code.data, gen->out);
    dynstr_clear(&gen->code);

    // pomocne premenne ma kazda funkcia vo svojom ramci
    emitHelperVars(gen);
    genParams(gen, function);

    gen->function = function;

    if (isUnitType(function->returnType)) {
        // pri type Unit sa hodnota tela zahodi a funkcia vrati ()
        genStatement(gen, function->body);
        emit(gen, "LABEL ");
        emitFunctionLabel(gen, function);
        emit(gen, "$end\n");
        emit(gen, "PUSHS nil@nil\n");
    } else {
        // navratova hodnota je hodnota tela, ostane na zasobniku
        genValue(gen, function->body);

        // sem skace return, hodnota uz je na zasobniku
        emit(gen, "LABEL ");
        emitFunctionLabel(gen, function);
        emit(gen, "$end\n");
    }

    if (gen->error != ERR_OK) {
        return;
    }

    // kazda funkcia ma vlastny lokalny ramec
    writeBuffers(gen);
    fprintf(gen->out, "POPFRAME\n");
    fprintf(gen->out, "RETURN\n");
}

void useHelper(CodeGen *gen, HelperKind helper) { gen->helperUsed[helper] = true; }

void writeHelper(CodeGen *gen, HelperKind helper) {
    // kazda pomocna funkcia ma vlastny ramec, argumenty berie zo zasobnika
    // a vysledok necha na zasobniku, rovnako ako funkcie z programu
    const char *name = "";
    emitHelperVars(gen);

    if (helper == HELPER_SUBSTR) {
        name = "$subStr";
        genSubStrHelper(gen);
    } else if (helper == HELPER_ORD) {
        name = "$ord";
        genOrdHelper(gen);
    } else if (helper == HELPER_TOSTR_ANY) {
        name = "$toStr_any";
        genToStrAnyHelper(gen);
    }

    if (gen->error != ERR_OK) {
        return;
    }

    fprintf(gen->out, "LABEL %s\n", name);
    fprintf(gen->out, "CREATEFRAME\n");
    fprintf(gen->out, "PUSHFRAME\n");
    writeBuffers(gen);
    fprintf(gen->out, "POPFRAME\n");
    fprintf(gen->out, "RETURN\n");
}

void writeHelpers(CodeGen *gen) {
    bool isWritten[HELPER_COUNT];
    bool hasWrittenAny = true;

    for (int helper = 0; helper < HELPER_COUNT; helper++) {
        isWritten[helper] = false;
    }

    // pomocna funkcia moze pouzit dalsiu pomocnu funkciu, preto sa opakuje,
    // kym sa nevypisu vsetky pouzite
    while (hasWrittenAny && gen->error == ERR_OK) {
        hasWrittenAny = false;

        for (int helper = 0; helper < HELPER_COUNT; helper++) {
            if (gen->helperUsed[helper] && !isWritten[helper]) {
                writeHelper(gen, (HelperKind)helper);
                isWritten[helper] = true;
                hasWrittenAny = true;
            }
        }
    }
}

int codegenGenerate(ProgramNode *prog, FILE *out) {
    CodeGen gen;
    initCodeGen(&gen, out);

    const DefFuncNode *mainFunction = findMain(prog);
    if (mainFunction == NULL) {
        gen.error = ERR_INTERNAL;
    }

    // program zacne volanim main a po navrate skonci
    if (gen.error == ERR_OK) {
        emitHeader(&gen);
        // main sa nemusi volat main, navestie je podla skutocneho mena
        emit(&gen, "CALL f$%s\n", mainFunction->name);
        emit(&gen, "EXIT int@0\n");
        writeBuffers(&gen);
    }

    for (size_t itemIndex = 0; itemIndex < prog->itemsCount && gen.error == ERR_OK; itemIndex++) {
        ASTNode *item = prog->items[itemIndex];

        if (item->kind == AST_FUNCTION_DEF) {
            genFunction(&gen, (DefFuncNode *)item);
        } else if (item->kind == AST_EXTENSION) {
            const ExtensionNode *extension = (const ExtensionNode *)item;

            // metody su bezne funkcie s prijemcom ako prvym parametrom
            for (size_t methodIndex = 0; methodIndex < extension->methodsCount; methodIndex++) {
                genFunction(&gen, (DefFuncNode *)extension->methods[methodIndex]);
            }
        }
    }

    // pomocne funkcie za behu sa vypisu az za vsetkymi funkciami
    if (gen.error == ERR_OK) {
        writeHelpers(&gen);
    }

    int result = gen.error;
    freeCodeGen(&gen);
    return result;
}

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
#include "error.h"

#include <stdarg.h>
#include <string.h>

void emit(CodeGen *gen, const char *format, ...) {
    va_list args;

    va_start(args, format);
    vfprintf(gen->out, format, args);
    va_end(args);
}

void emitHeader(CodeGen *gen) { emit(gen, ".IFJcode26\n"); }

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

void emitLiteral(CodeGen *gen, LiteralNode *literal) {
    if (literal->litType == LIT_INT) {
        emit(gen, "int@%lld", literal->litValue.int_value);
    } else if (literal->litType == LIT_DOUBLE) {
        // desatinne cislo sa zapisuje vo formate %a
        emit(gen, "float@%a", literal->litValue.float_value);
    } else if (literal->litType == LIT_STRING) {
        emitString(gen, literal->litValue.string_value);
    } else if (literal->litType == LIT_BOOLEAN) {
        if (literal->litValue.bool_value) {
            emit(gen, "bool@true");
        } else {
            emit(gen, "bool@false");
        }
    } else if (literal->litType == LIT_UNIT) {
        // hodnota () je za behu nil
        emit(gen, "nil@nil");
    }
}

void initCodeGen(CodeGen *gen, FILE *out) {
    gen->out = out;
    gen->labelCounter = 0;
}

int newLabel(CodeGen *gen) {
    gen->labelCounter++;
    return gen->labelCounter;
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

    if (arg->kind != AST_LIT) {
        return;
    }

    LiteralNode *literal = (LiteralNode *)arg;

    // () sa vypisuje ako retazec "()"
    if (literal->litType == LIT_UNIT) {
        emit(gen, "WRITE string@()\n");
        return;
    }

    emit(gen, "WRITE ");
    emitLiteral(gen, literal);
    emit(gen, "\n");
}

void genCall(CodeGen *gen, CallNode *call) {
    if (call->callKind != CALL_BUILTIN || call->caller->kind != AST_ID) {
        return;
    }

    const char *name = ((IDNode *)call->caller)->name;

    if (strcmp(name, "print") == 0) {
        genPrint(gen, call);
    }
}

void genBinary(CodeGen *gen, BinaryNode *binary) {
    // operandy sa vyhodnocuju zlava doprava, pravy je na vrchu zasobnika
    genExpr(gen, binary->left);
    genExpr(gen, binary->right);

    if (binary->operator == TOK_PLUS) {
        emit(gen, "ADDS\n");
    } else if (binary->operator == TOK_MINUS) {
        emit(gen, "SUBS\n");
    } else if (binary->operator == TOK_MULTIPLY) {
        emit(gen, "MULS\n");
    } else if (binary->operator == TOK_DIVIDE) {
        emit(gen, "IDIVS\n");
    }
}

void genExpr(CodeGen *gen, ASTNode *node) {
    // kazdy vyraz necha na zasobniku presne jednu hodnotu
    if (node->kind == AST_LIT) {
        emit(gen, "PUSHS ");
        emitLiteral(gen, (LiteralNode *)node);
        emit(gen, "\n");
    } else if (node->kind == AST_ID) {
        emit(gen, "PUSHS GF@%s\n", ((IDNode *)node)->name);
    } else if (node->kind == AST_BINARY) {
        genBinary(gen, (BinaryNode *)node);
    }
}

void genVarDef(CodeGen *gen, VarOrValNode *varDef) {
    emit(gen, "DEFVAR GF@%s\n", varDef->name);
    genExpr(gen, varDef->init);
    emit(gen, "POPS GF@%s\n", varDef->name);
}

void genBlock(CodeGen *gen, BlockNode *block) {
    for (size_t commandIndex = 0; commandIndex < block->commandOrderCount; commandIndex++) {
        genStatement(gen, block->commandOrder[commandIndex]);
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
    } else if (node->kind == AST_CALL) {
        genCall(gen, (CallNode *)node);
    }
}

int codegenGenerate(ProgramNode *prog, FILE *out) {
    CodeGen gen;
    initCodeGen(&gen, out);

    DefFuncNode *mainFunction = findMain(prog);
    if (mainFunction == NULL) {
        return ERR_INTERNAL;
    }

    emitHeader(&gen);

    // telo main sa vykona priamo v globalnom ramci
    genStatement(&gen, mainFunction->body);
    emit(&gen, "EXIT int@0\n");

    return ERR_OK;
}

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

void emit(FILE *out, const char *format, ...) {
    va_list args;

    va_start(args, format);
    vfprintf(out, format, args);
    va_end(args);
}

void emitHeader(FILE *out) { emit(out, ".IFJcode26\n"); }

void emitString(FILE *out, const char *text) {
    emit(out, "string@");

    for (size_t charIndex = 0; text[charIndex] != '\0'; charIndex++) {
        unsigned char currentChar = (unsigned char)text[charIndex];

        // biele znaky, # a \ sa musia zapisat ako \ddd
        if (currentChar <= 32 || currentChar == 35 || currentChar == 92) {
            emit(out, "\\%03d", currentChar);
        } else {
            emit(out, "%c", currentChar);
        }
    }
}

void emitLiteral(FILE *out, LiteralNode *literal) {
    if (literal->litType == LIT_INT) {
        emit(out, "int@%lld", literal->litValue.int_value);
    } else if (literal->litType == LIT_DOUBLE) {
        // desatinne cislo sa zapisuje vo formate %a
        emit(out, "float@%a", literal->litValue.float_value);
    } else if (literal->litType == LIT_STRING) {
        emitString(out, literal->litValue.string_value);
    } else if (literal->litType == LIT_BOOLEAN) {
        if (literal->litValue.bool_value) {
            emit(out, "bool@true");
        } else {
            emit(out, "bool@false");
        }
    } else if (literal->litType == LIT_UNIT) {
        // hodnota () je za behu nil
        emit(out, "nil@nil");
    }
}

int codegenGenerate(ProgramNode *prog, FILE *out) {
    (void)prog;

    emitHeader(out);
    return ERR_OK;
}

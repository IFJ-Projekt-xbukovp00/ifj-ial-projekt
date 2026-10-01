/**
 * @file error.c
 * @brief Návratové kódy a hlásenie chýb prekladača
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

#include "error.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/** @copydoc error_exit */
_Noreturn void error_exit(ErrorCode code, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "error (%d): ", (int)code);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
    exit((int)code);
}

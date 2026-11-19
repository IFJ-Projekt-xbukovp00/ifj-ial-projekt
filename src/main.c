/**
 * @file main.c
 * @brief Vstupný bod prekladača IFJ26 -> IFJcode26
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
#include "parser.h"
#include "scanner.h"
#include "semantic.h"

#include <stdio.h>

/**
 * @brief Načíta program IFJ26 zo stdin a vypíše IFJcode26 na stdout.
 * @return 0 pri úspechu, inak kód chyby.
 */
int main(void) {
    scanner_init(stdin);
    int result = parser_parse();

    // TODO: Parser not implemented yet
    /*
    ASTNode *program = NULL;
    int result = parser_parse(&program);
    if (result != ERR_OK){
        return result;
    }

    Symtable table;
    ErrorCode semanticError = ERR_OK;
    symtableInit(&table);
    semanticAnalyze(program, &table, &semanticError);
    result = semanticError;

    symtableFree(&table);
    type_free_all();
    */

    return result;
}

/**
 * @file scanner.c
 * @brief Lexikálny analyzátor (konečný automat)
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

#include "scanner.h"

#include "error.h"

#define EOL '\n'

static FILE *source = NULL;

/** @copydoc scanner_init */
void scanner_init(FILE *input) { 
    source = input; 
}

/* TODO: implementácia konečného automatu */
/** @copydoc scanner_get_token */
int scanner_get_token(Token *token) {
    (void)source;
    token->type = TOK_EOF;
    return ERR_OK;
}

bool getLine(FILE *f){
    int c = fgetc(f);
    while (c != EOL && c != EOF){
        while (isspace(c)){
            if (c == '\t'){
                return false;
            }
            c = fgetc(f);
        }
        if (isdigit(c)){
            /*TODO: stavový automat pre detekciu čísla*/
        }
        else if (isalpha(c) || c == '_'){
            /*TODO: statový automat pre detekciu mena premennej*/
        }
        else if (c == '"' || c == '\''){
            /*TODO: statový automat pre reťazce*/
        }
        else if (c == '/'){
            /*TODO: stavový automat pre detenciu komentáru alebo operácie deleno*/
        }
        else if (c == '=' || c == '<' || c == '>' || c == '!' || c == '|'){
            /*TODO: stavový automat pre logické operátory*/
        }
        else if (c == '+' || c == '-' || c == '*' || c == '%'){
            /*TODO: stavový automat pre jednoduché matematické operácie*/
        }
    }
} 
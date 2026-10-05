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
#include "error.h"
#include "scanner.h"

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
/**
 * @brief funkcia na skenovanie čísla
 * @param f je skenovaný súbor
 * @param firstChar je prvé číslo 
 * @param line je číslo riadku
 * @param error je pointer, cez ktorý si vrátim typ chybovej hlášky
 * @returns Token, kde sú uložené informácie
 */
Token *scanNumber(FILE *f, int firstChar, int line, ErrorCode *error){
    Token *newToken = malloc(sizeof(Token));
    if (newToken == NULL){
        *error = ERR_INTERNAL;
        return NULL;
    }
    int c = firstChar;
    int i = 0;
    char tmp[100];
    bool isDouble = false;
    while (isdigit(c)){
        tmp[i++] = c;
        c = fgetc(f);
    }
    if (c == '.'){
        tmp[i++] = c;
        isDouble = true;
        while (isdigit(c)){
            tmp[i++] = c;
            c = fgetc(f);
        }
    }
    tmp[i] = '\0';
    strcpy(newToken->lexeme, tmp);
    if (isDouble){
        newToken->type = TOK_DOUBLE_LIT;
    }
    else{
        newToken->type = TOK_INT_LIT;
    }
    newToken->line = line;
    ungetc(c, f);
    *error = ERR_OK;
    return newToken;
}
/**
 * @brief funkcia na zistenie, či je naskenované slovo medzi kľúčovými slovami
 * @param word je slovo na kontrolu
 * @returns true ak je medzi nimi, inak false
 */
bool IskeyWord(char *word){
    return false;
}
/**
 * @brief funkcia na skenovanie identifikátora
 * @param f je skenovaný súbor
 * @param firstChar je prvé písmeno
 * @param line je číslo riadku
 * @param error je pointer, cez ktorý v prípade chyby vrátim typ chybovej hlášky
 * @returns Token, kde sú získané informácie
 */
Token *scanName(FILE *f, int firstChar, int line, ErrorCode *error){
    Token *newToken = malloc(sizeof(Token));
    if (newToken == NULL){
        *error = ERR_INTERNAL;
        return NULL;
    }
    int c = firstChar;
    int i = 0;
    char tmp[100];
    while (isalnum(c) || c == '_'){
        tmp[i++] = c;
        c = fgetc(f);
    }
    tmp[i] = '\0';
    strcpy(newToken->lexeme, tmp);
    if (keyWord(tmp)){
        newToken->type = TOK_KEYWORD;
    }
    else{
        newToken->type = TOK_IDENTIFIER;
    }
    newToken->line = line;
    *error = ERR_OK;
    ungetc(c, f);
    return newToken;
}
/**
 * @brief funkcia na načítanie stringov
 * @param f je skenovaný súbor
 * @param firstChar je prvý znak
 * @param line je číslo riadku
 * @param error je pointer, cez ktorý vrátim chybovú hlášku
 * @returns Token, kde sú získané informácie
 */
Token *getString(FILE *f, int firstChar, int line, ErrorCode *error){
    Token *newToken = malloc(sizeof(Token));
    if (newToken == NULL){
        *error = ERR_INTERNAL;
        return NULL;
    }
    int c = firstChar;
    int i = 0;
    char tmp[1024];
    if (c == '\''){ /*string s jedným znakom*/
        tmp[i++] = c;
        c = fgetc(f);
        if (c == EOL || c == EOF){
            *error = ERR_LEX;
            return NULL;
        }
        else if (c == '\\'){ /*\n, \t, \\*/
            tmp[i++] = c;
            c = fgetc(f);
            if (c != 'n' || c != 't' || c != '\\' || c != '\'' || c != '"'){
                *error = ERR_LEX;
                return NULL;
            }
            tmp[i++] = c;
        }
        else{
            tmp[i++] = c;
        }
        c = fgetc(f);
        if (c != '\''){
            *error = ERR_LEX;
            return NULL;
        }
        tmp[i++] = c;
        tmp[i] = '\0';
        newToken->type = TOK_CHAR_LIT;
    }
    else{ /*string s viacerými znakmi*/
        tmp[i++] = c;
        c = fgetc(f);
        int start_quote_seq = 1;
        while (start_quote_seq < 3 && c == '"'){ /*zisťujem úvodnú dĺžku sekvecie "*/
            tmp[i++] = c;
            c = fgetc(f);
            start_quote_seq++;
        }
        if (start_quote_seq == 2){/*""*/
            tmp[i] = '\0';
        }
        else if (start_quote_seq == 3){ /*začiatok multilane stringu*/
            int end_quote_seq = 0;
            while (end_quote_seq < 3){
                tmp[i++] = c;
                c = fgetc(f);
                if (c == '"'){
                    end_quote_seq++;
                }
                else{
                    end_quote_seq = 0;
                }
                if (i == 1024){
                    *error = ERR_INTERNAL; /*priveľký string na spracovanie*/
                    return NULL;
                }
            }
        }
        else{ /*klasický string*/
            while (c != '"'){
                if (c == EOL || c == EOF){
                    *error = ERR_LEX;
                    return NULL;
                }
                tmp[i++] = c;
                c = fgetc(f);
            }
        }
        tmp[i] = '\0';
        newToken->type = TOK_STRING_LIT;
    }
    strcpy(newToken->lexeme, tmp);
    newToken->line = line; 
    *error = ERR_OK;
    return newToken;
}
/**
 * @brief funkcia na načítanie riadku
 * @param f je čítaný súbor
 * @param line je číslo riadku
 * @returns true, ak je riadok načítaný celý, inak false
 */
bool getLine(FILE *f, int line){
    int c = fgetc(f);
    while (c != EOL && c != EOF){
        while (isspace(c)){
            if (c == '\t'){
                return false;
            }
            c = fgetc(f);
        }
        if (isdigit(c)){
            ErrorCode error;
            Token *newToken = scanNumber(f, c, line, &error);
            if (error != ERR_OK){
                return false;
            }
        }
        else if (isalpha(c) || c == '_'){
            ErrorCode error;
            Token *new_token = scanName(f, c, line, &error);
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
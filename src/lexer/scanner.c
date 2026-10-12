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
    int lexemeLen = strlen(tmp);
    newToken->lexeme.data = malloc(lexemeLen+1);
    if (newToken->lexeme.data == NULL){
        *error = ERR_INTERNAL;
        return NULL;
    }
    strcpy(newToken->lexeme.data, tmp);
    if (isDouble){
        newToken->type = TOK_DOUBLE_LIT;
    }
    else{
        newToken->type = TOK_INT_LIT;
    }
    newToken->line = line;
    newToken->lexeme.len = lexemeLen;
    ungetc(c, f);
    *error = ERR_OK;
    return newToken;
}
/**
 * @brief funkcia na zistenie, či je naskenované slovo medzi kľúčovými slovami
 * @param word je slovo na kontrolu
 * @param type je pointer, cez ktorý si vrátim pri zhode typ tokenu
 * @returns true ak je medzi nimi, inak false
 */
bool IskeyWord(char *word, TokenType *type){
    return false;
}
/**
 * @brief funkcia na skenovanie mena premennej alebo identifikátora
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
    int lexemeLen = strlen(tmp);
    newToken->lexeme.data = malloc(lexemeLen+1);
    if (newToken->lexeme.data == NULL){
        *error = ERR_INTERNAL;
        return NULL;
    }
    strcpy(newToken->lexeme.data, tmp);

    if (!IskeyWord(tmp, &newToken->type)){
        newToken->type = TOK_IDENTIFIER;
    }
    newToken->line = line;
    newToken->lexeme.len = lexemeLen;
    *error = ERR_OK;
    ungetc(c, f);
    return newToken;
}
/**
 * @brief funkcia na skenovanie stringov
 * @param f je skenovaný súbor
 * @param firstChar je prvý znak
 * @param line je číslo riadku
 * @param error je pointer, cez ktorý vrátim chybovú hlášku
 * @returns Token, kde sú získané informácie
 */
Token *scanString(FILE *f, int firstChar, int line, ErrorCode *error){
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
    int lexemeLen = strlen(tmp);
    newToken->lexeme.data = malloc(lexemeLen+1);
    if (newToken->lexeme.data == NULL){
        *error = ERR_INTERNAL;
        return NULL;
    }
    strcpy(newToken->lexeme.data, tmp);
    newToken->line = line; 
    newToken->lexeme.len = lexemeLen;
    *error = ERR_OK;
    ungetc(c, f);
    return newToken;
}
/**
 * @brief funkcia na detenovanie komentáru
 * @param newToken je token na zachytenie komentáru
 * @param f je skenovaný súbor
 * @param firstChar je znak značiaci typ komentára
 * @param line je číslo riadku začiatku komentáru
 * @param error je pointer, cez ktorý si vrátim chybovú hlášku
 * @return ..
 */
void scanComment(Token *newToken, FILE *f, int firstChar, int line, ErrorCode *error){
    int c = firstChar;
    char tmp[1000];
    int i = 0;
    int depth = 1;
    tmp[i++] = '/';
    if (c == '/'){ /*1-line comment*/
        while (c != EOL && c != EOF){
            tmp[i++] = c;
            c = fgetc(f);
        }
        newToken->type = TOK_1LINE_COMMENT;
    }
    else{ /*multi-line koment*/
        int CASE = 0;
        const int CASE_DEFAULT = 0;
        const int CASE_POSSIBLE_DEPTH_INC = 1;
        const int CASE_POSSIBLE_DEPTH_DEC = 2;
        while (depth > 0 && c != EOF){
            tmp[i++] = c;
            c = fgetc(f);
            switch (CASE){
                case CASE_DEFAULT:
                    if (c == '/'){
                        CASE = CASE_POSSIBLE_DEPTH_INC;
                    }
                    else if (c == '*'){
                        CASE = CASE_POSSIBLE_DEPTH_DEC;
                    }
                    break;
                case CASE_POSSIBLE_DEPTH_INC:
                    if (c == '*'){
                        depth++;
                    }
                    CASE = CASE_DEFAULT;
                    break;
                case CASE_POSSIBLE_DEPTH_DEC:
                    if (c == '/'){
                        depth--;
                    }
                    CASE = CASE_DEFAULT;
                    break;
            }
        }
        newToken->type = TOK_MULTI_LINE_COMMENT;
    }
    tmp[i] = '\0';
    int lexemeLen = strlen(tmp);
    newToken->lexeme.data = malloc(lexemeLen+1);
    if (newToken->lexeme.data == NULL){
        *error = ERR_INTERNAL;
        return NULL;
    }
    strcpy(newToken->lexeme.data, tmp);
    newToken->line = line;
    newToken->lexeme.len = lexemeLen;
    *error = ERR_OK;
    return *newToken;
}
/**
 * @brief funkcia na zistenie, či je '/' deleno alebo začiatok komentáru
 * @param f je skenovaný súbor
 * @param line je číslo riadku
 * @param error je pointer, cez ktorý si vrátim chybovú hlášku
 * @returns token s informáciami
 */
Token *scanCommentOrDiv(FILE *f, int line, ErrorCode *error){
    Token *newToken = malloc(sizeof(Token));
    if (newToken == NULL){
        *error = ERR_INETRNAL;
        return NULL;
    }
    int c = fgetc(f);
    if (c == '*' || c == '/'){ /*začiatok komentáru*/
        scanComment(newToken, f, c, newToken, line, error);
    }
    else if (c == '='){
        newToken->lexeme.data = malloc(3);
        if (newToken->lexeme.data == NULL){
            *error = ERR_INTERNAL;
            return NULL;
        }
        strcpy(newToken->lexeme.data, "/=");
        newToken->line = line;
        newToken->type = TOK_DIVIDE_EQUALS;
        newToken->lexeme.len = 2;
    }
    else if (isdigit(c) || isspace(c)){
        newToken->lexeme.data = malloc(2);
            if (newToken->lexeme.data == NULL){
            *error = ERR_INTERNAL;
            return NULL;
        }
        strcpy(newToken->lexeme.data, '/');
        newToken->line = line;
        newToken->type = TOK_DIVIDE;
        newToken->lexeme.len = 1;
    }
    else{
        *error = ERR_LEX;
        return NULL;
    }
    *error = ERR_OK;
    return newToken;
}
/**
 * @brief funkcia na skenovanie logických operátorov
 * @param f je skenovaný súbor
 * @param firstChar je prvý znak
 * @param line je číslo riadku 
 * @param error je pointer, cez ktorý si vrátim chybovú hlášku
 * @returns token s informáciami
 */
Token *scanLogicalExpression(FILE *f, int firstChar, int line, ErrorCode *error){
    Token *newToken = malloc(sizeof(Token));
    if (newToken == NULL){
        *error = ERR_INETRNAL;
        return NULL;
    }
    int c = firstChar;
    char tmp[3];
    int i = 0;
    if (c == '='){
        tmp[i++] = c;
        c = fgetc(f);
        if (c == '='){
            tmp[i++] = c;
            tmp[i] = '\0';
            newToken->type = TOK_EQUAL;
        }
        else if (c == '>'){
            tmp[i++] = c;
            tmp[i] = '\0';
            newToken->type = TOK_ARROW;
        }
        else{
            tmp[i] = '\0';
            newToken->type = TOK_ASSIGN;
            ungetc(c, f);
        }
    }
    else if (c == '<'){
        tmp[i++] = c;
        c = fgetc(f);
        if (c == '='){
            tmp[i++] = c;
            tmp[i] = '\0';
            newToken->type = TOK_LESS_EQUAL;
        }
        else if (c == '-'){
            tmp[i++] = c;
            tmp[i] = '\0';
            newToken->type = TOK_LARROW;
        }
        else{
            tmp[i] = '\0';
            newToken->type = TOK_LESS;
            ungetc(c, f);
        }
    }
    else if (c == '>'){
        tmp[i++] = c;
        c = fgetc(f);
        if (c == '='){
            tmp[i++] = c;
            tmp[i] = '\0';
            newToken->type = TOK_GREATER_EQUAL;
        }
        else{
            tmp[i] = '\0';
            newToken->type = TOK_GREATER;
            ungetc(c, f);
        }
    }
    else if (c == '!'){
        tmp[i++] = c;
        c = fgetc(f);
        if (c == '='){
            tmp[i++] = c;
            tmp[i] = '\0';
            newToken->type = TOK_NOT_EQUAL;
        }
        else{
            tmp[i] = '\0';
            newToken->type = TOK_NOT;
            ungetc(c, f);
        }
    }
    else{
        tmp[i++] = c;
        c = fgetc(f);
        if (c == '|'){
            tmp[i++] = c;
            tmp[i] = '\0';
            newToken->type = TOK_OR;
        }
        else{
            tmp[i] = '\0';
            newToken->type = TOK_PIPE;
            ungetc(c, f); 
        }
    }
    int lexemeLen = strlen(tmp);
    newToken->lexeme.data = malloc(lexemeLen+1);
    if (newToken->lexeme.data == NULL){
        *error = ERR_INTERNAL;
        return NULL;
    }
    strcpy(newToken->lexeme.data, tmp);
    newToken->line = line;
    newToken->lexeme.len = lexemeLen;
    *error = ERR_OK;
    return newToken;
}

/**
 * @brief funkcia na skenovanie matematických operátorov
 * @param f je skenovaný súbor
 * @param firstChar je prvý znak
 * @param line je číslo riadku
 * @param error je pointer, cez ktorý si vrátim chybovú hlášku
 * @returns token s informáciami
 */
Token *scanMathExpression(FILE *f, int firstChar, int line, ErrorCode *error){
    Token *newToken = malloc(sizeof(Token));
    if (newToken == NULL){
        *error = ERR_INTERNAL;
        return NULL;
    }
    char tmp[3];
    int i = 0;
    int c = firstChar;
    if (c == '+'){
        tmp[i++] = c;
        c = fgetc(f);
        if (c == '+'){
            tmp[i++] = c;
            tmp[i] = '\0';
            newToken->type = TOK_CONCAT;
        }
        else if (c == '='){
            tmp[i++] = c;
            tmp[i] = '\0';
            newToken->type = TOK_PLUS_EQUALS;
        }
        else{
            tmp[i] = '\0';
            newToken->type = TOK_PLUS;
            ungetc(c, f);
        }
    }
    else if (c == '-'){
        tmp[i++] = c;
        c = fgetc(f);
        if (c == '-'){
            tmp[i++] = c;
            tmp[i] = '\0';
            newToken->type = TOK_REMOVE;
        }
        else if (c == '='){
            tmp[i++] = c;
            tmp[i] = '\0';
            newToken->type = TOK_MINUS_EQUALS;
        }
        else{
            tmp[i] = '\0';
            newToken->type = TOK_MINUS;
            ungetc(c, f);
        }
    }
    else if (c == '*'){
        tmp[i++] = c;
        c = fgetc(f);
        if (c == '='){
            tmp[i++] = c;
            tmp[i] = '\0';
            newToken->type = TOK_MULTIPLY_EQUALS;
        }
        else{
            tmp[i] = '\0';
            newToken->type = TOK_MULTIPLY;
            ungetc(c, f);
        }
    }
    else{
        tmp[i++] = c;
        tmp[i] = '\0';
        newToken->type = TOK_MODULO;
    }
    int lexemeLen = strlen(tmp);
    newToken->lexeme.data = malloc(lexemeLen+1);
    if (newToken->lexeme.data == NULL){
        *error = ERR_INTERNAL;
        return NULL;
    }
    strcpy(newToken->lexeme.data, tmp);
    newToken->line = line;
    newToken->lexeme.len = lexemeLen;
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
            ErrorCode error;
            Token *newToken = scanString(f, c, line, &error);
        }
        else if (c == '/'){
            ErrorCode error;
            Token *newToken = scanCommentOrDiv(f, line, &error);
        }
        else if (c == '=' || c == '<' || c == '>' || c == '!' || c == '|'){
            ErrorCode error;
            Token *newToken = scanLogicalExpression(f, c, line, &error);
        }
        else if (c == '+' || c == '-' || c == '*' || c == '%'){
            ErrorCode error;
            Token *newToken = scanMathExpression(f, c, line, &error);
        }
    }
} 
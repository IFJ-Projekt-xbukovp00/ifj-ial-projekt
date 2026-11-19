/**
 * @file lexer_dump.c
 * @brief Výpis tokenov lexera pre testy (tests/tokens)
 *
 * Použitie AI: tento súbor vygenerovala AI (Claude Opus 5.5).
 * Prompt: "vytvor nam testy pre lexer podla zadania IFJ26 pre tieto cases:
 *   - tabulator v odsadeni (aj po medzerach) -> chyba 1
 *   - neznamy znak # a ` -> chyba 1
 *   - neukonceny retazec (koniec riadku aj EOF) -> chyba 1
 *   - zle escape sekvencie \q, \x41, \u0041 -> chyba 1
 *   - neukonceny blokovy a vnoreny komentar -> chyba 1
 *   - neukonceny viacriadkovy retazec """ -> chyba 1
 *   - tokeny za viacriadkovym komentarom na tom istom riadku -> chyba 1
 *   - cele cislo s nulou na zaciatku (007) a s _ na konci (1_) -> chyba 1
 *   - exponent bez cislic (1e, 2.5e+) -> chyba 1
 *   - podciarkovniky v cisle (1_000_000), 0 a 2147483647
 *   - vsetky tvary desatinneho literalu (1.5, .5, 1e3, 2.5E-1, 1_0.0_1, 3.0d, 4e+1D)
 *   - escape sekvencie \t \\ \" \r \n
 *   - viacriadkovy retazec doslovne (aj \n a ") a prazdny """"""
 *   - riadkove, blokove a vnorene komentare, komentare pred prologom
 *   - retazec obsahujuci // a znacky blokoveho komentara
 *   - tabulator vo vnutri riadku, prazdne riadky a riadky len s bielymi znakmi
 *   - identifikatory (_a1, a_B_2, Value), literal (), strednik, @main na samostatnom riadku
 *   - tokeny: vsetky klucove slova, true/false a podobne identifikatory (define, Integer, VAL)
 *   - tokeny: vsetky operatory a oddelovace (++, <=, <-, =>, &&, ||, |, [ ], @main)
 *   - tokeny: lexemy cisel (podciarkovniky, exponent, pripona d/D)
 *   - tokeny: retazce s escape sekvenciami a viacriadkove retazce
 *   - tokeny: casti interpolovanych retazcov s"..." ($id, ${vyraz}, $$, s"""...""")
 *   - tokeny: EOL, INDENT a DEDENT pri blokoch, prazdnych riadkoch a v zatvorkach
 *   - tokeny: komentare a stredniky
 *   - tokeny: tokeny pred lexikalnou chybou a ERROR 1 (neukonceny retazec, tabulator)"
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

#include <stdio.h>
#include <string.h>

/** @brief Mená typov tokenov v poradí podľa token.h. */
static const char *const token_names[TOK_COUNT] = {
    [TOK_EOF] = "TOK_EOF",
    [TOK_EOL] = "TOK_EOL",
    [TOK_INDENT] = "TOK_INDENT",
    [TOK_DEDENT] = "TOK_DEDENT",
    [TOK_IDENTIFIER] = "TOK_IDENTIFIER",
    [TOK_INT_LIT] = "TOK_INT_LIT",
    [TOK_DOUBLE_LIT] = "TOK_DOUBLE_LIT",
    [TOK_CHAR_LIT] = "TOK_CHAR_LIT",
    [TOK_STRING_LIT] = "TOK_STRING_LIT",
    [TOK_INTERP_STRING] = "TOK_INTERP_STRING",
    [TOK_BOOL_LIT] = "TOK_BOOL_LIT",
    [TOK_UNDERSCORE] = "TOK_UNDERSCORE",
    [TOK_AT_MAIN] = "TOK_AT_MAIN",
    [TOK_KW_DEF] = "TOK_KW_DEF",
    [TOK_KW_DO] = "TOK_KW_DO",
    [TOK_KW_DOUBLE] = "TOK_KW_DOUBLE",
    [TOK_KW_ELSE] = "TOK_KW_ELSE",
    [TOK_KW_IF] = "TOK_KW_IF",
    [TOK_KW_IMPORT] = "TOK_KW_IMPORT",
    [TOK_KW_INT] = "TOK_KW_INT",
    [TOK_KW_RETURN] = "TOK_KW_RETURN",
    [TOK_KW_STRING] = "TOK_KW_STRING",
    [TOK_KW_THEN] = "TOK_KW_THEN",
    [TOK_KW_UNIT] = "TOK_KW_UNIT",
    [TOK_KW_VAL] = "TOK_KW_VAL",
    [TOK_KW_VAR] = "TOK_KW_VAR",
    [TOK_KW_WHILE] = "TOK_KW_WHILE",
    [TOK_KW_BOOLEAN] = "TOK_KW_BOOLEAN",
    [TOK_KW_CASE] = "TOK_KW_CASE",
    [TOK_KW_CLASS] = "TOK_KW_CLASS",
    [TOK_KW_EXTENSION] = "TOK_KW_EXTENSION",
    [TOK_KW_FOR] = "TOK_KW_FOR",
    [TOK_KW_LIST] = "TOK_KW_LIST",
    [TOK_KW_MATCH] = "TOK_KW_MATCH",
    [TOK_KW_TYPE] = "TOK_KW_TYPE",
    [TOK_PLUS] = "TOK_PLUS",
    [TOK_MINUS] = "TOK_MINUS",
    [TOK_MULTIPLY] = "TOK_MULTIPLY",
    [TOK_DIVIDE] = "TOK_DIVIDE",
    [TOK_MODULO] = "TOK_MODULO",
    [TOK_CONCAT] = "TOK_CONCAT",
    [TOK_REMOVE] = "TOK_REMOVE",
    [TOK_PLUS_EQUALS] = "TOK_PLUS_EQUALS",
    [TOK_MINUS_EQUALS] = "TOK_MINUS_EQUALS",
    [TOK_MULTIPLY_EQUALS] = "TOK_MULTIPLY_EQUALS",
    [TOK_DIVIDE_EQUALS] = "TOK_DIVIDE_EQUALS",
    [TOK_EQUAL] = "TOK_EQUAL",
    [TOK_NOT_EQUAL] = "TOK_NOT_EQUAL",
    [TOK_LESS] = "TOK_LESS",
    [TOK_LESS_EQUAL] = "TOK_LESS_EQUAL",
    [TOK_GREATER] = "TOK_GREATER",
    [TOK_GREATER_EQUAL] = "TOK_GREATER_EQUAL",
    [TOK_NOT] = "TOK_NOT",
    [TOK_AND] = "TOK_AND",
    [TOK_OR] = "TOK_OR",
    [TOK_ASSIGN] = "TOK_ASSIGN",
    [TOK_ARROW] = "TOK_ARROW",
    [TOK_LARROW] = "TOK_LARROW",
    [TOK_RARROW] = "TOK_RARROW",
    [TOK_LPAREN] = "TOK_LPAREN",
    [TOK_RPAREN] = "TOK_RPAREN",
    [TOK_LBRACKET] = "TOK_LBRACKET",
    [TOK_RBRACKET] = "TOK_RBRACKET",
    [TOK_COMMA] = "TOK_COMMA",
    [TOK_COLON] = "TOK_COLON",
    [TOK_SEMICOLON] = "TOK_SEMICOLON",
    [TOK_DOT] = "TOK_DOT",
    [TOK_PIPE] = "TOK_PIPE",
    [TOK_1LINE_COMMENT] = "TOK_1LINE_COMMENT",
    [TOK_MULTI_LINE_COMMENT] = "TOK_MULTI_LINE_COMMENT",
};

/** @brief Vypíše lexém v úvodzovkách, riadiace znaky ako escape sekvencie. */
static void print_lexeme(const char *s) {
    putchar('"');
    for (; s != NULL && *s != '\0'; s++) {
        unsigned char c = (unsigned char)*s;
        switch (c) {
        case '\n': fputs("\\n", stdout); break;
        case '\r': fputs("\\r", stdout); break;
        case '\t': fputs("\\t", stdout); break;
        case '\\': fputs("\\\\", stdout); break;
        case '"': fputs("\\\"", stdout); break;
        default:
            if (c < 32 || c > 126) {
                printf("\\x%02x", c);
            } else {
                putchar(c);
            }
        }
    }
    putchar('"');
}

/**
 * @brief Načíta program zo stdin a vypíše tokeny, jeden na riadok: riadok typ "lexém".
 * @return 0 pri úspechu, inak kód chyby z lexera (posledný riadok výpisu je ERROR kód).
 */
int main(void) {
    // ked pribudne token v token.h a tu nie, radsej skoncime, nez vypisat zle meno
    for (int i = 0; i < TOK_COUNT; i++) {
        if (token_names[i] == NULL) {
            fprintf(stderr, "lexer_dump: chyba meno pre token %d, doplnte token_names\n", i);
            return 99;
        }
    }

    scanner_init(stdin);
    Token token;
    memset(&token, 0, sizeof token);

    int rc = 0;
    for (;;) {
        rc = scanner_get_token(&token);
        if (rc != 0) {
            printf("ERROR %d\n", rc);
            break;
        }
        const char *name = (token.type < TOK_COUNT) ? token_names[token.type] : "TOK_?";
        printf("%d %s ", token.line, name);
        print_lexeme(token.lexeme.data);
        putchar('\n');
        if (token.type == TOK_EOF) {
            break;
        }
    }

    dynstr_free(&token.lexeme);
    scanner_free();
    return rc;
}

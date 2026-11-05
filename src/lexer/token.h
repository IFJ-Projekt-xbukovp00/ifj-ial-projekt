/**
 * @file token.h
 * @brief Definícia tokenov lexikálneho analyzátora
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

#pragma once

#include "dynstr.h"

/** @brief Typ tokenu. */
typedef enum {
    /* Riadenie */
    TOK_EOF,            /**< koniec vstupu */
    TOK_EOL,            /**< koniec logického riadku */
    TOK_INDENT,         /**< zvýšenie odsadenia */
    TOK_DEDENT,         /**< zníženie odsadenia */

    /* Identifikátory a literály */
    TOK_IDENTIFIER,     /**< identifikátor (vrátane `to`, `until`, `by`) */
    TOK_INT_LIT,        /**< celočíselný literál */
    TOK_DOUBLE_LIT,     /**< desatinný literál */
    TOK_CHAR_LIT,       /**< reťazcový literál `''` (jeden znak)*/ 
    TOK_STRING_LIT,     /**< reťazcový literál `"..."` alebo `"""..."""` */
    TOK_INTERP_STRING,  /**< interpolovaný reťazec `s"..."` (INTERPOLATEDS) */
    TOK_BOOL_LIT,       /**< `true` / `false` (BOOLTHEN) */
    TOK_UNDERSCORE,     /**< `_` – zástupný vzor (ALGEBDT) */
    TOK_AT_MAIN,        /**< `@main` – anotácia vstupnej funkcie */

    /* Kľúčové slová – základ */
    TOK_KW_DEF,         /**< `def` */
    TOK_KW_DO,          /**< `do` */
    TOK_KW_DOUBLE,      /**< `Double` */
    TOK_KW_ELSE,        /**< `else` */
    TOK_KW_IF,          /**< `if` */
    TOK_KW_IMPORT,      /**< `import` */
    TOK_KW_INT,         /**< `Int` */
    TOK_KW_RETURN,      /**< `return` */
    TOK_KW_STRING,      /**< `String` */
    TOK_KW_THEN,        /**< `then` */
    TOK_KW_UNIT,        /**< `Unit` */
    TOK_KW_VAL,         /**< `val` */
    TOK_KW_VAR,         /**< `var` */
    TOK_KW_WHILE,       /**< `while` */

    /* Kľúčové slová – rozšírenia */
    TOK_KW_BOOLEAN,     /**< `Boolean` (BOOLTHEN) */
    TOK_KW_CASE,        /**< `case` (ALGEBDT) */
    TOK_KW_CLASS,       /**< `class` (ALGEBDT) */
    TOK_KW_EXTENSION,   /**< `extension` (EXTKW) */
    TOK_KW_FOR,         /**< `for` (CONTROL) */
    TOK_KW_LIST,        /**< `List` (LISTS) */
    TOK_KW_MATCH,       /**< `match` (ALGEBDT) */
    TOK_KW_TYPE,        /**< `type` (ALGEBDT) */

    /* Aritmetické operátory */
    TOK_PLUS,           /**< `+` – sčítanie, konkatenácia, unárne plus */
    TOK_MINUS,          /**< `-` – odčítanie, unárne mínus */
    TOK_MULTIPLY,       /**< `*` – násobenie, hviezdička v prologu */
    TOK_DIVIDE,         /**< `/` – delenie */
    TOK_MODULO,         /**< `%` – modulo */
    TOK_CONCAT,         /**< `++` – spojenie zoznamov (LISTS) */
    TOK_REMOVE,         /**< `--` – odstránenie zo zoznamu (LISTS) */
    TOK_PLUS_EQUALS,    /**< `+=` – rýchle pričítanie */
    TOK_MINUS_EQUALS,   /**< `-=` – rýchle odpočítanie */
    TOK_MULTIPLY_EQUALS,    /**< `*=` – rýchle násobenie */
    TOK_DIVIDE_EQUALS,      /**< `/=` – rýchle delenie */

    /* Relačné operátory */
    TOK_EQUAL,          /**< `==` */
    TOK_NOT_EQUAL,      /**< `!=` */
    TOK_LESS,           /**< `<` */
    TOK_LESS_EQUAL,     /**< `<=` */
    TOK_GREATER,        /**< `>` */
    TOK_GREATER_EQUAL,  /**< `>=` */

    /* Logické operátory */
    TOK_NOT,            /**< `!` (BOOLTHEN) */
    TOK_AND,            /**< `&&` (BOOLTHEN) */
    TOK_OR,             /**< `||` (BOOLTHEN) */

    /* Priradenie a šípky */
    TOK_ASSIGN,         /**< `=` */
    TOK_ARROW,          /**< `=>` (ALGEBDT, LAMBDAS) */
    TOK_LARROW,         /**< `<-` (CONTROL) */
    TOK_RARROW,         /**< `->` (VYTVORENIE DVOJICE)*/

    /* Zátvorky */
    TOK_LPAREN,         /**< `(` */
    TOK_RPAREN,         /**< `)` */
    TOK_LBRACKET,       /**< `[` (LISTS) */
    TOK_RBRACKET,       /**< `]` (LISTS) */

    /* Interpunkcia */
    TOK_COMMA,          /**< `,` */
    TOK_COLON,          /**< `:` */
    TOK_SEMICOLON,      /**< `;` */
    TOK_DOT,            /**< `.` */
    TOK_PIPE,           /**< `|` (ALGEBDT) */

    /* Komentáre*/
    TOK_1LINE_COMMENT,      /*jednoriadkový koment*/
    TOK_MULTI_LINE_COMMENT,     /*viacriadkový koment*/

    TOK_COUNT           /**< počet typov tokenov, nie skutočný token */
} TokenType;

/** @brief Token vrátený lexikálnym analyzátorom. */
typedef struct {
    TokenType type; /**< Typ tokenu. */
    DynStr lexeme;  /**< Text lexému. */
    int line;       /**< Riadok vo zdrojovom súbore. */
} Token;

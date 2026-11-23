/**
 * @file parser.c
 * @brief Syntaktický analyzátor (rekurzívny zostup)
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

#include "parser.h"
#include "dynstr.h"
#include "error.h"
#include "scanner.h"
#include "token.h"

#include <stdbool.h>
#include <string.h>

#define TRY(call)                 \
    do {                          \
        int try_err = (call);     \
        if (try_err != ERR_OK) {  \
            return try_err;       \
        }                         \
    } while (0)


static Token currentToken;
static TokenType previousType;

static int advance(void) {
    previousType = currentToken.type;
    return scanner_get_token(&currentToken);
}

static bool check(TokenType type) {
    return currentToken.type == type;
}

static int expect(TokenType type) {
    if (!check(type)) {
        return ERR_SYNTAX;
    }
    return advance();
}


/* docasne len tak zatial bez napojenia na expr.c*/
static int parse_expr_stub(void) {
    int depth = 0;
    while (!check(TOK_EOF)) {
        if (check(TOK_LPAREN) || check(TOK_LBRACKET)) {
            depth++;
        } else if (check(TOK_RPAREN) || check(TOK_RBRACKET)) {
            if (depth == 0) {
                break;
            }
            depth--;
        } else if (depth == 0 && (check(TOK_EOL) || check(TOK_SEMICOLON) || check(TOK_DEDENT) ||
                                  check(TOK_KW_THEN) || check(TOK_KW_DO) || check(TOK_KW_ELSE) ||
                                  check(TOK_COMMA))) {
            break;
        }

        TRY(advance());
    }
    return ERR_OK;
}

static int parse_prolog(void);
static int parse_top_lvl_list(void);
/* 1. <program> -> <prolog> <top_lvl_list> TOK_EOF */
static int parse_program(void) {
    int err = parse_prolog();
    if (err != ERR_OK) {
        return err;
    }
    err = parse_top_lvl_list();
    if (err != ERR_OK) {
        return err;
    }
    if (!check(TOK_EOF)) {
        return ERR_SYNTAX;
    }
    return ERR_OK;
}

/* 2. <prolog> -> TOK_KW_IMPORT TOK_IDENTIFIER TOK_DOT TOK_MULTIPLY TOK_EOL */
static int parse_prolog(void) {
    int err = expect(TOK_KW_IMPORT);
    if (err != ERR_OK) {
        return err;
    }
    if (!check(TOK_IDENTIFIER) || currentToken.lexeme.data == NULL ||
        strcmp(currentToken.lexeme.data, "ifj26") != 0) {
        return ERR_SYNTAX;
    }
    if ((err = advance()) != ERR_OK)
        return err;
    if ((err = expect(TOK_DOT)) != ERR_OK)
        return err;
    if ((err = expect(TOK_MULTIPLY)) != ERR_OK)
        return err;
    if ((err = expect(TOK_EOL)) != ERR_OK)
        return err;
    return ERR_OK;
}

static int parse_top_lvl_def(void);
/* 3–4. <top_lvl_list> -> <top_lvl_def> <top_lvl_list> | eps */
static int parse_top_lvl_list(void) {
    while (check(TOK_AT_MAIN) || check(TOK_KW_DEF) || check(TOK_KW_TYPE) || check(TOK_KW_CASE) ||
           check(TOK_KW_EXTENSION)) {
        int err = parse_top_lvl_def();
        if (err != ERR_OK) {
            return err;
        }
    }
    return ERR_OK;
}

static int parse_func_def(bool isMain);
/* 5–9. <top_lvl_def> */
static int parse_top_lvl_def(void) {
    if (check(TOK_AT_MAIN)) {
        int err = advance();
        if (err != ERR_OK) {
            return err;
        }
        /* 10–11. <main_sepr> -> TOK_EOL | eps */
        if (check(TOK_EOL)) {
            err = advance();
            if (err != ERR_OK) {
                return err;
            }
        }
        return parse_func_def(true);
    }
    if (check(TOK_KW_DEF)) {
        return parse_func_def(false);
    }
    /* TODO: rozšírenia <type_alias>, <case_class>, <extens_def> */
    return ERR_SYNTAX;
}

static int parse_param_lists(void);
static int parse_ret_type_opt(void);
static int parse_func_body(void);
/* 20. <func_def> -> TOK_KW_DEF TOK_IDENTIFIER <param_lists> <ret_type_opt> TOK_ASSIGN <func_body>
 */
static int parse_func_def(bool isMain) {
    (void)isMain;
    int err = expect(TOK_KW_DEF);
    if (err != ERR_OK) {
        return err;
    }
    if ((err = expect(TOK_IDENTIFIER)) != ERR_OK)
        return err;
    if ((err = parse_param_lists()) != ERR_OK)
        return err;
    if ((err = parse_ret_type_opt()) != ERR_OK)
        return err;
    if ((err = expect(TOK_ASSIGN)) != ERR_OK)
        return err;
    /* TODO: vytvoriť DefFuncNode po napoj ast.c */
    return parse_func_body();
}

static int parse_param_list_opt(void);
/* 21–22. <param_lists> */
static int parse_param_lists(void) {
    while (check(TOK_LPAREN)) {
        int err = advance();
        if (err != ERR_OK) {
            return err;
        }
        if ((err = parse_param_list_opt()) != ERR_OK)
            return err;
        if ((err = expect(TOK_RPAREN)) != ERR_OK)
            return err;
    }
    return ERR_OK;
}

static int parse_param(void);
/* 23–28. <param_list_opt> (vrátane koncovej čiarky na novom riadku podľa §1.2) */
static int parse_param_list_opt(void) {
    if (!check(TOK_IDENTIFIER)) {
        return ERR_OK;
    }
    int err = parse_param();
    if (err != ERR_OK) {
        return err;
    }
    while (check(TOK_COMMA)) {
        int commaLine = currentToken.line;
        TRY(advance());
        if (!check(TOK_IDENTIFIER)) {
            if (!check(TOK_RPAREN) || currentToken.line <= commaLine) {
                return ERR_SYNTAX;
            }
            break;
        }
        TRY(parse_param());
    }
    return ERR_OK;
}

static int parse_type(void);
/* 29. <param> -> TOK_IDENTIFIER TOK_COLON <type> */
static int parse_param(void) {
    int err = expect(TOK_IDENTIFIER);
    if (err != ERR_OK) {
        return err;
    }
    if ((err = expect(TOK_COLON)) != ERR_OK)
        return err;
    return parse_type();
}

/* 30–31. <ret_type_opt> -> TOK_COLON <type> | eps */
static int parse_ret_type_opt(void) {
    if (!check(TOK_COLON)) {
        return ERR_OK;
    }
    int err = advance();
    if (err != ERR_OK) {
        return err;
    }
    return parse_type();
}

static int parse_prim_type(void);
/* 36–41, 49–52. <type> */
static int parse_type(void) {
    if (check(TOK_LPAREN)) {
        int err = advance();
        if (err != ERR_OK) {
            return err;
        }
        if (!check(TOK_RPAREN)) {
            if ((err = parse_type()) != ERR_OK)
                return err;
            while (check(TOK_COMMA)) {
                if ((err = advance()) != ERR_OK)
                    return err;
                if ((err = parse_type()) != ERR_OK)
                    return err;
            }
        }
        if ((err = expect(TOK_RPAREN)) != ERR_OK)
            return err;
        if ((err = expect(TOK_ARROW)) != ERR_OK)
            return err;
        return parse_type();
    }
    int err = parse_prim_type();
    if (err != ERR_OK) {
        return err;
    }
    /* 38–41. <union_tail> -> TOK_PIPE <union_member> | eps */
    if (check(TOK_PIPE)) {
        TRY(advance());
        return parse_type();
    }
    return ERR_OK;
}

/* 42–48. <prim_type> */
static int parse_prim_type(void) {
    if (check(TOK_KW_INT) || check(TOK_KW_DOUBLE) || check(TOK_KW_STRING) || check(TOK_KW_UNIT) ||
        check(TOK_KW_BOOLEAN) || check(TOK_IDENTIFIER)) {
        return advance();
    }
    if (check(TOK_KW_LIST)) {
        int err = advance();
        if (err != ERR_OK)
            return err;
        if ((err = expect(TOK_LBRACKET)) != ERR_OK)
            return err;
        if ((err = parse_type()) != ERR_OK)
            return err;
        return expect(TOK_RBRACKET);
    }
    return ERR_SYNTAX;
}

static int parse_block(void);
/* 32–35. <func_body> -> TOK_EOL <block> | <expr> <func_body_end> */
static int parse_func_body(void) {
    if (check(TOK_EOL)) {
        int err = advance();
        if (err != ERR_OK) {
            return err;
        }
        return parse_block();
    }
    int err = parse_expr_stub();
    if (err != ERR_OK) {
        return err;
    }
    /* 34–35. <func_body_end> -> TOK_EOL | eps (len po TOK_DEDENT) */
    if (check(TOK_EOL)) {
        return advance();
    }
    return previousType == TOK_DEDENT ? ERR_OK : ERR_SYNTAX;
}

static int parse_statem_list(void);
/* 53. <block> -> TOK_INDENT <statem_list> TOK_DEDENT */
static int parse_block(void) {
    int err = expect(TOK_INDENT);
    if (err != ERR_OK) {
        return err;
    }
    err = parse_statem_list();
    if (err != ERR_OK) {
        return err;
    }
    return expect(TOK_DEDENT);
}

static int parse_statem(void);
/* 54–60. <statem_list> */
static int parse_statem_list(void) {
    int err = parse_statem();
    if (err != ERR_OK) {
        return err;
    }
    for (;;) {
        if (check(TOK_EOL) || check(TOK_SEMICOLON)) {
            bool wasSemicolon = check(TOK_SEMICOLON);
            err = advance();
            if (err != ERR_OK) {
                return err;
            }
            if (wasSemicolon && check(TOK_EOL)) {
                err = advance();
                if (err != ERR_OK) {
                    return err;
                }
            }
        } else if (previousType != TOK_DEDENT && previousType != TOK_EOL) {
            break;
        }
        if (check(TOK_DEDENT) || check(TOK_EOF)) {
            break;
        }
        err = parse_statem();
        if (err != ERR_OK) {
            return err;
        }
    }
    return ERR_OK;
}

/* 75–76. <init_rhs> -> TOK_EOL <block> | <stmt_expr> */
static int parse_init_rhs(void) {
    if (check(TOK_EOL)) {
        TRY(advance());
        return parse_block();
    }
    return parse_expr_stub();
}

static int parse_var_decl(void);
static int parse_return_statem(void);
static int parse_if_statem(void);
static int parse_while_statem(void);
/* 61–69. <statem> */
static int parse_statem(void) {
    if (check(TOK_KW_VAL) || check(TOK_KW_VAR)) {
        return parse_var_decl();
    }
    if (check(TOK_KW_RETURN)) {
        return parse_return_statem();
    }
    if (check(TOK_KW_WHILE)) {
        return parse_while_statem();
    }
    if (check(TOK_KW_IF)) {
        return parse_if_statem();
    }
    if (check(TOK_IDENTIFIER)) {
        int err = advance();
        if (err != ERR_OK) {
            return err;
        }
        /* 68. priradenie id = <init_rhs> */
        if (check(TOK_ASSIGN)) {
            err = advance();
            if (err != ERR_OK) {
                return err;
            }
            return parse_init_rhs();
        }
        /* 69. pokračovanie výrazu za identif (volanie f(x) alebo a.b) */
        return parse_expr_stub();
    }
    /* 67. samostatný výraz ako príkaz */
    if (check(TOK_INT_LIT) || check(TOK_DOUBLE_LIT) || check(TOK_STRING_LIT) ||
        check(TOK_BOOL_LIT) || check(TOK_INTERP_STRING) || check(TOK_KW_LIST) ||
        check(TOK_LPAREN) || check(TOK_MINUS) || check(TOK_PLUS) || check(TOK_NOT)) {
        return parse_expr_stub();
    }
    return ERR_SYNTAX;
}

/* 70–76. <var_decl> -> (val | var) id [: type] = <init_rhs> */
static int parse_var_decl(void) {
    int err = advance(); /* TOK_KW_VAL alebo TOK_KW_VAR */
    if (err != ERR_OK) {
        return err;
    }
    if ((err = expect(TOK_IDENTIFIER)) != ERR_OK)
        return err;
    if (check(TOK_COLON)) {
        if ((err = advance()) != ERR_OK)
            return err;
        if ((err = parse_type()) != ERR_OK)
            return err;
    }
    if ((err = expect(TOK_ASSIGN)) != ERR_OK)
        return err;
    return parse_init_rhs();
}

/* 77–79. <return_statem> -> return [<stmt_expr>] */
static int parse_return_statem(void) {
    int err = advance(); /* TOK_KW_RETURN */
    if (err != ERR_OK) {
        return err;
    }
    if (check(TOK_EOL) || check(TOK_SEMICOLON) || check(TOK_DEDENT) || check(TOK_KW_ELSE) ||
        check(TOK_EOF)) {
        return ERR_OK;
    }

    return parse_expr_stub();
}

static int parse_branch_body(void);
/* 99–108. <if_statem> -> if <expr> then <if_then_body> */
static int parse_if_statem(void) {
    TRY(expect(TOK_KW_IF));
    TRY(parse_expr_stub());
    TRY(expect(TOK_KW_THEN));
    if (check(TOK_EOL)) {
        /* 100, 102–103. blokový then + voliteľné else */
        TRY(advance());
        TRY(parse_block());
        if (check(TOK_KW_ELSE)) {
            TRY(advance());
            return parse_branch_body();
        }
        return ERR_OK;
    }
    /* 101, 104–108. jednoriadkový then + else na rovnakom alebo ďalšom riadku */
    TRY(parse_statem());
    if (check(TOK_KW_ELSE)) {
        TRY(advance());
        return parse_branch_body();
    }
    if (check(TOK_EOL)) {
        TRY(advance());
        if (check(TOK_KW_ELSE)) {
            TRY(advance());
            return parse_branch_body();
        }
    }
    return ERR_OK;
}

/* 109–110. <stmt_body> -> TOK_EOL <block> | <statem> */
static int parse_branch_body(void) {
    if (check(TOK_EOL)) {
        int err = advance();
        if (err != ERR_OK) {
            return err;
        }
        return parse_block();
    }
    return parse_statem();
}

/* 126. <while_statem> -> while <expr> do <stmt_body> */
static int parse_while_statem(void) {
    int err = expect(TOK_KW_WHILE);
    if (err != ERR_OK) {
        return err;
    }
    if ((err = parse_expr_stub()) != ERR_OK)
        return err;
    if ((err = expect(TOK_KW_DO)) != ERR_OK)
        return err;
    return parse_branch_body();
}

int parser_parse(void) {
    if (!dynstr_init(&currentToken.lexeme)) {
        return ERR_INTERNAL;
    }
    int err = advance();
    if (err == ERR_OK) {
        err = parse_program();
    }
    dynstr_free(&currentToken.lexeme);
    return err;
}

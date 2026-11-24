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
#include "ast.h"
#include "expr.h"
#include "dynstr.h"
#include "error.h"

#include "scanner.h"
#include "token.h"

#include <stdbool.h>
#include <stdlib.h>
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

static bool check(TokenType type) { return currentToken.type == type; }

static int expect(TokenType type) {
    if (!check(type)) {
        return ERR_SYNTAX;
    }
    return advance();
}

/* 36–52. <type> */
static int parse_type(Type **out) {
    previousType = TOK_COUNT;
    return expr_parse_type(&currentToken, out);
}

static int parse_if_expr(ASTNode **out);
static int parse_match_block(ASTNode *subject, ASTNode **out);
/* 166–211. <expr> */
static int parse_expr(ASTNode *first, ASTNode **out) {
    if (first == NULL && check(TOK_KW_IF)) {
        TRY(parse_if_expr(out));
    } else {
        TRY(expr_parse(&currentToken, first, out));
        previousType = TOK_COUNT;
    }
    if (check(TOK_KW_MATCH)) {
        ASTNode *matchNode = NULL;
        int err = parse_match_block(*out, &matchNode);
        if (err != ERR_OK) {
            astFree(*out);
            *out = NULL;
            return err;
        }
        *out = matchNode;
    }
    return ERR_OK;
}

//3.1 Program, prológ, definície na najvyššej úrovni

static int parse_prolog(void);
static int parse_top_lvl_list(ProgramNode *prog);
/* 1. <program> -> <prolog> <top_lvl_list> TOK_EOF */
static int parse_program(ASTNode **out) {
    int err = parse_prolog();
    if (err != ERR_OK) {
        return err;
    }
    ASTNode *node = astNewProgram(1);
    if (node == NULL) {
        return ERR_INTERNAL;
    }
    err = parse_top_lvl_list((ProgramNode *)node);
    if (err != ERR_OK) {
        astFree(node);
        return err;
    }
    if (!check(TOK_EOF)) {
        astFree(node);
        return ERR_SYNTAX;
    }
    *out = node;
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

static int parse_top_lvl_def(ASTNode **out);
/* 3–4. <top_lvl_list> -> <top_lvl_def> <top_lvl_list> | eps */
static int parse_top_lvl_list(ProgramNode *prog) {
    while (check(TOK_AT_MAIN) || check(TOK_KW_DEF) || check(TOK_KW_TYPE) || check(TOK_KW_CASE) ||
            check(TOK_KW_EXTENSION)) {
        ASTNode *item = NULL;
        int err = parse_top_lvl_def(&item);
        if (err != ERR_OK) {
            return err;
        }
        if (!astAppend(&prog->items, &prog->itemsCount, item)) {
            astFree(item);
            return ERR_INTERNAL;
        }
    }
    return ERR_OK;
}

static int parse_func_def(bool isMain, ASTNode **out);
static int parse_type_alias(ASTNode **out);
static int parse_case_class(ASTNode **out);
static int parse_extens_def(ASTNode **out);
/* 5–9. <top_lvl_def> */
static int parse_top_lvl_def(ASTNode **out) {
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
        return parse_func_def(true, out);
    }
    if (check(TOK_KW_DEF)) {
        return parse_func_def(false, out);
    }
    if (check(TOK_KW_TYPE)) {
        return parse_type_alias(out);
    }
    if (check(TOK_KW_CASE)) {
        return parse_case_class(out);
    }
    if (check(TOK_KW_EXTENSION)) {
        return parse_extens_def(out);
    }
    return ERR_SYNTAX;
}


//3.2 Definícia funkcie
static int parse_param_lists(DefFuncNode *fn);
static int parse_ret_type_opt(Type **out);
static int parse_func_body(ASTNode **out);
/* 20. <func_def> -> TOK_KW_DEF TOK_IDENTIFIER <param_lists> <ret_type_opt> TOK_ASSIGN <func_body> */
static int parse_func_def(bool isMain, ASTNode **out) {
    int line = currentToken.line;
    int err = expect(TOK_KW_DEF);
    if (err != ERR_OK) {
        return err;
    }
    if (!check(TOK_IDENTIFIER)) {
        return ERR_SYNTAX;
    }
    ASTNode *node = astNewFuncDef(currentToken.lexeme.data, isMain, line);
    if (node == NULL) {
        return ERR_INTERNAL;
    }
    DefFuncNode *fn = (DefFuncNode *)node;
    err = advance();
    if (err == ERR_OK) {
        err = parse_param_lists(fn);
    }
    if (err == ERR_OK) {
        err = parse_ret_type_opt(&fn->returnType);
    }
    if (err == ERR_OK) {
        err = expect(TOK_ASSIGN);
    }
    if (err == ERR_OK) {
        err = parse_func_body(&fn->body);
    }
    if (err != ERR_OK) {
        astFree(node);
        return err;
    }
    *out = node;
    return ERR_OK;
}

static int parse_param_list_opt(ParamList *list);
/* 21–22. <param_lists> -> TOK_LPAREN <param_list_opt> TOK_RPAREN <param_lists> | eps */
static int parse_param_lists(DefFuncNode *fn) {
    while (check(TOK_LPAREN)) {
        int err = advance();
        if (err != ERR_OK) {
            return err;
        }
        ParamList *list = astFuncAddParamList(fn);
        if (list == NULL) {
            return ERR_INTERNAL;
        }
        if ((err = parse_param_list_opt(list)) != ERR_OK)
            return err;
        if ((err = expect(TOK_RPAREN)) != ERR_OK)
            return err;
    }
    return ERR_OK;
}

static int parse_param(ParamList *list);
/* 23–28. <param_list_opt> (vrátane koncovej čiarky na novom riadk) */
static int parse_param_list_opt(ParamList *list) {
    if (!check(TOK_IDENTIFIER)) {
        return ERR_OK;
    }
    int err = parse_param(list);
    if (err != ERR_OK) {
        return err;
    }
    while (check(TOK_COMMA)) {
        int commaLine = currentToken.line;
        err = advance();
        if (err != ERR_OK) {
            return err;
        }
        if (!check(TOK_IDENTIFIER)) {
            if (!check(TOK_RPAREN) || currentToken.line <= commaLine) {
                return ERR_SYNTAX;
            }
            break;
        }
        err = parse_param(list);
        if (err != ERR_OK) {
            return err;
        }
    }
    return ERR_OK;
}

/* 29. <param> -> TOK_IDENTIFIER TOK_COLON <type> */
static int parse_param(ParamList *list) {
    if (!check(TOK_IDENTIFIER)) {
        return ERR_SYNTAX;
    }
    int line = currentToken.line;
    char *name = astCopyString(currentToken.lexeme.data);
    if (name == NULL) {
        return ERR_INTERNAL;
    }
    int err = advance();
    if (err == ERR_OK) {
        err = expect(TOK_COLON);
    }
    Type *type = NULL;
    if (err == ERR_OK) {
        err = parse_type(&type);
    }
    if (err != ERR_OK) {
        free(name);
        return err;
    }
    bool ok = astParamListAdd(list, name, type, line);
    free(name);
    return ok ? ERR_OK : ERR_INTERNAL;
}

/* 30–31. <ret_type_opt> -> TOK_COLON <type> | eps */
static int parse_ret_type_opt(Type **out) {
    *out = NULL;
    if (!check(TOK_COLON)) {
        return ERR_OK;
    }
    int err = advance();
    if (err != ERR_OK) {
        return err;
    }
    return parse_type(out);
}

static int parse_block(ASTNode **out);
/* 32–35. <func_body> -> TOK_EOL <block> | <expr> <func_body_end> */
static int parse_func_body(ASTNode **out) {
    if (check(TOK_EOL)) {
        int err = advance();
        if (err != ERR_OK) {
            return err;
        }
        return parse_block(out);
    }
    ASTNode *expr = NULL;
    int err = parse_expr(NULL, &expr);
    if (err != ERR_OK) {
        return err;
    }
    /* 34–35. <func_body_end> -> TOK_EOL | eps (len po TOK_DEDENT) */
    if (check(TOK_EOL)) {
        err = advance();
        if (err != ERR_OK) {
            astFree(expr);
            return err;
        }
        *out = expr;
        return ERR_OK;
    }
    if (previousType == TOK_DEDENT) {
        *out = expr;
        return ERR_OK;
    }
    astFree(expr);
    return ERR_SYNTAX;
}

//3.4 Blok a zoznam príkazov

static int parse_statem_list(BlockNode *block);
/* 53. <block> -> TOK_INDENT <statem_list> TOK_DEDENT */
static int parse_block(ASTNode **out) {
    int line = currentToken.line;
    int err = expect(TOK_INDENT);
    if (err != ERR_OK) {
        return err;
    }
    ASTNode *node = astNewBlock(line);
    if (node == NULL) {
        return ERR_INTERNAL;
    }
    err = parse_statem_list((BlockNode *)node);
    if (err == ERR_OK) {
        err = expect(TOK_DEDENT);
    }
    if (err != ERR_OK) {
        astFree(node);
        return err;
    }
    *out = node;
    return ERR_OK;
}

static int parse_statem(ASTNode **out);
/* 54–60. <statem_list> */
static int parse_statem_list(BlockNode *block) {
    ASTNode *stmt = NULL;
    int err = parse_statem(&stmt);
    if (err != ERR_OK) {
        return err;
    }
    if (!astAppend(&block->commandOrder, &block->commandOrderCount, stmt)) {
        astFree(stmt);
        return ERR_INTERNAL;
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
        stmt = NULL;
        err = parse_statem(&stmt);
        if (err != ERR_OK) {
            return err;
        }
        if (!astAppend(&block->commandOrder, &block->commandOrderCount, stmt)) {
            astFree(stmt);
            return ERR_INTERNAL;
        }
    }
    return ERR_OK;
}


//3.5–3.6 Príkazy a základné riadiace štruktúry

/* 75–76. <init_rhs> -> TOK_EOL <block> | <stmt_expr> */
static int parse_init_rhs(ASTNode **out) {
    if (check(TOK_EOL)) {
        TRY(advance());
        return parse_block(out);
    }
    return parse_expr(NULL, out);
}

static int parse_var_decl(ASTNode **out);
static int parse_return_statem(ASTNode **out);
static int parse_if_statem(ASTNode **out);
static int parse_while_statem(ASTNode **out);
static int parse_for_statem(ASTNode **out);
/* 61–69. <statem> */
static int parse_statem(ASTNode **out) {
    if (check(TOK_KW_VAL) || check(TOK_KW_VAR)) {
        return parse_var_decl(out);
    }
    if (check(TOK_KW_RETURN)) {
        return parse_return_statem(out);
    }
    if (check(TOK_KW_WHILE)) {
        return parse_while_statem(out);
    }
    if (check(TOK_KW_FOR)) {
        return parse_for_statem(out);
    }
    if (check(TOK_KW_IF)) {
        return parse_if_statem(out);
    }
    if (check(TOK_IDENTIFIER)) {
        int line = currentToken.line;
        char *name = astCopyString(currentToken.lexeme.data);
        if (name == NULL) {
            return ERR_INTERNAL;
        }
        int err = advance();
        if (err != ERR_OK) {
            free(name);
            return err;
        }
        /* 68. priradenie id = <init_rhs> */
        if (check(TOK_ASSIGN)) {
            err = advance();
            if (err != ERR_OK) {
                free(name);
                return err;
            }
            ASTNode *rhs = NULL;
            err = parse_init_rhs(&rhs);
            if (err != ERR_OK) {
                free(name);
                return err;
            }
            *out = astNewAssign(name, rhs, line);
            free(name);
            if (*out == NULL) {
                astFree(rhs);
                return ERR_INTERNAL;
            }
            return ERR_OK;
        }
        /* 69. pokračovanie výrazu za id (volanie f(x), a.b, a + 1, samostatné id) */
        ASTNode *idNode = astNewId(name, line);
        free(name);
        if (idNode == NULL) {
            return ERR_INTERNAL;
        }
        ASTNode *expr = NULL;
        TRY(parse_expr(idNode, &expr)); /* expr_parse pri chybe idNode samo uvoľní */
        *out = astNewExprStatement(expr, line);
        if (*out == NULL) {
            astFree(expr);
            return ERR_INTERNAL;
        }
        return ERR_OK;
    }
    /* 67. samostatný výraz ako príkaz */
    if (check(TOK_INT_LIT) || check(TOK_DOUBLE_LIT) || check(TOK_STRING_LIT) ||
        check(TOK_BOOL_LIT) || check(TOK_INTERP_STRING) || check(TOK_KW_LIST) ||
        check(TOK_LPAREN) || check(TOK_MINUS) || check(TOK_PLUS) || check(TOK_NOT)) {
        int line = currentToken.line;
        ASTNode *expr = NULL;
        TRY(parse_expr(NULL, &expr));
        *out = astNewExprStatement(expr, line);
        if (*out == NULL) {
            astFree(expr);
            return ERR_INTERNAL;
        }
        return ERR_OK;
    }
    return ERR_SYNTAX;
}

/* 70–76. <var_decl> -> (val | var) id [: type] = <init_rhs> */
static int parse_var_decl(ASTNode **out) {
    int line = currentToken.line;
    bool isMutable = check(TOK_KW_VAR);
    int err = advance(); /* TOK_KW_VAL alebo TOK_KW_VAR */
    if (err != ERR_OK) {
        return err;
    }
    if (!check(TOK_IDENTIFIER)) {
        return ERR_SYNTAX;
    }
    char *name = astCopyString(currentToken.lexeme.data);
    if (name == NULL) {
        return ERR_INTERNAL;
    }
    err = advance();
    Type *declType = NULL;
    if (err == ERR_OK && check(TOK_COLON)) {
        err = advance();
        if (err == ERR_OK) {
            err = parse_type(&declType);
        }
    }
    if (err == ERR_OK) {
        err = expect(TOK_ASSIGN);
    }
    ASTNode *init = NULL;
    if (err == ERR_OK) {
        err = parse_init_rhs(&init);
    }
    if (err != ERR_OK) {
        free(name);
        astFree(init);
        return err;
    }
    *out = astNewVarDef(name, isMutable, declType, init, line);
    free(name);
    if (*out == NULL) {
        astFree(init);
        return ERR_INTERNAL;
    }
    return ERR_OK;
}

/* 77–79. <return_statem> -> return [<stmt_expr>] */
static int parse_return_statem(ASTNode **out) {
    int line = currentToken.line;
    int err = advance(); /* TOK_KW_RETURN */
    if (err != ERR_OK) {
        return err;
    }
    ASTNode *val = NULL;
    if (!check(TOK_EOL) && !check(TOK_SEMICOLON) && !check(TOK_DEDENT) && !check(TOK_KW_ELSE) &&
        !check(TOK_EOF)) {
        err = parse_expr(NULL, &val);
        if (err != ERR_OK) {
            return err;
        }
    }
    *out = astNewReturn(val, line);
    if (*out == NULL) {
        astFree(val);
        return ERR_INTERNAL;
    }
    return ERR_OK;
}

static int parse_branch_body(ASTNode **out);
/* 99–108. <if_statem> -> if <expr> then <if_then_body> */
static int parse_if_statem(ASTNode **out) {
    int line = currentToken.line;
    TRY(expect(TOK_KW_IF));
    ASTNode *cond = NULL;
    ASTNode *thenB = NULL;
    ASTNode *elseB = NULL;
    TRY(parse_expr(NULL, &cond));
    int err = expect(TOK_KW_THEN);
    if (err == ERR_OK) {
        if (check(TOK_EOL)) {
            /* 100, 102–103. blokový then + voliteľné else */
            err = advance();
            if (err == ERR_OK) {
                err = parse_block(&thenB);
            }
            if (err == ERR_OK && check(TOK_KW_ELSE)) {
                err = advance();
                if (err == ERR_OK) {
                    err = parse_branch_body(&elseB);
                }
            }
        } else {
            /* 101, 104–108. jednoriadkový then + else na rovnakom alebo ďalšom riadku */
            err = parse_statem(&thenB);
            if (err == ERR_OK && check(TOK_KW_ELSE)) {
                err = advance();
                if (err == ERR_OK) {
                    err = parse_branch_body(&elseB);
                }
            } else if (err == ERR_OK && check(TOK_EOL)) {
                err = advance();
                if (err == ERR_OK && check(TOK_KW_ELSE)) {
                    err = advance();
                    if (err == ERR_OK) {
                        err = parse_branch_body(&elseB);
                    }
                }
            }
        }
    }
    if (err != ERR_OK) {
        astFree(cond);
        astFree(thenB);
        astFree(elseB);
        return err;
    }
    *out = astNewIf(cond, thenB, elseB, line);
    if (*out == NULL) {
        astFree(cond);
        astFree(thenB);
        astFree(elseB);
        return ERR_INTERNAL;
    }
    return ERR_OK;
}

/* 109–110. <stmt_body> -> TOK_EOL <block> | <statem> */
static int parse_branch_body(ASTNode **out) {
    if (check(TOK_EOL)) {
        int err = advance();
        if (err != ERR_OK) {
            return err;
        }
        return parse_block(out);
    }
    return parse_statem(out);
}

/* 126. <while_statem> -> while <expr> do <stmt_body> */
static int parse_while_statem(ASTNode **out) {
    int line = currentToken.line;
    int err = expect(TOK_KW_WHILE);
    if (err != ERR_OK) {
        return err;
    }
    ASTNode *cond = NULL;
    ASTNode *body = NULL;
    if ((err = parse_expr(NULL, &cond)) != ERR_OK)
        return err;
    err = expect(TOK_KW_DO);
    if (err == ERR_OK) {
        err = parse_branch_body(&body);
    }
    if (err != ERR_OK) {
        astFree(cond);
        astFree(body);
        return err;
    }
    *out = astNewWhile(cond, body, line);
    if (*out == NULL) {
        astFree(cond);
        astFree(body);
        return ERR_INTERNAL;
    }
    return ERR_OK;
}

//vystup

int parser_parse(ASTNode **program) {
    if (program == NULL) {
        return ERR_INTERNAL;
    }
    *program = NULL;
    if (!dynstr_init(&currentToken.lexeme)) {
        return ERR_INTERNAL;
    }
    previousType = TOK_COUNT;
    int err = advance();
    if (err == ERR_OK) {
        err = parse_program(program);
    }
    if (err != ERR_OK) {
        astFree(*program);
        *program = NULL;
    }
    dynstr_free(&currentToken.lexeme);
    return err;
}

//12. <type_alias> -> TOK_KW_TYPE TOK_IDENTIFIER TOK_ASSIGN <type> TOK_EOL
static int parse_type_alias(ASTNode **out) {
    int line = currentToken.line;

    TRY(expect(TOK_KW_TYPE));

    if (!check(TOK_IDENTIFIER)){
        return ERR_SYNTAX;
    }
    char *name = astCopyString(currentToken.lexeme.data);
    if (name == NULL) {
        return ERR_INTERNAL;
    }
    int err = advance();

    if (err == ERR_OK) {
        err = expect(TOK_ASSIGN);
    }

    Type *type = NULL;

    if (err == ERR_OK) {
        err = parse_type(&type);
    }

    if (err == ERR_OK){
        err = expect(TOK_EOL);
    }

    if (err != ERR_OK) {
        free(name);
        return err;
    }

    *out = astNewTypeAlias(name, type, line);
    free(name);
    if (*out == NULL){
        return ERR_INTERNAL;
    }
    return ERR_OK;
}

//13. <case_class> -> TOK_KW_CASE TOK_KW_CLASS TOK_IDENTIFIER TOK_LPAREN <param_list_opt> TOK_RPAREN TOK_EOL
static int parse_case_class(ASTNode **out) {
    int line = currentToken.line;

    TRY(expect(TOK_KW_CASE));
    TRY(expect(TOK_KW_CLASS));

    if (!check(TOK_IDENTIFIER)) {
        return ERR_SYNTAX;
    }

    ASTNode *node = astNewClassDef(currentToken.lexeme.data, line);
    if (node == NULL) {
        return ERR_INTERNAL;
    }

    int err = advance();

    if (err == ERR_OK) {
        err = expect(TOK_LPAREN);
    }

    if (err == ERR_OK) {
        err = parse_param_list_opt(&((ClassDefNode *)node)->fields);
    }

    if (err == ERR_OK) {
        err = expect(TOK_RPAREN);
    }

    if (err == ERR_OK) {
        err = expect(TOK_EOL);
    }

    if (err != ERR_OK) {
        astFree(node);
        return err;
    }

    *out = node;
    return ERR_OK;
}

/*
 * 14. <extens_def>      -> TOK_KW_EXTENSION TOK_LPAREN <param> TOK_RPAREN <ext_body>
 * 15. <ext_body>        -> TOK_EOL TOK_INDENT <ext_method_list> TOK_DEDENT
 * 16. <ext_body>        -> <func_def>
 * 17. <ext_method_list> -> <func_def> <ext_method_tail>
 * 18. <ext_method_tail> -> <func_def> <ext_method_tail>
 * 19. <ext_method_tail> -> eps
 */
static int parse_extens_def(ASTNode **out) {
    int line = currentToken.line;

    TRY(expect(TOK_KW_EXTENSION));
    TRY(expect(TOK_LPAREN));

    if (!check(TOK_IDENTIFIER)) {
        return ERR_SYNTAX;
    }
    int recvLine = currentToken.line;
    char *recvName = astCopyString(currentToken.lexeme.data);
    if (recvName == NULL) {
        return ERR_INTERNAL;
    }

    int err = advance();
    if (err == ERR_OK) {
        err = expect(TOK_COLON);
    }

    Type *recvType = NULL;
    if (err == ERR_OK) {
        err = parse_type(&recvType);
    }

    if (err == ERR_OK) {
        err = expect(TOK_RPAREN);
    }

    if (err != ERR_OK) {
        free(recvName);
        return err;
    }

    ASTNode *node = astNewExtension(recvName, recvType, line);
    free(recvName);
    if (node == NULL) {
        return ERR_INTERNAL;
    }
    ExtensionNode *ext = (ExtensionNode *)node;
    ext->receiver.line = recvLine;

    if (check(TOK_EOL)) {
        err = advance();
        if (err == ERR_OK) {
            err = expect(TOK_INDENT);
        }
        if (err == ERR_OK) {
            do {
                ASTNode *method = NULL;
                err = parse_func_def(false, &method);

                if (err != ERR_OK) {
                    break;
                }

                if (!astAppend(&ext->methods, &ext->methodsCount, method)) {
                    astFree(method);
                    err = ERR_INTERNAL;
                    break;
                }
            } while (check(TOK_KW_DEF));
        }
        if (err == ERR_OK) {
            err = expect(TOK_DEDENT);
        }
    } else {
        ASTNode *method = NULL;
        err = parse_func_def(false, &method);
        if (err == ERR_OK && !astAppend(&ext->methods, &ext->methodsCount, method)) {
            astFree(method);
            err = ERR_INTERNAL;
        }
    }

    if (err != ERR_OK) {
        astFree(node);
        return err;
    }

    *out = node;
    return ERR_OK;
}

/*
 * 121. <if_expr>        -> TOK_KW_IF <expr> TOK_KW_THEN <expr_then_body> TOK_KW_ELSE <expr_else_body>
 * 122. <expr_then_body> -> TOK_EOL <block>
 * 123. <expr_then_body> -> <expr> <opt_eol>
 * 124. <expr_else_body> -> TOK_EOL <block>
 * 125. <expr_else_body> -> <expr>
 */
static int parse_if_expr(ASTNode **out) {
    int line = currentToken.line;

    TRY(expect(TOK_KW_IF));

    ASTNode *cond = NULL;
    ASTNode *thenB = NULL;
    ASTNode *elseB = NULL;

    TRY(parse_expr(NULL, &cond));

    int err = expect(TOK_KW_THEN);
    if (err == ERR_OK) {
        if (check(TOK_EOL)) {
            err = advance();

            if (err == ERR_OK) {
                err = parse_block(&thenB);
            }

        } else {
            err = parse_expr(NULL, &thenB);
            if (err == ERR_OK && check(TOK_EOL)) {
                err = advance();
            }
        }
    }

    if (err == ERR_OK) {
        err = expect(TOK_KW_ELSE);
    }

    if (err == ERR_OK) {
        if (check(TOK_EOL)) {
            err = advance();
            if (err == ERR_OK) {
                err = parse_block(&elseB);
            }
        } else {
            err = parse_expr(NULL, &elseB);
        }
    }

    if (err != ERR_OK) {
        astFree(cond);
        astFree(thenB);
        astFree(elseB);
        return err;
    }

    *out = astNewIf(cond, thenB, elseB, line);
    if (*out == NULL) {
        astFree(cond);
        astFree(thenB);
        astFree(elseB);
        return ERR_INTERNAL;
    }
    return ERR_OK;
}

/*
 * 127. <for_statem>       -> TOK_KW_FOR <for_header> TOK_KW_DO <stmt_body>
 * 128. <for_header>       -> TOK_EOL TOK_INDENT <for_gen> <for_guards_ml> TOK_DEDENT
 * 129. <for_header>       -> <for_gen> <for_guard_sl>
 * 130. <for_gen>          -> TOK_IDENTIFIER TOK_LARROW <for_source>  (zdroj: parse_expr(NULL, &src))
 * 136–137. <for_guard_sl> -> TOK_KW_IF <expr> <for_guard_sl> | eps
 * 138–141. <for_guards_ml>-> TOK_EOL [TOK_KW_IF <expr> <for_guards_ml>] | eps
 */
static int parse_for_statem(ASTNode **out) {
    int line = currentToken.line;

    TRY(expect(TOK_KW_FOR));

    bool isMultiline = check(TOK_EOL);
    if (isMultiline) {
        TRY(advance());
        TRY(expect(TOK_INDENT));
    }

    if (!check(TOK_IDENTIFIER)) {
        return ERR_SYNTAX;
    }
    char *varName = astCopyString(currentToken.lexeme.data);
    if (varName == NULL) {
        return ERR_INTERNAL;
    }

    int err = advance();
    if (err == ERR_OK) {
        err = expect(TOK_LARROW);
    }

    ASTNode *source = NULL;
    if (err == ERR_OK) {
        err = parse_expr(NULL, &source);
    }

    if (err != ERR_OK) {
        free(varName);
        astFree(source);
        return err;
    }

    ASTNode *node = astNewFor(varName, source, NULL, line);
    free(varName);
    if (node == NULL) {
        astFree(source);
        return ERR_INTERNAL;
    }
    ForNode *forNode = (ForNode *)node;

    if (isMultiline) {
        while (err == ERR_OK && check(TOK_EOL)) {
            err = advance();
            if (err != ERR_OK || !check(TOK_KW_IF)) {
                break;
            }
            err = advance();
            if (err != ERR_OK) {
                break;
            }
            ASTNode *guard = NULL;
            err = parse_expr(NULL, &guard);
            if (err != ERR_OK) {
                break;
            }
            if (!astAppend(&forNode->guards, &forNode->guardsCount, guard)) {
                astFree(guard);
                err = ERR_INTERNAL;
                break;
            }
        }
        if (err == ERR_OK) {
            err = expect(TOK_DEDENT);
        }
    } else {
        while (err == ERR_OK && check(TOK_KW_IF)) {
            err = advance();
            if (err != ERR_OK) {
                break;
            }
            ASTNode *guard = NULL;
            err = parse_expr(NULL, &guard);
            if (err != ERR_OK) {
                break;
            }
            if (!astAppend(&forNode->guards, &forNode->guardsCount, guard)) {
                astFree(guard);
                err = ERR_INTERNAL;
                break;
            }
        }
    }

    if (err == ERR_OK) {
        err = expect(TOK_KW_DO);
    }
    if (err == ERR_OK) {
        err = parse_branch_body(&forNode->body);
    }

    if (err != ERR_OK) {
        astFree(node);
        return err;
    }

    *out = node;
    return ERR_OK;
}

static char *copy_num_lexeme(const char *src, size_t len, bool negative) {

    char *buf = malloc(len + (negative ? 2 : 1));

    if (buf == NULL) {
        return NULL;
    }

    size_t j = 0;
    if (negative) {
        buf[j++] = '-';
    }

    for (size_t i = 0; i < len; i++) {
        if (src[i] != '_') {
            buf[j++] = src[i];
        }
    }

    buf[j] = '\0';
    return buf;
}

static char decode_escape_char(char c) {
    switch (c) {
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case '\\':
        return '\\';
    case '"':
        return '"';
    default:
        return c;
    }
}

//154–156. literálový vzor
static int parse_pattern_literal(ASTNode **out) {
    int line = currentToken.line;

    if (check(TOK_LPAREN)) {
        TRY(advance());
        TRY(expect(TOK_RPAREN));

        *out = astNewUnitLit(line);
        return *out != NULL ? ERR_OK : ERR_INTERNAL;
    }

    bool negative = false;
    if (check(TOK_MINUS)) {
        negative = true;
        TRY(advance());

        if (!check(TOK_INT_LIT) && !check(TOK_DOUBLE_LIT)) {
            return ERR_SYNTAX;
        }
    }

    ASTNode *lit;
    if (check(TOK_INT_LIT)) {
        char *clean = copy_num_lexeme(currentToken.lexeme.data, currentToken.lexeme.len, negative);
        if (clean == NULL) {
            return ERR_INTERNAL;
        }
        long long val = strtoll(clean, NULL, 10);
        free(clean);
        lit = astNewIntLit(val, line);

    } else if (check(TOK_DOUBLE_LIT)) {
        char *clean = copy_num_lexeme(currentToken.lexeme.data, currentToken.lexeme.len, negative);
        if (clean == NULL) {
            return ERR_INTERNAL;
        }

        double val = strtod(clean, NULL);
        free(clean);
        lit = astNewDoubleLit(val, line);

    } else if (check(TOK_BOOL_LIT)) {
        lit = astNewBoolLit(strcmp(currentToken.lexeme.data, "true") == 0, line);
    } else if (check(TOK_STRING_LIT)) {
        const char *lexeme = currentToken.lexeme.data;
        size_t len = currentToken.lexeme.len;
        size_t quotes = (len >= 6 && strncmp(lexeme, "\"\"\"", 3) == 0) ? 3 : 1;

        if (len < 2 * quotes) {
            return ERR_SYNTAX;
        }

        size_t innerLen = len - 2 * quotes;
        char *buf = malloc(innerLen + 1);

        if (buf == NULL) {
            return ERR_INTERNAL;
        }

        if (quotes == 3) {
            memcpy(buf, lexeme + 3, innerLen);
            buf[innerLen] = '\0';
        } else {
            size_t j = 0;
            for (size_t i = 0; i < innerLen; i++) {
                if (lexeme[1 + i] == '\\' && i + 1 < innerLen) {
                    i++;
                    buf[j++] = decode_escape_char(lexeme[1 + i]);
                } else {
                    buf[j++] = lexeme[1 + i];
                }
            }
            buf[j] = '\0';
        }
        lit = astNewStringLit(buf, line);
        free(buf);
    } else {
        return ERR_SYNTAX;
    }

    if (lit == NULL) {
        return ERR_INTERNAL;
    }

    int err = advance();
    if (err != ERR_OK) {
        astFree(lit);
        return err;
    }

    *out = lit;
    return ERR_OK;
}

/* 153–165. <pattern> */
static int parse_pattern(int caseLine, ASTNode **out) {
    if (check(TOK_UNDERSCORE)) {
        TRY(advance());
        *out = astNewCase(PAT_WILDCARD, caseLine);
        return *out != NULL ? ERR_OK : ERR_INTERNAL;
    }

    if (check(TOK_IDENTIFIER)) {
        int idLine = currentToken.line;
        char *idName = astCopyString(currentToken.lexeme.data);
        if (idName == NULL) {
            return ERR_INTERNAL;
        }

        int err = advance();
        if (err != ERR_OK) {
            free(idName);
            return err;
        }

        //158. typovaný vzor: id : <type>
        if (check(TOK_COLON)) {
            err = advance();
            Type *type = NULL;
            if (err == ERR_OK) {
                err = parse_type(&type);
            }

            if (err != ERR_OK) {
                free(idName);
                return err;
            }

            ASTNode *node = astNewCase(PAT_TYPED, caseLine);
            if (node == NULL) {
                free(idName);
                return ERR_INTERNAL;
            }

            CaseNode *c = (CaseNode *)node;
            c->binder.name = idName;
            c->binder.type = type;
            c->binder.line = idLine;

            *out = node;
            return ERR_OK;
        }

        //159–165. konštruktorový vzor: ClassName(<ctor_binders_opt>)
        if (check(TOK_LPAREN)) {
            err = advance();
            if (err != ERR_OK) {
                free(idName);
                return err;
            }

            ASTNode *node = astNewCase(PAT_CONSTRUCTOR, caseLine);
            if (node == NULL) {
                free(idName);
                return ERR_INTERNAL;
            }

            CaseNode *c = (CaseNode *)node;
            c->className = idName;

            if (check(TOK_IDENTIFIER) || check(TOK_UNDERSCORE)) {
                const char *bname = check(TOK_UNDERSCORE) ? "_" : currentToken.lexeme.data;
                if (!astParamListAdd(&c->ctorBinders, bname, NULL, currentToken.line)) {
                    astFree(node);
                    return ERR_INTERNAL;
                }

                err = advance();
                while (err == ERR_OK && check(TOK_COMMA)) {
                    err = advance();
                    if (err != ERR_OK) {
                        break;
                    }

                    if (!check(TOK_IDENTIFIER) && !check(TOK_UNDERSCORE)) {
                        err = ERR_SYNTAX;
                        break;
                    }

                    bname = check(TOK_UNDERSCORE) ? "_" : currentToken.lexeme.data;
                    if (!astParamListAdd(&c->ctorBinders, bname, NULL, currentToken.line)) {
                        err = ERR_INTERNAL;
                        break;
                    }

                    err = advance();
                }
            }

            if (err == ERR_OK) {
                err = expect(TOK_RPAREN);
            }

            if (err != ERR_OK) {
                astFree(node);
                return err;
            }

            *out = node;
            return ERR_OK;
        }

        free(idName);
        return ERR_SYNTAX;
    }

    //154–156. literálový vzor
    ASTNode *lit = NULL;
    TRY(parse_pattern_literal(&lit));

    ASTNode *node = astNewCase(PAT_LITERAL, caseLine);
    if (node == NULL) {
        astFree(lit);
        return ERR_INTERNAL;
    }

    ((CaseNode *)node)->literal = lit;
    *out = node;
    return ERR_OK;
}

//146–152. <case_branch> -> TOK_KW_CASE <pattern> <guard_opt> TOK_ARROW <case_body>
static int parse_case_branch(ASTNode **out) {
    int line = currentToken.line;

    TRY(expect(TOK_KW_CASE));

    ASTNode *node = NULL;
    TRY(parse_pattern(line, &node));
    CaseNode *c = (CaseNode *)node;

    int err = ERR_OK;
    if (check(TOK_KW_IF)) {
        err = advance();
        if (err == ERR_OK) {
            err = parse_expr(NULL, &c->guard);
        }
    }

    if (err == ERR_OK) {
        err = expect(TOK_ARROW);
    }

    if (err == ERR_OK) {
        if (check(TOK_EOL)) {
            err = advance();

            if (err == ERR_OK) {
                err = parse_block(&c->body);
            }

        } else {
            err = parse_expr(NULL, &c->body);

            if (err == ERR_OK) {
                if (check(TOK_EOL)) {
                    err = advance();
                } else if (previousType != TOK_DEDENT) {
                    err = ERR_SYNTAX;
                }
            }
        }
    }

    if (err != ERR_OK) {
        astFree(node);
        return err;
    }

    *out = node;
    return ERR_OK;
}

/*
 * 142. <match_block>       -> TOK_KW_MATCH TOK_EOL TOK_INDENT <case_list> TOK_DEDENT
 * 143–145. <case_list>     -> <case_branch> (<case_branch>)*
 */
static int parse_match_block(ASTNode *subject, ASTNode **out) {
    int line = currentToken.line;

    TRY(expect(TOK_KW_MATCH));
    TRY(expect(TOK_EOL));
    TRY(expect(TOK_INDENT));

    ASTNode *node = astNewMatch(NULL, line);
    if (node == NULL) {
        return ERR_INTERNAL;
    }
    MatchNode *match = (MatchNode *)node;

    int err;
    do {
        ASTNode *branch = NULL;
        err = parse_case_branch(&branch);
        if (err != ERR_OK) {
            break;
        }
        if (!astAppend(&match->cases, &match->casesCount, branch)) {
            astFree(branch);
            err = ERR_INTERNAL;
            break;
        }
    } while (check(TOK_KW_CASE));

    if (err == ERR_OK) {
        err = expect(TOK_DEDENT);
    }

    if (err != ERR_OK) {
        astFree(node);
        return err;
    }

    match->subject = subject;
    *out = node;
    return ERR_OK;
}

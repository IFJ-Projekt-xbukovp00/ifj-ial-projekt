/**
 * @file dfa.c
 * @brief Konečný automat lexikálneho analyzátora – prechodová funkcia a maximal munch
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

// clang-format off

#include "dfa.h"

#include "error.h"

/** @brief Znak patrí do vstupnej abecedy (ASCII: \t, \n a 32–126). */
#define DFA_IN_SIGMA(c) ((c) == '\t' || (c) == '\n' || ((c) >= 32 && (c) <= 126))

/** @brief Je stav koncový (akceptujúci)? */
static const bool dfa_final[DFA_COUNT] = {
    [DFA_T] = false,
    [DFA_L] = true,
    [DFA_EOFT] = true,
    [DFA_LPAR] = true,
    [DFA_LBR] = true,
    [DFA_RPAR] = true,
    [DFA_RBR] = true,
    [DFA_ID] = true,
    [DFA_IDS] = true,
    [DFA_U] = true,
    [DFA_AT] = false,
    [DFA_AT1] = false,
    [DFA_AT2] = false,
    [DFA_AT3] = false,
    [DFA_MAIN] = true,
    [DFA_ERR_ID] = false,
    [DFA_Z] = true,
    [DFA_INT] = true,
    [DFA_INT_] = false,
    [DFA_IDOT] = false,
    [DFA_DOT] = true,
    [DFA_F] = true,
    [DFA_F_] = false,
    [DFA_E] = false,
    [DFA_ES] = false,
    [DFA_X] = true,
    [DFA_X_] = false,
    [DFA_DS] = true,
    [DFA_ERR_N0] = false,
    [DFA_ERR_NI] = false,
    [DFA_ERR_NF] = false,
    [DFA_ERR_NX] = false,
    [DFA_ERR_ND] = false,
    [DFA_Q1] = false,
    [DFA_Q2] = true,
    [DFA_B] = false,
    [DFA_ESC] = false,
    [DFA_STR] = true,
    [DFA_ML] = false,
    [DFA_M1] = false,
    [DFA_M2] = false,
    [DFA_M3] = true,
    [DFA_S] = true,
    [DFA_LC] = true,
    [DFA_BC] = false,
    [DFA_BS] = false,
    [DFA_BST] = false,
    [DFA_END] = true,
    [DFA_EQ] = true,
    [DFA_EQEQ] = true,
    [DFA_ARR] = true,
    [DFA_LT] = true,
    [DFA_LE] = true,
    [DFA_LAR] = true,
    [DFA_GT] = true,
    [DFA_GE] = true,
    [DFA_NOT] = true,
    [DFA_NE] = true,
    [DFA_PIPE] = true,
    [DFA_OR] = true,
    [DFA_PLUS] = true,
    [DFA_CONC] = true,
    [DFA_AMP] = false,
    [DFA_AND] = true,
    [DFA_SGL] = true,
    [DFA_S1] = false,
    [DFA_S2] = true,
    [DFA_IB] = false,
    [DFA_IESC] = false,
    [DFA_D] = false,
    [DFA_IID] = false,
    [DFA_IEND] = true,
    [DFA_EXS] = true,
    [DFA_IM] = false,
    [DFA_IM1] = false,
    [DFA_IM2] = false,
    [DFA_IMEND] = true,
    [DFA_IMD] = false,
    [DFA_IMID] = false,
    [DFA_IMEXS] = true,
};

/** @brief Smie sa pri zaseknutí v stave vrátiť k poslednému koncovému stavu? */
static const bool dfa_backtrack[DFA_COUNT] = {
    [DFA_IDOT] = true,
};

const char *dfa_state_name(DfaState s) {
    switch (s) {
    case DFA_T: return "T";
    case DFA_L: return "\\n";
    case DFA_EOFT: return "EOF";
    case DFA_LPAR: return "(";
    case DFA_LBR: return "[";
    case DFA_RPAR: return ")";
    case DFA_RBR: return "]";
    case DFA_ID: return "ID";
    case DFA_IDS: return "s";
    case DFA_U: return "_";
    case DFA_AT: return "@";
    case DFA_AT1: return "@m";
    case DFA_AT2: return "@ma";
    case DFA_AT3: return "@mai";
    case DFA_MAIN: return "@main";
    case DFA_ERR_ID: return "ERR_ID";
    case DFA_Z: return "0";
    case DFA_INT: return "INT";
    case DFA_INT_: return "INT_";
    case DFA_IDOT: return "INT.";
    case DFA_DOT: return ".";
    case DFA_F: return "DBL₁";
    case DFA_F_: return "DBL₁_";
    case DFA_E: return "e";
    case DFA_ES: return "±";
    case DFA_X: return "DBL₃";
    case DFA_X_: return "DBL₃_";
    case DFA_DS: return "DBL₂";
    case DFA_ERR_N0: return "ERR 0";
    case DFA_ERR_NI: return "ERR INT";
    case DFA_ERR_NF: return "ERR DBL₁";
    case DFA_ERR_NX: return "ERR DBL₃";
    case DFA_ERR_ND: return "ERR DBL₂";
    case DFA_Q1: return "\"";
    case DFA_Q2: return "\"\"";
    case DFA_B: return "B";
    case DFA_ESC: return "\\ [ESC]";
    case DFA_STR: return "STR";
    case DFA_ML: return "ML";
    case DFA_M1: return "ML\"";
    case DFA_M2: return "ML\"\"";
    case DFA_M3: return "\"\"\"";
    case DFA_S: return "/";
    case DFA_LC: return "//";
    case DFA_BC: return "/*";
    case DFA_BS: return "/*/";
    case DFA_BST: return "/**";
    case DFA_END: return "*/";
    case DFA_EQ: return "=";
    case DFA_EQEQ: return "==";
    case DFA_ARR: return "=>";
    case DFA_LT: return "<";
    case DFA_LE: return "<=";
    case DFA_LAR: return "<-";
    case DFA_GT: return ">";
    case DFA_GE: return ">=";
    case DFA_NOT: return "!";
    case DFA_NE: return "!=";
    case DFA_PIPE: return "|";
    case DFA_OR: return "||";
    case DFA_PLUS: return "+";
    case DFA_CONC: return "++";
    case DFA_AMP: return "&";
    case DFA_AND: return "&&";
    case DFA_SGL: return "- * : , ;";
    case DFA_S1: return "s\"";
    case DFA_S2: return "s\"\"";
    case DFA_IB: return "IB";
    case DFA_IESC: return "\\ [IESC]";
    case DFA_D: return "$";
    case DFA_IID: return "$id";
    case DFA_IEND: return "s\"…\"";
    case DFA_EXS: return "${ [s\"]";
    case DFA_IM: return "IM";
    case DFA_IM1: return "IM\"";
    case DFA_IM2: return "IM\"\"";
    case DFA_IMEND: return "s\"\"\"…\"\"\"";
    case DFA_IMD: return "$ [IM]";
    case DFA_IMID: return "$id [IM]";
    case DFA_IMEXS: return "${ [s\"\"\"]";
    default: return "?";
    }
}

TokenType dfa_token(DfaState s, const DynStr *lexeme) {
    switch (s) {
    case DFA_L: return TOK_EOL;          // \n
    case DFA_EOFT: return TOK_EOF;       // EOF
    case DFA_LPAR: return TOK_LPAREN;    // (
    case DFA_LBR: return TOK_LBRACKET;   // [
    case DFA_RPAR: return TOK_RPAREN;    // )
    case DFA_RBR: return TOK_RBRACKET;   // ]
    case DFA_ID: return TOK_IDENTIFIER;  // ID
    case DFA_IDS: return TOK_IDENTIFIER; // s
    case DFA_U: return TOK_UNDERSCORE;   // _
    case DFA_MAIN: return TOK_AT_MAIN;   // @main
    case DFA_Z: return TOK_INT_LIT;      // 0
    case DFA_INT: return TOK_INT_LIT;    // INT
    case DFA_DOT: return TOK_DOT;        // .
    case DFA_F: return TOK_DOUBLE_LIT;   // DBL₁
    case DFA_X: return TOK_DOUBLE_LIT;   // DBL₃
    case DFA_DS: return TOK_DOUBLE_LIT;  // DBL₂
    case DFA_Q2: return TOK_STRING_LIT;  // ""
    case DFA_STR: return TOK_STRING_LIT; // STR
    case DFA_M3: return TOK_STRING_LIT;  // """
    case DFA_S: return TOK_DIVIDE;       // /
    case DFA_LC: return TOK_1LINE_COMMENT; // //
    case DFA_END: return TOK_MULTI_LINE_COMMENT; // * /
    case DFA_EQ: return TOK_ASSIGN;      // =
    case DFA_EQEQ: return TOK_EQUAL;     // ==
    case DFA_ARR: return TOK_ARROW;      // =>
    case DFA_LT: return TOK_LESS;        // <
    case DFA_LE: return TOK_LESS_EQUAL;  // <=
    case DFA_LAR: return TOK_LARROW;     // <-
    case DFA_GT: return TOK_GREATER;     // >
    case DFA_GE: return TOK_GREATER_EQUAL; // >=
    case DFA_NOT: return TOK_NOT;        // !
    case DFA_NE: return TOK_NOT_EQUAL;   // !=
    case DFA_PIPE: return TOK_PIPE;      // |
    case DFA_OR: return TOK_OR;          // ||
    case DFA_PLUS: return TOK_PLUS;      // +
    case DFA_CONC: return TOK_CONCAT;    // ++
    case DFA_AND: return TOK_AND;        // &&
    case DFA_SGL: // - * : , ;
        switch (lexeme->len ? lexeme->data[0] : 0) {
        case '-': return TOK_MINUS;
        case '*': return TOK_MULTIPLY;
        case ':': return TOK_COLON;
        case ',': return TOK_COMMA;
        case ';': return TOK_SEMICOLON;
        default: break;
        }
        break;
    case DFA_S2: return TOK_INTERP_STRING; // s""
    case DFA_IEND: return TOK_INTERP_STRING; // s"…"
    case DFA_EXS: return TOK_INTERP_STRING; // ${ [s"]
    case DFA_IMEND: return TOK_INTERP_STRING; // s"""…"""
    case DFA_IMEXS: return TOK_INTERP_STRING; // ${ [s"""]
    default: break;
    }
    return DFA_NO_TOKEN;
}

DfaState dfa_step(DfaState s, int c, DfaCtx *ctx) {
    (void)ctx;
    switch (s) {
    case DFA_T: // T
        if ((c == '}') && dfa_guard_custom(ctx, DFA_GUARD_0)) return DFA_IB; // } [vrchol = s"]
        if ((c == '}') && dfa_guard_custom(ctx, DFA_GUARD_1)) return DFA_IM; // } [vrchol = s"""]
        if (c == '\t') return DFA_T; // \t, medzera
        if (c == ' ') return DFA_T; // \t, medzera
        if (c == '\n') return DFA_L; // \n
        if (c == EOF) return DFA_EOFT; // EOF
        if (c == '(') return DFA_LPAR; // (
        if (c == '[') return DFA_LBR; // [
        if (c == ')') return DFA_RPAR; // )
        if (c == ']') return DFA_RBR; // ]
        if ((c >= 'A' && c <= 'Z')) return DFA_ID; // A–Z, a–r, t–z
        if ((c >= 'a' && c <= 'r')) return DFA_ID; // A–Z, a–r, t–z
        if ((c >= 't' && c <= 'z')) return DFA_ID; // A–Z, a–r, t–z
        if (c == 's') return DFA_IDS; // s
        if (c == '_') return DFA_U; // _
        if (c == '@') return DFA_AT; // @
        if (c == '0') return DFA_Z; // 0
        if ((c >= '1' && c <= '9')) return DFA_INT; // 1–9
        if (c == '.') return DFA_DOT; // .
        if (c == '"') return DFA_Q1; // "
        if (c == '/') return DFA_S; // /
        if (c == '=') return DFA_EQ; // =
        if (c == '<') return DFA_LT; // <
        if (c == '>') return DFA_GT; // >
        if (c == '!') return DFA_NOT; // !
        if (c == '|') return DFA_PIPE; // |
        if (c == '+') return DFA_PLUS; // +
        if (c == '&') return DFA_AMP; // &
        if (c == '*') return DFA_SGL; // *, čiarka, -, :, ;
        if (c == ',') return DFA_SGL; // *, čiarka, -, :, ;
        if (c == '-') return DFA_SGL; // *, čiarka, -, :, ;
        if (c == ':') return DFA_SGL; // *, čiarka, -, :, ;
        if (c == ';') return DFA_SGL; // *, čiarka, -, :, ;
        return DFA_FAIL;
    case DFA_L: // \n
        if (c == '\t') return DFA_L; // \t, \n, medzera
        if (c == '\n') return DFA_L; // \t, \n, medzera
        if (c == ' ') return DFA_L; // \t, \n, medzera
        return DFA_FAIL;
    case DFA_EOFT: // EOF
        return DFA_FAIL;
    case DFA_LPAR: // (
        return DFA_FAIL;
    case DFA_LBR: // [
        return DFA_FAIL;
    case DFA_RPAR: // )
        return DFA_FAIL;
    case DFA_RBR: // ]
        return DFA_FAIL;
    case DFA_ID: // ID
        if ((c >= '0' && c <= '9')) return DFA_ID; // 0–9, A–Z, _, a–z
        if ((c >= 'A' && c <= 'Z')) return DFA_ID; // 0–9, A–Z, _, a–z
        if (c == '_') return DFA_ID; // 0–9, A–Z, _, a–z
        if ((c >= 'a' && c <= 'z')) return DFA_ID; // 0–9, A–Z, _, a–z
        return DFA_FAIL;
    case DFA_IDS: // s
        if ((c >= '0' && c <= '9')) return DFA_ID; // 0–9, A–Z, _, a–z
        if ((c >= 'A' && c <= 'Z')) return DFA_ID; // 0–9, A–Z, _, a–z
        if (c == '_') return DFA_ID; // 0–9, A–Z, _, a–z
        if ((c >= 'a' && c <= 'z')) return DFA_ID; // 0–9, A–Z, _, a–z
        if (c == '"') return DFA_S1; // "
        return DFA_FAIL;
    case DFA_U: // _
        if ((c >= '0' && c <= '9')) return DFA_ID; // 0–9, A–Z, _, a–z
        if ((c >= 'A' && c <= 'Z')) return DFA_ID; // 0–9, A–Z, _, a–z
        if (c == '_') return DFA_ID; // 0–9, A–Z, _, a–z
        if ((c >= 'a' && c <= 'z')) return DFA_ID; // 0–9, A–Z, _, a–z
        return DFA_FAIL;
    case DFA_AT: // @
        if (c == 'm') return DFA_AT1; // m
        return DFA_FAIL;
    case DFA_AT1: // @m
        if (c == 'a') return DFA_AT2; // a
        return DFA_FAIL;
    case DFA_AT2: // @ma
        if (c == 'i') return DFA_AT3; // i
        return DFA_FAIL;
    case DFA_AT3: // @mai
        if (c == 'n') return DFA_MAIN; // n
        return DFA_FAIL;
    case DFA_MAIN: // @main
        if ((c >= '0' && c <= '9')) return DFA_ERR_ID; // 0–9, A–Z, _, a–z
        if ((c >= 'A' && c <= 'Z')) return DFA_ERR_ID; // 0–9, A–Z, _, a–z
        if (c == '_') return DFA_ERR_ID; // 0–9, A–Z, _, a–z
        if ((c >= 'a' && c <= 'z')) return DFA_ERR_ID; // 0–9, A–Z, _, a–z
        return DFA_FAIL;
    case DFA_ERR_ID: // ERR_ID
        return DFA_FAIL;
    case DFA_Z: // 0
        if ((c >= '0' && c <= '9')) return DFA_ERR_N0; // 0–9, A–D, F–Z, _, a–d, f–z
        if ((c >= 'A' && c <= 'D')) return DFA_ERR_N0; // 0–9, A–D, F–Z, _, a–d, f–z
        if ((c >= 'F' && c <= 'Z')) return DFA_ERR_N0; // 0–9, A–D, F–Z, _, a–d, f–z
        if (c == '_') return DFA_ERR_N0; // 0–9, A–D, F–Z, _, a–d, f–z
        if ((c >= 'a' && c <= 'd')) return DFA_ERR_N0; // 0–9, A–D, F–Z, _, a–d, f–z
        if ((c >= 'f' && c <= 'z')) return DFA_ERR_N0; // 0–9, A–D, F–Z, _, a–d, f–z
        if (c == '.') return DFA_IDOT; // .
        if (c == 'E') return DFA_E; // E, e
        if (c == 'e') return DFA_E; // E, e
        return DFA_FAIL;
    case DFA_INT: // INT
        if ((c >= '0' && c <= '9')) return DFA_INT; // 0–9
        if (c == '_') return DFA_INT_; // _
        if (c == '.') return DFA_IDOT; // .
        if (c == 'E') return DFA_E; // E, e
        if (c == 'e') return DFA_E; // E, e
        if ((c >= 'A' && c <= 'D')) return DFA_ERR_NI; // A–D, F–Z, a–d, f–z
        if ((c >= 'F' && c <= 'Z')) return DFA_ERR_NI; // A–D, F–Z, a–d, f–z
        if ((c >= 'a' && c <= 'd')) return DFA_ERR_NI; // A–D, F–Z, a–d, f–z
        if ((c >= 'f' && c <= 'z')) return DFA_ERR_NI; // A–D, F–Z, a–d, f–z
        return DFA_FAIL;
    case DFA_INT_: // INT_
        if (c == '_') return DFA_INT_; // _
        if ((c >= '0' && c <= '9')) return DFA_INT; // 0–9
        return DFA_FAIL;
    case DFA_IDOT: // INT.
        if ((c >= '0' && c <= '9')) return DFA_F; // 0–9
        return DFA_FAIL;
    case DFA_DOT: // .
        if ((c >= '0' && c <= '9')) return DFA_F; // 0–9
        return DFA_FAIL;
    case DFA_F: // DBL₁
        if ((c >= '0' && c <= '9')) return DFA_F; // 0–9
        if (c == '_') return DFA_F_; // _
        if (c == 'E') return DFA_E; // E, e
        if (c == 'e') return DFA_E; // E, e
        if (c == 'D') return DFA_DS; // D, d
        if (c == 'd') return DFA_DS; // D, d
        if ((c >= 'A' && c <= 'C')) return DFA_ERR_NF; // A–C, F–Z, a–c, f–z
        if ((c >= 'F' && c <= 'Z')) return DFA_ERR_NF; // A–C, F–Z, a–c, f–z
        if ((c >= 'a' && c <= 'c')) return DFA_ERR_NF; // A–C, F–Z, a–c, f–z
        if ((c >= 'f' && c <= 'z')) return DFA_ERR_NF; // A–C, F–Z, a–c, f–z
        return DFA_FAIL;
    case DFA_F_: // DBL₁_
        if (c == '_') return DFA_F_; // _
        if ((c >= '0' && c <= '9')) return DFA_F; // 0–9
        return DFA_FAIL;
    case DFA_E: // e
        if (c == '+') return DFA_ES; // +, -
        if (c == '-') return DFA_ES; // +, -
        if ((c >= '0' && c <= '9')) return DFA_X; // 0–9
        return DFA_FAIL;
    case DFA_ES: // ±
        if ((c >= '0' && c <= '9')) return DFA_X; // 0–9
        return DFA_FAIL;
    case DFA_X: // DBL₃
        if ((c >= '0' && c <= '9')) return DFA_X; // 0–9
        if (c == '_') return DFA_X_; // _
        if (c == 'D') return DFA_DS; // D, d
        if (c == 'd') return DFA_DS; // D, d
        if ((c >= 'A' && c <= 'C')) return DFA_ERR_NX; // A–C, E–Z, a–c, e–z
        if ((c >= 'E' && c <= 'Z')) return DFA_ERR_NX; // A–C, E–Z, a–c, e–z
        if ((c >= 'a' && c <= 'c')) return DFA_ERR_NX; // A–C, E–Z, a–c, e–z
        if ((c >= 'e' && c <= 'z')) return DFA_ERR_NX; // A–C, E–Z, a–c, e–z
        return DFA_FAIL;
    case DFA_X_: // DBL₃_
        if (c == '_') return DFA_X_; // _
        if ((c >= '0' && c <= '9')) return DFA_X; // 0–9
        return DFA_FAIL;
    case DFA_DS: // DBL₂
        if ((c >= '0' && c <= '9')) return DFA_ERR_ND; // 0–9, A–Z, _, a–z
        if ((c >= 'A' && c <= 'Z')) return DFA_ERR_ND; // 0–9, A–Z, _, a–z
        if (c == '_') return DFA_ERR_ND; // 0–9, A–Z, _, a–z
        if ((c >= 'a' && c <= 'z')) return DFA_ERR_ND; // 0–9, A–Z, _, a–z
        return DFA_FAIL;
    case DFA_ERR_N0: // ERR 0
        return DFA_FAIL;
    case DFA_ERR_NI: // ERR INT
        return DFA_FAIL;
    case DFA_ERR_NF: // ERR DBL₁
        return DFA_FAIL;
    case DFA_ERR_NX: // ERR DBL₃
        return DFA_FAIL;
    case DFA_ERR_ND: // ERR DBL₂
        return DFA_FAIL;
    case DFA_Q1: // "
        if (c == '"') return DFA_Q2; // "
        if (DFA_IN_SIGMA(c) && !(c == '\n' || c == '"' || c == '\\')) return DFA_B; // okrem \n " \ (spätné lomítko)
        if (c == '\\') return DFA_ESC; // \ (spätné lomítko)
        return DFA_FAIL;
    case DFA_Q2: // ""
        if (c == '"') return DFA_ML; // "
        return DFA_FAIL;
    case DFA_B: // B
        if (DFA_IN_SIGMA(c) && !(c == '\n' || c == '"' || c == '\\')) return DFA_B; // okrem \n " \ (spätné lomítko)
        if (c == '\\') return DFA_ESC; // \ (spätné lomítko)
        if (c == '"') return DFA_STR; // "
        return DFA_FAIL;
    case DFA_ESC: // \ [ESC]
        if (c == '"') return DFA_B; // ", \, n, r, t
        if (c == '\\') return DFA_B; // ", \, n, r, t
        if (c == 'n') return DFA_B; // ", \, n, r, t
        if (c == 'r') return DFA_B; // ", \, n, r, t
        if (c == 't') return DFA_B; // ", \, n, r, t
        return DFA_FAIL;
    case DFA_STR: // STR
        return DFA_FAIL;
    case DFA_ML: // ML
        if (DFA_IN_SIGMA(c) && !(c == '"')) return DFA_ML; // okrem "
        if (c == '"') return DFA_M1; // "
        return DFA_FAIL;
    case DFA_M1: // ML"
        if (c == '"') return DFA_M2; // "
        if (DFA_IN_SIGMA(c) && !(c == '"')) return DFA_ML; // okrem "
        return DFA_FAIL;
    case DFA_M2: // ML""
        if (c == '"') return DFA_M3; // "
        if (DFA_IN_SIGMA(c) && !(c == '"')) return DFA_ML; // okrem "
        return DFA_FAIL;
    case DFA_M3: // """
        if (c == '"') return DFA_M3; // "
        return DFA_FAIL;
    case DFA_S: // /
        if (c == '/') return DFA_LC; // /
        if (c == '*') { // * (d = 1, nl = 0)
            ctx->d = 1;
            ctx->nl = 0;
            return DFA_BC;
        }
        return DFA_FAIL;
    case DFA_LC: // //
        if (c == '\t' || (c >= ' ' && c <= '~')) return DFA_LC; // okrem \n
        return DFA_FAIL;
    case DFA_BC: // / *
        if (DFA_IN_SIGMA(c) && !(c == '*' || c == '/')) { // okrem * / (\n: nl = 1)
            if (c == '\n') ctx->nl = 1;
            return DFA_BC;
        }
        if (c == '/') return DFA_BS; // /
        if (c == '*') return DFA_BST; // *
        return DFA_FAIL;
    case DFA_BS: // / * /
        if (c == '/') return DFA_BS; // /
        if (c == '*') { // * (d + 1), okrem * / (\n: nl = 1)
            ctx->d += 1;
            return DFA_BC;
        }
        if (DFA_IN_SIGMA(c) && !(c == '*' || c == '/')) { // * (d + 1), okrem * / (\n: nl = 1)
            if (c == '\n') ctx->nl = 1;
            return DFA_BC;
        }
        return DFA_FAIL;
    case DFA_BST: // / **
        if ((c == '/') && ctx->d > 1) { // okrem * / (\n: nl = 1), / [d > 1] (d − 1)
            ctx->d -= 1;
            return DFA_BC;
        }
        if ((c == '/') && ctx->d == 1) return DFA_END; // / [d = 1]
        if (c == '*') return DFA_BST; // *
        if (DFA_IN_SIGMA(c) && !(c == '*' || c == '/')) { // okrem * / (\n: nl = 1), / [d > 1] (d − 1)
            if (c == '\n') ctx->nl = 1;
            return DFA_BC;
        }
        return DFA_FAIL;
    case DFA_END: // * /
        return DFA_FAIL;
    case DFA_EQ: // =
        if (c == '=') return DFA_EQEQ; // =
        if (c == '>') return DFA_ARR; // >
        return DFA_FAIL;
    case DFA_EQEQ: // ==
        return DFA_FAIL;
    case DFA_ARR: // =>
        return DFA_FAIL;
    case DFA_LT: // <
        if (c == '=') return DFA_LE; // =
        if (c == '-') return DFA_LAR; // -
        return DFA_FAIL;
    case DFA_LE: // <=
        return DFA_FAIL;
    case DFA_LAR: // <-
        return DFA_FAIL;
    case DFA_GT: // >
        if (c == '=') return DFA_GE; // =
        return DFA_FAIL;
    case DFA_GE: // >=
        return DFA_FAIL;
    case DFA_NOT: // !
        if (c == '=') return DFA_NE; // =
        return DFA_FAIL;
    case DFA_NE: // !=
        return DFA_FAIL;
    case DFA_PIPE: // |
        if (c == '|') return DFA_OR; // |
        return DFA_FAIL;
    case DFA_OR: // ||
        return DFA_FAIL;
    case DFA_PLUS: // +
        if (c == '+') return DFA_CONC; // +
        return DFA_FAIL;
    case DFA_CONC: // ++
        return DFA_FAIL;
    case DFA_AMP: // &
        if (c == '&') return DFA_AND; // &
        return DFA_FAIL;
    case DFA_AND: // &&
        return DFA_FAIL;
    case DFA_SGL: // - * : , ;
        return DFA_FAIL;
    case DFA_S1: // s"
        if (c == '"') return DFA_S2; // "
        if (DFA_IN_SIGMA(c) && !(c == '\n' || c == '"' || c == '$' || c == '\\')) return DFA_IB; // okrem \n " $ \ (spätné lomítko)
        if (c == '\\') return DFA_IESC; // \ (spätné lomítko)
        if (c == '$') return DFA_D; // $
        return DFA_FAIL;
    case DFA_S2: // s""
        if (c == '"') return DFA_IM; // "
        return DFA_FAIL;
    case DFA_IB: // IB
        if (DFA_IN_SIGMA(c) && !(c == '\n' || c == '"' || c == '$' || c == '\\')) return DFA_IB; // okrem \n " $ \ (spätné lomítko)
        if (c == '\\') return DFA_IESC; // \ (spätné lomítko)
        if (c == '$') return DFA_D; // $
        if (c == '"') return DFA_IEND; // "
        return DFA_FAIL;
    case DFA_IESC: // \ [IESC]
        if (c == '"') return DFA_IB; // ", \, n, r, t
        if (c == '\\') return DFA_IB; // ", \, n, r, t
        if (c == 'n') return DFA_IB; // ", \, n, r, t
        if (c == 'r') return DFA_IB; // ", \, n, r, t
        if (c == 't') return DFA_IB; // ", \, n, r, t
        return DFA_FAIL;
    case DFA_D: // $
        if (c == '$') return DFA_IB; // $ ($$ → $) – akcia: $$ → $
        if ((c >= 'A' && c <= 'Z')) return DFA_IID; // A–Z, _, a–z
        if (c == '_') return DFA_IID; // A–Z, _, a–z
        if ((c >= 'a' && c <= 'z')) return DFA_IID; // A–Z, _, a–z
        if (c == '{') return DFA_EXS; // {
        return DFA_FAIL;
    case DFA_IID: // $id
        if ((c >= '0' && c <= '9')) return DFA_IID; // 0–9, A–Z, _, a–z
        if ((c >= 'A' && c <= 'Z')) return DFA_IID; // 0–9, A–Z, _, a–z
        if (c == '_') return DFA_IID; // 0–9, A–Z, _, a–z
        if ((c >= 'a' && c <= 'z')) return DFA_IID; // 0–9, A–Z, _, a–z
        if (DFA_IN_SIGMA(c) && !(c == '\n' || c == '"' || c == '$' || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || c == '\\' || c == '_' || (c >= 'a' && c <= 'z'))) return DFA_IB; // okrem \n " $ 0–9 A–Z \ _ a–z
        if (c == '\\') return DFA_IESC; // \ (spätné lomítko)
        if (c == '$') return DFA_D; // $
        if (c == '"') return DFA_IEND; // "
        return DFA_FAIL;
    case DFA_IEND: // s"…"
        return DFA_FAIL;
    case DFA_EXS: // ${ [s"]
        return DFA_FAIL;
    case DFA_IM: // IM
        if (DFA_IN_SIGMA(c) && !(c == '"' || c == '$')) return DFA_IM; // okrem " $
        if (c == '"') return DFA_IM1; // "
        if (c == '$') return DFA_IMD; // $
        return DFA_FAIL;
    case DFA_IM1: // IM"
        if (c == '"') return DFA_IM2; // "
        if (DFA_IN_SIGMA(c) && !(c == '"' || c == '$')) return DFA_IM; // okrem " $
        if (c == '$') return DFA_IMD; // $
        return DFA_FAIL;
    case DFA_IM2: // IM""
        if (c == '"') return DFA_IMEND; // "
        if (DFA_IN_SIGMA(c) && !(c == '"' || c == '$')) return DFA_IM; // okrem " $
        if (c == '$') return DFA_IMD; // $
        return DFA_FAIL;
    case DFA_IMEND: // s"""…"""
        if (c == '"') return DFA_IMEND; // "
        return DFA_FAIL;
    case DFA_IMD: // $ [IM]
        if (c == '$') return DFA_IM; // $ ($$ → $) – akcia: $$ → $
        if ((c >= 'A' && c <= 'Z')) return DFA_IMID; // A–Z, _, a–z
        if (c == '_') return DFA_IMID; // A–Z, _, a–z
        if ((c >= 'a' && c <= 'z')) return DFA_IMID; // A–Z, _, a–z
        if (c == '{') return DFA_IMEXS; // {
        return DFA_FAIL;
    case DFA_IMID: // $id [IM]
        if ((c >= '0' && c <= '9')) return DFA_IMID; // 0–9, A–Z, _, a–z
        if ((c >= 'A' && c <= 'Z')) return DFA_IMID; // 0–9, A–Z, _, a–z
        if (c == '_') return DFA_IMID; // 0–9, A–Z, _, a–z
        if ((c >= 'a' && c <= 'z')) return DFA_IMID; // 0–9, A–Z, _, a–z
        if (DFA_IN_SIGMA(c) && !(c == '"' || c == '$' || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || c == '_' || (c >= 'a' && c <= 'z'))) return DFA_IM; // okrem " $ 0–9 A–Z _ a–z
        if (c == '"') return DFA_IM1; // "
        if (c == '$') return DFA_IMD; // $
        return DFA_FAIL;
    case DFA_IMEXS: // ${ [s"""]
        return DFA_FAIL;
    default:
        return DFA_FAIL;
    }
}

void dfa_input_init(DfaInput *in, FILE *f) {
    in->f = f;
    in->nback = 0;
    in->line = 1;
    in->lead = 0;
    in->lead_tab = false;
    in->ws_only = true;
    in->tok_line = 1;
    in->tok_lead = 0;
    in->tok_lead_tab = false;
    in->nhist = 0;
}

/** @brief Prečíta znak a aktualizuje polohu (riadok, odsadenie riadku). */
static int dfa_getc(DfaInput *in) {
    int c = in->nback ? in->back[--in->nback] : fgetc(in->f);
    unsigned h = in->nhist++ % 64; // ulož polohu pred znakom, aby ju dfa_ungetc vedel obnoviť
    in->hist[h].line = in->line;
    in->hist[h].lead = in->lead;
    in->hist[h].lead_tab = in->lead_tab;
    in->hist[h].ws_only = in->ws_only;
    if (c == '\n') {
        in->line++;
        in->lead = 0;
        in->lead_tab = false;
        in->ws_only = true;
    } else if (in->ws_only && c == ' ') {
        in->lead++;
    } else if (in->ws_only && c == '\t') {
        in->lead_tab = true;
    } else if (c != EOF) {
        in->ws_only = false;
    }
    return c;
}

/** @brief Vráti znak na vstup (prečíta sa znova ako ďalší) a obnoví polohu pred ním. */
static void dfa_ungetc(DfaInput *in, int c) {
    if (in->nhist > 0) {
        unsigned h = --in->nhist % 64;
        in->line = in->hist[h].line;
        in->lead = in->hist[h].lead;
        in->lead_tab = in->hist[h].lead_tab;
        in->ws_only = in->hist[h].ws_only;
    }
    if (in->nback < (int)(sizeof in->back / sizeof in->back[0])) in->back[in->nback++] = c;
}

int dfa_scan(DfaInput *in, DfaCtx *ctx, DfaState *state, DynStr *lexeme) {
    DfaState s = DFA_START, last = DFA_FAIL;
    size_t last_len = 0;
    dynstr_clear(lexeme);
    for (;;) {
        int line = in->line, lead = in->lead;
        bool lead_tab = in->lead_tab;
        int c = dfa_getc(in);
        DfaState n = dfa_step(s, c, ctx);
        if (n == DFA_FAIL) {
            dfa_ungetc(in, c);
            break;
        }
        if (s == DFA_START && n == DFA_START) continue; // biele znaky pred lexémom
        if (s == DFA_START) { // prvý znak lexému: zapamätaj polohu
            in->tok_line = line;
            in->tok_lead = lead;
            in->tok_lead_tab = lead_tab;
        }
        if (c != EOF && !dynstr_append_char(lexeme, (char)c)) return ERR_INTERNAL;
        s = n;
        if (dfa_final[s]) {
            last = s;
            last_len = lexeme->len;
        }
        if (c == EOF) break;
    }
    if (last == DFA_FAIL || (!dfa_final[s] && !dfa_backtrack[s])) return ERR_LEX;
    while (lexeme->len > last_len) dfa_ungetc(in, (unsigned char)lexeme->data[--lexeme->len]);
    lexeme->data[lexeme->len] = '\0';
    *state = last;
    return ERR_OK;
}

int dfa_next_token(DfaInput *in, DfaCtx *ctx, DfaState *state, Token *tok) {
    for (;;) {
        int err = dfa_scan(in, ctx, state, &tok->lexeme);
        if (err != ERR_OK) return err;
        tok->type = dfa_token(*state, &tok->lexeme);
        tok->line = in->tok_line;
        if (tok->type != DFA_NO_TOKEN) return ERR_OK;
    }
}

// clang-format on


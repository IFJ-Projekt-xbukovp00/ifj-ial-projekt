/**
 * @file scanner.c
 * @brief Lexikálny analyzátor – z lexémov konečného automatu (dfa.c) robí tokeny pre parser
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

#include "dfa.h"
#include "dynstr.h"
#include "error.h"
#include "keywords.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/** @brief Určuje, či máme otvorený výraz v jednoriadkovom alebo viacriadkovom interpolovanom reťazci. */
typedef enum {
    INTERP_SINGLE, // s"…"
    INTERP_MULTI,  // s"""…"""
} InterpMode;

/** @brief Ukladáme si informácie o otvorenom výraze ${…} v interpolovanom reťazci. */
typedef struct {
    InterpMode mode; // podľa toho zistíme, do ktorého reťazca sa po } vrátime
    int depth;       // sledujeme zátvorky, aby } ukončilo výraz až na správnej úrovni
} InterpFrame;

/** @brief Pomocná štruktúra pre token, ktorý čaká vo fronte. */
typedef struct {
    TokenType type;
    int line;
} QueuedToken;

/** @brief Zoznam slov, ktoré po načítaní identifikátora rozpoznáme ako kľúčové slová alebo literály. */
static const struct {
    const char *word;
    TokenType type;
} keywords[] = {
    {"def", TOK_KW_DEF},
    {"do", TOK_KW_DO},
    {"Double", TOK_KW_DOUBLE},
    {"else", TOK_KW_ELSE},
    {"if", TOK_KW_IF},
    {"import", TOK_KW_IMPORT},
    {"Int", TOK_KW_INT},
    {"return", TOK_KW_RETURN},
    {"String", TOK_KW_STRING},
    {"then", TOK_KW_THEN},
    {"Unit", TOK_KW_UNIT},
    {"val", TOK_KW_VAL},
    {"var", TOK_KW_VAR},
    {"while", TOK_KW_WHILE},
    {"Boolean", TOK_KW_BOOLEAN},
    {"case", TOK_KW_CASE},
    {"class", TOK_KW_CLASS},
    {"extension", TOK_KW_EXTENSION},
    {"for", TOK_KW_FOR},
    {"List", TOK_KW_LIST},
    {"match", TOK_KW_MATCH},
    {"type", TOK_KW_TYPE},
    {"true", TOK_BOOL_LIT},
    {"false", TOK_BOOL_LIT},
};

static DfaInput input; // uchovávame vstup a informácie o polohe pri čítaní znakov
static DfaCtx ctx;     // tu máme počítadlá a príznaky, ktoré používa automat
static Token cur;      // sem sa načíta aktuálny lexém
static Token held;     // token, ktorý vydáme až po spracovaní tokenov vo fronte
static bool has_held;

static QueuedToken *queue; // tu čakajú tokeny konca riadku, odsadenia a konca vstupu
static size_t queue_len, queue_head, queue_cap;

static int *indents; // ukladáme si úrovne odsadenia, pričom začíname od nuly
static size_t indents_len, indents_cap;

static InterpFrame *interps; // evidujeme otvorené výrazy interpolovaných reťazcov
static size_t interps_len, interps_cap;

static int depth;        // počítame otvorené okrúhle a hranaté zátvorky
static int last_line;    // pamätáme si riadok posledného tokenu, aby sme ho vedeli priradiť k TOK_EOL
static bool line_start;  // označuje, že ďalší token bude prvý na riadku
static bool had_line;    // zistíme, či už máme za sebou nejaký logický riadok
static bool expect_eol;  // po takomto blokovom komentári ešte očakávame koniec riadku
static bool at_eof;      // označuje, že koniec vstupu sme už spracovali
static bool init_failed; // inicializácia zlyhala, pretože sa nepodarilo získať potrebnú pamäť

static KeyWordTree *keyword_tree; // strom kľúčových slov a literálov true/false (z keywords[])

/**
 * @brief Zväčší dynamické pole, keď už v ňom nie je miesto pre ďalší prvok.
 * @return Ukazovateľ na pole alebo NULL, ak sa pole nepodarilo zväčšiť.
 */
static void *grow(void *array, size_t *cap, size_t len, size_t item_size) {
    if (len < *cap) {
        return array;
    }
    size_t new_cap = *cap ? *cap * 2 : 16;
    void *tmp = realloc(array, new_cap * item_size);
    if (tmp != NULL) {
        *cap = new_cap;
    }
    return tmp;
}

/** @brief Pridá nový token na koniec fronty. */
static bool enqueue(TokenType type, int line) {
    QueuedToken *q = grow(queue, &queue_cap, queue_len, sizeof *queue);
    if (q == NULL) {
        return false;
    }
    queue = q;
    queue[queue_len].type = type;
    queue[queue_len].line = line;
    queue_len++;
    return true;
}

/** @brief Uloží novú úroveň odsadenia na zásobník. */
static bool push_indent(int col) {
    int *p = grow(indents, &indents_cap, indents_len, sizeof *indents);
    if (p == NULL) {
        return false;
    }
    indents = p;
    indents[indents_len++] = col;
    return true;
}

/** @brief Nastaví typ, riadok a obsah výstupného tokenu. */
static int give(Token *out, TokenType type, int line, const char *text) {
    dynstr_clear(&out->lexeme);
    if (text != NULL && !dynstr_append(&out->lexeme, text)) {
        return ERR_INTERNAL;
    }
    out->type = type;
    out->line = line;
    return ERR_OK;
}

/** @brief Vyberie prvý čakajúci token z fronty a vráti ho ako výstup. */
static int give_queued(Token *out) {
    QueuedToken q = queue[queue_head++];
    if (queue_head == queue_len) {
        queue_head = queue_len = 0;
    }
    return give(out, q.type, q.line, "");
}

/**
 * @brief Naplní strom kľúčových slov zo zoznamu keywords[].
 * @return false pri nedostatku pamäte
 */
static bool fillKeyWordTree(void) {
    for (size_t i = 0; i < sizeof keywords / sizeof keywords[0]; i++) {
        // addNodeKW si slovo skopíruje, preto smie dostať slovo z konštantnej tabuľky
        KeyWordTree *root = addNodeKW(keyword_tree, (char *)keywords[i].word);
        if (root == NULL) {
            return false;
        }
        keyword_tree = root; // výsledok vloženia sa musí priradiť, inak strom ostane prázdny
    }
    return true;
}

/**
 * @brief Typ tokenu pre identifikátor: kľúčové slovo alebo literál, ak je v strome, inak TOK_IDENTIFIER.
 *
 * Strom zatiaľ ukladá len slovo, preto sa typ nájdeného slova dohľadá v keywords[].
 */
static TokenType keyword(char *word) {
    if (!findNodeKW(keyword_tree, word)) {
        return TOK_IDENTIFIER;
    }
    for (size_t i = 0; i < sizeof keywords / sizeof keywords[0]; i++) {
        if (strcmp(word, keywords[i].word) == 0) {
            return keywords[i].type;
        }
    }
    return TOK_IDENTIFIER;
}

/**
 * @brief Pri znaku } kontrolujeme, či môžeme ukončiť vložený výraz v interpolovanom reťazci
 *        a či sú všetky zátvorky vo vnútri výrazu už uzavreté.
 */
bool dfa_guard_custom(DfaCtx *c, DfaGuard guard) {
    (void)c;
    if (interps_len == 0) {
        return false;
    }
    const InterpFrame *top = &interps[interps_len - 1];
    if (top->depth != depth) {
        return false;
    }
    switch (guard) {
    case DFA_GUARD_0: // na vrchole zásobníka je jednoriadkový interpolovaný reťazec
        return top->mode == INTERP_SINGLE;
    case DFA_GUARD_1: // na vrchole zásobníka je viacriadkový interpolovaný reťazec
        return top->mode == INTERP_MULTI;
    default:
        return false;
    }
}

/**
 * @brief Pri prvom tokene na riadku podľa odsadenia pripravíme TOK_EOL, TOK_INDENT
 *        alebo TOK_DEDENT a uložíme ich do fronty.
 * @param lead Počet medzier na začiatku riadku.
 * @param lead_tab Informácia, či sa v odsadení nachádza tabulátor.
 */
static int resolve_indent(int lead, bool lead_tab, int line) {
    if (lead_tab) {
        return ERR_LEX; // Na začiatku neprázdneho riadku tabulátor nepovoľujeme.
    }
    int top = indents[indents_len - 1];
    if (depth > 0) {
        // Ak pokračujeme v zátvorkách, riadok musí byť odsadený viac ako jeho začiatok.
        return lead > top ? ERR_OK : ERR_LEX;
    }
    if (had_line && !enqueue(TOK_EOL, last_line)) {
        return ERR_INTERNAL;
    }
    had_line = true;
    if (lead > top) {
        if (!push_indent(lead) || !enqueue(TOK_INDENT, line)) {
            return ERR_INTERNAL;
        }
        return ERR_OK;
    }
    while (lead < indents[indents_len - 1]) {
        indents_len--;
        if (!enqueue(TOK_DEDENT, line)) {
            return ERR_INTERNAL;
        }
    }
    // Nové odsadenie musí sedieť na niektorú z už použitých úrovní.
    return lead == indents[indents_len - 1] ? ERR_OK : ERR_SYNTAX;
}

/** @brief Pri konci vstupu ešte vrátime TOK_EOL, všetky potrebné TOK_DEDENT a nakoniec TOK_EOF. */
static int finish(Token *out, int line) {
    at_eof = true;
    if (interps_len > 0) {
        return ERR_LEX; // Na konci vstupu nám zostal neukončený interpolovaný reťazec.
    }
    if (had_line && !enqueue(TOK_EOL, last_line)) {
        return ERR_INTERNAL;
    }
    while (indents_len > 1) {
        indents_len--;
        if (!enqueue(TOK_DEDENT, line)) {
            return ERR_INTERNAL;
        }
    }
    if (!enqueue(TOK_EOF, line)) {
        return ERR_INTERNAL;
    }
    return give_queued(out);
}

/** @copydoc scanner_free */
void scanner_free(void) {
    free(queue);
    free(indents);
    free(interps);
    queue = NULL;
    indents = NULL;
    interps = NULL;
    queue_len = queue_head = queue_cap = 0;
    indents_len = indents_cap = 0;
    interps_len = interps_cap = 0;
    dynstr_free(&cur.lexeme);
    dynstr_free(&held.lexeme);
    clearTreeKW(keyword_tree);
    keyword_tree = NULL;
}

/** @copydoc scanner_init */
void scanner_init(FILE *input_file) {
    scanner_free();
    dfa_input_init(&input, input_file);
    memset(&ctx, 0, sizeof ctx);
    has_held = false;
    depth = 0;
    last_line = 1;
    line_start = true;
    had_line = false;
    expect_eol = false;
    at_eof = false;
    init_failed = !dynstr_init(&cur.lexeme) || !dynstr_init(&held.lexeme) || !push_indent(0) ||
                  !fillKeyWordTree();
}

/** @copydoc scanner_get_token */
int scanner_get_token(Token *token) {
    if (init_failed) {
        return ERR_INTERNAL;
    }
    if (queue_head < queue_len) {
        return give_queued(token);
    }
    if (has_held) {
        has_held = false;
        return give(token, held.type, held.line, held.lexeme.data);
    }
    if (at_eof) {
        return give(token, TOK_EOF, input.line, "");
    }

    for (;;) {
        DfaState state;
        int err = dfa_next_token(&input, &ctx, &state, &cur);
        if (err != ERR_OK) {
            return err;
        }

        switch (cur.type) {
        case TOK_1LINE_COMMENT:
            continue; // Znak nového riadku potom spracujeme ako koniec riadku.
        case TOK_MULTI_LINE_COMMENT:
            if (ctx.nl && depth == 0) {
                expect_eol = true; // Ak komentár obsahuje nový riadok, berieme ho ako koniec riadku.
            }
            continue;
        case TOK_EOL:
            expect_eol = false;
            line_start = true;
            continue;
        case TOK_EOF:
            return finish(token, cur.line);
        default:
            break;
        }

        if (expect_eol) {
            return ERR_LEX; // Za blokovým komentárom s novým riadkom už ďalší token nepovoľujeme.
        }
        if (cur.type == TOK_IDENTIFIER) {
            cur.type = keyword(cur.lexeme.data);
        }

        if (line_start) {
            line_start = false;
            err = resolve_indent(input.tok_lead, input.tok_lead_tab, cur.line);
            if (err != ERR_OK) {
                return err;
            }
        }

        // Teraz aktualizujeme stav zátvoriek a interpolácie podľa načítaného lexému.
        switch (cur.type) {
        case TOK_LPAREN:
        case TOK_LBRACKET:
            depth++;
            break;
        case TOK_RPAREN:
        case TOK_RBRACKET:
            if (depth > 0) {
                depth--;
            }
            break;
        default:
            break;
        }
        if (cur.type == TOK_INTERP_STRING && cur.lexeme.data[0] == '}') {
            interps_len--; // Znak } ukončil vložený výraz, ktorý sme mali otvorený.
        }
        if (state == DFA_EXS || state == DFA_IMEXS) {
            InterpFrame *f = grow(interps, &interps_cap, interps_len, sizeof *interps);
            if (f == NULL) {
                return ERR_INTERNAL;
            }
            interps = f;
            interps[interps_len].mode = state == DFA_EXS ? INTERP_SINGLE : INTERP_MULTI;
            interps[interps_len].depth = depth;
            interps_len++;
        }

        last_line = cur.line;
        if (queue_head < queue_len) {
            // Najskôr musíme vrátiť tokeny odsadenia a konca riadku, až potom tento token.
            has_held = true;
            err = give(&held, cur.type, cur.line, cur.lexeme.data);
            if (err != ERR_OK) {
                return err;
            }
            return give_queued(token);
        }
        return give(token, cur.type, cur.line, cur.lexeme.data);
    }
}

/**
 * @file dfa.h
 * @brief Konečný automat lexikálneho analyzátora (stavy, prechody, maximal munch)
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

#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "dynstr.h"
#include "token.h"

/** @brief Tento stav prijme lexém, ale nevytvorí token, takže ho preskočíme. */
#define DFA_NO_TOKEN TOK_COUNT

/** @brief Tu máme vypísané všetky stavy, do ktorých sa môže automat dostať. */
typedef enum {
    DFA_T,             // T – Začiatočný stav; pri každom novom lexéme začíname práve tu.
    DFA_L,             // \n (koncový) – Automat narazil na koniec riadku.
    DFA_EOFT,          // EOF (koncový) – Koniec súboru; týmto stavom sme spojili pôvodné EOFL a EOFT.
    DFA_LPAR,          // ( (koncový) – Načítali sme ľavú okrúhlu zátvorku, takže vrátime LPAREN.
    DFA_LBR,           // [ (koncový) – Načítali sme ľavú hranatú zátvorku (napr. pri List[T]), vrátime LBRACKET.
    DFA_RPAR,          // ) (koncový) – Načítali sme pravú okrúhlu zátvorku a vrátime RPAREN.
    DFA_RBR,           // ] (koncový) – Načítali sme pravú hranatú zátvorku a vrátime RBRACKET.
    DFA_ID,            // ID (koncový) – Načítaný text je identifikátor alebo kľúčové slovo.
    DFA_IDS,           // s (koncový) – V tomto stave máme načítané presne jedno písmeno „s“.
    DFA_U,             // _ (koncový) – Samotný podčiarkovník nepovažujeme za identifikátor; vraciame UNDERSCORE (používa sa napr. vo vzore case _).
    DFA_AT,            // @ – Zatiaľ sme načítali iba znak @.
    DFA_AT1,           // @m – Máme načítané @m, ale na dokončenie anotácie ešte čakáme.
    DFA_AT2,           // @ma – Zatiaľ máme @ma; tento stav ešte nie je akceptujúci.
    DFA_AT3,           // @mai – Načítali sme @mai, ešte nám chýba zvyšok anotácie.
    DFA_MAIN,          // @main (koncový) – Celá anotácia je načítaná, vrátime token ANNOT_MAIN.
    DFA_ERR_ID,        // ERR_ID – Lexikálna chyba 1: za @main nasleduje znak identifikátora (napr.
    DFA_Z,             // 0 (koncový) – Načítali sme celočíselný literál s hodnotou 0.
    DFA_INT,           // INT (koncový) – Máme načítaný celočíselný literál typu Int.
    DFA_INT_,          // INT_ – Po čísliciach prišlo podčiarkovníkové oddeľovadlo; musí za ním nasledovať ďalšia číslica, inak hlásime chybu.
    DFA_IDOT,          // INT. – Návrh opravy lookaheadu „.
    DFA_DOT,           // . (koncový) – Bodku vraciame ako token DOT (využíva sa napr. pri importe).
    DFA_F,             // DBL₁ (koncový) – Desatinný literál s desatinnou časťou (1.5, .5).
    DFA_F_,            // DBL₁_ – Podčiarkovník v desatinnej časti – musí nasledovať číslica.
    DFA_E,             // e – Prečítané e/E exponentu – musí nasledovať znamienko alebo číslica.
    DFA_ES,            // ± – Znamienko exponentu – musí nasledovať číslica.
    DFA_X,             // DBL₃ (koncový) – Desatinný literál s exponentom (1e5, 1.5E-3).
    DFA_X_,            // DBL₃_ – Podčiarkovník v exponente – musí nasledovať číslica.
    DFA_DS,            // DBL₂ (koncový) – Desatinný literál s príponou d/D (1.5d).
    DFA_ERR_N0,        // ERR 0 – Lexikálna chyba 1: neplatné pokračovanie číselného literálu (01, 0_, 0x).
    DFA_ERR_NI,        // ERR INT – Lexikálna chyba 1: neplatné pokračovanie číselného literálu (1abc).
    DFA_ERR_NF,        // ERR DBL₁ – Lexikálna chyba 1: neplatné pokračovanie číselného literálu (1.5x).
    DFA_ERR_NX,        // ERR DBL₃ – Lexikálna chyba 1: neplatné pokračovanie číselného literálu (1e5x).
    DFA_ERR_ND,        // ERR DBL₂ – Lexikálna chyba 1: neplatné pokračovanie číselného literálu (1.5dd, 1.5d_).
    DFA_Q1,            // " – Otváracia úvodzovka.
    DFA_Q2,            // "" (koncový) – Prázdny reťazec "" – token STRING s prázdnou hodnotou.
    DFA_B,             // B – Telo jednoriadkového reťazca.
    DFA_ESC,           // \ [ESC] – Escape sekvencia – povolené sú len \n \r \t \\ \", iná je chyba 1.
    DFA_STR,           // STR (koncový) – Token STRING (hodnota bez úvodzoviek, escape sekvencie vyhodnotené).
    DFA_ML,            // ML – Telo viacriadkového reťazca """…""" – všetko doslova vrátane \n, escape sekvencie sa nevyhodnocujú.
    DFA_M1,            // ML" – Jedna " vo viacriadkovom reťazci – môže to byť začiatok ukončenia.
    DFA_M2,            // ML"" – Dve " za sebou vo viacriadkovom reťazci.
    DFA_M3,            // """ (koncový) – Token STRING (viacriadkový).
    DFA_S,             // / (koncový) – Lomka tvorí operátor delenia, preto vraciame DIV.
    DFA_LC,            // // (koncový) – Začal sa riadkový komentár; jeho obsah preskočíme a token nevraciame.
    DFA_BC,            // / * – Sme vo vnútri blokového komentára a jeho obsah zatiaľ ignorujeme.
    DFA_BS,            // / * / – V komentári sme načítali lomku; ak príde hviezdička, začína ďalšia úroveň komentára.
    DFA_BST,           // / ** – V komentári sme načítali hviezdičku; ak za ňou príde lomka, ukončíme jednu úroveň komentára.
    DFA_END,           // * / (koncový) – Jedna úroveň blokového komentára sa skončila; pri d = 0 je uzavretý celý komentár.
    DFA_EQ,            // = (koncový) – Token ASSIGN.
    DFA_EQEQ,          // == (koncový) – Token EQ (porovnanie).
    DFA_ARR,           // => (koncový) – Token ARROW (LAMBDAS, case v ALGEBDT).
    DFA_LT,            // < (koncový) – Token LT.
    DFA_LE,            // <= (koncový) – Token LE.
    DFA_LAR,           // <- (koncový) – Token LARROW (generátor for v CONTROL).
    DFA_GT,            // > (koncový) – Token GT.
    DFA_GE,            // >= (koncový) – Token GE.
    DFA_NOT,           // ! (koncový) – Token NOT (BOOLTHEN).
    DFA_NE,            // != (koncový) – Token NE.
    DFA_PIPE,          // | (koncový) – Token PIPE – zjednotenie typov (ALGEBDT).
    DFA_OR,            // || (koncový) – Token OR (BOOLTHEN).
    DFA_PLUS,          // + (koncový) – Token PLUS (sčítanie, konkatenácia reťazcov, unárne +).
    DFA_CONC,          // ++ (koncový) – Token CONCAT – konkatenácia zoznamov (LISTS).
    DFA_AMP,           // & – Prečítaný &.
    DFA_AND,           // && (koncový) – Token AND (BOOLTHEN).
    DFA_SGL,           // - * : , ; (koncový) – Jednoznakové operátory/oddeľovače – token sa určí podľa prečítaného znaku: - MINUS (aj unárne), * MUL, : COLON, čiarka COMMA, ; SEMICOLON.
    DFA_S1,            // s" – Prečítané s" – začiatok jednoriadkového interpolovaného reťazca.
    DFA_S2,            // s"" (koncový) – Prázdny interpolovaný reťazec s"" – token INTERP_STR s prázdnym obsahom (bývalý IEMP).
    DFA_IB,            // IB – Telo jednoriadkového interpolovaného reťazca.
    DFA_IESC,          // \ [IESC] – Escape sekvencia v s"…" – povolené \n \r \t \\ \" (\" je jediný spôsob zápisu úvodzovky).
    DFA_D,             // $ – Prečítaný $ v s"…" – musí nasledovať $, znak začínajúci identifikátor alebo {, inak chyba 1.
    DFA_IID,           // $id – Časť $id v s"…" – identifikátor premennej.
    DFA_IEND,          // s"…" (koncový) – Koniec jednoriadkového interpolovaného reťazca.
    DFA_EXS,           // ${ [s"] (koncový) – ${ – koniec časti reťazca: lexém začal s" → INTERP_HEAD, začal } → INTERP_MID; za ním token EXPR_START.
    DFA_IM,            // IM – Telo viacriadkového interpolovaného reťazca s"""…""" (dokreslené, bývalý box IML „= ML + vetva $“).
    DFA_IM1,           // IM" – Jedna " v s"""…""".
    DFA_IM2,           // IM"" – Dve " za sebou v s"""…""".
    DFA_IMEND,         // s"""…""" (koncový) – Koniec viacriadkového interpolovaného reťazca.
    DFA_IMD,           // $ [IM] – Prečítaný $ v s"""…""" – musí nasledovať $, znak začínajúci identifikátor alebo {.
    DFA_IMID,          // $id [IM] – Časť $id v s"""…""" (ako IID).
    DFA_IMEXS,         // ${ [s"""] (koncový) – ${ – koniec časti reťazca: lexém začal s" → INTERP_HEAD, začal } → INTERP_MID; za ním token EXPR_START.
    DFA_COUNT,           // počet stavov
    DFA_FAIL = -1        // žiadny prechod
} DfaState;

/** @brief Tu nastavujeme stav, v ktorom automat začína. */
#define DFA_START DFA_T

/** @brief Kontext automatu, do ktorého si ukladáme počítadlá a príznaky potrebné pri prechodoch. */
typedef struct DfaCtx {
    int d;                // Túto premennú používame pri strážach a akciách prechodov.
    int nl;               // Túto premennú používame pri strážach a akciách prechodov.
    void *user;          // Sem si scanner môže odložiť vlastné dáta.
} DfaCtx;

/** @brief Tieto špeciálne stráže nevieme vyhodnotiť jednoduchým porovnaním, preto ich rieši scanner. */
typedef enum {
    DFA_GUARD_0, // vrchol = s"
    DFA_GUARD_1, // vrchol = s"""
} DfaGuard;

/**
 * @brief Vyhodnotí stráž, ktorú automat nevie vyhodnotiť sám. Implementuje scanner.
 * @return true, ak sa prechod smie použiť.
 */
bool dfa_guard_custom(DfaCtx *ctx, DfaGuard guard);

/** @brief Informácie o vstupe vrátane znakov, ktoré potrebujeme vrátiť, a aktuálnej pozície v súbore. */
typedef struct {
    FILE *f;             // Súbor, z ktorého práve čítame.
    int back[64];        // Sem odkladáme znaky, ktoré potrebujeme vrátiť späť (aj viac než jeden).
    int nback;           // Počet znakov, ktoré čakajú na vrátenie.
    int line;            // Číslo riadku, na ktorom sa práve nachádzame (počítame od 1).
    int lead;            // Koľko medzier je na začiatku aktuálneho riadku.
    bool lead_tab;       // Či sa v odsadení aktuálneho riadku nachádza tabulátor.
    bool ws_only;        // Či sme na tomto riadku zatiaľ načítali iba biele znaky.
    int tok_line;        // Riadok, na ktorom sa začal posledný lexém.
    struct {             // poloha pred posledne prečítanými znakmi (obnoví ju dfa_ungetc)
        int line, lead;
        bool lead_tab, ws_only;
    } hist[64];
    unsigned nhist;      // počet uložených polôh (kruhovo, posledných 64)
    int tok_lead;        // Odsadenie riadku, na ktorom sa začal posledný lexém.
    bool tok_lead_tab;   // Či sa v tomto odsadení nachádzal tabulátor.
} DfaInput;

/** @brief Vráti názov stavu, čo sa hodí hlavne pri ladení. */
const char *dfa_state_name(DfaState s);

/** @brief Podľa akceptujúceho stavu a lexému určíme token; pri preskakovanom lexéme vrátime DFA_NO_TOKEN. */
TokenType dfa_token(DfaState s, const DynStr *lexeme);

/**
 * @brief Posunie automat o jeden krok podľa aktuálneho stavu a načítaného znaku (prípadne EOF).
 * Pri prechode zároveň vykoná príslušné akcie a podľa potreby upraví hodnoty v ctx.
 * @return Nový stav; ak takýto prechod nemáme definovaný, vráti DFA_FAIL.
 */
DfaState dfa_step(DfaState s, int c, DfaCtx *ctx);

/** @brief Pripraví vstupný objekt na čítanie zo zadaného súboru. */
void dfa_input_init(DfaInput *in, FILE *f);

/**
 * @brief Funkcia načíta celý lexém a snaží sa pritom získať jeho najdlhšiu platnú podobu (maximal munch).
 *
 * Biele znaky na začiatku preskočíme, takže sa nestanú súčasťou lexému. Ak sa automat
 * dostane do koncového stavu alebo môže vrátiť prečítané znaky späť, použijeme posledný akceptovaný
 * lexém a všetko navyše vrátime na vstup. Ak sa to nepodarí, ide o lexikálnu chybu.
 * @param[out] state  sem uložíme koncový stav načítaného lexému
 * @param[out] lexeme sem uložíme text lexému; dynamický reťazec musí byť vopred inicializovaný
 * @return ERR_OK pri úspechu, ERR_LEX pri chybe v lexéme a ERR_INTERNAL pri probléme s pamäťou
 */
int dfa_scan(DfaInput *in, DfaCtx *ctx, DfaState *state, DynStr *lexeme);

/**
 * @brief Funkcia postupne načítava lexémy a vracia ďalší token; tie, ktoré token nevytvárajú, preskočí.
 * @param[out] state sem uložíme koncový stav lexému
 * @param[out] tok   sem uložíme token; jeho tok->lexeme musí byť vopred inicializovaný
 * @return ERR_OK pri úspechu, ERR_LEX pri lexikálnej chybe alebo ERR_INTERNAL pri internej chybe
 */
int dfa_next_token(DfaInput *in, DfaCtx *ctx, DfaState *state, Token *tok);

// clang-format on

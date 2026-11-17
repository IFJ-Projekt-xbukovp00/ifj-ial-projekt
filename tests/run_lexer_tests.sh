#!/usr/bin/env bash
# Projekt: Implementace překladače imperativního jazyka IFJ26
# Predmety: IFJ a IAL, FIT VUT v Brne, 2026/2027
#
# Súbor:   run_lexer_tests.sh
# Popis:   Testy tokenov lexera (tests/tokens)
#
# Použitie AI: tento súbor vygenerovala AI (Claude Opus 5.5).
# Prompt: "vytvor nam testy pre lexer podla zadania IFJ26 pre tieto cases:
#   - tabulator v odsadeni (aj po medzerach) -> chyba 1
#   - neznamy znak # a ` -> chyba 1
#   - neukonceny retazec (koniec riadku aj EOF) -> chyba 1
#   - zle escape sekvencie \q, \x41, \u0041 -> chyba 1
#   - neukonceny blokovy a vnoreny komentar -> chyba 1
#   - neukonceny viacriadkovy retazec """ -> chyba 1
#   - tokeny za viacriadkovym komentarom na tom istom riadku -> chyba 1
#   - cele cislo s nulou na zaciatku (007) a s _ na konci (1_) -> chyba 1
#   - exponent bez cislic (1e, 2.5e+) -> chyba 1
#   - podciarkovniky v cisle (1_000_000), 0 a 2147483647
#   - vsetky tvary desatinneho literalu (1.5, .5, 1e3, 2.5E-1, 1_0.0_1, 3.0d, 4e+1D)
#   - escape sekvencie \t \\ \" \r \n
#   - viacriadkovy retazec doslovne (aj \n a ") a prazdny """"""
#   - riadkove, blokove a vnorene komentare, komentare pred prologom
#   - retazec obsahujuci // a znacky blokoveho komentara
#   - tabulator vo vnutri riadku, prazdne riadky a riadky len s bielymi znakmi
#   - identifikatory (_a1, a_B_2, Value), literal (), strednik, @main na samostatnom riadku
#   - tokeny: vsetky klucove slova, true/false a podobne identifikatory (define, Integer, VAL)
#   - tokeny: vsetky operatory a oddelovace (++, <=, <-, =>, &&, ||, |, [ ], @main)
#   - tokeny: lexemy cisel (podciarkovniky, exponent, pripona d/D)
#   - tokeny: retazce s escape sekvenciami a viacriadkove retazce
#   - tokeny: casti interpolovanych retazcov s"..." ($id, ${vyraz}, $$, s"""...""")
#   - tokeny: EOL, INDENT a DEDENT pri blokoch, prazdnych riadkoch a v zatvorkach
#   - tokeny: komentare a stredniky
#   - tokeny: tokeny pred lexikalnou chybou a ERROR 1 (neukonceny retazec, tabulator)"
#
# Autori:  Patrik Bukovský  (xbukovp00)
#          Tomáš Szalay     (xszalat00)
#          Mikuláš Gärtner  (xgartnm00)
#          Daniel Malček    (xmalced00)
#
# Copyright (C) 2026  Patrik Bukovský, Tomáš Szalay, Mikuláš Gärtner,
#                     Daniel Malček
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Každý test je dvojica tests/tokens/<meno>.scala a <meno>.tokens. Súbor .tokens
# je očakávaný výstup tests/lexer_dump: jeden token na riadok v tvare
#   <riadok> <TOK_TYP> "<lexém>"
# Pri lexikálnej chybe je posledný riadok "ERROR <kód>".
#
# Použitie: tests/run_lexer_tests.sh [filter]
#   LEXDUMP  cesta k výpisu tokenov (predvolene tests/lexer_dump)

set -u

LEXDUMP=${LEXDUMP:-tests/lexer_dump}
CASES_DIR="$(dirname "$0")/tokens"
TMP_DIR="$(dirname "$0")/tmp/tokens"
FILTER=${1:-}

mkdir -p "$TMP_DIR"

pass=0
fail=0

for src in "$CASES_DIR"/*.scala; do
    [ -e "$src" ] || continue
    name=$(basename "$src" .scala)
    [ -n "$FILTER" ] && [[ "$name" != *"$FILTER"* ]] && continue
    expected="$CASES_DIR/$name.tokens"

    if [ ! -f "$expected" ]; then
        echo "FAIL  $name: chýba $name.tokens"
        fail=$((fail + 1))
        continue
    fi

    "$LEXDUMP" <"$src" >"$TMP_DIR/$name.tokens" 2>"$TMP_DIR/$name.err"

    if diff -q "$expected" "$TMP_DIR/$name.tokens" >/dev/null; then
        echo "PASS  $name"
        pass=$((pass + 1))
    else
        echo "FAIL  $name: iné tokeny (< očakávané, > lexer)"
        diff "$expected" "$TMP_DIR/$name.tokens" | head -n 10 | sed 's/^/      /'
        fail=$((fail + 1))
    fi
done

echo
echo "Výsledok: $pass OK, $fail FAIL"
[ "$fail" -eq 0 ]

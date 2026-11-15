#!/usr/bin/env bash
# Projekt: Implementace překladače imperativního jazyka IFJ26
# Predmety: IFJ a IAL, FIT VUT v Brne, 2026/2027
#
# Súbor:   run_tests.sh
# Popis:   End-to-end testy prekladača
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

# Každý test je súbor tests/cases/<cast>/<meno>.scala (cast = lexer, parser,
# semantic, codegen) a k nemu voliteľne:
#   <meno>.rc      očakávaný návratový kód prekladača (predvolene 0)
#   <meno>.in      štandardný vstup pre interpret
#   <meno>.out     očakávaný štandardný výstup interpretu
#   <meno>.int_rc  očakávaný návratový kód interpretu (predvolene 0)
#
# Testy v časti lexer/ sa nespúšťajú cez celý prekladač, ale priamo cez
# tests/lexer_dump: kontroluje sa len návratový kód lexera (1 alebo 0), .out sa
# pre ne nepoužíva. Takto ukazujú stav lexera aj bez hotového parsera.
#
# Použitie: tests/run_tests.sh [filter]   (napr. lexer/ spustí len testy lexera)
#   IFJ26    cesta k prekladaču   (predvolene ./ifj26)
#   IC26INT  cesta k interpretu   (predvolene ic26int)
#   LEXDUMP  cesta k výpisu tokenov (predvolene tests/lexer_dump)

set -u

IFJ26=${IFJ26:-./ifj26}
IC26INT=${IC26INT:-ic26int}
LEXDUMP=${LEXDUMP:-$(dirname "$0")/lexer_dump}
CASES_DIR="$(dirname "$0")/cases"
TMP_DIR="$(dirname "$0")/tmp"
FILTER=${1:-}

mkdir -p "$TMP_DIR"

have_int=1
command -v "$IC26INT" >/dev/null 2>&1 || have_int=0

pass=0
fail=0
skip=0

# počty pre jednotlivé časti (lexer, parser, ...) v poradí, v akom prišli
declare -A sec_pass sec_fail sec_skip
sections=()
section=""

# zapíše výsledok testu do celkového aj sekčného počtu
count() {
    case "$1" in
    pass) pass=$((pass + 1)); sec_pass[$section]=$((sec_pass[$section] + 1)) ;;
    fail) fail=$((fail + 1)); sec_fail[$section]=$((sec_fail[$section] + 1)) ;;
    skip) skip=$((skip + 1)); sec_skip[$section]=$((sec_skip[$section] + 1)) ;;
    esac
}

while IFS= read -r src; do
    name=${src#"$CASES_DIR"/}
    name=${name%.scala}
    [ -n "$FILTER" ] && [[ "$name" != *"$FILTER"* ]] && continue
    base="$CASES_DIR/$name"
    mkdir -p "$(dirname "$TMP_DIR/$name")"

    # nová časť -> nadpis, napr. "LEXER tests:"
    sec=other
    [[ "$name" == */* ]] && sec=${name%%/*}
    if [ "$sec" != "$section" ]; then
        section=$sec
        sections+=("$section")
        sec_pass[$section]=0; sec_fail[$section]=0; sec_skip[$section]=0
        [ ${#sections[@]} -gt 1 ] && echo
        echo "${section^^} tests:"
    fi

    exp_rc=0
    [ -f "$base.rc" ] && exp_rc=$(tr -d '[:space:]' <"$base.rc")

    # časť lexer: len lexer, len návratový kód
    if [ "$section" = "lexer" ]; then
        if [ ! -x "$LEXDUMP" ]; then
            echo "SKIP  $name: $LEXDUMP nie je preložený (make tests/lexer_dump)"
            count skip
            continue
        fi
        "$LEXDUMP" <"$src" >"$TMP_DIR/$name.tokens" 2>"$TMP_DIR/$name.err"
        rc=$?
        if [ "$rc" -eq "$exp_rc" ]; then
            echo "PASS  $name"
            count pass
        else
            echo "FAIL  $name: lexer vrátil $rc, očakávané $exp_rc"
            count fail
        fi
        continue
    fi

    "$IFJ26" <"$src" >"$TMP_DIR/$name.code" 2>"$TMP_DIR/$name.err"
    rc=$?

    if [ "$rc" -ne "$exp_rc" ]; then
        echo "FAIL  $name: prekladač vrátil $rc, očakávané $exp_rc"
        count fail
        continue
    fi

    # chybový test alebo test bez očakávaného výstupu končí po preklade
    if [ "$exp_rc" -ne 0 ] || [ ! -f "$base.out" ]; then
        echo "PASS  $name"
        count pass
        continue
    fi

    if [ "$have_int" -eq 0 ]; then
        echo "SKIP  $name: interpret $IC26INT nenájdený"
        count skip
        continue
    fi

    in_file=/dev/null
    [ -f "$base.in" ] && in_file="$base.in"
    exp_int_rc=0
    [ -f "$base.int_rc" ] && exp_int_rc=$(tr -d '[:space:]' <"$base.int_rc")

    "$IC26INT" "$TMP_DIR/$name.code" <"$in_file" >"$TMP_DIR/$name.out" 2>/dev/null
    int_rc=$?

    if [ "$int_rc" -ne "$exp_int_rc" ]; then
        echo "FAIL  $name: interpret vrátil $int_rc, očakávané $exp_int_rc"
        count fail
    elif ! diff -q "$base.out" "$TMP_DIR/$name.out" >/dev/null; then
        echo "FAIL  $name: rozdielny výstup"
        diff "$base.out" "$TMP_DIR/$name.out" | head -n 10 | sed 's/^/      /'
        count fail
    else
        echo "PASS  $name"
        count pass
    fi
done < <(
    # najprv časti v poradí, v akom pracuje prekladač, potom zvyšok
    for part in lexer parser semantic codegen; do
        [ -d "$CASES_DIR/$part" ] && find "$CASES_DIR/$part" -name '*.scala' | sort
    done
    find "$CASES_DIR" -name '*.scala' \
        -not -path "$CASES_DIR/lexer/*" -not -path "$CASES_DIR/parser/*" \
        -not -path "$CASES_DIR/semantic/*" -not -path "$CASES_DIR/codegen/*" | sort
)

echo
echo "Súhrn:"
for sec in "${sections[@]}"; do
    printf "  %-10s %3d OK, %3d FAIL, %3d SKIP\n" "${sec^^}" \
        "${sec_pass[$sec]}" "${sec_fail[$sec]}" "${sec_skip[$sec]}"
done
echo "Výsledok: $pass OK, $fail FAIL, $skip SKIP"
[ "$fail" -eq 0 ]

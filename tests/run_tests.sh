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

# Každý test je súbor tests/cases/<meno>.scala a k nemu voliteľne:
#   <meno>.rc      očakávaný návratový kód prekladača (predvolene 0)
#   <meno>.in      štandardný vstup pre interpret
#   <meno>.out     očakávaný štandardný výstup interpretu
#   <meno>.int_rc  očakávaný návratový kód interpretu (predvolene 0)
#
# Použitie: tests/run_tests.sh [filter]
#   IFJ26    cesta k prekladaču   (predvolene ./ifj26)
#   IC26INT  cesta k interpretu   (predvolene ic26int)

set -u

IFJ26=${IFJ26:-./ifj26}
IC26INT=${IC26INT:-ic26int}
CASES_DIR="$(dirname "$0")/cases"
TMP_DIR="$(dirname "$0")/tmp"
FILTER=${1:-}

mkdir -p "$TMP_DIR"

have_int=1
command -v "$IC26INT" >/dev/null 2>&1 || have_int=0

pass=0
fail=0
skip=0

for src in "$CASES_DIR"/*.scala; do
    [ -e "$src" ] || continue
    name=$(basename "$src" .scala)
    [ -n "$FILTER" ] && [[ "$name" != *"$FILTER"* ]] && continue
    base="$CASES_DIR/$name"

    exp_rc=0
    [ -f "$base.rc" ] && exp_rc=$(tr -d '[:space:]' <"$base.rc")

    "$IFJ26" <"$src" >"$TMP_DIR/$name.code" 2>"$TMP_DIR/$name.err"
    rc=$?

    if [ "$rc" -ne "$exp_rc" ]; then
        echo "FAIL  $name: prekladač vrátil $rc, očakávané $exp_rc"
        fail=$((fail + 1))
        continue
    fi

    # chybový test alebo test bez očakávaného výstupu končí po preklade
    if [ "$exp_rc" -ne 0 ] || [ ! -f "$base.out" ]; then
        echo "PASS  $name"
        pass=$((pass + 1))
        continue
    fi

    if [ "$have_int" -eq 0 ]; then
        echo "SKIP  $name: interpret $IC26INT nenájdený"
        skip=$((skip + 1))
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
        fail=$((fail + 1))
    elif ! diff -q "$base.out" "$TMP_DIR/$name.out" >/dev/null; then
        echo "FAIL  $name: rozdielny výstup"
        diff "$base.out" "$TMP_DIR/$name.out" | head -n 10 | sed 's/^/      /'
        fail=$((fail + 1))
    else
        echo "PASS  $name"
        pass=$((pass + 1))
    fi
done

echo
echo "Výsledok: $pass OK, $fail FAIL, $skip SKIP"
[ "$fail" -eq 0 ]

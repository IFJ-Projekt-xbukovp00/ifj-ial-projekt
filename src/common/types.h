/**
 * @file types.h
 * @brief Dátové typy jazyka IFJ26 – spoločné pre parser, sémantiku aj generátor
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

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct Symbol; // Definícia v symtable.h

/** @brief Druh typu a podľa neho vieme, ktorú časť štruktúry Type máme čítať. */
typedef enum {
    TY_INT,    // Int
    TY_DOUBLE, // Double
    TY_STRING, // String
    TY_UNIT,   // Unit, t.j. typ s jedinou hodnotou () - napr. pre case
    TY_BOOL,   // Boolean (BOOLTHEN)
    TY_LIST,   // List[T] (LISTS)
    TY_FUNC,   // Funkcia (T1, T2) => R (LAMBDAS)
    TY_UNION,  // Zjednotenie Int | String (ALGEBDT)
    TY_CLASS,  // Case class, napr. Point (ALGEBDT)
    TY_NAMED,  // Meno typu, o ktorom parser ešte nevie, čo je (Point, Input).
    TY_RANGE,  // Interne – hodnota `1 to 5` v cykle for (CONTROL).
    TY_ANY     // Interne – parameter print a toStr, ktorý zoberie čokoľvek.
} TypeKind;

struct Type;

/** @brief Dynamické pole typov, používa sa pre parametre funkcie (TY_FUNC) a členy zjednotenia
 * (TY_UNION). */
typedef struct {
    struct Type **items; // Pole ukazovateľov na typy.
    size_t len;          // Koľko typov v poli práve je.
    size_t cap;          // Cap, potom zväčšujeme
} TypeList;

/**
 * @brief Jeden typ jazyka IFJ26.
 *
 * Jednoduchým typom (Int, Double, ...) stačí uložiť ich kind.
 * Zložitejšie typy potrebujú aj ďalšie údaje v unione. Napríklad pri
 * List[Int] si ukladáme typ prvkov zoznamu.
 *
 * Typy po vytvorení nemeníme a môžu sa používať na viacerých miestach
 * naraz (napr. v AST alebo v tabuľke symbolov). Preto ich neuvoľňujeme
 * jednotlivo, ale všetky naraz pomocou type_free_all().
 * Ak je typ NULL, znamená to, že ešte nebol určený alebo nebol uvedený.
 */
typedef struct Type {
    TypeKind kind; // Druh typu.
    union {
        struct Type *elem; // TY_LIST: typ prvkov zoznamu.
        struct {
            TypeList params;  // Typy parametrov.
            struct Type *ret; // Návratový typ.
        } func;               // TY_FUNC
        TypeList members;     // TY_UNION: typy, z ktorých sa zjednotenie skladá.
        struct Symbol *cls;   // TY_CLASS: symbol triedy, v ňom sú jej atribúty.
        struct {
            char *name;            // Meno tak, ako je v zdrojáku.
            struct Type *resolved; // Na čo sa meno odkazuje – sémantika job
        } named;                   // TY_NAMED
    };
} Type;

/* Vytváranie a správa typov */

/**
 * @brief Vráti jednoduchý typ.
 *
 * Nič nealokuje a pre ten istý kind vráti vždy ten istý ukazovateľ, takže
 * jednoduché typy môžeme porovnávať aj obyčajným ==.
 * @param kind Druh typu, napr. TY_INT.
 * @return Ukazovateľ na typ.
 */
Type *type_get_basic(TypeKind kind);

/**
 * @brief Vytvorí typ zoznamu List[elem].
 * @param elem Typ prvkov zoznamu.
 * @return Nový typ.
 */
Type *type_create_list(Type *elem);

/**
 * @brief Vytvorí typ funkcie (params) => ret.
 * @param params Typy parametrov. Pole si typ nechá, volajúci ho už neuvoľňuje.
 * @param ret Návratový typ.
 * @return Nový typ.
 */
Type *type_create_function(TypeList params, Type *ret);

/**
 * @brief Vytvorí zjednotenie typov, napr. Int | Unit.
 * @param members Typy, z ktorých sa zjednotenie skladá.
 * @return Nový typ.
 */
Type *type_create_union(TypeList members);

/**
 * @brief Vytvorí typ podla mena, ktoré parser nevie rozlíšit (Point, Input).
 *
 * Meno sa skopíruje. Čo sa za ním skrýva, zistí až semantika.
 * @param name Meno typu zo zdrojového suboru.
 * @return Nový typ.
 */
Type *type_create_named(const char *name);

/**
 * @brief Vytvorí typ pre case class.
 * @param cls Symbol triedy z tabulky symbolov.
 * @return Nový typ.
 */
Type *type_create_class(struct Symbol *cls);

/**
 * @brief Pridá typ na koniec pola, pole sa podľa potreby zväčší.
 * @param list Pole typov.
 * @param t Typ, ktorý pridávame.
 */
void type_list_append(TypeList *list, Type *t);

/* Porovnávanie */

/**
 * @brief Zistí, či sú dva typy rovnaké.
 *
 * Porovnáva aj vnútro typu, takže List[Int] a List[Int] sú rovnaké, aj keď
 * ide o dva rôzne ukazovatele. Pri TY_NAMED sa pozerá na to, na čo meno ukazuje.
 * @return true, ak sú typy rovnaké.
 */
bool type_equals(const Type *a, const Type *b);

/**
 * @brief Zistí, či môžeme hodnotu typu actual použiť tam, kde čakáme typ expected.
 *
 * Táto funkcia kontroluje, či je typ actual kompatibilný s typom expected.
 * @param expected Typ, ktorý na danom mieste čakáme (napr. premenná, parameter).
 * @param actual Typ hodnoty, ktorú tam dávame.
 * @return true, ak je to v poriadku.
 */
bool type_accepts(const Type *expected, const Type *actual);

/**
 * @brief IFJ26 prepis pre chyby - development.
 *
 * Vypíše typ na štandardný výstup alebo do súboru. Používa sa hlavne pri debugovaní.
 * @param t Typ, ktorý vypisujeme.
 * @param out Kam ho vypíšeme, napr. stderr.
 */
void type_print(const Type *t, FILE *out);

/**
 * @brief Uvoľní všetky vytvorené typy naraz.
 *
 * Voláme ju až úplne na konci, keď už typy nepoužíva AST ani tabuľka symbolov.
 */
void type_free_all(void);

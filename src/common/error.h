/**
 * @file error.h
 * @brief Návratové kódy a hlásenie chýb prekladača
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

/** @brief Návratové kódy prekladača (sekcia 6 špecifikácie). */
typedef enum {
    ERR_OK = 0,
    ERR_LEX = 1,        /**< lexikálna chyba */
    ERR_SYNTAX = 2,     /**< syntaktická chyba */
    ERR_UNDEF = 3,      /**< nedefinovaná funkcia/premenná/typ, chýba \@main */
    ERR_REDEF = 4,      /**< nepovolená redefinícia */
    ERR_PARAMS = 5,     /**< zlý počet/typ argumentov, parametre \@main */
    ERR_TYPE = 6,       /**< typová kompatibilita / odvodenie typu */
    ERR_IMMUTABLE = 7,  /**< zápis do nemodifikovateľnej premennej */
    ERR_SEM_OTHER = 10, /**< ostatné sémantické chyby */
    ERR_INTERNAL = 99   /**< interná chyba prekladača */
} ErrorCode;

/**
 * @brief Vypíše chybové hlásenie na stderr a ukončí prekladač.
 * @param code Návratový kód, s ktorým prekladač skončí.
 * @param fmt  Formátovací reťazec (ako pri printf).
 */
_Noreturn void error_exit(ErrorCode code, const char *fmt, ...);

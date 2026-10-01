/**
 * @file symtable.h
 * @brief Tabuľka symbolov (ADT)
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

#include "bst.h"

/** 
 * @name štruktúra pre funkcie
 * @param *func_name: meno funkcie
 * @param *func_inputs: vstupy funkcie
 * @param *return_value: návratová hodnota (od nej sa odvíja dátový typ funkcie)
*/
typedef struct Functions{
    char *func_name;
    char *func_inputs;
    char *return_value;
    struct Functions *left;
    struct Functions *right;   
}Functions;

/** 
 * @name štruktúra pre variables (menné premenné)
 * @param *var_dataType: dátový typ premennej
 * @param *var_name: meno premennej 
 * @param *var_value: hodnota premennej 
*/


typedef struct Variables{
    char *var_dataType;
    char *var_name;
    char *var_value;
    struct Variables *left;
    struct Variables *right;
}Variables;

/** 
 * @name štruktúra pre values (nemenné premenné)
 * @param *val_dataType: dátový typ premennej (začína "const") 
 * @param *val_name: meno premennej 
 * @param *val_value: hodnota premennej 
*/
typedef struct Values{
    char *val_dataType;
    char *val_name;
    char *val_value;
    struct Values *left;
    struct Values *right;   
}Values;

/** 
 * @name funkcie pre vytvorenie štruktúry
 * @returns ukazateľ na novo vytvorenú štruktúru
*/
Functions *funcCtor(void);
Variables *varCtor(void);
Values *valCtor(void);

/** 
 * @name funkcie pre pridávanie štruktúry na koniec
 * @param *tail ukazuje na list stromu, kde sa bude pridávať
*/
void *AddFunction (Functions *tail);
void *AddVariable (Variables *tail);
void *AddValue (Values *tail);

/** 
 *@name odstraňuje list zo stromu
 *@param *cur ukazuje na list, ktorý sa odstraňuje
*/
void FunctionDtor(Functions *cur);
void VariableDtor(Variables *cur);
void ValueDtor(Values *cur);

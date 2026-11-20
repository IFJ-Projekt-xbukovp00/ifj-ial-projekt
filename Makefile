# Projekt: Implementace překladače imperativního jazyka IFJ26
# Predmety: IFJ a IAL, FIT VUT v Brne, 2026/2027
#
# Súbor:   Makefile
# Popis:   Preklad projektu (cieľ build je predvolený)
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

CC      ?= gcc
TARGET  := ifj26
OLD     := src/lexer/old_scanner.c
SRC     := $(filter-out $(OLD),$(wildcard src/*.c src/*/*.c))
HDR     := $(wildcard src/*.h src/*/*.h)
OBJ     := $(SRC:.c=.o)
INC     := $(addprefix -I,$(sort $(dir $(SRC) $(HDR))))
CFLAGS  ?= -std=c11 -Wall -Wextra -pedantic -g
CFLAGS  += $(INC)

IC26INT ?= tests/bin/ic26int-linux-$(shell uname -m)

LEXDUMP     := tests/lexer_dump
LEXDUMP_OBJ := src/lexer/scanner.o src/lexer/dfa.o src/common/dynstr.o src/common/error.o \
               src/lexer/keywords.o

UNIT_BST     := tests/unit/test_bst
UNIT_BST_OBJ := src/symtable/bst.o src/common/error.o
UNIT_SYMTABLE     := tests/unit/test_symtable
UNIT_SYMTABLE_OBJ := src/symtable/symtable.o src/symtable/bst.o src/common/error.o
UNIT_BUILTINS     := tests/unit/test_builtins
UNIT_BUILTINS_OBJ := src/semantic/semantic.o src/symtable/symtable.o src/symtable/bst.o \
                     src/common/types.o src/common/error.o
UNIT_SEMANTIC_GLOBAL     := tests/unit/test_semantic_global
UNIT_SEMANTIC_GLOBAL_OBJ := $(UNIT_BUILTINS_OBJ)
UNIT_TYPES     := tests/unit/test_types
UNIT_TYPES_OBJ := src/common/types.o src/common/error.o
UNIT_SEMANTIC_BODY     := tests/unit/test_semantic_body
UNIT_SEMANTIC_BODY_OBJ := $(UNIT_BUILTINS_OBJ)
UNIT_CODEGEN     := tests/unit/test_codegen
UNIT_CODEGEN_OBJ := src/codegen/codegen.o src/common/dynstr.o $(UNIT_BUILTINS_OBJ)

PACK_DIR := pack
PACK_ZIP := xbukovp00.zip

.PHONY: build check build-check format docs test test-lexer test-unit pack clean

build: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

check:
	clang-format --dry-run --Werror $(SRC) $(HDR)
	cppcheck --enable=warning,style --suppress=nullPointerRedundantCheck:src/common/types.c \
	         --error-exitcode=1 --quiet $(INC) -i $(OLD) src/

build-check: check build

format:
	clang-format -i $(SRC) $(HDR)

docs:
	doxygen Doxyfile

test: build $(LEXDUMP)
	IFJ26=./$(TARGET) IC26INT=$(IC26INT) LEXDUMP=./$(LEXDUMP) tests/run_tests.sh

$(LEXDUMP): tests/lexer_dump.c $(LEXDUMP_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

test-lexer: $(LEXDUMP)
	LEXDUMP=./$(LEXDUMP) tests/run_lexer_tests.sh

$(UNIT_BST): tests/unit/test_bst.c $(UNIT_BST_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(UNIT_SYMTABLE): tests/unit/test_symtable.c $(UNIT_SYMTABLE_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(UNIT_BUILTINS): tests/unit/test_builtins.c $(UNIT_BUILTINS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(UNIT_SEMANTIC_GLOBAL): tests/unit/test_semantic_global.c $(UNIT_SEMANTIC_GLOBAL_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(UNIT_TYPES): tests/unit/test_types.c $(UNIT_TYPES_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(UNIT_SEMANTIC_BODY): tests/unit/test_semantic_body.c $(UNIT_SEMANTIC_BODY_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(UNIT_CODEGEN): tests/unit/test_codegen.c $(UNIT_CODEGEN_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

test-unit: $(UNIT_BST) $(UNIT_SYMTABLE) $(UNIT_BUILTINS) $(UNIT_SEMANTIC_GLOBAL) $(UNIT_TYPES) \
           $(UNIT_SEMANTIC_BODY) $(UNIT_CODEGEN)
	./$(UNIT_BST)
	./$(UNIT_SYMTABLE)
	./$(UNIT_BUILTINS)
	./$(UNIT_SEMANTIC_GLOBAL)
	./$(UNIT_TYPES)
	./$(UNIT_SEMANTIC_BODY)
	IC26INT=$(IC26INT) ./$(UNIT_CODEGEN)

pack:
	rm -rf $(PACK_DIR) $(PACK_ZIP)
	mkdir -p $(PACK_DIR)/src
	cp $(SRC) $(HDR) $(PACK_DIR)/src/
	cp Makefile $(PACK_DIR)/
	-cp rozdeleni $(PACK_DIR)/
	-cp rozsireni $(PACK_DIR)/
	-cp docs/dokumentace.pdf $(PACK_DIR)/
	git ls-files tests | grep -v -e '^tests/bin/' -e '^tests/is_it_ok.sh$$' | \
	    xargs cp --parents -t $(PACK_DIR)/
	cd $(PACK_DIR) && zip -rq ../$(PACK_ZIP) .
	rm -rf $(PACK_DIR)

clean:
	rm -f $(TARGET) $(OBJ) $(LEXDUMP) $(UNIT_BST) $(UNIT_SYMTABLE) $(UNIT_BUILTINS) \
	      $(UNIT_SEMANTIC_GLOBAL) $(UNIT_TYPES) $(UNIT_SEMANTIC_BODY) $(UNIT_CODEGEN)
	rm -rf docs/doxygen $(PACK_DIR) $(PACK_ZIP) tests/tmp

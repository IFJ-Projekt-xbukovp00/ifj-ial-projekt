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
               src/symtable/symtable.o

PACK_DIR := pack
PACK_ZIP := xbukovp00.zip

.PHONY: build check build-check format docs test test-lexer pack clean

build: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

check:
	clang-format --dry-run --Werror $(SRC) $(HDR)
	cppcheck --enable=warning,style --error-exitcode=1 --quiet $(INC) -i $(OLD) src/

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

pack:
	rm -rf $(PACK_DIR) $(PACK_ZIP)
	mkdir -p $(PACK_DIR)/src
	cp $(SRC) $(HDR) $(PACK_DIR)/src/
	cp Makefile $(PACK_DIR)/
	-cp rozdeleni $(PACK_DIR)/
	-cp rozsireni $(PACK_DIR)/
	-cp docs/dokumentace.pdf $(PACK_DIR)/
	cd $(PACK_DIR) && zip -rq ../$(PACK_ZIP) .
	rm -rf $(PACK_DIR)

clean:
	rm -f $(TARGET) $(OBJ) $(LEXDUMP)
	rm -rf docs/doxygen $(PACK_DIR) $(PACK_ZIP) tests/tmp

# IFJ26 → IFJcode26 prekladač

Projekt z predmetov IFJ a IAL, FIT VUT v Brne, 2026/2027.

Autori: Patrik Bukovský (xbukovp00), Tomáš Szalay (xszalat00),
Mikuláš Gärtner (xgartnm00), Daniel Malček (xmalced00)

## Preklad

```
make            # build
make build-check  # clang-format + cppcheck + build
make format
make pack          # odovzdávaný zip (ploché src/)
máme test.         # testy pre každý modul
máme test-unit.    # jedine unit testy pre každý modul
```

Použitie: `./ifj26 < prog.scala > prog.code`

## Štruktúra

```
src/
├── main.c
├── common/     error, dynstr
├── lexer/      token, scanner
├── parser/     parser, expr (precedenčná analýza), ast
├── symtable/   symtable, bst (vv-BVS)
├── semantic/   semantic
└── codegen/    codegen
```

Includy sa píšu bez cesty (`#include "token.h"`), cesty dodáva Makefile cez `-I`.

## Hodnocení:
- Základ:
- Rozšírenia:
- Bonusové body:

## Rady pre budúce tímy:
- Silne neodporúčame mať metodiku, že ideme robiť všetky rozšírenia a budeme odoberať postupe, ako nám čas dovolí. Prečo? Skončí to tak, že budete robiť aj tak všetky a budete mať chuť si vytrhať vlasy.
- Tím si nájdite už ideálne počas IUSu a buďte BFFs.


## Licencia

Copyright (C) 2026 Patrik Bukovský, Tomáš Szalay, Mikuláš Gärtner, Daniel Malček.
Šírené pod licenciou GNU GPL v3 alebo novšou — viď [LICENSE](LICENSE).

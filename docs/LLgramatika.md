# LL gramatika IFJ26 (pracovná kostra)

> Kostra obsahuje iba štruktúru a zoznam konštrukcií, ktoré musí gramatika pokryť.
> **Pravidlá, LL tabuľku a precedenčnú tabuľku píšete vy.** Zadanie (sekcia 4) zakazuje
> ich prípravu pomocou AI. AI smie váš hotový návrh skontrolovať a hľadať v ňom chyby.
> Rozsah: základ + všetky povinne voliteľné rozšírenia
> (FUNEXP, BOOLTHEN, CONTROL, VALMEM, ALGEBDT, INTERPOLATEDS, LAMBDAS, LISTS, EXTKW, PARAMLISTS).
> Odkazy `§` smerujú na *Specifikace IFJ26 a IFJcode26* (revize z 11. 10. 2026).

## 1. Notácia

Notácia je zvolená tak, aby sa dala prepísať do LaTeXu mechanicky (makrá sú v `latex/gramatika.tex`):

| V tomto súbore | Význam | LaTeX |
| --- | --- | --- |
| `<nazov>` | nonterminál | `\nt{nazov}` |
| `TOK_X` | terminál = typ tokenu z `src/lexer/token.h` | `\tok{TOK\_X}` |
| `ε` | prázdny reťazec | `\eps` |
| `→` | prepisovacie pravidlo | (stĺpec tabuľky) |
| `<expr>` | miesto, kde riadenie preberá precedenčná analýza | `\nt{expr}` |

Pravidlá číslujte priebežne (1, 2, 3, …). Čísla sa potom objavia v bunkách LL tabuľky.

Formát riadku (jedno pravidlo na riadok, alternatívy ako samostatné pravidlá):

```
N. <lava_strana> → symbol symbol symbol
```

## 2. Terminály

Pred písaním pravidiel spíšte, ktoré tokeny posiela lexer parseru. Ide najmä o tokeny,
ktoré nie sú „obyčajné“ (konce riadkov, odsadenie, časti interpolovaného reťazca…).

| Token | Kedy ho lexer vráti | Poznámka |
| --- | --- | --- |
| `TOK_EOF` | na konci vstupného súboru | ukončuje `<program>`, predchádza mu prípadný `TOK_DEDENT` / `TOK_EOL` |
| `TOK_EOL` | na konci logického riadku mimo zátvoriek `()` a `[]` | oddeľovač definícií a príkazov+(ignoruje prázdne riadky) |
| `TOK_INDENT` | pri zväčšení odsadenia na novom riadku | otvára odsadený blok príkazov alebo vetiev `match` |
| `TOK_DEDENT` | pri zmenšení odsadenia späť na predchádzajúcu úroveň | uzatvára blok+(môže prísť viac `TOK_DEDENT` za sebou) |
| `TOK_IDENTIFIER` | meno premennej, funkcie, typu aj kontext slová `to`, `until`, `by` | pri `for` cykle sa `to`/`until`/`by` rozlišujú podľa `lexeme` |
| `TOK_UNDERSCORE` | samostatný znak `_` (ALGEBDT) | zástupný vzor (wildcard) vo vetve `case _` |
| `TOK_AT_MAIN` | je to `@main` | predchádza definícu vstupnej funkcie `def` (na tom istom alebo predchádzajúcom riadku) |
| `TOK_INT_LIT`, `TOK_DOUBLE_LIT`, `TOK_BOOL_LIT` | číselný alebo pravdivostný literál | vystupujú vo výrazoch `<expr>` aj ako vzory vo vetvách `case` |
| `TOK_STRING_LIT`, `TOK_CHAR_LIT` | retazec `"..."` / `"""..."""` alebo znak `'a'` | `TOK_CHAR_LIT` sa mapuje na `LiteralNode` (`LIT_STRING` alebo `LIT_INT`) |
| `TOK_INTERP_STRING` | interpolovaný retazec `s"..."` (INTERPOLATEDS) | parser aleb/lexer rozloží časti reťazca a vnútorné výrazy do `InterpNode` |
| `TOK_1LINE_COMMENT`, `TOK_MULTI_LINE_COMMENT` | jednoriadkový alebo viacriadkový komentár | zahadzuje ich lexer (alebo`next_token()`) |
| kľúčové slová (základné) (`TOK_KW_DEF`, `DO`, `DOUBLE`, `ELSE`, `IF`, `IMPORT`, `INT`, `RETURN`, `STRING`, `THEN`, `UNIT`, `VAL`, `VAR`, `WHILE`) | rezervácia pre slová základného jazyka | tvorý kostru LL pravidiel (deklarácie, typy, tok riaden) |
| kľúčové slová (rozšírené) (`TOK_KW_BOOLEAN`, `CASE`, `CLASS`, `EXTENSION`, `FOR`, `LIST`, `MATCH`, `TYPE`) | rezervovane  slová pre rozšírenia | tvorý LL pravidlá pre rozš `ALGEBDT`, `EXTKW`, `CONTROL`, `LISTS`, `BOOLTHEN` |
| operátory v LL pravd (`=`, `+=`, `-=`, `*=`, `/=`, `=>`, `<-`, `\|`, `.`, `*`) | priradenia, šípky, bodka, zjednotenie, `*` v prologu | použíjú sa priamo v LL gramatike (`*` sa zdieľa aj s `<expr>`!!) |
| operátory len pre `<expr>` nezdiel (`+`, `-`, `/`, `%`, `++`, `--`, `->`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `!`, `&&`, `\|\|`) | aritmetika, zoznam, relaci a logi operátory | rieši výhradne/iba expr analýza (`expr.c`); LL pars ich nerobý |
| zátvorky a interpunkcia (`(`, `)`, `[`, `]`, `,`, `:`, `;`) | oddeľovanie zoznamov, typeov a príkazou | `()`+`[]` sa používajú v LL pravidlách aj pri `<expr>` |

## 3. Pravidlá

### 3.1 Program, prológ, definície na najvyššej úrovni

Pokryť:
- prológ `import ifj26.*` (§2)
- postupnosť definícií za prológom (§2)
- anotácia `@main` na samostatnom riadku aj na riadku s `def` (§2)
- `type` alias (ALGEBDT, §5.5)
- `case class` (ALGEBDT, §5.5)
- `extension (p: T)` s jednou alebo viacerými metódami (EXTKW, §5.9)

```
1.  <program> → <prolog> <top_lvl_list> TOK_EOF
2.  <prolog> → TOK_KW_IMPORT TOK_IDENTIFIER TOK_DOT TOK_MULTIPLY TOK_EOL

3.  <top_lvl_list> → <top_lvl_def> <top_lvl_list>
4.  <top_lvl_list> → ε

5.  <top_lvl_def> → TOK_AT_MAIN <main_sepr> <func_def>
6.  <top_lvl_def> → <func_def>
7.  <top_lvl_def> → <type_alias>
8.  <top_lvl_def> → <case_class>
9.  <top_lvl_def> → <extens_def>

10. <main_sepr> → TOK_EOL
11. <main_sepr> → ε

12. <type_alias> → TOK_KW_TYPE TOK_IDENTIFIER TOK_ASSIGN <type> TOK_EOL

13. <case_class> → TOK_KW_CASE TOK_KW_CLASS TOK_IDENTIFIER <case_parms_opti> TOK_EOL
14. <case_parms_opti> → TOK_LPAREN <param_list_opt> TOK_RPAREN
15. <case_parms_opti> → ε

16. <extens_def> → TOK_KW_EXTENSION TOK_LPAREN <param> TOK_RPAREN <ext_body>
17. <ext_body> → TOK_EOL TOK_INDENT <ext_method_list> TOK_DEDENT
18. <ext_body> → <func_def>
19. <ext_method_list> → <func_def> <ext_method_tail>
20. <ext_method_tail> → <func_def> <ext_method_tail>
21. <ext_method_tail> → ε
```

### 3.2 Definícia funkcie

Pokryť:
- jednoriadkové aj viacriadkové telo (§2.3)
- nepovinný návratový typ (§2.3)
- parametre `id : typ`, koncová čiarka (§1.2)
- funkcia bez zoznamu parametrov a s viacerými zoznamami (PARAMLISTS, §5.10)
- bezparametrické metódy v `extension` (EXTKW, §5.9)

```
TODO
```

### 3.3 Typy

Pokryť:
- `Int`, `Double`, `String`, `Unit` (§1.3), `Boolean` (BOOLTHEN, §5.2)
- sjednotenie `T1 | T2` a meno aliasu (ALGEBDT, §5.5)
- funkčný typ `(T1, T2) => R`, `() => R` (LAMBDAS, §5.7)
- `List[T]` (LISTS, §5.8)

```
36. <type> → <prim_type> <union_tail>
37. <union_tail> → TOK_PIPE <prim_type> <union_tail>
38. <union_tail> → ε

39. <prim_type> → TOK_KW_INT
40. <prim_type> → TOK_KW_DOUBLE
41. <prim_type> → TOK_KW_STRING
42. <prim_type> → TOK_KW_UNIT
43. <prim_type> → TOK_KW_BOOLEAN
44. <prim_type> → TOK_IDENTIFIER
45. <prim_type> → TOK_KW_LIST TOK_LBRACKET <type> TOK_RBRACKET
46. <type> → TOK_LPAREN <type_list_opt> TOK_RPAREN TOK_ARROW <type>

47. <type_list_opt> → <type> <type_list_tail>
48. <type_list_opt> → ε
49. <type_list_tail> → TOK_COMMA <type> <type_list_tail>
50. <type_list_tail> → ε
```

### 3.4 Blok a oddeľovače príkazov

Pokryť:
- blok vymedzený odsadením, neprázdny (§1.2, §2.1)
- oddeľovanie koncom riadku a `;` (§1.2)
- posledný príkaz bez oddeľovača (§1.2)

```
51. <block> → TOK_INDENT <statem_list> TOK_DEDENT

52. <statem_list> → <statem> <statem_list_tail>
53. <statem_list_tail> → <statem_sep> <statem_list_after_sep>
54. <statem_list_tail> → ε

55. <statem_list_after_sep> → <statem> <statem_list_tail>
56. <statem_list_after_sep> → ε

57. <statem_sep> → TOK_EOL
58. <statem_sep> → TOK_SEMICOLON <opt_eol>
59. <opt_eol> → TOK_EOL
60. <opt_eol> → ε
```

### 3.5 Príkazy

Pokryť:
- `val` / `var` s nepovinným typom (§2.2)
- priradenie `id = …` (§2.2)
- volanie funkcie ako príkaz (§2.3)
- `return` s hodnotou aj bez nej (§2.3)
- samostatný výraz ako príkaz (§2.1)

```
61. <statem> → <var_decl>
62. <statem> → <return_statem>
63. <statem> → <while_statem>
64. <statem> → <for_statem>
65. <statem> → TOK_IDENTIFIER <id_statem_tail>
66. <statem> → <expr_non_id>

67. <id_statem_tail> → TOK_ASSIGN <expr>
68. <id_statem_tail> → <expr_after_id>

74. <var_decl> → <var_kind> TOK_IDENTIFIER <var_type_opt> TOK_ASSIGN <expr>
75. <var_kind> → TOK_KW_VAL
76. <var_kind> → TOK_KW_VAR
77. <var_type_opt> → TOK_COLON <type>
78. <var_type_opt> → ε

79. <return_statem> → TOK_KW_RETURN <return_val_opt>
80. <return_val_opt> → <expr>
81. <return_val_opt> → ε
```

### 3.6 Riadiace štruktúry

Pokryť:
- `if … then … else …` s povinným `else` (§2.4)
- `if` bez `else`, `else if` (BOOLTHEN, §5.2)
- `if` ako výraz, jednoriadkové telá vetiev (CONTROL, §5.3)
- `while … do` (§2.5)
- `for` s jedným generátorom a strážami, jednoriadkový aj viacriadkový zápis (CONTROL, §5.3)
- zdroje generátora `to`, `until`, `by` (CONTROL, §5.3) a `List[T]` (LISTS, §5.8)

```
82. <if_expr> → TOK_KW_IF <expr> TOK_KW_THEN <branch_body> <else_part>

83. <branch_body> → TOK_EOL <block>
84. <branch_body> → <statem>

85. <else_part> → TOK_KW_ELSE <branch_body>
86. <else_part> → ε

87. <while_statem> → TOK_KW_WHILE <expr> TOK_KW_DO <branch_body>

88. <for_statem> → TOK_KW_FOR <for_header> TOK_KW_DO <branch_body>
89. <for_header> → TOK_INDENT <for_gen> <for_guards_ml> TOK_DEDENT
90. <for_header> → <for_gen> <for_guard_sl>

91. <for_gen> → TOK_IDENTIFIER TOK_LARROW <for_source>
92. <for_source> → <expr> <range_tail_opt>
93. <range_tail_opt> → TOK_IDENTIFIER <expr> <range_step_opt>
94. <range_tail_opt> → ε
95. <range_step_opt> → TOK_IDENTIFIER <expr>
96. <range_step_opt> → ε

97. <for_guard_sl> → TOK_KW_IF <expr> <for_guard_sl>
98. <for_guard_sl> → ε

99.  <for_guards_ml> → TOK_EOL <for_guard_ml_opt>
100. <for_guards_ml> → ε
101. <for_guard_ml_opt> → TOK_KW_IF <expr> <for_guards_ml>
102. <for_guard_ml_opt> → ε
```

### 3.7 `match` (ALGEBDT, §5.5)

Pokryť:
- `výraz match` + odsadené vetvy `case`
- vzory: literál, `id: typ`, konštruktor `Point(a, b)`, `_`
- nepovinná stráž `if …`

```
103. <match_block> → TOK_KW_MATCH TOK_EOL TOK_INDENT <case_list> TOK_DEDENT

104. <case_list> → <case_branch> <case_list_tail>
105. <case_list_tail> → <case_branch> <case_list_tail>
106. <case_list_tail> → ε

107. <case_branch> → TOK_KW_CASE <pattern> <guard_opt> TOK_ARROW <branch_body> <case_sep_opt>
108. <case_sep_opt> → TOK_EOL
109. <case_sep_opt> → ε

110. <guard_opt> → TOK_KW_IF <expr>
111. <guard_opt> → ε

112. <pattern> → TOK_UNDERSCORE
113. <pattern> → <literal>
114. <pattern> → TOK_MINUS <num_literal>
115. <pattern> → TOK_IDENTIFIER <id_pattern_tail>

116. <id_pattern_tail> → TOK_COLON <type>
117. <id_pattern_tail> → TOK_LPAREN <ctor_binders_opt> TOK_RPAREN

119. <ctor_binders_opt> → <ctor_binder> <ctor_binders_tail>
120. <ctor_binders_opt> → ε
121. <ctor_binders_tail> → TOK_COMMA <ctor_binder> <ctor_binders_tail>
122. <ctor_binders_tail> → ε
123. <ctor_binder> → TOK_IDENTIFIER
```

### 3.8 Volania, argumenty a členy

Pokryť:
- `id(argumenty)` a v základe argumenty iba ako termy (§2.3)
- výrazy ako argumenty a vnorené volania (FUNEXP, §5.1)
- viac zoznamov argumentov `f(a)(b)` (PARAMLISTS, §5.10)
- členy `výraz.id` a reťazenie (VALMEM, §5.4)
- metódy `x.m(…)` (EXTKW, §5.9; LISTS, §5.8)
- indexovanie `lst(i)`, `List(…)`, `List.range(…)`, `(1 to 5).toList` (LISTS, §5.8)
- lambda `(a: T) => telo`, `() => telo`, telo ako výraz aj blok (LAMBDAS, §5.7)

```
124. <primary> → TOK_IDENTIFIER
125. <primary> → <primary_non_id>
126. <primary_non_id> → <literal>
127. <primary_non_id> → TOK_INTERP_STRING
128. <primary_non_id> → TOK_KW_LIST <list_ctor_tail>
129. <primary_non_id> → <if_expr>   

130. <list_ctor_tail> → TOK_LPAREN <arg_list_opt> TOK_RPAREN

131. <list_ctor_tail> → TOK_DOT TOK_IDENTIFIER TOK_LPAREN <arg_list_opt> TOK_RPAREN

132. <paren_or_lambda> → TOK_RPAREN TOK_ARROW <lambda_body>
133. <paren_or_lambda> → TOK_IDENTIFIER <paren_id_tail>
134. <paren_or_lambda> → <expr_non_id> <range_tail_opt> TOK_RPAREN

135. <paren_id_tail> → TOK_COLON <type> <lambda_params_tail> TOK_RPAREN TOK_ARROW <lambda_body>
136. <paren_id_tail> → <expr_after_id> <range_tail_opt> TOK_RPAREN

137. <lambda_params_tail> → TOK_COMMA <param> <lambda_params_tail>
138. <lambda_params_tail> → ε

139. <lambda_body> → TOK_EOL <block>
140. <lambda_body> → <expr>

141. <postfix_tail> → TOK_DOT TOK_IDENTIFIER <postfix_tail>
142. <postfix_tail> → TOK_LPAREN <arg_list_opt> TOK_RPAREN <postfix_tail>
143. <postfix_tail> → <match_block>
144. <postfix_tail> → ε

145. <arg_list_opt> → <expr> <arg_list_tail>
146. <arg_list_opt> → ε
147. <arg_list_tail> → TOK_COMMA <arg_after_comma>
148. <arg_list_tail> → ε
149. <arg_after_comma> → <expr> <arg_list_tail>
150. <arg_after_comma> → ε

151. <literal> → <num_literal>
153. <literal> → TOK_STRING_LIT
154. <literal> → TOK_BOOL_LIT

155. <num_literal> → TOK_INT_LIT
156. <num_literal> → TOK_DOUBLE_LIT

157. <primary_non_id> → TOK_LPAREN <paren_or_lambda>
```

### 3.9 Hranica s precedenčnou analýzou

Výrazy (vrátane pravdivostných) musí spracovať **precedenčná analýza** (zadanie §2.2).
Zapíšte, v ktorých miestach gramatiky sa odovzdáva riadenie na `<expr>`, podľa čoho
parser pozná koniec výrazu a čo sa stane s volaním funkcie, členom alebo lambdou vo vnútri výrazu.

```
1.Miesta odovzdania riadenia na `<expr>`:
    LL parser odovzdáva riadenie precedenčnej analýze (`parse_expr`/`expr.c`) v bodoch:
    - telo jednoriadkovej funkcie za `=` (pravidlo 35),
    - inicializácia v `val` / `var` za `=` (pravidlo 74) a pravá strana priradenia za `=`, `+=`, `-=`, `*=`, `/=` (pravidlo 67),
    - návratova hodnota daná za `return` (pravidlo 80),
    - podmienky nahcadz sa v `if <expr> then` (pravidlo 82), `while <expr> do` (pravidlo 87) a `if <expr>` pre `for` a `case` (pravidlá 97, 101, 110),
    - zdroj rozsahu/zozn v `for` za `<-` a za `to`/`until`/`by` (pravidlá 92, 93, 95),
    - sám ten výraz ako príkaz (pravidlá 66, 68), argumenti volaní (pravidlá 145, 149) a telo lambdy (pravidlo 140).
2.Ukončenie výrazu (stop(ping)-tokeny):
    Spracovávnie operandov a operátory dovtedy, až kým nenarazíme na token ktory nepatrí medzi bin/unárne operátory ani postfix pokračovanie (`.` / `(` / `match`) na aktuál úrovni  závoriek. Typicky s ukončovacím tokenmi vo `FOLLOW(<expr>)`:
    - oddeľovače príkazov a konstruktov: `TOK_EOL`, `TOK_SEMICOLON`, `TOK_DEDENT`,
    - kľúčové slová (KWcka) uvádzajúc ďalšiu časť konštrukcie: `TOK_KW_THEN`, `TOK_KW_DO`, `TOK_KW_ELSE`, `TOK_KW_IF` (stráž), `TOK_KW_CASE`, `TOK_ARROW` (`=>`),
    - kontextové slová rozsahu v `for`: `TOK_IDENTIFIER` s lexémy `"to"`, `"until"` alebo `"by"`,
    - oddeľovanie zoznamov a zátvorky: `TOK_COMMA`, `TOK_RPAREN`, `TOK_RBRACKET`.
    Ukončovací token zostáva v prúde ako aktuálny token. Pre nadradenú LL funkciu.
3.Komplexné konštrukcie vo vnútri výrazu (`FUNEXP`, `VALMEM`, `LAMBDAS`, `ALGEBDT`, `CONTROL`):
    Keď čakávame primárny operand (alebo postfixové pokračovanie) (za operandom), zavoláme LL funkcie pre `<primary>` a `<postfix_tail>` (pravidlá 124–144). Tým si bez zmeny precedencnej tabuľky spracujeme:
    - vnorené funkci /a volania `f(g(x))` a reťazové funkc /a volania `f(a)(b)` (`CallNode`),
    - prístup k členom aj metódam `obj.field`, `(1 to 5).toList` (`MemberNode`, `CallNode`),
    - lambdy `(x: Int) => x + 1` (`LambdaNode`), vnorené `if ... then ... else ...` (`IfNode`) a `x match ...` (`MatchNode`).
    Pri `TOK_INTERP_STRING` sa tu text rozdelí na literál časti (`LiteralNode`) a vnútor `${...}`. Výrazy sa rekurzivne odovzdajú spat na `<expr>` potom výsledok sa zabalí do `InterpNode`.
```

## 4. Množiny FIRST, FOLLOW, PREDICT

Vyplňte pre každý nonterminál a pravidlo. Z množín PREDICT vznikne LL tabuľka.

| Nonterminál | FIRST | FOLLOW |
| --- | --- | --- |
| | | |

| Pravidlo | PREDICT |
| --- | --- |
| | |

## 5. LL tabuľka

Riadky sú nonterminály, stĺpce terminály, bunka obsahuje číslo pravidla.
Pre dokumentáciu je kostra tabuľky v `latex/gramatika.tex`.

## 6. Precedenčná tabuľka

Operátory podľa špecifikácie (fakty zo zadania, nie tabuľka):

| Priorita (od najvyššej) | Operátory | Asociativita | Zdroj |
| --- | --- | --- | --- |
| — | `!` (vyššia ako `*`) | | BOOLTHEN §5.2 |
| 1 | unárne `+`, `-` | pravá | §3 |
| 2 | `*`, `/` | ľavá | §3 |
| 3 | `+`, `-`, `++` | ľavá | §3, LISTS §5.8 |
| 4 | `<`, `<=`, `>`, `>=` | bez asociativity | §3 |
| 5 | `==`, `!=` | bez asociativity | §3 |
| — | `&&` (nižšia ako relačné) | | BOOLTHEN §5.2 |
| — | `\|\|` (nižšia ako `&&`) | | BOOLTHEN §5.2 |

Vzájomné poradie `!` a unárneho `-` si dohodnite sami podľa správania Scaly.

Samotnú tabuľku (`<`, `>`, `=`, prázdne bunky) vyplňte v `latex/gramatika.tex`.

## 7. Kontrola návrhu (pred odovzdaním na review)

- [ ] žiadna ľavá rekurzia
- [ ] žiadne dve pravidlá s rovnakou ľavou stranou nemajú prienik PREDICT množín
- [ ] každá konštrukcia zo sekcie 3 je pokrytá aspoň jedným pravidlom
- [ ] ukážky z `tests/examples/` sa dajú odvodiť (aspoň ručne skúsiť `basic/factorial.scala`)
- [ ] gramatika a implementácia rekurzívneho zostupu majú rovnaké názvy nonterminálov

Na review pošlite tento súbor (alebo jeho časť). Kontrola nájde konflikty, chýbajúce
konštrukcie a nesúlad so špecifikáciou, ale nebude za vás dopisovať pravidlá.

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
22. <func_def> → TOK_KW_DEF TOK_IDENTIFIER <param_lists> <ret_type_opt> TOK_ASSIGN <func_body>

23. <param_lists> → TOK_LPAREN <param_list_opt> TOK_RPAREN <param_lists>
24. <param_lists> → ε

25. <param_list_opt> → <param> <param_list_tail>
26. <param_list_opt> → ε
27. <param_list_tail> → TOK_COMMA <param_after_comma>
28. <param_list_tail> → ε
29. <param_after_comma> → <param> <param_list_tail>
30. <param_after_comma> → ε

31. <param> → TOK_IDENTIFIER TOK_COLON <type>
32. <ret_type_opt> → TOK_COLON <type>
33. <ret_type_opt> → ε

34. <func_body> → TOK_EOL <block>
35. <func_body> → <expr> TOK_EOL
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

159. <else_part> → TOK_EOL <eol_else_tail>
160. <eol_else_tail> → TOK_KW_ELSE <branch_body>
161. <eol_else_tail> → TOK_INDENT TOK_KW_ELSE <branch_body> <opt_eol> TOK_DEDENT
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

158. <ctor_binder> → TOK_UNDERSCORE
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
    - inicializácia v `val` / `var` za `=` (pravidlo 74) a pravá strana priradenia za `=` (pravidlo 67),
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

## 4. Množiny FIRST, FOLLOW a PREDICT

### FIRST a FOLLOW
| Nonterminál | FIRST | FOLLOW |
| --- | --- | --- |
| `<arg_after_comma>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT, ε` | `TOK_RPAREN` |
| `<arg_list_opt>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT, ε` | `TOK_RPAREN` |
| `<arg_list_tail>` | `TOK_COMMA, ε` | `TOK_RPAREN` |
| `<block>` | `TOK_INDENT` | `TOK_AND, TOK_ARROW, TOK_AT_MAIN, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOF, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DEF, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_EXTENSION, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_KW_TYPE, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<branch_body>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_EOL, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_FOR, TOK_KW_IF, TOK_KW_LIST, TOK_KW_RETURN, TOK_KW_VAL, TOK_KW_VAR, TOK_KW_WHILE, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<case_branch>` | `TOK_KW_CASE` | `TOK_DEDENT, TOK_KW_CASE` |
| `<case_class>` | `TOK_KW_CASE` | `TOK_AT_MAIN, TOK_EOF, TOK_KW_CASE, TOK_KW_DEF, TOK_KW_EXTENSION, TOK_KW_TYPE` |
| `<case_list>` | `TOK_KW_CASE` | `TOK_DEDENT` |
| `<case_list_tail>` | `TOK_KW_CASE, ε` | `TOK_DEDENT` |
| `<case_parms_opti>` | `TOK_LPAREN, ε` | `TOK_EOL` |
| `<case_sep_opt>` | `TOK_EOL, ε` | `TOK_DEDENT, TOK_KW_CASE` |
| `<ctor_binder>` | `TOK_IDENTIFIER, TOK_UNDERSCORE` | `TOK_COMMA, TOK_RPAREN` |
| `<ctor_binders_opt>` | `TOK_IDENTIFIER, TOK_UNDERSCORE, ε` | `TOK_RPAREN` |
| `<ctor_binders_tail>` | `TOK_COMMA, ε` | `TOK_RPAREN` |
| `<else_part>` | `TOK_EOL, TOK_KW_ELSE, ε` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<eol_else_tail>` | `TOK_INDENT, TOK_KW_ELSE` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<ext_body>` | `TOK_EOL, TOK_KW_DEF` | `TOK_AT_MAIN, TOK_EOF, TOK_KW_CASE, TOK_KW_DEF, TOK_KW_EXTENSION, TOK_KW_TYPE` |
| `<ext_method_list>` | `TOK_KW_DEF` | `TOK_DEDENT` |
| `<ext_method_tail>` | `TOK_KW_DEF, ε` | `TOK_DEDENT` |
| `<extens_def>` | `TOK_KW_EXTENSION` | `TOK_AT_MAIN, TOK_EOF, TOK_KW_CASE, TOK_KW_DEF, TOK_KW_EXTENSION, TOK_KW_TYPE` |
| `<for_gen>` | `TOK_IDENTIFIER` | `TOK_DEDENT, TOK_EOL, TOK_KW_DO, TOK_KW_IF` |
| `<for_guard_ml_opt>` | `TOK_KW_IF, ε` | `TOK_DEDENT` |
| `<for_guard_sl>` | `TOK_KW_IF, ε` | `TOK_KW_DO` |
| `<for_guards_ml>` | `TOK_EOL, ε` | `TOK_DEDENT` |
| `<for_header>` | `TOK_IDENTIFIER, TOK_INDENT` | `TOK_KW_DO` |
| `<for_source>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` | `TOK_DEDENT, TOK_EOL, TOK_KW_DO, TOK_KW_IF` |
| `<for_statem>` | `TOK_KW_FOR` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<func_body>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_EOL, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` | `TOK_AT_MAIN, TOK_DEDENT, TOK_EOF, TOK_KW_CASE, TOK_KW_DEF, TOK_KW_EXTENSION, TOK_KW_TYPE` |
| `<func_def>` | `TOK_KW_DEF` | `TOK_AT_MAIN, TOK_DEDENT, TOK_EOF, TOK_KW_CASE, TOK_KW_DEF, TOK_KW_EXTENSION, TOK_KW_TYPE` |
| `<guard_opt>` | `TOK_KW_IF, ε` | `TOK_ARROW` |
| `<id_pattern_tail>` | `TOK_COLON, TOK_LPAREN` | `TOK_ARROW, TOK_KW_IF` |
| `<id_statem_tail>` | `TOK_AND, TOK_ASSIGN, TOK_CONCAT, TOK_DIVIDE, TOK_DOT, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_KW_MATCH, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, ε` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<if_expr>` | `TOK_KW_IF` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<lambda_body>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_EOL, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<lambda_params_tail>` | `TOK_COMMA, ε` | `TOK_RPAREN` |
| `<list_ctor_tail>` | `TOK_DOT, TOK_LPAREN` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<literal>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_INT_LIT, TOK_STRING_LIT` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<main_sepr>` | `TOK_EOL, ε` | `TOK_KW_DEF` |
| `<match_block>` | `TOK_KW_MATCH` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<num_literal>` | `TOK_DOUBLE_LIT, TOK_INT_LIT` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<opt_eol>` | `TOK_EOL, ε` | `TOK_BOOL_LIT, TOK_DEDENT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_FOR, TOK_KW_IF, TOK_KW_LIST, TOK_KW_RETURN, TOK_KW_VAL, TOK_KW_VAR, TOK_KW_WHILE, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` |
| `<param>` | `TOK_IDENTIFIER` | `TOK_COMMA, TOK_RPAREN` |
| `<param_after_comma>` | `TOK_IDENTIFIER, ε` | `TOK_RPAREN` |
| `<param_list_opt>` | `TOK_IDENTIFIER, ε` | `TOK_RPAREN` |
| `<param_list_tail>` | `TOK_COMMA, ε` | `TOK_RPAREN` |
| `<param_lists>` | `TOK_LPAREN, ε` | `TOK_ASSIGN, TOK_COLON` |
| `<paren_id_tail>` | `TOK_AND, TOK_COLON, TOK_CONCAT, TOK_DIVIDE, TOK_DOT, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_MATCH, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<paren_or_lambda>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_RPAREN, TOK_STRING_LIT` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<pattern>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INT_LIT, TOK_MINUS, TOK_STRING_LIT, TOK_UNDERSCORE` | `TOK_ARROW, TOK_KW_IF` |
| `<postfix_tail>` | `TOK_DOT, TOK_KW_MATCH, TOK_LPAREN, ε` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<prim_type>` | `TOK_IDENTIFIER, TOK_KW_BOOLEAN, TOK_KW_DOUBLE, TOK_KW_INT, TOK_KW_LIST, TOK_KW_STRING, TOK_KW_UNIT` | `TOK_ARROW, TOK_ASSIGN, TOK_COMMA, TOK_EOL, TOK_KW_IF, TOK_PIPE, TOK_RBRACKET, TOK_RPAREN` |
| `<primary>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_STRING_LIT` | `` |
| `<primary_non_id>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_STRING_LIT` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<program>` | `TOK_KW_IMPORT` | `TOK_EOF` |
| `<prolog>` | `TOK_KW_IMPORT` | `TOK_AT_MAIN, TOK_EOF, TOK_KW_CASE, TOK_KW_DEF, TOK_KW_EXTENSION, TOK_KW_TYPE` |
| `<range_step_opt>` | `TOK_IDENTIFIER, ε` | `TOK_DEDENT, TOK_EOL, TOK_KW_DO, TOK_KW_IF, TOK_RPAREN` |
| `<range_tail_opt>` | `TOK_IDENTIFIER, ε` | `TOK_DEDENT, TOK_EOL, TOK_KW_DO, TOK_KW_IF, TOK_RPAREN` |
| `<ret_type_opt>` | `TOK_COLON, ε` | `TOK_ASSIGN` |
| `<return_statem>` | `TOK_KW_RETURN` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<return_val_opt>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT, ε` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<statem>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_FOR, TOK_KW_IF, TOK_KW_LIST, TOK_KW_RETURN, TOK_KW_VAL, TOK_KW_VAR, TOK_KW_WHILE, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<statem_list>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_FOR, TOK_KW_IF, TOK_KW_LIST, TOK_KW_RETURN, TOK_KW_VAL, TOK_KW_VAR, TOK_KW_WHILE, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` | `TOK_DEDENT` |
| `<statem_list_after_sep>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_FOR, TOK_KW_IF, TOK_KW_LIST, TOK_KW_RETURN, TOK_KW_VAL, TOK_KW_VAR, TOK_KW_WHILE, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT, ε` | `TOK_DEDENT` |
| `<statem_list_tail>` | `TOK_EOL, TOK_SEMICOLON, ε` | `TOK_DEDENT` |
| `<statem_sep>` | `TOK_EOL, TOK_SEMICOLON` | `TOK_BOOL_LIT, TOK_DEDENT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_FOR, TOK_KW_IF, TOK_KW_LIST, TOK_KW_RETURN, TOK_KW_VAL, TOK_KW_VAR, TOK_KW_WHILE, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` |
| `<top_lvl_def>` | `TOK_AT_MAIN, TOK_KW_CASE, TOK_KW_DEF, TOK_KW_EXTENSION, TOK_KW_TYPE` | `TOK_AT_MAIN, TOK_EOF, TOK_KW_CASE, TOK_KW_DEF, TOK_KW_EXTENSION, TOK_KW_TYPE` |
| `<top_lvl_list>` | `TOK_AT_MAIN, TOK_KW_CASE, TOK_KW_DEF, TOK_KW_EXTENSION, TOK_KW_TYPE, ε` | `TOK_EOF` |
| `<type>` | `TOK_IDENTIFIER, TOK_KW_BOOLEAN, TOK_KW_DOUBLE, TOK_KW_INT, TOK_KW_LIST, TOK_KW_STRING, TOK_KW_UNIT, TOK_LPAREN` | `TOK_ARROW, TOK_ASSIGN, TOK_COMMA, TOK_EOL, TOK_KW_IF, TOK_RBRACKET, TOK_RPAREN` |
| `<type_alias>` | `TOK_KW_TYPE` | `TOK_AT_MAIN, TOK_EOF, TOK_KW_CASE, TOK_KW_DEF, TOK_KW_EXTENSION, TOK_KW_TYPE` |
| `<type_list_opt>` | `TOK_IDENTIFIER, TOK_KW_BOOLEAN, TOK_KW_DOUBLE, TOK_KW_INT, TOK_KW_LIST, TOK_KW_STRING, TOK_KW_UNIT, TOK_LPAREN, ε` | `TOK_RPAREN` |
| `<type_list_tail>` | `TOK_COMMA, ε` | `TOK_RPAREN` |
| `<union_tail>` | `TOK_PIPE, ε` | `TOK_ARROW, TOK_ASSIGN, TOK_COMMA, TOK_EOL, TOK_KW_IF, TOK_RBRACKET, TOK_RPAREN` |
| `<var_decl>` | `TOK_KW_VAL, TOK_KW_VAR` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| `<var_kind>` | `TOK_KW_VAL, TOK_KW_VAR` | `TOK_IDENTIFIER` |
| `<var_type_opt>` | `TOK_COLON, ε` | `TOK_ASSIGN` |
| `<while_statem>` | `TOK_KW_WHILE` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |

### PREDICT
| Pravidlo | Pravidlo (prepis) | PREDICT |
| --- | --- | --- |
| 1 | `<program> → <prolog> <top_lvl_list> TOK_EOF` | `TOK_KW_IMPORT` |
| 2 | `<prolog> → TOK_KW_IMPORT TOK_IDENTIFIER TOK_DOT TOK_MULTIPLY TOK_EOL` | `TOK_KW_IMPORT` |
| 3 | `<top_lvl_list> → <top_lvl_def> <top_lvl_list>` | `TOK_AT_MAIN, TOK_KW_CASE, TOK_KW_DEF, TOK_KW_EXTENSION, TOK_KW_TYPE` |
| 4 | `<top_lvl_list> → ε` | `TOK_EOF` |
| 5 | `<top_lvl_def> → TOK_AT_MAIN <main_sepr> <func_def>` | `TOK_AT_MAIN` |
| 6 | `<top_lvl_def> → <func_def>` | `TOK_KW_DEF` |
| 7 | `<top_lvl_def> → <type_alias>` | `TOK_KW_TYPE` |
| 8 | `<top_lvl_def> → <case_class>` | `TOK_KW_CASE` |
| 9 | `<top_lvl_def> → <extens_def>` | `TOK_KW_EXTENSION` |
| 10 | `<main_sepr> → TOK_EOL` | `TOK_EOL` |
| 11 | `<main_sepr> → ε` | `TOK_KW_DEF` |
| 12 | `<type_alias> → TOK_KW_TYPE TOK_IDENTIFIER TOK_ASSIGN <type> TOK_EOL` | `TOK_KW_TYPE` |
| 13 | `<case_class> → TOK_KW_CASE TOK_KW_CLASS TOK_IDENTIFIER <case_parms_opti> TOK_EOL` | `TOK_KW_CASE` |
| 14 | `<case_parms_opti> → TOK_LPAREN <param_list_opt> TOK_RPAREN` | `TOK_LPAREN` |
| 15 | `<case_parms_opti> → ε` | `TOK_EOL` |
| 16 | `<extens_def> → TOK_KW_EXTENSION TOK_LPAREN <param> TOK_RPAREN <ext_body>` | `TOK_KW_EXTENSION` |
| 17 | `<ext_body> → TOK_EOL TOK_INDENT <ext_method_list> TOK_DEDENT` | `TOK_EOL` |
| 18 | `<ext_body> → <func_def>` | `TOK_KW_DEF` |
| 19 | `<ext_method_list> → <func_def> <ext_method_tail>` | `TOK_KW_DEF` |
| 20 | `<ext_method_tail> → <func_def> <ext_method_tail>` | `TOK_KW_DEF` |
| 21 | `<ext_method_tail> → ε` | `TOK_DEDENT` |
| 22 | `<func_def> → TOK_KW_DEF TOK_IDENTIFIER <param_lists> <ret_type_opt> TOK_ASSIGN <func_body>` | `TOK_KW_DEF` |
| 23 | `<param_lists> → TOK_LPAREN <param_list_opt> TOK_RPAREN <param_lists>` | `TOK_LPAREN` |
| 24 | `<param_lists> → ε` | `TOK_ASSIGN, TOK_COLON` |
| 25 | `<param_list_opt> → <param> <param_list_tail>` | `TOK_IDENTIFIER` |
| 26 | `<param_list_opt> → ε` | `TOK_RPAREN` |
| 27 | `<param_list_tail> → TOK_COMMA <param_after_comma>` | `TOK_COMMA` |
| 28 | `<param_list_tail> → ε` | `TOK_RPAREN` |
| 29 | `<param_after_comma> → <param> <param_list_tail>` | `TOK_IDENTIFIER` |
| 30 | `<param_after_comma> → ε` | `TOK_RPAREN` |
| 31 | `<param> → TOK_IDENTIFIER TOK_COLON <type>` | `TOK_IDENTIFIER` |
| 32 | `<ret_type_opt> → TOK_COLON <type>` | `TOK_COLON` |
| 33 | `<ret_type_opt> → ε` | `TOK_ASSIGN` |
| 34 | `<func_body> → TOK_EOL <block>` | `TOK_EOL` |
| 35 | `<func_body> → <expr> TOK_EOL` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` |
| 36 | `<type> → <prim_type> <union_tail>` | `TOK_IDENTIFIER, TOK_KW_BOOLEAN, TOK_KW_DOUBLE, TOK_KW_INT, TOK_KW_LIST, TOK_KW_STRING, TOK_KW_UNIT` |
| 37 | `<union_tail> → TOK_PIPE <prim_type> <union_tail>` | `TOK_PIPE` |
| 38 | `<union_tail> → ε` | `TOK_ARROW, TOK_ASSIGN, TOK_COMMA, TOK_EOL, TOK_KW_IF, TOK_RBRACKET, TOK_RPAREN` |
| 39 | `<prim_type> → TOK_KW_INT` | `TOK_KW_INT` |
| 40 | `<prim_type> → TOK_KW_DOUBLE` | `TOK_KW_DOUBLE` |
| 41 | `<prim_type> → TOK_KW_STRING` | `TOK_KW_STRING` |
| 42 | `<prim_type> → TOK_KW_UNIT` | `TOK_KW_UNIT` |
| 43 | `<prim_type> → TOK_KW_BOOLEAN` | `TOK_KW_BOOLEAN` |
| 44 | `<prim_type> → TOK_IDENTIFIER` | `TOK_IDENTIFIER` |
| 45 | `<prim_type> → TOK_KW_LIST TOK_LBRACKET <type> TOK_RBRACKET` | `TOK_KW_LIST` |
| 46 | `<type> → TOK_LPAREN <type_list_opt> TOK_RPAREN TOK_ARROW <type>` | `TOK_LPAREN` |
| 47 | `<type_list_opt> → <type> <type_list_tail>` | `TOK_IDENTIFIER, TOK_KW_BOOLEAN, TOK_KW_DOUBLE, TOK_KW_INT, TOK_KW_LIST, TOK_KW_STRING, TOK_KW_UNIT, TOK_LPAREN` |
| 48 | `<type_list_opt> → ε` | `TOK_RPAREN` |
| 49 | `<type_list_tail> → TOK_COMMA <type> <type_list_tail>` | `TOK_COMMA` |
| 50 | `<type_list_tail> → ε` | `TOK_RPAREN` |
| 51 | `<block> → TOK_INDENT <statem_list> TOK_DEDENT` | `TOK_INDENT` |
| 52 | `<statem_list> → <statem> <statem_list_tail>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_FOR, TOK_KW_IF, TOK_KW_LIST, TOK_KW_RETURN, TOK_KW_VAL, TOK_KW_VAR, TOK_KW_WHILE, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` |
| 53 | `<statem_list_tail> → <statem_sep> <statem_list_after_sep>` | `TOK_EOL, TOK_SEMICOLON` |
| 54 | `<statem_list_tail> → ε` | `TOK_DEDENT` |
| 55 | `<statem_list_after_sep> → <statem> <statem_list_tail>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_FOR, TOK_KW_IF, TOK_KW_LIST, TOK_KW_RETURN, TOK_KW_VAL, TOK_KW_VAR, TOK_KW_WHILE, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` |
| 56 | `<statem_list_after_sep> → ε` | `TOK_DEDENT` |
| 57 | `<statem_sep> → TOK_EOL` | `TOK_EOL` |
| 58 | `<statem_sep> → TOK_SEMICOLON <opt_eol>` | `TOK_SEMICOLON` |
| 59 | `<opt_eol> → TOK_EOL` | `TOK_EOL` |
| 60 | `<opt_eol> → ε` | `TOK_BOOL_LIT, TOK_DEDENT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_FOR, TOK_KW_IF, TOK_KW_LIST, TOK_KW_RETURN, TOK_KW_VAL, TOK_KW_VAR, TOK_KW_WHILE, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` |
| 61 | `<statem> → <var_decl>` | `TOK_KW_VAL, TOK_KW_VAR` |
| 62 | `<statem> → <return_statem>` | `TOK_KW_RETURN` |
| 63 | `<statem> → <while_statem>` | `TOK_KW_WHILE` |
| 64 | `<statem> → <for_statem>` | `TOK_KW_FOR` |
| 65 | `<statem> → TOK_IDENTIFIER <id_statem_tail>` | `TOK_IDENTIFIER` |
| 66 | `<statem> → <expr_non_id>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` |
| 67 | `<id_statem_tail> → TOK_ASSIGN <expr>` | `TOK_ASSIGN` |
| 68 | `<id_statem_tail> → <expr_after_id>` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| 74 | `<var_decl> → <var_kind> TOK_IDENTIFIER <var_type_opt> TOK_ASSIGN <expr>` | `TOK_KW_VAL, TOK_KW_VAR` |
| 75 | `<var_kind> → TOK_KW_VAL` | `TOK_KW_VAL` |
| 76 | `<var_kind> → TOK_KW_VAR` | `TOK_KW_VAR` |
| 77 | `<var_type_opt> → TOK_COLON <type>` | `TOK_COLON` |
| 78 | `<var_type_opt> → ε` | `TOK_ASSIGN` |
| 79 | `<return_statem> → TOK_KW_RETURN <return_val_opt>` | `TOK_KW_RETURN` |
| 80 | `<return_val_opt> → <expr>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` |
| 81 | `<return_val_opt> → ε` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| 82 | `<if_expr> → TOK_KW_IF <expr> TOK_KW_THEN <branch_body> <else_part>` | `TOK_KW_IF` |
| 83 | `<branch_body> → TOK_EOL <block>` | `TOK_EOL` |
| 84 | `<branch_body> → <statem>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_FOR, TOK_KW_IF, TOK_KW_LIST, TOK_KW_RETURN, TOK_KW_VAL, TOK_KW_VAR, TOK_KW_WHILE, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` |
| 85 | `<else_part> → TOK_KW_ELSE <branch_body>` | `TOK_KW_ELSE` |
| 86 | `<else_part> → ε` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| 87 | `<while_statem> → TOK_KW_WHILE <expr> TOK_KW_DO <branch_body>` | `TOK_KW_WHILE` |
| 88 | `<for_statem> → TOK_KW_FOR <for_header> TOK_KW_DO <branch_body>` | `TOK_KW_FOR` |
| 89 | `<for_header> → TOK_INDENT <for_gen> <for_guards_ml> TOK_DEDENT` | `TOK_INDENT` |
| 90 | `<for_header> → <for_gen> <for_guard_sl>` | `TOK_IDENTIFIER` |
| 91 | `<for_gen> → TOK_IDENTIFIER TOK_LARROW <for_source>` | `TOK_IDENTIFIER` |
| 92 | `<for_source> → <expr> <range_tail_opt>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` |
| 93 | `<range_tail_opt> → TOK_IDENTIFIER <expr> <range_step_opt>` | `TOK_IDENTIFIER` |
| 94 | `<range_tail_opt> → ε` | `TOK_DEDENT, TOK_EOL, TOK_KW_DO, TOK_KW_IF, TOK_RPAREN` |
| 95 | `<range_step_opt> → TOK_IDENTIFIER <expr>` | `TOK_IDENTIFIER` |
| 96 | `<range_step_opt> → ε` | `TOK_DEDENT, TOK_EOL, TOK_KW_DO, TOK_KW_IF, TOK_RPAREN` |
| 97 | `<for_guard_sl> → TOK_KW_IF <expr> <for_guard_sl>` | `TOK_KW_IF` |
| 98 | `<for_guard_sl> → ε` | `TOK_KW_DO` |
| 99 | `<for_guards_ml> → TOK_EOL <for_guard_ml_opt>` | `TOK_EOL` |
| 100 | `<for_guards_ml> → ε` | `TOK_DEDENT` |
| 101 | `<for_guard_ml_opt> → TOK_KW_IF <expr> <for_guards_ml>` | `TOK_KW_IF` |
| 102 | `<for_guard_ml_opt> → ε` | `TOK_DEDENT` |
| 159 | `<else_part> → TOK_EOL <eol_else_tail>` | `TOK_EOL` |
| 160 | `<eol_else_tail> → TOK_KW_ELSE <branch_body>` | `TOK_KW_ELSE` |
| 161 | `<eol_else_tail> → TOK_INDENT TOK_KW_ELSE <branch_body> <opt_eol> TOK_DEDENT` | `TOK_INDENT` |
| 103 | `<match_block> → TOK_KW_MATCH TOK_EOL TOK_INDENT <case_list> TOK_DEDENT` | `TOK_KW_MATCH` |
| 104 | `<case_list> → <case_branch> <case_list_tail>` | `TOK_KW_CASE` |
| 105 | `<case_list_tail> → <case_branch> <case_list_tail>` | `TOK_KW_CASE` |
| 106 | `<case_list_tail> → ε` | `TOK_DEDENT` |
| 107 | `<case_branch> → TOK_KW_CASE <pattern> <guard_opt> TOK_ARROW <branch_body> <case_sep_opt>` | `TOK_KW_CASE` |
| 108 | `<case_sep_opt> → TOK_EOL` | `TOK_EOL` |
| 109 | `<case_sep_opt> → ε` | `TOK_DEDENT, TOK_KW_CASE` |
| 110 | `<guard_opt> → TOK_KW_IF <expr>` | `TOK_KW_IF` |
| 111 | `<guard_opt> → ε` | `TOK_ARROW` |
| 112 | `<pattern> → TOK_UNDERSCORE` | `TOK_UNDERSCORE` |
| 113 | `<pattern> → <literal>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_INT_LIT, TOK_STRING_LIT` |
| 114 | `<pattern> → TOK_MINUS <num_literal>` | `TOK_MINUS` |
| 115 | `<pattern> → TOK_IDENTIFIER <id_pattern_tail>` | `TOK_IDENTIFIER` |
| 116 | `<id_pattern_tail> → TOK_COLON <type>` | `TOK_COLON` |
| 117 | `<id_pattern_tail> → TOK_LPAREN <ctor_binders_opt> TOK_RPAREN` | `TOK_LPAREN` |
| 119 | `<ctor_binders_opt> → <ctor_binder> <ctor_binders_tail>` | `TOK_IDENTIFIER, TOK_UNDERSCORE` |
| 120 | `<ctor_binders_opt> → ε` | `TOK_RPAREN` |
| 121 | `<ctor_binders_tail> → TOK_COMMA <ctor_binder> <ctor_binders_tail>` | `TOK_COMMA` |
| 122 | `<ctor_binders_tail> → ε` | `TOK_RPAREN` |
| 123 | `<ctor_binder> → TOK_IDENTIFIER` | `TOK_IDENTIFIER` |
| 158 | `<ctor_binder> → TOK_UNDERSCORE` | `TOK_UNDERSCORE` |
| 124 | `<primary> → TOK_IDENTIFIER` | `TOK_IDENTIFIER` |
| 125 | `<primary> → <primary_non_id>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_STRING_LIT` |
| 126 | `<primary_non_id> → <literal>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_INT_LIT, TOK_STRING_LIT` |
| 127 | `<primary_non_id> → TOK_INTERP_STRING` | `TOK_INTERP_STRING` |
| 128 | `<primary_non_id> → TOK_KW_LIST <list_ctor_tail>` | `TOK_KW_LIST` |
| 129 | `<primary_non_id> → <if_expr>` | `TOK_KW_IF` |
| 130 | `<list_ctor_tail> → TOK_LPAREN <arg_list_opt> TOK_RPAREN` | `TOK_LPAREN` |
| 131 | `<list_ctor_tail> → TOK_DOT TOK_IDENTIFIER TOK_LPAREN <arg_list_opt> TOK_RPAREN` | `TOK_DOT` |
| 132 | `<paren_or_lambda> → TOK_RPAREN TOK_ARROW <lambda_body>` | `TOK_RPAREN` |
| 133 | `<paren_or_lambda> → TOK_IDENTIFIER <paren_id_tail>` | `TOK_IDENTIFIER` |
| 134 | `<paren_or_lambda> → <expr_non_id> <range_tail_opt> TOK_RPAREN` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` |
| 135 | `<paren_id_tail> → TOK_COLON <type> <lambda_params_tail> TOK_RPAREN TOK_ARROW <lambda_body>` | `TOK_COLON` |
| 136 | `<paren_id_tail> → <expr_after_id> <range_tail_opt> TOK_RPAREN` | `TOK_AND, TOK_CONCAT, TOK_DIVIDE, TOK_DOT, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_MATCH, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN` |
| 137 | `<lambda_params_tail> → TOK_COMMA <param> <lambda_params_tail>` | `TOK_COMMA` |
| 138 | `<lambda_params_tail> → ε` | `TOK_RPAREN` |
| 139 | `<lambda_body> → TOK_EOL <block>` | `TOK_EOL` |
| 140 | `<lambda_body> → <expr>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` |
| 141 | `<postfix_tail> → TOK_DOT TOK_IDENTIFIER <postfix_tail>` | `TOK_DOT` |
| 142 | `<postfix_tail> → TOK_LPAREN <arg_list_opt> TOK_RPAREN <postfix_tail>` | `TOK_LPAREN` |
| 143 | `<postfix_tail> → <match_block>` | `TOK_KW_MATCH` |
| 144 | `<postfix_tail> → ε` | `TOK_AND, TOK_ARROW, TOK_COMMA, TOK_CONCAT, TOK_DEDENT, TOK_DIVIDE, TOK_DOT, TOK_EOL, TOK_EQUAL, TOK_GREATER, TOK_GREATER_EQUAL, TOK_IDENTIFIER, TOK_KW_CASE, TOK_KW_DO, TOK_KW_ELSE, TOK_KW_IF, TOK_KW_MATCH, TOK_KW_THEN, TOK_LESS, TOK_LESS_EQUAL, TOK_LPAREN, TOK_MINUS, TOK_MODULO, TOK_MULTIPLY, TOK_NOT_EQUAL, TOK_OR, TOK_PLUS, TOK_RARROW, TOK_REMOVE, TOK_RPAREN, TOK_SEMICOLON` |
| 145 | `<arg_list_opt> → <expr> <arg_list_tail>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` |
| 146 | `<arg_list_opt> → ε` | `TOK_RPAREN` |
| 147 | `<arg_list_tail> → TOK_COMMA <arg_after_comma>` | `TOK_COMMA` |
| 148 | `<arg_list_tail> → ε` | `TOK_RPAREN` |
| 149 | `<arg_after_comma> → <expr> <arg_list_tail>` | `TOK_BOOL_LIT, TOK_DOUBLE_LIT, TOK_IDENTIFIER, TOK_INTERP_STRING, TOK_INT_LIT, TOK_KW_IF, TOK_KW_LIST, TOK_LPAREN, TOK_MINUS, TOK_NOT, TOK_PLUS, TOK_STRING_LIT` |
| 150 | `<arg_after_comma> → ε` | `TOK_RPAREN` |
| 151 | `<literal> → <num_literal>` | `TOK_DOUBLE_LIT, TOK_INT_LIT` |
| 153 | `<literal> → TOK_STRING_LIT` | `TOK_STRING_LIT` |
| 154 | `<literal> → TOK_BOOL_LIT` | `TOK_BOOL_LIT` |
| 155 | `<num_literal> → TOK_INT_LIT` | `TOK_INT_LIT` |
| 156 | `<num_literal> → TOK_DOUBLE_LIT` | `TOK_DOUBLE_LIT` |
| 157 | `<primary_non_id> → TOK_LPAREN <paren_or_lambda>` | `TOK_LPAREN` |


## 5. LL(1) tabuľka

| Nonterminál | `TOK_AND` | `TOK_ARROW` | `TOK_ASSIGN` | `TOK_AT_MAIN` | `TOK_BOOL_LIT` | `TOK_COLON` | `TOK_COMMA` | `TOK_CONCAT` | `TOK_DEDENT` | `TOK_DIVIDE` | `TOK_DOT` | `TOK_DOUBLE_LIT` | `TOK_EOF` | `TOK_EOL` | `TOK_EQUAL` | `TOK_GREATER` | `TOK_GREATER_EQUAL` | `TOK_IDENTIFIER` | `TOK_INDENT` | `TOK_INTERP_STRING` | `TOK_INT_LIT` | `TOK_KW_BOOLEAN` | `TOK_KW_CASE` | `TOK_KW_CLASS` | `TOK_KW_DEF` | `TOK_KW_DO` | `TOK_KW_DOUBLE` | `TOK_KW_ELSE` | `TOK_KW_EXTENSION` | `TOK_KW_FOR` | `TOK_KW_IF` | `TOK_KW_IMPORT` | `TOK_KW_INT` | `TOK_KW_LIST` | `TOK_KW_MATCH` | `TOK_KW_RETURN` | `TOK_KW_STRING` | `TOK_KW_THEN` | `TOK_KW_TYPE` | `TOK_KW_UNIT` | `TOK_KW_VAL` | `TOK_KW_VAR` | `TOK_KW_WHILE` | `TOK_LARROW` | `TOK_LBRACKET` | `TOK_LESS` | `TOK_LESS_EQUAL` | `TOK_LPAREN` | `TOK_MINUS` | `TOK_MODULO` | `TOK_MULTIPLY` | `TOK_NOT` | `TOK_NOT_EQUAL` | `TOK_OR` | `TOK_PIPE` | `TOK_PLUS` | `TOK_RARROW` | `TOK_RBRACKET` | `TOK_REMOVE` | `TOK_RPAREN` | `TOK_SEMICOLON` | `TOK_STRING_LIT` | `TOK_UNDERSCORE` |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `<arg_after_comma>` |  |  |  |  | `149` |  |  |  |  |  |  | `149` |  |  |  |  |  | `149` |  | `149` | `149` |  |  |  |  |  |  |  |  |  | `149` |  |  | `149` |  |  |  |  |  |  |  |  |  |  |  |  |  | `149` | `149` |  |  | `149` |  |  |  | `149` |  |  |  | `150` |  | `149` |  |
| `<arg_list_opt>` |  |  |  |  | `145` |  |  |  |  |  |  | `145` |  |  |  |  |  | `145` |  | `145` | `145` |  |  |  |  |  |  |  |  |  | `145` |  |  | `145` |  |  |  |  |  |  |  |  |  |  |  |  |  | `145` | `145` |  |  | `145` |  |  |  | `145` |  |  |  | `146` |  | `145` |  |
| `<arg_list_tail>` |  |  |  |  |  |  | `147` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `148` |  |  |  |
| `<block>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `51` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<branch_body>` |  |  |  |  | `84` |  |  |  |  |  |  | `84` |  | `83` |  |  |  | `84` |  | `84` | `84` |  |  |  |  |  |  |  |  | `84` | `84` |  |  | `84` |  | `84` |  |  |  |  | `84` | `84` | `84` |  |  |  |  | `84` | `84` |  |  | `84` |  |  |  | `84` |  |  |  |  |  | `84` |  |
| `<case_branch>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `107` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<case_class>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `13` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<case_list>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `104` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<case_list_tail>` |  |  |  |  |  |  |  |  | `106` |  |  |  |  |  |  |  |  |  |  |  |  |  | `105` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<case_parms_opti>` |  |  |  |  |  |  |  |  |  |  |  |  |  | `15` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `14` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<case_sep_opt>` |  |  |  |  |  |  |  |  | `109` |  |  |  |  | `108` |  |  |  |  |  |  |  |  | `109` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<ctor_binder>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `123` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `158` |
| `<ctor_binders_opt>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `119` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `120` |  |  | `119` |
| `<ctor_binders_tail>` |  |  |  |  |  |  | `121` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `122` |  |  |  |
| `<else_part>` | `86` | `86` |  |  |  |  | `86` | `86` | `86` | `86` | `86` |  |  | `159` | `86` | `86` | `86` | `86` |  |  |  |  | `86` |  |  | `86` |  | `85` |  |  | `86` |  |  |  | `86` |  |  | `86` |  |  |  |  |  |  |  | `86` | `86` | `86` | `86` | `86` | `86` |  | `86` | `86` |  | `86` | `86` |  | `86` | `86` | `86` |  |  |
| `<eol_else_tail>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `161` |  |  |  |  |  |  |  |  | `160` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<ext_body>` |  |  |  |  |  |  |  |  |  |  |  |  |  | `17` |  |  |  |  |  |  |  |  |  |  | `18` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<ext_method_list>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `19` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<ext_method_tail>` |  |  |  |  |  |  |  |  | `21` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `20` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<extens_def>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `16` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<for_gen>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `91` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<for_guard_ml_opt>` |  |  |  |  |  |  |  |  | `102` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `101` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<for_guard_sl>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `98` |  |  |  |  | `97` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<for_guards_ml>` |  |  |  |  |  |  |  |  | `100` |  |  |  |  | `99` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<for_header>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `90` | `89` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<for_source>` |  |  |  |  | `92` |  |  |  |  |  |  | `92` |  |  |  |  |  | `92` |  | `92` | `92` |  |  |  |  |  |  |  |  |  | `92` |  |  | `92` |  |  |  |  |  |  |  |  |  |  |  |  |  | `92` | `92` |  |  | `92` |  |  |  | `92` |  |  |  |  |  | `92` |  |
| `<for_statem>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `88` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<func_body>` |  |  |  |  | `35` |  |  |  |  |  |  | `35` |  | `34` |  |  |  | `35` |  | `35` | `35` |  |  |  |  |  |  |  |  |  | `35` |  |  | `35` |  |  |  |  |  |  |  |  |  |  |  |  |  | `35` | `35` |  |  | `35` |  |  |  | `35` |  |  |  |  |  | `35` |  |
| `<func_def>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `22` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<guard_opt>` |  | `111` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `110` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<id_pattern_tail>` |  |  |  |  |  | `116` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `117` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<id_statem_tail>` | `68` | `68` | `67` |  |  |  | `68` | `68` | `68` | `68` | `68` |  |  | `68` | `68` | `68` | `68` | `68` |  |  |  |  | `68` |  |  | `68` |  | `68` |  |  | `68` |  |  |  | `68` |  |  | `68` |  |  |  |  |  |  |  | `68` | `68` | `68` | `68` | `68` | `68` |  | `68` | `68` |  | `68` | `68` |  | `68` | `68` | `68` |  |  |
| `<if_expr>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `82` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<lambda_body>` |  |  |  |  | `140` |  |  |  |  |  |  | `140` |  | `139` |  |  |  | `140` |  | `140` | `140` |  |  |  |  |  |  |  |  |  | `140` |  |  | `140` |  |  |  |  |  |  |  |  |  |  |  |  |  | `140` | `140` |  |  | `140` |  |  |  | `140` |  |  |  |  |  | `140` |  |
| `<lambda_params_tail>` |  |  |  |  |  |  | `137` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `138` |  |  |  |
| `<list_ctor_tail>` |  |  |  |  |  |  |  |  |  |  | `131` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `130` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<literal>` |  |  |  |  | `154` |  |  |  |  |  |  | `151` |  |  |  |  |  |  |  |  | `151` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `153` |  |
| `<main_sepr>` |  |  |  |  |  |  |  |  |  |  |  |  |  | `10` |  |  |  |  |  |  |  |  |  |  | `11` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<match_block>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `103` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<num_literal>` |  |  |  |  |  |  |  |  |  |  |  | `156` |  |  |  |  |  |  |  |  | `155` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<opt_eol>` |  |  |  |  | `60` |  |  |  | `60` |  |  | `60` |  | `59` |  |  |  | `60` |  | `60` | `60` |  |  |  |  |  |  |  |  | `60` | `60` |  |  | `60` |  | `60` |  |  |  |  | `60` | `60` | `60` |  |  |  |  | `60` | `60` |  |  | `60` |  |  |  | `60` |  |  |  |  |  | `60` |  |
| `<param>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `31` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<param_after_comma>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `29` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `30` |  |  |  |
| `<param_list_opt>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `25` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `26` |  |  |  |
| `<param_list_tail>` |  |  |  |  |  |  | `27` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `28` |  |  |  |
| `<param_lists>` |  |  | `24` |  |  | `24` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `23` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<paren_id_tail>` | `136` |  |  |  |  | `135` |  | `136` |  | `136` | `136` |  |  |  | `136` | `136` | `136` | `136` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `136` |  |  |  |  |  |  |  |  |  |  | `136` | `136` | `136` | `136` | `136` | `136` |  | `136` | `136` |  | `136` | `136` |  | `136` | `136` |  |  |  |
| `<paren_or_lambda>` |  |  |  |  | `134` |  |  |  |  |  |  | `134` |  |  |  |  |  | `133` |  | `134` | `134` |  |  |  |  |  |  |  |  |  | `134` |  |  | `134` |  |  |  |  |  |  |  |  |  |  |  |  |  | `134` | `134` |  |  | `134` |  |  |  | `134` |  |  |  | `132` |  | `134` |  |
| `<pattern>` |  |  |  |  | `113` |  |  |  |  |  |  | `113` |  |  |  |  |  | `115` |  |  | `113` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `114` |  |  |  |  |  |  |  |  |  |  |  |  | `113` | `112` |
| `<postfix_tail>` | `144` | `144` |  |  |  |  | `144` | `144` | `144` | `144` | `141` |  |  | `144` | `144` | `144` | `144` | `144` |  |  |  |  | `144` |  |  | `144` |  | `144` |  |  | `144` |  |  |  | `143` |  |  | `144` |  |  |  |  |  |  |  | `144` | `144` | `142` | `144` | `144` | `144` |  | `144` | `144` |  | `144` | `144` |  | `144` | `144` | `144` |  |  |
| `<prim_type>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `44` |  |  |  | `43` |  |  |  |  | `40` |  |  |  |  |  | `39` | `45` |  |  | `41` |  |  | `42` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<primary>` |  |  |  |  | `125` |  |  |  |  |  |  | `125` |  |  |  |  |  | `124` |  | `125` | `125` |  |  |  |  |  |  |  |  |  | `125` |  |  | `125` |  |  |  |  |  |  |  |  |  |  |  |  |  | `125` |  |  |  |  |  |  |  |  |  |  |  |  |  | `125` |  |
| `<primary_non_id>` |  |  |  |  | `126` |  |  |  |  |  |  | `126` |  |  |  |  |  |  |  | `127` | `126` |  |  |  |  |  |  |  |  |  | `129` |  |  | `128` |  |  |  |  |  |  |  |  |  |  |  |  |  | `157` |  |  |  |  |  |  |  |  |  |  |  |  |  | `126` |  |
| `<program>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `1` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<prolog>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `2` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<range_step_opt>` |  |  |  |  |  |  |  |  | `96` |  |  |  |  | `96` |  |  |  | `95` |  |  |  |  |  |  |  | `96` |  |  |  |  | `96` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `96` |  |  |  |
| `<range_tail_opt>` |  |  |  |  |  |  |  |  | `94` |  |  |  |  | `94` |  |  |  | `93` |  |  |  |  |  |  |  | `94` |  |  |  |  | `94` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `94` |  |  |  |
| `<ret_type_opt>` |  |  | `33` |  |  | `32` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<return_statem>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `79` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<return_val_opt>` | `81` | `81` |  |  | `80` |  | `81` | `81` | `81` | `81` | `81` | `80` |  | `81` | `81` | `81` | `81` | `80` |  | `80` | `80` |  | `81` |  |  | `81` |  | `81` |  |  | `80` |  |  | `80` | `81` |  |  | `81` |  |  |  |  |  |  |  | `81` | `81` | `80` | `80` | `81` | `81` | `80` | `81` | `81` |  | `80` | `81` |  | `81` | `81` | `81` | `80` |  |
| `<statem>` |  |  |  |  | `66` |  |  |  |  |  |  | `66` |  |  |  |  |  | `65` |  | `66` | `66` |  |  |  |  |  |  |  |  | `64` | `66` |  |  | `66` |  | `62` |  |  |  |  | `61` | `61` | `63` |  |  |  |  | `66` | `66` |  |  | `66` |  |  |  | `66` |  |  |  |  |  | `66` |  |
| `<statem_list>` |  |  |  |  | `52` |  |  |  |  |  |  | `52` |  |  |  |  |  | `52` |  | `52` | `52` |  |  |  |  |  |  |  |  | `52` | `52` |  |  | `52` |  | `52` |  |  |  |  | `52` | `52` | `52` |  |  |  |  | `52` | `52` |  |  | `52` |  |  |  | `52` |  |  |  |  |  | `52` |  |
| `<statem_list_after_sep>` |  |  |  |  | `55` |  |  |  | `56` |  |  | `55` |  |  |  |  |  | `55` |  | `55` | `55` |  |  |  |  |  |  |  |  | `55` | `55` |  |  | `55` |  | `55` |  |  |  |  | `55` | `55` | `55` |  |  |  |  | `55` | `55` |  |  | `55` |  |  |  | `55` |  |  |  |  |  | `55` |  |
| `<statem_list_tail>` |  |  |  |  |  |  |  |  | `54` |  |  |  |  | `53` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `53` |  |  |
| `<statem_sep>` |  |  |  |  |  |  |  |  |  |  |  |  |  | `57` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `58` |  |  |
| `<top_lvl_def>` |  |  |  | `5` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `8` |  | `6` |  |  |  | `9` |  |  |  |  |  |  |  |  |  | `7` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<top_lvl_list>` |  |  |  | `3` |  |  |  |  |  |  |  |  | `4` |  |  |  |  |  |  |  |  |  | `3` |  | `3` |  |  |  | `3` |  |  |  |  |  |  |  |  |  | `3` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<type>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `36` |  |  |  | `36` |  |  |  |  | `36` |  |  |  |  |  | `36` | `36` |  |  | `36` |  |  | `36` |  |  |  |  |  |  |  | `46` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<type_alias>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `12` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<type_list_opt>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `47` |  |  |  | `47` |  |  |  |  | `47` |  |  |  |  |  | `47` | `47` |  |  | `47` |  |  | `47` |  |  |  |  |  |  |  | `47` |  |  |  |  |  |  |  |  |  |  |  | `48` |  |  |  |
| `<type_list_tail>` |  |  |  |  |  |  | `49` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `50` |  |  |  |
| `<union_tail>` |  | `38` | `38` |  |  |  | `38` |  |  |  |  |  |  | `38` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `38` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `37` |  |  | `38` |  | `38` |  |  |  |
| `<var_decl>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `74` | `74` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<var_kind>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `75` | `76` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<var_type_opt>` |  |  | `78` |  |  | `77` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| `<while_statem>` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | `87` |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |

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

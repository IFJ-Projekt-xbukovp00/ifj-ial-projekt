# LL gramatika IFJ26 (pracovná kostra)

> Kostra obsahuje iba štruktúru a zoznam konštrukcií, ktoré musí gramatika pokryť.
> **Pravidlá, LL tabuľku a precedenčnú tabuľku píšete vy.** Zadanie (sekcia 4) zakazuje
> ich prípravu pomocou AI. AI smie váš hotový návrh skontrolovať a hľadať v ňom chyby.
>
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
| | | |

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
TODO
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
TODO
```

### 3.4 Blok a oddeľovače príkazov

Pokryť:
- blok vymedzený odsadením, neprázdny (§1.2, §2.1)
- oddeľovanie koncom riadku a `;` (§1.2)
- posledný príkaz bez oddeľovača (§1.2)

```
TODO
```

### 3.5 Príkazy

Pokryť:
- `val` / `var` s nepovinným typom (§2.2)
- priradenie `id = …` (§2.2)
- volanie funkcie ako príkaz (§2.3)
- `return` s hodnotou aj bez nej (§2.3)
- samostatný výraz ako príkaz (§2.1)

```
TODO
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
TODO
```

### 3.7 `match` (ALGEBDT, §5.5)

Pokryť:
- `výraz match` + odsadené vetvy `case`
- vzory: literál, `id: typ`, konštruktor `Point(a, b)`, `_`
- nepovinná stráž `if …`

```
TODO
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
TODO
```

### 3.9 Hranica s precedenčnou analýzou

Výrazy (vrátane pravdivostných) musí spracovať **precedenčná analýza** (zadanie §2.2).
Zapíšte, v ktorých miestach gramatiky sa odovzdáva riadenie na `<expr>`, podľa čoho
parser pozná koniec výrazu a čo sa stane s volaním funkcie, členom alebo lambdou vo vnútri výrazu.

```
TODO
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

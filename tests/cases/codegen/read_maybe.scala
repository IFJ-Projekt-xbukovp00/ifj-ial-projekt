// Tento test vygenerovala AI (Claude).
import ifj26.*

def show(v: Int | Unit): String =
    v match
        case n: Int => toStr(n)
        case _ => "none"

import ifj26.*

@main def main(): Unit =
    val a = readIntMaybe()
    val b = readIntMaybe()
    val sa = show(a)
    val sb = show(b)
    print(sa)
    print(" ")
    print(sb)

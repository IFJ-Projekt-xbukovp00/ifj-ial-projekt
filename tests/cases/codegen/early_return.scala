// Tento test vygenerovala AI (Claude).
import ifj26.*

def firstDivisor(n: Int): Int =
    var d = 2
    while d < n do
        if n / d * d == n then
            return d
        else
            ()
        d = d + 1
    n

import ifj26.*

@main def main(): Unit =
    val a = firstDivisor(91)
    val b = firstDivisor(13)
    print(a)
    print(" ")
    print(b)

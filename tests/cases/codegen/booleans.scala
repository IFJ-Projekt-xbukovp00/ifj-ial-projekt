// Tento test vygenerovala AI (Claude).
import ifj26.*

def side(v: Boolean): Boolean =
    print("s")
    v

import ifj26.*

@main def main(): Unit =
    val a = false && side(true)
    val b = true || side(false)
    val c = true && side(false)
    print(a)
    print(b)
    print(c)
    print(!c)
    print(a == c)

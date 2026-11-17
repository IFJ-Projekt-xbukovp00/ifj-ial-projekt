// Tento test vygenerovala AI (Claude).
import ifj26.*

def applyTo(f: (Int) => Int, x: Int): Int = f(x)

def makeDouble(): (Int) => Int =
    return (a: Int) => a * 2

def inc(a: Int): Int = a + 1

import ifj26.*

@main def main(): Unit =
    val sq = (a: Int) => a * a
    val g: (Int) => Int = inc
    val m = makeDouble()
    val z = () => 42
    val r1 = applyTo(sq, 5)
    val r2 = applyTo(g, 5)
    val r3 = m(7)
    val r4 = applyTo((a: Int) => a - 1, 0)
    val r5 = z()
    print(r1)
    print(" ")
    print(r2)
    print(" ")
    print(r3)
    print(" ")
    print(r4)
    print(" ")
    print(r5)

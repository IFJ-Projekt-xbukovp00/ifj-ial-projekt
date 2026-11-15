// Tento test vygenerovala AI (Claude).
import ifj26.*

extension (v: Int)
    def twice: Int = v * 2
    def plus(o: Int): Int = v + o
    def show(): String = "n" + v.toString

def answer: Int = 42
def add(a: Int)(b: Int)(c: Int): Int = a + b + c

def noisy: Int =
    print("!")
    1

import ifj26.*

@main def main(): Unit =
    val x = 5
    val a = x.twice
    val b = x.plus(3)
    val c = x.show()
    val d = answer
    val e = add(1)(2)(3)
    print(a)
    print(" ")
    print(b)
    print(" ")
    print(c)
    print(" ")
    print(d)
    print(" ")
    print(e)
    print(" ")
    val s = noisy + noisy
    print(s)

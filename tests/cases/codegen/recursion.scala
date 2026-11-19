// Tento test vygenerovala AI (Claude).
import ifj26.*

def fact(n: Int): Int =
    if n <= 1 then
        return 1
    else
        return n * fact(n - 1)

def fib(n: Int): Int =
    if n < 2 then
        return n
    else
        val a = fib(n - 1)
        val b = fib(n - 2)
        return a + b

def sumTo(n: Int): Int =
    if n == 0 then
        return 0
    else
        val rest = sumTo(n - 1)
        return n + rest

import ifj26.*

@main def main(): Unit =
    val f = fact(10)
    val g = fib(15)
    val s = sumTo(100)
    print(f)
    print(" ")
    print(g)
    print(" ")
    print(s)

// Tento test vygenerovala AI (Claude).
import ifj26.*

@main def main(): Unit =
    val a = later(4)
    val b = isEven(10)
    val u = sayX()
    print(a)
    print(" ")
    print(b)
    print(" ")
    print(u)

def later(n: Int): Int = n * n

def sayX(): Unit =
    print("x ")

def isEven(n: Int): Int =
    if n == 0 then
        return 1
    else
        return isOdd(n - 1)

def isOdd(n: Int): Int =
    if n == 0 then
        return 0
    else
        return isEven(n - 1)

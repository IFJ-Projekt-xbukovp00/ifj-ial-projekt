// Tento test vygenerovala AI (Claude).
import ifj26.*

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

@main def main(): Unit =
    val x = isEven(4)
    print(x)

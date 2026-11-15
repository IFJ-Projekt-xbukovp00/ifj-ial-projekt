// Tento test vygenerovala AI (Claude).
import ifj26.*

def sign(x: Int): Int =
    if x < 0 then
        return -1
    else
        return 1

@main def main(): Unit =
    val s = sign(-5)
    print(s)

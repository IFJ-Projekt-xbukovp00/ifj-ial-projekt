// Tento test vygenerovala AI (Claude).
import ifj26.*

def answer: Int = 42
def add(a: Int)(b: Int)(c: Int): Int = a + b + c
def offset()(v: Int): Int = v + 1

@main def main(): Unit =
    val r = offset()(add(answer)(1)(2))
    print(r)

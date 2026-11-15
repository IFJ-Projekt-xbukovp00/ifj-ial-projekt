// Tento test vygenerovala AI (Claude).
import ifj26.*

extension (value: Int)
    def twice: Int = value * 2
    def foo(): String = "foo"
    def plus(other: Int): Int = value + other

@main def main(): Unit =
    val x: Int = 6
    val y = x.twice
    val z = x.foo()
    val w = x.plus(12)
    print(y + w)
    print(z)

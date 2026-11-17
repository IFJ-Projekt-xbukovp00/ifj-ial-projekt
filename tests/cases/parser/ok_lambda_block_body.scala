// Tento test vygenerovala AI (Claude).
import ifj26.*

@main def main(): Unit =
    val f: (Int) => Int = (a: Int) =>
        val b = a + 1
        b * 2
    val g = () => 1
    print(f(g()))

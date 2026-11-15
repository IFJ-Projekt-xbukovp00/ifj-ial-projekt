// Tento test vygenerovala AI (Claude).
import ifj26.*

case class Point(x: Int, y: Int)

def d(v: Int | String | Point): String =
    v match
        case 0 => "zero"
        case n: Int if n > 0 => "pos"
        case s: String => s
        case Point(a, b) => "point"
        case _ => "neg"

@main def main(): Unit =
    val r = d(0)
    print(r)

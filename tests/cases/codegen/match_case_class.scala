// Tento test vygenerovala AI (Claude).
import ifj26.*

case class Point(x: Int, y: Int)

type Value = Int | String | Point

def describe(v: Value): String =
    v match
        case 0 => "zero"
        case n: Int if n > 0 => "pos"
        case s: String => s
        case Point(a, b) => toStr(a + b)
        case _ => "neg"

import ifj26.*

@main def main(): Unit =
    val a = describe(0)
    val b = describe(5)
    val c = describe("txt")
    val d = describe(Point(2, 3))
    val e = describe(-1)
    val p = Point(4, 6)
    val f = p.x + p.y
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
    print(f)

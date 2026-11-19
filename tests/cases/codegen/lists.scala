// Tento test vygenerovala AI (Claude).
import ifj26.*

@main def main(): Unit =
    val lst = List(1, 2, 3, 4, 5)
    print(lst)
    print("\n")
    val x = lst(2)
    val h = lst.head
    print(x)
    print(" ")
    print(h)
    print("\n")
    print(lst.tail)
    print("\n")
    print(lst.drop(2))
    print("\n")
    print(lst.take(10))
    print("\n")
    print(lst.slice(1, 3))
    print("\n")
    print(List(1) ++ List(2, 3))
    print("\n")
    print(List.range(0, 10, 3))
    print("\n")
    val e: List[Int] = List()
    print(e)
    print("\n")
    val r: List[Int] = (1 to 3).toList
    print(r)
    print("\n")
    val s = List("a", "b").toString
    print(s)
    print("\n")
    print(List(1.5, 2.0))
    print("\n")
    print(lst.filter((v: Int) => v > 2))
    print("\n")
    print(lst.takeWhile((v: Int) => v < 3))
    print("\n")
    print(lst.dropWhile((v: Int) => v < 3))
    print("\n")
    for
        v <- lst
        if v > 3
    do
        print(v)

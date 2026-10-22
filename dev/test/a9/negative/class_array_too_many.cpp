/* The element count of a class array is an int: an array with more
   elements is rejected instead of overflowing the count (the overflowed
   count was 0 here, and the destructors were silently skipped). */
struct A { int v; ~A() { } };

A g[65536][65536];

int main()
{
    return 0;
}

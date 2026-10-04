/* DWARF gate (C): gdb must stop inside probe() and read a parameter, a local,
   a local struct and a global.  dwarf_gate.bat finds the line to break at by
   the marker comment on the return statement - keep it on that statement. */
int g_counter = 1234;
struct pair { int a; int b; };
static int probe(int arg, struct pair *p)
{
    int local = arg + 1;
    struct pair copy = *p;
    g_counter += local;
    return local + copy.a + copy.b; /* DWARF_C_BREAK */
}
int main(void)
{
    struct pair p = { 7, 9 };
    return probe(41, &p) == 58 ? 0 : 1;
}

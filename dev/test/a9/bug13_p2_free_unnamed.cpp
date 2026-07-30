// BUG-13-P2: C++ allows unnamed parameters in a function DEFINITION (C89
// does not).  Free-function definitions used to reject them with
// expect("identifier"); now, in C++ mode, an unnamed param gets a fresh
// anonymous token id (so gfunc_prolog's sym_push does not index
// table_ident with a negative index and crash).  In C mode the K&R rule
// still rejects unnamed params (covered by the .c regression suite).
int only_second(int, int y) { return y; }        // first param unnamed
int ignore_all(int, int) { return 99; }          // both unnamed

int main()
{
    int a = only_second(123, 7);   // 7 (first arg ignored)
    int b = ignore_all(1, 2);      // 99
    return a * 100 + b - 799;      // 700 + 99 - 799 = 0
}

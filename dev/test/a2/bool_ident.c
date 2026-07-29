/* FEAT-BOOL regression: in C (.c) `bool` is NOT a keyword - it is a
   <stdbool.h> macro or an ordinary identifier.  The C++-only `bool` keyword
   must be demoted to an identifier when lex_c is active, so this plain C
   file using `bool` as a variable name still compiles and runs. */
int main(void)
{
    int bool = 5;      /* bool used as an identifier, legal in C */
    int true = 3;      /* true / false likewise demote in C */
    return bool - true - 2;   /* 5 - 3 - 2 = 0 */
}

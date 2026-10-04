// Inside the braces this is a definition, so the missing default constructor
// must still be diagnosed (the fix must not turn it into a declaration).
struct C { C(int); };
extern "C" {
    C dcl_neg_c;
}
int main() { return 0; }

// [dcl.link]/7 case C: `extern "C" C dcl_c;` is a declaration, so the class
// does not need a default constructor.
struct C { C(int); };
extern "C" C dcl_c;
int main() { return 0; }

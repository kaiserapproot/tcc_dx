// FEAT-4F: pointer / copy-init declarators must NOT trigger the
// implicit default ctor.
// NOTE: local struct reference (P &r = a;) is excluded here: it hangs
// at runtime even without any ctor (pre-existing issue, see the
// "struct local reference" section in the issues doc).
class P {
public:
    int v;
    P() { v = 7; }
};

int main() {
    P a;            /* ctor: v = 7 */
    P *p;
    P c = a;        /* copy-init (memcpy path): v = 7 */
    p = &a;
    p->v += 1;      /* a.v = 8 */
    return p->v + c.v - 15;
}

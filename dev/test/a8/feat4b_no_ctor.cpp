class Plain { public: int v; };
int main() {
    Plain p;        /* no constructor: existing behavior preserved */
    p.v = 42;
    return p.v - 42;
}

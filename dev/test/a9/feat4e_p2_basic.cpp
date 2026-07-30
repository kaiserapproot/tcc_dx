// FEAT-4E-P2: out-of-class dtor definition + block-end auto call
static int dtor_count;

class P {
public:
    int v;
    P() { v = 1; }
    ~P();
};

P::~P() { dtor_count++; }

int main()
{
    {
        P a;
        if (a.v != 1) return 1;
        if (dtor_count != 0) return 2;
    }
    if (dtor_count != 1) return 3;
    return 0;
}

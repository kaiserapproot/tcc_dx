// BUG-9: local reference to a struct must bind (store the address)
class P { public: int v; };

int main()
{
    P a;
    a.v = 7;
    P &r = a;
    r.v += 1;
    return a.v - 8;
}

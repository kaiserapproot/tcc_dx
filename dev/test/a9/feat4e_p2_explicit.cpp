// FEAT-4E-P2: explicit q.~Q() call bound to the out-of-class body
static int dtor_mark;

class Q {
public:
    int v;
    ~Q();
};

Q::~Q() { dtor_mark = 7; }

int main()
{
    Q q;
    q.v = 1;
    q.~Q();
    return dtor_mark == 7 ? 0 : 1;
}

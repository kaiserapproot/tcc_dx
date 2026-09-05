// N6-02 fail-closed: non-trivial destructor 付き型の thread_local は
// 受理しない (N6_02_NONTRIVIAL_DTOR=NOT_IMPLEMENTED).
// 期待: "thread_local object with non-trivial destructor is unsupported in N6-02"
struct P {
    int value;
    P();
    ~P();
};

thread_local P object;

P::P() { value = 1; }
P::~P() { value = 0; }

int main()
{
    return object.value;
}

// NEGATIVE: assigning to a const member OUTSIDE the mem-initializer
// list must still be rejected - the const_member_init.cpp fix strips
// the qualifier only for the mem-init store, not for the body.
struct R {
    const int m_times;
    R(int repeat) : m_times(repeat) {}
    void bump() { m_times = 9; }
};
int main()
{
    R r(1);
    r.bump();
    return 0;
}

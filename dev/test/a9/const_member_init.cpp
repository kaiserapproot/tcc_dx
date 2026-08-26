// A const data member initialized by the ctor's mem-initializer list -
// the RepeatedTest.h/.cpp:13 shape (`const int m_timesRepeat;` with
// `: m_timesRepeat(repeat)`).  Initialization is not assignment, so
// this is legal C++98; assignment elsewhere must still be an error
// (covered by a9/negative/const_member_assign.cpp).
struct R {
    const int m_times;
    int m_sum;
    R(int repeat) : m_times(repeat), m_sum(0) {}
    int times() const { return m_times; }
};
int main()
{
    R r(7);
    if (r.times() != 7)
        return 1;
    if (r.m_times != 7)
        return 2;
    return 0;
}

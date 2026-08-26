// BUG-45: an unqualified call to a STATIC member function of the same
// class, from within ANOTHER member function, BEFORE the static
// member's own out-of-class definition has been parsed - the
// SimpleString.cpp shape (`assign()` calls `frobSize()` before
// `frobSize` is defined later in the file).
//
// BUG-22's this-injection explicitly excludes static members (no
// `this` to pass), and nothing else in the unqualified-identifier
// lookup chain is class-aware for a FUNCTION - so the call fell
// through to C's plain "implicit declaration of function" path
// (untyped, K&R-style), a total ABI mismatch with the real signature.
// The miscompiled call jumped to a raw stack value and crashed
// (confirmed via a VEH diagnostic harness: RIP came back equal to RSP,
// and the fault landed exactly between entering and returning from the
// call - the callee's body never ran at all).
extern "C" int printf(const char *, ...);

struct Box
{
    typedef unsigned long long size_type;
    static const size_type npos;

    Box();
    Box &assign(size_type n, char c);
    size_type length() const { return m_length; }
    size_type capacity() const { return m_capacity; }
    static size_type frobSize(size_type n);

  private:
    size_type m_length;
    size_type m_capacity;
    char *m_data;
};

const Box::size_type Box::npos = size_type(-1);

Box::Box() : m_length(0), m_capacity(0), m_data(0)
{
    // calls assign() which calls frobSize() - BOTH not yet defined at
    // this point in the file, matching SimpleString.cpp's own order
    assign((size_type)3, 'x');
}

Box &Box::assign(size_type n, char c)
{
    // unqualified call to a STATIC member of the SAME class, before
    // frobSize's own definition below - this is the exact BUG-45 shape
    size_type size = frobSize(n + 1);
    if (capacity() < size) {
        char *data = new char[size];
        delete[] m_data;
        m_data = data;
        m_capacity = size;
    }
    for (size_type i = 0; i < n; i++)
        m_data[i] = c;
    m_data[n] = 0;
    m_length = n;
    return *this;
}

Box::size_type Box::frobSize(size_type n)
{
    size_type size = 8;
    while (size < n)
        size *= 2;
    return size;
}

int main()
{
    Box b;

    printf("len=%llu cap=%llu\n", b.length(), b.capacity());
    if (b.length() != 3)
        return 1;
    if (b.capacity() != 8)          // frobSize(4) -> 8
        return 2;
    return 0;
}

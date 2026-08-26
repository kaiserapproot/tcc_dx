// BUG-42: a class defined INSIDE a function whose inline member bodies
// touch members - the SIMPLE_AUTO_PTR pseudo-template macro shape
// (TestRunner.cpp:255).  Inline bodies replay at end of TU, but the
// local tag and its field chain lived on the LOCAL stack and were freed
// with the function ("field not found: m_ptr").  The definition syms
// now go to the global stack (trade-off: the tag name stays visible at
// file scope afterwards - see tpp仕様.md).
struct R {
    int r;
    int go();
};
int R::go()
{
    class AP {
      public:
        AP(int *p = 0) : m_ptr(p) {}
        ~AP() { m_ptr = 0; }
        int *get() const { return m_ptr; }
        int *release()
        {
            int *tmp = m_ptr;
            m_ptr = 0;
            return tmp;
        }
      private:
        int *m_ptr;
    } ap(&r);

    if (ap.get() != &r)
        return 1;
    if (ap.release() != &r || ap.get() != 0)
        return 2;
    return 0;
}
int main()
{
    R x;
    return x.go();
}

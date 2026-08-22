// BUG-32b: a defaulted argument on an OVERLOADED member routes through the
// deferred-conversion path, where the call's ')' has already been consumed
// when the default is replayed.  begin_macro() does not preserve the
// pending token, so the token after the call used to be lost and the
// parser hit the replay's terminator ("';' expected, found <eof>").
// This is the SimpleString::erase(n) shape.
class S {
public:
    typedef char value_type;
    typedef unsigned int size_type;
    typedef char* iterator;
    int m;

    int erase(size_type pos, size_type n = 9);
    iterator erase(iterator pos);
    iterator erase(iterator first, iterator last);

    void resize(size_type n);
};
int S::erase(size_type pos, size_type n) { m = (int)(pos * 100 + n); return m; }
S::iterator S::erase(iterator pos) { m = -1; return pos; }
S::iterator S::erase(iterator first, iterator last) { m = -2; return first; }
void S::resize(size_type n)
{
    erase(n);
}
int after_the_call;
int main()
{
    S s;
    s.m = 0;
    s.resize(3);
    if (s.m != 309)         /* erase(3, 9): the defaulted 9 must survive */
        return 1;
    s.erase((S::size_type)2, (S::size_type)5);
    if (s.m != 205)
        return 2;
    return after_the_call;
}

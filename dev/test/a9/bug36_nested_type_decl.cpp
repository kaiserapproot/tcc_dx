// BUG-36: a NAMED nested type declaration (`struct Inner { ... };`, no
// declarator) must declare a type and nothing else.  The ms-extensions
// fallthrough used to turn it into an anonymous MEMBER of the outer
// class: the outer layout silently grew an Inner-sized field, and a
// same-named member in outer and nested (TestResult::m_mutex vs
// AutoMutexLock::m_mutex) was falsely reported as duplicated.
struct Outer {
    struct Mutex {
        virtual ~Mutex() {}
        int mv;
    };
    struct Inner {
        Inner(Mutex* m) : m_x(m) {}
        ~Inner() {}
        Mutex* m_x;             /* same name as Outer's member */
    };
    Mutex* m_x;
    int tail;
};
/* an untagged anonymous struct member must KEEP working (the extension
   the buggy path was meant for) */
struct Anon {
    struct {
        int a;
        int b;
    };
    int c;
};
int main()
{
    Outer o;
    Outer::Mutex mu;
    Anon x;

    /* layout: Outer must contain ONLY its own members */
    if (sizeof(Outer) != sizeof(void*) + sizeof(int) + 4)   /* +pad */
        return 1;
    o.m_x = &mu;
    o.tail = 7;
    if (o.m_x != &mu || o.tail != 7)
        return 2;

    /* the nested class still works as a type, same-name member intact */
    {
        Outer::Inner in(&mu);
        mu.mv = 5;
        if (in.m_x != &mu || in.m_x->mv != 5)
            return 3;
    }

    /* untagged anonymous member: members flatten into the outer class */
    x.a = 1;
    x.b = 2;
    x.c = 3;
    if (sizeof(Anon) != 12)
        return 4;
    if (x.a + x.b + x.c != 6)
        return 5;
    return 0;
}

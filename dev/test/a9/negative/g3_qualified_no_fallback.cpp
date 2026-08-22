// G3 negative (rev.4 Blocker 2): O::I::T where neither I nor its bases
// declare T must be an error - the qualified lookup must NOT fall back
// to the enclosing O::T.
struct O {
    typedef int T;
    struct I {
    };
};
O::I::T x;

#ifndef N7_CROSS_01C_GUARD_BODY_H
#define N7_CROSS_01C_GUARD_BODY_H

struct V { int x; };

static V make_v_guard(void)
{
    V v;
    v.x = 13;
    return v;
}

static const V g_guard = make_v_guard();

#endif

static int alive;
static int ctor_count;
static int dtor_count;
static int invalid_dtor;
static int double_dtor;
static int log_count;
static int logv[32];
static void *live_objects[32];

static void reset_observation(void)
{
    alive = 0;
    ctor_count = 0;
    dtor_count = 0;
    invalid_dtor = 0;
    double_dtor = 0;
    log_count = 0;
}

static void note_constructed(void *object)
{
    if (alive >= 32) {
        ++invalid_dtor;
        return;
    }
    live_objects[alive] = object;
    ++alive;
    ++ctor_count;
}

static int note_destroyed(void *object)
{
    int i;

    for (i = 0; i < alive; ++i) {
        if (live_objects[i] == object) {
            live_objects[i] = live_objects[alive - 1];
            --alive;
            return 1;
        }
    }
    return 0;
}

struct A {
    int id;

    A(int value)
    {
        id = value;
        note_constructed(this);
    }

    A(const A &other)
    {
        id = other.id;
        note_constructed(this);
    }

    int test()
    {
        return id;
    }

    ~A()
    {
        ++dtor_count;
        if (!note_destroyed(this)) {
            ++invalid_dtor;
            ++double_dtor;
        }
        if (log_count < 32)
            logv[log_count++] = id;
    }
};

static int check_outward(void)
{
    int result;

    reset_observation();
    result = 0;
    {
        A a(1);
        goto n4_g1_out;
    }
n4_g1_out:
    if (alive != 0 || dtor_count != 1 || log_count != 1 || logv[0] != 1)
        result = 1;
    if (invalid_dtor != 0 || double_dtor != 0 || ctor_count != dtor_count)
        result = 2;
    return result;
}

static int check_nested_order(void)
{
    int result;

    reset_observation();
    result = 0;
    {
        A outer(2);
        {
            A inner(3);
            goto n4_g2_out;
        }
    }
n4_g2_out:
    if (alive != 0 || dtor_count != 2 || log_count != 2)
        result = 1;
    if (logv[0] != 3 || logv[1] != 2)
        result = 2;
    if (invalid_dtor != 0 || double_dtor != 0 || ctor_count != dtor_count)
        result = 3;
    return result;
}

static int check_same_scope_forward(void)
{
    int result;

    reset_observation();
    result = 0;
    {
        A a(4);
        goto n4_g3_same;
    n4_g3_same:
        if (alive != 1 || dtor_count != 0 || invalid_dtor != 0)
            result = 1;
        if (a.test() != 4)
            result = 2;
    }
    if (alive != 0 || dtor_count != 1 || log_count != 1 || logv[0] != 4)
        result = 3;
    if (invalid_dtor != 0 || double_dtor != 0 || ctor_count != dtor_count)
        result = 4;
    return result;
}

static int check_same_scope_backward(void)
{
    int result;
    int i;

    reset_observation();
    result = 0;
    {
        A a(5);
        i = 0;
    n4_g4_loop:
        ++i;
        if (i < 3)
            goto n4_g4_loop;
        if (alive != 1 || dtor_count != 0 || a.test() != 5)
            result = 1;
    }
    if (alive != 0 || dtor_count != 1 || log_count != 1 || logv[0] != 5)
        result = 2;
    if (invalid_dtor != 0 || double_dtor != 0 || ctor_count != dtor_count)
        result = 3;
    return result;
}

static int check_backward_inner_exit(void)
{
    int result;
    int i;

    reset_observation();
    result = 0;
    i = 0;
n4_g5_loop:
    {
        A a(10 + i);
        ++i;
        if (i < 3)
            goto n4_g5_loop;
    }
    if (alive != 0 || dtor_count != 3 || log_count != 3)
        result = 1;
    if (logv[0] != 10 || logv[1] != 11 || logv[2] != 12)
        result = 2;
    if (invalid_dtor != 0 || double_dtor != 0 || ctor_count != dtor_count)
        result = 3;
    return result;
}

static int check_target_scope_preserved(void)
{
    int result;
    int i;

    reset_observation();
    result = 0;
    i = 0;
    {
        A outside(6);
        n4_g6_preserve:
        {
            A inside(60 + i);
            ++i;
            if (i < 2)
                goto n4_g6_preserve;
        }
        if (alive != 1 || dtor_count != 2 || outside.test() != 6)
            result = 1;
    }
    if (alive != 0 || ctor_count != 3 || dtor_count != 3 || log_count != 3)
        result = 2;
    if (logv[0] != 60 || logv[1] != 61 || logv[2] != 6)
        result = 3;
    if (invalid_dtor != 0 || double_dtor != 0 || ctor_count != dtor_count)
        result = 4;
    return result;
}

static int check_forward_resolution(void)
{
    int result;

    result = 0;
    goto n4_g7_target;
    result = 1;
n4_g7_target:
    return result;
}

static A make_temp(int value)
{
    A result(value);

    return result;
}

static int check_temp_goto(void)
{
    int result;

    reset_observation();
    result = 0;
    {
        A local(80);
        if (make_temp(90).test() != 90)
            result = 1;
        if (alive != 1 || ctor_count != 3 || dtor_count != 2 || log_count != 2 || logv[0] != 90 || logv[1] != 90)
            result = 2;
        goto n4_g8_out;
    }
n4_g8_out:
    if (alive != 0 || ctor_count != 3 || dtor_count != 3 || log_count != 3 || logv[2] != 80)
        result = 3;
    if (invalid_dtor != 0 || double_dtor != 0 || ctor_count != dtor_count)
        result = 4;
    return result;
}

static int check_forward_skips_later_local(void)
{
    int result;

    reset_observation();
    result = 0;
    {
        goto n4_g9_out;
        A a(9);
    }
n4_g9_out:
    if (alive != 0 || ctor_count != 0 || dtor_count != 0 || log_count != 0)
        result = 1;
    if (invalid_dtor != 0 || double_dtor != 0 || ctor_count != dtor_count)
        result = 2;
    return result;
}

static int check_multiple_forward_ranges_once(int take_first)
{
    int result;

    reset_observation();
    result = 0;
    {
        A a(20);
        if (take_first)
            goto n4_g10_out;
        A b(21);
        goto n4_g10_out;
    }
n4_g10_out:
    if (take_first) {
        if (alive != 0 || ctor_count != 1 || dtor_count != 1
            || log_count != 1 || logv[0] != 20)
            result = 1;
    } else {
        if (alive != 0 || ctor_count != 2 || dtor_count != 2
            || log_count != 2 || logv[0] != 21 || logv[1] != 20)
            result = 2;
    }
    if (invalid_dtor != 0 || double_dtor != 0 || ctor_count != dtor_count)
        result = 3;
    return result;
}

static int check_multiple_forward_ranges(void)
{
    int result;

    result = check_multiple_forward_ranges_once(1);
    if (result)
        return result;
    return check_multiple_forward_ranges_once(0);
}

static int check_same_scope_backward_across_decl(void)
{
    int result;
    int i;

    reset_observation();
    result = 0;
    i = 0;
    {
    n4_g11_loop:
        A a(30 + i);
        ++i;
        if (i < 3)
            goto n4_g11_loop;
        if (alive != 1 || ctor_count != 3 || dtor_count != 2
            || a.test() != 32)
            result = 1;
    }
    if (alive != 0 || ctor_count != 3 || dtor_count != 3
        || log_count != 3)
        result = 2;
    if (logv[0] != 30 || logv[1] != 31 || logv[2] != 32)
        result = 3;
    if (invalid_dtor != 0 || double_dtor != 0 || ctor_count != dtor_count)
        result = 4;
    return result;
}

static int check_sibling_backward(void)
{
    int result;
    int jumped;

    reset_observation();
    result = 0;
    jumped = 0;
    {
        A outside(40);
        {
        n4_g12_target:
            if (jumped)
                goto n4_g12_done;
        }
        {
            A inside(41);
            jumped = 1;
            goto n4_g12_target;
        }
    n4_g12_done:
        if (alive != 1 || dtor_count != 1 || log_count != 1
            || logv[0] != 41 || outside.test() != 40)
            result = 1;
    }
    if (alive != 0 || ctor_count != 2 || dtor_count != 2
        || log_count != 2 || logv[1] != 40)
        result = 2;
    if (invalid_dtor != 0 || double_dtor != 0 || ctor_count != dtor_count)
        result = 3;
    return result;
}

static int check_label_prefix_safe(void)
{
    int result;

    reset_observation();
    result = 0;
    goto n4_g13_entry;
    return 99;
    {
    n4_g13_entry:
        A a(50);
        if (alive != 1 || ctor_count != 1 || dtor_count != 0
            || a.test() != 50)
            result = 1;
    }
    if (alive != 0 || ctor_count != 1 || dtor_count != 1
        || log_count != 1 || logv[0] != 50)
        result = 2;
    if (invalid_dtor != 0 || double_dtor != 0 || ctor_count != dtor_count)
        result = 3;
    return result;
}

static int check_lca_backward(void)
{
    int result;
    int jumped;

    reset_observation();
    result = 0;
    jumped = 0;
    {
        {
        n4_g15_target:
            if (jumped)
                goto n4_g15_done;
        }
        {
            A a(61);
            jumped = 1;
            goto n4_g15_target;
        }
        {
        n4_g15_done:
            A b(62);
            if (alive != 1 || ctor_count != 2 || dtor_count != 1
                || log_count != 1 || logv[0] != 61)
                result = 1;
        }
    }
    if (alive != 0 || ctor_count != 2 || dtor_count != 2
        || log_count != 2 || logv[0] != 61 || logv[1] != 62)
        result = 2;
    if (invalid_dtor != 0 || double_dtor != 0 || ctor_count != dtor_count)
        result = 3;
    return result;
}

static int check_closed_target_vacuous(void)
{
    int result;
    int jumped;

    reset_observation();
    result = 0;
    jumped = 0;
    {
        {
            int x;
        n4_g16_target:
            if (jumped)
                goto n4_g16_done;
        }
        {
            A a(63);
            jumped = 1;
            goto n4_g16_target;
        }
    n4_g16_done:
        if (alive != 0 || ctor_count != 1 || dtor_count != 1
            || log_count != 1 || logv[0] != 63)
            result = 1;
    }
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        result = 2;
    return result;
}

static int check_vacuous_entry(void)
{
    int result;

    result = 0;
    {
        goto n4_g17_ok;
        int x;
    n4_g17_ok:
        result = 0;
    }
    return result;
}
static void n4_seed_symbol_reuse_1(void)
{
    A first(70);
    A second(71);
}

static void n4_seed_symbol_reuse_2(void)
{
    A first(72);
    A second(73);
}

static int n4_run_symbol_reuse(A param)
{
    int result;

    result = 0;
    {
        A local(74);
        goto n4_g18_out;
    }
n4_g18_out:
    if (alive != 1 || dtor_count != 1 || invalid_dtor != 0
        || log_count != 1 || logv[0] != 74 || param.test() != 70)
        result = 1;
    return result;
}

static int check_symbol_reuse(void)
{
    int result;

    reset_observation();
    n4_seed_symbol_reuse_1();
    n4_seed_symbol_reuse_2();
    reset_observation();
    result = 0;
    {
        A arg(70);
        if (n4_run_symbol_reuse(arg) != 0)
            result |= 1;
        if (alive != 1 || ctor_count != 2 || dtor_count != 1
            || invalid_dtor != 0 || double_dtor != 0
            || log_count != 1 || logv[0] != 74)
            result |= 2;
    }
    if (alive != 0 || ctor_count != 2 || dtor_count != 2
        || invalid_dtor != 0 || double_dtor != 0
        || log_count != 2 || logv[0] != 74 || logv[1] != 70
        || ctor_count != dtor_count)
        result |= 4;
    return result;
}
int main(int argc, char **argv)
{
    int case_no;
    char *arg;

    case_no = 0;
    arg = argc > 1 ? argv[1] : 0;
    while (arg && *arg >= '0' && *arg <= '9') {
        case_no = case_no * 10 + *arg - '0';
        ++arg;
    }
    if (arg && *arg)
        case_no = 0;
    if (case_no == 1)
        return check_outward();
    if (case_no == 2)
        return check_nested_order();
    if (case_no == 3)
        return check_same_scope_forward();
    if (case_no == 4)
        return check_same_scope_backward();
    if (case_no == 5)
        return check_backward_inner_exit();
    if (case_no == 6)
        return check_target_scope_preserved();
    if (case_no == 7)
        return check_forward_resolution();
    if (case_no == 8)
        return check_temp_goto();
    if (case_no == 9)
        return check_forward_skips_later_local();
    if (case_no == 10)
        return check_multiple_forward_ranges();
    if (case_no == 11)
        return check_same_scope_backward_across_decl();
    if (case_no == 12)
        return check_sibling_backward();
    if (case_no == 13)
        return check_label_prefix_safe();
    if (case_no == 15)
        return check_lca_backward();
    if (case_no == 16)
        return check_closed_target_vacuous();
    if (case_no == 17)
        return check_vacuous_entry();
    if (case_no == 18)
        return check_symbol_reuse();
    return 99;
}
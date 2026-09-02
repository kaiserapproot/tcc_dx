static int alive;
static int ctor_count;
static int dtor_count;
static int invalid_dtor;
static int double_dtor;
static void *live_objects[64];
static int dtor_log[64];
static int log_count;
static int condition_value;

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
    if (alive >= 64) {
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

struct Owner {
    int id;

    Owner(int value) : id(value)
    {
        note_constructed(this);
    }

    Owner(const Owner &other) : id(other.id)
    {
        note_constructed(this);
    }

    int test()
    {
        return condition_value;
    }

    ~Owner()
    {
        ++dtor_count;
        if (!note_destroyed(this)) {
            ++invalid_dtor;
            ++double_dtor;
        }
        if (log_count < 64)
            dtor_log[log_count++] = id;
        else
            ++invalid_dtor;
    }
};

static Owner make_condition(void)
{
    Owner value(90);

    return value;
}

static int check_while_break(void)
{
    int result;

    reset_observation();
    result = 0;
    {
        Owner outside(1);

        while (1) {
            Owner inside(2);
            break;
        }
        if (alive != 1 || dtor_count != 1 || log_count != 1
            || dtor_log[0] != 2)
            result = 1;
    }
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        return 2;
    if (log_count != 2 || dtor_log[1] != 1)
        return 3;
    return result;
}

static int check_while_continue(void)
{
    int i;
    int steps;
    int result;

    reset_observation();
    i = 0;
    steps = 0;
    result = 0;
    while (i < 3) {
        Owner value(10 + i);
        ++i;
        ++steps;
        if (steps > 5) {
            result = 11;
            break;
        }
        continue;
    }
    if (result)
        return result;
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        return 12;
    if (log_count != 3 || dtor_log[0] != 10 || dtor_log[1] != 11
        || dtor_log[2] != 12)
        return 13;
    return 0;
}

static int check_nested_break(void)
{
    int result;

    reset_observation();
    result = 0;
    {
        Owner outside(1);

        while (1) {
            Owner outer(2);
            {
                Owner inner(3);
                break;
            }
        }
        if (alive != 1 || dtor_count != 2 || log_count != 2
            || dtor_log[0] != 3 || dtor_log[1] != 2)
            result = 21;
    }
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        return 22;
    if (log_count != 3 || dtor_log[2] != 1)
        return 23;
    return result;
}

static int check_nested_continue(void)
{
    int i;
    int result;

    reset_observation();
    i = 0;
    result = 0;
    {
        Owner outside(1);

        while (i < 2) {
            Owner outer(20 + i);
            {
                Owner inner(30 + i);
                ++i;
                continue;
            }
        }
        if (alive != 1 || dtor_count != 4 || log_count != 4
            || dtor_log[0] != 30 || dtor_log[1] != 20
            || dtor_log[2] != 31 || dtor_log[3] != 21)
            result = 31;
    }
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        return 32;
    if (log_count != 5 || dtor_log[4] != 1)
        return 33;
    return result;
}

static int check_for_continue(void)
{
    int steps;
    int result;

    reset_observation();
    steps = 0;
    result = 0;
    for (int i = 0; i < 3; ++i) {
        Owner value(40 + i);
        ++steps;
        if (steps > 5) {
            result = 41;
            break;
        }
        continue;
    }
    if (result)
        return result;
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        return 42;
    if (log_count != 3 || dtor_log[0] != 40 || dtor_log[1] != 41
        || dtor_log[2] != 42)
        return 43;
    return 0;
}

static int check_do_continue(void)
{
    int i;
    int steps;
    int result;

    reset_observation();
    i = 0;
    steps = 0;
    result = 0;
    do {
        Owner value(60 + i);
        ++i;
        ++steps;
        if (steps > 5) {
            result = 61;
            break;
        }
        continue;
    } while (i < 3);
    if (result)
        return result;
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        return 62;
    if (log_count != 3 || dtor_log[0] != 60 || dtor_log[1] != 61
        || dtor_log[2] != 62)
        return 63;
    return 0;
}

static int check_temp_plus_local(void)
{
    int i;
    int result;
    int local_dtor_count;

    reset_observation();
    condition_value = 1;
    i = 0;
    result = 0;
    while (i < 1) {
        Owner local(80);
        if (make_condition().test())
            break;
        ++i;
    }
    local_dtor_count = 0;
    for (i = 0; i < log_count; ++i)
        if (dtor_log[i] == 80)
            ++local_dtor_count;
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        result = 71;
    if (log_count < 2 || dtor_log[log_count - 1] != 80
        || local_dtor_count != 1)
        result = 72;
    return result;
}

static int check_switch_break(void)
{
    int result;

    reset_observation();
    result = 0;
    {
        Owner outside(1);

        while (1) {
            switch (1) {
            case 1: {
                Owner inside(2);
                break;
            }
            }
            if (alive != 1 || dtor_count != 1 || log_count != 1
                || dtor_log[0] != 2)
                result = 81;
            break;
        }
        if (result)
            return result;
        if (alive != 1 || invalid_dtor != 0 || double_dtor != 0)
            return 82;
    }
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        return 83;
    if (log_count != 2 || dtor_log[1] != 1)
        return 84;
    return 0;
}

static int check_for_init_continue(void)
{
    int attempts;
    int i;
    int result;

    reset_observation();
    attempts = 0;
    i = 0;
    result = 0;
    for (Owner header(100); i < 3; ++i) {
        ++attempts;
        if (attempts > 5) {
            result = 91;
            break;
        }
        if (alive != 1 || dtor_count != i)
            result = 92;
        Owner body(110 + i);
        if (alive != 2 || dtor_count != i)
            result = 93;
        continue;
    }
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        return 94;
    if (log_count != 4 || dtor_log[0] != 110 || dtor_log[1] != 111
        || dtor_log[2] != 112 || dtor_log[3] != 100)
        return 95;
    return result;
}

static int check_for_init_break(void)
{
    reset_observation();
    for (Owner header(120); 1; ) {
        Owner body(121);
        break;
    }
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        return 96;
    if (log_count != 2 || dtor_log[0] != 121 || dtor_log[1] != 120)
        return 97;
    return 0;
}

static int check_switch_break_preserves_loop(void)
{
    int i;
    int result;

    reset_observation();
    i = 0;
    result = 0;
    while (i < 1) {
        Owner loop_local(130);
        switch (1) {
        case 1: {
            Owner case_local(131);
            break;
        }
        }
        if (alive != 1 || dtor_count != 1 || log_count != 1
            || dtor_log[0] != 131)
            result = 101;
        ++i;
    }
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        return 102;
    if (log_count != 2 || dtor_log[1] != 130)
        return 103;
    return result;
}

static int check_loop_break_preserves_switch(void)
{
    int result;

    reset_observation();
    result = 0;
    switch (1) {
    case 1: {
        Owner switch_local(140);
        while (1) {
            Owner loop_local(141);
            break;
        }
        if (alive != 1 || dtor_count != 1 || log_count != 1
            || dtor_log[0] != 141)
            result = 111;
        break;
    }
    }
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        return 112;
    if (log_count != 2 || dtor_log[1] != 140)
        return 113;
    return result;
}

static int check_switch_continue_outer_loop(void)
{
    int i;
    int result;

    reset_observation();
    i = 0;
    result = 0;
    while (i < 3) {
        if (i > 0 && (alive != 0 || invalid_dtor != 0
                      || double_dtor != 0))
            result = 121;
        Owner loop_local(150 + i);
        switch (1) {
        case 1: {
            Owner case_local(160 + i);
            ++i;
            continue;
        }
        }
    }
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        return 122;
    if (log_count != 6
        || dtor_log[0] != 160 || dtor_log[1] != 150
        || dtor_log[2] != 161 || dtor_log[3] != 151
        || dtor_log[4] != 162 || dtor_log[5] != 152)
        return 123;
    return result;
}

static int check_temp_continue(void)
{
    int i;
    int result;
    int local_dtor_count;

    reset_observation();
    condition_value = 1;
    i = 0;
    result = 0;
    while (i < 3) {
        if (i > 0 && (alive != 0 || invalid_dtor != 0
                      || double_dtor != 0))
            result = 131;
        Owner local(180 + i);
        if (make_condition().test()) {
            ++i;
            continue;
        }
        result = 132;
        break;
    }
    local_dtor_count = 0;
    for (i = 0; i < log_count; ++i)
        if (dtor_log[i] >= 180 && dtor_log[i] <= 182)
            ++local_dtor_count;
    if (alive != 0 || invalid_dtor != 0 || double_dtor != 0
        || ctor_count != dtor_count)
        return 133;
    if (local_dtor_count != 3)
        return 134;
    return result;
}

static int run_case(int case_no)
{
    if (case_no == 1)
        return check_while_break();
    if (case_no == 2)
        return check_while_continue();
    if (case_no == 3)
        return check_nested_break();
    if (case_no == 4)
        return check_nested_continue();
    if (case_no == 5)
        return check_for_continue();
    if (case_no == 6)
        return check_do_continue();
    if (case_no == 7)
        return check_temp_plus_local();
    if (case_no == 8)
        return check_switch_break();
    if (case_no == 9)
        return check_for_init_continue();
    if (case_no == 10)
        return check_for_init_break();
    if (case_no == 11)
        return check_switch_break_preserves_loop();
    if (case_no == 12)
        return check_loop_break_preserves_switch();
    if (case_no == 13)
        return check_switch_continue_outer_loop();
    if (case_no == 14)
        return check_temp_continue();
    return 99;
}

int main(int argc, char **argv)
{
    int case_no;
    int result;
    char *arg;

    case_no = 0;
    arg = argc > 1 ? argv[1] : 0;
    while (arg && *arg >= '0' && *arg <= '9') {
        case_no = case_no * 10 + *arg - '0';
        ++arg;
    }
    if (arg && *arg)
        case_no = 0;
    if (case_no > 0)
        return run_case(case_no);
    for (case_no = 1; case_no <= 14; ++case_no) {
        result = run_case(case_no);
        if (result)
            return result;
    }
    return 0;
}

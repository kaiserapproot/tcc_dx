static int alive;
static int ctor_count;
static int dtor_count;
static int invalid_dtor;
static void *live_objects[32];

static void reset_observation(void)
{
    ctor_count = 0;
    dtor_count = 0;
    invalid_dtor = 0;
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

struct Owner {
    int value;

    Owner() : value(7)
    {
        note_constructed(this);
    }

    Owner(const Owner &other) : value(other.value)
    {
        note_constructed(this);
    }

    int read()
    {
        return value;
    }

    int test()
    {
        return 0;
    }

    ~Owner()
    {
        ++dtor_count;
        if (!note_destroyed(this))
            ++invalid_dtor;
    }
};

static Owner make_owner(void)
{
    Owner value;

    return value;
}

static Owner make_condition(void)
{
    Owner value;

    return value;
}

static Owner choose_owner(int condition)
{
    return condition ? make_owner() : make_owner();
}

static int check_short_circuit_and(void)
{
    int value = false && make_owner().read();

    if (value != 0)
        return 101;
    if (alive != 0 || invalid_dtor != 0 || ctor_count != dtor_count)
        return 102;
    return 0;
}

static int check_short_circuit_or(void)
{
    int value = true || make_owner().read();

    if (value == 0)
        return 201;
    if (alive != 0 || invalid_dtor != 0 || ctor_count != dtor_count)
        return 202;
    return 0;
}

static int check_conditional_operand(void)
{
    int condition = 0;
    int value = (condition ? make_owner() : make_owner()).read();

    if (value != 7)
        return 301;
    if (alive != 0 || invalid_dtor != 0 || ctor_count != dtor_count)
        return 302;
    return 0;
}

static int check_loop_condition(void)
{
    int observed;

    reset_observation();
    while (make_condition().test()) {
        observed = 99;
    }
    observed = alive;
    if (observed != 0 || invalid_dtor != 0)
        return 401;
    if (ctor_count != dtor_count)
        return 402;
    return 0;
}

static int check_conditional_return(void)
{
    reset_observation();
    {
        Owner result = choose_owner(0);

        if (result.read() != 7)
            return 501;
        if (alive != 1 || invalid_dtor != 0)
            return 502;
    }
    if (alive != 0 || invalid_dtor != 0 || ctor_count != dtor_count)
        return 503;
    return 0;
}

int main(int argc, char **argv)
{
    int result;

    if (argc > 1) {
        reset_observation();
        if (argv[1][0] == '1')
            return check_short_circuit_and();
        if (argv[1][0] == '2')
            return check_short_circuit_or();
        if (argv[1][0] == '3')
            return check_conditional_operand();
        if (argv[1][0] == '4')
            return check_loop_condition();
        if (argv[1][0] == '5')
            return check_conditional_return();
        return 99;
    }

    reset_observation();
    result = check_short_circuit_and();
    if (result)
        return result;
    reset_observation();
    result = check_short_circuit_or();
    if (result)
        return result;
    reset_observation();
    result = check_conditional_operand();
    if (result)
        return result;
    result = check_loop_condition();
    if (result)
        return result;
    result = check_conditional_return();
    if (result)
        return result;
    return 0;
}

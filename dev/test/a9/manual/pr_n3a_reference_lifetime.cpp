static int alive;
static int ctor_count;
static int dtor_count;
static int invalid_dtor;
static void *live_objects[16];

static void reset_observation(void)
{
    alive = 0;
    ctor_count = 0;
    dtor_count = 0;
    invalid_dtor = 0;
}

static void note_constructed(void *object)
{
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

    int read() const
    {
        return value;
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

struct Child {
    int value;
};

struct OwnerWithChild {
    int padding;
    Child child;

    OwnerWithChild()
    {
        padding = 0;
        child.value = 7;
        note_constructed(this);
    }

    OwnerWithChild(const OwnerWithChild &other)
    {
        padding = other.padding;
        child.value = other.child.value;
        note_constructed(this);
    }

    ~OwnerWithChild()
    {
        ++dtor_count;
        if (!note_destroyed(this))
            ++invalid_dtor;
    }
};

static OwnerWithChild make_owner_with_child(void)
{
    OwnerWithChild value;

    return value;
}

struct Base {
    int value;
};

struct Derived : Base {
    Derived()
    {
        value = 7;
        note_constructed(this);
    }

    Derived(const Derived &other)
    {
        value = other.value;
        note_constructed(this);
    }

    ~Derived()
    {
        ++dtor_count;
        if (!note_destroyed(this))
            ++invalid_dtor;
    }
};

static Derived make_derived(void)
{
    Derived value;

    return value;
}

static int check_complete_reference(void)
{
    int result;

    reset_observation();
    result = 0;
    {
        const Owner &ref = make_owner();

        if (alive != 1 || invalid_dtor != 0)
            result = 1;
        else if (ref.value != 7)
            result = 2;
    }
    if (alive != 0 || invalid_dtor != 0 || ctor_count != dtor_count)
        return 3;
    return result;
}

static int check_scalar_subobject_reference(void)
{
    int result;

    reset_observation();
    result = 0;
    {
        const int &ref = make_owner().value;

        if (alive != 1 || invalid_dtor != 0)
            result = 11;
        else if (ref != 7)
            result = 12;
    }
    if (alive != 0 || invalid_dtor != 0 || ctor_count != dtor_count)
        return 13;
    return result;
}

static int check_class_subobject_reference(void)
{
    int result;

    reset_observation();
    result = 0;
    {
        const Child &ref = make_owner_with_child().child;

        if (alive != 1 || invalid_dtor != 0)
            result = 21;
        else if (ref.value != 7)
            result = 22;
    }
    if (alive != 0 || invalid_dtor != 0 || ctor_count != dtor_count)
        return 23;
    return result;
}

static int check_base_subobject_reference(void)
{
    int result;

    reset_observation();
    result = 0;
    {
        const Base &ref = make_derived();

        if (alive != 1 || invalid_dtor != 0)
            result = 31;
        else if (ref.value != 7)
            result = 32;
    }
    if (alive != 0 || invalid_dtor != 0 || ctor_count != dtor_count)
        return 33;
    return result;
}

int main(int argc, char **argv)
{
    int case_no;

    case_no = argc > 1 ? argv[1][0] - '0' : 1;
    if (case_no == 1)
        return check_complete_reference();
    if (case_no == 2)
        return check_scalar_subobject_reference();
    if (case_no == 3)
        return check_class_subobject_reference();
    if (case_no == 4)
        return check_base_subobject_reference();
    return 99;
}

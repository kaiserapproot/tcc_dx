static int alive;
static int ctor_count;
static int dtor_count;
static int invalid_dtor;
static void *live_objects[16];

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

int main(void)
{
    int observed;

    {
        const Owner &ref = make_owner();

        if (ref.read() != 7)
            return 1;
        observed = alive;
        if (observed != 1 || invalid_dtor != 0)
            return 2;
    }
    if (alive != 0 || invalid_dtor != 0 || ctor_count != dtor_count)
        return 3;
    return 0;
}

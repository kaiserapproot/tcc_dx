struct vec3 {
    float x, y, z;

    vec3() : x(0), y(0), z(0) {}
};

static void f()
{
    static vec3 vertex_storage[32];
    int i;

    for (i = 0; i < 32; i++) {
        if (vertex_storage[i].x != 0.0f
            || vertex_storage[i].y != 0.0f
            || vertex_storage[i].z != 0.0f)
            abort();
    }
}

int main()
{
    f();
    f();
    return 0;
}

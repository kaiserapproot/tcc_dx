struct vec3 {
    float x, y, z;

    vec3() : x(0), y(0), z(0) {}
    vec3 operator=(const vec3& in_data)
    {
        x = in_data.x;
        y = in_data.y;
        z = in_data.z;
        return *this;
    }
};

struct vec4 {
    float x, y, z, w;

    vec4() : x(0), y(0), z(0), w(0) {}
    vec4& operator=(const vec4& in_data)
    {
        x = in_data.x;
        y = in_data.y;
        z = in_data.z;
        w = in_data.w;
        return *this;
    }
};

typedef struct {
    float frame;
    vec3 pos;
    vec4 rot;
    unsigned char interp_x[16];
    unsigned char interp_y[16];
    unsigned char interp_z[16];
    unsigned char interp_r[16];
} mmd_vmd_bone_key_t;

typedef struct {
    mmd_vmd_bone_key_t *keys;
    int key_count;
} mmd_vmd_bone_track_t;

int main(void)
{
    mmd_vmd_bone_track_t tr;
    mmd_vmd_bone_key_t key;
    mmd_vmd_bone_key_t store[2];

    tr.keys = store;
    tr.key_count = 0;
    key.frame = 3.0f;
    key.pos.x = 7.0f;
    key.rot.w = 1.0f;
    tr.keys[tr.key_count++] = key;
    return tr.keys[0].frame == 3.0f && tr.keys[0].pos.x == 7.0f && tr.keys[0].rot.w == 1.0f ? 0 : 1;
}

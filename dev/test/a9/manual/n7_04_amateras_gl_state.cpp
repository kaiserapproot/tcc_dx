typedef struct {
    char path[260];
    unsigned char ramp[256 * 3];
    int valid;
} mmd_toon_ramp_t;

#define MMD_TOON_RAMP_CACHE_MAX 64

typedef struct {
    int max_tex_size;
    mmd_toon_ramp_t toon_ramp[MMD_TOON_RAMP_CACHE_MAX];
    int toon_ramp_n;
} mmd_gl_backend_state_t;

static mmd_gl_backend_state_t g_mmd_gl_default_state;

int main(void)
{
    g_mmd_gl_default_state.toon_ramp_n = 7;
    return g_mmd_gl_default_state.toon_ramp_n == 7 ? 0 : 1;
}

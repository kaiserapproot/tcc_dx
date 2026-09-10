struct mmd_dq_t {
    float x;
    float y;
    float z;
    float w;
};

static mmd_dq_t mmd_dq_make_identity(void)
{
    mmd_dq_t dq;
    dq.x = 0.0f;
    dq.y = 0.0f;
    dq.z = 0.0f;
    dq.w = 1.0f;
    return dq;
}

static const mmd_dq_t c_identity_dq = mmd_dq_make_identity();

int main(void)
{
    if (c_identity_dq.w != 1.0f)
        return 1;
    return 0;
}

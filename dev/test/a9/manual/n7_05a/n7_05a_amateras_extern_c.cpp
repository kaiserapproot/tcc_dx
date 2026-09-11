typedef unsigned int am_job_result_t;
typedef unsigned int am_job_config_t;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct am_job_memory_layout {
    unsigned struct_size;
    unsigned core_bytes;
    unsigned core_alignment;
} am_job_memory_layout_t;

am_job_result_t am_job_memory_layout(
    const am_job_config_t *config,
    am_job_memory_layout_t *out_layout);

#ifdef __cplusplus
}
#endif

int main(void)
{
    return 0;
}

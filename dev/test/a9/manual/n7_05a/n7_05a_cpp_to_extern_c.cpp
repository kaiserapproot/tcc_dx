typedef unsigned int am_job_result_t;
typedef unsigned int am_job_config_t;
typedef unsigned int am_job_memory_layout_t;

typedef struct am_job_memory_layout {
    unsigned struct_size;
    unsigned core_bytes;
    unsigned core_alignment;
} am_job_memory_layout_t;

extern "C" {
typedef struct am_job_memory_layout {
    unsigned struct_size;
    unsigned core_bytes;
    unsigned core_alignment;
} am_job_memory_layout_t;
}

int main(void)
{
    return 0;
}

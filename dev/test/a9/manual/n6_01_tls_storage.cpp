#include <windows.h>
#include <stdio.h>

thread_local int tls_value;
thread_local int tls_other;
thread_local int tls_initialized = 7;
int shared_value;

static int worker_initial_value;
static int worker_initial_other;
static int worker_initial_initialized;
static int worker_value_after;
static int worker_other_after;
static int worker_initialized_after;
static int worker_shared_before;
static int worker_shared_after;
static int *worker_value_address;
static int *worker_value_address_again;
static int *worker_other_address;

static DWORD WINAPI worker(void *arg)
{
    worker_initial_value = tls_value;
    worker_initial_other = tls_other;
    worker_initial_initialized = tls_initialized;
    worker_value_address = &tls_value;
    worker_value_address_again = &tls_value;
    worker_other_address = &tls_other;
    worker_shared_before = shared_value;
    tls_value = 222;
    tls_other = 223;
    shared_value = 44;
    worker_value_after = tls_value;
    worker_other_after = tls_other;
    worker_initialized_after = tls_initialized;
    worker_shared_after = shared_value;
    return 0;
}

static int run_worker()
{
    HANDLE handle;
    DWORD wait_result;

    handle = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (!handle)
        return 1;
    wait_result = WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
    return wait_result == WAIT_OBJECT_0 ? 0 : 2;
}

int main()
{
    int *main_value_address;
    int *main_value_address_again;
    int *main_other_address;

    if (tls_value != 0 || tls_other != 0 || tls_initialized != 7)
        return 1;
    main_value_address = &tls_value;
    main_value_address_again = &tls_value;
    main_other_address = &tls_other;
    if (main_value_address != main_value_address_again
        || main_value_address == main_other_address)
        return 2;
    tls_value = 111;
    tls_other = 112;
    shared_value = 33;

    if (run_worker() != 0)
        return 3;
    if (worker_initial_value != 0 || worker_initial_other != 0
        || worker_initial_initialized != 7)
        return 4;
    if (worker_value_after != 222 || worker_other_after != 223
        || worker_initialized_after != 7)
        return 5;
    if (worker_value_address != worker_value_address_again
        || worker_value_address == worker_other_address)
        return 6;
    if (worker_shared_before != 33 || worker_shared_after != 44)
        return 7;
    if (tls_value != 111 || tls_other != 112
        || &tls_value != main_value_address
        || &tls_other != main_other_address)
        return 8;

    if (run_worker() != 0)
        return 9;
    if (worker_initial_value != 0 || worker_initial_other != 0
        || worker_initial_initialized != 7)
        return 10;
    if (worker_value_after != 222 || worker_other_after != 223
        || worker_initialized_after != 7)
        return 11;
    if (worker_value_address == worker_other_address
        || worker_value_address == main_value_address)
        return 12;
    if (worker_shared_before != 44 || shared_value != 44)
        return 13;

    printf("N6_01_TLS_STORAGE_PRIMITIVE=PASS\n");
    printf("THREAD_LOCAL_INT=PASS\n");
    printf("TLS_OWNER=TCB\n");
    printf("SAME_THREAD_ADDRESS_STABLE=PASS\n");
    printf("CROSS_THREAD_ADDRESS_ISOLATION=PASS\n");
    printf("CROSS_THREAD_VALUE_ISOLATION=PASS\n");
    printf("PER_THREAD_ZERO_INITIALIZATION=PASS\n");
    printf("MULTIPLE_TLS_VARIABLES=PASS\n");
    printf("ORDINARY_GLOBAL_SHARED=PASS\n");
    return 0;
}

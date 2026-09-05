// N6-01 closure: concurrent-worker TLS address/value isolation.
//
// n6_01_tls_storage.cpp は worker を順番に生成・join するため (sequential),
// 「同時に生存する別 thread 同士で address が異なる」ことは直接証明していない.
// (join 後に OS/CRT が address を再利用しても検出できない.)
// このテストは event で 2 worker を同時に生存させ, 両者が TLS storage を
// 保持している間に main が address/value を比較する (concurrent authority).
//
// N6-01 の範囲: thread_local int のみ. ctor/dtor/LIFO/thread-exit cleanup は
// 実装しない (N6-02 以降).
#include <windows.h>
#include <stdio.h>

thread_local int tls_value;

// worker -> main: "TLS storage を取得して値を書いた" 通知 (auto-reset)
static HANDLE ready_a;
static HANDLE ready_b;
// main -> worker: "比較が終わったので終了してよい" (manual-reset, shared)
static HANDLE release_all;

static int *worker_a_address;
static int *worker_b_address;
static int worker_a_value_before_release;
static int worker_b_value_before_release;
static int worker_a_value_after_release;
static int worker_b_value_after_release;
static int worker_a_initial;
static int worker_b_initial;

static DWORD WINAPI worker_a(void *arg)
{
    worker_a_initial = tls_value;
    tls_value = 222;
    worker_a_address = &tls_value;
    worker_a_value_before_release = tls_value;
    SetEvent(ready_a);
    // worker_b と main がこの間に tls_value を触っても, この thread の
    // storage は影響を受けないことを release 後の再読で確認する (isolation).
    if (WaitForSingleObject(release_all, INFINITE) != WAIT_OBJECT_0)
        return 1;
    worker_a_value_after_release = tls_value;
    if (&tls_value != worker_a_address)
        return 2;
    return 0;
}

static DWORD WINAPI worker_b(void *arg)
{
    worker_b_initial = tls_value;
    tls_value = 333;
    worker_b_address = &tls_value;
    worker_b_value_before_release = tls_value;
    SetEvent(ready_b);
    if (WaitForSingleObject(release_all, INFINITE) != WAIT_OBJECT_0)
        return 1;
    worker_b_value_after_release = tls_value;
    if (&tls_value != worker_b_address)
        return 2;
    return 0;
}

int main()
{
    HANDLE threads[2];
    HANDLE readies[2];
    DWORD exit_a;
    DWORD exit_b;
    int *main_address;

    ready_a = CreateEventA(NULL, FALSE, FALSE, NULL);
    ready_b = CreateEventA(NULL, FALSE, FALSE, NULL);
    release_all = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (!ready_a || !ready_b || !release_all)
        return 1;

    tls_value = 111;
    main_address = &tls_value;

    threads[0] = CreateThread(NULL, 0, worker_a, NULL, 0, NULL);
    threads[1] = CreateThread(NULL, 0, worker_b, NULL, 0, NULL);
    if (!threads[0] || !threads[1])
        return 2;

    // 両 worker が TLS storage を保持した状態 (どちらもまだ終了していない)
    // になるまで待つ. ここが「同時生存」の barrier.
    readies[0] = ready_a;
    readies[1] = ready_b;
    if (WaitForMultipleObjects(2, readies, TRUE, INFINITE) != WAIT_OBJECT_0)
        return 3;

    // --- 同時生存中の Authority 判定 ---
    if (worker_a_initial != 0 || worker_b_initial != 0)
        return 4;
    if (main_address == worker_a_address)
        return 5;
    if (main_address == worker_b_address)
        return 6;
    if (worker_a_address == worker_b_address)
        return 7;
    if (tls_value != 111 || &tls_value != main_address)
        return 8;
    if (worker_a_value_before_release != 222)
        return 9;
    if (worker_b_value_before_release != 333)
        return 10;
    // main が自分の storage を書き換えても worker 側は変わらないことを
    // release 後の worker 再読で確認する.
    tls_value = 1111;

    SetEvent(release_all);
    if (WaitForMultipleObjects(2, threads, TRUE, INFINITE) != WAIT_OBJECT_0)
        return 11;
    if (!GetExitCodeThread(threads[0], &exit_a) || exit_a != 0)
        return 12;
    if (!GetExitCodeThread(threads[1], &exit_b) || exit_b != 0)
        return 13;
    if (worker_a_value_after_release != 222)
        return 14;
    if (worker_b_value_after_release != 333)
        return 15;
    if (tls_value != 1111 || &tls_value != main_address)
        return 16;

    CloseHandle(threads[0]);
    CloseHandle(threads[1]);
    CloseHandle(ready_a);
    CloseHandle(ready_b);
    CloseHandle(release_all);

    // worker 終了後の address 再利用は判定対象外 (FAIL にしない).
    printf("N6_01_CONCURRENT_CROSS_THREAD_ADDRESS=PASS\n");
    printf("CONCURRENT_WORKERS=2\n");
    printf("MAIN_ADDRESS_NE_WORKER_A_ADDRESS=PASS\n");
    printf("MAIN_ADDRESS_NE_WORKER_B_ADDRESS=PASS\n");
    printf("WORKER_A_ADDRESS_NE_WORKER_B_ADDRESS=PASS\n");
    printf("MAIN_VALUE=111\n");
    printf("WORKER_A_VALUE=222\n");
    printf("WORKER_B_VALUE=333\n");
    printf("CONCURRENT_VALUE_ISOLATION=PASS\n");
    printf("ADDRESS_REUSE_AFTER_THREAD_EXIT=NOT_GATED\n");
    return 0;
}

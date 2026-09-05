// N6-02: per-thread lazy non-trivial default constructor.
//
// Authority:
//   N6_02_INITIALIZATION=PER_THREAD_LAZY_ON_FIRST_ACCESS
//   N6_02_CTOR_EXACTLY_ONCE=PER_THREAD
//   N6_02_STORAGE=PER_THREAD / N6_02_ADDRESS=PER_THREAD
//
// 検証内容:
//   1. 起動時 (main 到達時) には constructor は 1 回も走っていない (lazy).
//   2. main の最初のアクセスで main thread 上で 1 回だけ構築される.
//      2 回目以降のアクセスでは再構築されない (exactly once per thread).
//   3. event barrier で同時に生存する worker A / B それぞれが, 自分の thread 上で
//      1 回だけ構築する (ctor が記録した owner thread id == その worker の id).
//   4. address は main / A / B で全て異なり, 値も互いに漏れない (isolation).
//   5. default 引数のみの ctor (`Q(int a = 7)`) も default ctor として受理され,
//      thunk 経由で default 引数が渡る.
//   6. ctor を持たない plain class は zero-fill (N6-01 int と同じ扱い).
//   7. TLS object に対する member function 呼び出しが動く.
//
// N6-02 の範囲外 (別 gate で fail-closed を確認): non-trivial destructor,
// thread-exit cleanup, dtor registry.
#include <windows.h>
#include <stdio.h>

// worker A/B の ctor は同時に走り得るので, カウンタ更新は critical section で
// 直列化する (InterlockedIncrement は dev/include の SDK subset では未解決).
static LONG g_ctor_calls;
static CRITICAL_SECTION g_ctor_lock;

struct P {
    int value;
    DWORD owner;
    P() {
        value = 123;
        owner = GetCurrentThreadId();
        EnterCriticalSection(&g_ctor_lock);
        ++g_ctor_calls;
        LeaveCriticalSection(&g_ctor_lock);
    }
    int get() { return value; }
    void set(int v) { value = v; }
};

struct Q {
    int v;
    Q(int a = 7) { v = a; }
};

struct T {
    int a;
    int b;
};

thread_local P object;
thread_local Q q_object;
thread_local T trivial;

// ctor の中から別の thread_local object を触る (runtime の entries が
// ctor 実行中に realloc されても, 構築中 entry を index で再参照する契約).
struct R {
    int copy;
    R() { copy = object.value + 1; }
};

thread_local R nested;

static HANDLE ready_a;
static HANDLE ready_b;
static HANDLE release_all;

static P *worker_a_address;
static P *worker_b_address;
static int worker_a_value_before_release;
static int worker_b_value_before_release;
static int worker_a_value_after_release;
static int worker_b_value_after_release;
static DWORD worker_a_owner;
static DWORD worker_b_owner;
static DWORD worker_a_id;
static DWORD worker_b_id;
static LONG worker_a_ctor_calls_after_first_access;
static int worker_a_q_value;
static int worker_b_trivial_sum;

static DWORD WINAPI worker_a(void *arg)
{
    worker_a_id = GetCurrentThreadId();
    // 最初のアクセス: この thread 上で ctor が 1 回走る.
    if (object.get() != 123)
        return 20;
    worker_a_ctor_calls_after_first_access = g_ctor_calls;
    worker_a_owner = object.owner;
    object.set(222);
    worker_a_address = &object;
    worker_a_value_before_release = object.value;
    worker_a_q_value = q_object.v;
    SetEvent(ready_a);
    if (WaitForSingleObject(release_all, INFINITE) != WAIT_OBJECT_0)
        return 1;
    worker_a_value_after_release = object.value;
    if (&object != worker_a_address)
        return 2;
    return 0;
}

static DWORD WINAPI worker_b(void *arg)
{
    worker_b_id = GetCurrentThreadId();
    if (object.value != 123)
        return 20;
    worker_b_owner = object.owner;
    object.value = 333;
    worker_b_address = &object;
    worker_b_value_before_release = object.value;
    worker_b_trivial_sum = trivial.a + trivial.b;
    trivial.a = 40;
    SetEvent(ready_b);
    if (WaitForSingleObject(release_all, INFINITE) != WAIT_OBJECT_0)
        return 1;
    worker_b_value_after_release = object.value;
    if (&object != worker_b_address)
        return 2;
    return 0;
}

int main()
{
    HANDLE threads[2];
    HANDLE readies[2];
    DWORD exit_a;
    DWORD exit_b;
    P *main_address;
    LONG calls_after_first;

    // 1. lazy: main 到達時点で ctor は未実行.
    if (g_ctor_calls != 0)
        return 1;
    InitializeCriticalSection(&g_ctor_lock);

    ready_a = CreateEventA(NULL, FALSE, FALSE, NULL);
    ready_b = CreateEventA(NULL, FALSE, FALSE, NULL);
    release_all = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (!ready_a || !ready_b || !release_all)
        return 2;

    // 2. main の最初のアクセスで 1 回だけ構築.
    if (object.value != 123)
        return 3;
    calls_after_first = g_ctor_calls;
    if (calls_after_first != 1)
        return 4;
    if (object.owner != GetCurrentThreadId())
        return 5;
    if (object.get() != 123 || g_ctor_calls != 1)
        return 6;
    object.set(111);
    if (object.value != 111 || g_ctor_calls != 1)
        return 7;
    main_address = &object;
    if (&object != main_address)
        return 8;

    // 5. default 引数のみの ctor.
    if (q_object.v != 7)
        return 9;
    q_object.v = 70;
    if (q_object.v != 70)
        return 10;

    // 6. ctor 無し class は zero-fill.
    if (trivial.a != 0 || trivial.b != 0)
        return 11;
    trivial.a = 4;
    trivial.b = 5;
    // 8. ctor 内から別 TLS object を参照 (object.value は上で 111 に設定済み).
    if (nested.copy != 112 || g_ctor_calls != 1)
        return 31;

    threads[0] = CreateThread(NULL, 0, worker_a, NULL, 0, NULL);
    threads[1] = CreateThread(NULL, 0, worker_b, NULL, 0, NULL);
    if (!threads[0] || !threads[1])
        return 12;

    readies[0] = ready_a;
    readies[1] = ready_b;
    if (WaitForMultipleObjects(2, readies, TRUE, INFINITE) != WAIT_OBJECT_0)
        return 13;

    // --- 同時生存中の Authority 判定 ---
    // 3. 各 worker が自分の thread 上で構築した.
    if (worker_a_owner != worker_a_id || worker_a_id == GetCurrentThreadId())
        return 14;
    if (worker_b_owner != worker_b_id || worker_b_id == GetCurrentThreadId())
        return 15;
    if (g_ctor_calls != 3)
        return 16;
    // 4. address isolation.
    if (main_address == worker_a_address || main_address == worker_b_address)
        return 17;
    if (worker_a_address == worker_b_address)
        return 18;
    // 4. value isolation.
    if (object.value != 111 || &object != main_address)
        return 19;
    if (worker_a_value_before_release != 222)
        return 20;
    if (worker_b_value_before_release != 333)
        return 21;
    if (worker_a_q_value != 7 || q_object.v != 70)
        return 22;
    if (worker_b_trivial_sum != 0 || trivial.a != 4 || trivial.b != 5)
        return 23;
    object.value = 1111;

    SetEvent(release_all);
    if (WaitForMultipleObjects(2, threads, TRUE, INFINITE) != WAIT_OBJECT_0)
        return 24;
    if (!GetExitCodeThread(threads[0], &exit_a) || exit_a != 0)
        return 25;
    if (!GetExitCodeThread(threads[1], &exit_b) || exit_b != 0)
        return 26;
    if (worker_a_value_after_release != 222)
        return 27;
    if (worker_b_value_after_release != 333)
        return 28;
    if (object.value != 1111 || &object != main_address)
        return 29;
    if (g_ctor_calls != 3)
        return 30;

    CloseHandle(threads[0]);
    CloseHandle(threads[1]);
    CloseHandle(ready_a);
    CloseHandle(ready_b);
    CloseHandle(release_all);

    printf("N6_02_TLS_LAZY_CTOR=PASS\n");
    printf("CTOR_CALLS_AT_MAIN_ENTRY=0\n");
    printf("CTOR_CALLS_AFTER_MAIN_FIRST_ACCESS=%ld\n", calls_after_first);
    printf("CTOR_CALLS_TOTAL=%ld\n", g_ctor_calls);
    printf("CTOR_RUNS_ON_ACCESSING_THREAD=PASS\n");
    printf("CTOR_EXACTLY_ONCE_PER_THREAD=PASS\n");
    printf("CONCURRENT_WORKERS=2\n");
    printf("MAIN_ADDRESS_NE_WORKER_ADDRESSES=PASS\n");
    printf("WORKER_A_ADDRESS_NE_WORKER_B_ADDRESS=PASS\n");
    printf("CONCURRENT_VALUE_ISOLATION=PASS\n");
    printf("DEFAULT_ARG_CTOR_VIA_THUNK=PASS\n");
    printf("TRIVIAL_CLASS_ZERO_FILL=PASS\n");
    printf("MEMBER_CALL_ON_TLS_OBJECT=PASS\n");
    printf("NESTED_TLS_ACCESS_IN_CTOR=PASS\n");
    return 0;
}

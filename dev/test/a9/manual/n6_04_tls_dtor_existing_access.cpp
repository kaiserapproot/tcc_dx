// N6-04 Authority: destructor が drain 中に「既に構築済みで, まだ破棄されていない」
// 別の thread_local object を読める (EXISTING_TLS_ACCESS_DURING_DRAIN=SUPPORTED).
//
// drain order を崩さないテスト設計:
//   worker は a を先に, b を後に touch する -> 構築順 A, B -> 破棄順 B, A.
//   B::~B() が a (自分より先に構築 = 自分より後に破棄) を読む.  この時点で a は
//   INITIALIZED のまま runtime から返るので値 (0xA + 1) が読める.
//   A::~A() は「b が既に破棄済み」を g_b_destroyed で確認する (順序の再確認).
//
// 加えて, A::~A() と B::~B() は trivial な thread_local int (g_slot) も読む.
// また B の破棄後に同じ thread から b を読む経路は本 test では作らない
// (destroyed object へのアクセスは runtime が abort する fail-closed 経路であり,
// n6_04_tls_new_tls_during_drain.cpp と同じ種類の負のテストになる).
#include <windows.h>
#include <stdio.h>

struct Record {
    volatile DWORD tid;
    volatile int a_ctor, b_ctor;
    volatile int a_dtor, b_dtor;
    volatile int a_value_seen_in_b_dtor;
    volatile int b_destroyed_before_a_dtor;
    volatile int slot_seen_in_a_dtor;
    volatile int dtor_seq_b, dtor_seq_a;
    volatile DWORD a_dtor_tid, b_dtor_tid;
};
static Record g_rec[2];
static volatile int g_seq;

thread_local int g_slot;

struct A { int v; A(); ~A(); };
struct B { int v; B(); ~B(); };

thread_local A a;
thread_local B b;

A::A() { v = 0xA; ++g_rec[g_slot].a_ctor; }
B::B() { v = 0xB; ++g_rec[g_slot].b_ctor; }

B::~B()
{
    Record *r = &g_rec[g_slot];
    ++r->b_dtor;
    r->b_dtor_tid = GetCurrentThreadId();
    r->dtor_seq_b = ++g_seq;
    // a は自分より先に構築されたので, まだ生きている (existing TLS access).
    r->a_value_seen_in_b_dtor = a.v;
}

A::~A()
{
    Record *r = &g_rec[g_slot];
    ++r->a_dtor;
    r->a_dtor_tid = GetCurrentThreadId();
    r->dtor_seq_a = ++g_seq;
    r->b_destroyed_before_a_dtor = (r->b_dtor == 1);
    r->slot_seen_in_a_dtor = g_slot;
}

static DWORD WINAPI worker(void *p)
{
    int slot = (int)(INT_PTR)p;
    g_slot = slot;
    g_rec[slot].tid = GetCurrentThreadId();
    a.v += 1;      // 構築順 1
    b.v += 1;      // 構築順 2
    return 0;
}

int main()
{
    HANDLE h;
    Record *r = &g_rec[1];
    int ok;

    memset(g_rec, 0, sizeof g_rec);
    g_slot = 0;
    h = CreateThread(NULL, 0, worker, (void *)(INT_PTR)1, 0, NULL);
    if (!h) { printf("CreateThread failed\n"); return 1; }
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);

    ok = r->a_ctor == 1 && r->b_ctor == 1 && r->a_dtor == 1 && r->b_dtor == 1
         && r->a_value_seen_in_b_dtor == 0xA + 1
         && r->b_destroyed_before_a_dtor == 1
         && r->dtor_seq_b < r->dtor_seq_a
         && r->slot_seen_in_a_dtor == 1
         && r->a_dtor_tid == r->tid && r->b_dtor_tid == r->tid;
    printf("ctor a=%d b=%d dtor a=%d b=%d\n", r->a_ctor, r->b_ctor, r->a_dtor, r->b_dtor);
    printf("A_VALUE_SEEN_IN_B_DTOR=%d (expected %d)\n", r->a_value_seen_in_b_dtor, 0xA + 1);
    printf("B_DESTROYED_BEFORE_A_DTOR=%s DTOR_ORDER=%s\n",
           r->b_destroyed_before_a_dtor ? "YES" : "NO",
           r->dtor_seq_b < r->dtor_seq_a ? "B,A" : "A,B");
    printf("TRIVIAL_TLS_INT_SEEN_IN_DTOR=%d (expected 1)\n", r->slot_seen_in_a_dtor);
    printf("DTOR_TIDS_EQ_OWNER=%s\n",
           (r->a_dtor_tid == r->tid && r->b_dtor_tid == r->tid) ? "YES" : "NO");
    if (!ok) {
        printf("N6_04_DTOR_ACCESS_EXISTING_TLS=FAIL\n");
        return 1;
    }
    printf("N6_04_DTOR_ACCESS_EXISTING_TLS=PASS\n");
    printf("EXISTING_TLS_ACCESS_DURING_DRAIN=PASS\n");
    return 0;
}

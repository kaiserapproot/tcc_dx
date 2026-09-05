// N6-02 REVIEW FIX-2: TLS object alignment authority.
//
// storage は注入 runtime の CRT calloc() から取得する. MSVC CRT の malloc/calloc
// は MEMORY_ALLOCATION_ALIGNMENT (winnt.h: _WIN64 で 16, それ以外 8) に揃った
// 領域を返す (fundamental alignment). compiler 側はこれを超える alignment の型を
// 宣言時に fail-closed するので, runtime に到達する object は全て
// alignment <= 16 (x64). このテストは main と worker の両 thread で, 受理される
// 最大 alignment (16) まで含めて address が 16 境界にあることを実測する.
#include <windows.h>
#include <stdio.h>

struct Small {
    char c;
    Small() { c = 1; }
};

struct WithDouble {
    char pad;
    double d;
    WithDouble() { d = 2.5; }
};

struct __attribute__((aligned(16))) Aligned16 {
    int v;
    Aligned16() { v = 16; }
};

struct Plain {
    long long ll;
    int i;
};

thread_local int tls_int;
thread_local Small small;
thread_local WithDouble with_double;
thread_local Aligned16 aligned16;
thread_local Plain plain;

#define ALIGNED16(p) ((((UINT_PTR)(p)) & 15) == 0)

static int check_all(const char *who)
{
    int ok = 1;
    ok &= ALIGNED16(&tls_int);
    ok &= ALIGNED16(&small);
    ok &= ALIGNED16(&with_double);
    ok &= ALIGNED16(&aligned16);
    ok &= ALIGNED16(&plain);
    ok &= (small.c == 1);
    ok &= (with_double.d == 2.5);
    ok &= (aligned16.v == 16);
    ok &= (plain.ll == 0 && plain.i == 0);
    printf("%s_ALIGN16=%s\n", who, ok ? "PASS" : "FAIL");
    return ok;
}

static DWORD WINAPI worker(void *arg)
{
    return check_all("WORKER") ? 0 : 1;
}

int main()
{
    HANDLE th;
    DWORD rc;

    if (!check_all("MAIN"))
        return 1;
    th = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (!th)
        return 2;
    if (WaitForSingleObject(th, INFINITE) != WAIT_OBJECT_0)
        return 3;
    if (!GetExitCodeThread(th, &rc) || rc != 0)
        return 4;
    CloseHandle(th);
    printf("N6_02_TLS_ALIGNMENT=PASS\n");
    printf("TLS_OBJECT_ALIGNMENT_AUTHORITY=CRT_CALLOC_MEMORY_ALLOCATION_ALIGNMENT_16\n");
    printf("MAX_ACCEPTED_TLS_ALIGNMENT=16\n");
    return 0;
}

// N6-05 J: worker thread calling exit() is fail-closed (unsupported in N6-05).
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

static DWORD WINAPI worker(void *p)
{
    (void)p;
    exit(9);
    return 0;
}

int main()
{
    HANDLE h;
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    h = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (!h)
        return 1;
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    return 0;
}

/* The intrinsics that have a TCC body must really execute from C:
   __debugbreak (the breakpoint is caught and stepped over), _abs64 and
   __readgsqword.  Loading the header and running an intrinsic are separate
   gates on purpose. */
#include <windows.h>
#include <stdlib.h>
#include <intrin.h>

static volatile LONG g_breaks;

static LONG CALLBACK on_exception(PEXCEPTION_POINTERS ep)
{
    if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT) {
        unsigned char *ip = (unsigned char *)ep->ContextRecord->Rip;
        if (*ip == 0xCC) {
            /* int3 is one byte: step over it and resume. */
            ep->ContextRecord->Rip += 1;
            g_breaks += 1;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

int main(void)
{
    if (!AddVectoredExceptionHandler(1, on_exception)) return 10;
    __debugbreak();
    __debugbreak();
    if (g_breaks != 2) return 11;
    if (_abs64(-5) != 5) return 12;
    if ((void *)__readgsqword(0x30) != (void *)NtCurrentTeb()) return 13;
    return 0;
}

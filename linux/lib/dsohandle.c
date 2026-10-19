/* __dso_handle and its __cxa_finalize hook (what gcc gets from
   crtbegin.o / crtbeginS.o, which tcc does not link).

   atexit() from libc_nonshared.a registers through
   __cxa_atexit(fn, NULL, __dso_handle).  In a shared library the handle
   is per-DSO (hidden), and the handlers it registered must run and be
   dropped when the library is dlclose()d; otherwise they outlive the
   mapping and the process jumps into unmapped code at exit.  crtbeginS.o
   does that from .fini_array with __cxa_finalize(__dso_handle); do the
   same here.  This object is pulled in exactly when __dso_handle is
   referenced.  In an executable exit() has already run and removed every
   handler before .fini_array, so the call finds nothing to do. */
void *__dso_handle __attribute((visibility("hidden"))) = &__dso_handle;

extern void __cxa_finalize(void *d);

__attribute__((destructor)) static void __tcc_dso_finalize(void)
{
    __cxa_finalize(__dso_handle);
}

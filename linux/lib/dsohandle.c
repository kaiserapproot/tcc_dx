/* __dso_handle for atexit()/__cxa_atexit() from libc_nonshared.a.
   gcc gets it from crtbegin.o, which tcc does not link (as upstream
   tinycc lib/dsohandle.c). */
void *__dso_handle __attribute((visibility("hidden"))) = &__dso_handle;

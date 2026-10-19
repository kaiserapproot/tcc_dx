/* stdarg.h for the Linux x86_64 build of tpp.
   va_list and the __builtin_va_* macros come from tccdefs.h (the
   GCC compatible layout); the old __va_list_struct based header in
   include/stdarg.h does not match it.  Installed over include/stdarg.h
   by linux/Makefile. */
#ifndef _STDARG_H
#define _STDARG_H

typedef __builtin_va_list va_list;
#define va_start __builtin_va_start
#define va_arg __builtin_va_arg
#define va_copy __builtin_va_copy
#define va_end __builtin_va_end

/* fix a buggy dependency on GCC in libio.h */
typedef va_list __gnuc_va_list;
#define _VA_LIST_DEFINED

#endif /* _STDARG_H */

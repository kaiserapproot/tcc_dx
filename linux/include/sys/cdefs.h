/* sys/cdefs.h wrapper for the Linux build of tpp.
   glibc's sys/cdefs.h does "#define __attribute__(xyz)" when __GNUC__ is
   not defined, which silently drops every __attribute__ after the first
   libc include (constructor / destructor never reached .init_array /
   .fini_array).  tcc parses __attribute__ itself, so undo that define.
   Installed by linux/Makefile ahead of /usr/include. */
#include_next <sys/cdefs.h>
#undef __attribute__

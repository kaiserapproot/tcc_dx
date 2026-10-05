/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

/* TCC bodies for x86-64 intrinsics: Interlocked, bit scan, __rdtsc, fences.

   Why this file exists
     psdk_inc/intrin-impl.h is skipped for TCC (GCC builtins and GCC asm), and
     on x86-64 winbase.h has the Interlocked prototypes commented out.  So
     without this file TCC sees no declaration at all: in C the call becomes an
     implicit declaration and fails at link time, in C++ it does not compile.

   Which functions
     The same Interlocked set that intrin-impl.h defines for x86-64:
       32 bit: _InterlockedIncrement, _InterlockedDecrement, _InterlockedAdd,
               _InterlockedExchange, _InterlockedExchangeAdd,
               _InterlockedCompareExchange, _InterlockedAnd, _InterlockedOr,
               _InterlockedXor
       64 bit: the same nine with the 64 suffix
       16 bit: _InterlockedIncrement16, _InterlockedDecrement16,
               _InterlockedCompareExchange16
       pointer: _InterlockedExchangePointer, _InterlockedCompareExchangePointer
     The 64-bit and pointer forms are needed even by code that only uses the
     32-bit ones: in C++ the winbase.h overloads for unsigned types call
     InterlockedIncrement64 and friends.
     Bit scan: _BitScanForward, _BitScanReverse and their 64 forms.
     Time stamp counter: __rdtsc.
     Fences: _mm_lfence, _mm_mfence, _mm_sfence, _mm_pause, __faststorefence,
     and the compiler barriers _ReadWriteBarrier / _ReadBarrier / _WriteBarrier.

   How
     Each read-modify-write is one locked instruction (lock xadd, xchg, lock
     cmpxchg), so it is atomic and a full barrier, as with MSVC.  The barrier
     comes from the locked instruction: TCC accepts the "memory" clobber but
     does not act on it.  And / Or /
     Xor are a lock cmpxchg loop and return the old value.

   Rules (see the __MINGW_INTRIN_INLINE note in _mingw.h)
     - _mingw.h includes this file, so every body here is the FIRST declaration
       of its function in the TU.  Do not put a prototype in front of a body.
     - The functions are static __inline__: they are compiled only where they
       are used, and two TUs never define the same symbol.
     - TCC asm: plain AT&T syntax only (no {x|y} dialects).

   dev\test\a9\sdk_gate.bat checks values, both languages, two TUs and, for
   Interlocked, contention from several threads. */

#ifndef _INTRIN_TCC_H_
#define _INTRIN_TCC_H_

#if defined(__TINYC__) && defined(__x86_64__)

/* 32 bit */

static __inline__ __LONG32 _InterlockedExchangeAdd(__LONG32 volatile *Addend, __LONG32 Value)
{
  __asm__ __volatile__("lock xaddl %0, (%1)" : "+r"(Value) : "r"(Addend) : "memory");
  return Value;
}

static __inline__ __LONG32 _InterlockedAdd(__LONG32 volatile *Addend, __LONG32 Value)
{
  /* The result wraps like the locked add did: unsigned arithmetic, because
     old + Value in signed arithmetic would overflow at LONG_MAX. */
  return (__LONG32)((unsigned __LONG32)_InterlockedExchangeAdd(Addend, Value) + (unsigned __LONG32)Value);
}

static __inline__ __LONG32 _InterlockedIncrement(__LONG32 volatile *Addend)
{
  return (__LONG32)((unsigned __LONG32)_InterlockedExchangeAdd(Addend, 1) + 1U);
}

static __inline__ __LONG32 _InterlockedDecrement(__LONG32 volatile *Addend)
{
  return (__LONG32)((unsigned __LONG32)_InterlockedExchangeAdd(Addend, -1) - 1U);
}

static __inline__ __LONG32 _InterlockedExchange(__LONG32 volatile *Target, __LONG32 Value)
{
  /* xchg with a memory operand is locked without the prefix. */
  __asm__ __volatile__("xchgl %0, (%1)" : "+r"(Value) : "r"(Target) : "memory");
  return Value;
}

static __inline__ __LONG32 _InterlockedCompareExchange(__LONG32 volatile *Destination, __LONG32 ExChange, __LONG32 Comperand)
{
  __LONG32 old;
  __asm__ __volatile__("lock cmpxchgl %2, (%1)" : "=a"(old) : "r"(Destination), "r"(ExChange), "0"(Comperand) : "memory");
  return old;
}

static __inline__ __LONG32 _InterlockedAnd(__LONG32 volatile *Destination, __LONG32 Value)
{
  __LONG32 old;
  do {
    old = *Destination;
  } while (_InterlockedCompareExchange(Destination, old & Value, old) != old);
  return old;
}

static __inline__ __LONG32 _InterlockedOr(__LONG32 volatile *Destination, __LONG32 Value)
{
  __LONG32 old;
  do {
    old = *Destination;
  } while (_InterlockedCompareExchange(Destination, old | Value, old) != old);
  return old;
}

static __inline__ __LONG32 _InterlockedXor(__LONG32 volatile *Destination, __LONG32 Value)
{
  __LONG32 old;
  do {
    old = *Destination;
  } while (_InterlockedCompareExchange(Destination, old ^ Value, old) != old);
  return old;
}

/* 64 bit */

static __inline__ __int64 _InterlockedExchangeAdd64(__int64 volatile *Addend, __int64 Value)
{
  __asm__ __volatile__("lock xaddq %0, (%1)" : "+r"(Value) : "r"(Addend) : "memory");
  return Value;
}

static __inline__ __int64 _InterlockedAdd64(__int64 volatile *Addend, __int64 Value)
{
  /* unsigned for the same reason as _InterlockedAdd */
  return (__int64)((unsigned __int64)_InterlockedExchangeAdd64(Addend, Value) + (unsigned __int64)Value);
}

static __inline__ __int64 _InterlockedIncrement64(__int64 volatile *Addend)
{
  return (__int64)((unsigned __int64)_InterlockedExchangeAdd64(Addend, 1) + 1U);
}

static __inline__ __int64 _InterlockedDecrement64(__int64 volatile *Addend)
{
  return (__int64)((unsigned __int64)_InterlockedExchangeAdd64(Addend, -1) - 1U);
}

static __inline__ __int64 _InterlockedExchange64(__int64 volatile *Target, __int64 Value)
{
  __asm__ __volatile__("xchgq %0, (%1)" : "+r"(Value) : "r"(Target) : "memory");
  return Value;
}

static __inline__ __int64 _InterlockedCompareExchange64(__int64 volatile *Destination, __int64 ExChange, __int64 Comperand)
{
  __int64 old;
  __asm__ __volatile__("lock cmpxchgq %2, (%1)" : "=a"(old) : "r"(Destination), "r"(ExChange), "0"(Comperand) : "memory");
  return old;
}

static __inline__ __int64 _InterlockedAnd64(__int64 volatile *Destination, __int64 Value)
{
  __int64 old;
  do {
    old = *Destination;
  } while (_InterlockedCompareExchange64(Destination, old & Value, old) != old);
  return old;
}

static __inline__ __int64 _InterlockedOr64(__int64 volatile *Destination, __int64 Value)
{
  __int64 old;
  do {
    old = *Destination;
  } while (_InterlockedCompareExchange64(Destination, old | Value, old) != old);
  return old;
}

static __inline__ __int64 _InterlockedXor64(__int64 volatile *Destination, __int64 Value)
{
  __int64 old;
  do {
    old = *Destination;
  } while (_InterlockedCompareExchange64(Destination, old ^ Value, old) != old);
  return old;
}

/* 16 bit */

static __inline__ short _InterlockedCompareExchange16(short volatile *Destination, short ExChange, short Comperand)
{
  short old;
  __asm__ __volatile__("lock cmpxchgw %2, (%1)" : "=a"(old) : "r"(Destination), "r"(ExChange), "0"(Comperand) : "memory");
  return old;
}

static __inline__ short _InterlockedIncrement16(short volatile *Addend)
{
  short v = 1;
  __asm__ __volatile__("lock xaddw %0, (%1)" : "+r"(v) : "r"(Addend) : "memory");
  return (short)(unsigned short)((unsigned short)v + 1U);
}

static __inline__ short _InterlockedDecrement16(short volatile *Addend)
{
  short v = -1;
  __asm__ __volatile__("lock xaddw %0, (%1)" : "+r"(v) : "r"(Addend) : "memory");
  return (short)(unsigned short)((unsigned short)v - 1U);
}

/* pointer (8 bytes on x86-64) */

static __inline__ void *_InterlockedExchangePointer(void *volatile *Target, void *Value)
{
  __asm__ __volatile__("xchgq %0, (%1)" : "+r"(Value) : "r"(Target) : "memory");
  return Value;
}

static __inline__ void *_InterlockedCompareExchangePointer(void *volatile *Destination, void *ExChange, void *Comperand)
{
  void *old;
  __asm__ __volatile__("lock cmpxchgq %2, (%1)" : "=a"(old) : "r"(Destination), "r"(ExChange), "0"(Comperand) : "memory");
  return old;
}

/* bit scan.  MSVC leaves *Index undefined when Mask is 0; here it is not
   written at all, and the result is 0.  bsf / bsr leave the destination
   undefined for a zero source, so Mask is tested before the instruction. */

static __inline__ unsigned char _BitScanForward(unsigned __LONG32 *Index, unsigned __LONG32 Mask)
{
  unsigned __LONG32 n;
  if (!Mask)
    return 0;
  __asm__("bsfl %1, %0" : "=r"(n) : "r"(Mask));
  *Index = n;
  return 1;
}

static __inline__ unsigned char _BitScanReverse(unsigned __LONG32 *Index, unsigned __LONG32 Mask)
{
  unsigned __LONG32 n;
  if (!Mask)
    return 0;
  __asm__("bsrl %1, %0" : "=r"(n) : "r"(Mask));
  *Index = n;
  return 1;
}

static __inline__ unsigned char _BitScanForward64(unsigned __LONG32 *Index, unsigned __int64 Mask)
{
  unsigned __int64 n;
  if (!Mask)
    return 0;
  __asm__("bsfq %1, %0" : "=r"(n) : "r"(Mask));
  *Index = (unsigned __LONG32)n;
  return 1;
}

static __inline__ unsigned char _BitScanReverse64(unsigned __LONG32 *Index, unsigned __int64 Mask)
{
  unsigned __int64 n;
  if (!Mask)
    return 0;
  __asm__("bsrq %1, %0" : "=r"(n) : "r"(Mask));
  *Index = (unsigned __LONG32)n;
  return 1;
}

/* time stamp counter (edx:eax).  Not serializing, as with MSVC. */

static __inline__ unsigned __int64 __rdtsc(void)
{
  unsigned __LONG32 lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((unsigned __int64)hi << 32) | lo;
}
/* fences and pause.  _mm_* is the MSVC / SSE2 spelling; winnt.h maps
   MemoryBarrier, MemoryFence, LoadFence, StoreFence, FastFence and
   YieldProcessor onto these names.  dev\include has no xmmintrin.h /
   emmintrin.h, and TCC does not define __SSE__, so nothing else defines them. */

static __inline__ void _mm_lfence(void)
{
  __asm__ __volatile__("lfence" : : : "memory");
}

static __inline__ void _mm_mfence(void)
{
  __asm__ __volatile__("mfence" : : : "memory");
}

static __inline__ void _mm_sfence(void)
{
  __asm__ __volatile__("sfence" : : : "memory");
}

static __inline__ void _mm_pause(void)
{
  __asm__ __volatile__("pause");
}

/* A full fence, as MSVC documents it (loads and stores before it are
   globally visible before any after it), done the MSVC way with a locked
   no-op on the stack.  mingw's intrin-impl.h uses sfence here, which does
   not order a store before a later load. */
static __inline__ void __faststorefence(void)
{
  __asm__ __volatile__("lock orl $0, (%%rsp)" : : : "memory");
}

/* Compiler barriers, the same macros as mingw's intrin-impl.h.  TCC accepts
   the "memory" clobber without acting on it; it does not move memory
   accesses across statements, so the empty asm is enough. */
#ifndef _ReadWriteBarrier
#define _ReadWriteBarrier() __asm__ __volatile__ ("" ::: "memory")
#define _ReadBarrier _ReadWriteBarrier
#define _WriteBarrier _ReadWriteBarrier
#endif
#endif /* defined(__TINYC__) && defined(__x86_64__) */

#endif /* _INTRIN_TCC_H_ */

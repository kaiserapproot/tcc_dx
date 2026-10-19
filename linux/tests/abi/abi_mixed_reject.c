/* Structs whose two eightbytes have different SysV classes (one SSE, one
   INTEGER).  tpp cannot pass or return them the way gcc does, so each use
   by value must be a compile error (run_tests.sh Phase 7 compiles every
   CASE separately and checks the message).  Passing by pointer is fine. */
typedef struct { double d; long l; } SDL;          /* SSE, INTEGER */
typedef struct { int id; float x, y, z; } SIXYZ;   /* INTEGER, SSE */
typedef struct { int i[2]; float f[2]; } SIF;      /* INTEGER, SSE */

#if CASE == 1
double take(SDL s) { return s.d + s.l; }
#elif CASE == 2
SDL make(void) { SDL s; s.d = 1; s.l = 2; return s; }
#elif CASE == 3
double take(SIXYZ s);
double call(SIXYZ *p) { return take(*p); }
#elif CASE == 4
SIF make(void);
float call(void) { return make().f[0]; }
#elif CASE == 0
/* accepted: by pointer, and an 8-byte float+int struct (one INTEGER
   eightbyte, same as gcc) */
typedef struct { float f; int i; } SFI;
double by_ptr(SDL *s) { return s->d + s->l; }
int by_val_ok(SFI s) { return s.i; }
#endif
int main(void) { return 0; }

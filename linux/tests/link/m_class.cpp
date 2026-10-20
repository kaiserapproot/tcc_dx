/* multi-file C++ link: class defined here, used from m_main.cpp;
   global object with ctor + dtor; overloaded free functions */
#include <stdio.h>
#include "m_class.h"
Counter::Counter(int start) { n = start; }
Counter::~Counter() { fputs("M_GLOBAL_DTOR\n", stderr); }
int Counter::bump(int by) { n += by; return n; }
int twice(int x) { return x * 2; }
double twice(double x) { return x * 2.0 + 0.5; }
Counter g_counter(40);

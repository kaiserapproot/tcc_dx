// The reason the rule exists: two C++ TUs that both include windows.h must
// link.  The SDK's `EXTERN_C const GUID name;` lines are single-declaration
// forms and must not emit a zero-filled copy per TU.
#include <windows.h>
#include <intrin.h>
int dcl_w2_value(void);
int main() { return dcl_w2_value() != 5; }

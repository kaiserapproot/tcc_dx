#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

struct opaque_value;
typedef struct opaque_value *opaque_ptr;

struct opaque_holder
{
    opaque_ptr value;
};

int main()
{
    _locale_t locale = 0;
    opaque_holder holder;
    holder.value = 0;
    return locale != 0 || holder.value != 0;
}

#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

struct opaque_value { int x; };
typedef struct opaque_value *opaque_ptr;

struct opaque_holder
{
    opaque_ptr value;
};

int main() {
    opaque_holder holder;
    holder.value = 0;
    return holder.value != 0;
}

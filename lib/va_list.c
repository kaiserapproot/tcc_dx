/* va_list.c - tinycc support for va_list on X86_64 */

#if defined __x86_64__

/* Avoid include files, they may not be available when cross compiling */
extern void *memset(void *s, int c, __SIZE_TYPE__ n);
extern void *memcpy(void *d, const void *s, __SIZE_TYPE__ n);
extern void abort(void) __attribute__((noreturn));

/* This should be in sync with our include/stdarg.h */
enum __va_arg_type {
    __va_gen_reg, __va_float_reg, __va_stack
};

/* __builtin_va_list is the GCC compatible va_list from tccdefs.h.
   The declaration of __va_arg must match the one there. */

/* Kept for objects built with the old __va_list_struct based stdarg.h,
   whose va_start called this (same layout as __builtin_va_list). */
void __va_start(__builtin_va_list ap, void *fp)
{
    memset(ap, 0, sizeof(*ap));
    memcpy(ap, (char *)fp - 16, sizeof(*ap));
    ap->overflow_arg_area = (char *)fp + ap->overflow_offset;
    ap->reg_save_area = (char *)fp - 176 - 16;
}

void *__va_arg(__builtin_va_list ap,
               int arg_type,
               int size, int align)
{
    size = (size + 7) & ~7;
    align = (align + 7) & ~7;
    switch ((enum __va_arg_type)arg_type) {
    case __va_gen_reg:
        if (ap->gp_offset + size <= 48) {
            ap->gp_offset += size;
            return ap->reg_save_area + ap->gp_offset - size;
        }
        goto use_overflow_area;

    case __va_float_reg:
        if (ap->fp_offset < 128 + 48) {
            ap->fp_offset += 16;
            return ap->reg_save_area + ap->fp_offset - 16;
        }
        size = 8;
        goto use_overflow_area;

    case __va_stack:
    use_overflow_area:
        ap->overflow_arg_area += size;
        ap->overflow_arg_area = (char*)((long long)(ap->overflow_arg_area + align - 1) & -align);
        return ap->overflow_arg_area - size;

    default: /* should never happen */
        abort();
    }
}
#endif

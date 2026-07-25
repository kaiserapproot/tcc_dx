/*
 *  TCC - Tiny C Compiler
 *
 *  Copyright (c) 2001-2004 Fabrice Bellard
 *
 * ���̃��C�u�����̓t���[�\�t�g�E�F�A�ł��B�ĔЕz�����/�܂���
 * �C���́AFree Software Foundation �ɂ���Č��J���ꂽ GNU Lesser
 * General Public License �̏����i�o�[�W����2 �܂��́i�I���ɂ��j����ȍ~�j
 * �ɏ]���čs�����Ƃ��ł��܂��B
 *
 * ���̃��C�u�����͗L�p�ł��邱�Ƃ�ړI�Ƃ��Ĕz�z����܂����A
 * ���i�������̖ړI�ւ̓K�������܂ޖ����I�܂��َ͖��I�ȕۏ؂͂���܂���B
 * �ڍׂ� GNU Lesser General Public License ���Q�Ƃ��Ă��������B
 *
 * �{���C�u�����ƂƂ��� GNU Lesser General Public License �̎ʂ���
 * �z�z����Ă���͂��ł��B�z�z����Ă��Ȃ��ꍇ�́AFree Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * �܂ł��₢���킹���������B
 */

#define USING_GLOBALS
#include "tcc.h"

 /********************************************************/
 /* �O���[�o���ϐ� */

 /* loc : ���[�J���ϐ��̃C���f�b�N�X
     ind : �o�̓R�[�h�̃C���f�b�N�X
     rsym: �߂�l�p�V���{��
     anon_sym: �����V���{���̃C���f�b�N�X
 */
ST_DATA int rsym, anon_sym, ind, loc;

ST_DATA Sym* global_stack;
ST_DATA Sym* local_stack;
ST_DATA Sym* define_stack;
ST_DATA Sym* global_label_stack;
ST_DATA Sym* local_label_stack;

static Sym* sym_free_first;
static void** sym_pools;
static int nb_sym_pools;

static Sym* all_cleanups, * pending_gotos;
static int local_scope;
ST_DATA char debug_modes;

ST_DATA SValue* vtop;
static SValue _vstack[1 + VSTACK_SIZE];
#define vstack (_vstack + 1)

ST_DATA int nocode_wanted; /* �R�[�h������}������t���O */
#define NODATA_WANTED (nocode_wanted > 0) /* �ÓI�f�[�^�o�͂��s�v�ł��邱�Ƃ��Ӗ����� */
#define DATA_ONLY_WANTED 0x80000000 /* �֐��O����ѐÓI�������q��ON�ɂȂ� */

/* if (0) �̂悤�Ȗ������W�����v�̌�̓R�[�h�o�͂��s��Ȃ� */
#define CODE_OFF_BIT 0x20000000
#define CODE_OFF() if(!nocode_wanted)(nocode_wanted |= CODE_OFF_BIT)
#define CODE_ON() (nocode_wanted &= ~CODE_OFF_BIT)

#define NOEVAL_MASK 0x0000FFFF
#define NOEVAL_WANTED (nocode_wanted & NOEVAL_MASK)

/* sizeof()/typeof() ������͂��Ă���Ԃ̓R�[�h�o�͂��s��Ȃ��inocode_wanted++/-- ���g�p�j */
#define NOEVAL_MASK 0x0000FFFF
#define NOEVAL_WANTED (nocode_wanted & NOEVAL_MASK)

#define CONST_WANTED_BIT  0x00010000
#define CONST_WANTED_MASK 0x0FFF0000
#define CONST_WANTED  (nocode_wanted & CONST_WANTED_MASK)

/* �萔������͂��Ă���Ԃ̓R�[�h�o�͂��s��Ȃ� */
#define CONST_WANTED_BIT  0x00010000
#define CONST_WANTED_MASK 0x0FFF0000
#define CONST_WANTED  (nocode_wanted & CONST_WANTED_MASK)

ST_DATA int global_expr;  /* �������e�������O���[�o���Ɋ��蓖�Ă�K�v������ꍇ�ɐ^�i�������q��͎��Ɏg�p�j */
ST_DATA CType func_vt; /* ���݂̊֐��̖߂�l�^�ireturn ���߂Ŏg�p�j */
ST_DATA int func_var; /* ���݂̊֐����ϒ��������ǂ����ireturn ���߂Ŏg�p�j */
ST_DATA int func_vc;
ST_DATA int func_ind;
ST_DATA const char* funcname;
ST_DATA CType int_type, func_old_type, char_type, char_pointer_type;
static CString initstr;

#if PTR_SIZE == 4
#define VT_SIZE_T (VT_INT | VT_UNSIGNED)
#define VT_PTRDIFF_T VT_INT
#elif LONG_SIZE == 4
#define VT_SIZE_T (VT_LLONG | VT_UNSIGNED)
#define VT_PTRDIFF_T VT_LLONG
#else
#define VT_SIZE_T (VT_LONG | VT_LLONG | VT_UNSIGNED)
#define VT_PTRDIFF_T (VT_LONG | VT_LLONG)
#endif

static struct switch_t {
    struct case_t {
        int64_t v1, v2;
        int ind, line;
    } **p; int n; /* case �͈͂̃��X�g */
    int def_sym; /* �f�t�H���g�̃V���{�� */
    int nocode_wanted;
    int* bsym;
    struct scope* scope;
    struct switch_t* prev;
    SValue sv;
} *cur_switch; /* ���݂� switch �\���� */

#define MAX_TEMP_LOCAL_VARIABLE_NUMBER 8
/* ���݂̊֐��ł̃X�^�b�N��̈ꎞ���[�J���ϐ��̈ꗗ */
static struct temp_local_variable {
    int location; // �X�^�b�N��̃I�t�Z�b�g�BSValue.c.i
    short size;
    short align;
} arr_temp_local_vars[MAX_TEMP_LOCAL_VARIABLE_NUMBER];
static int nb_temp_local_vars;

static struct scope {
    struct scope* prev;
    struct { int loc, locorig, num; } vla;
    struct { Sym* s; int n; } cl;
    int* bsym, * csym;
    Sym* lstk, * llstk;
} *cur_scope, * loop_scope, * root_scope;

typedef struct {
    Section* sec;
    int local_offset;
    Sym* flex_array_ref;
} init_params;

#if 1
#define precedence_parser
static void init_prec(void);
#endif

static void block(int flags);
#define STMT_EXPR 1
#define STMT_COMPOUND 2

static void gen_cast(CType* type);
static void gen_cast_s(int t);
static inline CType* pointed_type(CType* type);
static int is_compatible_types(CType* type1, CType* type2);
static int parse_btype(CType* type, AttributeDef* ad, int ignore_label);
static CType* type_decl(CType* type, AttributeDef* ad, int* v, int td);
static void parse_expr_type(CType* type);
static void init_putv(init_params* p, CType* type, unsigned long c);
static void decl_initializer(init_params* p, CType* type, unsigned long c, int flags);
static void decl_initializer_alloc(CType* type, AttributeDef* ad, int r, int has_init, int v, int scope);
static int decl(int l);
static void expr_eq(void);
static void vpush_type_size(CType* type, int* a);
static int is_compatible_unqualified_types(CType* type1, CType* type2);
static inline int64_t expr_const64(void);
static void vpush64(int ty, unsigned long long v);
static void vpush(CType* type);
static int gvtst(int inv, int t);
static void gen_inline_functions(TCCState* s);
static void free_inline_functions(TCCState* s);
static void skip_or_save_block(TokenString** str);
static void gv_dup(void);
static int get_temp_local_var(int size, int align, int* r2);
static void cast_error(CType* st, CType* dt);
static int cpp_can_bind_lvalue_to_reference(CType *ref, CType *arg);
static int is_integer_btype(int bt);
/* --- C++ Stage 2: mangling, references, qualified names --- */
static Sym *cpp_qualified_class;
static Sym *cpp_cur_class;
static int decl_once_flag;
/* --- C++ Stage 3: this, member calls, default args --- */
static SValue cpp_member_this;
static int cpp_member_this_pending;
static Sym *cpp_this_sym;
static Sym *cpp_cur_func_class;
static Sym *external_sym(int v, CType *type, int r, AttributeDef *ad);
static void gfunc_param_typed(Sym *func, Sym *arg);
static int cpp_func_param_count(Sym *field);
static void cpp_apply_default_args(Sym *func, int *pnb_args, Sym **psa);
ST_FUNC void greloca(Section *s, Sym *sym, unsigned long offset, int type, addr_t addend);

static void mangle_clamp_pos(int buf_size, int *pos)
{
    if (*pos >= buf_size)
        *pos = buf_size - 1;
}

static void mangle_append_char(char *buf, int buf_size, int *pos, char c)
{
    if (*pos + 1 < buf_size)
        buf[(*pos)++] = c;
    mangle_clamp_pos(buf_size, pos);
}

static void mangle_append_type(CType *t, char *buf, int buf_size, int *pos)
{
    CType ty;
    int bt, wrote, nlen;
    char c;
    const char *sn;

    ty = *t;
    while ((ty.t & VT_BTYPE) == VT_PTR) {
        mangle_append_char(buf, buf_size, pos,
            (ty.t & VT_MPTR) ? 'm' :
            (ty.t & VT_REFERENCE) ? 'r' : 'p');
        ty = *pointed_type(&ty);
    }
    bt = ty.t & VT_BTYPE;
    c = 'x';
    switch (bt) {
    case VT_VOID: c = 'v'; break;
    case VT_BYTE:
        c = (ty.t & VT_UNSIGNED) ? 'h' : 'c';
        break;
    case VT_SHORT:
        c = (ty.t & VT_UNSIGNED) ? 't' : 's';
        break;
    case VT_INT:
        if (ty.t & VT_LONG)
            c = 'l';
        else if (ty.t & VT_UNSIGNED)
            c = 'u';
        else
            c = 'i';
        break;
    case VT_LLONG: c = 'L'; break;
    case VT_FLOAT: c = 'f'; break;
    case VT_DOUBLE: c = 'd'; break;
    case VT_LDOUBLE: c = 'e'; break;
    case VT_BOOL: c = 'b'; break;
    case VT_STRUCT:
        sn = get_tok_str(ty.ref->v & ~SYM_STRUCT, NULL);
        nlen = (int)strlen(sn);
        wrote = snprintf(buf + *pos, buf_size - *pos, "S%d_%s", nlen, sn);
        if (wrote < 0 || wrote >= buf_size - *pos) {
            mangle_clamp_pos(buf_size, pos);
            return;
        }
        *pos += wrote;
        mangle_clamp_pos(buf_size, pos);
        return;
    default:
        break;
    }
    mangle_append_char(buf, buf_size, pos, c);
}

/* BUG-11: the one-token lookahead after an extern "C" region is lexed
   while lex_c is still active, so a C++ keyword there (e.g. "class")
   arrives as its demoted identifier twin and the following declaration
   fails to parse.  The twin is registered in table_ident but never
   linked into the hash chain (tok_alloc_demote), so a lookup by name
   returns the original keyword token: use that to re-promote.  Plain
   identifiers map to themselves and stay untouched. */
static void cpp_repromote_stale_lookahead(void)
{
    const char *name;
    TokenSym *ts;

    if (!tcc_state->cpp || tcc_state->lex_c)
        return;
    if (tok < TOK_IDENT)
        return;
    name = get_tok_str(tok, NULL);
    if (!name || !name[0])
        return;
    ts = tok_alloc(name, (int)strlen(name));
    if (ts->tok != tok)
        tok = ts->tok;
}

static int cpp_build_func_mangle(int v, CType *type, char *mbuf, int buf_size)
{
    const char *name;
    Sym *arg;
    int pos;

    if (!tcc_state->cpp || tcc_state->extern_c)
        return 0;
    if (!type || (type->t & VT_BTYPE) != VT_FUNC)
        return 0;
    name = get_tok_str(v, NULL);
    pos = snprintf(mbuf, buf_size, "__tcc_%s_", name);
    if (pos < 0 || pos >= buf_size)
        pos = buf_size - 1;
    for (arg = type->ref->next; arg; arg = arg->next) {
        if (arg->type.t == VT_VOID)
            break;
        mangle_append_type(&arg->type, mbuf, buf_size, &pos);
    }
    if (type->ref->f.func_const) {
        mangle_append_char(mbuf, buf_size, &pos, '_');
        mangle_append_char(mbuf, buf_size, &pos, 'C');
    }
    mangle_clamp_pos(buf_size, &pos);
    mbuf[pos] = '\0';
    return pos;
}

static int cpp_build_call_mangle(int v, int nb_args, char *mbuf, int buf_size)
{
    int pos, i;

    pos = snprintf(mbuf, buf_size, "__tcc_%s_", get_tok_str(v, NULL));
    if (pos < 0 || pos >= buf_size)
        pos = buf_size - 1;
    for (i = nb_args; i > 0; i--)
        mangle_append_type(&vtop[-nb_args + i].type, mbuf, buf_size, &pos);
    mangle_clamp_pos(buf_size, &pos);
    mbuf[pos] = '\0';
    return pos;
}

static int cpp_arg_matches_param(CType *param, CType *arg, int *score_out)
{
    int p_bt, a_bt;
    CType *pt;

    /* Shared by cpp_resolve_func_call / cpp_resolve_free_func_call.
     * Struct lvalue may bind to T& / const T& (score 10); this extends
     * cpp_resolve_func_call beyond is_compatible_types exact-match only. */
    if (is_compatible_types(param, arg)) {
        *score_out = 10;
        return 1;
    }
    if (tcc_state->cpp && (param->t & VT_REFERENCE)
        && (arg->t & VT_BTYPE) == VT_STRUCT) {
        pt = pointed_type(param);
        if (is_compatible_unqualified_types(pt, arg)) {
            *score_out = 10;
            return 1;
        }
    }
    p_bt = param->t & VT_BTYPE;
    a_bt = arg->t & VT_BTYPE;
    if ((is_float(p_bt) || is_integer_btype(p_bt)) &&
        (is_float(a_bt) || is_integer_btype(a_bt))) {
        *score_out = 1;
        return 1;
    }
    if (p_bt == VT_PTR && a_bt == VT_PTR) {
        *score_out = 1;
        return 1;
    }
    return 0;
}

/* C++: does the bound function sym belong to an overload set (>= 2
   candidates with same class and const-ness)?  Used to defer argument
   conversion until the overload is resolved from the raw arg types. */
static int cpp_call_has_overloads(Sym *cur)
{
    Sym *s;
    int n;

    if (!cur || (cur->type.t & VT_BTYPE) != VT_FUNC)
        return 0;
    n = 0;
    for (s = sym_find(cur->v); s; s = s->prev_tok) {
        if ((s->type.t & VT_BTYPE) != VT_FUNC)
            continue;
        if (s->parent_class != cur->parent_class)
            continue;
        if (!!(s->type.ref && s->type.ref->f.func_const)
            != !!(cur->type.ref && cur->type.ref->f.func_const))
            continue;
        n++;
        if (n >= 2)
            return 1;
    }
    return 0;
}

static Sym *cpp_resolve_func_call(int v, int nb_args, Sym *cur)
{
    Sym *s, *best = NULL;
    int best_score = -1;

    if (!tcc_state->cpp || tcc_state->extern_c)
        return sym_find(v);

    for (s = sym_find(v); s; s = s->prev_tok) {
        Sym *p;
        int i, score = 0, match = 1;

        if ((s->type.t & VT_BTYPE) != VT_FUNC)
            continue;
        /* BUG-7: re-resolution must not cross class boundaries nor
           drop the const-ness already chosen by member lookup */
        if (cur) {
            if (s->parent_class != cur->parent_class)
                continue;
            if (!!(s->type.ref && s->type.ref->f.func_const)
                != !!(cur->type.ref && cur->type.ref->f.func_const))
                continue;
        }

        p = s->type.ref->next;
        for (i = 0; i < nb_args; i++) {
            CType *arg_type = &vtop[-nb_args + 1 + i].type;
            int arg_score;

            if (!p || p->type.t == VT_VOID) {
                if (s->type.ref->f.func_type == FUNC_ELLIPSIS)
                    break;
                match = 0;
                break;
            }

            if (cpp_arg_matches_param(&p->type, arg_type, &arg_score)) {
                score += arg_score;
            } else {
                match = 0;
                break;
            }
            p = p->next;
        }

        if (match && p && p->type.t != VT_VOID)
            match = 0;

        if (match && score > best_score) {
            best_score = score;
            best = s;
        }
    }

    if (best)
        return best;
    return sym_find(v);
}

static Sym *cpp_resolve_free_func_call(int v, int nb_args)
{
    Sym *s, *best = NULL;
    int best_score = -1;

    if (!tcc_state->cpp || tcc_state->extern_c)
        return NULL;

    for (s = sym_find(v); s; s = s->prev_tok) {
        Sym *p;
        int i, score = 0, match = 1;

        if ((s->type.t & VT_BTYPE) != VT_FUNC)
            continue;
        if (s->parent_class)
            continue;

        p = s->type.ref->next;
        for (i = 0; i < nb_args; i++) {
            CType *arg_type = &vtop[-nb_args + 1 + i].type;
            int arg_score;

            if (!p || p->type.t == VT_VOID) {
                if (s->type.ref->f.func_type == FUNC_ELLIPSIS)
                    break;
                match = 0;
                break;
            }

            if (cpp_arg_matches_param(&p->type, arg_type, &arg_score)) {
                score += arg_score;
            } else {
                match = 0;
                break;
            }
            p = p->next;
        }

        if (match && p && p->type.t != VT_VOID)
            match = 0;

        if (match && score > best_score) {
            best_score = score;
            best = s;
        }
    }

    return best;
}

static void cpp_set_func_mangle_label(Sym *sym, CType *type)
{
    char mbuf[256];
    TokenSym *ts;
    int len;
    const char *entry_name;

    if (!tcc_state->cpp || tcc_state->extern_c)
        return;
    if (!sym || (type->t & VT_BTYPE) != VT_FUNC)
        return;
    entry_name = get_tok_str(sym->v, NULL);
    if (!strcmp(entry_name, "main") || !strcmp(entry_name, "wmain"))
        return;
    len = cpp_build_func_mangle(sym->v, type, mbuf, sizeof mbuf);
    if (len <= 0)
        return;
    ts = tok_alloc(mbuf, len);
    sym->asm_label = ts->tok;
}

static int cpp_dtor_name_tok(int class_tok);

static int parse_cpp_scope_qualifier(int *v)
{
    int class_v;

    if (!tcc_state->cpp || tok != ':')
        return 0;
    next();
    if (tok != ':') {
        unget_tok(':');
        return 0;
    }
    next();
    class_v = *v;
    /* FEAT-4E-P2: Class::~Class out-of-class dtor definition.  '~' is
       a single-char token, so the generic member-name check below
       would reject it.  Reuse the FEAT-4E mangled global token so the
       existing auto/explicit dtor call paths link against this body. */
    if (tok == '~') {
        next();
        if (tok != class_v)
            tcc_error("destructor name does not match class name");
        next();
        *v = cpp_dtor_name_tok(class_v);
        if (!*v)
            tcc_error("cannot build destructor name");
        cpp_qualified_class = struct_find(class_v);
        if (!cpp_qualified_class)
            tcc_error("unknown class in qualified name");
        return 1;
    }
    if (tok < TOK_IDENT)
        tcc_error("expected member name after ::");
    *v = tok;
    next();
    cpp_qualified_class = struct_find(class_v);
    if (!cpp_qualified_class)
        tcc_error("unknown class in qualified name");
    return 1;
}

/* If tok is Class::member, unget tokens for expression parsing. */
static int cpp_unget_scoped_expr(void)
{
    int cls_tok, mem_tok;

    if (!tcc_state->cpp || tok < TOK_IDENT)
        return 0;
    cls_tok = tok;
    next();
    if (tok != ':') {
        unget_tok(cls_tok);
        return 0;
    }
    next();
    /* Need a SECOND ':' to form `::`.  A single ':' here is a label
     * (e.g., `foo:` goto target), not a scope qualifier — fall back
     * to the regular decl/statement path. */
    if (tok != ':') {
        unget_tok(':');
        unget_tok(cls_tok);
        return 0;
    }
    next();
    if (!(tok >= TOK_IDENT)) {
        unget_tok(':');
        unget_tok(':');
        unget_tok(cls_tok);
        return 0;
    }
    mem_tok = tok;
    (void)mem_tok;  /* tok is already mem_tok; unget_tok auto-saves it. */
    /* Restore the token stream as: cls_tok, ':', ':', mem_tok, <rest>.
     * Each unget_tok auto-saves the current tok before overwriting it,
     * so unget'ing mem_tok explicitly here would push count TWICE. */
    unget_tok(':');
    unget_tok(':');
    unget_tok(cls_tok);
    return 1;
}

static int cpp_unget_scoped_after_class_tok(int cls_tok)
{
    int mem_tok;

    if (!tcc_state->cpp || tok != ':')
        return 0;
    next();
    if (tok != ':') {
        unget_tok(':');
        return 0;
    }
    next();
    if (!(tok >= TOK_IDENT)) {
        unget_tok(':');
        unget_tok(':');
        return 0;
    }
    mem_tok = tok;
    unget_tok(mem_tok);
    unget_tok(':');
    unget_tok(':');
    unget_tok(cls_tok);
    return 1;
}

static Sym *cpp_lookup_static_member(Sym *class_sym, int mem_v)
{
    Sym *f, *s;
    int v1;

    if (!class_sym)
        return NULL;
    v1 = mem_v | SYM_FIELD;
    for (f = class_sym->next; f; f = f->next) {
        if (f->v == v1 && (f->type.t & VT_STATIC))
            break;
    }
    if (!f)
        return NULL;
    for (s = sym_find(mem_v); s; s = s->prev_tok) {
        if ((f->type.t & VT_BTYPE) == VT_FUNC) {
            if ((s->type.t & VT_BTYPE) == VT_FUNC && s->parent_class == class_sym)
                return s;
        } else {
            if ((s->type.t & VT_BTYPE) != VT_FUNC && !(s->v & SYM_FIELD))
                return s;
                return s;
        }
    }
    return NULL;
}
/* A C++ ctor member has the same identifier as the class itself, which
 * already lives as a typedef in the global identifier chain.  Storing
 * the ctor as a global Sym under that token would either collide with
 * the typedef ("redefinition of incompatible type") or hide it from
 * later `Foo f;` declarations.  Instead we generate a distinct token
 * `__cpp_ctor_<Class>` for the global Sym.  Lookups (`f.Foo()`) go
 * through cpp_lookup_member_func, which translates Foo→__cpp_ctor_Foo. */
static int cpp_ctor_name_tok(int class_tok)
{
    char buf[256];
    int len;
    TokenSym *ts;
    len = snprintf(buf, sizeof buf, "__cpp_ctor_%s",
                   get_tok_str(class_tok, NULL));
    if (len <= 0 || len >= (int)sizeof buf)
        return 0;
    ts = tok_alloc(buf, len);
    return ts->tok;
}

/* Internal class-member token for the destructor field (distinct from the
 * ctor field, which reuses the class name token). */
static int cpp_dtor_field_tok(int class_tok)
{
    char buf[256];
    int len;
    TokenSym *ts;
    len = snprintf(buf, sizeof buf, "__cpp_dtor_fld_%s",
                   get_tok_str(class_tok, NULL));
    if (len <= 0 || len >= (int)sizeof buf)
        return 0;
    ts = tok_alloc(buf, len);
    return ts->tok;
}

static int cpp_dtor_name_tok(int class_tok)
{
    char buf[256];
    int len;
    TokenSym *ts;
    len = snprintf(buf, sizeof buf, "__cpp_dtor_%s",
                   get_tok_str(class_tok, NULL));
    if (len <= 0 || len >= (int)sizeof buf)
        return 0;
    ts = tok_alloc(buf, len);
    return ts->tok;
}

/* FEAT-6A: operator overload helpers */
static const char *cpp_operator_suffix(int op_tok)
{
    switch (op_tok) {
    case '+': return "plus";
    case '-': return "minus";
    case '*': return "mul";
    case '/': return "div";
    case '[': return "index";
    case '=': return "assign";
    case TOK_A_ADD: return "plus_assign";
    case TOK_A_SUB: return "minus_assign";
    case TOK_A_MUL: return "mul_assign";
    case TOK_A_DIV: return "div_assign";
    case '!': return "not";
    case '~': return "compl";
    case TOK_INC: return "inc";
    case TOK_DEC: return "dec";
    default:
        return NULL;
    }
}

static int cpp_operator_field_tok(int op_tok)
{
    char buf[64];
    int len;
    TokenSym *ts;
    const char *suffix;

    suffix = cpp_operator_suffix(op_tok);
    if (!suffix)
        return 0;
    len = snprintf(buf, sizeof buf, "__cpp_op_%s", suffix);
    if (len <= 0 || len >= (int)sizeof buf)
        return 0;
    ts = tok_alloc(buf, len);
    return ts->tok;
}

static int cpp_parse_operator_token(void)
{
    int op;

    if (tok == '[') {
        next();
        skip(']');
        return '[';
    }
    op = tok;
    if (!cpp_operator_suffix(op))
        tcc_error("unsupported operator");
    next();
    return op;
}

static void cpp_parse_operator_decl_name(int *v)
{
    int op;

    if (tok != TOK_OPERATOR)
        tcc_error("operator expected");
    next();
    op = cpp_parse_operator_token();
    *v = cpp_operator_field_tok(op);
    if (!*v)
        tcc_error("unsupported operator");
}

static int cpp_is_ctor_field(Sym *field)
{
    if (!field || !field->parent_class)
        return 0;
    if ((field->type.t & VT_BTYPE) != VT_FUNC)
        return 0;
    return (field->v & ~SYM_FIELD)
        == (field->parent_class->v & ~SYM_STRUCT);
}

static int cpp_is_dtor_field(Sym *field)
{
    int fld_tok;

    if (!field || !field->parent_class)
        return 0;
    if ((field->type.t & VT_BTYPE) != VT_FUNC)
        return 0;
    fld_tok = cpp_dtor_field_tok(field->parent_class->v & ~SYM_STRUCT);
    if (!fld_tok)
        return 0;
    return (field->v & ~SYM_FIELD) == fld_tok;
}

static Sym *cpp_lookup_member_func(Sym *field, CType *obj_type)
{
    int v;
    Sym *s, *class_sym;

    if (!field || !obj_type || !obj_type->ref)
        return field;
    class_sym = field->parent_class ? field->parent_class : obj_type->ref;
    if (cpp_is_ctor_field(field)) {
        /* ctor lives under the mangled global token. */
        v = cpp_ctor_name_tok(field->v & ~SYM_FIELD);
        if (!v)
            return field;
    } else if (cpp_is_dtor_field(field)) {
        v = cpp_dtor_name_tok(field->parent_class->v & ~SYM_STRUCT);
        if (!v)
            return field;
    } else {
        v = field->v & ~SYM_FIELD;
    }
    {
        int want_const;

        /* pass 1: full signature match (is_compatible_func via
           is_compatible_types covers return type, param types and
           func_const), so same-arity overloads with different param
           types bind to the right global */
        for (s = sym_find(v); s; s = s->prev_tok) {
            if ((s->type.t & VT_BTYPE) != VT_FUNC)
                continue;
            if (s->parent_class != class_sym)
                continue;
            if (is_compatible_types(&s->type, &field->type))
                return s;
        }
        /* pass 2 (fallback): const + arity filter for syms whose type
           does not compare equal to the field (e.g. internal ctor/dtor
           protos) */
        want_const = field->type.ref && field->type.ref->f.func_const;
        for (s = sym_find(v); s; s = s->prev_tok) {
            if ((s->type.t & VT_BTYPE) != VT_FUNC)
                continue;
            if (s->parent_class != class_sym)
                continue;
            if (!!(s->type.ref && s->type.ref->f.func_const) != !!want_const)
                continue;
            /* arity overloads (e.g. unary vs binary operator-) share the
               same token; pick the global matching the field's arity */
            if (cpp_func_param_count(s) != cpp_func_param_count(field))
                continue;
            return s;
        }
    }
    return field;
}

static Sym *find_field(CType *type, int v, int *cumofs);
static Sym *cpp_lookup_member_field(int v, Sym *class_sym)
{
    CType ct;
    int cumofs;

    if (!class_sym)
        return NULL;
    ct.t = VT_STRUCT;
    ct.ref = class_sym;
    return find_field(&ct, v, &cumofs);
}

/* FEAT-5B: member pointer helpers */
static int cpp_is_member_pointer(CType *type)
{
    return (type->t & (VT_BTYPE | VT_MPTR)) == (VT_PTR | VT_MPTR);
}

static Sym *cpp_mptr_class(CType *type)
{
    if (!cpp_is_member_pointer(type) || !type->ref)
        return NULL;
    return type->ref->parent_class;
}

static int cpp_is_mptr_to_func(CType *type)
{
    if (!cpp_is_member_pointer(type))
        return 0;
    return (pointed_type(type)->t & VT_BTYPE) == VT_FUNC;
}

static void mk_member_pointer(CType *type, Sym *class_sym, int field_tok)
{
    Sym *s;

    s = sym_push(SYM_FIELD, type, 0, field_tok);
    s->parent_class = class_sym;
    type->t = (type->t & VT_STORAGE) | VT_PTR | VT_MPTR;
    type->ref = s;
}

/* Parse Class::* in a declarator; returns 1 if consumed. */
static int cpp_parse_member_pointer(CType *type, CType **ret)
{
    int cls_tok;
    Sym *class_sym;

    if (!tcc_state->cpp || tok < TOK_IDENT)
        return 0;
    cls_tok = tok;
    next();
    if (tok != ':') {
        unget_tok(cls_tok);
        return 0;
    }
    next();
    if (tok != ':') {
        unget_tok(':');
        unget_tok(cls_tok);
        return 0;
    }
    next();
    if (tok != '*') {
        unget_tok(':');
        unget_tok(':');
        unget_tok(cls_tok);
        return 0;
    }
    next();
    class_sym = struct_find(cls_tok);
    if (!class_sym)
        tcc_error("unknown class in member pointer");
    mk_member_pointer(type, class_sym, 0);
    if (ret && *ret == type)
        *ret = pointed_type(type);
    return 1;
}

/* Peek/consume Class::member for &Class::member. */
static int cpp_parse_qualified_member(int *cls_tok, int *mem_tok)
{
    int cls, mem;

    if (!tcc_state->cpp || tok < TOK_IDENT)
        return 0;
    cls = tok;
    next();
    if (tok != ':') {
        unget_tok(cls);
        return 0;
    }
    next();
    if (tok != ':') {
        unget_tok(':');
        unget_tok(cls);
        return 0;
    }
    next();
    if (tok < TOK_IDENT) {
        unget_tok(':');
        unget_tok(':');
        unget_tok(cls);
        return 0;
    }
    mem = tok;
    next();
    *cls_tok = cls;
    *mem_tok = mem;
    return 1;
}

static int cpp_mptr_compatible_class(CType *obj_type, Sym *mptr_class)
{
    Sym *s;

    if (!obj_type || !mptr_class)
        return 0;
    if ((obj_type->t & VT_BTYPE) != VT_STRUCT || !obj_type->ref)
        return 0;
    if (obj_type->ref == mptr_class)
        return 1;
    for (s = obj_type->ref->next; s; s = s->next) {
        if ((s->type.t & VT_BTYPE) == VT_STRUCT
            && s->v >= (SYM_FIRST_ANOM | SYM_FIELD)
            && s->parent_class == mptr_class)
            return 1;
    }
    return 0;
}

static void cpp_emit_mptr_dmp_access(SValue *obj, SValue *pm)
{
    CType mem_type;
    int qualifiers;

    if (!cpp_is_member_pointer(&pm->type) || cpp_is_mptr_to_func(&pm->type))
        tcc_error("invalid data member pointer");
    mem_type = *pointed_type(&pm->type);
    qualifiers = obj->type.t & (VT_CONSTANT | VT_VOLATILE);
    vpushv(obj);
    test_lvalue();
    gaddrof();
    vtop->type = char_pointer_type;
    vpushv(pm);
    if (vtop->r & VT_LVAL)
        gv(RC_INT);
    vtop->type = int_type;
    gen_op('+');
    vtop->type = mem_type;
    vtop->type.t |= qualifiers;
    if (!(vtop->type.t & VT_ARRAY)) {
        vtop->r |= VT_LVAL;
#ifdef CONFIG_TCC_BCHECK
        if (tcc_state->do_bounds_check)
            vtop->r |= VT_MUSTBOUND;
#endif
    }
}

static void cpp_emit_mptr_pmf_invoke(SValue *obj, SValue *pm)
{
    Sym *class_sym, *field, *fsym;
    CType obj_type;
    int field_tok;

    if (!cpp_is_member_pointer(&pm->type) || !cpp_is_mptr_to_func(&pm->type))
        tcc_error("invalid member function pointer");
    class_sym = cpp_mptr_class(&pm->type);
    field_tok = pm->type.ref ? pm->type.ref->c : 0;
    if (!class_sym || !field_tok)
        tcc_error("invalid member function pointer");
    if (!cpp_mptr_compatible_class(&obj->type, class_sym))
        tcc_error("member function pointer class mismatch");
    field = cpp_lookup_member_field(field_tok, class_sym);
    if (!field || (field->type.t & VT_BTYPE) != VT_FUNC)
        tcc_error("invalid member function pointer");
    if (field->type.ref && field->type.ref->f.func_virtual)
        tcc_error("member function pointer to virtual method not supported");
    obj_type = obj->type;
    fsym = cpp_lookup_member_func(field, &obj_type);
    vset(&fsym->type, fsym->r | VT_SYM, 0);
    vtop->sym = fsym;
    vtop->r &= ~VT_LVAL;
    vpushv(obj);
    test_lvalue();
    gaddrof();
    cpp_member_this = *vtop;
    vpop();
    cpp_member_this_pending = 1;
}

/* C++: scan class_sym's member chain for the constructor field (a VT_FUNC
 * member whose name equals the class name).  Returns the field Sym or NULL.
 * Used by FEAT-4B to detect `Foo f(args);` ctor-call declarations. */
static Sym *cpp_find_ctor_field(Sym *class_sym)
{
    Sym *f;
    int class_name_tok;

    if (!class_sym)
        return NULL;
    class_name_tok = class_sym->v & ~SYM_STRUCT;
    for (f = class_sym->next; f; f = f->next) {
        if ((f->v & ~SYM_FIELD) != class_name_tok)
            continue;
        if ((f->type.t & VT_BTYPE) != VT_FUNC)
            continue;
        return f;
    }
    return NULL;
}

/* FEAT-4F: does the class declare a 0-arg (default) constructor?
   Used to decide whether `Foo f;` should call the ctor implicitly. */
static int cpp_class_has_default_ctor(Sym *class_sym)
{
    Sym *f;
    int class_name_tok;

    if (!class_sym)
        return 0;
    class_name_tok = class_sym->v & ~SYM_STRUCT;
    for (f = class_sym->next; f; f = f->next) {
        if ((f->v & ~SYM_FIELD) != class_name_tok)
            continue;
        if ((f->type.t & VT_BTYPE) != VT_FUNC)
            continue;
        if (cpp_func_param_count(f) == 0)
            return 1;
    }
    return 0;
}

/* C++: scan class_sym's member chain for the destructor field. */
static Sym *cpp_find_dtor_field(Sym *class_sym)
{
    Sym *f;

    if (!class_sym)
        return NULL;
    for (f = class_sym->next; f; f = f->next) {
        if (!cpp_is_dtor_field(f))
            continue;
        return f;
    }
    return NULL;
}

/* FEAT-4E: emit __cpp_dtor_Class(&obj) for a stack local. */
static void cpp_emit_local_dtor(Sym *obj_sym)
{
    Sym *class_sym;
    Sym *dtor_field;
    Sym *dtor_global;
    CType obj_type;

    if (!obj_sym || !tcc_state->cpp)
        return;
    if ((obj_sym->type.t & VT_BTYPE) != VT_STRUCT)
        return;
    if ((obj_sym->r & VT_VALMASK) != VT_LOCAL)
        return;
    class_sym = obj_sym->type.ref;
    dtor_field = cpp_find_dtor_field(class_sym);
    if (!dtor_field)
        return;
    obj_type.t = VT_STRUCT;
    obj_type.ref = class_sym;
    dtor_global = cpp_lookup_member_func(dtor_field, &obj_type);
    vset(&dtor_global->type, dtor_global->r | VT_SYM, 0);
    vtop->sym = dtor_global;
    vtop->r &= ~VT_LVAL;
    vset(&obj_sym->type, obj_sym->r, obj_sym->c);
    gaddrof();
    gfunc_call(1);
}

/* FEAT-4E: call dtors for locals declared since bottom (reverse decl order). */
static void cpp_call_scope_dtors(Sym *bottom)
{
    Sym *s;

    if (!tcc_state->cpp || !bottom)
        return;
    for (s = local_stack; s != bottom; s = s->prev)
        cpp_emit_local_dtor(s);
}

/* FEAT-4G: deferred global ctor/dtor thunks for .init_array / .fini_array. */
typedef struct CppGlobalDynEntry {
    Sym *obj_sym;
    TokenString *ctor_args; /* NULL for default ctor or dtor entry */
    int is_dtor;
} CppGlobalDynEntry;

static CppGlobalDynEntry **cpp_global_dyns;
static int nb_cpp_global_dyns;
static int cpp_global_dyn_serial;

static TokenString *cpp_save_paren_expr_tokens(void)
{
    TokenString *ts;

    ts = tok_str_alloc();
    while (tok != ')') {
        if (tok == TOK_EOF)
            tcc_error("unexpected end in global ctor arguments");
        tok_str_add_tok(ts);
        next();
    }
    /* every replayed TokenString needs the TOK_EOF terminator (see
       gen_inline_functions); without it begin_macro replay runs past
       the buffer into garbage tokens and crashes tcc. */
    tok_str_add(ts, TOK_EOF);
    return ts;
}

static void cpp_register_global_dyn(Sym *obj_sym, TokenString *ctor_args, int is_dtor)
{
    CppGlobalDynEntry *ent;
    Sym *class_sym;

    if (!obj_sym || !tcc_state->cpp)
        return;
    if ((obj_sym->type.t & VT_BTYPE) != VT_STRUCT)
        return;
    /* dynarray_add stores the pointer as-is (no struct copy), so each
       entry must be heap-allocated; a stack address here dangles and
       crashed tcc at thunk-emission time.  dynarray_reset frees them. */
    ent = tcc_malloc(sizeof(CppGlobalDynEntry));
    ent->obj_sym = obj_sym;
    ent->ctor_args = ctor_args;
    ent->is_dtor = is_dtor;
    dynarray_add(&cpp_global_dyns, &nb_cpp_global_dyns, ent);
    if (!is_dtor) {
        class_sym = obj_sym->type.ref;
        if (class_sym && cpp_find_dtor_field(class_sym)) {
            ent = tcc_malloc(sizeof(CppGlobalDynEntry));
            ent->obj_sym = obj_sym;
            ent->ctor_args = NULL;
            ent->is_dtor = 1;
            dynarray_add(&cpp_global_dyns, &nb_cpp_global_dyns, ent);
        }
    }
}

/* returns the gfunc_call arg count so the thunk can size its scratch
   area (0 when the class has no dtor and no call was emitted). */
static int cpp_emit_global_dtor_call(Sym *obj_sym)
{
    Sym *class_sym;
    Sym *dtor_field;
    Sym *dtor_global;
    CType obj_type;

    class_sym = obj_sym->type.ref;
    dtor_field = cpp_find_dtor_field(class_sym);
    if (!dtor_field)
        return 0;
    obj_type.t = VT_STRUCT;
    obj_type.ref = class_sym;
    dtor_global = cpp_lookup_member_func(dtor_field, &obj_type);
    vset(&dtor_global->type, dtor_global->r | VT_SYM, 0);
    vtop->sym = dtor_global;
    vtop->r &= ~VT_LVAL;
    /* addend 0: obj_sym->c is the ELF symbol index for globals, not a
       section offset; passing it skewed the object address by c bytes. */
    vset(&obj_sym->type, obj_sym->r | VT_SYM, 0);
    vtop->sym = obj_sym;
    gaddrof();
    gfunc_call(1);
    return 1;
}

/* returns the gfunc_call arg count (this + ctor args) so the thunk can
   size its scratch area (0 when no ctor was found). */
static int cpp_emit_global_ctor_call(Sym *obj_sym, TokenString *arg_toks)
{
    Sym *ctor_field;
    Sym *ctor_global;
    Sym *resolved;
    Sym *sa;
    CType obj_type;
    SValue obj_addr;
    int nb_args;
    int na;
    int i;

    obj_type.t = VT_STRUCT;
    obj_type.ref = obj_sym->type.ref;
    ctor_field = cpp_find_ctor_field(obj_sym->type.ref);
    if (!ctor_field)
        return 0;
    ctor_global = cpp_lookup_member_func(ctor_field, &obj_type);
    vset(&ctor_global->type, ctor_global->r | VT_SYM, 0);
    vtop->sym = ctor_global;
    vtop->r &= ~VT_LVAL;
    nb_args = 0;
    if (arg_toks) {
        begin_macro(arg_toks, 1);
        next();
        while (tok != TOK_EOF) {
            int arg_align;
            int arg_size;

            expr_eq();
            /* The hand-written thunk (cpp_emit_global_dyn_thunk) reserves
               only max(32, nb_args*8) of scratch, which covers register/
               stack scalar slots but NOT gfunc_call's by-value struct /
               long-double staging (it grows struct_size past args_size and
               would overflow into the thunk's saved rbp / return address).
               using_regs(size) == !(size>8 || size&(size-1)); anything else
               is staged, so reject it with a diagnostic instead of emitting
               silently-crashing code. */
            arg_size = type_size(&vtop->type, &arg_align);
            if (arg_size > 8 || (arg_size & (arg_size - 1)))
                tcc_error("global constructor with by-value struct / long "
                          "double argument is not supported");
            nb_args++;
            if (tok == ',')
                next();
        }
        /* no next() after end_macro: the outer stream sits at file EOF
           here and gen_inline_functions ends its replays the same way. */
        end_macro();
    }
    na = nb_args;
    resolved = cpp_resolve_func_call(ctor_global->v, na, ctor_global);
    if (resolved) {
        vtop[-na].sym = resolved;
        vtop[-na].type.ref = resolved->type.ref;
        ctor_global = resolved;
    }
    sa = ctor_global->type.ref->next;
    for (i = 0; i < na; i++) {
        vrotb(na);
        gfunc_param_typed(ctor_global->type.ref, sa);
        if (sa)
            sa = sa->next;
    }
    if (sa && sa->type.t != VT_VOID) {
        cpp_apply_default_args(ctor_global->type.ref, &nb_args, &sa);
        na = nb_args;
    }
    /* addend 0: obj_sym->c is the ELF symbol index for globals, not a
       section offset; passing it skewed the object address by c bytes. */
    vset(&obj_sym->type, obj_sym->r | VT_SYM, 0);
    vtop->sym = obj_sym;
    gaddrof();
    obj_addr = *vtop;
    vpop();
    if (na == 0) {
        vpushv(&obj_addr);
        gfunc_call(1);
        return 1;
    } else {
        vtop++;
        nb_args = na + 1;
        memmove(vtop - nb_args + 2, vtop - nb_args + 1,
            na * sizeof(SValue));
        vtop[-nb_args + 1] = obj_addr;
        gfunc_call(nb_args);
        return nb_args;
    }
}

static void cpp_emit_global_dyn_thunk(CppGlobalDynEntry *ent)
{
    CType void_type;
    CType func_type;
    Sym *proto;
    Sym *wrapper;
    char name[48];
    const char *sec;
    int name_tok;
    int thunk_start;
    int sub_imm_off;
    int nb_call_args;
    int scratch;

    void_type.t = VT_VOID;
    func_type.t = VT_FUNC;
    proto = sym_push(SYM_FIELD, &void_type, 0, 0);
    proto->f.func_call = FUNC_CDECL;
    proto->f.func_type = FUNC_NEW;
    func_type.ref = proto;

    snprintf(name, sizeof(name), ent->is_dtor ? "__cpp_gd_%d" : "__cpp_gi_%d",
        cpp_global_dyn_serial++);
    name_tok = tok_alloc(name, strlen(name))->tok;
    wrapper = external_global_sym(name_tok, &func_type);

    loc = 0;
    thunk_start = ind = cur_text_section->data_offset;
    put_extern_sym(wrapper, cur_text_section, ind, 0);

    /* Minimal cdecl thunk: push rbp; mov rbp,rsp; sub rsp,imm32;
       call ctor/dtor; leave; ret.
       Avoid gfunc_prolog/epilog here: epilog rewinds ind and calls
       pe_add_unwind_data, which made FEAT-4G init thunks take minutes. */
    o(0xe5894855);
    /* sub rsp,imm32: win64 callees home their register args in the
       CALLER's 32-byte shadow area; without it the ctor prolog's
       "mov [rbp+10h],rcx" smashed this thunk's saved rbp and the
       .init_array walker crashed right after the call returned.
       A fixed 0x20 only covers 4 args: gfunc_call stages arg 5+ at
       [rsp+arg*8], which then lands on the saved rbp / return address
       (crashed on a 6-arg global ctor).  imm32 form so the real size
       can be patched below once the arg count is known. */
    o(0xec8148);
    sub_imm_off = ind;
    gen_le32(0x20);

    if (ent->is_dtor)
        nb_call_args = cpp_emit_global_dtor_call(ent->obj_sym);
    else
        nb_call_args = cpp_emit_global_ctor_call(ent->obj_sym, ent->ctor_args);

    /* same sizing rule as gfunc_call's stack staging ([rsp+arg*8],
       32-byte minimum), 16-aligned to keep the callee entry aligned.
       By-value struct/long-double ctor args would need gfunc_call's
       func_scratch on top of this; not supported here. */
    scratch = nb_call_args * 8;
    if (scratch < 32)
        scratch = 32;
    scratch = (scratch + 15) & -16;
    write32le(cur_text_section->data + sub_imm_off, scratch);

    o(0xc9);
    o(0xc3);
    check_vstack();
    /* o()/g() advance only ind, not data_offset (gen_function normally
       syncs it at its end).  Without this sync gen_inline_functions
       would emit the deferred member bodies over these thunks and the
       .init_array pointers would jump into overwritten code. */
    cur_text_section->data_offset = ind;
    put_extern_sym(wrapper, cur_text_section, thunk_start, ind - thunk_start);

    sec = ent->is_dtor ? ".fini_array" : ".init_array";
    add_array(tcc_state, sec, wrapper->c);

    /* no tok_str_free(ent->ctor_args) here: begin_macro(arg_toks, 1)
       hands ownership to end_macro, which already frees the string;
       freeing again corrupted the heap (injected startup then failed
       with a bogus \x01 parse error). */
}

static void cpp_finish_global_dyns(TCCState *s1)
{
    int i;
    int saved_nocode;

    if (!s1->cpp || nb_cpp_global_dyns == 0)
        return;
    saved_nocode = nocode_wanted;
    nocode_wanted = 0;
    cur_text_section = text_section;
    cpp_global_dyn_serial = 0;
    for (i = 0; i < nb_cpp_global_dyns; i++)
        cpp_emit_global_dyn_thunk(cpp_global_dyns[i]);
    dynarray_reset(&cpp_global_dyns, &nb_cpp_global_dyns);
    /* survives the per-TU save/restore of s1->cpp; gates the PE startup
       injection at link time (tccpe.c). */
    s1->cpp_global_ctors = 1;
    nocode_wanted = saved_nocode;
}

/* FEAT-5A: anonymous embedded base subobject (class D : public Base). */
static Sym *cpp_get_anon_base_field(Sym *class_sym)
{
    Sym *f;

    if (!class_sym)
        return NULL;
    for (f = class_sym->next; f; f = f->next) {
        if ((f->type.t & VT_BTYPE) != VT_STRUCT)
            continue;
        if ((f->v & ~SYM_FIELD) < SYM_FIRST_ANOM)
            continue;
        if (f->parent_class)
            return f;
    }
    return NULL;
}

static int cpp_type_has_virtual(Sym *class_sym)
{
    Sym *f, *base_field;

    if (!class_sym)
        return 0;
    for (f = class_sym->next; f; f = f->next) {
        if ((f->type.t & VT_BTYPE) == VT_FUNC && f->type.ref
            && f->type.ref->f.func_virtual)
            return 1;
    }
    base_field = cpp_get_anon_base_field(class_sym);
    if (base_field && base_field->parent_class)
        return cpp_type_has_virtual(base_field->parent_class);
    return 0;
}

static Sym *cpp_find_virtual_field_by_name(Sym *class_sym, int member_tok)
{
    Sym *f;

    if (!class_sym)
        return NULL;
    for (f = class_sym->next; f; f = f->next) {
        if ((f->type.t & VT_BTYPE) != VT_FUNC)
            continue;
        if (!f->type.ref || !f->type.ref->f.func_virtual)
            continue;
        if ((f->v & ~SYM_FIELD) == member_tok)
            return f;
    }
    return NULL;
}

static int cpp_count_virtual_slots(Sym *class_sym)
{
    Sym *f, *base_field;
    int n, maxslot;

    if (!class_sym)
        return 0;
    base_field = cpp_get_anon_base_field(class_sym);
    if (base_field && base_field->parent_class)
        n = cpp_count_virtual_slots(base_field->parent_class);
    else
        n = 0;
    maxslot = n;
    for (f = class_sym->next; f; f = f->next) {
        if ((f->type.t & VT_BTYPE) != VT_FUNC)
            continue;
        if (!f->type.ref || !f->type.ref->f.func_virtual)
            continue;
        if (f->c + 1 > maxslot)
            maxslot = f->c + 1;
    }
    return maxslot;
}

static void cpp_assign_virtual_slots(Sym *class_sym)
{
    Sym *base_field, *base_class, *f;
    int nslots, member_tok;

    if (!class_sym)
        return;
    base_field = cpp_get_anon_base_field(class_sym);
    base_class = base_field ? base_field->parent_class : NULL;
    nslots = base_class ? cpp_count_virtual_slots(base_class) : 0;
    for (f = class_sym->next; f; f = f->next) {
        if ((f->type.t & VT_BTYPE) != VT_FUNC)
            continue;
        if (!f->type.ref || !f->type.ref->f.func_virtual)
            continue;
        member_tok = f->v & ~SYM_FIELD;
        if (base_class) {
            Sym *bf = cpp_find_virtual_field_by_name(base_class, member_tok);
            if (bf) {
                f->c = bf->c;
                continue;
            }
        }
        f->c = nslots++;
    }
}

static void cpp_insert_vptr_field(Sym *class_sym)
{
    Sym *vptr, *first;
    CType pt;

    if (!class_sym || !cpp_type_has_virtual(class_sym))
        return;
    pt.t = VT_VOID | VT_PTR;
    pt.ref = NULL;
    vptr = sym_push(anon_sym++ | SYM_FIELD, &pt, 0, 0);
    vptr->a.access = ACCESS_PRIVATE;
    first = class_sym->next;
    vptr->next = first;
    class_sym->next = vptr;
}

static Sym *cpp_find_virtual_by_slot(Sym *class_sym, int slot)
{
    Sym *f, *base_field;

    if (!class_sym)
        return NULL;
    for (f = class_sym->next; f; f = f->next) {
        if ((f->type.t & VT_BTYPE) != VT_FUNC)
            continue;
        if (!f->type.ref || !f->type.ref->f.func_virtual)
            continue;
        if (f->c == slot)
            return f;
    }
    base_field = cpp_get_anon_base_field(class_sym);
    if (base_field && base_field->parent_class)
        return cpp_find_virtual_by_slot(base_field->parent_class, slot);
    return NULL;
}

static Sym *cpp_lookup_virtual_impl(Sym *field)
{
    int v;
    Sym *s, *class_sym;

    if (!field || !field->type.ref)
        return NULL;
    class_sym = field->parent_class;
    if (!class_sym)
        return NULL;
    if (cpp_is_ctor_field(field)) {
        v = cpp_ctor_name_tok(field->v & ~SYM_FIELD);
        if (!v)
            return NULL;
    } else if (cpp_is_dtor_field(field)) {
        v = cpp_dtor_name_tok(class_sym->v & ~SYM_STRUCT);
        if (!v)
            return NULL;
    } else {
        v = field->v & ~SYM_FIELD;
    }
    for (s = sym_find(v); s; s = s->prev_tok) {
        if ((s->type.t & VT_BTYPE) != VT_FUNC)
            continue;
        if (s->parent_class == class_sym)
            return s;
    }
    return NULL;
}

static void cpp_emit_vtable(Sym *class_sym)
{
    char buf[256];
    TokenSym *ts;
    Sym *vtable_sym, *field, *impl;
    CType arr_type, fptr_type;
    AttributeDef ad;
    Section *sec;
    unsigned long addr;
    int nslots, slot, len;

    if (!class_sym || !tcc_state->cpp)
        return;
    nslots = cpp_count_virtual_slots(class_sym);
    if (nslots <= 0)
        return;
    len = snprintf(buf, sizeof buf, "__cpp_vtable_%s",
                   get_tok_str(class_sym->v & ~SYM_STRUCT, NULL));
    if (len <= 0 || len >= (int)sizeof buf)
        return;
    ts = tok_alloc(buf, len);
    class_sym->cpp_vtable_tok = ts->tok;
    fptr_type.t = VT_VOID | VT_PTR;
    fptr_type.ref = NULL;
    arr_type.t = VT_PTR | VT_ARRAY;
    arr_type.ref = sym_push(SYM_FIELD, &fptr_type, 0, nslots);
    memset(&ad, 0, sizeof ad);
    vtable_sym = external_sym(ts->tok, &arr_type, VT_CONST, &ad);
    sec = rodata_section;
    addr = section_add(sec, (unsigned long)nslots * PTR_SIZE, PTR_SIZE);
    put_extern_sym(vtable_sym, sec, addr, (unsigned long)nslots * PTR_SIZE);
    for (slot = 0; slot < nslots; slot++) {
        field = cpp_find_virtual_by_slot(class_sym, slot);
        if (!field)
            continue;
        impl = cpp_lookup_virtual_impl(field);
        if (!impl)
            continue;
        greloca(sec, impl, addr + (unsigned long)slot * PTR_SIZE, R_DATA_PTR, 0);
    }
}

static void cpp_init_local_vptr(Sym *obj_sym)
{
    Sym *vtable_sym;
    CType voidp;

    if (!obj_sym || !obj_sym->type.ref || !obj_sym->type.ref->cpp_vtable_tok)
        return;
    if ((obj_sym->r & VT_VALMASK) != VT_LOCAL)
        return;
    if (nocode_wanted)
        return;
    vtable_sym = sym_find(obj_sym->type.ref->cpp_vtable_tok);
    if (!vtable_sym)
        return;
    /* Build a real void* type (mk_pointer allocates ->ref; a NULL ref would
     * crash later passes that dereference pointed_type). */
    voidp.t = VT_VOID;
    voidp.ref = NULL;
    mk_pointer(&voidp);
    /* vstore() expects vtop[-1] = destination lvalue, vtop = source value.
     * The vptr lives at object offset 0, so its slot address is obj_sym->c. */
    vset(&voidp, VT_LOCAL | VT_LVAL, obj_sym->c);   /* destination */
    vpushsym(&voidp, vtable_sym);                    /* value = &vtable */
    vstore();
    vpop();
}

/* FEAT-5A Phase 4: static vptr init for global/static polymorphic objects. */
static void cpp_init_global_vptr(Sym *obj_sym, Section *sec, unsigned long addr)
{
    Sym *vtable_sym;

    if (!obj_sym || !obj_sym->type.ref || !obj_sym->type.ref->cpp_vtable_tok)
        return;
    if (!sec || sec == common_section || NODATA_WANTED)
        return;
    vtable_sym = sym_find(obj_sym->type.ref->cpp_vtable_tok);
    if (!vtable_sym)
        return;
    greloca(sec, vtable_sym, addr, R_DATA_PTR, 0);
}

/* Load virtual member fn from vtable[slot]; leaves a function-pointer
 * lvalue on vtop (the standard `fp()` representation, so the call handler
 * performs an indirect call), and stores 'this' in cpp_member_this so it is
 * injected as the implicit first argument.
 *
 * Pointer levels (T = the member function type, fn-ptr = T(*)()):
 *   fnptr_t   = T(*)()     one vtable entry
 *   fnptr_pp  = T(**)()    the vptr (points at the fn-ptr array)
 *   fnptr_ppp = T(***)()   'this' reinterpreted (points at the vptr slot)
 *
 * NB: tcc's indir() dereferences vtop->type.ref unconditionally, so every
 * pointer type fed to it must be built with mk_pointer (a NULL ref crashes
 * the compiler). */
static void cpp_prepare_virtual_member_call(Sym *field, CType *obj_type)
{
    CType fnptr_t, fnptr_pp, fnptr_ppp;
    int slot;

    if (!field || !field->type.ref || !field->type.ref->f.func_virtual)
        return;
    slot = field->c;
    test_lvalue();
    gaddrof();
    cpp_member_this = *vtop;
    vpop();

    fnptr_t = field->type;          /* T()       -> */
    mk_pointer(&fnptr_t);           /* T(*)()       */
    fnptr_pp = fnptr_t;
    mk_pointer(&fnptr_pp);          /* T(**)()      */
    fnptr_ppp = fnptr_pp;
    mk_pointer(&fnptr_ppp);         /* T(***)()     */

    /* this -> &vptr, load vptr */
    vpushv(&cpp_member_this);
    vtop->type = fnptr_ppp;
    vtop->r &= ~VT_LVAL;
    indir();                        /* lvalue T(**)() == vptr storage */
    /* index the vtable: typed pointer arithmetic scales slot by PTR_SIZE */
    vpushi(slot);
    gen_op('+');                    /* T(**)() rvalue == &vtable[slot] */
    indir();                        /* lvalue T(*)() == vtable[slot]    */

    /* This is an indirect (function-pointer) call, not a named function:
     * clear the leftover .sym copied from 'this' so the call handler's C++
     * overload-resolution path does not dereference a stale Sym. */
    vtop->sym = NULL;
    cpp_member_this_pending = 1;
}

/* FEAT-4C: peek `Class::Class(` at file/block scope without consuming
 * the stream.  Returns 1 and sets *class_tok when matched. */
static int cpp_peek_out_of_class_ctor(int *class_tok)
{
    int cls, mem;
    int tilde;

    if (!tcc_state->cpp || tok < TOK_IDENT || !class_tok)
        return 0;
    cls = tok;
    if (!struct_find(cls))
        return 0;
    next();
    if (tok != ':') {
        unget_tok(cls);
        return 0;
    }
    next();
    if (tok != ':') {
        unget_tok(':');
        unget_tok(cls);
        return 0;
    }
    next();
    /* FEAT-4E-P2: also accept Class::~Class( so out-of-class dtor
       definitions take the qualified-class decl path (btype VT_VOID),
       mirroring the out-of-class ctor peek. */
    tilde = 0;
    if (tok == '~') {
        tilde = 1;
        next();
    }
    mem = tok;
    if (mem != cls) {
        if (tilde)
            unget_tok('~');
        unget_tok(':');
        unget_tok(':');
        unget_tok(cls);
        return 0;
    }
    next();
    if (tok != '(') {
        unget_tok(mem);
        if (tilde)
            unget_tok('~');
        unget_tok(':');
        unget_tok(':');
        unget_tok(cls);
        return 0;
    }
    unget_tok(mem);
    if (tilde)
        unget_tok('~');
    unget_tok(':');
    unget_tok(':');
    unget_tok(cls);
    *class_tok = cls;
    return 1;
}

static void cpp_push_member_var(Sym *field)
{
    int cumofs, qualifiers;

    if (!cpp_this_sym)
        tcc_error("invalid use of member name");
    cumofs = field->c;
    vset(&cpp_this_sym->type, cpp_this_sym->r, cpp_this_sym->c);
    vtop->sym = cpp_this_sym;
    indir();
    qualifiers = vtop->type.t & (VT_CONSTANT | VT_VOLATILE);
    gaddrof();
    vtop->type = char_pointer_type;
    vpushi(cumofs);
    gen_op('+');
    vtop->type = field->type;
    vtop->type.t |= qualifiers;
    if (!(vtop->type.t & VT_ARRAY))
        vtop->r |= VT_LVAL;
}

/* FEAT-4D: find embedded base subobject field in a derived class by base
 * class name token (matches anonymous base from `class D : public Base`). */
static Sym *cpp_find_base_field(Sym *derived_class, int base_name_tok)
{
    Sym *f;
    Sym *bc;

    if (!derived_class)
        return NULL;
    for (f = derived_class->next; f; f = f->next) {
        if ((f->type.t & VT_BTYPE) != VT_STRUCT)
            continue;
        bc = f->type.ref;
        if (!bc)
            continue;
        if ((bc->v & ~SYM_STRUCT) == base_name_tok)
            return f;
    }
    return NULL;
}

/* FEAT-4D: emit `Base(args)` in a derived ctor mem-initializer list as a
 * call to __cpp_ctor_Base(&base_subobject, args).  tok is at the first
 * token inside `(`.  Consumes through the closing `)`. */
static void cpp_emit_base_ctor_call(Sym *base_field, Sym *base_class)
{
    Sym *ctor_field;
    Sym *ctor_global;
    Sym *resolved;
    Sym *sa;
    CType base_type;
    int nb_args;
    int na;
    int i;
    SValue base_this;

    if (!base_field || !base_class || !cpp_this_sym)
        return;
    ctor_field = cpp_find_ctor_field(base_class);
    if (!ctor_field)
        tcc_error("no constructor for base class");
    base_type.t = VT_STRUCT;
    base_type.ref = base_class;
    ctor_global = cpp_lookup_member_func(ctor_field, &base_type);
    vset(&ctor_global->type, ctor_global->r | VT_SYM, 0);
    vtop->sym = ctor_global;
    vtop->r &= ~VT_LVAL;
    nb_args = 0;
    while (tok != ')' && tok != TOK_EOF) {
        expr_eq();
        nb_args++;
        if (tok == ',')
            next();
    }
    na = nb_args;
    /* resolve the ctor overload from the raw argument types (the initial
       bind above only sees the first ctor field), then convert each arg
       to the resolved prototype: args are the top na entries (func below),
       so na rotations of vrotb(na) visit each arg once and restore the
       original order */
    resolved = cpp_resolve_func_call(ctor_global->v, na, ctor_global);
    if (resolved) {
        vtop[-na].sym = resolved;
        vtop[-na].type.ref = resolved->type.ref;
        ctor_global = resolved;
    }
    sa = ctor_global->type.ref->next;
    for (i = 0; i < na; i++) {
        vrotb(na);
        gfunc_param_typed(ctor_global->type.ref, sa);
        if (sa)
            sa = sa->next;
    }
    if (sa) {
        cpp_apply_default_args(ctor_global->type.ref, &nb_args, &sa);
        na = nb_args;
    }
    cpp_push_member_var(base_field);
    gaddrof();
    base_this = *vtop;
    vpop();
    if (na == 0) {
        vpushv(&base_this);
        nb_args = 1;
    } else {
        vtop++;
        nb_args = na + 1;
        memmove(vtop - nb_args + 2, vtop - nb_args + 1,
            na * sizeof(SValue));
        vtop[-nb_args + 1] = base_this;
    }
    gfunc_call(nb_args);
}

static void cpp_save_default_arg(Sym *param)
{
    TokenString *def;
    int level;

    if (tok != '=')
        return;
    next();
    def = tok_str_alloc();
    level = 0;
    while (1) {
        int t = tok;
        if (level == 0 && (t == ',' || t == ')'))
            break;
        if (tok == TOK_EOF)
            tcc_error("unexpected end in default argument");
        tok_str_add_tok(def);
        next();
        if (t == '(')
            level++;
        else if (t == ')')
            level--;
    }
    tok_str_add(def, TOK_EOF);
    param->inline_func_str = def;
}

static void cpp_save_mem_init_list(Sym *ctor)
{
    TokenString *def;

    if (tok != ':')
        return;
    next();
    def = tok_str_alloc();
    while (tok != '{') {
        int t = tok;
        if (t == TOK_EOF)
            tcc_error("unexpected end in mem-initializer-list");
        tok_str_add_tok(def);
        next();
    }
    tok_str_add(def, TOK_EOF);
    ctor->cpp_mem_init_list = def;
}


static void cpp_register_member_body(Sym *field_sym, Sym *class_sym, CType *ftype, TokenString *body)
{
    if (!body || !tcc_state->cpp || !field_sym)
        return;
    field_sym->inline_func_str = body;
}

static void cpp_finish_member_inlines(Sym *class_sym)
{
    Sym *f;
    AttributeDef ad;
    CType type;
    struct InlineFunc *fn;
    Sym *sym;

    if (!class_sym || !tcc_state->cpp)
        return;
    for (f = class_sym->next; f; f = f->next) {
        TokenString *body;
        int global_tok;
        if ((f->type.t & VT_BTYPE) != VT_FUNC)
            continue;
        body = f->inline_func_str;
        if (!body)
            continue;
        f->inline_func_str = NULL;
        memset(&ad, 0, sizeof ad);
        type = f->type;
        type.t = (type.t & ~VT_STORAGE) | VT_EXTERN;
        /* ctor: use a mangled token name to dodge the class typedef.
         * (See cpp_lookup_member_func for the matching lookup path.) */
        if (cpp_is_ctor_field(f)) {
            global_tok = cpp_ctor_name_tok(f->v & ~SYM_FIELD);
            if (!global_tok)
                global_tok = f->v & ~SYM_FIELD;  /* fallback */
        } else if (cpp_is_dtor_field(f)) {
            global_tok = cpp_dtor_name_tok(f->parent_class->v & ~SYM_STRUCT);
            if (!global_tok)
                global_tok = f->v & ~SYM_FIELD;  /* fallback */
        } else {
            global_tok = f->v & ~SYM_FIELD;
        }
        sym = external_sym(global_tok, &type, 0, &ad);
        sym->parent_class = class_sym;
        sym->type.t |= VT_INLINE;
        /* Inline ctor: propagate the saved mem-initializer-list so
         * gen_function can expand it when codegen runs. */
        if (f->cpp_mem_init_list)
            sym->cpp_mem_init_list = f->cpp_mem_init_list;
        cpp_set_func_mangle_label(sym, &type);
        fn = tcc_malloc(sizeof *fn + strlen(file->filename));
        strcpy(fn->filename, file->filename);
        fn->sym = sym;
        fn->func_str = body;
        dynarray_add(&tcc_state->inline_fns, &tcc_state->nb_inline_fns, fn);
    }
}

static void cpp_apply_default_args(Sym *func, int *pnb_args, Sym **psa)
{
    Sym *sa;
    int nb_args;

    sa = *psa;
    nb_args = *pnb_args;
    while (sa) {
        if (sa->inline_func_str) {
            begin_macro(sa->inline_func_str, 1);
            next();
            expr_eq();
            gfunc_param_typed(func, sa);
            end_macro();
            nb_args++;
        } else {
            tcc_error("too few arguments to function");
        }
        sa = sa->next;
    }
    *pnb_args = nb_args;
    *psa = sa;
}

/* Deep-copy a token string so begin_macro/end_macro(alloc=1) can free the
 * copy without invalidating the original stored in the field sym param. */
static TokenString *tok_str_dup_for_default(TokenString *src)
{
    TokenString *dst;
    int i;
    if (!src || !src->str)
        return NULL;
    dst = tok_str_alloc();
    for (i = 0; i < src->len; i++)
        tok_str_add(dst, src->str[i]);
    return dst;
}

/* Copy default-arg token strings and the VT_STATIC storage class from the
 * class declaration field sym to the out-of-class definition sym.
 *   class Foo { static int bar(int x = 10); };
 *   int Foo::bar(int x) { ... }
 * The out-of-class definition never repeats "static", so without this
 * propagation gen_function() would treat bar() as a non-static member and
 * implicitly insert a `this` parameter, corrupting the calling convention.
 * The default-arg TokenString is deep-copied so end_macro(alloc=1) frees
 * only the copy and the original on the field sym stays valid. */
static void cpp_inherit_decl_defaults(Sym *sym)
{
    Sym *field, *dp, *sp;

    if (!sym || !sym->parent_class || !tcc_state->cpp)
        return;
    if ((sym->type.t & VT_BTYPE) != VT_FUNC || !sym->type.ref)
        return;
    for (field = sym->parent_class->next; field; field = field->next) {
        if (field->v == (sym->v | SYM_FIELD)
            && (field->type.t & VT_BTYPE) == VT_FUNC)
            break;
    }
    if (!field || !field->type.ref)
        return;
    /* propagate VT_STATIC so gen_function() does not add an implicit
     * `this` to a static member function defined outside the class. */
    if (field->type.t & VT_STATIC)
        sym->type.t |= VT_STATIC;
    dp = field->type.ref->next;
    sp = sym->type.ref->next;
    while (dp && sp) {
        if (dp->type.t == VT_VOID || sp->type.t == VT_VOID)
            break;
        if (!sp->inline_func_str && dp->inline_func_str)
            sp->inline_func_str = tok_str_dup_for_default(dp->inline_func_str);
        dp = dp->next;
        sp = sp->next;
    }
}

static void end_switch(void);
static void do_Static_assert(void);

/* ------------------------------------------------------------------------- */
/* �����I�ȃR�[�h�}�� */

/* ���������x���Ŏg�p����Ă����ꍇ�� 'nocode_wanted' ���N���A���� */
ST_FUNC void gsym(int t)
{
    if (t) {
        gsym_addr(t, ind);
        CODE_ON();
    }
}

/* ���݂̃v���O�����J�E���^�����x���ł���ꍇ�� 'nocode_wanted' ���N���A���� */
static int gind()
{
    int t = ind;
    CODE_ON();
    if (debug_modes)
        tcc_tcov_block_begin(tcc_state);
    return t;
}

/* �������i����j�W�����v�̌�� 'nocode_wanted' ��ݒ肷�� */
static void gjmp_addr_acs(int t)
{
    gjmp_addr(t);
    CODE_OFF();
}

/* �������i�O���j�W�����v�̌�� 'nocode_wanted' ��ݒ肷�� */
static int gjmp_acs(int t)
{
    t = gjmp(t);
    CODE_OFF();
    return t;
}

/* �����̓t�@�C���̍Ō�� #undef ����܂� */
#define gjmp_addr gjmp_addr_acs
#define gjmp gjmp_acs
/* ------------------------------------------------------------------------- */

ST_INLN int is_float(int t)
{
    int bt = t & VT_BTYPE;
    return bt == VT_LDOUBLE
        || bt == VT_DOUBLE
        || bt == VT_FLOAT
        || bt == VT_QFLOAT;
}

static inline int is_integer_btype(int bt)
{
    return bt == VT_BYTE
        || bt == VT_BOOL
        || bt == VT_SHORT
        || bt == VT_INT
        || bt == VT_LLONG;
}

static int btype_size(int bt)
{
    return bt == VT_BYTE || bt == VT_BOOL ? 1 :
        bt == VT_SHORT ? 2 :
        bt == VT_INT ? 4 :
        bt == VT_LLONG ? 8 :
        bt == VT_PTR ? PTR_SIZE : 0;
}

/* �^�ɉ������֐��߂背�W�X�^��Ԃ� */
static int R_RET(int t)
{
    if (!is_float(t))
        return REG_IRET;
#ifdef TCC_TARGET_X86_64
    if ((t & VT_BTYPE) == VT_LDOUBLE)
        return TREG_ST0;
#elif defined TCC_TARGET_RISCV64
    if ((t & VT_BTYPE) == VT_LDOUBLE)
        return REG_IRET;
#endif
    return REG_FRET;
}

/* 2�Ԗڂ̊֐��߂背�W�X�^��Ԃ��i���݂���ꍇ�j */
static int R2_RET(int t)
{
    t &= VT_BTYPE;
#if PTR_SIZE == 4
    if (t == VT_LLONG)
        return REG_IRE2;
#elif defined TCC_TARGET_X86_64
    if (t == VT_QLONG)
        return REG_IRE2;
    if (t == VT_QFLOAT)
        return REG_FRE2;
#elif defined TCC_TARGET_RISCV64
    if (t == VT_LDOUBLE)
        return REG_IRE2;
#endif
    return VT_CONST;
}
/* 2���[�h�^���ǂ�����Ԃ� */
#define USING_TWO_WORDS(t) (R2_RET(t) != VT_CONST)

/* 2���[�h�^���ǂ����𔻒肷�� */
#define USING_TWO_WORDS(t) (R2_RET(t) != VT_CONST)

/* �֐��߂背�W�X�^���X�^�b�N�l�ɐݒ肷�� */
static void PUT_R_RET(SValue* sv, int t)
{
    sv->r = R_RET(t), sv->r2 = R2_RET(t);
}

/* �^t�ɑ΂���֐��߂背�W�X�^�̃��W�X�^�N���X��Ԃ� */
static int RC_RET(int t)
{
    return reg_classes[R_RET(t)] & ~(RC_FLOAT | RC_INT);
}

/* �^t�ɑ΂����ʓI�ȃ��W�X�^�N���X��Ԃ� */
static int RC_TYPE(int t)
{
    if (!is_float(t))
        return RC_INT;
#ifdef TCC_TARGET_X86_64
    if ((t & VT_BTYPE) == VT_LDOUBLE)
        return RC_ST0;
    if ((t & VT_BTYPE) == VT_QFLOAT)
        return RC_FRET;
#elif defined TCC_TARGET_RISCV64
    if ((t & VT_BTYPE) == VT_LDOUBLE)
        return RC_INT;
#endif
    return RC_FLOAT;
}

/* t��rc�ɑΉ�����2�Ԗڂ̃��W�X�^�N���X��Ԃ� */
static int RC2_TYPE(int t, int rc)
{
    if (!USING_TWO_WORDS(t))
        return 0;
#ifdef RC_IRE2
    if (rc == RC_IRET)
        return RC_IRE2;
#endif
#ifdef RC_FRE2
    if (rc == RC_FRET)
        return RC_FRE2;
#endif
    if (rc & RC_FLOAT)
        return RC_FLOAT;
    return RC_INT;
}

/* ��W���̐��w���C�u�����ł̖�������邽�ߓƎ��� 'finite' �֐����g�p */
/* XXX: �G���f�B�A���ˑ� */
ST_FUNC int ieee_finite(double d)
{
    int p[4];
    memcpy(p, &d, sizeof(double));
    return ((unsigned)((p[1] | 0x800fffff) + 1)) >> 31;
}

/* Intel������ long double ���l�C�e�B�u�ɏ�������ݒ� */
#if (defined __i386__ || defined __x86_64__) \
    && (defined TCC_TARGET_I386 || defined TCC_TARGET_X86_64)
# define TCC_IS_NATIVE_387
#endif

ST_FUNC void test_lvalue(void)
{
    if (!(vtop->r & VT_LVAL))
        expect("lvalue");
}

ST_FUNC void check_vstack(void)
{
    if (vtop != vstack - 1)
        tcc_error("�����R���p�C���G���[: vstack �̃��[�N (%d)",
            (int)(vtop - vstack + 1));
}

/* vstack �̃f�o�b�O�⏕ */
#if 0
void pv(const char* lbl, int a, int b)
{
    int i;
    for (i = a; i < a + b; ++i) {
        SValue* p = &vtop[-i];
        printf("%s vtop[-%d] : type.t:%04x  r:%04x  r2:%04x  c.i:%d\n",
            lbl, i, p->type.t, p->r, p->r2, (int)p->c.i);
    }
}
#endif

/* ------------------------------------------------------------------------- */
/* vstack �ƌ^�̏������Btcc -E�i�v���v���Z�X�̂݁j�ł�������s���K�v������ */
ST_FUNC void tccgen_init(TCCState* s1)
{
    vtop = vstack - 1;
    memset(vtop, 0, sizeof * vtop);

    /* �悭�g����^���` */
    int_type.t = VT_INT;

    char_type.t = VT_BYTE;
    if (s1->char_is_unsigned)
        char_type.t |= VT_UNSIGNED;
    char_pointer_type = char_type;
    mk_pointer(&char_pointer_type);

    func_old_type.t = VT_FUNC;
    func_old_type.ref = sym_push(SYM_FIELD, &int_type, 0, 0);
    func_old_type.ref->f.func_call = FUNC_CDECL;
    func_old_type.ref->f.func_type = FUNC_OLD;
#ifdef precedence_parser
    init_prec();
#endif
    cstr_new(&initstr);
    cpp_qualified_class = NULL;
    cpp_cur_class = NULL;
    decl_once_flag = 0;
    cpp_member_this_pending = 0;
    cpp_this_sym = NULL;
    cpp_cur_func_class = NULL;
    dynarray_reset(&cpp_global_dyns, &nb_cpp_global_dyns);
    cpp_global_dyn_serial = 0;
}

ST_FUNC int tccgen_compile(TCCState* s1)
{
    funcname = "";
    func_ind = -1;
    anon_sym = SYM_FIRST_ANOM;
    nocode_wanted = DATA_ONLY_WANTED; /* �֐��O�ł̓R�[�h�𐶐����Ȃ� */
    debug_modes = (s1->do_debug ? 1 : 0) | s1->test_coverage << 1;

    tcc_debug_start(s1);
    tcc_tcov_start(s1);
#ifdef TCC_TARGET_ARM
    arm_init(s1);
#endif
#ifdef INC_DEBUG
    printf("%s: **** new file\n", file->filename);
#endif
    parse_flags = PARSE_FLAG_PREPROCESS | PARSE_FLAG_TOK_NUM | PARSE_FLAG_TOK_STR;
    next();
    decl(VT_CONST);
    cpp_finish_global_dyns(s1);
    gen_inline_functions(s1);
    check_vstack();
    /* �|��P�ʂ̏��̏I��� */
#if TCC_EH_FRAME
    tcc_eh_frame_end(s1);
#endif
    tcc_debug_end(s1);
    tcc_tcov_end(s1);
    return 0;
}

ST_FUNC void tccgen_finish(TCCState* s1)
{
    tcc_debug_end(s1); /* �G���[�������ɔ����ă���������� */
    free_inline_functions(s1);
    sym_pop(&global_stack, NULL, 0);
    sym_pop(&local_stack, NULL, 0);
    /* �v���v���Z�b�T�̃}�N������� */
    free_defines(NULL);
    /* sym_pools ����� */
    dynarray_reset(&sym_pools, &nb_sym_pools);
    cstr_free(&initstr);
    dynarray_reset(&stk_data, &nb_stk_data);
    while (cur_switch)
        end_switch();
    local_scope = 0;
    loop_scope = NULL;
    all_cleanups = NULL;
    pending_gotos = NULL;
    nb_temp_local_vars = 0;
    global_label_stack = NULL;
    local_label_stack = NULL;
    cur_text_section = NULL;
    sym_free_first = NULL;
}

/* ------------------------------------------------------------------------- */
ST_FUNC ElfSym* elfsym(Sym* s)
{
    if (!s || !s->c)
        return NULL;
    return &((ElfSym*)symtab_section->data)[s->c];
}

/* ELF �V���{���ɃX�g���[�W������K�p */
ST_FUNC void update_storage(Sym* sym)
{
    ElfSym* esym;
    int sym_bind, old_sym_bind;

    esym = elfsym(sym);
    if (!esym)
        return;

    if (sym->a.visibility)
        esym->st_other = (esym->st_other & ~ELFW(ST_VISIBILITY)(-1))
        | sym->a.visibility;

    if (sym->type.t & (VT_STATIC | VT_INLINE))
        sym_bind = STB_LOCAL;
    else if (sym->a.weak)
        sym_bind = STB_WEAK;
    else
        sym_bind = STB_GLOBAL;
    old_sym_bind = ELFW(ST_BIND)(esym->st_info);
    if (sym_bind != old_sym_bind) {
        esym->st_info = ELFW(ST_INFO)(sym_bind, ELFW(ST_TYPE)(esym->st_info));
    }

#ifdef TCC_TARGET_PE
    if (sym->a.dllimport)
        esym->st_other |= ST_PE_IMPORT;
    if (sym->a.dllexport)
        esym->st_other |= ST_PE_EXPORT;
#endif

#if 0
    printf("storage %s: bind=%c vis=%d exp=%d imp=%d\n",
        get_tok_str(sym->v, NULL),
        sym_bind == STB_WEAK ? 'w' : sym_bind == STB_LOCAL ? 'l' : 'g',
        sym->a.visibility,
        sym->a.dllexport,
        sym->a.dllimport
    );
#endif
}

/* ------------------------------------------------------------------------- */
/* sym->c ���X�V���A'section' �Z�N�V�������̊O���V���{���i�l value�j���w���悤�ɂ��� */

ST_FUNC void put_extern_sym2(Sym* sym, int sh_num,
    addr_t value, unsigned long size,
    int can_add_underscore)
{
    int sym_type, sym_bind, info, other, t;
    ElfSym* esym;
    const char* name;
    char buf1[256];

    if (!sym->c) {
        name = get_tok_str(sym->v, NULL);
        t = sym->type.t;
        if ((t & VT_BTYPE) == VT_FUNC) {
            sym_type = STT_FUNC;
        }
        else if ((t & VT_BTYPE) == VT_VOID) {
            sym_type = STT_NOTYPE;
            if ((t & (VT_BTYPE | VT_ASM_FUNC)) == VT_ASM_FUNC)
                sym_type = STT_FUNC;
        }
        else {
            sym_type = STT_OBJECT;
        }
        if (t & (VT_STATIC | VT_INLINE))
            sym_bind = STB_LOCAL;
        else
            sym_bind = STB_GLOBAL;
        other = 0;

#ifdef TCC_TARGET_PE
        if (sym_type == STT_FUNC && sym->type.ref) {
            Sym* ref = sym->type.ref;
            if (ref->a.nodecorate) {
                can_add_underscore = 0;
            }
            if (ref->f.func_call == FUNC_STDCALL && can_add_underscore) {
                sprintf(buf1, "_%s@%d", name, ref->f.func_args * PTR_SIZE);
                name = buf1;
                other |= ST_PE_STDCALL;
                can_add_underscore = 0;
            }
        }
#endif

        if (sym->asm_label) {
            name = get_tok_str(sym->asm_label, NULL);
            can_add_underscore = 0;
        }

        if (tcc_state->leading_underscore && can_add_underscore) {
            buf1[0] = '_';
            pstrcpy(buf1 + 1, sizeof(buf1) - 1, name);
            name = buf1;
        }

        info = ELFW(ST_INFO)(sym_bind, sym_type);
        sym->c = put_elf_sym(symtab_section, value, size, info, other, sh_num, name);

        if (debug_modes)
            tcc_debug_extern_sym(tcc_state, sym, sh_num, sym_bind, sym_type);

    }
    else {
        esym = elfsym(sym);
        esym->st_value = value;
        esym->st_size = size;
        esym->st_shndx = sh_num;
    }
    update_storage(sym);
}

ST_FUNC void put_extern_sym(Sym* sym, Section* s, addr_t value, unsigned long size)
{
    if (nocode_wanted && (NODATA_WANTED || (s && s == cur_text_section)))
        return;
    put_extern_sym2(sym, s ? s->sh_num : SHN_UNDEF, value, size, 1);
}

/* �Z�N�V���� s �̃V���{�� sym �ɐV�����Ĕz�u�G���g����ǉ� */
ST_FUNC void greloca(Section* s, Sym* sym, unsigned long offset, int type,
    addr_t addend)
{
    int c = 0;

    if (nocode_wanted && s == cur_text_section)
        return;

    if (sym) {
        if (0 == sym->c)
            put_extern_sym(sym, NULL, 0, 0);
        c = sym->c;
    }

    /* ����� ELF �Ĕz�u����ǉ��ł��� */
    put_elf_reloca(symtab_section, s, offset, type, c, addend);
}

#if PTR_SIZE == 4
ST_FUNC void greloc(Section* s, Sym* sym, unsigned long offset, int type)
{
    greloca(s, sym, offset, type, 0);
}
#endif

/* ------------------------------------------------------------------------- */
/* �V���{���A���P�[�^ */
static Sym* __sym_malloc(void)
{
    Sym* sym_pool, * sym, * last_sym;
    int i;

    sym_pool = tcc_malloc(SYM_POOL_NB * sizeof(Sym));
    dynarray_add(&sym_pools, &nb_sym_pools, sym_pool);

    last_sym = sym_free_first;
    sym = sym_pool;
    for (i = 0; i < SYM_POOL_NB; i++) {
        sym->next = last_sym;
        last_sym = sym;
        sym++;
    }
    sym_free_first = last_sym;
    return last_sym;
}

static inline Sym* sym_malloc(void)
{
    Sym* sym;
#ifndef SYM_DEBUG
    sym = sym_free_first;
    if (!sym)
        sym = __sym_malloc();
    sym_free_first = sym->next;
    return sym;
#else
    sym = tcc_malloc(sizeof(Sym));
    return sym;
#endif
}

ST_INLN void sym_free(Sym* sym)
{
#ifndef SYM_DEBUG
    sym->next = sym_free_first;
    sym_free_first = sym;
#else
    tcc_free(sym);
#endif
}

/* �n�b�V�����g�킸�Ƀv�b�V�� */
ST_FUNC Sym* sym_push2(Sym** ps, int v, int t, int c)
{
    Sym* s;

    s = sym_malloc();
    memset(s, 0, sizeof * s);
    s->v = v;
    s->type.t = t;
    s->c = c;
    /* add in stack */
    s->prev = *ps;
    *ps = s;
    return s;
}

/* �V���{�����������Ή�����\���̂�Ԃ��B's' �̓V���{���X�^�b�N�̃g�b�v */
ST_FUNC Sym* sym_find2(Sym* s, int v)
{
    while (s) {
        if (s->v == v)
            return s;
        s = s->prev;
    }
    return NULL;
}

/* �\���̂̌��� */
ST_INLN Sym* struct_find(int v)
{
    v -= TOK_IDENT;
    if ((unsigned)v >= (unsigned)(tok_ident - TOK_IDENT))
        return NULL;
    return table_ident[v]->sym_struct;
}

/* ���ʎq������ */
ST_INLN Sym* sym_find(int v)
{
    v -= TOK_IDENT;
    if ((unsigned)v >= (unsigned)(tok_ident - TOK_IDENT))
        return NULL;
    return table_ident[v]->sym_identifier;
}

static int sym_scope(Sym* s)
{
    if (IS_ENUM_VAL(s->type.t))
        return s->type.ref->sym_scope;
    else
        return s->sym_scope;
}

/* �w�肳�ꂽ�V���{�����V���{���X�^�b�N�Ƀv�b�V�� */
ST_FUNC Sym* sym_push(int v, CType* type, int r, int c)
{
    Sym* s, ** ps;
    TokenSym* ts;

    if (local_stack)
        ps = &local_stack;
    else
        ps = &global_stack;
    s = sym_push2(ps, v, type->t, c);
    s->type.ref = type->ref;
    s->r = r;
    /* �t�B�[���h�⓽���V���{���͋L�^���Ȃ� */
    /* XXX: simplify */
    if (!(v & SYM_FIELD) && (v & ~SYM_STRUCT) < SYM_FIRST_ANOM) {
        /* �g�[�N���z��ɃV���{�����L�^ */
        ts = table_ident[(v & ~SYM_STRUCT) - TOK_IDENT];
        if (v & SYM_STRUCT)
            ps = &ts->sym_struct;
        else
            ps = &ts->sym_identifier;
        s->prev_tok = *ps;
        *ps = s;
        s->sym_scope = local_scope;
        if (s->prev_tok && sym_scope(s->prev_tok) == s->sym_scope) {
            if (tcc_state->cpp && (type->t & VT_BTYPE) == VT_FUNC) {
                if (is_compatible_types(&s->prev_tok->type, type))
                    tcc_error("redefinition of '%s'",
                        get_tok_str(v & ~SYM_STRUCT, NULL));
            } else
                tcc_error("�Ē�`: '%s'",
                get_tok_str(v & ~SYM_STRUCT, NULL));
        }
    }
    return s;
}

/* �O���[�o�����ʎq���v�b�V�� */
ST_FUNC Sym* global_identifier_push(int v, int t, int c)
{
    Sym* s, ** ps;
    s = sym_push2(&global_stack, v, t, c);
    s->r = VT_CONST | VT_SYM;
    /* don't record anonymous symbol */
    if (v < SYM_FIRST_ANOM) {
        ps = &table_ident[v - TOK_IDENT]->sym_identifier;
        /* �g�b�v�̃��[�J�����ʎq��ύX���A�|�b�v���ꂽ�Ƃ���
           sym_identifier �� 's' ���w���悤�ɂ���G���̏����̓C�����C�� asm ����
           �Ăяo���ꂽ�Ƃ��ɔ������� */
        while (*ps != NULL && (*ps)->sym_scope)
            ps = &(*ps)->prev_tok;
        s->prev_tok = *ps;
        *ps = s;
    }
    return s;
}

/* �g�b�v�� 'b' �ɓ��B����܂ŃV���{�����|�b�v����BKEEP ����[���̏ꍇ��
    ���ۂɂ̓��X�g�����菜�����A�g�[�N���z�񂩂�̂ݍ폜���� */
ST_FUNC void sym_pop(Sym** ptop, Sym* b, int keep)
{
    Sym* s, * ss, ** ps;
    TokenSym* ts;
    int v;

    s = *ptop;
    while (s != b) {
        ss = s->prev;
        v = s->v;
        /* remove symbol in token array */
        /* XXX: simplify */
        if (!(v & SYM_FIELD) && (v & ~SYM_STRUCT) < SYM_FIRST_ANOM) {
            ts = table_ident[(v & ~SYM_STRUCT) - TOK_IDENT];
            if (v & SYM_STRUCT)
                ps = &ts->sym_struct;
            else
                ps = &ts->sym_identifier;
            *ps = s->prev_tok;
        }
        if (!keep)
            sym_free(s);
        s = ss;
    }
    if (!keep)
        *ptop = b;
}

/* ���x���̌��� */
ST_FUNC Sym* label_find(int v)
{
    v -= TOK_IDENT;
    if ((unsigned)v >= (unsigned)(tok_ident - TOK_IDENT))
        return NULL;
    return table_ident[v]->sym_label;
}

ST_FUNC Sym* label_push(Sym** ptop, int v, int flags)
{
    Sym* s, ** ps;
    s = sym_push2(ptop, v, VT_STATIC, 0);
    s->r = flags;
    ps = &table_ident[v - TOK_IDENT]->sym_label;
    if (ptop == &global_label_stack) {
        /* modify the top most local identifier, so that
           sym_identifier will point to 's' when popped */
        while (*ps != NULL)
            ps = &(*ps)->prev_tok;
    }
    s->prev_tok = *ps;
    *ps = s;
    return s;
}

/* �Ō�̗v�f�ɒB����܂Ń��x�����|�b�v����B����`�̃��x�����Ȃ����m�F����B
    '&&label' ���g���Ă���΃V���{�����`���� */
ST_FUNC void label_pop(Sym** ptop, Sym* slast, int keep)
{
    Sym* s, * s1;
    for (s = *ptop; s != slast; s = s1) {
        s1 = s->prev;
        if (s->r == LABEL_DECLARED) {
            tcc_warning_c(warn_all)("���x�� '%s' �͐錾����Ă��܂����g�p����Ă��܂���", get_tok_str(s->v, NULL));
        }
        else if (s->r == LABEL_FORWARD) {
            tcc_error("���x�� '%s' ���g�p����Ă��܂�����`����Ă��܂���",
                get_tok_str(s->v, NULL));
        }
        else {
            if (s->c) {
                /* define corresponding symbol. A size of
                   1 is put. */
                put_extern_sym(s, cur_text_section, s->jnext, 1);
            }
        }
        /* remove label */
        if (s->r != LABEL_GONE)
            table_ident[s->v - TOK_IDENT]->sym_label = s->prev_tok;
        if (!keep)
            sym_free(s);
        else
            s->r = LABEL_GONE;
    }
    if (!keep)
        *ptop = slast;
}

/* ------------------------------------------------------------------------- */
static void vcheck_cmp(void)
{
    /* ���̖��߂����������ꍇ�ɂ� CPU �t���O���c���Ă����Ȃ��B�܂�
       VT_JMP ���X�^�b�N�̐擪�ȊO�Ɏc���ƃR�[�h�����킪���G�ɂȂ邽�ߔ�����B

       nocode_wanted �̂Ƃ��͂�����s���Ă͂����Ȃ��Bvtop ��
       !nocode_wanted �̗̈悩�痈��ꍇ������i88_codeopt.c ���Q�Ɓj�A
       ���ۂɃR�[�h�𐶐��������W�X�^�ɕϊ�����ƁA�l�����ۂɎg�p�����\����������ƂȂ�B
       nocode_wanted ���Ńv�b�V�����ꂽ�S�Ă̒l�͍ŏI�I�Ƀ|�b�v����,
       �R�[�h�}�~���������ꂽ�Ƃ��� VT_CMP/VT_JMP �̒l�� vtop �ɖ߂�B */

       /* �������ACODE_OFF/ON() �ɂ�鎩���}�~�����̏ꍇ�́A���̂܂ܓ����W���Ȃ������ǂ��B
          nocode_wanted ���łǂ̂悤�ɋ@�\����̂��Hgv() �����[�h/VT_JMP �ɂ����� gsym() ��
          ���ۂɃN���A���邽�߂ł���i�W�F�l���[�^�̃o�b�N�G���h�Q�Ɓj�B */

    if (vtop->r == VT_CMP && 0 == (nocode_wanted & ~CODE_OFF_BIT))
        gv(RC_INT);
}

static void vsetc(CType* type, int r, CValue* vc)
{
    if (vtop >= vstack + (VSTACK_SIZE - 1))
        tcc_error("�������s�� (vstack)");
    vcheck_cmp();
    vtop++;
    vtop->type = *type;
    vtop->r = r;
    vtop->r2 = VT_CONST;
    vtop->c = *vc;
    vtop->sym = NULL;
}

ST_FUNC void vswap(void)
{
    SValue tmp;

    vcheck_cmp();
    tmp = vtop[0];
    vtop[0] = vtop[-1];
    vtop[-1] = tmp;
}

/* �X�^�b�N�l���|�b�v */
ST_FUNC void vpop(void)
{
    int v;
    v = vtop->r & VT_VALMASK;
#if defined(TCC_TARGET_I386) || defined(TCC_TARGET_X86_64)
    /* x86 �ł� FPU �X�^�b�N���|�b�v����K�v������ */
    if (v == TREG_ST0) {
        o(0xd8dd); /* fstp %st(0) */
    }
    else
#endif
        if (v == VT_CMP) {
            /* && �� || �̃e�X�g�����̏ꍇ�A�������W�����v�𐶐����� */
            gsym(vtop->jtrue);
            gsym(vtop->jfalse);
        }
    vtop--;
}

/* �^ 'type' �̒萔�i�_�~�[�l�j���v�b�V�� */
static void vpush(CType* type)
{
    vset(type, VT_CONST, 0);
}

/* �C�ӂ� 64bit �萔���v�b�V�� */
static void vpush64(int ty, unsigned long long v)
{
    CValue cval;
    CType ctype;
    ctype.t = ty;
    ctype.ref = NULL;
    cval.i = v;
    vsetc(&ctype, VT_CONST, &cval);
}

/* �����萔���v�b�V�� */
ST_FUNC void vpushi(int v)
{
    vpush64(VT_INT, v);
}

/* �|�C���^�T�C�Y�̒萔���v�b�V�� */
static void vpushs(addr_t v)
{
    vpush64(VT_SIZE_T, v);
}

/* long long �^�̒萔���v�b�V�� */
static inline void vpushll(long long v)
{
    vpush64(VT_LLONG, v);
}

ST_FUNC void vset(CType* type, int r, int v)
{
    CValue cval;
    cval.i = v;
    vsetc(type, r, &cval);
}

static void vseti(int r, int v)
{
    CType type;
    type.t = VT_INT;
    type.ref = NULL;
    vset(&type, r, v);
}

ST_FUNC void vpushv(SValue* v)
{
    if (vtop >= vstack + (VSTACK_SIZE - 1))
        tcc_error("�������s�� (vstack)");
    vtop++;
    *vtop = *v;
}

static void vdup(void)
{
    vpushv(vtop);
}

/* �X�^�b�N�̈ʒu n-1 �̗v�f���g�b�v�։�] */
ST_FUNC void vrotb(int n)
{
    SValue tmp;
    if (--n < 1)
        return;
    vcheck_cmp();
    tmp = vtop[-n];
    memmove(vtop - n, vtop - n + 1, sizeof * vtop * n);
    vtop[0] = tmp;
}

/* �X�^�b�N�̃g�b�v�v�f���ʒu n-1 �ɉ�] */
ST_FUNC void vrott(int n)
{
    SValue tmp;
    if (--n < 1)
        return;
    vcheck_cmp();
    tmp = vtop[0];
    memmove(vtop - n + 1, vtop - n, sizeof * vtop * n);
    vtop[-n] = tmp;
}

/* �X�^�b�N�̐擪 n �v�f�̏����𔽓] */
ST_FUNC void vrev(int n)
{
    int i;
    SValue tmp;
    vcheck_cmp();
    for (i = 0, n = -n; i > ++n; --i)
        tmp = vtop[i], vtop[i] = vtop[n], vtop[n] = tmp;
}

/* ------------------------------------------------------------------------- */
/* vtop->r = VT_CMP �͔�r��e�X�g�ɂ�� CPU �t���O���ݒ肳��Ă��邱�Ƃ��Ӗ����� */

/* �W�F�l���[�^����Ă΂�A�֌W���Z�̌��ʂ�ݒ肷�� */
ST_FUNC void vset_VT_CMP(int op)
{
    vtop->r = VT_CMP;
    vtop->cmp_op = op;
    vtop->jfalse = 0;
    vtop->jtrue = 0;
}

/* �W�F�l���[�^�� VT_CMP �����W�X�^�փ��[�h������O�Ɉ�x�Ă΂�� */
static void vset_VT_JMP(void)
{
    int op = vtop->cmp_op;

    if (vtop->jtrue || vtop->jfalse) {
        int origt = vtop->type.t;
        /* 'mov $0,%R' �܂��� 'mov $1,%R' �փW�����v����K�v������ */
        int inv = op & (op < 2); /* small optimization */
        vseti(VT_JMP + inv, gvtst(inv, 0));
        vtop->type.t |= origt & (VT_UNSIGNED | VT_DEFSIGN);
    }
    else {
        /* ����ȊO�̏ꍇ�̓t���O�i0/1�j�����W�X�^�ɕϊ����� */
        vtop->c.i = op;
        if (op < 2) /* doesn't seem to happen */
            vtop->r = VT_CONST;
    }
}

/* CPU �t���O��ݒ肷��i�܂��W�����v�͍s��Ȃ��j */
static void gvtst_set(int inv, int t)
{
    int* p;

    if (vtop->r != VT_CMP) {
        vpushi(0);
        gen_op(TOK_NE);
        if (vtop->r != VT_CMP) /* must be VT_CONST then */
            vset_VT_CMP(vtop->c.i != 0);
    }

    p = inv ? &vtop->jfalse : &vtop->jtrue;
    *p = gjmp_append(*p, t);
}

/* �l�e�X�g�̐���
 *
 * �C�ӂ̒l�ɑ΂���e�X�g�𐶐�����i�W�����v�A��r�A�������܂ށj */
static int gvtst(int inv, int t)
{
    int op, x, u;

    gvtst_set(inv, t);
    t = vtop->jtrue, u = vtop->jfalse;
    if (inv)
        x = u, u = t, t = x;
    op = vtop->cmp_op;

    /* jump to the wanted target */
    if (op > 1)
        t = gjmp_cond(op ^ inv, t);
    else if (op != inv)
        t = gjmp(t);
    /* resolve complementary jumps to here */
    gsym(u);

    vtop--;
    return t;
}

/* �[��/��[���̃e�X�g�𐶐� */
static void gen_test_zero(int op)
{
    if (vtop->r == VT_CMP) {
        int j;
        if (op == TOK_EQ) {
            j = vtop->jfalse;
            vtop->jfalse = vtop->jtrue;
            vtop->jtrue = j;
            vtop->cmp_op ^= 1;
        }
    }
    else {
        vpushi(0);
        gen_op(op);
    }
}

/* ------------------------------------------------------------------------- */
/* �w��^�̃V���{���l���v�b�V�� */
ST_FUNC void vpushsym(CType* type, Sym* sym)
{
    CValue cval;
    cval.i = 0;
    vsetc(type, VT_CONST | VT_SYM, &cval);
    vtop->sym = sym;
}

/* �Z�N�V�������w���ÓI�V���{����Ԃ� */
ST_FUNC Sym* get_sym_ref(CType* type, Section* sec, unsigned long offset, unsigned long size)
{
    int v;
    Sym* sym;

    v = anon_sym++;
    sym = sym_push(v, type, VT_CONST | VT_SYM, 0);
    sym->type.t |= VT_STATIC;
    put_extern_sym(sym, sec, offset, size);
    return sym;
}

/* �_�~�[�V���{����ǉ����ăZ�N�V�����ւ̎Q�Ƃ��v�b�V�� */
static void vpush_ref(CType* type, Section* sec, unsigned long offset, unsigned long size)
{
    vpushsym(type, get_sym_ref(type, sec, offset, size));
}

/* �^ 'u' �̃V���{�� 'v' �ւ̐V�����O���Q�Ƃ��` */
ST_FUNC Sym* external_global_sym(int v, CType* type)
{
    Sym* s;

    s = sym_find(v);
    if (!s) {
        /* push forward reference */
        s = global_identifier_push(v, type->t | VT_EXTERN, 0);
        s->type.ref = type->ref;
    }
    else if (IS_ASM_SYM(s)) {
        s->type.t = type->t | (s->type.t & VT_EXTERN);
        s->type.ref = type->ref;
        update_storage(s);
    }
    return s;
}

/* asm ���x���Ɏ����^�w��̂Ȃ��O���Q�Ƃ��쐬����B
    ����ɂ��V���{���� C ����g���Ă��^�̏Փ˂�������� */
ST_FUNC Sym* external_helper_sym(int v)
{
    CType ct = { VT_ASM_FUNC, NULL };
    return external_global_sym(v, &ct);
}

/* �w���p�֐��i��: memmove�j�ւ̎Q�Ƃ��v�b�V�� */
ST_FUNC void vpush_helper_func(int v)
{
    vpushsym(&func_old_type, external_helper_sym(v));
}

/* �V���{���������}�[�W���� */
static void merge_symattr(struct SymAttr* sa, struct SymAttr* sa1)
{
    if (sa1->aligned && !sa->aligned)
        sa->aligned = sa1->aligned;
    sa->packed |= sa1->packed;
    sa->weak |= sa1->weak;
    sa->nodebug |= sa1->nodebug;
    if (sa1->visibility != STV_DEFAULT) {
        int vis = sa->visibility;
        if (vis == STV_DEFAULT
            || vis > sa1->visibility)
            vis = sa1->visibility;
        sa->visibility = vis;
    }
    sa->dllexport |= sa1->dllexport;
    sa->nodecorate |= sa1->nodecorate;
    sa->dllimport |= sa1->dllimport;
}

/* �֐��������}�[�W���� */
static void merge_funcattr(struct FuncAttr* fa, struct FuncAttr* fa1)
{
    if (fa1->func_call && !fa->func_call)
        fa->func_call = fa1->func_call;
    if (fa1->func_type && !fa->func_type)
        fa->func_type = fa1->func_type;
    if (fa1->func_args && !fa->func_args)
        fa->func_args = fa1->func_args;
    if (fa1->func_noreturn)
        fa->func_noreturn = 1;
    if (fa1->func_ctor)
        fa->func_ctor = 1;
    if (fa1->func_dtor)
        fa->func_dtor = 1;
    if (fa1->func_virtual)
        fa->func_virtual = 1;
    if (fa1->func_const)
        fa->func_const = 1;
}

/* �������}�[�W���� */
static void merge_attr(AttributeDef* ad, AttributeDef* ad1)
{
    merge_symattr(&ad->a, &ad1->a);
    merge_funcattr(&ad->f, &ad1->f);

    if (ad1->section)
        ad->section = ad1->section;
    if (ad1->alias_target)
        ad->alias_target = ad1->alias_target;
    if (ad1->asm_label)
        ad->asm_label = ad1->asm_label;
    if (ad1->attr_mode)
        ad->attr_mode = ad1->attr_mode;
}

/* �^�����̈ꕔ���}�[�W���� */
static void patch_type(Sym* sym, CType* type)
{
    if (!(type->t & VT_EXTERN) || IS_ENUM_VAL(sym->type.t)) {
        if (!(sym->type.t & VT_EXTERN))
            tcc_error("�Ē�`: '%s'", get_tok_str(sym->v, NULL));
        sym->type.t &= ~VT_EXTERN;
    }

    if (IS_ASM_SYM(sym)) {
        /* stay static if both are static */
        sym->type.t = type->t & (sym->type.t | ~VT_STATIC);
        sym->type.ref = type->ref;
        if ((type->t & VT_BTYPE) != VT_FUNC && !(type->t & VT_ARRAY))
            sym->r |= VT_LVAL;
    }

    if (!is_compatible_types(&sym->type, type)) {
        tcc_error("�Ē�`�̌^���݊���������܂���: '%s'",
            get_tok_str(sym->v, NULL));

    }
    else if ((sym->type.t & VT_BTYPE) == VT_FUNC) {
        int static_proto = sym->type.t & VT_STATIC;
        /* warn if static follows non-static function declaration */
        if ((type->t & VT_STATIC) && !static_proto
            /* XXX this test for inline shouldn't be here.  Until we
               implement gnu-inline mode again it silences a warning for
               mingw caused by our workarounds.  */
            && !((type->t | sym->type.t) & VT_INLINE))
            tcc_warning("�Ē�`: '%s' �� static �w��͖�������܂�",
                get_tok_str(sym->v, NULL));

        /* set 'inline' if both agree or if one has static */
        if ((type->t | sym->type.t) & VT_INLINE) {
            if (!((type->t ^ sym->type.t) & VT_INLINE)
                || ((type->t | sym->type.t) & VT_STATIC))
                static_proto |= VT_INLINE;
        }

        if (0 == (type->t & VT_EXTERN)) {
            struct FuncAttr f = sym->type.ref->f;
            /* put complete type, use static from prototype */
            sym->type.t = (type->t & ~(VT_STATIC | VT_INLINE)) | static_proto;
            sym->type.ref = type->ref;
            merge_funcattr(&sym->type.ref->f, &f);
        }
        else {
            sym->type.t &= ~VT_INLINE | static_proto;
        }

        if (sym->type.ref->f.func_type == FUNC_OLD
            && type->ref->f.func_type != FUNC_OLD) {
            sym->type.ref = type->ref;
        }

    }
    else {
        if ((sym->type.t & VT_ARRAY) && type->ref->c >= 0) {
            /* set array size if it was omitted in extern declaration */
            sym->type.ref->c = type->ref->c;
        }
        if ((type->t ^ sym->type.t) & VT_STATIC)
            tcc_warning("�Ē�`: '%s' �̃X�g���[�W�w�肪�s��v�ł�",
                get_tok_str(sym->v, NULL));
    }
}

/* �X�g���[�W�����̈ꕔ���}�[�W���� */
static void patch_storage(Sym* sym, AttributeDef* ad, CType* type)
{
    if (type)
        patch_type(sym, type);

#ifdef TCC_TARGET_PE
    if (sym->a.dllimport != ad->a.dllimport)
        tcc_error("�Ē�` '%s' �� DLL �����P�[�W���݊���������܂���",
            get_tok_str(sym->v, NULL));
#endif
    merge_symattr(&sym->a, &ad->a);
    if (ad->asm_label)
        sym->asm_label = ad->asm_label;
    update_storage(sym);
}

/* sym ��ʂ̃X�^�b�N�փR�s�[ */
static Sym* sym_copy(Sym* s0, Sym** ps)
{
    Sym* s;
    s = sym_malloc(), * s = *s0;
    s->prev = *ps, * ps = s;
    if (s->v < SYM_FIRST_ANOM) {
        ps = &table_ident[s->v - TOK_IDENT]->sym_identifier;
        s->prev_tok = *ps, * ps = s;
    }
    return s;
}

/* VT_FUNC �� VT_PTR �̂��߂� s->type.ref ���X�^�b�N 'ps' �ɃR�s�[ */
static void sym_copy_ref(Sym* s, Sym** ps)
{
    int bt = s->type.t & VT_BTYPE;
    if (bt == VT_FUNC || bt == VT_PTR || (bt == VT_STRUCT && s->sym_scope)) {
        Sym** sp = &s->type.ref;
        for (s = *sp, *sp = NULL; s; s = s->next) {
            Sym* s2 = sym_copy(s, ps);
            sp = &(*sp = s2)->next;
            sym_copy_ref(s2, ps);
        }
    }
}
static Sym* external_sym(int v, CType* type, int r, AttributeDef* ad)
{
    Sym* s;

    /* �O���[�o���V���{����T�� */
    s = sym_find(v);
    while (s && s->sym_scope)
        s = s->prev_tok;

    if (!s) {
        /* ��s�Q�Ƃ��v�b�V������ */
        s = global_identifier_push(v, type->t, 0);
        s->r |= r;
        s->a = ad->a;
        s->asm_label = ad->asm_label;
        s->type.ref = type->ref;
        /* copy type to the global stack */
        if (local_stack)
            sym_copy_ref(s, &global_stack);
        if (tcc_state->cpp && (type->t & VT_BTYPE) == VT_FUNC)
            cpp_set_func_mangle_label(s, type);
    }
    else {
        if (tcc_state->cpp && (type->t & VT_BTYPE) == VT_FUNC
            && (s->type.t & VT_BTYPE) == VT_FUNC
            && !is_compatible_types(&s->type, type)) {
            s = global_identifier_push(v, type->t, 0);
            s->r |= r;
            s->a = ad->a;
            s->asm_label = ad->asm_label;
            s->type.ref = type->ref;
            if (local_stack)
                sym_copy_ref(s, &global_stack);
            cpp_set_func_mangle_label(s, type);
        } else {
            patch_storage(s, ad, type);
        }
    }
    /* ���[�J���X�^�b�N�ɕϐ�������΃v�b�V������ */
    if (local_stack && (s->type.t & VT_BTYPE) != VT_FUNC)
        s = sym_copy(s, &local_stack);
    return s;
}

/* (vtop - n) �̃X�^�b�N�G���g���܂ł̃��W�X�^��ۑ� */

ST_FUNC void save_regs(int n)
{
    SValue* p, * p1;
    for (p = vstack, p1 = vtop - n; p <= p1; p++)
        save_reg(p->r);
}

/* ���W�X�^ r ���������X�^�b�N�ɕۑ����A�󂫂Ƃ��ă}�[�N���� */
ST_FUNC void save_reg(int r)
{
    save_reg_upstack(r, 0);
}

/* ���W�X�^ r ���������X�^�b�N�ɕۑ����A(vtop - n) �܂łɌ��������ꍇ��
    �X�^�b�N��ɕۑ�����Ă��邱�Ƃ������ċ󂫂Ƃ��ă}�[�N���� */
ST_FUNC void save_reg_upstack(int r, int n)
{
    int l, size, align, bt, r2;
    SValue* p, * p1, sv;

    if ((r &= VT_VALMASK) >= VT_CONST)
        return;
    if (nocode_wanted)
        return;
    l = r2 = 0;
    for (p = vstack, p1 = vtop - n; p <= p1; p++) {
        if ((p->r & VT_VALMASK) == r || p->r2 == r) {
            /* �܂��ۑ�����Ă��Ȃ��ꍇ�͒l���X�^�b�N�ɕۑ�����K�v������ */
            if (!l) {
                bt = p->type.t & VT_BTYPE;
                if (bt == VT_VOID)
                    continue;
                if ((p->r & VT_LVAL) || bt == VT_FUNC)
                    bt = VT_PTR;
                sv.type.t = bt;
                size = type_size(&sv.type, &align);
                l = get_temp_local_var(size, align, &r2);
                sv.r = VT_LOCAL | VT_LVAL;
                sv.c.i = l;
                store(p->r & VT_VALMASK, &sv);
#if defined(TCC_TARGET_I386) || defined(TCC_TARGET_X86_64)
                /* x86 �ŗL: �ۑ�����Ă���ꍇ�� FP ���W�X�^ ST0 ���|�b�v����K�v������ */
                if (r == TREG_ST0) {
                    o(0xd8dd); /* fstp %st(0) */
                }
#endif
                /* long long �̓���P�[�X */
                if (p->r2 < VT_CONST && USING_TWO_WORDS(bt)) {
                    sv.c.i += PTR_SIZE;
                    store(p->r2, &sv);
                }
            }
            /* ���̃X�^�b�N�G���g�����X�^�b�N��ɕۑ����ꂽ���Ƃ����� */
            if (p->r & VT_LVAL) {
                /* also clear the bounded flag because the
                   relocation address of the function was stored in
                   p->c.i */
                p->r = (p->r & ~(VT_VALMASK | VT_BOUNDED)) | VT_LLOCAL;
            }
            else {
                p->r = VT_LVAL | VT_LOCAL;
                p->type.t &= ~VT_ARRAY; /* cannot combine VT_LVAL with VT_ARRAY */
            }
            p->sym = NULL;
            p->r2 = r2;
            p->c.i = l;
        }
    }
}

#ifdef TCC_TARGET_ARM
/* �X�^�b�N��ōő�1�����Q�Ƃ���Ă��Ȃ��N���X 'rc2' �̃��W�X�^��T���B
 * ������Ȃ���� get_reg(rc) ���Ă� */
ST_FUNC int get_reg_ex(int rc, int rc2)
{
    int r;
    SValue* p;

    for (r = 0; r < NB_REGS; r++) {
        if (reg_classes[r] & rc2) {
            int n;
            n = 0;
            for (p = vstack; p <= vtop; p++) {
                if ((p->r & VT_VALMASK) == r ||
                    p->r2 == r)
                    n++;
            }
            if (n <= 1)
                return r;
        }
    }
    return get_reg(rc);
}
#endif

/* �N���X 'rc' �̋󂫃��W�X�^��T���B�������1�ۑ����Ċm�ۂ��� */
ST_FUNC int get_reg(int rc)
{
    int r;
    SValue* p;

    /* �󂫃��W�X�^��T�� */
    for (r = 0; r < NB_REGS; r++) {
        if (reg_classes[r] & rc) {
            if (nocode_wanted)
                return r;
            for (p = vstack; p <= vtop; p++) {
                if ((p->r & VT_VALMASK) == r ||
                    p->r2 == r)
                    goto notfound;
            }
            return r;
        }
    notfound:;
    }

    /* �󂫃��W�X�^���Ȃ�: �X�^�b�N��̍ŏ��̂��̂��������
       �igen_opi() �Ŏg���郌�W�X�^�����ڂ��Ȃ��悤�A���i�{�g���j����
       �J�n���邱�Ƃ����ɏd�v�j */
    for (p = vstack; p <= vtop; p++) {
        /* look at second register (if long long) */
        r = p->r2;
        if (r < VT_CONST && (reg_classes[r] & rc))
            goto save_found;
        r = p->r & VT_VALMASK;
        if (r < VT_CONST && (reg_classes[r] & rc)) {
        save_found:
            save_reg(r);
            return r;
        }
    }
    /* Should never comes here */
    return -1;
}

/* �T�C�Y�ƃA���C���ɍ����ꎞ���[�J���ϐ���T���i�X�^�b�N��̃I�t�Z�b�g��Ԃ��j�B
    ������Ȃ���ΐV�����ꎞ�ϐ���ǉ����� */
static int get_temp_local_var(int size, int align, int* r2)
{
    int i;
    struct temp_local_variable* temp_var;
    SValue* p;
    int r;
    unsigned used = 0;

    /* mark locations that are still in use */
    for (p = vstack; p <= vtop; p++) {
        r = p->r & VT_VALMASK;
        if (r == VT_LOCAL || r == VT_LLOCAL) {
            r = p->r2 - (VT_CONST + 1);
            if (r >= 0 && r < MAX_TEMP_LOCAL_VARIABLE_NUMBER)
                used |= 1 << r;
        }
    }
    for (i = 0; i < nb_temp_local_vars; i++) {
        temp_var = &arr_temp_local_vars[i];
        if (!(used & 1 << i)
            && temp_var->size >= size
            && temp_var->align >= align) {
        ret_tmp:
            *r2 = (VT_CONST + 1) + i;
            return temp_var->location;
        }
    }
    loc = (loc - size) & -align;
    if (nb_temp_local_vars < MAX_TEMP_LOCAL_VARIABLE_NUMBER) {
        temp_var = &arr_temp_local_vars[i];
        temp_var->location = loc;
        temp_var->size = size;
        temp_var->align = align;
        nb_temp_local_vars++;
        goto ret_tmp;
    }
    *r2 = VT_CONST;
    return loc;
}

/* ���W�X�^ 's'�i�^ t�j�� 'r' �Ɉړ����A�K�v�Ȃ�� r �̈ȑO�̒l���������֏����o�� */
static void move_reg(int r, int s, int t)
{
    SValue sv;

    if (r != s) {
        save_reg(r);
        sv.type.t = t;
        sv.type.ref = NULL;
        sv.r = s;
        sv.c.i = 0;
        load(r, &sv);
    }
}

/* vtop �̃A�h���X�𓾂� (vtop �� lvalue �ł���K�v������) */
ST_FUNC void gaddrof(void)
{
    vtop->r &= ~VT_LVAL;
    /* �g���b�L�[: �ۑ����ꂽ lvalue �̏ꍇ�͍Ă� lvalue �ɖ߂��� */
    if ((vtop->r & VT_VALMASK) == VT_LLOCAL)
        vtop->r = (vtop->r & ~VT_VALMASK) | VT_LOCAL | VT_LVAL;
}

#ifdef CONFIG_TCC_BCHECK
/* ���E�t���|�C���^�̉��Z�R�[�h�𐶐� */
static void gen_bounded_ptr_add(void)
{
    int save = (vtop[-1].r & VT_VALMASK) == VT_LOCAL;
    if (save) {
        vpushv(&vtop[-1]);
        vrott(3);
    }
    vpush_helper_func(TOK___bound_ptr_add);
    vrott(3);
    gfunc_call(2);
    vtop -= save;
    vpushi(0);
    /* �߂�|�C���^�� REG_IRET �ɓ��� */
    vtop->r = REG_IRET | VT_BOUNDED;
    if (nocode_wanted)
        return;
    /* ���E�`�F�b�N�֐��Ăяo���n�_�̍Ĕz�u�I�t�Z�b�g */
    vtop->c.i = (cur_text_section->reloc->data_offset - sizeof(ElfW_Rel));
}

/* vtop �̃|�C���^���Z���C�����ă|�C���^�Q�Ƃ��e�X�g���� */
static void gen_bounded_ptr_deref(void)
{
    addr_t func;
    int size, align;
    ElfW_Rel* rel;
    Sym* sym;

    if (nocode_wanted)
        return;

    size = type_size(&vtop->type, &align);
    switch (size) {
    case  1: func = TOK___bound_ptr_indir1; break;
    case  2: func = TOK___bound_ptr_indir2; break;
    case  4: func = TOK___bound_ptr_indir4; break;
    case  8: func = TOK___bound_ptr_indir8; break;
    case 12: func = TOK___bound_ptr_indir12; break;
    case 16: func = TOK___bound_ptr_indir16; break;
    default:
        /* �\���̃����o�A�N�Z�X�Ŕ������邱�Ƃ����� */
        return;
    }
    sym = external_helper_sym(func);
    if (!sym->c)
        put_extern_sym(sym, NULL, 0, 0);
    /* �Ĕz�u���C�� */
    /* XXX: ���ǂ��������������H */
    rel = (ElfW_Rel*)(cur_text_section->reloc->data + vtop->c.i);
    rel->r_info = ELFW(R_INFO)(sym->c, ELFW(R_TYPE)(rel->r_info));
}

/* lvalue �̋��E�`�F�b�N�R�[�h�𐶐� */
static void gbound(void)
{
    CType type1;

    vtop->r &= ~VT_MUSTBOUND;
    /* lvalue �̏ꍇ�A�Q�Ɖ����i�f���t�@�����X�j����O�Ƀ`�F�b�N�R�[�h���g�� */
    if (vtop->r & VT_LVAL) {
        /* VT_BOUNDED �łȂ��l�Ȃ�A���E�t���l���쐬���� */
        if (!(vtop->r & VT_BOUNDED)) {
            /* �|�C���^�𓾂邽�߂Ɍ^�� int �ɐݒ肷��K�v������̂ŁA���̌^��ۑ����� */
            type1 = vtop->type;
            vtop->type.t = VT_PTR;
            gaddrof();
            vpushi(0);
            gen_bounded_ptr_add();
            vtop->r |= VT_LVAL;
            vtop->type = type1;
        }
        /* �����ăf���t�@�����X�̃`�F�b�N���s�� */
        gen_bounded_ptr_deref();
    }
}

/* �֐����������W�X�^�Ƀ��[�h���n�߂�O�� __bound_ptr_add ���ĂԕK�v������ */
ST_FUNC void gbound_args(int nb_args)
{
    int i, v;
    SValue* sv;

    for (i = 1; i <= nb_args; ++i)
        if (vtop[1 - i].r & VT_MUSTBOUND) {
            vrotb(i);
            gbound();
            vrott(i);
        }

    sv = vtop - nb_args;
    if (sv->r & VT_SYM) {
        v = sv->sym->v;
        if (v == TOK_setjmp
            || v == TOK__setjmp
#ifndef TCC_TARGET_PE
            || v == TOK_sigsetjmp
            || v == TOK___sigsetjmp
#endif
            ) {
            vpush_helper_func(TOK___bound_setjmp);
            vpushv(sv + 1);
            gfunc_call(1);
            func_bound_add_epilog = 1;
        }
#if defined TCC_TARGET_I386 || defined TCC_TARGET_X86_64
        if (v == TOK_alloca)
            func_bound_add_epilog = 1;
#endif
#if TARGETOS_NetBSD
        if (v == TOK_longjmp) /* __longjmp14 �ւ̖��O�ύX�����ɖ߂� */
            sv->sym->asm_label = TOK___bound_longjmp;
#endif
    }
}

/* S ���� E �܂ł̃��[�J���V���{���ɑ΂��ċ��E����ǉ�����i->prev ��H��j */
static void add_local_bounds(Sym* s, Sym* e)
{
    for (; s != e; s = s->prev) {
        if (!s->v || (s->r & VT_VALMASK) != VT_LOCAL)
            continue;
        /* �z��E�\���́E���p�̂͏�ɃA�h���X����邽�ߒǉ����� */
        if ((s->type.t & VT_ARRAY)
            || (s->type.t & VT_BTYPE) == VT_STRUCT
            || s->a.addrtaken) {
            /* ���[�J���̋��E����ǉ� */
            int align, size = type_size(&s->type, &align);
            addr_t* bounds_ptr = section_ptr_add(lbounds_section,
                2 * sizeof(addr_t));
            bounds_ptr[0] = s->c;
            bounds_ptr[1] = size;
        }
    }
}
#endif

/* sym_pop �̃��b�p�[�B�K�v�ɉ����ă��[�J���̋��E�����o�^���� */
static void pop_local_syms(Sym* b, int keep)
{
#ifdef CONFIG_TCC_BCHECK
    if (tcc_state->do_bounds_check && !keep && (local_scope || !func_var))
        add_local_bounds(local_stack, b);
#endif
    if (debug_modes)
        tcc_add_debug_info(tcc_state, !local_scope, local_stack, b);
    sym_pop(&local_stack, b, keep);
}

/* lvalue �|�C���^�̃I�t�Z�b�g�𑝂₷�i�|�C���^���C���N�������g�j */
static void incr_offset(int offset)
{
    int t = vtop->type.t;
    gaddrof(); /* VT_LVAL ����菜�� */
    vtop->type.t = VT_PTRDIFF_T; /* �X�J���[�^�ɐݒ� */
    vpushs(offset);
    gen_op('+');
    vtop->r |= VT_LVAL;
    vtop->type.t = t;
}

static void incr_bf_adr(int o)
{
    vtop->type.t = VT_BYTE | VT_UNSIGNED;
    incr_offset(o);
}

/* �p�b�N���ꂽ�A�܂��͐��񂳂�Ă��Ȃ��r�b�g�t�B�[���h�̂��߂̃o�C�g�P�ʃ��[�h���[�h */
static void load_packed_bf(CType* type, int bit_pos, int bit_size)
{
    int n, o, bits;
    save_reg_upstack(vtop->r, 1);
    vpush64(type->t & VT_BTYPE, 0); // B X
    bits = 0, o = bit_pos >> 3, bit_pos &= 7;
    do {
        vswap(); // X B
        incr_bf_adr(o);
        vdup(); // X B B
        n = 8 - bit_pos;
        if (n > bit_size)
            n = bit_size;
        if (bit_pos)
            vpushi(bit_pos), gen_op(TOK_SHR), bit_pos = 0; // X B Y
        if (n < 8)
            vpushi((1 << n) - 1), gen_op('&');
        gen_cast(type);
        if (bits)
            vpushi(bits), gen_op(TOK_SHL);
        vrotb(3); // B Y X
        gen_op('|'); // B X
        bits += n, bit_size -= n, o = 1;
    } while (bit_size);
    vswap(), vpop();
    if (!(type->t & VT_UNSIGNED)) {
        n = ((type->t & VT_BTYPE) == VT_LLONG ? 64 : 32) - bits;
        vpushi(n), gen_op(TOK_SHL);
        vpushi(n), gen_op(TOK_SAR);
    }
}

/* �p�b�N���ꂽ�A�܂��͐��񂳂�Ă��Ȃ��r�b�g�t�B�[���h�̂��߂̃o�C�g�P�ʃX�g�A���[�h */
static void store_packed_bf(int bit_pos, int bit_size)
{
    int bits, n, o, m, c;
    c = (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;
    vswap(); // X B
    save_reg_upstack(vtop->r, 1);
    bits = 0, o = bit_pos >> 3, bit_pos &= 7;
    do {
        incr_bf_adr(o); // X B
        vswap(); //B X
        c ? vdup() : gv_dup(); // B V X
        vrott(3); // X B V
        if (bits)
            vpushi(bits), gen_op(TOK_SHR);
        if (bit_pos)
            vpushi(bit_pos), gen_op(TOK_SHL);
        n = 8 - bit_pos;
        if (n > bit_size)
            n = bit_size;
        if (n < 8) {
            m = ((1 << n) - 1) << bit_pos;
            vpushi(m), gen_op('&'); // X B V1
            vpushv(vtop - 1); // X B V1 B
            vpushi(m & 0x80 ? ~m & 0x7f : ~m);
            gen_op('&'); // X B V1 B1
            gen_op('|'); // X B V2
        }
        vdup(), vtop[-1] = vtop[-2]; // X B B V2
        vstore(), vpop(); // X B
        bits += n, bit_size -= n, bit_pos = 0, o = 1;
    } while (bit_size);
    vpop(), vpop();
}

static int adjust_bf(SValue* sv, int bit_pos, int bit_size)
{
    int t;
    if (0 == sv->type.ref)
        return 0;
    t = sv->type.ref->auxtype;
    if (t != -1 && t != VT_STRUCT) {
        sv->type.t = (sv->type.t & ~(VT_BTYPE | VT_LONG)) | t;
        sv->r |= VT_LVAL;
    }
    return t;
}

/* vtop ���N���X 'rc' �ɑ����郌�W�X�^�Ɋi�[����Blvalue �͒l�ɕϊ������B
   �\���̂̂悤�Ƀ��W�X�^�l�ɕϊ��ł��Ȃ��ꍇ�͎g���Ȃ��B */
ST_FUNC int gv(int rc)
{
    int r, r2, r_ok, r2_ok, rc2, bt;
    int bit_pos, bit_size, size, align;

    /* ����: get_reg �� vstack[] ��ύX����\�������� */
    if (vtop->type.t & VT_BITFIELD) {
        CType type;

        bit_pos = BIT_POS(vtop->type.t);
        bit_size = BIT_SIZE(vtop->type.t);
        /* ���[�v������邽�߃r�b�g�t�B�[���h������菜�� */
        vtop->type.t &= ~VT_STRUCT_MASK;

        type.ref = NULL;
        type.t = vtop->type.t & VT_UNSIGNED;
        if ((vtop->type.t & VT_BTYPE) == VT_BOOL)
            type.t |= VT_UNSIGNED;

        r = adjust_bf(vtop, bit_pos, bit_size);

        if ((vtop->type.t & VT_BTYPE) == VT_LLONG)
            type.t |= VT_LLONG;
        else
            type.t |= VT_INT;

        if (r == VT_STRUCT) {
            load_packed_bf(&type, bit_pos, bit_size);
        }
        else {
            int bits = (type.t & VT_BTYPE) == VT_LLONG ? 64 : 32;
            /* �㑱�̉��Z�ŕ�����`�d�����邽�߂� int �ɃL���X�g���� */
            gen_cast(&type);
            /* �V�t�g�𐶐� */
            vpushi(bits - (bit_pos + bit_size));
            gen_op(TOK_SHL);
            vpushi(bits - bit_size);
            /* ����: ���������̏ꍇ�� SHR �֕ϊ������ */
            gen_op(TOK_SAR);
        }
        r = gv(rc);
    }
    else {
        if (is_float(vtop->type.t) &&
            (vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST) {
            /* CPU �͒ʏ핂�������_�萔�𒼐ڈ����Ȃ����߁A�f�[�^�Z�O�����g�Ɋi�[���� */
            init_params p = { rodata_section };
            unsigned long offset;
            size = type_size(&vtop->type, &align);
            if (NODATA_WANTED)
                size = 0, align = 1;
            offset = section_add(p.sec, size, align);
            vpush_ref(&vtop->type, p.sec, offset, size);
            vswap();
            init_putv(&p, &vtop->type, offset);
            vtop->r |= VT_LVAL;
        }
#ifdef CONFIG_TCC_BCHECK
        if (vtop->r & VT_MUSTBOUND)
            gbound();
#endif

        bt = vtop->type.t & VT_BTYPE;

#ifdef TCC_TARGET_RISCV64
        /* XXX: �傫�ȃn�b�N */
        if (bt == VT_LDOUBLE && rc == RC_FLOAT)
            rc = RC_INT;
#endif
        rc2 = RC2_TYPE(bt, rc);

        /* �ă��[�h���K�v�ȏꍇ:
           - �萔
           - lvalue�i�|�C���^���Q�Ɖ�������K�v������j
           - ���łɃ��W�X�^�ɂ��邪�K�؂ȃN���X�łȂ� */
        r = vtop->r & VT_VALMASK;
        r_ok = !(vtop->r & VT_LVAL) && (r < VT_CONST) && (reg_classes[r] & rc);
        r2_ok = !rc2 || ((vtop->r2 < VT_CONST) && (reg_classes[vtop->r2] & rc2));

        if (!r_ok || !r2_ok) {

            if (!r_ok) {
                if (1 /* �P�[�X�ɂ���Ă� 'mov (r),r' ���\ */
                    && r < VT_CONST
                    && (reg_classes[r] & rc)
                    && !rc2
                    )
                    save_reg_upstack(r, 1);
                else
                    r = get_reg(rc);
            }

            if (rc2) {
                int load_type = (bt == VT_QFLOAT) ? VT_DOUBLE : VT_PTRDIFF_T;
                int original_type = vtop->type.t;

                /* 2 ���W�X�^�^�̃��[�h: �ꎞ�I�� 2 ���[�h�Ɋg������ */
                if ((vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST) {
                    /* �萔�����[�h */
                    unsigned long long ll = vtop->c.i;
                    vtop->c.i = ll; /* �ŏ��̃��[�h */
                    load(r, vtop);
                    vtop->r = r; /* ���W�X�^�l��ۑ� */
                    vpushi(ll >> 32); /* 2 �Ԗڂ̃��[�h */
                }
                else if (vtop->r & VT_LVAL) {
                    /* long long �|�C���^�������ŕύX�������Ȃ����߁A
                       ���̃C���X�^���X���X�^�b�N�֑ޔ����� */
                    save_reg_upstack(vtop->r, 1);
                    /* ���������烍�[�h */
                    vtop->type.t = load_type;
                    load(r, vtop);
                    vdup();
                    vtop[-1].r = r; /* ���W�X�^�l��ۑ� */
                    /* 2 �Ԗڂ̃��[�h�𓾂邽�߂Ƀ|�C���^�𑝂₷ */
                    incr_offset(PTR_SIZE);
                }
                else {
                    /* ���W�X�^�Ԃ̈ړ� */
                    if (!r_ok)
                        load(r, vtop);
                    if (r2_ok && vtop->r2 < VT_CONST)
                        goto done;
                    vdup();
                    vtop[-1].r = r; /* ���W�X�^�l��ۑ� */
                    vtop->r = vtop[-1].r2;
                }
                /* 2 �Ԗڂ̃��W�X�^���m�ہBget_reg() ���܂� SValue �� r2 ���J�����悤�Ƃ���_�Ɉˑ����� */
                r2 = get_reg(rc2);
                load(r2, vtop);
                vpop();
                /* 2 �Ԗڂ̃��W�X�^���������� */
                vtop->r2 = r2;
            done:
                vtop->type.t = original_type;
            }
            else {
                if (vtop->r == VT_CMP)
                    vset_VT_JMP();
                /* 1 ���W�X�^�^�̃��[�h */
                load(r, vtop);
            }
        }
        vtop->r = r;
#ifdef TCC_TARGET_C67
        /* double �̓��W�X�^�y�A���g�p */
        if (bt == VT_DOUBLE)
            vtop->r2 = r + 1;
#endif
    }
    return r;
}

/* vtop[-1] �� vtop[0] �����ꂼ�� rc1 �� rc2 �̃N���X�Ő������� */
ST_FUNC void gv2(int rc1, int rc2)
{
    /* �ėp�I�ȃ��W�X�^���ɐ�������B������ VT_JMP �� VT_CMP �̒l��
       �����[�h�G���[������邽�ߏ�ɐ�ɐ�������K�v������ */
    if (vtop->r != VT_CMP && rc1 <= rc2) {
        vswap();
        gv(rc1);
        vswap();
        gv(rc2);
        /* �ŏ��̃��W�X�^�ɑ΂��čă��[�h���K�v�����e�X�g */
        if ((vtop[-1].r & VT_VALMASK) >= VT_CONST) {
            vswap();
            gv(rc1);
            vswap();
        }
    }
    else {
        gv(rc2);
        vswap();
        gv(rc1);
        vswap();
        /* �ŏ��̃��W�X�^�ɑ΂��čă��[�h���K�v�����e�X�g */
        if ((vtop[0].r & VT_VALMASK) >= VT_CONST) {
            gv(rc2);
        }
    }
}

#if PTR_SIZE == 4
/* �X�^�b�N��� 64bit �l�� 2 �� int �ɓW�J���� */
ST_FUNC void lexpand(void)
{
    int u, v;
    u = vtop->type.t & (VT_DEFSIGN | VT_UNSIGNED);
    v = vtop->r & (VT_VALMASK | VT_LVAL);
    if (v == VT_CONST) {
        vdup();
        vtop[0].c.i >>= 32;
    }
    else if (v == (VT_LVAL | VT_CONST) || v == (VT_LVAL | VT_LOCAL)) {
        vdup();
        vtop[0].c.i += 4;
    }
    else {
        gv(RC_INT);
        vdup();
        vtop[0].r = vtop[-1].r2;
        vtop[0].r2 = vtop[-1].r2 = VT_CONST;
    }
    vtop[0].type.t = vtop[-1].type.t = VT_INT | u;
}
#endif

#if PTR_SIZE == 4
/* 2 �� int ���� long long ���\�z���� */
static void lbuild(int t)
{
    gv2(RC_INT, RC_INT);
    vtop[-1].r2 = vtop[0].r;
    vtop[-1].type.t = t;
    vpop();
}
#endif

/* �X�^�b�N�G���g�������W�X�^�ɕϊ����A���̒l��ʂ̃��W�X�^�ɕ������� */
static void gv_dup(void)
{
    int t, rc, r;

    t = vtop->type.t;
#if PTR_SIZE == 4
    if ((t & VT_BTYPE) == VT_LLONG) {
        if (t & VT_BITFIELD) {
            gv(RC_INT);
            t = vtop->type.t;
        }
        lexpand();
        gv_dup();
        vswap();
        vrotb(3);
        gv_dup();
        vrotb(4);
        /* stack: H L L1 H1 */
        lbuild(t);
        vrotb(3);
        vrotb(3);
        vswap();
        lbuild(t);
        vswap();
        return;
    }
#endif
    /* �l�𕡐� */
    rc = RC_TYPE(t);
    gv(rc);
    r = get_reg(rc);
    vdup();
    load(r, vtop);
    vtop->r = r;
}

#if PTR_SIZE == 4
/* CPU ��ˑ��́i���������jlong long ���Z�𐶐����� */
static void gen_opl(int op)
{
    int t, a, b, op1, c, i;
    int func;
    unsigned short reg_iret = REG_IRET;
    unsigned short reg_lret = REG_IRE2;
    SValue tmp;

    switch (op) {
    case '/':
    case TOK_PDIV:
        func = TOK___divdi3;
        goto gen_func;
    case TOK_UDIV:
        func = TOK___udivdi3;
        goto gen_func;
    case '%':
        func = TOK___moddi3;
        goto gen_mod_func;
    case TOK_UMOD:
        func = TOK___umoddi3;
    gen_mod_func:
#ifdef TCC_ARM_EABI
        reg_iret = TREG_R2;
        reg_lret = TREG_R3;
#endif
    gen_func:
        /* �ėp�� long long �֐����Ăяo�� */
        vpush_helper_func(func);
        vrott(3);
        gfunc_call(2);
        vpushi(0);
        vtop->r = reg_iret;
        vtop->r2 = reg_lret;
        break;
    case '^':
    case '&':
    case '|':
    case '*':
    case '+':
    case '-':
        //pv("gen_opl A",0,2);
        t = vtop->type.t;
        vswap();
        lexpand();
        vrotb(3);
        lexpand();
        /* stack: L1 H1 L2 H2 */
        tmp = vtop[0];
        vtop[0] = vtop[-3];
        vtop[-3] = tmp;
        tmp = vtop[-2];
        vtop[-2] = vtop[-3];
        vtop[-3] = tmp;
        vswap();
        /* stack: H1 H2 L1 L2 */
        //pv("gen_opl B",0,4);
        if (op == '*') {
            vpushv(vtop - 1);
            vpushv(vtop - 1);
            gen_op(TOK_UMULL);
            lexpand();
            /* stack: H1 H2 L1 L2 ML MH */
            for (i = 0; i < 4; i++)
                vrotb(6);
            /* stack: ML MH H1 H2 L1 L2 */
            tmp = vtop[0];
            vtop[0] = vtop[-2];
            vtop[-2] = tmp;
            /* stack: ML MH H1 L2 H2 L1 */
            gen_op('*');
            vrotb(3);
            vrotb(3);
            gen_op('*');
            /* stack: ML MH M1 M2 */
            gen_op('+');
            gen_op('+');
        }
        else if (op == '+' || op == '-') {
            /* XXX: �L�����[�Ȃ��̕��@���ǉ�����iMIPS �� alpha �p�j */
            if (op == '+')
                op1 = TOK_ADDC1;
            else
                op1 = TOK_SUBC1;
            gen_op(op1);
            /* stack: H1 H2 (L1 op L2) */
            vrotb(3);
            vrotb(3);
            gen_op(op1 + 1); /* TOK_xxxC2 */
        }
        else {
            gen_op(op);
            /* stack: H1 H2 (L1 op L2) */
            vrotb(3);
            vrotb(3);
            /* stack: (L1 op L2) H1 H2 */
            gen_op(op);
            /* stack: (L1 op L2) (H1 op H2) */
        }
        /* stack: L H */
        lbuild(t);
        break;
    case TOK_SAR:
    case TOK_SHR:
    case TOK_SHL:
        if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST) {
            t = vtop[-1].type.t;
            vswap();
            lexpand();
            vrotb(3);
            /* stack: L H shift */
            c = (int)vtop->c.i;
            /* �萔�̏ꍇ: ���P�� */
            /* ����: ���ׂẴR�����g�� SHL �p�B���̑��̓��[�h�����ւ��邱�Ƃŏ�������� */
            vpop();
            if (op != TOK_SHL)
                vswap();
            if (c >= 32) {
                /* stack: L H */
                vpop();
                if (c > 32) {
                    vpushi(c - 32);
                    gen_op(op);
                }
                if (op != TOK_SAR) {
                    vpushi(0);
                }
                else {
                    gv_dup();
                    vpushi(31);
                    gen_op(TOK_SAR);
                }
                vswap();
            }
            else {
                vswap();
                gv_dup();
                /* stack: H L L */
                vpushi(c);
                gen_op(op);
                vswap();
                vpushi(32 - c);
                if (op == TOK_SHL)
                    gen_op(TOK_SHR);
                else
                    gen_op(TOK_SHL);
                vrotb(3);
                /* stack: L L H */
                vpushi(c);
                if (op == TOK_SHL)
                    gen_op(TOK_SHL);
                else
                    gen_op(TOK_SHR);
                gen_op('|');
            }
            if (op != TOK_SHL)
                vswap();
            lbuild(t);
        }
        else {
            /* XXX: x86 �p�̍����ȃt�H�[���o�b�N��p�ӂ��ׂ����H */
            switch (op) {
            case TOK_SAR:
                func = TOK___ashrdi3;
                goto gen_func;
            case TOK_SHR:
                func = TOK___lshrdi3;
                goto gen_func;
            case TOK_SHL:
                func = TOK___ashldi3;
                goto gen_func;
            }
        }
        break;
    default:
        /* ��r���Z */
        t = vtop->type.t;
        vswap();
        lexpand();
        vrotb(3);
        lexpand();
        /* stack: L1 H1 L2 H2 */
        tmp = vtop[-1];
        vtop[-1] = vtop[-2];
        vtop[-2] = tmp;
        /* stack: L1 L2 H1 H2 */
        if (!cur_switch || cur_switch->bsym) {
            /* ����ňقȂ郌�W�X�^���ۑ������̂������B
               switch �� case ��r�ł͕s�v */
            save_regs(4);
        }
        /* ��ʃ��[�h���r */
        op1 = op;
        /* �l���������ꍇ�A���ʃ��[�h���r����K�v������B�W�����v�����]���邽�߁A�e�X�g�����]���� */
        if (op1 == TOK_LT)
            op1 = TOK_LE;
        else if (op1 == TOK_GT)
            op1 = TOK_GE;
        else if (op1 == TOK_ULT)
            op1 = TOK_ULE;
        else if (op1 == TOK_UGT)
            op1 = TOK_UGE;
        a = 0;
        b = 0;
        gen_op(op1);
        if (op == TOK_NE) {
            b = gvtst(0, 0);
        }
        else {
            a = gvtst(1, 0);
            if (op != TOK_EQ) {
                /* �������Ȃ��e�X�g�𐶐� */
                vpushi(0);
                vset_VT_CMP(TOK_NE);
                b = gvtst(0, 0);
            }
        }
        /* ���ʃ��[�h�̔�r�B��ɕ����Ȃ� */
        op1 = op;
        if (op1 == TOK_LT)
            op1 = TOK_ULT;
        else if (op1 == TOK_LE)
            op1 = TOK_ULE;
        else if (op1 == TOK_GT)
            op1 = TOK_UGT;
        else if (op1 == TOK_GE)
            op1 = TOK_UGE;
        gen_op(op1);
#if 0//def TCC_TARGET_I386
        if (op == TOK_NE) { gsym(b); break; }
        if (op == TOK_EQ) { gsym(a); break; }
#endif
        gvtst_set(1, a);
        gvtst_set(0, b);
        break;
    }
}
#endif

/* �l�𐳋K�� */
static uint64_t value64(uint64_t l1, int t)
{
    if ((t & VT_BTYPE) == VT_LLONG
        || (PTR_SIZE == 8 && (t & VT_BTYPE) == VT_PTR))
        return l1;
    else if (t & VT_UNSIGNED)
        return (uint32_t)l1;
    else
        return (uint32_t)l1 | -(l1 & 0x80000000);
}

static uint64_t gen_opic_sdiv(uint64_t a, uint64_t b)
{
    uint64_t x = (a >> 63 ? -a : a) / (b >> 63 ? -b : b);
    return (a ^ b) >> 63 ? -x : x;
}

static int gen_opic_lt(uint64_t a, uint64_t b)
{
    return (a ^ (uint64_t)1 << 63) < (b ^ (uint64_t)1 << 63);
}

/* �����萔�̍œK���Ɗe��@�B��ˑ��̍œK�������� */
static void gen_opic(int op)
{
    SValue* v1 = vtop - 1;
    SValue* v2 = vtop;
    int t1 = v1->type.t & VT_BTYPE;
    int t2 = v2->type.t & VT_BTYPE;
    int c1 = (v1->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;
    int c2 = (v2->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;
    uint64_t l1 = c1 ? value64(v1->c.i, v1->type.t) : 0;
    uint64_t l2 = c2 ? value64(v2->c.i, v2->type.t) : 0;
    int shm = (t1 == VT_LLONG) ? 63 : 31;
    int r;

    if (c1 && c2) {
        switch (op) {
        case '+': l1 += l2; break;
        case '-': l1 -= l2; break;
        case '&': l1 &= l2; break;
        case '^': l1 ^= l2; break;
        case '|': l1 |= l2; break;
        case '*': l1 *= l2; break;

        case TOK_PDIV:
        case '/':
        case '%':
        case TOK_UDIV:
        case TOK_UMOD:
            /* 0 �ŏ��Z����ꍇ�́A�����I�ȏ��Z�𐶐����� */
            if (l2 == 0) {
                if (CONST_WANTED && !NOEVAL_WANTED)
                    tcc_error("�萔���ł̃[���ɂ�鏜�Z");
                goto general_case;
            }
            switch (op) {
            default: l1 = gen_opic_sdiv(l1, l2); break;
            case '%': l1 = l1 - l2 * gen_opic_sdiv(l1, l2); break;
            case TOK_UDIV: l1 = l1 / l2; break;
            case TOK_UMOD: l1 = l1 % l2; break;
            }
            break;
        case TOK_SHL: l1 <<= (l2 & shm); break;
        case TOK_SHR: l1 >>= (l2 & shm); break;
        case TOK_SAR:
            l1 = (l1 >> 63) ? ~(~l1 >> (l2 & shm)) : l1 >> (l2 & shm);
            break;
            /* tests */
        case TOK_ULT: l1 = l1 < l2; break;
        case TOK_UGE: l1 = l1 >= l2; break;
        case TOK_EQ: l1 = l1 == l2; break;
        case TOK_NE: l1 = l1 != l2; break;
        case TOK_ULE: l1 = l1 <= l2; break;
        case TOK_UGT: l1 = l1 > l2; break;
        case TOK_LT: l1 = gen_opic_lt(l1, l2); break;
        case TOK_GE: l1 = !gen_opic_lt(l1, l2); break;
        case TOK_LE: l1 = !gen_opic_lt(l2, l1); break;
        case TOK_GT: l1 = gen_opic_lt(l2, l1); break;
            /* logical */
        case TOK_LAND: l1 = l1 && l2; break;
        case TOK_LOR: l1 = l1 || l2; break;
        default:
            goto general_case;
        }
        v1->c.i = value64(l1, v1->type.t);
        v1->r |= v2->r & VT_NONCONST;
        vtop--;
    }
    else {
        /* if commutative ops, put c2 as constant */
        if (c1 && (op == '+' || op == '&' || op == '^' ||
            op == '|' || op == '*' || op == TOK_EQ || op == TOK_NE)) {
            vswap();
            c2 = c1; //c = c1, c1 = c2, c2 = c;
            l2 = l1; //l = l1, l1 = l2, l2 = l;
        }
        if (c1 && ((l1 == 0 &&
            (op == TOK_SHL || op == TOK_SHR || op == TOK_SAR)) ||
            (l1 == -1 && op == TOK_SAR))) {
            /* (0 << x), (0 >> x) ����� (-1 >> x) ��萔�Ƃ��Ĉ��� */
            vpop();
        }
        else if (c2 && ((l2 == 0 && (op == '&' || op == '*')) ||
            (op == '|' &&
                (l2 == -1 || (l2 == 0xFFFFFFFF && t2 != VT_LLONG))) ||
            (l2 == 1 && (op == '%' || op == TOK_UMOD)))) {
            /* (x & 0), (x * 0), (x | -1) ����� (x % 1) ��萔�Ƃ��Ĉ��� */
            if (l2 == 1)
                vtop->c.i = 0;
            vswap();
            vtop--;
        }
        else if (c2 && (((op == '*' || op == '/' || op == TOK_UDIV ||
            op == TOK_PDIV) &&
            l2 == 1) ||
            ((op == '+' || op == '-' || op == '|' || op == '^' ||
                op == TOK_SHL || op == TOK_SHR || op == TOK_SAR) &&
                l2 == 0) ||
            (op == '&' &&
                (l2 == -1 || (l2 == 0xFFFFFFFF && t2 != VT_LLONG))))) {
            /* x*1, x-0, x&-1 �̂悤�� NOP ���Z���������� */
            vtop--;
        }
        else if (c2 && (op == '*' || op == TOK_PDIV || op == TOK_UDIV || op == TOK_UMOD)) {
            /* ��Z�⏜�Z�̑���ɃV�t�g���g���邩���� */
            if (l2 > 0 && (l2 & (l2 - 1)) == 0) {
                int n = -1;
                if (op == TOK_UMOD) {
                    vtop->c.i = l2 - 1;
                    op = '&';
                    goto general_case;
                }
                while (l2) {
                    l2 >>= 1;
                    n++;
                }
                vtop->c.i = n;
                if (op == '*')
                    op = TOK_SHL;
                else if (op == TOK_PDIV)
                    op = TOK_SAR;
                else
                    op = TOK_SHR;
            }
            goto general_case;
        }
        else if (c2 && (op == '+' || op == '-') &&
            (r = vtop[-1].r & (VT_VALMASK | VT_LVAL | VT_SYM),
                r == (VT_CONST | VT_SYM) || r == VT_LOCAL)) {
            /* �V���{�� + �萔 �̃P�[�X */
            if (op == '-')
                l2 = -l2;
            l2 += vtop[-1].c.i;
            /* �o�b�N�G���h�̓V���{���ɑ΂��� +-1<<31 �𒴂�����Z����Ɉ�����Ƃ͌���Ȃ��B���������l�͍��Ȃ� */
            if ((int)l2 != l2)
                goto general_case;
            vtop--;
            vtop->c.i = l2;
        }
        else {
        general_case:
            /* call low level op generator */
            if (t1 == VT_LLONG || t2 == VT_LLONG ||
                (PTR_SIZE == 8 && (t1 == VT_PTR || t2 == VT_PTR)))
                gen_opl(op);
            else
                gen_opi(op);
        }
        if (vtop->r == VT_CONST)
            vtop->r |= VT_NONCONST; /* is const, but only by optimization */
    }
}

#if defined TCC_TARGET_X86_64 || defined TCC_TARGET_I386
# define gen_negf gen_opf
#elif defined TCC_TARGET_ARM
void gen_negf(int op)
{
    /* arm will detect 0-x and replace by vneg */
    vpushi(0), vswap(), gen_op('-');
}
#else
/* XXX: implement in gen_opf() for other backends too */
void gen_negf(int op)
{
    /* In IEEE negate(x) isn't subtract(0,x).  Without NaNs it's
       subtract(-0, x), but with them it's really a sign flip
       operation.  We implement this with bit manipulation and have
       to do some type reinterpretation for this, which TCC can do
       only via memory.  */

    int align, size, bt;

    size = type_size(&vtop->type, &align);
    bt = vtop->type.t & VT_BTYPE;
    save_reg(gv(RC_TYPE(bt)));
    vdup();
    incr_bf_adr(size - 1);
    vdup();
    vpushi(0x80); /* flip sign */
    gen_op('^');
    vstore();
    vpop();
}
#endif

/* �萔�`���𔺂����������_���Z�𐶐����� */
static void gen_opif(int op)
{
    int c1, c2, i, bt;
    SValue* v1, * v2;
#if defined _MSC_VER && defined __x86_64__
    /* f1:-0.0, f2:0.0 �̏ꍇ�� f1 -= f2 �̈����œK��������� */
    volatile
#endif
        long double f1, f2;

    v1 = vtop - 1;
    v2 = vtop;
    if (op == TOK_NEG)
        v1 = v2;
    bt = v1->type.t & VT_BTYPE;

    /* currently, we cannot do computations with forward symbols */
    c1 = (v1->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;
    c2 = (v2->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;
    if (c1 && c2) {
        if (bt == VT_FLOAT) {
            f1 = v1->c.f;
            f2 = v2->c.f;
        }
        else if (bt == VT_DOUBLE) {
            f1 = v1->c.d;
            f2 = v2->c.d;
        }
        else {
            f1 = v1->c.ld;
            f2 = v2->c.ld;
        }
        /* ����: �萔�`���͗L���Ȑ��iNaN �△����łȂ��j�ɑ΂��Ă̂ݍs���iANSI �K�i�j */
        if (!(ieee_finite(f1) || !ieee_finite(f2)) && !CONST_WANTED)
            goto general_case;
        switch (op) {
        case '+': f1 += f2; break;
        case '-': f1 -= f2; break;
        case '*': f1 *= f2; break;
        case '/':
            if (f2 == 0.0) {
                union { float f; unsigned u; } x1, x2, y;
                /* �������q���łȂ��ꍇ�͎��s���ɕ��������_��O�𔭐�������K�v������\�������邽�߁A
                   �����łȂ���Β萔��ݍ��݂��s�� */
                if (!CONST_WANTED)
                    goto general_case;
                /* x87 ��ł� 0.0/0.0 �̎��s�����ʂ� -nan �ƂȂ�i���̃R���p�C���ł����l�j */
                x1.f = f1, x2.f = f2;
                if (f1 == 0.0)
                    y.u = 0x7fc00000; /* nan */
                else
                    y.u = 0x7f800000; /* infinity */
                y.u |= (x1.u ^ x2.u) & 0x80000000; /* set sign */
                f1 = y.f;
                break;
            }
            f1 /= f2;
            break;
        case TOK_NEG:
            f1 = -f1;
            goto unary_result;
        case TOK_EQ:
            i = f1 == f2;
        make_int:
            vtop -= 2;
            vpushi(i);
            return;
        case TOK_NE:
            i = f1 != f2;
            goto make_int;
        case TOK_LT:
            i = f1 < f2;
            goto make_int;
        case TOK_GE:
            i = f1 >= f2;
            goto make_int;
        case TOK_LE:
            i = f1 <= f2;
            goto make_int;
        case TOK_GT:
            i = f1 > f2;
            goto make_int;
        default:
            goto general_case;
        }
        vtop--;
    unary_result:
        /* XXX: overflow test ? */
        if (bt == VT_FLOAT) {
            v1->c.f = f1;
        }
        else if (bt == VT_DOUBLE) {
            v1->c.d = f1;
        }
        else {
            v1->c.ld = f1;
        }
    }
    else {
    general_case:
        if (op == TOK_NEG) {
            gen_negf(op);
        }
        else {
            gen_opf(op);
        }
    }
}

/* �^���o�͂���B'varstr' �� NULL �łȂ��ꍇ�͕ϐ������^�ɕ\������ */
/* XXX: ���p�� (union) */
/* XXX: �z�񂨂�ъ֐��|�C���^�̕\����ǉ����� */
static void type_to_str(char* buf, int buf_size,
    CType* type, const char* varstr)
{
    int bt, v, t;
    Sym* s, * sa;
    char buf1[256];
    const char* tstr;

    t = type->t;
    bt = t & VT_BTYPE;
    buf[0] = '\0';

    if (t & VT_EXTERN)
        pstrcat(buf, buf_size, "extern ");
    if (t & VT_STATIC)
        pstrcat(buf, buf_size, "static ");
    if (t & VT_TYPEDEF)
        pstrcat(buf, buf_size, "typedef ");
    if (t & VT_INLINE)
        pstrcat(buf, buf_size, "inline ");
    if (bt != VT_PTR) {
        if (t & VT_VOLATILE)
            pstrcat(buf, buf_size, "volatile ");
        if (t & VT_CONSTANT)
            pstrcat(buf, buf_size, "const ");
    }
    if (((t & VT_DEFSIGN) && bt == VT_BYTE)
        || ((t & VT_UNSIGNED)
            && (bt == VT_SHORT || bt == VT_INT || bt == VT_LLONG)
            && !IS_ENUM(t)
            ))
        pstrcat(buf, buf_size, (t & VT_UNSIGNED) ? "unsigned " : "signed ");

    buf_size -= strlen(buf);
    buf += strlen(buf);

    switch (bt) {
    case VT_VOID:
        tstr = "void";
        goto add_tstr;
    case VT_BOOL:
        tstr = "_Bool";
        goto add_tstr;
    case VT_BYTE:
        tstr = "char";
        goto add_tstr;
    case VT_SHORT:
        tstr = "short";
        goto add_tstr;
    case VT_INT:
        tstr = "int";
        goto maybe_long;
    case VT_LLONG:
        tstr = "long long";
    maybe_long:
        if (t & VT_LONG)
            tstr = "long";
        if (!IS_ENUM(t))
            goto add_tstr;
        tstr = "enum ";
        goto tstruct;
    case VT_FLOAT:
        tstr = "float";
        goto add_tstr;
    case VT_DOUBLE:
        tstr = "double";
        if (!(t & VT_LONG))
            goto add_tstr;
    case VT_LDOUBLE:
        tstr = "long double";
    add_tstr:
        pstrcat(buf, buf_size, tstr);
        break;
    case VT_STRUCT:
        tstr = "struct ";
        if (IS_UNION(t))
            tstr = "union ";
    tstruct:
        pstrcat(buf, buf_size, tstr);
        v = type->ref->v & ~SYM_STRUCT;
        if (v >= SYM_FIRST_ANOM)
            pstrcat(buf, buf_size, "<anonymous>");
        else
            pstrcat(buf, buf_size, get_tok_str(v, NULL));
        break;
    case VT_FUNC:
        s = type->ref;
        buf1[0] = 0;
        if (varstr && '*' == *varstr) {
            pstrcat(buf1, sizeof(buf1), "(");
            pstrcat(buf1, sizeof(buf1), varstr);
            pstrcat(buf1, sizeof(buf1), ")");
        }
        pstrcat(buf1, buf_size, "(");
        sa = s->next;
        while (sa != NULL) {
            char buf2[256];
            type_to_str(buf2, sizeof(buf2), &sa->type, NULL);
            pstrcat(buf1, sizeof(buf1), buf2);
            sa = sa->next;
            if (sa)
                pstrcat(buf1, sizeof(buf1), ", ");
        }
        if (s->f.func_type == FUNC_ELLIPSIS)
            pstrcat(buf1, sizeof(buf1), ", ...");
        pstrcat(buf1, sizeof(buf1), ")");
        type_to_str(buf, buf_size, &s->type, buf1);
        goto no_var;
    case VT_PTR:
        s = type->ref;
        if (t & (VT_ARRAY | VT_VLA)) {
            if (varstr && '*' == *varstr)
                snprintf(buf1, sizeof(buf1), "(%s)[%d]", varstr, s->c);
            else
                snprintf(buf1, sizeof(buf1), "%s[%d]", varstr ? varstr : "", s->c);
            type_to_str(buf, buf_size, &s->type, buf1);
            goto no_var;
        }
        pstrcpy(buf1, sizeof(buf1), "*");
        if (t & VT_CONSTANT)
            pstrcat(buf1, buf_size, "const ");
        if (t & VT_VOLATILE)
            pstrcat(buf1, buf_size, "volatile ");
        if (varstr)
            pstrcat(buf1, sizeof(buf1), varstr);
        type_to_str(buf, buf_size, &s->type, buf1);
        goto no_var;
    }
    if (varstr) {
        pstrcat(buf, buf_size, " ");
        pstrcat(buf, buf_size, varstr);
    }
no_var:;
}

static void type_incompatibility_error(CType* st, CType* dt, const char* fmt)
{
    char buf1[256], buf2[256];
    type_to_str(buf1, sizeof(buf1), st, NULL);
    type_to_str(buf2, sizeof(buf2), dt, NULL);
    tcc_error(fmt, buf1, buf2);
}

static void type_incompatibility_warning(CType* st, CType* dt, const char* fmt)
{
    char buf1[256], buf2[256];
    type_to_str(buf1, sizeof(buf1), st, NULL);
    type_to_str(buf2, sizeof(buf2), dt, NULL);
    tcc_warning(fmt, buf1, buf2);
}

static int pointed_size(CType* type)
{
    int align;
    return type_size(pointed_type(type), &align);
}

static inline int is_null_pointer(SValue* p)
{
    if ((p->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) != VT_CONST)
        return 0;
    return ((p->type.t & VT_BTYPE) == VT_INT && (uint32_t)p->c.i == 0) ||
        ((p->type.t & VT_BTYPE) == VT_LLONG && p->c.i == 0) ||
        ((p->type.t & VT_BTYPE) == VT_PTR &&
            (PTR_SIZE == 4 ? (uint32_t)p->c.i == 0 : p->c.i == 0) &&
            ((pointed_type(&p->type)->t & VT_BTYPE) == VT_VOID) &&
            0 == (pointed_type(&p->type)->t & (VT_CONSTANT | VT_VOLATILE))
            );
}

/* �֐��^���r����BOLD�i�Â��j�֐��͔C�ӂ̐V�����֐��Ƀ}�b�`���� */
static int is_compatible_func(CType* type1, CType* type2)
{
    Sym* s1, * s2;

    s1 = type1->ref;
    s2 = type2->ref;
    if (s1->f.func_call != s2->f.func_call)
        return 0;
    if (s1->f.func_const != s2->f.func_const)
        return 0;
    if (s1->f.func_type != s2->f.func_type
        && s1->f.func_type != FUNC_OLD
        && s2->f.func_type != FUNC_OLD)
        return 0;
    for (;;) {
        if (!is_compatible_unqualified_types(&s1->type, &s2->type))
            return 0;
        if (s1->f.func_type == FUNC_OLD || s2->f.func_type == FUNC_OLD)
            return 1;
        s1 = s1->next;
        s2 = s2->next;
        if (!s1)
            return !s2;
        if (!s2)
            return 0;
    }
}

/* type1 �� type2 �������Ȃ�^��Ԃ��Bunqualified ���^�Ȃ�A�^�̏C���q�͖��������B */
static int compare_types(CType* type1, CType* type2, int unqualified)
{
    int bt1, t1, t2;

    if (IS_ENUM(type1->t)) {
        if (IS_ENUM(type2->t))
            return type1->ref == type2->ref;
        type1 = &type1->ref->type;
    }
    else if (IS_ENUM(type2->t))
        type2 = &type2->ref->type;

    t1 = type1->t & VT_TYPE;
    t2 = type2->t & VT_TYPE;
    if (unqualified) {
        /* strip qualifiers before comparing */
        t1 &= ~(VT_CONSTANT | VT_VOLATILE);
        t2 &= ~(VT_CONSTANT | VT_VOLATILE);
    }

    /* Default Vs explicit signedness only matters for char */
    if ((t1 & VT_BTYPE) != VT_BYTE) {
        t1 &= ~VT_DEFSIGN;
        t2 &= ~VT_DEFSIGN;
    }
    /* XXX: bitfields ? */
    if (t1 != t2)
        return 0;

    if ((t1 & VT_ARRAY)
        && !(type1->ref->c < 0
            || type2->ref->c < 0
            || type1->ref->c == type2->ref->c))
        return 0;

    /* test more complicated cases */
    bt1 = t1 & VT_BTYPE;
    if (bt1 == VT_PTR) {
        if ((t1 & VT_MPTR) != (t2 & VT_MPTR))
            return 0;
        if (t1 & VT_MPTR) {
            Sym *c1, *c2;
            c1 = type1->ref ? type1->ref->parent_class : NULL;
            c2 = type2->ref ? type2->ref->parent_class : NULL;
            if (c1 != c2)
                return 0;
        }
        type1 = pointed_type(type1);
        type2 = pointed_type(type2);
        return is_compatible_types(type1, type2);
    }
    else if (bt1 == VT_STRUCT) {
        return (type1->ref == type2->ref);
    }
    else if (bt1 == VT_FUNC) {
        return is_compatible_func(type1, type2);
    }
    else {
        return 1;
    }
}

#define CMP_OP 'C'
#define SHIFT_OP 'S'

/* OP1 �� OP2 �����Z OP �� "����" �ł��邩���`�F�b�N����B�����^�� DEST �Ɋi�[�����i�|�C���^�̉����Z�������j�B */
    /* for shifts, 'combine' only left operand */
    /* �V�t�g�̏ꍇ�A'combine' �͍��I�y�����h�݂̂����� */
    /* strip qualifiers before comparing */
    /* ��r�O�ɏC���q����菜�� */
    /* Default Vs explicit signedness only matters for char */
    /* �f�t�H���g�Ɩ����I�ȕ����w��̈Ⴂ�� char �̏ꍇ�ɂ̂ݖ��ƂȂ� */
    /* XXX: bitfields ? */
    /* XXX: �r�b�g�t�B�[���h ? */
static int combine_types(CType* dest, SValue* op1, SValue* op2, int op)
{
    CType* type1, * type2, type;
    int t1, t2, bt1, bt2;
    int ret = 1;

    /* for shifts, 'combine' only left operand */
    if (op == SHIFT_OP)
        op2 = op1;

    type1 = &op1->type, type2 = &op2->type;
    t1 = type1->t, t2 = type2->t;
    bt1 = t1 & VT_BTYPE, bt2 = t2 & VT_BTYPE;

    type.t = VT_VOID;
    type.ref = NULL;

    if (bt1 == VT_VOID || bt2 == VT_VOID) {
        ret = op == '?' ? 1 : 0;
        /* NOTE: as an extension, we accept void on only one side */
        type.t = VT_VOID;
    }
    else if (bt1 == VT_PTR || bt2 == VT_PTR) {
        if (op == '+') {
            if (!is_integer_btype(bt1 == VT_PTR ? bt2 : bt1))
                ret = 0;
        }
        /* http://port70.net/~nsz/c/c99/n1256.html#6.5.15p6 */
        /* If one is a null ptr constant the result type is the other.  */
        else if (is_null_pointer(op2)) type = *type1;
        else if (is_null_pointer(op1)) type = *type2;
        else if (bt1 != bt2) {
            /* accept comparison or cond-expr between pointer and integer
               with a warning */
            if ((op == '?' || op == CMP_OP)
                && (is_integer_btype(bt1) || is_integer_btype(bt2)))
                tcc_warning("�|�C���^/�����̕s��v: %s",
                    op == '?' ? "�������Z�q" : "��r");
            else if (op != '-' || !is_integer_btype(bt2))
                ret = 0;
            type = *(bt1 == VT_PTR ? type1 : type2);
        }
        else {
            CType* pt1 = pointed_type(type1);
            CType* pt2 = pointed_type(type2);
            int pbt1 = pt1->t & VT_BTYPE;
            int pbt2 = pt2->t & VT_BTYPE;
            int newquals, copied = 0;
            if (pbt1 != VT_VOID && pbt2 != VT_VOID
                && !compare_types(pt1, pt2, 1/*unqualif*/)) {
                if (op != '?' && op != CMP_OP)
                    ret = 0;
                else
                    type_incompatibility_warning(type1, type2,
                        op == '?'
                        ? "pointer type mismatch in conditional expression ('%s' and '%s')"
                        : "pointer type mismatch in comparison('%s' and '%s')");
            }
            if (op == '?') {
                /* pointers to void get preferred, otherwise the
                   pointed to types minus qualifs should be compatible */
                type = *((pbt1 == VT_VOID) ? type1 : type2);
                /* combine qualifs */
                newquals = ((pt1->t | pt2->t) & (VT_CONSTANT | VT_VOLATILE));
                if ((~pointed_type(&type)->t & (VT_CONSTANT | VT_VOLATILE))
                    & newquals)
                {
                    /* copy the pointer target symbol */
                    type.ref = sym_push(SYM_FIELD, &type.ref->type,
                        0, type.ref->c);
                    copied = 1;
                    pointed_type(&type)->t |= newquals;
                }
                /* pointers to incomplete arrays get converted to
                   pointers to completed ones if possible */
                if (pt1->t & VT_ARRAY
                    && pt2->t & VT_ARRAY
                    && pointed_type(&type)->ref->c < 0
                    && (pt1->ref->c > 0 || pt2->ref->c > 0))
                {
                    if (!copied)
                        type.ref = sym_push(SYM_FIELD, &type.ref->type,
                            0, type.ref->c);
                    pointed_type(&type)->ref =
                        sym_push(SYM_FIELD, &pointed_type(&type)->ref->type,
                            0, pointed_type(&type)->ref->c);
                    pointed_type(&type)->ref->c =
                        0 < pt1->ref->c ? pt1->ref->c : pt2->ref->c;
                }
            }
        }
        if (op == CMP_OP)
            type.t = VT_SIZE_T;
    }
    else if (bt1 == VT_STRUCT || bt2 == VT_STRUCT) {
        if (op != '?' || !compare_types(type1, type2, 1))
            ret = 0;
        type = *type1;
    }
    else if (is_float(bt1) || is_float(bt2)) {
        if (bt1 == VT_LDOUBLE || bt2 == VT_LDOUBLE) {
            type.t = VT_LDOUBLE;
        }
        else if (bt1 == VT_DOUBLE || bt2 == VT_DOUBLE) {
            type.t = VT_DOUBLE;
        }
        else {
            type.t = VT_FLOAT;
        }
    }
    else if (bt1 == VT_LLONG || bt2 == VT_LLONG) {
        /* cast to biggest op */
        type.t = VT_LLONG | VT_LONG;
        if (bt1 == VT_LLONG)
            type.t &= t1;
        if (bt2 == VT_LLONG)
            type.t &= t2;
        /* convert to unsigned if it does not fit in a long long */
        if ((t1 & (VT_BTYPE | VT_UNSIGNED | VT_BITFIELD)) == (VT_LLONG | VT_UNSIGNED) ||
            (t2 & (VT_BTYPE | VT_UNSIGNED | VT_BITFIELD)) == (VT_LLONG | VT_UNSIGNED))
            type.t |= VT_UNSIGNED;
    }
    else {
        /* integer operations */
        type.t = VT_INT | (VT_LONG & (t1 | t2));
        /* convert to unsigned if it does not fit in an integer */
        if ((t1 & (VT_BTYPE | VT_UNSIGNED | VT_BITFIELD)) == (VT_INT | VT_UNSIGNED) ||
            (t2 & (VT_BTYPE | VT_UNSIGNED | VT_BITFIELD)) == (VT_INT | VT_UNSIGNED))
            type.t |= VT_UNSIGNED;
    }
    if (dest)
        *dest = type;
    return ret;
}

/* generic gen_op: handles types problems */
ST_FUNC void gen_op(int op)
{
    int t1, t2, bt1, bt2, t;
    CType type1, combtype;
    int op_class = op;

    if (op == TOK_SHR || op == TOK_SAR || op == TOK_SHL)
        op_class = SHIFT_OP;
    else if (TOK_ISCOND(op)) /* == != > ... */
        op_class = CMP_OP;

redo:
    t1 = vtop[-1].type.t;
    t2 = vtop[0].type.t;
    bt1 = t1 & VT_BTYPE;
    bt2 = t2 & VT_BTYPE;

    if (bt1 == VT_FUNC || bt2 == VT_FUNC) {
        if (bt2 == VT_FUNC) {
            mk_pointer(&vtop->type);
            gaddrof();
        }
        if (bt1 == VT_FUNC) {
            vswap();
            mk_pointer(&vtop->type);
            gaddrof();
            vswap();
        }
        goto redo;
    }
    else if (!combine_types(&combtype, vtop - 1, vtop, op_class)) {
    op_err:
        tcc_error("�񍀉��Z�̃I�y�����h�^���s���ł�");
    }
    else if (bt1 == VT_PTR || bt2 == VT_PTR) {
        /* at least one operand is a pointer */
        /* relational op: must be both pointers */
        int align;
        if (op_class == CMP_OP)
            goto std_op;
        /* if both pointers, then it must be the '-' op */
        if (bt1 == VT_PTR && bt2 == VT_PTR) {
            if (op != '-')
                goto op_err;
            vpush_type_size(pointed_type(&vtop[-1].type), &align);
            vtop->type.t &= ~VT_UNSIGNED;
            vrott(3);
            gen_opic(op);
            vtop->type.t = VT_PTRDIFF_T;
            vswap();
            gen_op(TOK_PDIV);
        }
        else {
            /* exactly one pointer : must be '+' or '-'. */
            if (op != '-' && op != '+')
                goto op_err;
            /* Put pointer as first operand */
            if (bt2 == VT_PTR) {
                vswap();
                t = t1, t1 = t2, t2 = t;
                bt2 = bt1;
            }
#if PTR_SIZE == 4
            if (bt2 == VT_LLONG)
                /* XXX: �����Ő؂�̂Ă�Bgen_opl �� ptr + long long �������Ȃ����� */
                gen_cast_s(VT_INT);
#endif
            type1 = vtop[-1].type;
            vpush_type_size(pointed_type(&vtop[-1].type), &align);
            gen_op('*');
#ifdef CONFIG_TCC_BCHECK
            if (tcc_state->do_bounds_check && !CONST_WANTED) {
                /* ���E�����t���|�C���^�̏ꍇ�A���E���e�X�g���邽�߂̓��ʂȃR�[�h�𐶐����� */
                if (op == '-') {
                    vpushi(0);
                    vswap();
                    gen_op('-');
                }
                gen_bounded_ptr_add();
            }
            else
#endif
            {
                gen_opic(op);
            }
            type1.t &= ~(VT_ARRAY | VT_VLA);
            /* put again type if gen_opic() swaped operands */
            vtop->type = type1;
        }
    }
    else {
        /* floats can only be used for a few operations */
        if (is_float(combtype.t)
            && op != '+' && op != '-' && op != '*' && op != '/'
            && op_class != CMP_OP) {
            goto op_err;
        }
    std_op:
        t = t2 = combtype.t;
        /* �V�t�g�� long long �̓���P�[�X: �V�t�g�͐����̂܂܂ɂ��� */
        if (op_class == SHIFT_OP)
            t2 = VT_INT;
        /* XXX: ����A�ꕔ�̕����Ȃ����Z�͖����I�Ȃ̂ŁA�����ŕϊ����� */
        if (t & VT_UNSIGNED) {
            if (op == TOK_SAR)
                op = TOK_SHR;
            else if (op == '/')
                op = TOK_UDIV;
            else if (op == '%')
                op = TOK_UMOD;
            else if (op == TOK_LT)
                op = TOK_ULT;
            else if (op == TOK_GT)
                op = TOK_UGT;
            else if (op == TOK_LE)
                op = TOK_ULE;
            else if (op == TOK_GE)
                op = TOK_UGE;
        }
        vswap();
        gen_cast_s(t);
        vswap();
        gen_cast_s(t2);
        if (is_float(t))
            gen_opif(op);
        else
            gen_opic(op);
        if (op_class == CMP_OP) {
            /* �֌W���Z�q: ���ʂ� int */
            vtop->type.t = VT_INT;
        }
        else {
            vtop->type.t = t;
        }
    }
    // Make sure that we have converted to an rvalue:
    if (vtop->r & VT_LVAL)
        gv(is_float(vtop->type.t & VT_BTYPE) ? RC_FLOAT : RC_INT);
}

#if defined TCC_TARGET_ARM64 || defined TCC_TARGET_RISCV64 || defined TCC_TARGET_ARM
#define gen_cvt_itof1 gen_cvt_itof
#else
/* generic itof for unsigned long long case */
static void gen_cvt_itof1(int t)
{
    if ((vtop->type.t & (VT_BTYPE | VT_UNSIGNED)) ==
        (VT_LLONG | VT_UNSIGNED)) {

        if (t == VT_FLOAT)
            vpush_helper_func(TOK___floatundisf);
#if LDOUBLE_SIZE != 8
        else if (t == VT_LDOUBLE)
            vpush_helper_func(TOK___floatundixf);
#endif
        else
            vpush_helper_func(TOK___floatundidf);
        vrott(2);
        gfunc_call(1);
        vpushi(0);
        PUT_R_RET(vtop, t);
    }
    else {
        gen_cvt_itof(t);
    }
}
#endif

#if defined TCC_TARGET_ARM64 || defined TCC_TARGET_RISCV64
#define gen_cvt_ftoi1 gen_cvt_ftoi
#else
/* generic ftoi for unsigned long long case */
static void gen_cvt_ftoi1(int t)
{
    int st;
    if (t == (VT_LLONG | VT_UNSIGNED)) {
        /* not handled natively */
        st = vtop->type.t & VT_BTYPE;
        if (st == VT_FLOAT)
            vpush_helper_func(TOK___fixunssfdi);
#if LDOUBLE_SIZE != 8
        else if (st == VT_LDOUBLE)
            vpush_helper_func(TOK___fixunsxfdi);
#endif
        else
            vpush_helper_func(TOK___fixunsdfdi);
        vrott(2);
        gfunc_call(1);
        vpushi(0);
        PUT_R_RET(vtop, t);
    }
    else {
        gen_cvt_ftoi(t);
    }
}
#endif

/* special delayed cast for char/short */
static void force_charshort_cast(void)
{
    int sbt = BFGET(vtop->r, VT_MUSTCAST) == 2 ? VT_LLONG : VT_INT;
    int dbt = vtop->type.t;
    vtop->r &= ~VT_MUSTCAST;
    vtop->type.t = sbt;
    gen_cast_s(dbt == VT_BOOL ? VT_BYTE | VT_UNSIGNED : dbt);
    vtop->type.t = dbt;
}

static void gen_cast_s(int t)
{
    CType type;
    type.t = t;
    type.ref = NULL;
    gen_cast(&type);
}

/* C++: may an lvalue bind directly to T& / const T& (param, return)? */
static int cpp_can_bind_lvalue_to_reference(CType *ref, CType *arg)
{
    CType *pt;

    pt = pointed_type(ref);
    if ((arg->t & VT_BTYPE) == VT_STRUCT)
        return is_compatible_unqualified_types(pt, arg);
    if ((pt->t & VT_BTYPE) != (arg->t & VT_BTYPE))
        return 0;
    if ((pt->t & VT_UNSIGNED) != (arg->t & VT_UNSIGNED))
        return 0;
    if ((arg->t & VT_CONSTANT) && !(pt->t & VT_CONSTANT))
        return 0;
    return 1;
}

/* cast 'vtop' to 'type'. Casting to bitfields is forbidden. */
static void gen_cast(CType* type)
{
    int sbt, dbt, sf, df, c;
    int dbt_bt, sbt_bt, ds, ss, bits, trunc;
    CType *pt;
    CType ref_pt;

    /* special delayed cast for char/short */
    if (vtop->r & VT_MUSTCAST)
        force_charshort_cast();

    /* C++: bind lvalue to reference (param / return) */
    if (tcc_state->cpp && (type->t & VT_REFERENCE) && (vtop->r & VT_LVAL)) {
        if (cpp_can_bind_lvalue_to_reference(type, &vtop->type)) {
            if (!(vtop->r & VT_LVAL)
                && (vtop->r & VT_VALMASK) == VT_LOCAL)
                vtop->r |= VT_LVAL;
            if (vtop->r & VT_LVAL)
                gaddrof();
            else
                tcc_error("rvalue cannot bind to reference");
            vtop->type = *type;
            vtop->r &= ~VT_LVAL;
            return;
        }
    }

    /* C++: assignment through reference stores the pointed-to type */
    if (tcc_state->cpp && (type->t & VT_REFERENCE) && !(vtop->r & VT_LVAL)) {
        ref_pt = *pointed_type(type);
        if (!(ref_pt.t & VT_REFERENCE)) {
            gen_cast(&ref_pt);
            return;
        }
    }

    /* bitfields first get cast to ints */
    if (vtop->type.t & VT_BITFIELD)
        gv(RC_INT);

    if (IS_ENUM(type->t) && type->ref->c < 0)
        tcc_error("�s���S�^�ւ̃L���X�g");

    dbt = type->t & (VT_BTYPE | VT_UNSIGNED);
    sbt = vtop->type.t & (VT_BTYPE | VT_UNSIGNED);
    if (sbt == VT_FUNC)
        sbt = VT_PTR;

again:
    if (sbt != dbt) {
        sf = is_float(sbt);
        df = is_float(dbt);
        dbt_bt = dbt & VT_BTYPE;
        sbt_bt = sbt & VT_BTYPE;
        if (dbt_bt == VT_VOID)
            goto done;
        if (sbt_bt == VT_VOID) {
        error:
            cast_error(&vtop->type, type);
        }

        c = (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST;
#if !defined TCC_IS_NATIVE && !defined TCC_IS_NATIVE_387
        /* don't try to convert to ldouble when cross-compiling
           (except when it's '0' which is needed for arm:gen_negf()) */
        if (dbt_bt == VT_LDOUBLE && !nocode_wanted && (sf || vtop->c.i != 0))
            c = 0;
#endif
        if (c) {
            /* constant case: we can do it now */
            /* XXX: in ISOC, cannot do it if error in convert */
            if (sbt == VT_FLOAT)
                vtop->c.ld = vtop->c.f;
            else if (sbt == VT_DOUBLE)
                vtop->c.ld = vtop->c.d;

            if (df) {
                if (sbt_bt == VT_LLONG) {
                    if ((sbt & VT_UNSIGNED) || !(vtop->c.i >> 63))
                        vtop->c.ld = vtop->c.i;
                    else
                        vtop->c.ld = -(long double)-vtop->c.i;
                }
                else if (!sf) {
                    if ((sbt & VT_UNSIGNED) || !(vtop->c.i >> 31))
                        vtop->c.ld = (uint32_t)vtop->c.i;
                    else
                        vtop->c.ld = -(long double)-(uint32_t)vtop->c.i;
                }

                if (dbt == VT_FLOAT)
                    vtop->c.f = (float)vtop->c.ld;
                else if (dbt == VT_DOUBLE)
                    vtop->c.d = (double)vtop->c.ld;
            }
            else if (sf && dbt == VT_BOOL) {
                vtop->c.i = (vtop->c.ld != 0);
            }
            else {
                if (sf) {
                    if (dbt & VT_UNSIGNED)
                        vtop->c.i = (uint64_t)vtop->c.ld;
                    else
                        vtop->c.i = (int64_t)vtop->c.ld;
                }
                else if (sbt_bt == VT_LLONG || (PTR_SIZE == 8 && sbt == VT_PTR))
                    ;
                else if (sbt & VT_UNSIGNED)
                    vtop->c.i = (uint32_t)vtop->c.i;
                else
                    vtop->c.i = ((uint32_t)vtop->c.i | -(vtop->c.i & 0x80000000));

                if (dbt_bt == VT_LLONG || (PTR_SIZE == 8 && dbt == VT_PTR))
                    ;
                else if (dbt == VT_BOOL)
                    vtop->c.i = (vtop->c.i != 0);
                else {
                    uint32_t m = dbt_bt == VT_BYTE ? 0xff :
                        dbt_bt == VT_SHORT ? 0xffff :
                        0xffffffff;
                    vtop->c.i &= m;
                    if (!(dbt & VT_UNSIGNED))
                        vtop->c.i |= -(vtop->c.i & ((m >> 1) + 1));
                }
            }
            goto done;

        }
        else if (dbt == VT_BOOL
            && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM))
            == (VT_CONST | VT_SYM)) {
            /* addresses are considered non-zero (see tcctest.c:sinit23) */
            vtop->r = VT_CONST;
            vtop->c.i = 1;
            goto done;
        }

        /* cannot generate code for global or static initializers */
        if (nocode_wanted & DATA_ONLY_WANTED)
            goto done;

        /* non constant case: generate code */
        if (dbt == VT_BOOL) {
            gen_test_zero(TOK_NE);
            goto done;
        }

        if (sf || df) {
            if (sf && df) {
                /* convert from fp to fp */
                gen_cvt_ftof(dbt);
            }
            else if (df) {
                /* convert int to fp */
                gen_cvt_itof1(dbt);
            }
            else {
                /* convert fp to int */
                sbt = dbt;
                if (dbt_bt != VT_LLONG && dbt_bt != VT_INT)
                    sbt = VT_INT;
                gen_cvt_ftoi1(sbt);
                goto again; /* may need char/short cast */
            }
            goto done;
        }

        ds = btype_size(dbt_bt);
        ss = btype_size(sbt_bt);
        if (ds == 0 || ss == 0)
            goto error;

        /* same size and no sign conversion needed */
        if (ds == ss && ds >= 4)
            goto done;
        if (dbt_bt == VT_PTR || sbt_bt == VT_PTR) {
            tcc_warning("�|�C���^�Ɛ����̃T�C�Y���قȂ�Ԃ̃L���X�g�ł�");
            if (sbt_bt == VT_PTR) {
                /* put integer type to allow logical operations below */
                vtop->type.t = (PTR_SIZE == 8 ? VT_LLONG : VT_INT);
            }
        }

        /* processor allows { int a = 0, b = *(char*)&a; }
           That means that if we cast to less width, we can just
           change the type and read it still later. */
#define ALLOW_SUBTYPE_ACCESS 1

        if (ALLOW_SUBTYPE_ACCESS && (vtop->r & VT_LVAL)) {
            /* value still in memory */
            if (ds <= ss)
                goto done;
            /* ss <= 4 here */
            if (ds <= 4 && !(dbt == (VT_SHORT | VT_UNSIGNED) && sbt == VT_BYTE)) {
                gv(RC_INT);
                goto done; /* no 64bit envolved */
            }
        }
        gv(RC_INT);

        trunc = 0;
#if PTR_SIZE == 4
        if (ds == 8) {
            /* generate high word */
            if (sbt & VT_UNSIGNED) {
                vpushi(0);
                gv(RC_INT);
            }
            else {
                gv_dup();
                vpushi(31);
                gen_op(TOK_SAR);
            }
            lbuild(dbt);
        }
        else if (ss == 8) {
            /* from long long: just take low order word */
            lexpand();
            vpop();
        }
        ss = 4;

#elif PTR_SIZE == 8
        if (ds == 8) {
            /* need to convert from 32bit to 64bit */
            if (sbt & VT_UNSIGNED) {
#if defined(TCC_TARGET_RISCV64)
                /* RISC-V keeps 32bit vals in registers sign-extended.
                   So here we need a zero-extension.  */
                trunc = 32;
#else
                goto done;
#endif
            }
            else {
                gen_cvt_sxtw();
                goto done;
            }
            ss = ds, ds = 4, dbt = sbt;
        }
        else if (ss == 8) {
            /* RISC-V keeps 32bit vals in registers sign-extended.
               So here we need a sign-extension for signed types and
               zero-extension. for unsigned types. */
#if !defined(TCC_TARGET_RISCV64)
            trunc = 32; /* zero upper 32 bits for non RISC-V targets */
#endif
        }
        else {
            ss = 4;
        }
#endif

        if (ds >= ss)
            goto done;
#if defined TCC_TARGET_I386 || defined TCC_TARGET_X86_64 || defined TCC_TARGET_ARM64
        if (ss == 4) {
            gen_cvt_csti(dbt);
            goto done;
        }
#endif
        bits = (ss - ds) * 8;
        /* for unsigned, gen_op will convert SAR to SHR */
        vtop->type.t = (ss == 8 ? VT_LLONG : VT_INT) | (dbt & VT_UNSIGNED);
        vpushi(bits);
        gen_op(TOK_SHL);
        vpushi(bits - trunc);
        gen_op(TOK_SAR);
        vpushi(trunc);
        gen_op(TOK_SHR);
    }
done:
    vtop->type = *type;
    vtop->type.t &= ~(VT_CONSTANT | VT_VOLATILE | VT_ARRAY);
}

/* return type size as known at compile time. Put alignment at 'a' */
ST_FUNC int type_size(CType* type, int* a)
{
    Sym* s;
    int bt;

    bt = type->t & VT_BTYPE;
    if (bt == VT_STRUCT) {
        /* struct/union */
        s = type->ref;
        *a = s->r;
        return s->c;
    }
    else if (bt == VT_PTR) {
        if (type->t & VT_ARRAY) {
            int ts;
            s = type->ref;
            ts = type_size(&s->type, a);
            if (ts < 0 && s->c < 0)
                ts = -ts;
            return ts * s->c;
        }
        else {
            *a = PTR_SIZE;
            return PTR_SIZE;
        }
    }
    else if (IS_ENUM(type->t) && type->ref->c < 0) {
        *a = 0;
        return -1; /* incomplete enum */
    }
    else if (bt == VT_LDOUBLE) {
        *a = LDOUBLE_ALIGN;
        return LDOUBLE_SIZE;
    }
    else if (bt == VT_DOUBLE || bt == VT_LLONG) {
#if (defined TCC_TARGET_I386 && !defined TCC_TARGET_PE) \
 || (defined TCC_TARGET_ARM && !defined TCC_ARM_EABI)
        * a = 4;
#else
        * a = 8;
#endif
        return 8;
    }
    else if (bt == VT_INT || bt == VT_FLOAT) {
        *a = 4;
        return 4;
    }
    else if (bt == VT_SHORT) {
        *a = 2;
        return 2;
    }
    else if (bt == VT_QLONG || bt == VT_QFLOAT) {
        *a = 8;
        return 16;
    }
    else {
        /* char, void, function, _Bool */
        *a = 1;
        return 1;
    }
}

/* push type size as known at runtime time on top of value stack. Put
   alignment at 'a' */
static void vpush_type_size(CType* type, int* a)
{
    if (type->t & VT_VLA) {
        type_size(&type->ref->type, a);
        vset(&int_type, VT_LOCAL | VT_LVAL, type->ref->c);
    }
    else {
        int size = type_size(type, a);
        if (size < 0)
            tcc_error("�s���Ȍ^�T�C�Y�ł�");
        vpushs(size);
    }
}

/* return the pointed type of t */
static inline CType* pointed_type(CType* type)
{
    return &type->ref->type;
}

/* modify type so that its it is a pointer to type. */
ST_FUNC void mk_pointer(CType* type)
{
    Sym* s;
    s = sym_push(SYM_FIELD, type, 0, -1);
    type->t = VT_PTR | (type->t & VT_STORAGE);
    type->ref = s;
}

/* return true if type1 and type2 are exactly the same (including
   qualifiers).
*/
static int is_compatible_types(CType* type1, CType* type2)
{
    return compare_types(type1, type2, 0);
}

/* return true if type1 and type2 are the same (ignoring qualifiers).
*/
static int is_compatible_unqualified_types(CType* type1, CType* type2)
{
    return compare_types(type1, type2, 1);
}

static void cast_error(CType* st, CType* dt)
{
    type_incompatibility_error(st, dt, "'%s' �� '%s' �ɕϊ��ł��܂���");
}

/* verify type compatibility to store vtop in 'dt' type */
static void verify_assign_cast(CType* dt)
{
    CType* st, * type1, * type2;
    int dbt, sbt, qualwarn, lvl;

    st = &vtop->type; /* source type */
    dbt = dt->t & VT_BTYPE;
    sbt = st->t & VT_BTYPE;
    if (dt->t & VT_CONSTANT) {
        /* FEAT-6B-P2: hard error in C++ mode.  const T& destinations are
           exempt: argument binding passes the param type (which keeps
           top-level VT_CONSTANT, see convert_parameter_type) through
           gen_assign_cast on every call. */
        if (tcc_state->cpp && !(dt->t & VT_REFERENCE))
            tcc_error("assignment of read-only location");
        tcc_warning("�ǂݎ���p�̏ꏊ�ւ̑���ł�");
    }
    switch (dbt) {
    case VT_VOID:
        if (sbt != dbt)
            tcc_error("void �^�ւ̑��");
        break;
    case VT_PTR:
        /* special cases for pointers */
        /* '0' can also be a pointer */
        if (is_null_pointer(vtop))
            break;
        type1 = pointed_type(dt);
        if ((dt->t & VT_REFERENCE) && sbt != VT_PTR && sbt != VT_FUNC
            && cpp_can_bind_lvalue_to_reference(dt, st))
            break;
        /* accept implicit pointer to integer cast with warning */
        if (is_integer_btype(sbt)) {
            tcc_warning("�L���X�g�Ȃ��Ő�������|�C���^������܂�");
            break;
        }
        if (sbt == VT_PTR)
            type2 = pointed_type(st);
        else if (sbt == VT_FUNC)
            type2 = st; /* a function is implicitly a function pointer */
        else
            goto error;
        /* FEAT-5B: propagate PMF field token before compatible-types break */
        if ((dt->t & VT_MPTR) && (st->t & VT_MPTR)
            && cpp_is_mptr_to_func(dt) && st->ref && st->ref->c
            && dt->ref && !dt->ref->c)
            dt->ref->c = st->ref->c;
        if (is_compatible_types(type1, type2))
            break;
        for (qualwarn = lvl = 0;; ++lvl) {
            if (((type2->t & VT_CONSTANT) && !(type1->t & VT_CONSTANT)) ||
                ((type2->t & VT_VOLATILE) && !(type1->t & VT_VOLATILE)))
                qualwarn = 1;
            dbt = type1->t & (VT_BTYPE | VT_LONG);
            sbt = type2->t & (VT_BTYPE | VT_LONG);
            if (dbt != VT_PTR || sbt != VT_PTR)
                break;
            type1 = pointed_type(type1);
            type2 = pointed_type(type2);
        }
        if (!is_compatible_unqualified_types(type1, type2)) {
            if ((dbt == VT_VOID || sbt == VT_VOID) && lvl == 0) {
                /* void * can match anything */
            }
            else if (dbt == sbt
                && is_integer_btype(sbt & VT_BTYPE)
                && IS_ENUM(type1->t) + IS_ENUM(type2->t)
                + !!((type1->t ^ type2->t) & VT_UNSIGNED) < 2) {
                /* Like GCC don't warn by default for merely changes
                   in pointer target signedness.  Do warn for different
                   base types, though, in particular for unsigned enums
                   and signed int targets.  */
            }
            else {
                tcc_warning("�݊����̂Ȃ��|�C���^�^����̑���ł�");
                break;
            }
        }
        if (qualwarn)
            tcc_warning_c(warn_discarded_qualifiers)("assignment discards qualifiers from pointer target type");
        break;
    case VT_BYTE:
    case VT_SHORT:
    case VT_INT:
    case VT_LLONG:
        if (sbt == VT_PTR || sbt == VT_FUNC) {
            tcc_warning("�L���X�g�Ȃ��Ń|�C���^���琮��������܂�");
        }
        else if (sbt == VT_STRUCT) {
            goto case_VT_STRUCT;
        }
        /* XXX: more tests */
        break;
    case VT_STRUCT:
    case_VT_STRUCT:
        if (!is_compatible_unqualified_types(dt, st)) {
        error:
            cast_error(st, dt);
        }
        break;
    }
}

static void gen_assign_cast(CType* dt)
{
    verify_assign_cast(dt);
    gen_cast(dt);
}

/* store vtop in lvalue pushed on stack */
ST_FUNC void vstore(void)
{
    int sbt, dbt, ft, r, size, align, bit_size, bit_pos, delayed_cast;

    ft = vtop[-1].type.t;
    sbt = vtop->type.t & VT_BTYPE;
    dbt = ft & VT_BTYPE;
    verify_assign_cast(&vtop[-1].type);
    if (vtop[-1].sym && (vtop[-1].type.t & VT_MPTR)
        && vtop[-1].type.ref && vtop->type.ref && vtop->type.ref->c
        && vtop[-1].type.ref->c != vtop->type.ref->c) {
        vtop[-1].type.ref->c = vtop->type.ref->c;
        vtop[-1].sym->type.ref->c = vtop->type.ref->c;
    }

    if (sbt == VT_STRUCT) {
        /* if structure, only generate pointer */
        /* structure assignment : generate memcpy */
        size = type_size(&vtop->type, &align);
        /* destination, keep on stack() as result */
        vpushv(vtop - 1);
#ifdef CONFIG_TCC_BCHECK
        if (vtop->r & VT_MUSTBOUND)
            gbound(); /* check would be wrong after gaddrof() */
#endif
        vtop->type.t = VT_PTR;
        gaddrof();
        /* source */
        vswap();
#ifdef CONFIG_TCC_BCHECK
        if (vtop->r & VT_MUSTBOUND)
            gbound();
#endif
        vtop->type.t = VT_PTR;
        gaddrof();

#ifdef TCC_TARGET_NATIVE_STRUCT_COPY
        if (1
#ifdef CONFIG_TCC_BCHECK
            && !tcc_state->do_bounds_check
#endif
            ) {
            gen_struct_copy(size);
        }
        else
#endif
        {
            /* type size */
            vpushi(size);
            /* Use memmove, rather than memcpy, as dest and src may be same: */
#ifdef TCC_ARM_EABI
            if (!(align & 7))
                vpush_helper_func(TOK_memmove8);
            else if (!(align & 3))
                vpush_helper_func(TOK_memmove4);
            else
#endif
                vpush_helper_func(TOK_memmove);
            vrott(4);
            gfunc_call(3);
        }

    }
    else if (ft & VT_BITFIELD) {
        /* bitfield store handling */

        /* save lvalue as expression result (example: s.b = s.a = n;) */
        vdup(), vtop[-1] = vtop[-2];

        bit_pos = BIT_POS(ft);
        bit_size = BIT_SIZE(ft);
        /* remove bit field info to avoid loops */
        vtop[-1].type.t = ft & ~VT_STRUCT_MASK;

        if (dbt == VT_BOOL) {
            gen_cast(&vtop[-1].type);
            vtop[-1].type.t = (vtop[-1].type.t & ~VT_BTYPE) | (VT_BYTE | VT_UNSIGNED);
        }
        r = adjust_bf(vtop - 1, bit_pos, bit_size);
        if (dbt != VT_BOOL) {
            gen_cast(&vtop[-1].type);
            dbt = vtop[-1].type.t & VT_BTYPE;
        }
        if (r == VT_STRUCT) {
            store_packed_bf(bit_pos, bit_size);
        }
        else {
            unsigned long long mask = (1ULL << bit_size) - 1;
            if (dbt != VT_BOOL) {
                /* mask source */
                if (dbt == VT_LLONG)
                    vpushll(mask);
                else
                    vpushi((unsigned)mask);
                gen_op('&');
            }
            /* shift source */
            vpushi(bit_pos);
            gen_op(TOK_SHL);
            vswap();
            /* duplicate destination */
            vdup();
            vrott(3);
            /* load destination, mask and or with source */
            if (dbt == VT_LLONG)
                vpushll(~(mask << bit_pos));
            else
                vpushi(~((unsigned)mask << bit_pos));
            gen_op('&');
            gen_op('|');
            /* store result */
            vstore();
            /* ... and discard */
            vpop();
        }
    }
    else if (dbt == VT_VOID) {
        --vtop;
    }
    else {
        /* optimize char/short casts */
        delayed_cast = 0;
        if ((dbt == VT_BYTE || dbt == VT_SHORT)
            && is_integer_btype(sbt)
            ) {
            if ((vtop->r & VT_MUSTCAST)
                && btype_size(dbt) > btype_size(sbt)
                )
                force_charshort_cast();
            delayed_cast = 1;
        }
        else {
            if (tcc_state->cpp && (vtop[-1].type.t & VT_REFERENCE)) {
                CType pt;

                pt = *pointed_type(&vtop[-1].type);
                gen_cast(&pt);
                dbt = pt.t & VT_BTYPE;
            }
            else {
                gen_cast(&vtop[-1].type);
            }
        }

#ifdef CONFIG_TCC_BCHECK
        /* bound check case */
        if (vtop[-1].r & VT_MUSTBOUND) {
            vswap();
            gbound();
            vswap();
        }
#endif
        gv(RC_TYPE(dbt)); /* generate value */

        if (delayed_cast) {
            vtop->r |= BFVAL(VT_MUSTCAST, (sbt == VT_LLONG) + 1);
            //tcc_warning("deley cast %x -> %x", sbt, dbt);
            vtop->type.t = ft & VT_TYPE;
        }

        /* if lvalue was saved on stack, must read it */
        if ((vtop[-1].r & VT_VALMASK) == VT_LLOCAL) {
            SValue sv;
            r = get_reg(RC_INT);
            sv.type.t = VT_PTRDIFF_T;
            sv.r = VT_LOCAL | VT_LVAL;
            sv.c.i = vtop[-1].c.i;
            load(r, &sv);
            vtop[-1].r = r | VT_LVAL;
        }

        r = vtop->r & VT_VALMASK;
        /* two word case handling :
           store second register at word + 4 (or +8 for x86-64)  */
        if (USING_TWO_WORDS(dbt)) {
            int load_type = (dbt == VT_QFLOAT) ? VT_DOUBLE : VT_PTRDIFF_T;
            vtop[-1].type.t = load_type;
            store(r, vtop - 1);
            vswap();
            incr_offset(PTR_SIZE);
            vswap();
            /* XXX: it works because r2 is spilled last ! */
            store(vtop->r2, vtop - 1);
        }
        else {
            /* single word */
            store(r, vtop - 1);
        }
        vswap();
        vtop--; /* NOT vpop() because on x86 it would flush the fp stack */
    }
}

/* post defines POST/PRE add. c is the token ++ or -- */
ST_FUNC void inc(int post, int c)
{
    test_lvalue();
    vdup(); /* save lvalue */
    if (post) {
        gv_dup(); /* duplicate value */
        vrotb(3);
        vrotb(3);
    }
    /* add constant */
    vpushi(c - TOK_MID);
    gen_op('+');
    vstore(); /* store value */
    if (post)
        vpop(); /* if post op, return saved value */
}

ST_FUNC CString* parse_mult_str(const char* msg)
{
    /* read the string */
    if (tok != TOK_STR)
        expect(msg);
    cstr_reset(&initstr);
    while (tok == TOK_STR) {
        /* XXX: add \0 handling too ? */
        cstr_cat(&initstr, tokc.str.data, -1);
        next();
    }
    cstr_ccat(&initstr, '\0');
    return &initstr;
}

/* If I is >= 1 and a power of two, returns log2(i)+1.
   If I is 0 returns 0.  */
ST_FUNC int exact_log2p1(int i)
{
    int ret;
    if (!i)
        return 0;
    for (ret = 1; i >= 1 << 8; ret += 8)
        i >>= 8;
    if (i >= 1 << 4)
        ret += 4, i >>= 4;
    if (i >= 1 << 2)
        ret += 2, i >>= 2;
    if (i >= 1 << 1)
        ret++;
    return ret;
}

/* Parse __attribute__((...)) GNUC extension. */
static void parse_attribute(AttributeDef* ad)
{
    int t, n;
    char* astr;

redo:
    if (tok != TOK_ATTRIBUTE1 && tok != TOK_ATTRIBUTE2)
        return;
    next();
    skip('(');
    skip('(');
    while (tok != ')') {
        if (tok < TOK_IDENT)
            expect("attribute name");
        t = tok;
        next();
        switch (t) {
        case TOK_CLEANUP1:
        case TOK_CLEANUP2:
        {
            Sym* s;

            skip('(');
            s = sym_find(tok);
            if (!s) {
                tcc_warning_c(warn_implicit_function_declaration)(
                    "implicit declaration of function '%s'", get_tok_str(tok, &tokc));
                s = external_global_sym(tok, &func_old_type);
            }
            else if ((s->type.t & VT_BTYPE) != VT_FUNC)
                tcc_error("'%s' �͊֐��Ƃ��Đ錾����Ă��܂���", get_tok_str(tok, &tokc));
            ad->cleanup_func = s;
            next();
            skip(')');
            break;
        }
        case TOK_CONSTRUCTOR1:
        case TOK_CONSTRUCTOR2:
            ad->f.func_ctor = 1;
            break;
        case TOK_DESTRUCTOR1:
        case TOK_DESTRUCTOR2:
            ad->f.func_dtor = 1;
            break;
        case TOK_ALWAYS_INLINE1:
        case TOK_ALWAYS_INLINE2:
            ad->f.func_alwinl = 1;
            break;
        case TOK_SECTION1:
        case TOK_SECTION2:
            skip('(');
            astr = parse_mult_str("section name")->data;
            ad->section = find_section(tcc_state, astr);
            skip(')');
            break;
        case TOK_ALIAS1:
        case TOK_ALIAS2:
            skip('(');
            astr = parse_mult_str("alias(\"target\")")->data;
            /* save string as token, for later */
            ad->alias_target = tok_alloc_const(astr);
            skip(')');
            break;
        case TOK_VISIBILITY1:
        case TOK_VISIBILITY2:
            skip('(');
            astr = parse_mult_str("visibility(\"default|hidden|internal|protected\")")->data;
            if (!strcmp(astr, "default"))
                ad->a.visibility = STV_DEFAULT;
            else if (!strcmp(astr, "hidden"))
                ad->a.visibility = STV_HIDDEN;
            else if (!strcmp(astr, "internal"))
                ad->a.visibility = STV_INTERNAL;
            else if (!strcmp(astr, "protected"))
                ad->a.visibility = STV_PROTECTED;
            else
                expect("visibility(\"default|hidden|internal|protected\")");
            skip(')');
            break;
        case TOK_ALIGNED1:
        case TOK_ALIGNED2:
            if (tok == '(') {
                next();
                n = expr_const();
                if (n <= 0 || (n & (n - 1)) != 0)
                    tcc_error("�A���C�������g�͐���2�̙p�łȂ���΂Ȃ�܂���");
                skip(')');
            }
            else {
                n = MAX_ALIGN;
            }
            ad->a.aligned = exact_log2p1(n);
            if (n != 1 << (ad->a.aligned - 1))
                tcc_error("�A���C�������g %d �͎������傫�����܂�", n);
            break;
        case TOK_PACKED1:
        case TOK_PACKED2:
            ad->a.packed = 1;
            break;
        case TOK_WEAK1:
        case TOK_WEAK2:
            ad->a.weak = 1;
            break;
        case TOK_NODEBUG1:
        case TOK_NODEBUG2:
            ad->a.nodebug = 1;
            break;
        case TOK_UNUSED1:
        case TOK_UNUSED2:
            /* currently, no need to handle it because tcc does not
               track unused objects */
            break;
        case TOK_NORETURN1:
        case TOK_NORETURN2:
            ad->f.func_noreturn = 1;
            break;
        case TOK_CDECL1:
        case TOK_CDECL2:
        case TOK_CDECL3:
            ad->f.func_call = FUNC_CDECL;
            break;
        case TOK_STDCALL1:
        case TOK_STDCALL2:
        case TOK_STDCALL3:
            ad->f.func_call = FUNC_STDCALL;
            break;
#ifdef TCC_TARGET_I386
        case TOK_REGPARM1:
        case TOK_REGPARM2:
            skip('(');
            n = expr_const();
            if (n > 3)
                n = 3;
            else if (n < 0)
                n = 0;
            if (n > 0)
                ad->f.func_call = FUNC_FASTCALL1 + n - 1;
            skip(')');
            break;
        case TOK_FASTCALL1:
        case TOK_FASTCALL2:
        case TOK_FASTCALL3:
            ad->f.func_call = FUNC_FASTCALLW;
            break;
        case TOK_THISCALL1:
        case TOK_THISCALL2:
        case TOK_THISCALL3:
            ad->f.func_call = FUNC_THISCALL;
            break;
#endif
        case TOK_MODE:
            skip('(');
            switch (tok) {
            case TOK_MODE_DI:
                ad->attr_mode = VT_LLONG + 1;
                break;
            case TOK_MODE_QI:
                ad->attr_mode = VT_BYTE + 1;
                break;
            case TOK_MODE_HI:
                ad->attr_mode = VT_SHORT + 1;
                break;
            case TOK_MODE_SI:
            case TOK_MODE_word:
                ad->attr_mode = VT_INT + 1;
                break;
            default:
                tcc_warning("__mode__(%s) �̓T�|�[�g����Ă��܂���\n", get_tok_str(tok, NULL));
                break;
            }
            next();
            skip(')');
            break;
        case TOK_DLLEXPORT:
            ad->a.dllexport = 1;
            break;
        case TOK_NODECORATE:
            ad->a.nodecorate = 1;
            break;
        case TOK_DLLIMPORT:
            ad->a.dllimport = 1;
            break;
        default:
            tcc_warning_c(warn_unsupported)("���� '%s' �͖�������܂�", get_tok_str(t, NULL));
            /* skip parameters */
            if (tok == '(') {
                int parenthesis = 0;
                do {
                    if (tok == '(')
                        parenthesis++;
                    else if (tok == ')')
                        parenthesis--;
                    next();
                } while (parenthesis && tok != -1);
            }
            break;
        }
        if (tok != ',')
            break;
        next();
    }
    skip(')');
    skip(')');
    goto redo;
}

static int cpp_field_is_const(Sym *field)
{
    return field && field->type.ref && field->type.ref->f.func_const;
}

static Sym *cpp_pick_func_field(Sym *const_match, Sym *nonconst_match,
                                int obj_const, int *cumofs)
{
    if (obj_const) {
        if (const_match) {
            *cumofs = const_match->c;
            return const_match;
        }
        tcc_error("const object requires const member function");
    }
    if (nonconst_match) {
        *cumofs = nonconst_match->c;
        return nonconst_match;
    }
    if (const_match) {
        *cumofs = const_match->c;
        return const_match;
    }
    return NULL;
}

/* C++: number of declared parameters of a member function field
   (hidden `this` is not part of the field's param list). */
static int cpp_func_param_count(Sym *field)
{
    Sym *sa;
    int n;

    n = 0;
    for (sa = field->type.ref ? field->type.ref->next : NULL; sa; sa = sa->next)
        n++;
    return n;
}

/* C++: find operator member without error (for implicit binop/subscript/unop).
   want_args selects by declared arity (e.g. unary vs binary operator-);
   pass -1 to accept any.  Falls back to any-arity when nothing matches. */
static Sym *cpp_find_operator_member(CType *type, int v, int *cumofs, int want_args)
{
    Sym *s, *class_sym;
    Sym *const_match, *nonconst_match, *ret;
    Sym *any_const_match, *any_nonconst_match;
    int v1, obj_const;

    if ((type->t & VT_BTYPE) != VT_STRUCT || !type->ref)
        return NULL;
    class_sym = type->ref;
    if (class_sym->c < 0)
        return NULL;
    v1 = v | SYM_FIELD;
    obj_const = (type->t & VT_CONSTANT) ? 1 : 0;
    const_match = NULL;
    nonconst_match = NULL;
    any_const_match = NULL;
    any_nonconst_match = NULL;
    s = class_sym;
    while ((s = s->next) != NULL) {
        if (s->v == v1) {
            if ((s->type.t & VT_BTYPE) == VT_FUNC) {
                int arity_ok = (want_args < 0
                                || cpp_func_param_count(s) == want_args);
                if (cpp_field_is_const(s)) {
                    if (arity_ok && !const_match)
                        const_match = s;
                    if (!any_const_match)
                        any_const_match = s;
                } else {
                    if (arity_ok && !nonconst_match)
                        nonconst_match = s;
                    if (!any_nonconst_match)
                        any_nonconst_match = s;
                }
            } else {
                *cumofs = s->c;
                return s;
            }
            continue;
        }
        if ((s->type.t & VT_BTYPE) == VT_STRUCT
            && s->v >= (SYM_FIRST_ANOM | SYM_FIELD)) {
            ret = cpp_find_operator_member(&s->type, v1, cumofs, want_args);
            if (ret) {
                if ((ret->type.t & VT_BTYPE) != VT_FUNC)
                    *cumofs += s->c;
                return ret;
            }
        }
    }
    ret = cpp_pick_func_field(const_match, nonconst_match, obj_const, cumofs);
    if (ret)
        return ret;
    /* no exact-arity candidate: keep pre-arity behavior (default args etc.) */
    ret = cpp_pick_func_field(any_const_match, any_nonconst_match, obj_const, cumofs);
    if (ret)
        return ret;
    return NULL;
}

static int cpp_has_free_func(int v)
{
    Sym *s;

    if (!tcc_state->cpp)
        return 0;
    for (s = sym_find(v); s; s = s->prev_tok) {
        if ((s->type.t & VT_BTYPE) == VT_FUNC && !s->parent_class)
            return 1;
    }
    return 0;
}

/* C++: find class member for call; resolves get() vs get() const overloads. */
static Sym *cpp_find_field_for_call(CType *type, int v, int *cumofs)
{
    Sym *s, *class_sym;
    Sym *const_match, *nonconst_match, *ret;
    int v1, obj_const;

    class_sym = type->ref;
    v1 = v | SYM_FIELD;
    if (!(v & SYM_FIELD)) {
        if ((type->t & VT_BTYPE) != VT_STRUCT)
            expect("struct or union");
        if (v < TOK_UIDENT)
            expect("field name");
        if (class_sym->c < 0)
            tcc_error("incomplete type '%s'",
                get_tok_str(class_sym->v & ~SYM_STRUCT, 0));
    }
    obj_const = (type->t & VT_CONSTANT) ? 1 : 0;
    const_match = NULL;
    nonconst_match = NULL;
    s = class_sym;
    while ((s = s->next) != NULL) {
        if (s->v == v1) {
            if ((s->type.t & VT_BTYPE) == VT_FUNC) {
                if (cpp_field_is_const(s)) {
                    if (!const_match)
                        const_match = s;
                } else {
                    if (!nonconst_match)
                        nonconst_match = s;
                }
            } else {
                *cumofs = s->c;
                return s;
            }
            continue;
        }
        if ((s->type.t & VT_BTYPE) == VT_STRUCT
            && s->v >= (SYM_FIRST_ANOM | SYM_FIELD)) {
            ret = cpp_find_field_for_call(&s->type, v1, cumofs);
            if (ret) {
                /* cumofs is unused for VT_FUNC calls; only adjust for data members */
                if ((ret->type.t & VT_BTYPE) != VT_FUNC)
                    *cumofs += s->c;
                return ret;
            }
        }
    }
    ret = cpp_pick_func_field(const_match, nonconst_match, obj_const, cumofs);
    if (ret)
        return ret;
    if (!(v & SYM_FIELD))
        tcc_error("field not found: %s", get_tok_str(v, NULL));
    return NULL;
}

static Sym* find_field(CType* type, int v, int* cumofs)
{
    Sym* s = type->ref;
    int v1 = v | SYM_FIELD;
    if (!(v & SYM_FIELD)) { /* top-level call */
        if ((type->t & VT_BTYPE) != VT_STRUCT)
            expect("struct or union");
        if (v < TOK_UIDENT)
            expect("field name");
        if (s->c < 0)
            tcc_error("�s���S�^ '%s' �̎Q�Ɖ���",
                get_tok_str(s->v & ~SYM_STRUCT, 0));
    }
    while ((s = s->next) != NULL) {
        if (s->v == v1) {
            *cumofs = s->c;
            return s;
        }
        if ((s->type.t & VT_BTYPE) == VT_STRUCT
            && s->v >= (SYM_FIRST_ANOM | SYM_FIELD)) {
            /* try to find field in anonymous sub-struct/union */
            Sym* ret = find_field(&s->type, v1, cumofs);
            if (ret) {
                *cumofs += s->c;
                return ret;
            }
        }
    }
    if (!(v & SYM_FIELD))
        tcc_error("�t�B�[���h��������܂���: %s", get_tok_str(v, NULL));
    return s;
}

/* Prepare non-virtual member call: obj lvalue on vtop -> func on vtop. */
static Sym *cpp_prepare_member_func_call(Sym *field)
{
    CType obj_type;
    Sym *fsym;

    obj_type = vtop->type;
    test_lvalue();
    gaddrof();
    cpp_member_this = *vtop;
    vpop();
    fsym = cpp_lookup_member_func(field, &obj_type);
    vset(&fsym->type, fsym->r | VT_SYM, 0);
    vtop->sym = fsym;
    vtop->r &= ~VT_LVAL;
    return fsym->type.ref;
}

/* Complete member call: func on vtop, user args in array. */
static void cpp_finish_member_call(Sym *s, SValue *user_args, int nb_user_args)
{
    SValue ret;
    Sym *sa;
    int nb_args, na, ret_nregs, ret_align, regsize, variadic, n, r, i;
    int size, align, t;

    nb_args = 0;
    ret.r2 = VT_CONST;
    if ((s->type.t & VT_BTYPE) == VT_STRUCT) {
        variadic = (s->f.func_type == FUNC_ELLIPSIS);
        ret_nregs = gfunc_sret(&s->type, variadic, &ret.type,
            &ret_align, &regsize);
        if (ret_nregs <= 0) {
            size = type_size(&s->type, &align);
            loc = (loc - size) & -align;
            ret.type = s->type;
            ret.r = VT_LOCAL | VT_LVAL;
            vseti(VT_LOCAL, loc);
#ifdef CONFIG_TCC_BCHECK
            if (tcc_state->do_bounds_check)
                --loc;
#endif
            ret.c = vtop->c;
            if (ret_nregs < 0)
                vpop();
            else
                nb_args++;
        }
    } else {
        ret_nregs = 1;
        ret.type = s->type;
    }

    if (ret_nregs > 0) {
        ret.c.i = 0;
        PUT_R_RET(&ret, ret.type.t);
    }

    sa = s->next;
    for (i = 0; i < nb_user_args; i++) {
        vpushv(&user_args[i]);
        gfunc_param_typed(s, sa);
        if (sa)
            sa = sa->next;
    }
    nb_args += nb_user_args;

    vcheck_cmp();
    na = nb_args;
    vtop++;
    nb_args++;
    {
        /* BUG-12: `this` is normally inserted as arg0/RCX (matches
           gen_function's this_param, the callee's first *type-list*
           parameter).  But when the struct return doesn't fit in
           registers (ret_nregs==0, sret pointer pushed above as the
           first arg), Win64 reserves arg0/RCX for that hidden pointer
           and the type-list params - `this` included - start at
           arg1/RDX (gfunc_prolog inserts sret ahead of the list).
           Inserting `this` at position 0 here pushed sret to arg1
           instead, swapping this<->sret for any >8-byte struct return
           (feat6a_big_struct, 問題と原因.md 12d/BUG-12). */
        int has_sret = (s->type.t & VT_BTYPE) == VT_STRUCT && ret_nregs == 0;
        int insert_at = has_sret ? 1 : 0;
        int nmove = na - insert_at;
        memmove(vtop - nb_args + 2 + insert_at, vtop - nb_args + 1 + insert_at,
            nmove * sizeof(SValue));
        vtop[-nb_args + 1 + insert_at] = cpp_member_this;
    }
    gfunc_call(nb_args);

    if (ret_nregs < 0) {
        vsetc(&ret.type, ret.r, &ret.c);
#ifdef TCC_TARGET_RISCV64
        arch_transfer_ret_regs(1);
#endif
    } else {
        n = ret_nregs;
        while (n > 1) {
            int rc = reg_classes[ret.r] & ~(RC_INT | RC_FLOAT);
            rc <<= --n;
            for (r = 0; r < NB_REGS; ++r)
                if (reg_classes[r] & rc)
                    break;
            vsetc(&ret.type, r, &ret.c);
        }
        vsetc(&ret.type, ret.r, &ret.c);
        vtop->r2 = ret.r2;

        if (((s->type.t & VT_BTYPE) == VT_STRUCT) && ret_nregs) {
            int addr, offset;

            size = type_size(&s->type, &align);
            size = (size + regsize - 1) & -regsize;
            if (ret_align > align)
                align = ret_align;
            loc = (loc - size) & -align;
            addr = loc;
            offset = 0;
            for (;;) {
                vset(&ret.type, VT_LOCAL | VT_LVAL, addr + offset);
                vswap();
                vstore();
                vtop--;
                if (--ret_nregs == 0)
                    break;
                offset += regsize;
            }
            vset(&s->type, VT_LOCAL | VT_LVAL, addr);
        }

        t = s->type.t & VT_BTYPE;
        if (t == VT_BYTE || t == VT_SHORT || t == VT_BOOL) {
#ifdef PROMOTE_RET
            vtop->r |= BFVAL(VT_MUSTCAST, 1);
#else
            vtop->type.t = VT_INT;
#endif
        }
        if (s->type.t & VT_REFERENCE)
            indir();
    }
}

/* Complete free (non-member) call: func on vtop, user args in array. */
static void cpp_finish_free_call(Sym *s, SValue *user_args, int nb_user_args)
{
    SValue ret;
    Sym *sa;
    int nb_args, ret_nregs, ret_align, regsize, variadic, n, r, i;
    int size, align, t;

    nb_args = 0;
    ret.r2 = VT_CONST;
    if ((s->type.t & VT_BTYPE) == VT_STRUCT) {
        variadic = (s->f.func_type == FUNC_ELLIPSIS);
        ret_nregs = gfunc_sret(&s->type, variadic, &ret.type,
            &ret_align, &regsize);
        if (ret_nregs <= 0) {
            size = type_size(&s->type, &align);
            loc = (loc - size) & -align;
            ret.type = s->type;
            ret.r = VT_LOCAL | VT_LVAL;
            vseti(VT_LOCAL, loc);
#ifdef CONFIG_TCC_BCHECK
            if (tcc_state->do_bounds_check)
                --loc;
#endif
            ret.c = vtop->c;
            if (ret_nregs < 0)
                vpop();
            else
                nb_args++;
        }
    } else {
        ret_nregs = 1;
        ret.type = s->type;
    }

    if (ret_nregs > 0) {
        ret.c.i = 0;
        PUT_R_RET(&ret, ret.type.t);
    }

    sa = s->next;
    for (i = 0; i < nb_user_args; i++) {
        vpushv(&user_args[i]);
        gfunc_param_typed(s, sa);
        if (sa)
            sa = sa->next;
    }
    nb_args += nb_user_args;

    vcheck_cmp();
    gfunc_call(nb_args);

    if (ret_nregs < 0) {
        vsetc(&ret.type, ret.r, &ret.c);
#ifdef TCC_TARGET_RISCV64
        arch_transfer_ret_regs(1);
#endif
    } else {
        n = ret_nregs;
        while (n > 1) {
            int rc = reg_classes[ret.r] & ~(RC_INT | RC_FLOAT);
            rc <<= --n;
            for (r = 0; r < NB_REGS; ++r)
                if (reg_classes[r] & rc)
                    break;
            vsetc(&ret.type, r, &ret.c);
        }
        vsetc(&ret.type, ret.r, &ret.c);
        vtop->r2 = ret.r2;

        if (((s->type.t & VT_BTYPE) == VT_STRUCT) && ret_nregs) {
            int addr, offset;

            size = type_size(&s->type, &align);
            size = (size + regsize - 1) & -regsize;
            if (ret_align > align)
                align = ret_align;
            loc = (loc - size) & -align;
            addr = loc;
            offset = 0;
            for (;;) {
                vset(&ret.type, VT_LOCAL | VT_LVAL, addr + offset);
                vswap();
                vstore();
                vtop--;
                if (--ret_nregs == 0)
                    break;
                offset += regsize;
            }
            vset(&s->type, VT_LOCAL | VT_LVAL, addr);
        }

        t = s->type.t & VT_BTYPE;
        if (t == VT_BYTE || t == VT_SHORT || t == VT_BOOL) {
#ifdef PROMOTE_RET
            vtop->r |= BFVAL(VT_MUSTCAST, 1);
#else
            vtop->type.t = VT_INT;
#endif
        }
        if (s->type.t & VT_REFERENCE)
            indir();
    }
}

static int cpp_try_free_binop(int op_tok)
{
    Sym *resolved, *s;
    SValue left, right, user_args[2];
    int v;

    if (!tcc_state->cpp)
        return 0;
    v = cpp_operator_field_tok(op_tok);
    if (!v || !cpp_has_free_func(v))
        return 0;
    if ((vtop[-1].type.t & VT_BTYPE) != VT_STRUCT
        && (vtop[0].type.t & VT_BTYPE) != VT_STRUCT)
        return 0;

    right = vtop[0];
    left = vtop[-1];
    vpop();
    vpop();

    user_args[0] = left;
    user_args[1] = right;
    vpushv(&left);
    vpushv(&right);
    resolved = cpp_resolve_free_func_call(v, 2);
    if (!resolved) {
        /* Restore stack for gen_op(); copied SValues keep r/lval as-is. */
        vpop();
        vpop();
        vpushv(&left);
        vpushv(&right);
        return 0;
    }

    vpop();
    vpop();
    vset(&resolved->type, resolved->r | VT_SYM, 0);
    vtop->sym = resolved;
    vtop->r &= ~VT_LVAL;
    s = resolved->type.ref;
    cpp_finish_free_call(s, user_args, 2);
    return 1;
}

static int cpp_try_cpp_subscript(void)
{
    Sym *field, *resolved, *s;
    SValue obj, index, user_args[2];
    int cumofs, v;

    if (!tcc_state->cpp || (vtop->type.t & VT_BTYPE) != VT_STRUCT || !vtop->type.ref)
        return 0;
    v = cpp_operator_field_tok('[');
    if (!v)
        return 0;

    field = cpp_find_operator_member(&vtop->type, v, &cumofs, 1);
    if (field && (field->type.t & VT_BTYPE) == VT_FUNC
        && !(field->type.ref && field->type.ref->f.func_virtual)) {
        obj = *vtop;
        vpop();
        next();
        gexpr();
        index = *vtop;
        vpop();
        vpushv(&obj);
        s = cpp_prepare_member_func_call(field);
        cpp_finish_member_call(s, &index, 1);
        skip(']');
        return 1;
    }

    if (!cpp_has_free_func(v))
        return 0;

    obj = *vtop;
    vpop();
    next();
    gexpr();
    index = *vtop;
    vpop();

    user_args[0] = obj;
    user_args[1] = index;
    vpushv(&obj);
    vpushv(&index);
    resolved = cpp_resolve_free_func_call(v, 2);
    if (!resolved) {
        vpop();
        vpop();
        tcc_error("operator[] not found for this type");
    }

    vpop();
    vpop();
    vset(&resolved->type, resolved->r | VT_SYM, 0);
    vtop->sym = resolved;
    vtop->r &= ~VT_LVAL;
    s = resolved->type.ref;
    cpp_finish_free_call(s, user_args, 2);
    skip(']');
    return 1;
}

/* FEAT-6A-ext3: unary operator on a struct operand (vtop).
   Member (0-arg) form; returns 0 to let the caller try the free form
   or the plain C path. */
static int cpp_try_member_unop(int op_tok)
{
    Sym *field, *s;
    int cumofs;

    if (!tcc_state->cpp || (vtop->type.t & VT_BTYPE) != VT_STRUCT)
        return 0;
    field = cpp_find_operator_member(&vtop->type, cpp_operator_field_tok(op_tok), &cumofs, 0);
    if (!field || (field->type.t & VT_BTYPE) != VT_FUNC)
        return 0;
    /* any-arity fallback may hand back a binary form; reject it here */
    if (cpp_func_param_count(field) != 0)
        return 0;
    if (field->type.ref && field->type.ref->f.func_virtual)
        return 0;
    s = cpp_prepare_member_func_call(field);
    cpp_finish_member_call(s, NULL, 0);
    return 1;
}

/* FEAT-6A-ext3: free unary operator(operand) fallback. */
static int cpp_try_free_unop(int op_tok)
{
    Sym *resolved, *s;
    SValue operand, user_args[1];
    int v;

    if (!tcc_state->cpp || (vtop->type.t & VT_BTYPE) != VT_STRUCT)
        return 0;
    v = cpp_operator_field_tok(op_tok);
    if (!v || !cpp_has_free_func(v))
        return 0;

    operand = *vtop;
    vpop();
    user_args[0] = operand;
    vpushv(&operand);
    resolved = cpp_resolve_free_func_call(v, 1);
    if (!resolved) {
        /* restore stack for the plain C path */
        vpop();
        vpushv(&operand);
        return 0;
    }

    vpop();
    vset(&resolved->type, resolved->r | VT_SYM, 0);
    vtop->sym = resolved;
    vtop->r &= ~VT_LVAL;
    s = resolved->type.ref;
    cpp_finish_free_call(s, user_args, 1);
    return 1;
}

static int cpp_try_member_binop(int op_tok)
{
    Sym *field, *s;
    SValue rhs;
    int cumofs;

    if (!tcc_state->cpp || (vtop[-1].type.t & VT_BTYPE) != VT_STRUCT)
        return 0;
    field = cpp_find_operator_member(&vtop[-1].type, cpp_operator_field_tok(op_tok), &cumofs, 1);
    if (!field || (field->type.t & VT_BTYPE) != VT_FUNC)
        return 0;
    if (field->type.ref && field->type.ref->f.func_virtual)
        return 0;

    rhs = vtop[0];
    vpop();
    s = cpp_prepare_member_func_call(field);
    cpp_finish_member_call(s, &rhs, 1);
    return 1;
}

static void check_fields(CType* type, int check)
{
    Sym* s = type->ref;

    while ((s = s->next) != NULL) {
        int v;
        /* C++: ctors/dtors are VT_FUNC members named after the class.
         * XORing SYM_FIELD onto that class token breaks later uses of
         * the class as a type (e.g. `Base *p` after `class D : public B`). */
        if (tcc_state->cpp && (s->type.t & VT_BTYPE) == VT_FUNC)
            continue;
        v = s->v & ~SYM_FIELD;
        if (v < SYM_FIRST_ANOM) {
            TokenSym* ts = table_ident[v - TOK_IDENT];
            if (check && (ts->tok & SYM_FIELD))
                tcc_error("�����o '%s' ���d�����Ă��܂�", get_tok_str(v, NULL));
            ts->tok ^= SYM_FIELD;
        }
        else if ((s->type.t & VT_BTYPE) == VT_STRUCT) {
            /* C++: embedded base subobjects were validated when the base
             * class was defined; do not recurse into them again. */
            if (tcc_state->cpp && s->parent_class)
                continue;
            check_fields(&s->type, check);
        }
    }
}

static void struct_layout(CType* type, AttributeDef* ad)
{
    int size, align, maxalign, offset, c, bit_pos, bit_size;
    int packed, a, bt, prevbt, prev_bit_size;
    int pcc = !tcc_state->ms_bitfields;
    int pragma_pack = *tcc_state->pack_stack_ptr;
    Sym* f;

    maxalign = 1;
    offset = 0;
    c = 0;
    bit_pos = 0;
    prevbt = VT_STRUCT; /* make it never match */
    prev_bit_size = 0;

    //#define BF_DEBUG

    for (f = type->ref->next; f; f = f->next) {
        if ((f->type.t & VT_BTYPE) == VT_FUNC)
            continue;
        if (f->type.t & VT_STATIC)
            continue;
        if (f->type.t & VT_BITFIELD)
            bit_size = BIT_SIZE(f->type.t);
        else
            bit_size = -1;
        size = type_size(&f->type, &align);
        a = f->a.aligned ? 1 << (f->a.aligned - 1) : 0;
        packed = 0;

        if (pcc && bit_size == 0) {
            /* in pcc mode, packing does not affect zero-width bitfields */

        }
        else {
            /* in pcc mode, attribute packed overrides if set. */
            if (pcc && (f->a.packed || ad->a.packed))
                align = packed = 1;

            /* pragma pack overrides align if lesser and packs bitfields always */
            if (pragma_pack) {
                packed = 1;
                if (pragma_pack < align)
                    align = pragma_pack;
                /* in pcc mode pragma pack also overrides individual align */
                if (pcc && pragma_pack < a)
                    a = 0;
            }
        }
        /* some individual align was specified */
        if (a)
            align = a;

        if (type->ref->type.t == VT_UNION) {
            if (pcc && bit_size >= 0)
                size = (bit_size + 7) >> 3;
            offset = 0;
            if (size > c)
                c = size;

        }
        else if (bit_size < 0) {
            if (pcc)
                c += (bit_pos + 7) >> 3;
            c = (c + align - 1) & -align;
            offset = c;
            if (size > 0)
                c += size;
            bit_pos = 0;
            prevbt = VT_STRUCT;
            prev_bit_size = 0;

        }
        else {
            /* A bit-field.  Layout is more complicated.  There are two
               options: PCC (GCC) compatible and MS compatible */
            if (pcc) {
                /* In PCC layout a bit-field is placed adjacent to the
                           preceding bit-fields, except if:
                           - it has zero-width
                           - an individual alignment was given
                           - it would overflow its base type container and
                             there is no packing */
                if (bit_size == 0) {
                new_field:
                    c = (c + ((bit_pos + 7) >> 3) + align - 1) & -align;
                    bit_pos = 0;
                }
                else if (f->a.aligned) {
                    goto new_field;
                }
                else if (!packed) {
                    int a8 = align * 8;
                    int ofs = ((c * 8 + bit_pos) % a8 + bit_size + a8 - 1) / a8;
                    if (ofs > size / align)
                        goto new_field;
                }

                /* in pcc mode, long long bitfields have type int if they fit */
                if (size == 8 && bit_size <= 32)
                    f->type.t = (f->type.t & ~VT_BTYPE) | VT_INT, size = 4;

                while (bit_pos >= align * 8)
                    c += align, bit_pos -= align * 8;
                offset = c;

                /* In PCC layout named bit-fields influence the alignment
                   of the containing struct using the base types alignment,
                   except for packed fields (which here have correct align).  */
                if (f->v & SYM_FIRST_ANOM
                    // && bit_size // ??? gcc on ARM/rpi does that
                    )
                    align = 1;

            }
            else {
                bt = f->type.t & VT_BTYPE;
                if ((bit_pos + bit_size > size * 8)
                    || (bit_size > 0) == (bt != prevbt)
                    ) {
                    c = (c + align - 1) & -align;
                    offset = c;
                    bit_pos = 0;
                    /* In MS bitfield mode a bit-field run always uses
                       at least as many bits as the underlying type.
                       To start a new run it's also required that this
                       or the last bit-field had non-zero width.  */
                    if (bit_size || prev_bit_size)
                        c += size;
                }
                /* In MS layout the records alignment is normally
                   influenced by the field, except for a zero-width
                   field at the start of a run (but by further zero-width
                   fields it is again).  */
                if (bit_size == 0 && prevbt != bt)
                    align = 1;
                prevbt = bt;
                prev_bit_size = bit_size;
            }

            f->type.t = (f->type.t & ~(0x3f << VT_STRUCT_SHIFT))
                | (bit_pos << VT_STRUCT_SHIFT);
            bit_pos += bit_size;
        }
        if (align > maxalign)
            maxalign = align;

#ifdef BF_DEBUG
        printf("set field %s offset %-2d size %-2d align %-2d",
            get_tok_str(f->v & ~SYM_FIELD, NULL), offset, size, align);
        if (f->type.t & VT_BITFIELD) {
            printf(" pos %-2d bits %-2d",
                BIT_POS(f->type.t),
                BIT_SIZE(f->type.t)
            );
        }
        printf("\n");
#endif

        f->c = offset;
        f->r = 0;
    }

    if (pcc)
        c += (bit_pos + 7) >> 3;

    /* store size and alignment */
    a = bt = ad->a.aligned ? 1 << (ad->a.aligned - 1) : 1;
    if (a < maxalign)
        a = maxalign;
    type->ref->r = a;
    if (pragma_pack && pragma_pack < maxalign && 0 == pcc) {
        /* can happen if individual align for some member was given.  In
           this case MSVC ignores maxalign when aligning the size */
        a = pragma_pack;
        if (a < bt)
            a = bt;
    }
    c = (c + a - 1) & -a;
    type->ref->c = c;

#ifdef BF_DEBUG
    printf("struct size %-2d align %-2d\n\n", c, a), fflush(stdout);
#endif

    /* check whether we can access bitfields by their type */
    for (f = type->ref->next; f; f = f->next) {
        int s, px, cx, c0;
        CType t;

        if (0 == (f->type.t & VT_BITFIELD))
            continue;
        f->type.ref = f;
        f->auxtype = -1;
        bit_size = BIT_SIZE(f->type.t);
        if (bit_size == 0)
            continue;
        bit_pos = BIT_POS(f->type.t);
        size = type_size(&f->type, &align);

        if (bit_pos + bit_size <= size * 8 && f->c + size <= c
#ifdef TCC_TARGET_ARM
            && !(f->c & (align - 1))
#endif
            )
            continue;

        /* try to access the field using a different type */
        c0 = -1, s = align = 1;
        t.t = VT_BYTE;
        for (;;) {
            px = f->c * 8 + bit_pos;
            cx = (px >> 3) & -align;
            px = px - (cx << 3);
            if (c0 == cx)
                break;
            s = (px + bit_size + 7) >> 3;
            if (s > 4) {
                t.t = VT_LLONG;
            }
            else if (s > 2) {
                t.t = VT_INT;
            }
            else if (s > 1) {
                t.t = VT_SHORT;
            }
            else {
                t.t = VT_BYTE;
            }
            s = type_size(&t, &align);
            c0 = cx;
        }

        if (px + bit_size <= s * 8 && cx + s <= c
#ifdef TCC_TARGET_ARM
            && !(cx & (align - 1))
#endif
            ) {
            /* update offset and bit position */
            f->c = cx;
            bit_pos = px;
            f->type.t = (f->type.t & ~(0x3f << VT_STRUCT_SHIFT))
                | (bit_pos << VT_STRUCT_SHIFT);
            if (s != size)
                f->auxtype = t.t;
#ifdef BF_DEBUG
            printf("FIX field %s offset %-2d size %-2d align %-2d "
                "pos %-2d bits %-2d\n",
                get_tok_str(f->v & ~SYM_FIELD, NULL),
                cx, s, align, px, bit_size);
#endif
        }
        else {
            /* fall back to load/store single-byte wise */
            f->auxtype = VT_STRUCT;
#ifdef BF_DEBUG
            printf("FIX field %s : load byte-wise\n",
                get_tok_str(f->v & ~SYM_FIELD, NULL));
#endif
        }
    }
}

/* enum/struct/union declaration. u is VT_ENUM/VT_STRUCT/VT_UNION */
static void struct_decl(CType* type, int u, int is_class)
{
    int v, c, size, align, flexible;
    int bit_size, bsize, bt, ut;
    Sym *s, *ss = NULL, **ps;
    AttributeDef ad, ad1;
    CType type1, btype;

    memset(&ad, 0, sizeof ad);
    next();
    parse_attribute(&ad);

    v = 0;
    if (tok >= TOK_IDENT) /* struct/enum tag */
        v = tok, next();

    bt = ut = 0;
    if (u == VT_ENUM) {
        ut = VT_INT;
        if (tok == ':') { /* C2x enum : <type> ... */
            next();
            if (!parse_btype(&btype, &ad1, 0)
                || !is_integer_btype(btype.t & VT_BTYPE))
                expect("enum type");
            bt = ut = btype.t & (VT_BTYPE | VT_LONG | VT_UNSIGNED | VT_DEFSIGN);
        }
    }

    if (v) {
        /* struct already defined ? return it */
        s = struct_find(v);
        if (s && (s->sym_scope == local_scope || (tok != '{' && tok != ';'))) {
            if (u == s->type.t)
                goto do_decl;
            if (u == VT_ENUM && IS_ENUM(s->type.t)) /* XXX: check integral types */
                goto do_decl;
            tcc_error("redefinition of '%s'", get_tok_str(v, NULL));
        }
    }
    else {
        if (tok != '{')
            expect("struct/union/enum name");
        v = anon_sym++;
    }
    /* Record the original enum/struct/union token.  */
    type1.t = u | ut;
    type1.ref = NULL;
    /* we put an undefined size for struct/union */
    s = sym_push(v | SYM_STRUCT, &type1, 0, bt ? 0 : -1);
    s->r = 0; /* default alignment is zero as gcc */
do_decl:
    type->t = s->type.t;
    type->ref = s;


    if (is_class && tok == ':') {
        Sym *base_class_sym, *base_field;
        CType base_type;

        next();
        if (tok == TOK_PUBLIC || tok == TOK_PRIVATE || tok == TOK_PROTECTED)
            next();
        if (tok < TOK_IDENT)
            tcc_error("expected base class name");
        base_class_sym = struct_find(tok);
        if (!base_class_sym)
            tcc_error("unknown base class");
        next();
        if (tok == ',')
            tcc_error("multiple inheritance not supported");
        base_type.t = VT_STRUCT;
        base_type.ref = base_class_sym;
        base_field = sym_push(anon_sym++ | SYM_FIELD, &base_type, 0, 0);
        base_field->a.access = ACCESS_PUBLIC;
        base_field->parent_class = base_class_sym;
        base_field->next = s->next;
        s->next = base_field;
    }

    if (tok == '{') {
        if (is_class)
            cpp_cur_class = s;
        next();
        if (s->c != -1
            && !(u == VT_ENUM && s->c == 0)) /* not yet defined typed enum */
            tcc_error("struct/union/enum already defined");
        s->c = -2;
        /* cannot be empty */
        /* non empty enums are not allowed */
        ps = &s->next;
        while (*ps)
            ps = &(*ps)->next;
        if (u == VT_ENUM) {
            long long ll = 0, pl = 0, nl = 0;
            CType t;
            t.ref = s;
            /* enum symbols have static storage */
            t.t = VT_INT | VT_STATIC | VT_ENUM_VAL;
            if (bt)
                t.t = bt | VT_STATIC | VT_ENUM_VAL;
            for (;;) {
                v = tok;
                if (v < TOK_UIDENT)
                    expect("identifier");
                ss = sym_find(v);
                if (ss && !local_stack)
                    tcc_error("redefinition of enumerator '%s'",
                        get_tok_str(v, NULL));
                next();
                if (tok == '=') {
                    next();
                    ll = expr_const64();
                }
                ss = sym_push(v, &t, VT_CONST, 0);
                ss->enum_val = ll;
                *ps = ss, ps = &ss->next;
                if (ll < nl)
                    nl = ll;
                if (ll > pl)
                    pl = ll;
                if (tok != ',')
                    break;
                next();
                ll++;
                /* NOTE: we accept a trailing comma */
                if (tok == '}')
                    break;
            }
            skip('}');

            if (bt) {
                t.t = bt;
                s->c = 2;
                goto enum_done;
            }

            /* set integral type of the enum */
            t.t = VT_INT;
            if (nl >= 0) {
                if (pl != (unsigned)pl)
                    t.t = (LONG_SIZE == 8 ? VT_LLONG | VT_LONG : VT_LLONG);
                t.t |= VT_UNSIGNED;
            }
            else if (pl != (int)pl || nl != (int)nl)
                t.t = (LONG_SIZE == 8 ? VT_LLONG | VT_LONG : VT_LLONG);

            /* set type for enum members */
            for (ss = s->next; ss; ss = ss->next) {
                ll = ss->enum_val;
                if (ll == (int)ll) /* default is int if it fits */
                    continue;
                if (t.t & VT_UNSIGNED) {
                    ss->type.t |= VT_UNSIGNED;
                    if (ll == (unsigned)ll)
                        continue;
                }
                ss->type.t = (ss->type.t & ~VT_BTYPE)
                    | (LONG_SIZE == 8 ? VT_LLONG | VT_LONG : VT_LLONG);
            }
            s->c = 1;
        enum_done:
            s->type.t = type->t = t.t | VT_ENUM;

        }
        else {
            int cur_access;
            c = 0;
            flexible = 0;
            cur_access = is_class ? ACCESS_PRIVATE : ACCESS_PUBLIC;
            while (tok != '}') {
                int skip_member_semi = 0;
                int is_ctor_decl = 0;
                int is_dtor_decl = 0;
                if (tok == TOK_PUBLIC || tok == TOK_PRIVATE || tok == TOK_PROTECTED) {
                    cur_access = (tok == TOK_PUBLIC) ? ACCESS_PUBLIC :
                                 (tok == TOK_PROTECTED) ? ACCESS_PROTECTED : ACCESS_PRIVATE;
                    next();
                    skip(':');
                    continue;
                }
                /* C++: a member named identically to the class with `(`
                 * after it is a ctor declaration with omitted return type.
                 * parse_btype() would otherwise consume the typedef'd class
                 * name as the return type and confuse the parser. */
                if (tcc_state->cpp && is_class
                    && tok == (s->v & ~SYM_STRUCT)) {
                    int saved = tok;
                    next();
                    if (tok == '(') {
                        is_ctor_decl = 1;
                        btype.t = VT_VOID;
                        btype.ref = NULL;
                        memset(&ad1, 0, sizeof ad1);
                    }
                    unget_tok(saved);
                }
                /* C++: `~Class(` is a destructor declaration with omitted
                 * return type (same pattern as ctor above). */
                if (tcc_state->cpp && is_class && tok == '~') {
                    int saved;
                    next();
                    if (struct_find(tok) == s) {
                        saved = tok;
                        next();
                        if (tok == '(') {
                            is_dtor_decl = 1;
                            btype.t = VT_VOID;
                            btype.ref = NULL;
                            memset(&ad1, 0, sizeof ad1);
                        }
                        unget_tok(saved);
                    } else {
                        unget_tok('~');
                    }
                }
                if (!is_ctor_decl && !is_dtor_decl && !parse_btype(&btype, &ad1, 0)) {
                    if (tok == TOK_STATIC_ASSERT) {
                        do_Static_assert();
                        continue;
                    }
                    skip(';');
                    continue;
                }
                while (1) {
                    if (flexible)
                        tcc_error("flexible array member cannot follow unnamed struct: %s",
                            get_tok_str(v, NULL));
                    bit_size = -1;
                    v = 0;
                    type1 = btype;
                    if (tok != ':') {
                        if (tok != ';')
                            type_decl(&type1, &ad1, &v, TYPE_DIRECT);
                        if (is_dtor_decl) {
                            int dtor_fld = cpp_dtor_field_tok(s->v & ~SYM_STRUCT);
                            if (!dtor_fld)
                                tcc_error("internal dtor field name failed");
                            v = dtor_fld;
                        }
                        if (v == 0) {
                            if ((type1.t & VT_BTYPE) != VT_STRUCT)
                                expect("identifier");
                            else {
                                int v = btype.ref->v;
                                if (!(v & SYM_FIELD) && (v & ~SYM_STRUCT) < SYM_FIRST_ANOM) {
                                    if (tcc_state->ms_extensions == 0)
                                        expect("identifier");
                                }
                            }
                        }
                        if (type_size(&type1, &align) < 0) {
                            if ((u == VT_STRUCT) && (type1.t & VT_ARRAY) && c)
                                flexible = 1;
                            else
                                tcc_error("�t�B�[���h '%s' �͕s���S�^�ł�",
                                    get_tok_str(v, NULL));
                        }
                        /* Stage 1: member function prototypes allowed in class only; struct in Stage 2+ */
                        if (((type1.t & VT_BTYPE) == VT_FUNC && !is_class) ||
                            ((type1.t & VT_BTYPE) == VT_VOID
                                && !is_ctor_decl && !is_dtor_decl) ||
                            ((type1.t & VT_STORAGE) && !(is_class && (type1.t & VT_STATIC))))
                            tcc_error("'%s' �ɑ΂��閳���Ȍ^",
                                get_tok_str(v, NULL));
                    }
                    if (tok == ':' && !is_ctor_decl) {
                        next();
                        bit_size = expr_const();
                        /* XXX: handle v = 0 case for messages */
                        if (bit_size < 0)
                            tcc_error("�r�b�g�t�B�[���h '%s' �̕������ł�",
                                get_tok_str(v, NULL));
                        if (v && bit_size == 0)
                            tcc_error("�r�b�g�t�B�[���h '%s' �̕���0�ł�",
                                get_tok_str(v, NULL));
                        parse_attribute(&ad1);
                    }
                    size = type_size(&type1, &align);
                    if (bit_size >= 0) {
                        bt = type1.t & VT_BTYPE;
                        if (bt != VT_INT &&
                            bt != VT_BYTE &&
                            bt != VT_SHORT &&
                            bt != VT_BOOL &&
                            bt != VT_LLONG)
                            tcc_error("�r�b�g�t�B�[���h�̓X�J���[�^�łȂ���΂Ȃ�܂���");
                        bsize = size * 8;
                        if (bit_size > bsize) {
                            tcc_error("'%s' �̕����^�̋��e�͈͂𒴂��Ă��܂�",
                                get_tok_str(v, NULL));
                        }
                        else if (bit_size == bsize
                            && !ad.a.packed && !ad1.a.packed) {
                            /* no need for bit fields */
                            ;
                        }
                        else if (bit_size == 64) {
                            tcc_error("�r�b�g�t�B�[���h��64�͖������ł�");
                        }
                        else {
                            type1.t = (type1.t & ~VT_STRUCT_MASK)
                                | VT_BITFIELD
                                | (bit_size << (VT_STRUCT_SHIFT + 6));
                        }
                    }
                    if (v != 0 || (type1.t & VT_BTYPE) == VT_STRUCT) {
                        /* Remember we've seen a real field to check
               for placement of flexible array member. */
                        c = 1;
                    }
                    /* If member is a struct or bit-field, enforce
                       placing into the struct (as anonymous).  */
                    if (v == 0 &&
                        ((type1.t & VT_BTYPE) == VT_STRUCT ||
                            bit_size >= 0)) {
                        v = anon_sym++;
                    }
                    if (v) {
                        ss = sym_push(v | SYM_FIELD, &type1, 0, 0);
                        ss->a = ad1.a;
                        ss->a.access = cur_access;
                        if ((type1.t & VT_BTYPE) == VT_FUNC)
                            ss->parent_class = s;
                        *ps = ss;
                        ps = &ss->next;
                        {
                            int is_ctor = is_class && !is_dtor_decl && v
                                && (v == (s->v & ~SYM_STRUCT))
                                && (type1.t & VT_BTYPE) == VT_FUNC;
                            if (is_ctor && tok == ':')
                                cpp_save_mem_init_list(ss);
                        }
                    }
                    if ((type1.t & VT_BTYPE) == VT_FUNC && tok == '{') {
                        /* Save the in-class inline body so cpp_finish_member_inlines
                         * can convert it into a real out-of-line function at end of
                         * the class declaration.  ss is non-NULL here because v != 0
                         * (a named function), so the `if (v)` block above ran. */
                        TokenString *body = NULL;
                        skip_or_save_block(&body);
                        if (tcc_state->cpp && v && body)
                            cpp_register_member_body(ss, s, &type1, body);
                        skip_member_semi = 1;
                        break;
                    }
                    if (tok == ';' || tok == TOK_EOF)
                        break;
                    skip(',');
                }
                if (!skip_member_semi)
                    skip(';');
            }
            skip('}');
            parse_attribute(&ad);
            if (ad.cleanup_func) {
                tcc_warning("���� '__cleanup__' �͌^�ɑ΂��Ė�������܂�");
            }
            check_fields(type, 1);
            check_fields(type, 0);
            if (is_class && tcc_state->cpp) {
                cpp_assign_virtual_slots(s);
                cpp_insert_vptr_field(s);
            }
            struct_layout(type, &ad);
            if (is_class) {
                int tag_v;
                CType ctype;

                tag_v = s->v & ~SYM_STRUCT;
                ctype = *type;
                ctype.t |= VT_TYPEDEF;
                sym_push(tag_v, &ctype, VT_TYPEDEF, 0);
            }
            if (debug_modes)
                tcc_debug_fix_anon(tcc_state, type);
            if (is_class) {
                /* Convert in-class inline bodies into real global functions
                 * (registered via inline_fns) so member calls can link. */
                cpp_finish_member_inlines(s);
                cpp_emit_vtable(s);
                cpp_cur_class = NULL;
            }
        }
    }
}

static void sym_to_attr(AttributeDef* ad, Sym* s)
{
    merge_symattr(&ad->a, &s->a);
    merge_funcattr(&ad->f, &s->f);
}

/* Add type qualifiers to a type. If the type is an array then the qualifiers
   are added to the element type, copied because it could be a typedef. */
static void parse_btype_qualify(CType* type, int qualifiers)
{
    while (type->t & VT_ARRAY) {
        type->ref = sym_push(SYM_FIELD, &type->ref->type, 0, type->ref->c);
        type = &type->ref->type;
    }
    type->t |= qualifiers;
}

/* return 0 if no type declaration. otherwise, return the basic type
   and skip it.
 */
static int parse_btype(CType* type, AttributeDef* ad, int ignore_label)
{
    int t, u, bt, st, type_found, typespec_found, g, n;
    Sym* s;
    CType type1;

    memset(ad, 0, sizeof(AttributeDef));
    type_found = 0;
    typespec_found = 0;
    t = VT_INT;
    bt = st = -1;
    type->ref = NULL;

    while (1) {
        switch (tok) {
        case TOK_EXTENSION:
            /* currently, we really ignore extension */
            next();
            continue;

            /* basic types */
        case TOK_CHAR:
            u = VT_BYTE;
        basic_type:
            next();
        basic_type1:
            if (u == VT_SHORT || u == VT_LONG) {
                if (st != -1 || (bt != -1 && bt != VT_INT))
                    tmbt: tcc_error("��{�^���������܂�");
                st = u;
            }
            else {
                if (bt != -1 || (st != -1 && u != VT_INT))
                    goto tmbt;
                bt = u;
            }
            if (u != VT_INT)
                t = (t & ~(VT_BTYPE | VT_LONG)) | u;
            typespec_found = 1;
            break;
        case TOK_VOID:
            u = VT_VOID;
            goto basic_type;
        case TOK_SHORT:
            u = VT_SHORT;
            goto basic_type;
        case TOK_INT:
            u = VT_INT;
            goto basic_type;
        case TOK_ALIGNAS:
        {
            int n;
            AttributeDef ad1;
            next();
            skip('(');
            memset(&ad1, 0, sizeof(AttributeDef));
            if (parse_btype(&type1, &ad1, 0)) {
                type_decl(&type1, &ad1, &n, TYPE_ABSTRACT);
                if (ad1.a.aligned)
                    n = 1 << (ad1.a.aligned - 1);
                else
                    type_size(&type1, &n);
            }
            else {
                n = expr_const();
                if (n < 0 || (n & (n - 1)) != 0)
                    tcc_error("�A���C�������g�͐���2�̙p�łȂ���΂Ȃ�܂���");
            }
            skip(')');
            ad->a.aligned = exact_log2p1(n);
        }
        continue;
        case TOK_LONG:
            if ((t & VT_BTYPE) == VT_DOUBLE) {
                t = (t & ~(VT_BTYPE | VT_LONG)) | VT_LDOUBLE;
            }
            else if ((t & (VT_BTYPE | VT_LONG)) == VT_LONG) {
                t = (t & ~(VT_BTYPE | VT_LONG)) | VT_LLONG;
            }
            else {
                u = VT_LONG;
                goto basic_type;
            }
            next();
            break;
        case TOK_BOOL:
            u = VT_BOOL;
            goto basic_type;
        case TOK_COMPLEX:
            tcc_error("_Complex �͂܂��T�|�[�g����Ă��܂���");
        case TOK_FLOAT:
            u = VT_FLOAT;
            goto basic_type;
        case TOK_DOUBLE:
            if ((t & (VT_BTYPE | VT_LONG)) == VT_LONG) {
                t = (t & ~(VT_BTYPE | VT_LONG)) | VT_LDOUBLE;
            }
            else {
                u = VT_DOUBLE;
                goto basic_type;
            }
            next();
            break;
        case TOK_ENUM:
            struct_decl(&type1, VT_ENUM, 0);
        basic_type2:
            u = type1.t;
            type->ref = type1.ref;
            goto basic_type1;
        case TOK_STRUCT:
            struct_decl(&type1, VT_STRUCT, 0);
            goto basic_type2;
        case TOK_CLASS:
            if (!tcc_state->cpp)
                tcc_error("class is C++ only");
            struct_decl(&type1, VT_STRUCT, 1);
            goto basic_type2;
        case TOK_UNION:
            struct_decl(&type1, VT_UNION, 0);
            goto basic_type2;

            /* type modifiers */
        case TOK__Atomic:
            next();
            type->t = t;
            parse_btype_qualify(type, VT_ATOMIC);
            t = type->t;
            if (tok == '(') {
                parse_expr_type(&type1);
                /* remove all storage modifiers except typedef */
                type1.t &= ~(VT_STORAGE & ~VT_TYPEDEF);
                if (type1.ref)
                    sym_to_attr(ad, type1.ref);
                goto basic_type2;
            }
            break;
        case TOK_CONST1:
        case TOK_CONST2:
        case TOK_CONST3:
            type->t = t;
            parse_btype_qualify(type, VT_CONSTANT);
            t = type->t;
            next();
            break;
        case TOK_VOLATILE1:
        case TOK_VOLATILE2:
        case TOK_VOLATILE3:
            type->t = t;
            parse_btype_qualify(type, VT_VOLATILE);
            t = type->t;
            next();
            break;
        case TOK_SIGNED1:
        case TOK_SIGNED2:
        case TOK_SIGNED3:
            if ((t & (VT_DEFSIGN | VT_UNSIGNED)) == (VT_DEFSIGN | VT_UNSIGNED))
                tcc_error("�����t���Ɩ������̏C���q�����݂��Ă��܂�");
            t |= VT_DEFSIGN;
            next();
            typespec_found = 1;
            break;
        case TOK_REGISTER:
        case TOK_AUTO:
        case TOK_RESTRICT1:
        case TOK_RESTRICT2:
        case TOK_RESTRICT3:
            next();
            break;
        case TOK_UNSIGNED:
            if ((t & (VT_DEFSIGN | VT_UNSIGNED)) == VT_DEFSIGN)
                tcc_error("�����t���Ɩ������̏C���q�����݂��Ă��܂�");
            t |= VT_DEFSIGN | VT_UNSIGNED;
            next();
            typespec_found = 1;
            break;

            /* storage */
        case TOK_EXTERN:
            g = VT_EXTERN;
            goto storage;
        case TOK_STATIC:
            g = VT_STATIC;
            goto storage;
        case TOK_TYPEDEF:
            g = VT_TYPEDEF;
            goto storage;
        storage:
            if (t & (VT_EXTERN | VT_STATIC | VT_TYPEDEF) & ~g)
                tcc_error("�X�g���[�W�N���X�������w�肳��Ă��܂�");
            t |= g;
            next();
            break;
        case TOK_INLINE1:
        case TOK_INLINE2:
        case TOK_INLINE3:
            t |= VT_INLINE;
            next();
            break;
        case TOK_VIRTUAL:
            if (!tcc_state->cpp)
                tcc_error("virtual requires C++");
            ad->f.func_virtual = 1;
            next();
            break;
        case TOK_NORETURN3:
            next();
            ad->f.func_noreturn = 1;
            break;
            /* GNUC attribute */
        case TOK_ATTRIBUTE1:
        case TOK_ATTRIBUTE2:
            parse_attribute(ad);
            if (ad->attr_mode) {
                u = ad->attr_mode - 1;
                t = (t & ~(VT_BTYPE | VT_LONG)) | u;
            }
            continue;
            /* GNUC typeof */
        case TOK_TYPEOF1:
        case TOK_TYPEOF2:
        case TOK_TYPEOF3:
            next();
            parse_expr_type(&type1);
            /* remove all storage modifiers except typedef */
            type1.t &= ~(VT_STORAGE & ~VT_TYPEDEF);
            if (type1.ref)
                sym_to_attr(ad, type1.ref);
            goto basic_type2;
        case TOK_THREAD_LOCAL:
            tcc_error("_Thread_local �͎�������Ă��܂���");
        default:
            if (typespec_found)
                goto the_end;
            s = sym_find(tok);
            if (tcc_state->cpp) {
                int cls_tok_lookup = tok & ~SYM_FIELD;
                Sym *stsym = struct_find(cls_tok_lookup);
                if (!stsym && s) {
                    Sym *ts;
                    for (ts = s; ts; ts = ts->prev_tok) {
                        if (ts->type.t & VT_TYPEDEF && ts->type.ref)
                            stsym = ts->type.ref;
                        else if ((ts->v & SYM_STRUCT)
                            && (ts->type.t & VT_BTYPE) == VT_STRUCT)
                            stsym = ts;
                        if (stsym)
                            break;
                    }
                }
                if (stsym && (stsym->type.t & VT_BTYPE) == VT_STRUCT) {
                    int cls_tok = tok;
                    next();
                    if (tok == ':') {
                        next();
                        if (tok == ':') {
                            unget_tok(':');
                            unget_tok(':');
                            unget_tok(cls_tok);
                            goto the_end;
                        }
                        unget_tok(':');
                    }
                    /* keep qualifiers: `const Foo cf` must stay const so
                       const member overloads resolve correctly (BUG-7) */
                    type->t = stsym->type.t
                        | (t & (VT_STORAGE | VT_CONSTANT | VT_VOLATILE));
                    type->ref = stsym;
                    typespec_found = 1;
                    st = bt = -2;
                    t = type->t;
                    break;
                }
            }
            if (!s || !(s->type.t & VT_TYPEDEF))
                goto the_end;
            n = tok, next();
            if (tok == ':' && ignore_label) {
                if (tcc_state->cpp) {
                    next();
                    if (tok == ':') {
                        unget_tok(':');
                        unget_tok(':');
                        unget_tok(n);
                        goto the_end;
                    }
                    unget_tok(':');
                }
                unget_tok(n);
                goto the_end;
            }

            t &= ~(VT_BTYPE | VT_LONG);
            u = t & ~(VT_CONSTANT | VT_VOLATILE), t ^= u;
            type->t = (s->type.t & ~VT_TYPEDEF) | u;
            type->ref = s->type.ref;
            if (t)
                parse_btype_qualify(type, t);
            t = type->t;
            /* get attributes from typedef */
            sym_to_attr(ad, s);
            typespec_found = 1;
            st = bt = -2;
            break;
        }
        type_found = 1;
    }
the_end:
    if (tcc_state->char_is_unsigned) {
        if ((t & (VT_DEFSIGN | VT_BTYPE)) == VT_BYTE)
            t |= VT_UNSIGNED;
    }
    /* VT_LONG is used just as a modifier for VT_INT / VT_LLONG */
    bt = t & (VT_BTYPE | VT_LONG);
    if (bt == VT_LONG)
        t |= LONG_SIZE == 8 ? VT_LLONG : VT_INT;
#ifdef TCC_USING_DOUBLE_FOR_LDOUBLE
    if (bt == VT_LDOUBLE)
        t = (t & ~(VT_BTYPE | VT_LONG)) | (VT_DOUBLE | VT_LONG);
#endif
    type->t = t;
    return type_found;
}

/* convert a function parameter type (array to pointer and function to
   function pointer) */
static inline void convert_parameter_type(CType* pt)
{
    int keep_const;

    /* remove const and volatile qualifiers (XXX: const could be used
       to indicate a const function parameter); keep const on C++ refs */
    keep_const = (tcc_state->cpp && (pt->t & VT_REFERENCE))
        ? (pt->t & VT_CONSTANT) : 0;
    pt->t &= ~(VT_CONSTANT | VT_VOLATILE);
    if (keep_const)
        pt->t |= VT_CONSTANT;
    /* array must be transformed to pointer according to ANSI C */
    pt->t &= ~(VT_ARRAY | VT_VLA);
    if ((pt->t & VT_BTYPE) == VT_FUNC) {
        mk_pointer(pt);
    }
}

ST_FUNC CString* parse_asm_str(void)
{
    skip('(');
    return parse_mult_str("string constant");
}

/* Parse an asm label and return the token */
static int asm_label_instr(void)
{
    int v;
    char* astr;

    next();
    astr = parse_asm_str()->data;
    skip(')');
#ifdef ASM_DEBUG
    printf("asm_alias: \"%s\"\n", astr);
#endif
    v = tok_alloc_const(astr);
    return v;
}

static int post_type(CType* type, AttributeDef* ad, int storage, int td)
{
    int n, l, t1, arg_size, align;
    Sym** plast, * s, * first;
    AttributeDef ad1;
    CType pt;
    TokenString* vla_array_tok = NULL;
    int* vla_array_str = NULL;

    if (tok == '(') {
        /* function type, or recursive declarator (return if so) */
        next();
        if (TYPE_DIRECT == (td & (TYPE_DIRECT | TYPE_ABSTRACT)))
            return 0;
        if (tok == ')')
            l = 0;
        else if (parse_btype(&pt, &ad1, 0))
            l = FUNC_NEW;
        else if (td & (TYPE_DIRECT | TYPE_ABSTRACT)) {
            merge_attr(ad, &ad1);
            return 0;
        }
        else
            l = FUNC_OLD;

        first = NULL;
        plast = &first;
        arg_size = 0;
        ++local_scope;
        if (l) {
            for (;;) {
                /* read param name and compute offset */
                if (l != FUNC_OLD) {
                    if ((pt.t & VT_BTYPE) == VT_VOID && tok == ')')
                        break;
                    type_decl(&pt, &ad1, &n, TYPE_DIRECT | TYPE_ABSTRACT | TYPE_PARAM);
                    if ((pt.t & VT_BTYPE) == VT_VOID)
                        tcc_error("�p�����[�^�� void �Ƃ��Đ錾����Ă��܂�");
                    if (n == 0)
                        n = SYM_FIELD;
                }
                else {
                    n = tok;
                    pt.t = VT_VOID; /* invalid type */
                    pt.ref = NULL;
                    next();
                }
                if (n < TOK_UIDENT)
                    expect("identifier");
                convert_parameter_type(&pt);
                arg_size += (type_size(&pt, &align) + PTR_SIZE - 1) / PTR_SIZE;
                /* these symbols may be evaluated for VLArrays (see below, under
                   nocode_wanted) which is why we push them here as normal symbols
                   temporarily.  Example: int func(int a, int b[++a]); */
                s = sym_push(n, &pt, VT_LOCAL | VT_LVAL, 0);
                if (tcc_state->cpp)
                    cpp_save_default_arg(s);
                *plast = s;
                plast = &s->next;
                if (tok == ')')
                    break;
                skip(',');
                if (l == FUNC_NEW && tok == TOK_DOTS) {
                    l = FUNC_ELLIPSIS;
                    next();
                    break;
                }
                if (l == FUNC_NEW && !parse_btype(&pt, &ad1, 0))
                    tcc_error("�����Ȍ^");
            }
        }
        else
            /* if no parameters, then old type prototype.
               C++: () is a real empty prototype, i.e. (void); keeping
               FUNC_OLD would make 0-arg and 1-arg overloads (e.g. unary
               vs binary operator-) "compatible" and break overloading. */
            l = tcc_state->cpp ? FUNC_NEW : FUNC_OLD;
        skip(')');
        if (tcc_state->cpp
            && (tok == TOK_CONST1 || tok == TOK_CONST2 || tok == TOK_CONST3)) {
            next();
            ad->f.func_const = 1;
        }
        /* remove parameter symbols from token table, keep on stack */
        if (first) {
            sym_pop(local_stack ? &local_stack : &global_stack, first->prev, 1);
            for (s = first; s; s = s->next)
                s->v |= SYM_FIELD;
        }
        --local_scope;
        /* NOTE: const is ignored in returned type as it has a special
           meaning in gcc / C++ */
        type->t &= ~VT_CONSTANT;
        /* some ancient pre-K&R C allows a function to return an array
           and the array brackets to be put after the arguments, such
           that "int c()[]" means something like "int[] c()" */
        if (tok == '[') {
            next();
            skip(']'); /* only handle simple "[]" */
            mk_pointer(type);
        }
        /* we push a anonymous symbol which will contain the function prototype */
        ad->f.func_args = arg_size;
        ad->f.func_type = l;
        s = sym_push(SYM_FIELD, type, 0, 0);
        s->a = ad->a;
        s->f = ad->f;
        s->next = first;
        type->t = VT_FUNC;
        type->ref = s;
    }
    else if (tok == '[') {
        int saved_nocode_wanted = nocode_wanted;
        /* array definition */
        next();
        n = -1;
        t1 = 0;
        if (td & TYPE_PARAM) while (1) {
            /* XXX The optional type-quals and static should only be accepted
               in parameter decls.  The '*' as well, and then even only
               in prototypes (not function defs).  */
            switch (tok) {
            case TOK_RESTRICT1: case TOK_RESTRICT2: case TOK_RESTRICT3:
            case TOK_CONST1:
            case TOK_VOLATILE1:
            case TOK_STATIC:
            case '*':
                next();
                continue;
            default:
                break;
            }
            if (tok != ']') {
                /* Code generation is not done now but has to be done
                   at start of function. Save code here for later use. */
                nocode_wanted = 1;
                skip_or_save_block(&vla_array_tok);
                unget_tok(0);
                vla_array_str = vla_array_tok->str;
                begin_macro(vla_array_tok, 2);
                next();
                gexpr();
                end_macro();
                next();
                goto check;
            }
            break;

        }
        else if (tok != ']') {
            if (!local_stack || (storage & VT_STATIC))
                vpushi(expr_const());
            else {
                /* VLAs (which can only happen with local_stack && !VT_STATIC)
                   length must always be evaluated, even under nocode_wanted,
                   so that its size slot is initialized (e.g. under sizeof
                   or typeof).  */
                nocode_wanted = 0;
                gexpr();
            }
        check:
            if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST) {
                n = vtop->c.i;
                if (n < 0)
                    tcc_error("�����Ȕz��T�C�Y");
            }
            else {
                if (!is_integer_btype(vtop->type.t & VT_BTYPE))
                    tcc_error("�ϒ��z��̃T�C�Y�͐����łȂ���΂Ȃ�܂���");
                n = 0;
                t1 = VT_VLA;
            }
        }
        skip(']');
        /* parse next post type */
        post_type(type, ad, storage, (td & ~(TYPE_DIRECT | TYPE_ABSTRACT)) | TYPE_NEST);

        if ((type->t & VT_BTYPE) == VT_FUNC)
            tcc_error("�֐��̔z��̐錾�͖����ł�");
        if ((type->t & VT_BTYPE) == VT_VOID
            || type_size(type, &align) < 0)
            tcc_error("�s���S�^�v�f�̔z��錾�͖����ł�");

        t1 |= type->t & VT_VLA;

        if (t1 & VT_VLA) {
            if (n < 0) {
                if (td & TYPE_NEST)
                    tcc_error("�ϒ��z��̓����T�C�Y�͖����I�Ɏw�肷��K�v������܂�");
            }
            else {
                loc -= type_size(&int_type, &align);
                loc &= -align;
                n = loc;

                vpush_type_size(type, &align);
                gen_op('*');
                vset(&int_type, VT_LOCAL | VT_LVAL, n);
                vswap();
                vstore();
            }
        }
        if (n != -1)
            vpop();
        nocode_wanted = saved_nocode_wanted;

        /* we push an anonymous symbol which will contain the array
           element type */
        s = sym_push(SYM_FIELD, type, 0, n);
        type->t = (t1 ? VT_VLA : VT_ARRAY) | VT_PTR;
        type->ref = s;

        if (vla_array_str) {
            /* for function args, the top dimension is converted to pointer */
            if ((t1 & VT_VLA) && (td & TYPE_NEST))
                s->vla_array_str = vla_array_str;
            else
                tok_str_free_str(vla_array_str);
        }
    }
    return 1;
}

/* Parse a type declarator (except basic type), and return the type
   in 'type'. 'td' is a bitmask indicating which kind of type decl is
   expected. 'type' should contain the basic type. 'ad' is the
   attribute definition of the basic type. It can be modified by
   type_decl().  If this (possibly abstract) declarator is a pointer chain
   it returns the innermost pointed to type (equals *type, but is a different
   pointer), otherwise returns type itself, that's used for recursive calls.  */
static CType* type_decl(CType* type, AttributeDef* ad, int* v, int td)
{
    CType* post, * ret;
    int qualifiers, storage;

    /* recursive type, remove storage bits first, apply them later again */
    storage = type->t & VT_STORAGE;
    type->t &= ~VT_STORAGE;
    post = ret = type;

    while (tcc_state->cpp && cpp_parse_member_pointer(type, &ret))
        ;
    while (tok == '&') {
        if (!tcc_state->cpp)
            tcc_error("reference type requires C++");
        qualifiers = 0;
    redo_ref:
        next();
        switch (tok) {
        case TOK_CONST1:
        case TOK_CONST2:
        case TOK_CONST3:
            qualifiers |= VT_CONSTANT;
            goto redo_ref;
        case TOK_VOLATILE1:
        case TOK_VOLATILE2:
        case TOK_VOLATILE3:
            qualifiers |= VT_VOLATILE;
            goto redo_ref;
        }
        mk_pointer(type);
        type->t |= qualifiers | VT_REFERENCE;
        if (ret == type)
            ret = pointed_type(type);
    }
    while (tok == '*') {
        qualifiers = 0;
    redo:
        next();
        switch (tok) {
        case TOK__Atomic:
            qualifiers |= VT_ATOMIC;
            goto redo;
        case TOK_CONST1:
        case TOK_CONST2:
        case TOK_CONST3:
            qualifiers |= VT_CONSTANT;
            goto redo;
        case TOK_VOLATILE1:
        case TOK_VOLATILE2:
        case TOK_VOLATILE3:
            qualifiers |= VT_VOLATILE;
            goto redo;
        case TOK_RESTRICT1:
        case TOK_RESTRICT2:
        case TOK_RESTRICT3:
            goto redo;
            /* XXX: clarify attribute handling */
        case TOK_ATTRIBUTE1:
        case TOK_ATTRIBUTE2:
            parse_attribute(ad);
            break;
        }
        mk_pointer(type);
        type->t |= qualifiers;
        if (ret == type)
            /* innermost pointed to type is the one for the first derivation */
            ret = pointed_type(type);
    }

    if (tok == '(') {
        /* This is possibly a parameter type list for abstract declarators
           ('int ()'), use post_type for testing this.  */
        if (!post_type(type, ad, 0, td)) {
            /* It's not, so it's a nested declarator, and the post operations
               apply to the innermost pointed to type (if any).  */
               /* XXX: this is not correct to modify 'ad' at this point, but
                  the syntax is not clear */
            parse_attribute(ad);
            post = type_decl(type, ad, v, td);
            skip(')');
        }
        else
            goto abstract;
    }
    else if (tcc_state->cpp && tok == TOK_OPERATOR && (td & TYPE_DIRECT)) {
        cpp_parse_operator_decl_name(v);
    }
    else if (tok >= TOK_IDENT && (td & TYPE_DIRECT)) {
        /* type identifier */
        *v = tok;
        next();
        parse_cpp_scope_qualifier(v);
        if (tcc_state->cpp && *v == TOK_OPERATOR) {
            if (tok == TOK_OPERATOR)
                cpp_parse_operator_decl_name(v);
            else
                *v = cpp_operator_field_tok(cpp_parse_operator_token());
        }
    }
    else {
    abstract:
        if (!(td & TYPE_ABSTRACT))
            expect("identifier");
        *v = 0;
    }
    post_type(post, ad, post != ret ? 0 : storage,
        td & ~(TYPE_DIRECT | TYPE_ABSTRACT));
    parse_attribute(ad);
    type->t |= storage;
    return ret;
}

/* indirection with full error checking and bound check */
ST_FUNC void indir(void)
{
    if ((vtop->type.t & VT_BTYPE) != VT_PTR) {
        if ((vtop->type.t & VT_BTYPE) == VT_FUNC)
            return;
        expect("pointer");
    }
    if (vtop->r & VT_LVAL)
        gv(RC_INT);
    vtop->type = *pointed_type(&vtop->type);
    /* Arrays and functions are never lvalues */
    if (!(vtop->type.t & (VT_ARRAY | VT_VLA))
        && (vtop->type.t & VT_BTYPE) != VT_FUNC) {
        vtop->r |= VT_LVAL;
        /* if bound checking, the referenced pointer must be checked */
#ifdef CONFIG_TCC_BCHECK
        if (tcc_state->do_bounds_check)
            vtop->r |= VT_MUSTBOUND;
#endif
    }
}

/* pass a parameter to a function and do type checking and casting */
static void gfunc_param_typed(Sym* func, Sym* arg)
{
    int func_type;
    CType type;

    func_type = func->f.func_type;
    if (func_type == FUNC_OLD ||
        (func_type == FUNC_ELLIPSIS && arg == NULL)) {
        /* default casting : only need to convert float to double */
        if ((vtop->type.t & VT_BTYPE) == VT_FLOAT) {
            gen_cast_s(VT_DOUBLE);
        }
        else if (vtop->type.t & VT_BITFIELD) {
            type.t = vtop->type.t & (VT_BTYPE | VT_UNSIGNED);
            type.ref = vtop->type.ref;
            gen_cast(&type);
        }
        else if (vtop->r & VT_MUSTCAST) {
            force_charshort_cast();
        }
    }
    else if (arg == NULL) {
        tcc_error("�֐��ւ̈������������܂�");
    }
    else {
        type = arg->type;
        if (!(type.t & VT_REFERENCE))
            type.t &= ~VT_CONSTANT; /* need to do that to avoid false warning */
        gen_assign_cast(&type);
    }
}

/* parse an expression and return its type without any side effect. */
static void expr_type(CType* type, void (*expr_fn)(void))
{
    nocode_wanted++;
    expr_fn();
    *type = vtop->type;
    vpop();
    nocode_wanted--;
}

/* parse an expression of the form '(type)' or '(expr)' and return its
   type */
static void parse_expr_type(CType* type)
{
    int n;
    AttributeDef ad;

    skip('(');
    if (parse_btype(type, &ad, 0)) {
        type_decl(type, &ad, &n, TYPE_ABSTRACT);
    }
    else {
        expr_type(type, gexpr);
    }
    skip(')');
}

static void parse_type(CType* type)
{
    AttributeDef ad;
    int n;

    if (!parse_btype(type, &ad, 0)) {
        expect("type");
    }
    type_decl(type, &ad, &n, TYPE_ABSTRACT);
}

static void parse_builtin_params(int nc, const char* args)
{
    char c, sep = '(';
    CType type;
    if (nc)
        nocode_wanted++;
    next();
    if (*args == 0)
        skip(sep);
    while ((c = *args++)) {
        skip(sep);
        sep = ',';
        if (c == 't') {
            parse_type(&type);
            vpush(&type);
            continue;
        }
        expr_eq();
        type.ref = NULL;
        type.t = 0;
        switch (c) {
        case 'e':
            continue;
        case 'V':
            type.t = VT_CONSTANT;
        case 'v':
            type.t |= VT_VOID;
            mk_pointer(&type);
            break;
        case 'S':
            type.t = VT_CONSTANT;
        case 's':
            type.t |= char_type.t;
            mk_pointer(&type);
            break;
        case 'i':
            type.t = VT_INT;
            break;
        case 'l':
            type.t = VT_SIZE_T;
            break;
        default:
            break;
        }
        gen_assign_cast(&type);
    }
    skip(')');
    if (nc)
        nocode_wanted--;
}

static void parse_atomic(int atok)
{
    int size, align, arg, t, save = 0;
    CType* atom, * atom_ptr, ct = { 0 };
    SValue store;
    char buf[40];
    static const char* const templates[] = {
        /*
         * Each entry consists of callback and function template.
         * The template represents argument types and return type.
         *
         * ? void (return-only)
         * b bool
         * a atomic
         * A read-only atomic
         * p pointer to memory
         * v value
         * l load pointer
         * s save pointer
         * m memory model
         */

         /* keep in order of appearance in tcctok.h: */
        /* __atomic_store */            "alm.?",
        /* __atomic_load */             "Asm.v",
        /* __atomic_exchange */         "alsm.v",
        /* __atomic_compare_exchange */ "aplbmm.b",
        /* __atomic_fetch_add */        "avm.v",
        /* __atomic_fetch_sub */        "avm.v",
        /* __atomic_fetch_or */         "avm.v",
        /* __atomic_fetch_xor */        "avm.v",
        /* __atomic_fetch_and */        "avm.v",
        /* __atomic_fetch_nand */       "avm.v",
        /* __atomic_and_fetch */        "avm.v",
        /* __atomic_sub_fetch */        "avm.v",
        /* __atomic_or_fetch */         "avm.v",
        /* __atomic_xor_fetch */        "avm.v",
        /* __atomic_and_fetch */        "avm.v",
        /* __atomic_nand_fetch */       "avm.v"
    };
    const char* template = templates[(atok - TOK___atomic_store)];

    atom = atom_ptr = NULL;
    size = 0; /* pacify compiler */
    next();
    skip('(');
    for (arg = 0;;) {
        expr_eq();
        switch (template[arg]) {
        case 'a':
        case 'A':
            atom_ptr = &vtop->type;
            if ((atom_ptr->t & VT_BTYPE) != VT_PTR)
                expect("pointer");
            atom = pointed_type(atom_ptr);
            size = type_size(atom, &align);
            if (size > 8
                || (size & (size - 1))
                || (atok > TOK___atomic_compare_exchange
                    && (0 == btype_size(atom->t & VT_BTYPE)
                        || (atom->t & VT_BTYPE) == VT_PTR)))
                expect("integral or integer-sized pointer target type");
            /* GCC does not care either: */
            /* if (!(atom->t & VT_ATOMIC))
                tcc_warning("pointer target declaration is missing '_Atomic'"); */
            break;

        case 'p':
            if ((vtop->type.t & VT_BTYPE) != VT_PTR
                || type_size(pointed_type(&vtop->type), &align) != size)
                tcc_error("���� %d �̃|�C���^�̃^�[�Q�b�g�^����v���܂���", arg + 1);
            gen_assign_cast(atom_ptr);
            break;
        case 'v':
            gen_assign_cast(atom);
            break;
        case 'l':
            indir();
            gen_assign_cast(atom);
            break;
        case 's':
            save = 1;
            indir();
            store = *vtop;
            vpop();
            break;
        case 'm':
            gen_assign_cast(&int_type);
            break;
        case 'b':
            ct.t = VT_BOOL;
            gen_assign_cast(&ct);
            break;
        }
        if ('.' == template[++arg])
            break;
        skip(',');
    }
    skip(')');

    ct.t = VT_VOID;
    switch (template[arg + 1]) {
    case 'b':
        ct.t = VT_BOOL;
        break;
    case 'v':
        ct = *atom;
        break;
    }

    sprintf(buf, "%s_%d", get_tok_str(atok, 0), size);
    vpush_helper_func(tok_alloc_const(buf));
    vrott(arg - save + 1);
    gfunc_call(arg - save);

    vpush(&ct);
    PUT_R_RET(vtop, ct.t);
    t = ct.t & VT_BTYPE;
    if (t == VT_BYTE || t == VT_SHORT || t == VT_BOOL) {
#ifdef PROMOTE_RET
        vtop->r |= BFVAL(VT_MUSTCAST, 1);
#else
        vtop->type.t = VT_INT;
#endif
    }
    gen_cast(&ct);
    if (save) {
        vpush(&ct);
        *vtop = store;
        vswap();
        vstore();
    }
}

ST_FUNC void unary(void)
{
    int n, t, align, size, r;
    CType type;
    Sym* s;
    AttributeDef ad;

    /* generate line number info */
    if (debug_modes)
        tcc_debug_line(tcc_state), tcc_tcov_check_line(tcc_state, 1);

    type.ref = NULL;
    /* XXX: GCC 2.95.3 does not generate a table although it should be
       better here */
tok_next:
    switch (tok) {
    case TOK_EXTENSION:
        next();
        goto tok_next;
    case TOK_LCHAR:
#ifdef TCC_TARGET_PE
        t = VT_SHORT | VT_UNSIGNED;
        goto push_tokc;
#endif
    case TOK_CINT:
    case TOK_CCHAR:
        t = VT_INT;
    push_tokc:
        type.t = t;
        vsetc(&type, VT_CONST, &tokc);
        next();
        break;
    case TOK_CUINT:
        t = VT_INT | VT_UNSIGNED;
        goto push_tokc;
    case TOK_CLLONG:
        t = VT_LLONG;
        goto push_tokc;
    case TOK_CULLONG:
        t = VT_LLONG | VT_UNSIGNED;
        goto push_tokc;
    case TOK_CFLOAT:
        t = VT_FLOAT;
        goto push_tokc;
    case TOK_CDOUBLE:
        t = VT_DOUBLE;
        goto push_tokc;
    case TOK_CLDOUBLE:
#ifdef TCC_USING_DOUBLE_FOR_LDOUBLE
        t = VT_DOUBLE | VT_LONG;
#else
        t = VT_LDOUBLE;
#endif
        goto push_tokc;
    case TOK_CLONG:
        t = (LONG_SIZE == 8 ? VT_LLONG : VT_INT) | VT_LONG;
        goto push_tokc;
    case TOK_CULONG:
        t = (LONG_SIZE == 8 ? VT_LLONG : VT_INT) | VT_LONG | VT_UNSIGNED;
        goto push_tokc;
    case TOK___FUNCTION__:
        if (!gnu_ext)
            goto tok_identifier;
        /* fall thru */
    case TOK___FUNC__:
        tok = TOK_STR;
        cstr_reset(&tokcstr);
        cstr_cat(&tokcstr, funcname, 0);
        tokc.str.size = tokcstr.size;
        tokc.str.data = tokcstr.data;
        goto case_TOK_STR;
    case TOK_LSTR:
#ifdef TCC_TARGET_PE
        t = VT_SHORT | VT_UNSIGNED;
#else
        t = VT_INT;
#endif
        goto str_init;
    case TOK_STR:
    case_TOK_STR:
        /* string parsing */
        t = char_type.t;
    str_init:
        if (tcc_state->warn_write_strings & WARN_ON)
            t |= VT_CONSTANT;
        type.t = t;
        mk_pointer(&type);
        type.t |= VT_ARRAY;
        memset(&ad, 0, sizeof(AttributeDef));
        ad.section = rodata_section;
        decl_initializer_alloc(&type, &ad, VT_CONST, 2, 0, 0);
        break;
    case TOK_SOTYPE:
    case '(':
        t = tok;
        next();
        /* cast ? */
        if (parse_btype(&type, &ad, 0)) {
            type_decl(&type, &ad, &n, TYPE_ABSTRACT);
            skip(')');
            /* check ISOC99 compound literal */
            if (tok == '{') {
                /* data is allocated locally by default */
                if (global_expr)
                    r = VT_CONST;
                else
                    r = VT_LOCAL;
                /* all except arrays are lvalues */
                if (!(type.t & VT_ARRAY))
                    r |= VT_LVAL;
                memset(&ad, 0, sizeof(AttributeDef));
                decl_initializer_alloc(&type, &ad, r, 1, 0, 0);
            }
            else if (t == TOK_SOTYPE) { /* from sizeof/alignof (...) */
                vpush(&type);
                return;
            }
            else {
                unary();
                gen_cast(&type);
            }
        }
        else if (tok == '{') {
            int saved_nocode_wanted = nocode_wanted;
            if (CONST_WANTED && !NOEVAL_WANTED)
                expect("constant");
            if (0 == local_scope)
                tcc_error("�֐��O�ł� statement expression �͋�����Ă��܂���");
            /* save all registers */
            save_regs(0);
            /* statement expression : we do not accept break/continue
               inside as GCC does.  We do retain the nocode_wanted state,
           as statement expressions can't ever be entered from the
           outside, so any reactivation of code emission (from labels
           or loop heads) can be disabled again after the end of it. */
            block(STMT_EXPR);
            /* If the statement expr can be entered, then we retain the current
               nocode_wanted state (from e.g. a 'return 0;' in the stmt-expr).
               If it can't be entered then the state is that from before the
               statement expression.  */
            if (saved_nocode_wanted)
                nocode_wanted = saved_nocode_wanted;
            skip(')');
        }
        else {
            gexpr();
            skip(')');
        }
        break;
    case '*':
        next();
        unary();
        indir();
        break;
    case '&':
        next();
        if (tcc_state->cpp) {
            int cls_tok, mem_tok;
            Sym *class_sym, *field, *fsym;
            CType ct, mpt;
            int cumofs;

            if (cpp_parse_qualified_member(&cls_tok, &mem_tok)) {
                class_sym = struct_find(cls_tok);
                if (!class_sym)
                    tcc_error("unknown class in member pointer");
                ct.t = VT_STRUCT;
                ct.ref = class_sym;
                field = find_field(&ct, mem_tok, &cumofs);
                if ((field->type.t & VT_BTYPE) == VT_FUNC) {
                    if (field->type.ref && field->type.ref->f.func_virtual)
                        tcc_error("member function pointer to virtual method not supported");
                    fsym = cpp_lookup_member_func(field, &ct);
                    mpt = field->type;
                    mk_member_pointer(&mpt, class_sym, mem_tok);
                    vset(&mpt, fsym->r | VT_SYM, 0);
                    vtop->sym = fsym;
                    vtop->c.i = 0;
                    vtop->r &= ~VT_LVAL;
                } else {
                    mpt = field->type;
                    mk_member_pointer(&mpt, class_sym, mem_tok);
                    vset(&mpt, VT_CONST, 0);
                    vtop->c.i = cumofs;
                }
                break;
            }
        }
        unary();
        /* functions names must be treated as function pointers,
           except for unary '&' and sizeof. Since we consider that
           functions are not lvalues, we only have to handle it
           there and in function calls. */
           /* arrays can also be used although they are not lvalues */
        if ((vtop->type.t & VT_BTYPE) != VT_FUNC &&
            !(vtop->type.t & (VT_ARRAY | VT_VLA)))
            test_lvalue();
        if (vtop->sym)
            vtop->sym->a.addrtaken = 1;
        mk_pointer(&vtop->type);
        gaddrof();
        break;
    case '!':
        next();
        unary();
        if (!cpp_try_member_unop('!') && !cpp_try_free_unop('!'))
            gen_test_zero(TOK_EQ);
        break;
    case '~':
        next();
        unary();
        if (!cpp_try_member_unop('~') && !cpp_try_free_unop('~')) {
            vpushi(-1);
            gen_op('^');
        }
        break;
    case '+':
        next();
        unary();
        if ((vtop->type.t & VT_BTYPE) == VT_PTR)
            tcc_error("�P�� + �ɑ΂��ă|�C���^�͋��e����܂���");
        /* In order to force cast, we add zero, except for floating point
       where we really need an noop (otherwise -0.0 will be transformed
       into +0.0).  */
        if (!is_float(vtop->type.t)) {
            vpushi(0);
            gen_op('+');
        }
        break;
    case TOK_SIZEOF:
    case TOK_ALIGNOF1:
    case TOK_ALIGNOF2:
    case TOK_ALIGNOF3:
        t = tok;
        next();
        if (tok == '(')
            tok = TOK_SOTYPE;
        expr_type(&type, unary);
        if (t == TOK_SIZEOF) {
            vpush_type_size(&type, &align);
            gen_cast_s(VT_SIZE_T);
        }
        else {
            type_size(&type, &align);
            s = NULL;
            if (vtop[1].r & VT_SYM)
                s = vtop[1].sym; /* hack: accessing previous vtop */
            if (s && s->a.aligned)
                align = 1 << (s->a.aligned - 1);
            vpushs(align);
        }
        break;

    case TOK_builtin_expect:
        /* __builtin_expect is a no-op for now */
        parse_builtin_params(0, "ee");
        vpop();
        break;
    case TOK_builtin_types_compatible_p:
        parse_builtin_params(0, "tt");
        vtop[-1].type.t &= ~(VT_CONSTANT | VT_VOLATILE);
        vtop[0].type.t &= ~(VT_CONSTANT | VT_VOLATILE);
        n = is_compatible_types(&vtop[-1].type, &vtop[0].type);
        vtop -= 2;
        vpushi(n);
        break;
    case TOK_builtin_choose_expr:
    {
        int64_t c;
        next();
        skip('(');
        c = expr_const64();
        skip(',');
        if (!c) {
            nocode_wanted++;
        }
        expr_eq();
        if (!c) {
            vpop();
            nocode_wanted--;
        }
        skip(',');
        if (c) {
            nocode_wanted++;
        }
        expr_eq();
        if (c) {
            vpop();
            nocode_wanted--;
        }
        skip(')');
    }
    break;
    case TOK_builtin_constant_p:
        parse_builtin_params(1, "e");
        n = 1;
        if ((vtop->r & (VT_VALMASK | VT_LVAL)) != VT_CONST
            || ((vtop->r & VT_SYM) && vtop->sym->a.addrtaken)
            )
            n = 0;
        vtop--;
        vpushi(n);
        break;
    case TOK_builtin_unreachable:
        parse_builtin_params(0, ""); /* just skip '()' */
        type.t = VT_VOID;
        vpush(&type);
        CODE_OFF();
        break;
    case TOK_builtin_frame_address:
    case TOK_builtin_return_address:
    {
        int tok1 = tok;
        int level;
        next();
        skip('(');
        level = expr_const();
        if (level < 0)
            tcc_error("%s �͐��̐����̂ݎ󂯕t���܂�", get_tok_str(tok1, 0));
        skip(')');
        type.t = VT_VOID;
        mk_pointer(&type);
        vset(&type, VT_LOCAL, 0);       /* local frame */
        while (level--) {
#ifdef TCC_TARGET_RISCV64
            vpushi(2 * PTR_SIZE);
            gen_op('-');
#endif
            mk_pointer(&vtop->type);
            indir();                    /* -> parent frame */
        }
        if (tok1 == TOK_builtin_return_address) {
            // assume return address is just above frame pointer on stack
#ifdef TCC_TARGET_ARM
            vpushi(2 * PTR_SIZE);
            gen_op('+');
#elif defined TCC_TARGET_RISCV64
            vpushi(PTR_SIZE);
            gen_op('-');
#else
            vpushi(PTR_SIZE);
            gen_op('+');
#endif
            mk_pointer(&vtop->type);
            indir();
        }
    }
    break;
#ifdef TCC_TARGET_RISCV64
    case TOK_builtin_va_start:
        parse_builtin_params(0, "ee");
        r = vtop->r & VT_VALMASK;
        if (r == VT_LLOCAL)
            r = VT_LOCAL;
        if (r != VT_LOCAL)
            tcc_error("__builtin_va_start �̓��[�J���ϐ���K�v�Ƃ��܂�");
        gen_va_start();
        vstore();
        break;
#endif
#ifdef TCC_TARGET_X86_64
#ifdef TCC_TARGET_PE
    case TOK_builtin_va_start:
        parse_builtin_params(0, "ee");
        r = vtop->r & VT_VALMASK;
        if (r == VT_LLOCAL)
            r = VT_LOCAL;
        if (r != VT_LOCAL)
            tcc_error("__builtin_va_start �̓��[�J���ϐ���K�v�Ƃ��܂�");
        vtop->r = r;
        vtop->type = char_pointer_type;
        vtop->c.i += 8;
        vstore();
        break;
#else
    case TOK_builtin_va_arg_types:
        parse_builtin_params(0, "t");
        vpushi(classify_x86_64_va_arg(&vtop->type));
        vswap();
        vpop();
        break;
#endif
#endif

#ifdef TCC_TARGET_ARM64
    case TOK_builtin_va_start: {
        parse_builtin_params(0, "ee");
        //xx check types
        gen_va_start();
        vpushi(0);
        vtop->type.t = VT_VOID;
        break;
    }
    case TOK_builtin_va_arg: {
        parse_builtin_params(0, "et");
        type = vtop->type;
        vpop();
        //xx check types
        gen_va_arg(&type);
        vtop->type = type;
        break;
    }
    case TOK___arm64_clear_cache: {
        parse_builtin_params(0, "ee");
        gen_clear_cache();
        vpushi(0);
        vtop->type.t = VT_VOID;
        break;
    }
#endif

                                /* atomic operations */
    case TOK___atomic_store:
    case TOK___atomic_load:
    case TOK___atomic_exchange:
    case TOK___atomic_compare_exchange:
    case TOK___atomic_fetch_add:
    case TOK___atomic_fetch_sub:
    case TOK___atomic_fetch_or:
    case TOK___atomic_fetch_xor:
    case TOK___atomic_fetch_and:
    case TOK___atomic_fetch_nand:
    case TOK___atomic_add_fetch:
    case TOK___atomic_sub_fetch:
    case TOK___atomic_or_fetch:
    case TOK___atomic_xor_fetch:
    case TOK___atomic_and_fetch:
    case TOK___atomic_nand_fetch:
        parse_atomic(tok);
        break;

        /* pre operations */
    case TOK_INC:
    case TOK_DEC:
        t = tok;
        next();
        unary();
        if (!cpp_try_member_unop(t) && !cpp_try_free_unop(t))
            inc(0, t);
        break;
    case '-':
        next();
        unary();
        if (cpp_try_member_unop('-') || cpp_try_free_unop('-'))
            break;
        if (is_float(vtop->type.t)) {
            gen_opif(TOK_NEG);
        }
        else {
            vpushi(0);
            vswap();
            gen_op('-');
        }
        break;
    case TOK_LAND:
        if (!gnu_ext)
            goto tok_identifier;
        next();
        /* allow to take the address of a label */
        if (tok < TOK_UIDENT)
            expect("label identifier");
        s = label_find(tok);
        if (!s) {
            s = label_push(&global_label_stack, tok, LABEL_FORWARD);
        }
        else {
            if (s->r == LABEL_DECLARED)
                s->r = LABEL_FORWARD;
        }
        if ((s->type.t & VT_BTYPE) != VT_PTR) {
            s->type.t = VT_VOID;
            mk_pointer(&s->type);
            s->type.t |= VT_STATIC;
        }
        vpushsym(&s->type, s);
        next();
        break;

    case TOK_GENERIC:
    {
        CType controlling_type;
        int has_default = 0;
        int has_match = 0;
        int learn = 0;
        TokenString* str = NULL;
        int saved_nocode_wanted = nocode_wanted;
        nocode_wanted &= ~CONST_WANTED_MASK;

        next();
        skip('(');
        expr_type(&controlling_type, expr_eq);
        convert_parameter_type(&controlling_type);

        nocode_wanted = saved_nocode_wanted;

        for (;;) {
            learn = 0;
            skip(',');
            if (tok == TOK_DEFAULT) {
                if (has_default)
                    tcc_error("'default' ���������܂�");
                has_default = 1;
                if (!has_match)
                    learn = 1;
                next();
            }
            else {
                AttributeDef ad_tmp;
                int itmp;
                CType cur_type;

                parse_btype(&cur_type, &ad_tmp, 0);
                type_decl(&cur_type, &ad_tmp, &itmp, TYPE_ABSTRACT);
                if (compare_types(&controlling_type, &cur_type, 0)) {
                    if (has_match) {
                        tcc_error("�^��2���v���܂���");
                    }
                    has_match = 1;
                    learn = 1;
                }
            }
            skip(':');
            if (learn) {
                if (str)
                    tok_str_free(str);
                skip_or_save_block(&str);
            }
            else {
                skip_or_save_block(NULL);
            }
            if (tok == ')')
                break;
        }
        if (!str) {
            char buf[60];
            type_to_str(buf, sizeof buf, &controlling_type, NULL);
            tcc_error("�^ '%s' �͂ǂ̊֘A�t���ɂ���v���܂���", buf);
        }
        begin_macro(str, 1);
        next();
        expr_eq();
        if (tok != TOK_EOF)
            expect(",");
        end_macro();
        next();
        break;
    }
    // special qnan , snan and infinity values
    case TOK___NAN__:
        n = 0x7fc00000;
    special_math_val:
        vpushi(n);
        vtop->type.t = VT_FLOAT;
        next();
        break;
    case TOK___SNAN__:
        n = 0x7f800001;
        goto special_math_val;
    case TOK___INF__:
        n = 0x7f800000;
        goto special_math_val;

    case TOK_THIS:
        if (!tcc_state->cpp || !cpp_this_sym)
            tcc_error("invalid use of 'this'");
        next();
        vset(&cpp_this_sym->type, cpp_this_sym->r, cpp_this_sym->c);
        vtop->sym = cpp_this_sym;
        break;
    default:
    tok_identifier:
        if (tok < TOK_UIDENT)
            tcc_error("'%s' �̑O�Ɏ����K�v�ł�", get_tok_str(tok, &tokc));
        t = tok;
        next();
        s = sym_find(t);
        if (!s && tcc_state->cpp && cpp_cur_func_class) {
            Sym *mf = cpp_lookup_member_field(t, cpp_cur_func_class);
            if (mf) {
                cpp_push_member_var(mf);
                break;
            }
        }
        if (!s || IS_ASM_SYM(s)) {
            const char* name = get_tok_str(t, NULL);
            if (tok != '(')
                tcc_error("'%s' �͐錾����Ă��܂���", name);
            /* for simple function calls, we tolerate undeclared
               external reference to int() function */
            tcc_warning_c(warn_implicit_function_declaration)(
                "implicit declaration of function '%s'", name);
            s = external_global_sym(t, &func_old_type);
        }

        r = s->r;
        /* A symbol that has a register is a local register variable,
           which starts out as VT_LOCAL value.  */
        if ((r & VT_VALMASK) < VT_CONST)
            r = (r & ~VT_VALMASK) | VT_LOCAL;

        vset(&s->type, r, s->c);
        /* Point to s as backpointer (even without r&VT_SYM).
       Will be used by at least the x86 inline asm parser for
       regvars.  */
        vtop->sym = s;

        if (r & VT_SYM) {
            vtop->c.i = 0;
#ifdef TCC_TARGET_PE
            if (s->a.dllimport) {
                mk_pointer(&vtop->type);
                vtop->r |= VT_LVAL;
                indir();
            }
#endif
        }
        else if (r == VT_CONST && IS_ENUM_VAL(s->type.t)) {
            vtop->c.i = s->enum_val;
        }

        /* C++ BUG-9: a reference variable always denotes its referent;
           dereference at use.  Function symbols keep VT_REFERENCE to
           mean "returns a reference", so exclude them. */
        if (tcc_state->cpp && (s->type.t & VT_REFERENCE)
            && (s->type.t & VT_BTYPE) != VT_FUNC)
            indir();
        break;
    }

    /* post operations */
    while (1) {
        if (tok == TOK_INC || tok == TOK_DEC) {
            inc(1, tok);
            next();
        }
        else if (tok == '.' || tok == TOK_ARROW) {
            int qualifiers, cumofs;
            CType obj_type;
            Sym *field;

            if (tok == TOK_ARROW)
                indir();
            /* C++: '.' on a reference is like '->' (deref then member) */
            else if (tcc_state->cpp && (vtop->type.t & VT_REFERENCE))
                indir();
            qualifiers = vtop->type.t & (VT_CONSTANT | VT_VOLATILE);
            test_lvalue();
            next();
            if (tcc_state->cpp && tok == '*') {
                SValue obj, pm;

                next();
                obj = *vtop;
                vpop();
                unary();
                pm = *vtop;
                vpop();
                if (!cpp_is_member_pointer(&pm.type))
                    tcc_error("invalid member pointer");
                if (!cpp_mptr_compatible_class(&obj.type, cpp_mptr_class(&pm.type)))
                    tcc_error("member pointer class mismatch");
                if (cpp_is_mptr_to_func(&pm.type))
                    cpp_emit_mptr_pmf_invoke(&obj, &pm);
                else
                    cpp_emit_mptr_dmp_access(&obj, &pm);
            } else if (tcc_state->cpp && tok == '~') {
                Sym *class_sym;
                Sym *fsym;

                next();
                if (!vtop->type.ref
                    || (vtop->type.t & VT_BTYPE) != VT_STRUCT
                    || tok != (vtop->type.ref->v & ~SYM_STRUCT))
                    expect("destructor name");
                class_sym = vtop->type.ref;
                field = cpp_find_dtor_field(class_sym);
                if (!field)
                    tcc_error("no destructor for class");
                obj_type = vtop->type;
                gaddrof();
                cpp_member_this = *vtop;
                vpop();
                fsym = cpp_lookup_member_func(field, &obj_type);
                vset(&fsym->type, fsym->r | VT_SYM, 0);
                vtop->sym = fsym;
                vtop->r &= ~VT_LVAL;
                cpp_member_this_pending = 1;
                next();
            } else {
            int mem_tok, operator_name;

            mem_tok = tok;
            operator_name = 0;
            if (tcc_state->cpp && tok == TOK_OPERATOR) {
                next();
                mem_tok = cpp_operator_field_tok(cpp_parse_operator_token());
                operator_name = 1;
            }
            field = cpp_find_field_for_call(&vtop->type, mem_tok, &cumofs);
            if (tcc_state->cpp && (field->type.t & VT_BTYPE) == VT_FUNC) {
                if (field->type.ref && field->type.ref->f.func_virtual) {
                    obj_type = vtop->type;
                    cpp_prepare_virtual_member_call(field, &obj_type);
                } else {
                cpp_prepare_member_func_call(field);
                cpp_member_this_pending = 1;
                }
            } else {
                gaddrof();
                vtop->type = char_pointer_type;
                vpushi(cumofs);
                gen_op('+');
                vtop->type = field->type;
                vtop->type.t |= qualifiers;
                if (!(vtop->type.t & VT_ARRAY)) {
                    vtop->r |= VT_LVAL;
#ifdef CONFIG_TCC_BCHECK
                    if (tcc_state->do_bounds_check)
                        vtop->r |= VT_MUSTBOUND;
#endif
                }
            }
            if (!operator_name)
                next();
            }
        }
        else if (tcc_state->cpp && tok == ':') {
            Sym *class_sym, *ss;
            int mem_tok, cls_tok;

            /* Check second ':' first so a ternary ':' does not trigger
             * the struct check and falsely fire "identifier required". */
            next();
            if (tok != ':') {
                unget_tok(':');
                break;
            }
            if (!vtop->sym || (vtop->type.t & VT_BTYPE) != VT_STRUCT || !vtop->type.ref)
                expect("identifier");
            class_sym = vtop->type.ref;
            cls_tok = vtop->sym->v;
            next();
            if (tok < TOK_IDENT)
                tcc_error("expected member name after ::");
            mem_tok = tok;
            next();
            ss = cpp_lookup_static_member(class_sym, mem_tok);
            if (!ss)
                tcc_error("static member not found");
            vpop();
            r = ss->r;
            if ((r & VT_VALMASK) < VT_CONST)
                r = (r & ~VT_VALMASK) | VT_LOCAL;
            vset(&ss->type, r, ss->c);
            vtop->sym = ss;
            if (r & VT_SYM)
                vtop->c.i = 0;
        }
        else if (tok == '[') {
            if (!cpp_try_cpp_subscript()) {
                next();
                gexpr();
                gen_op('+');
                indir();
                skip(']');
            }
        }
        else if (tok == '(') {
            SValue ret;
            Sym* sa;
            int nb_args, ret_nregs, ret_align, regsize, variadic;
            int cpp_defer, cpp_i;
            TokenString* p, * p2;

            /* function call  */
            if ((vtop->type.t & VT_BTYPE) != VT_FUNC) {
                /* pointer test (no array accepted) */
                if ((vtop->type.t & (VT_BTYPE | VT_ARRAY)) == VT_PTR) {
                    vtop->type = *pointed_type(&vtop->type);
                    if ((vtop->type.t & VT_BTYPE) != VT_FUNC)
                        goto error_func;
                }
                else {
                error_func:
                    expect("function pointer");
                }
            }
            else {
                vtop->r &= ~VT_LVAL; /* no lvalue */
            }
            /* get return type */
            s = vtop->type.ref;
            /* C++: when the callee belongs to an overload set, defer the
               per-arg conversion so the overload can be resolved from the
               raw argument types (otherwise args get cast to the params
               of the initially bound overload before re-resolution).
               Struct returns (sret slot) and reverse_funcargs keep the
               eager behavior. */
            cpp_defer = tcc_state->cpp && !tcc_state->extern_c
                && !tcc_state->reverse_funcargs
                && (vtop->r & VT_SYM) && vtop->sym
                && (s->type.t & VT_BTYPE) != VT_STRUCT
                && cpp_call_has_overloads(vtop->sym);
            next();
            sa = s->next; /* first parameter */
            nb_args = regsize = 0;
            ret.r2 = VT_CONST;
            /* compute first implicit argument if a structure is returned */
            if ((s->type.t & VT_BTYPE) == VT_STRUCT) {
                variadic = (s->f.func_type == FUNC_ELLIPSIS);
                ret_nregs = gfunc_sret(&s->type, variadic, &ret.type,
                    &ret_align, &regsize);
                if (ret_nregs <= 0) {
                    /* get some space for the returned structure */
                    size = type_size(&s->type, &align);
#ifdef TCC_TARGET_ARM64
                    /* On arm64, a small struct is return in registers.
                       It is much easier to write it to memory if we know
                       that we are allowed to write some extra bytes, so
                       round the allocated space up to a power of 2: */
                    if (size < 16)
                        while (size & (size - 1))
                            size = (size | (size - 1)) + 1;
#endif
                    loc = (loc - size) & -align;
                    ret.type = s->type;
                    ret.r = VT_LOCAL | VT_LVAL;
                    /* pass it as 'int' to avoid structure arg passing
                       problems */
                    vseti(VT_LOCAL, loc);
#ifdef CONFIG_TCC_BCHECK
                    if (tcc_state->do_bounds_check)
                        --loc;
#endif
                    ret.c = vtop->c;
                    if (ret_nregs < 0)
                        vtop--;
                    else
                        nb_args++;
                }
            }
            else {
                ret_nregs = 1;
                ret.type = s->type;
            }

            if (ret_nregs > 0) {
                /* return in register */
                ret.c.i = 0;
                PUT_R_RET(&ret, ret.type.t);
            }

            p = NULL;
            if (tok != ')') {
                r = tcc_state->reverse_funcargs;
                for (;;) {
                    if (r) {
                        skip_or_save_block(&p2);
                        p2->prev = p, p = p2;
                    }
                    else {
                        expr_eq();
                        if (!cpp_defer)
                            gfunc_param_typed(s, sa);
                    }
                    nb_args++;
                    if (sa)
                        sa = sa->next;
                    if (tok == ')')
                        break;
                    skip(',');
                }
            }
            if (!cpp_defer) {
                if (sa && tcc_state->cpp)
                    cpp_apply_default_args(s, &nb_args, &sa);
                else if (sa)
                    tcc_error("too few arguments to function");
            }

            if (p) { /* with reverse_funcargs */
                for (n = 0; p; p = p2, ++n) {
                    p2 = p, sa = s;
                    do {
                        sa = sa->next, p2 = p2->prev;
                    } while (p2 && sa);
                    p2 = p->prev;
                    begin_macro(p, 1), next();
                    expr_eq();
                    gfunc_param_typed(s, sa);
                    end_macro();
                }
                vrev(n);
            }

            next();
            vcheck_cmp(); /* the generators don't like VT_CMP on vtop */
            if (tcc_state->cpp && !tcc_state->extern_c && nb_args >= 0
                && vtop[-nb_args].sym
                && (vtop[-nb_args].type.t & VT_BTYPE) == VT_FUNC) {
                Sym *resolved = cpp_resolve_func_call(vtop[-nb_args].sym->v, nb_args,
                                                      vtop[-nb_args].sym);
                if (resolved) {
                    vtop[-nb_args].sym = resolved;
                    vtop[-nb_args].type.ref = resolved->type.ref;
                    s = resolved->type.ref;
                }
            }
            if (cpp_defer) {
                /* now that the overload is resolved, convert each arg in
                   place (vrotb(nb_args) brings the deepest arg to the top;
                   nb_args rotations restore the original order) */
                sa = s->next;
                for (cpp_i = 0; cpp_i < nb_args; cpp_i++) {
                    vrotb(nb_args);
                    gfunc_param_typed(s, sa);
                    if (sa)
                        sa = sa->next;
                }
                if (sa)
                    cpp_apply_default_args(s, &nb_args, &sa);
            }
            if (cpp_member_this_pending) {
                int na = nb_args;

                if (na == 0) {
                    vpushv(&cpp_member_this);
                    nb_args++;
                } else {
                    /* BUG-12: when the call already pushed a struct-return
                       (sret) pointer as its first arg (na includes it,
                       ret_nregs==0 below), that pointer must stay arg0/RCX
                       and `this` goes to arg1/RDX instead of arg0 - Win64
                       reserves arg0 for the hidden return pointer ahead of
                       the callee's type-list params, `this` included (see
                       gen_function/gfunc_prolog).  Inserting `this` at
                       position 0 unconditionally swapped this<->sret for
                       any method returning a struct too big for registers
                       (feat6a_big_struct, 問題と原因.md 12d/BUG-12). */
                    int has_sret = (s->type.t & VT_BTYPE) == VT_STRUCT && ret_nregs == 0;
                    int insert_at = has_sret ? 1 : 0;
                    int nmove;

                    vtop++;
                    nb_args++;
                    nmove = na - insert_at;
                    /* Shift the args from insert_at onward one slot up to
                     * make room for 'this' at vtop[-nb_args+1+insert_at].
                     * The args currently sit at
                     * [vtop-nb_args+1 .. vtop-1]; move
                     * [vtop-nb_args+1+insert_at .. vtop-1] to
                     * [vtop-nb_args+2+insert_at .. vtop].                */
                    memmove(vtop - nb_args + 2 + insert_at, vtop - nb_args + 1 + insert_at,
                        nmove * sizeof(SValue));
                    vtop[-nb_args + 1 + insert_at] = cpp_member_this;
                }
                cpp_member_this_pending = 0;
            }
            gfunc_call(nb_args);

            if (ret_nregs < 0) {
                vsetc(&ret.type, ret.r, &ret.c);
#ifdef TCC_TARGET_RISCV64
                arch_transfer_ret_regs(1);
#endif
            }
            else {
                /* return value */
                n = ret_nregs;
                while (n > 1) {
                    int rc = reg_classes[ret.r] & ~(RC_INT | RC_FLOAT);
                    /* We assume that when a structure is returned in multiple
                       registers, their classes are consecutive values of the
                       suite s(n) = 2^n */
                    rc <<= --n;
                    for (r = 0; r < NB_REGS; ++r)
                        if (reg_classes[r] & rc)
                            break;
                    vsetc(&ret.type, r, &ret.c);
                }
                vsetc(&ret.type, ret.r, &ret.c);
                vtop->r2 = ret.r2;

                /* handle packed struct return */
                if (((s->type.t & VT_BTYPE) == VT_STRUCT) && ret_nregs) {
                    int addr, offset;

                    size = type_size(&s->type, &align);
                    /* We're writing whole regs often, make sure there's enough
                       space.  Assume register size is power of 2.  */
                    size = (size + regsize - 1) & -regsize;
                    if (ret_align > align)
                        align = ret_align;
                    loc = (loc - size) & -align;
                    addr = loc;
                    offset = 0;
                    for (;;) {
                        vset(&ret.type, VT_LOCAL | VT_LVAL, addr + offset);
                        vswap();
                        vstore();
                        vtop--;
                        if (--ret_nregs == 0)
                            break;
                        offset += regsize;
                    }
                    vset(&s->type, VT_LOCAL | VT_LVAL, addr);
                }

                /* Promote char/short return values. This is matters only
                   for calling function that were not compiled by TCC and
                   only on some architectures.  For those where it doesn't
                   matter we expect things to be already promoted to int,
                   but not larger.  */
                t = s->type.t & VT_BTYPE;
                if (t == VT_BYTE || t == VT_SHORT || t == VT_BOOL) {
#ifdef PROMOTE_RET
                    vtop->r |= BFVAL(VT_MUSTCAST, 1);
#else
                    vtop->type.t = VT_INT;
#endif
                }
                if (s->type.t & VT_REFERENCE)
                    indir();
            }
            if (s->f.func_noreturn) {
                if (debug_modes)
                    tcc_tcov_block_end(tcc_state, -1);
                CODE_OFF();
            }
        }
        else {
            break;
        }
    }
}

#ifndef precedence_parser /* original top-down parser */

static void expr_prod(void)
{
    int t;

    unary();
    while ((t = tok) == '*' || t == '/' || t == '%') {
        next();
        unary();
        gen_op(t);
    }
}

static void expr_sum(void)
{
    int t;

    expr_prod();
    while ((t = tok) == '+' || t == '-') {
        next();
        expr_prod();
        gen_op(t);
    }
}

static void expr_shift(void)
{
    int t;

    expr_sum();
    while ((t = tok) == TOK_SHL || t == TOK_SAR) {
        next();
        expr_sum();
        gen_op(t);
    }
}

static void expr_cmp(void)
{
    int t;

    expr_shift();
    while (((t = tok) >= TOK_ULE && t <= TOK_GT) ||
        t == TOK_ULT || t == TOK_UGE) {
        next();
        expr_shift();
        gen_op(t);
    }
}

static void expr_cmpeq(void)
{
    int t;

    expr_cmp();
    while ((t = tok) == TOK_EQ || t == TOK_NE) {
        next();
        expr_cmp();
        gen_op(t);
    }
}

static void expr_and(void)
{
    expr_cmpeq();
    while (tok == '&') {
        next();
        expr_cmpeq();
        gen_op('&');
    }
}

static void expr_xor(void)
{
    expr_and();
    while (tok == '^') {
        next();
        expr_and();
        gen_op('^');
    }
}

static void expr_or(void)
{
    expr_xor();
    while (tok == '|') {
        next();
        expr_xor();
        gen_op('|');
    }
}

static void expr_landor(int op);

static void expr_land(void)
{
    expr_or();
    if (tok == TOK_LAND)
        expr_landor(tok);
}

static void expr_lor(void)
{
    expr_land();
    if (tok == TOK_LOR)
        expr_landor(tok);
}

# define expr_landor_next(op) op == TOK_LAND ? expr_or() : expr_land()
#else /* defined precedence_parser */
# define expr_landor_next(op) unary(), expr_infix(precedence(op) + 1)
# define expr_lor() unary(), expr_infix(1)

static int precedence(int tok)
{
    switch (tok) {
    case TOK_LOR: return 1;
    case TOK_LAND: return 2;
    case '|': return 3;
    case '^': return 4;
    case '&': return 5;
    case TOK_EQ: case TOK_NE: return 6;
    relat: case TOK_ULT: case TOK_UGE: return 7;
    case TOK_SHL: case TOK_SAR: return 8;
    case '+': case '-': return 9;
    case '*': case '/': case '%': return 10;
    default:
        if (tok >= TOK_ULE && tok <= TOK_GT)
            goto relat;
        return 0;
    }
}
static unsigned char prec[256];
static void init_prec(void)
{
    int i;
    for (i = 0; i < 256; i++)
        prec[i] = precedence(i);
}
#define precedence(i) ((unsigned)i < 256 ? prec[i] : 0)

static void expr_landor(int op);

static void expr_infix(int p)
{
    int t = tok, p2;
    while ((p2 = precedence(t)) >= p) {
        if (t == TOK_LOR || t == TOK_LAND) {
            expr_landor(t);
        }
        else {
            next();
            unary();
            if (precedence(tok) > p2)
                expr_infix(p2 + 1);
            if (!cpp_try_member_binop(t))
                if (!cpp_try_free_binop(t))
                    gen_op(t);
        }
        t = tok;
    }
}
#endif

/* Assuming vtop is a value used in a conditional context
   (i.e. compared with zero) return 0 if it's false, 1 if
   true and -1 if it can't be statically determined.  */
static int condition_3way(void)
{
    int c = -1;
    if ((vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST &&
        (!(vtop->r & VT_SYM) || !vtop->sym->a.weak)) {
        vdup();
        gen_cast_s(VT_BOOL);
        c = vtop->c.i;
        vpop();
    }
    return c;
}

static void expr_landor(int op)
{
    int t = 0, cc = 1, f = 0, i = op == TOK_LAND, c;
    for (;;) {
        c = f ? i : condition_3way();
        if (c < 0)
            save_regs(1), cc = 0;
        else if (c != i)
            nocode_wanted++, f = 1;
        if (tok != op)
            break;
        if (c < 0)
            t = gvtst(i, t);
        else
            vpop();
        next();
        expr_landor_next(op);
    }
    if (cc || f) {
        vpop();
        vpushi(i ^ f);
        gsym(t);
        nocode_wanted -= f;
    }
    else {
        gvtst_set(i, t);
    }
}

static int is_cond_bool(SValue* sv)
{
    if ((sv->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST
        && (sv->type.t & VT_BTYPE) == VT_INT)
        return (unsigned)sv->c.i < 2;
    if (sv->r == VT_CMP)
        return 1;
    return 0;
}

static void expr_cond(void)
{
    int tt, u, r1, r2, rc, t1, t2, islv, c, g;
    SValue sv;
    CType type;

    expr_lor();
    if (tok == '?') {
        next();
        c = condition_3way();
        g = (tok == ':' && gnu_ext);
        tt = 0;
        if (!g) {
            if (c < 0) {
                save_regs(1);
                tt = gvtst(1, 0);
            }
            else {
                vpop();
            }
        }
        else if (c < 0) {
            /* needed to avoid having different registers saved in
               each branch */
            save_regs(1);
            gv_dup();
            tt = gvtst(0, 0);
        }

        if (c == 0)
            nocode_wanted++;
        if (!g)
            gexpr();

        if ((vtop->type.t & VT_BTYPE) == VT_FUNC)
            mk_pointer(&vtop->type);
        sv = *vtop; /* save value to handle it later */
        vtop--; /* no vpop so that FP stack is not flushed */

        if (g) {
            u = tt;
        }
        else if (c < 0) {
            u = gjmp(0);
            gsym(tt);
        }
        else
            u = 0;

        if (c == 0)
            nocode_wanted--;
        if (c == 1)
            nocode_wanted++;
        skip(':');
        expr_cond();

        if ((vtop->type.t & VT_BTYPE) == VT_FUNC)
            mk_pointer(&vtop->type);

        /* cast operands to correct type according to ISOC rules */
        if (!combine_types(&type, &sv, vtop, '?'))
            type_incompatibility_error(&sv.type, &vtop->type,
                "type mismatch in conditional expression (have '%s' and '%s')");

        if (c < 0 && is_cond_bool(vtop) && is_cond_bool(&sv)) {
            /* optimize "if (f ? a > b : c || d) ..." for example, where normally
               "a < b" and "c || d" would be forced to "(int)0/1" first, whereas
               this code jumps directly to the if's then/else branches. */
            t1 = gvtst(0, 0);
            t2 = gjmp(0);
            gsym(u);
            vpushv(&sv);
            /* combine jump targets of 2nd op with VT_CMP of 1st op */
            gvtst_set(0, t1);
            gvtst_set(1, t2);
            gen_cast(&type);
            //  tcc_warning("two conditions expr_cond");
            return;
        }

        /* keep structs lvalue by transforming `(expr ? a : b)` to `*(expr ? &a : &b)` so
           that `(expr ? a : b).mem` does not error  with "lvalue expected" */
        islv = (vtop->r & VT_LVAL) && (sv.r & VT_LVAL) && VT_STRUCT == (type.t & VT_BTYPE);

        /* now we convert second operand */
        if (c != 1) {
            gen_cast(&type);
            if (islv) {
                mk_pointer(&vtop->type);
                gaddrof();
            }
            else if (VT_STRUCT == (vtop->type.t & VT_BTYPE))
                gaddrof();
        }

        rc = RC_TYPE(type.t);
        /* for long longs, we use fixed registers to avoid having
           to handle a complicated move */
        if (USING_TWO_WORDS(type.t))
            rc = RC_RET(type.t);

        tt = r2 = 0;
        if (c < 0) {
            r2 = gv(rc);
            tt = gjmp(0);
        }
        gsym(u);
        if (c == 1)
            nocode_wanted--;

        /* this is horrible, but we must also convert first
           operand */
        if (c != 0) {
            *vtop = sv;
            gen_cast(&type);
            if (islv) {
                mk_pointer(&vtop->type);
                gaddrof();
            }
            else if (VT_STRUCT == (vtop->type.t & VT_BTYPE))
                gaddrof();
        }

        if (c < 0) {
            r1 = gv(rc);
            move_reg(r2, r1, islv ? VT_PTR : type.t);
            vtop->r = r2;
            gsym(tt);
        }

        if (islv)
            indir();
    }
}

static void expr_eq(void)
{
    int t;

    expr_cond();
    if ((t = tok) == '=' || TOK_ASSIGN(t)) {
        test_lvalue();
        next();
        if (t == '=') {
            expr_eq();
            /* C++: member operator= (falls back to plain store) */
            if (cpp_try_member_binop(t))
                return;
        }
        else {
            /* C++: struct compound assignment via operator+= etc.
               Plain C structs reject gen_op below, so no regression. */
            if (tcc_state->cpp && (vtop->type.t & VT_BTYPE) == VT_STRUCT) {
                expr_eq();
                if (!cpp_try_member_binop(t))
                    if (!cpp_try_free_binop(t))
                        tcc_error("no operator%s defined for this type",
                                  get_tok_str(t, NULL));
                return;
            }
            vdup();
            expr_eq();
            gen_op(TOK_ASSIGN_OP(t));
        }
        vstore();
    }
}

ST_FUNC void gexpr(void)
{
    expr_eq();
    if (tok == ',') {
        do {
            vpop();
            next();
            expr_eq();
        } while (tok == ',');

        /* convert array & function to pointer */
        convert_parameter_type(&vtop->type);

        /* make builtin_constant_p((1,2)) return 0 (like on gcc) */
        if ((vtop->r & VT_VALMASK) == VT_CONST && nocode_wanted && !CONST_WANTED)
            gv(RC_TYPE(vtop->type.t));
    }
}

/* parse a constant expression and return value in vtop.  */
static void expr_const1(void)
{
    nocode_wanted += CONST_WANTED_BIT;
    expr_cond();
    nocode_wanted -= CONST_WANTED_BIT;
}

/* parse an integer constant and return its value. */
static inline int64_t expr_const64(void)
{
    int64_t c;
    expr_const1();
    if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM | VT_NONCONST)) != VT_CONST)
        expect("constant expression");
    c = vtop->c.i;
    vpop();
    return c;
}

/* parse an integer constant and return its value.
   Complain if it doesn't fit 32bit (signed or unsigned).  */
ST_FUNC int expr_const(void)
{
    int c;
    int64_t wc = expr_const64();
    c = wc;
    if (c != wc && (unsigned)c != wc)
        tcc_error("�萔��32�r�b�g�𒴂��Ă��܂�");
    return c;
}

/* ------------------------------------------------------------------------- */
/* return from function */

#ifndef TCC_TARGET_ARM64
static void gfunc_return(CType* func_type)
{
    if ((func_type->t & VT_BTYPE) == VT_STRUCT) {
        CType type, ret_type;
        int ret_align, ret_nregs, regsize;
        ret_nregs = gfunc_sret(func_type, func_var, &ret_type,
            &ret_align, &regsize);
        if (ret_nregs < 0) {
#ifdef TCC_TARGET_RISCV64
            arch_transfer_ret_regs(0);
#endif
        }
        else if (0 == ret_nregs) {
            /* if returning structure, must copy it to implicit
               first pointer arg location */
            type = *func_type;
            mk_pointer(&type);
            vset(&type, VT_LOCAL | VT_LVAL, func_vc);
            indir();
            vswap();
            /* copy structure value to pointer */
            vstore();
        }
        else {
            /* returning structure packed into registers */
            int size, addr, align, rc, n;
            size = type_size(func_type, &align);
            if ((align & (ret_align - 1))
                && ((vtop->r & VT_VALMASK) < VT_CONST /* pointer to struct */
                    || (vtop->c.i & (ret_align - 1))
                    )) {
                loc = (loc - size) & -ret_align;
                addr = loc;
                type = *func_type;
                vset(&type, VT_LOCAL | VT_LVAL, addr);
                vswap();
                vstore();
                vpop();
                vset(&ret_type, VT_LOCAL | VT_LVAL, addr);
            }
            vtop->type = ret_type;
            rc = RC_RET(ret_type.t);
            //printf("struct return: n:%d t:%02x rc:%02x\n", ret_nregs, ret_type.t, rc);
            for (n = ret_nregs; --n > 0;) {
                vdup();
                gv(rc);
                vswap();
                incr_offset(regsize);
                /* We assume that when a structure is returned in multiple
                   registers, their classes are consecutive values of the
                   suite s(n) = 2^n */
                rc <<= 1;
            }
            gv(rc);
            vtop -= ret_nregs - 1;
        }
    }
    else if (func_type->t & VT_REFERENCE) {
        if (vtop->r & VT_LVAL) {
            gaddrof();
            if (!(vtop->type.t & VT_REFERENCE))
                mk_pointer(&vtop->type);
        }
        gv(RC_RET(VT_PTR));
    }
    else {
        gv(RC_RET(func_type->t));
    }
    vtop--; /* NOT vpop() because on x86 it would flush the fp stack */
}
#endif

static void check_func_return(void)
{
    if ((func_vt.t & VT_BTYPE) == VT_VOID)
        return;
    if (!strcmp(funcname, "main")
        && (func_vt.t & VT_BTYPE) == VT_INT) {
        /* main returns 0 by default */
        vpushi(0);
        gen_assign_cast(&func_vt);
        gfunc_return(&func_vt);
    }
    else {
        tcc_warning("�֐����l��Ԃ��Ȃ��\��������܂�: '%s'", funcname);
    }
}

/* ------------------------------------------------------------------------- */
/* switch/case */

static int case_cmp(uint64_t a, uint64_t b)
{
    if (cur_switch->sv.type.t & VT_UNSIGNED)
        return a < b ? -1 : a > b;
    else
        return (int64_t)a < (int64_t)b ? -1 : (int64_t)a >(int64_t)b;
}

static int case_cmp_qs(const void* pa, const void* pb)
{
    return case_cmp((*(struct case_t**)pa)->v1, (*(struct case_t**)pb)->v1);
}

static void case_sort(struct switch_t* sw)
{
    struct case_t** p;
    if (sw->n < 2)
        return;
    qsort(sw->p, sw->n, sizeof * sw->p, case_cmp_qs);
    p = sw->p;
    while (p < sw->p + sw->n - 1) {
        if (case_cmp(p[0]->v2, p[1]->v1) >= 0) {
            int l1 = p[0]->line, l2 = p[1]->line;
            /* using special format "%i:..." to show specific line */
            tcc_error("%i:case �̒l���d�����Ă��܂�", l1 > l2 ? l1 : l2);
        }
        else if (p[0]->v2 + 1 == p[1]->v1 && p[0]->ind == p[1]->ind) {
            /* treat "case 1: case 2: case 3:" like "case 1 ... 3: */
            p[1]->v1 = p[0]->v1;
            tcc_free(p[0]);
            memmove(p, p + 1, (--sw->n - (p - sw->p)) * sizeof * p);
        }
        else
            ++p;
    }
}

static int gcase(struct case_t** base, int len, int dsym)
{
    struct case_t* p;
    int t, l2, e;

    t = vtop->type.t & VT_BTYPE;
    if (t != VT_LLONG)
        t = VT_INT;
    while (len) {
        /* binary search while len > 8, else linear */
        l2 = len > 8 ? len / 2 : 0;
        p = base[l2];
        vdup(), vpush64(t, p->v2);
        if (l2 == 0 && p->v1 == p->v2) {
            gen_op(TOK_EQ); /* jmp to case when equal */
            gsym_addr(gvtst(0, 0), p->ind);
        }
        else {
            /* case v1 ... v2 */
            gen_op(TOK_GT); /* jmp over when > V2 */
            if (len == 1) /* last case test jumps to default when false */
                dsym = gvtst(0, dsym), e = 0;
            else
                e = gvtst(0, 0);
            vdup(), vpush64(t, p->v1);
            gen_op(TOK_GE); /* jmp to case when >= V1 */
            gsym_addr(gvtst(0, 0), p->ind);
            dsym = gcase(base, l2, dsym);
            gsym(e);
        }
        ++l2, base += l2, len -= l2;
    }
    /* jump automagically will suppress more jumps */
    return gjmp(dsym);
}

static void end_switch(void)
{
    struct switch_t* sw = cur_switch;
    dynarray_reset(&sw->p, &sw->n);
    cur_switch = sw->prev;
    tcc_free(sw);
}

/* ------------------------------------------------------------------------- */
/* __attribute__((cleanup(fn))) */

static void try_call_scope_cleanup(Sym* stop)
{
    Sym* cls = cur_scope->cl.s;

    for (; cls != stop; cls = cls->next) {
        Sym* fs = cls->cleanup_func;
        Sym* vs = cls->prev_tok;

        vpushsym(&fs->type, fs);
        vset(&vs->type, vs->r, vs->c);
        vtop->sym = vs;
        mk_pointer(&vtop->type);
        gaddrof();
        gfunc_call(1);
    }
}

static void try_call_cleanup_goto(Sym* cleanupstate)
{
    Sym* oc, * cc;
    int ocd, ccd;

    if (!cur_scope->cl.s)
        return;

    /* search NCA of both cleanup chains given parents and initial depth */
    ocd = cleanupstate ? cleanupstate->v & ~SYM_FIELD : 0;
    for (ccd = cur_scope->cl.n, oc = cleanupstate; ocd > ccd; --ocd, oc = oc->next)
        ;
    for (cc = cur_scope->cl.s; ccd > ocd; --ccd, cc = cc->next)
        ;
    for (; cc != oc; cc = cc->next, oc = oc->next, --ccd)
        ;

    try_call_scope_cleanup(cc);
}

/* call 'func' for each __attribute__((cleanup(func))) */
static void block_cleanup(struct scope* o)
{
    int jmp = 0;
    Sym* g, ** pg;
    for (pg = &pending_gotos; (g = *pg) && g->c > o->cl.n;) {
        if (g->prev_tok->r & LABEL_FORWARD) {
            Sym* pcl = g->next;
            if (!jmp)
                jmp = gjmp(0);
            gsym(pcl->jnext);
            try_call_scope_cleanup(o->cl.s);
            pcl->jnext = gjmp(0);
            if (!o->cl.n)
                goto remove_pending;
            g->c = o->cl.n;
            pg = &g->prev;
        }
        else {
        remove_pending:
            *pg = g->prev;
            sym_free(g);
        }
    }
    gsym(jmp);
    try_call_scope_cleanup(o->cl.s);
}

/* ------------------------------------------------------------------------- */
/* VLA */

static void vla_restore(int loc)
{
    if (loc)
        gen_vla_sp_restore(loc);
}

static void vla_leave(struct scope* o)
{
    struct scope* c = cur_scope, * v = NULL;
    for (; c != o && c; c = c->prev)
        if (c->vla.num)
            v = c;
    if (v)
        vla_restore(v->vla.locorig);
}

/* ------------------------------------------------------------------------- */
/* local scopes */

static void new_scope(struct scope* o)
{
    /* copy and link previous scope */
    *o = *cur_scope;
    o->prev = cur_scope;
    cur_scope = o;
    cur_scope->vla.num = 0;

    /* record local declaration stack position */
    o->lstk = local_stack;
    o->llstk = local_label_stack;
    ++local_scope;
}

static void prev_scope(struct scope* o, int is_expr)
{
    vla_leave(o->prev);

    if (o->cl.s != o->prev->cl.s)
        block_cleanup(o->prev);

    /* pop locally defined labels */
    label_pop(&local_label_stack, o->llstk, is_expr);

    /* In the is_expr case (a statement expression is finished here),
       vtop might refer to symbols on the local_stack.  Either via the
       type or via vtop->sym.  We can't pop those nor any that in turn
       might be referred to.  To make it easier we don't roll back
       any symbols in that case; some upper level call to block() will
       do that.  We do have to remove such symbols from the lookup
       tables, though.  sym_pop will do that.  */

       /* pop locally defined symbols */
    if (tcc_state->cpp && !is_expr)
        cpp_call_scope_dtors(o->lstk);
    pop_local_syms(o->lstk, is_expr);
    cur_scope = o->prev;
    --local_scope;
}

/* leave a scope via break/continue(/goto) */
static void leave_scope(struct scope* o)
{
    if (!o)
        return;
    try_call_scope_cleanup(o->cl.s);
    vla_leave(o);
}

/* short versiona for scopes with 'if/do/while/switch' which can
   declare only types (of struct/union/enum) */
static void new_scope_s(struct scope* o)
{
    o->lstk = local_stack;
    ++local_scope;
}

static void prev_scope_s(struct scope* o)
{
    sym_pop(&local_stack, o->lstk, 0);
    --local_scope;
}

/* ------------------------------------------------------------------------- */
/* call block from 'for do while' loops */

static void lblock(int* bsym, int* csym)
{
    struct scope* lo = loop_scope, * co = cur_scope;
    int* b = co->bsym, * c = co->csym;
    if (csym) {
        co->csym = csym;
        loop_scope = co;
    }
    co->bsym = bsym;
    block(0);
    co->bsym = b;
    if (csym) {
        co->csym = c;
        loop_scope = lo;
    }
}

static void block(int flags)
{
    int a, b, c, d, e, t;
    struct scope o;
    Sym* s;

    if (flags & STMT_EXPR) {
        /* default return value is (void) */
        vpushi(0);
        vtop->type.t = VT_VOID;
    }

again:
    t = tok;
    /* If the token carries a value, next() might destroy it. Only with
       invalid code such as f(){"123"4;} */
    if (TOK_HAS_VALUE(t))
        goto expr;
    next();

    if (debug_modes)
        tcc_tcov_check_line(tcc_state, 0), tcc_tcov_block_begin(tcc_state);

    if (t == TOK_IF) {
        new_scope_s(&o);
        skip('(');
        gexpr();
        skip(')');
        a = gvtst(1, 0);
        block(0);
        if (tok == TOK_ELSE) {
            d = gjmp(0);
            gsym(a);
            next();
            block(0);
            gsym(d); /* patch else jmp */
        }
        else {
            gsym(a);
        }
        prev_scope_s(&o);

    }
    else if (t == TOK_WHILE) {
        new_scope_s(&o);
        d = gind();
        skip('(');
        gexpr();
        skip(')');
        a = gvtst(1, 0);
        b = 0;
        lblock(&a, &b);
        gjmp_addr(d);
        gsym_addr(b, d);
        gsym(a);
        prev_scope_s(&o);

    }
    else if (t == '{') {
        if (debug_modes)
            tcc_debug_stabn(tcc_state, N_LBRAC, ind - func_ind);
        new_scope(&o);

        /* handle local labels declarations */
        while (tok == TOK_LABEL) {
            do {
                next();
                if (tok < TOK_UIDENT)
                    expect("label identifier");
                label_push(&local_label_stack, tok, LABEL_DECLARED);
                next();
            } while (tok == ',');
            skip(';');
        }

        while (tok != '}') {
            if (cpp_unget_scoped_expr()) {
                gexpr();
                vpop();
                skip(';');
                continue;
            }
            decl(VT_LOCAL);
            if (tok != '}') {
                if (flags & STMT_EXPR)
                    vpop();
                block(flags | STMT_COMPOUND);
            }
        }

        prev_scope(&o, flags & STMT_EXPR);
        if (debug_modes)
            tcc_debug_stabn(tcc_state, N_RBRAC, ind - func_ind);
        if (local_scope)
            next();
        else if (!nocode_wanted)
            check_func_return();

    }
    else if (t == TOK_RETURN) {
        b = (func_vt.t & VT_BTYPE) != VT_VOID;
        if (tok != ';') {
            gexpr();
            if (b) {
                gen_assign_cast(&func_vt);
            }
            else {
                if (vtop->type.t != VT_VOID)
                    tcc_warning("void �^�̊֐����l��Ԃ��Ă��܂�");
                vtop--;
            }
        }
        else if (b) {
            tcc_warning("�l�̂Ȃ� 'return' �ł�");
            b = 0;
        }
        leave_scope(root_scope);
        if (b)
            gfunc_return(&func_vt);
        skip(';');
        /* jump unless last stmt in top-level block */
        if (tok != '}' || local_scope != 1)
            rsym = gjmp(rsym);
        if (debug_modes)
            tcc_tcov_block_end(tcc_state, -1);
        CODE_OFF();

    }
    else if (t == TOK_BREAK) {
        /* compute jump */
        if (!cur_scope->bsym)
            tcc_error("break �͂����ł͎g�p�ł��܂���");
        if (cur_switch && cur_scope->bsym == cur_switch->bsym)
            leave_scope(cur_switch->scope);
        else
            leave_scope(loop_scope);
        *cur_scope->bsym = gjmp(*cur_scope->bsym);
        skip(';');

    }
    else if (t == TOK_CONTINUE) {
        /* compute jump */
        if (!cur_scope->csym)
            tcc_error("continue �͂����ł͎g�p�ł��܂���");
        leave_scope(loop_scope);
        *cur_scope->csym = gjmp(*cur_scope->csym);
        skip(';');

    }
    else if (t == TOK_FOR) {
        new_scope(&o);

        skip('(');
        if (tok != ';') {
            /* c99 for-loop init decl? */
            if (!decl(VT_JMP)) {
                /* no, regular for-loop init expr */
                gexpr();
                vpop();
            }
        }
        skip(';');
        a = b = 0;
        c = d = gind();
        if (tok != ';') {
            gexpr();
            a = gvtst(1, 0);
        }
        skip(';');
        if (tok != ')') {
            e = gjmp(0);
            d = gind();
            gexpr();
            vpop();
            gjmp_addr(c);
            gsym(e);
        }
        skip(')');
        lblock(&a, &b);
        gjmp_addr(d);
        gsym_addr(b, d);
        gsym(a);
        prev_scope(&o, 0);

    }
    else if (t == TOK_DO) {
        new_scope_s(&o);
        a = b = 0;
        d = gind();
        lblock(&a, &b);
        gsym(b);
        skip(TOK_WHILE);
        skip('(');
        gexpr();
        skip(')');
        skip(';');
        c = gvtst(0, 0);
        gsym_addr(c, d);
        gsym(a);
        prev_scope_s(&o);

    }
    else if (t == TOK_SWITCH) {
        struct switch_t* sw;

        sw = tcc_mallocz(sizeof * sw);
        sw->bsym = &a;
        sw->scope = cur_scope;
        sw->prev = cur_switch;
        sw->nocode_wanted = nocode_wanted;
        cur_switch = sw;

        new_scope_s(&o);
        skip('(');
        gexpr();
        skip(')');
        if (!is_integer_btype(vtop->type.t & VT_BTYPE))
            tcc_error("switch �̒l�������ł͂���܂���");
        sw->sv = *vtop--; /* save switch value */
        a = 0;
        b = gjmp(0); /* jump to first case */
        lblock(&a, NULL);
        a = gjmp(a); /* add implicit break */
        /* case lookup */
        gsym(b);
        prev_scope_s(&o);
        if (sw->nocode_wanted)
            goto skip_switch;
        case_sort(sw);
        sw->bsym = NULL; /* marker for 32bit:gen_opl() */
        vpushv(&sw->sv);
        gv(RC_INT);
        d = gcase(sw->p, sw->n, 0);
        vpop();
        if (sw->def_sym)
            gsym_addr(d, sw->def_sym);
        else
            gsym(d);
    skip_switch:
        /* break label */
        gsym(a);
        end_switch();

    }
    else if (t == TOK_CASE) {
        struct case_t* cr;
        if (!cur_switch)
            expect("switch");
        cr = tcc_malloc(sizeof(struct case_t));
        dynarray_add(&cur_switch->p, &cur_switch->n, cr);
        t = cur_switch->sv.type.t;
        cr->v1 = cr->v2 = value64(expr_const64(), t);
        if (tok == TOK_DOTS && gnu_ext) {
            next();
            cr->v2 = value64(expr_const64(), t);
            if (case_cmp(cr->v2, cr->v1) < 0)
                tcc_warning("��� case �͈͂ł�");
        }
        /* case and default are unreachable from a switch under nocode_wanted */
        if (!cur_switch->nocode_wanted)
            cr->ind = gind();
        cr->line = file->line_num;
        skip(':');
        goto block_after_label;

    }
    else if (t == TOK_DEFAULT) {
        if (!cur_switch)
            expect("switch");
        if (cur_switch->def_sym)
            tcc_error("'default' ���������܂�");
        cur_switch->def_sym = cur_switch->nocode_wanted ? -1 : gind();
        skip(':');
        goto block_after_label;

    }
    else if (t == TOK_GOTO) {
        vla_restore(cur_scope->vla.locorig);
        if (tok == '*' && gnu_ext) {
            /* computed goto */
            next();
            gexpr();
            if ((vtop->type.t & VT_BTYPE) != VT_PTR)
                expect("pointer");
            ggoto();

        }
        else if (tok >= TOK_UIDENT) {
            s = label_find(tok);
            /* put forward definition if needed */
            if (!s)
                s = label_push(&global_label_stack, tok, LABEL_FORWARD);
            else if (s->r == LABEL_DECLARED)
                s->r = LABEL_FORWARD;

            if (s->r & LABEL_FORWARD) {
                /* start new goto chain for cleanups, linked via label->next */
                if (cur_scope->cl.s && !nocode_wanted) {
                    sym_push2(&pending_gotos, SYM_FIELD, 0, cur_scope->cl.n);
                    pending_gotos->prev_tok = s;
                    s = sym_push2(&s->next, SYM_FIELD, 0, 0);
                    pending_gotos->next = s;
                }
                s->jnext = gjmp(s->jnext);
            }
            else {
                try_call_cleanup_goto(s->cleanupstate);
                gjmp_addr(s->jind);
            }
            next();

        }
        else {
            expect("label identifier");
        }
        skip(';');

    }
    else if (t == TOK_ASM1 || t == TOK_ASM2 || t == TOK_ASM3) {
        asm_instr();

    }
    else {
        if (tok == ':' && t >= TOK_UIDENT) {
            /* label case */
            next();
            s = label_find(t);
            if (s) {
                if (s->r == LABEL_DEFINED)
                    tcc_error("���x�� '%s' ���d�����Ă��܂�", get_tok_str(s->v, NULL));
                s->r = LABEL_DEFINED;
                if (s->next) {
                    Sym* pcl; /* pending cleanup goto */
                    for (pcl = s->next; pcl; pcl = pcl->prev)
                        gsym(pcl->jnext);
                    sym_pop(&s->next, NULL, 0);
                }
                else
                    gsym(s->jnext);
            }
            else {
                s = label_push(&global_label_stack, t, LABEL_DEFINED);
            }
            s->jind = gind();
            s->cleanupstate = cur_scope->cl.s;

        block_after_label:
            {
                /* Accept attributes after labels (e.g. 'unused') */
                AttributeDef ad_tmp;
                parse_attribute(&ad_tmp);
            }
            if (debug_modes)
                tcc_tcov_reset_ind(tcc_state);
            vla_restore(cur_scope->vla.loc);

            if (tok != '}') {
                if (0 == (flags & STMT_COMPOUND))
                    goto again;
                /* C23: insert implicit null-statement whithin compound statement */
            }
            else {
                /* we accept this, but it is a mistake */
                tcc_warning_c(warn_all)("�������̖����ł̃��x���̎g�p�͔񐄏��ł�");
            }
        }
        else {
            /* expression case */
            if (t != ';') {
                unget_tok(t);
            expr:
                if (flags & STMT_EXPR) {
                    vpop();
                    gexpr();
                }
                else {
                    gexpr();
                    vpop();
                }
                skip(';');
            }
        }
    }

    if (debug_modes)
        tcc_tcov_check_line(tcc_state, 0), tcc_tcov_block_end(tcc_state, 0);
}

/* This skips over a stream of tokens containing balanced {} and ()
   pairs, stopping at outer ',' ';' and '}' (or matching '}' if we started
   with a '{').  If STR then allocates and stores the skipped tokens
   in *STR.  This doesn't check if () and {} are nested correctly,
   i.e. "({)}" is accepted.  */
static void skip_or_save_block(TokenString** str)
{
    int braces = tok == '{';
    int level = 0;
    if (str)
        *str = tok_str_alloc();

    while (1) {
        int t = tok;
        if (level == 0
            && (t == ','
                || t == ';'
                || t == '}'
                || t == ')'
                || t == ']'))
            break;
        if (tok == TOK_EOF) {
            if (str || level > 0)
                tcc_error("�t�@�C���̏I�[�ɗ\���������B���܂���");
            else
                break;
        }
        if (str)
            tok_str_add_tok(*str);
        next();
        if (t == '{' || t == '(' || t == '[') {
            level++;
        }
        else if (t == '}' || t == ')' || t == ']') {
            level--;
            if (level == 0 && braces && t == '}')
                break;
        }
    }
    if (str)
        tok_str_add(*str, TOK_EOF);
}

#define EXPR_CONST 1
#define EXPR_ANY   2

static void parse_init_elem(int expr_type)
{
    int saved_global_expr;
    switch (expr_type) {
    case EXPR_CONST:
        /* compound literals must be allocated globally in this case */
        saved_global_expr = global_expr;
        global_expr = 1;
        expr_const1();
        global_expr = saved_global_expr;
        /* NOTE: symbols are accepted, as well as lvalue for anon symbols
       (compound literals).  */
        if (((vtop->r & (VT_VALMASK | VT_LVAL)) != VT_CONST
            && ((vtop->r & (VT_SYM | VT_LVAL)) != (VT_SYM | VT_LVAL)
                || vtop->sym->v < SYM_FIRST_ANOM))
#ifdef TCC_TARGET_PE
            || ((vtop->r & VT_SYM) && vtop->sym->a.dllimport)
#endif
            )
            tcc_error("�������q�̗v�f���萔�ł͂���܂���");
        break;
    case EXPR_ANY:
        expr_eq();
        break;
    }
}

#if 1
static void init_assert(init_params* p, int offset)
{
    if (p->sec ? !NODATA_WANTED && offset > p->sec->data_offset
        : !nocode_wanted && offset > p->local_offset)
        tcc_internal_error("initializer overflow");
}
#else
#define init_assert(sec, offset)
#endif

/* put zeros for variable based init */
static void init_putz(init_params* p, unsigned long c, int size)
{
    init_assert(p, c + size);
    if (p->sec) {
        /* nothing to do because globals are already set to zero */
    }
    else {
        vpush_helper_func(TOK_memset);
        vseti(VT_LOCAL, c);
        vpushi(0);
        vpushs(size);
#if defined TCC_TARGET_ARM && defined TCC_ARM_EABI
        vswap();  /* using __aeabi_memset(void*, size_t, int) */
#endif
        gfunc_call(3);
    }
}

#define DIF_FIRST     1
#define DIF_SIZE_ONLY 2
#define DIF_HAVE_ELEM 4
#define DIF_CLEAR     8

/* delete relocations for specified range c ... c + size. Unfortunatly
   in very special cases, relocations may occur unordered */
static void decl_design_delrels(Section* sec, int c, int size)
{
    ElfW_Rel* rel, * rel2, * rel_end;
    if (!sec || !sec->reloc)
        return;
    rel = rel2 = (ElfW_Rel*)sec->reloc->data;
    rel_end = (ElfW_Rel*)(sec->reloc->data + sec->reloc->data_offset);
    while (rel < rel_end) {
        if (rel->r_offset >= c && rel->r_offset < c + size) {
            sec->reloc->data_offset -= sizeof * rel;
        }
        else {
            if (rel2 != rel)
                memcpy(rel2, rel, sizeof * rel);
            ++rel2;
        }
        ++rel;
    }
}

static void decl_design_flex(init_params* p, Sym* ref, int index)
{
    if (ref == p->flex_array_ref) {
        if (index >= ref->c)
            ref->c = index + 1;
    }
    else if (ref->c < 0)
        tcc_error("���̕����ł͉ϒ��z��̃T�C�Y��0�ł�");
}

/* t is the array or struct type. c is the array or struct
   address. cur_field is the pointer to the current
   field, for arrays the 'c' member contains the current start
   index.  'flags' is as in decl_initializer.
   'al' contains the already initialized length of the
   current container (starting at c).  This returns the new length of that.  */
static int decl_designator(init_params* p, CType* type, unsigned long c,
    Sym** cur_field, int flags, int al)
{
    Sym* s, * f;
    int index, index_last, align, l, nb_elems, elem_size;
    unsigned long corig = c;

    elem_size = 0;
    nb_elems = 1;

    if (flags & DIF_HAVE_ELEM)
        goto no_designator;

    if (gnu_ext && tok >= TOK_UIDENT) {
        l = tok, next();
        if (tok == ':')
            goto struct_field;
        unget_tok(l);
    }

    /* NOTE: we only support ranges for last designator */
    while (nb_elems == 1 && (tok == '[' || tok == '.')) {
        if (tok == '[') {
            if (!(type->t & VT_ARRAY))
                expect("array type");
            next();
            index = index_last = expr_const();
            if (tok == TOK_DOTS && gnu_ext) {
                next();
                index_last = expr_const();
            }
            skip(']');
            s = type->ref;
            decl_design_flex(p, s, index_last);
            if (index < 0 || index_last >= s->c || index_last < index)
                tcc_error("�Y�����z��͈̔͂𒴂��Ă��邩�A�͈͂���ł�");
            if (cur_field)
                (*cur_field)->c = index_last;
            type = pointed_type(type);
            elem_size = type_size(type, &align);
            c += index * elem_size;
            nb_elems = index_last - index + 1;
        }
        else {
            int cumofs;
            next();
            l = tok;
        struct_field:
            next();
            f = find_field(type, l, &cumofs);
            if (cur_field)
                *cur_field = f;
            type = &f->type;
            c += cumofs;
        }
        cur_field = NULL;
    }
    if (!cur_field) {
        if (tok == '=') {
            next();
        }
        else if (!gnu_ext) {
            expect("=");
        }
    }
    else {
    no_designator:
        if (type->t & VT_ARRAY) {
            index = (*cur_field)->c;
            s = type->ref;
            decl_design_flex(p, s, index);
            if (index >= s->c)
                tcc_error("�������q�̐����������܂�");
            type = pointed_type(type);
            elem_size = type_size(type, &align);
            c += index * elem_size;
        }
        else {
            f = *cur_field;
            /* Skip bitfield padding. Also with size 32 and 64. */
            while (f && (f->v & SYM_FIRST_ANOM) &&
                is_integer_btype(f->type.t & VT_BTYPE))
                *cur_field = f = f->next;
            if (!f)
                tcc_error("�������q�̐����������܂�");
            type = &f->type;
            c += f->c;
        }
    }

    if (!elem_size) /* for structs */
        elem_size = type_size(type, &align);

    /* Using designators the same element can be initialized more
       than once.  In that case we need to delete possibly already
       existing relocations. */
    if (!(flags & DIF_SIZE_ONLY) && c - corig < al) {
        decl_design_delrels(p->sec, c, elem_size * nb_elems);
        flags &= ~DIF_CLEAR; /* mark stack dirty too */
    }

    decl_initializer(p, type, c, flags & ~DIF_FIRST);

    if (!(flags & DIF_SIZE_ONLY) && nb_elems > 1) {
        Sym aref = { 0 };
        CType t1;
        int i;
        if (p->sec || (type->t & VT_ARRAY)) {
            /* make init_putv/vstore believe it were a struct */
            aref.c = elem_size;
            t1.t = VT_STRUCT, t1.ref = &aref;
            type = &t1;
        }
        if (p->sec)
            vpush_ref(type, p->sec, c, elem_size);
        else
            vset(type, VT_LOCAL | VT_LVAL, c);
        for (i = 1; i < nb_elems; i++) {
            vdup();
            init_putv(p, type, c + elem_size * i);
        }
        vpop();
    }

    c += nb_elems * elem_size;
    if (c - corig > al)
        al = c - corig;
    return al;
}

/* store a value or an expression directly in global data or in local array */
static void init_putv(init_params* p, CType* type, unsigned long c)
{
    int bt;
    void* ptr;
    CType dtype;
    int size, align;
    Section* sec = p->sec;
    uint64_t val;

    dtype = *type;
    dtype.t &= ~VT_CONSTANT; /* need to do that to avoid false warning */

    size = type_size(type, &align);
    if (type->t & VT_BITFIELD)
        size = (BIT_POS(type->t) + BIT_SIZE(type->t) + 7) / 8;
    init_assert(p, c + size);

    if (sec) {
        /* XXX: not portable */
        /* XXX: generate error if incorrect relocation */
        gen_assign_cast(&dtype);
        bt = type->t & VT_BTYPE;

        if ((vtop->r & VT_SYM)
            && bt != VT_PTR
            && (bt != (PTR_SIZE == 8 ? VT_LLONG : VT_INT)
                || (type->t & VT_BITFIELD))
            && !((vtop->r & VT_CONST) && vtop->sym->v >= SYM_FIRST_ANOM)
            )
            tcc_error("�������q�̗v�f�̓��[�h���Ɍv�Z�ł��܂���");

        if (NODATA_WANTED) {
            vtop--;
            return;
        }

        ptr = sec->data + c;
        val = vtop->c.i;

        /* XXX: make code faster ? */
        if ((vtop->r & (VT_SYM | VT_CONST)) == (VT_SYM | VT_CONST) &&
            vtop->sym->v >= SYM_FIRST_ANOM &&
            /* XXX This rejects compound literals like
               '(void *){ptr}'.  The problem is that '&sym' is
               represented the same way, which would be ruled out
               by the SYM_FIRST_ANOM check above, but also '"string"'
               in 'char *p = "string"' is represented the same
               with the type being VT_PTR and the symbol being an
               anonymous one.  That is, there's no difference in vtop
               between '(void *){x}' and '&(void *){x}'.  Ignore
               pointer typed entities here.  Hopefully no real code
               will ever use compound literals with scalar type.  */
            (vtop->type.t & VT_BTYPE) != VT_PTR) {
            /* These come from compound literals, memcpy stuff over.  */
            Section* ssec;
            ElfSym* esym;
            ElfW_Rel* rel;
            esym = elfsym(vtop->sym);
            ssec = tcc_state->sections[esym->st_shndx];
            memmove(ptr, ssec->data + esym->st_value + (int)vtop->c.i, size);
            if (ssec->reloc) {
                /* We need to copy over all memory contents, and that
                   includes relocations.  Use the fact that relocs are
                   created it order, so look from the end of relocs
                   until we hit one before the copied region.  */
                unsigned long relofs = ssec->reloc->data_offset;
                while (relofs >= sizeof(*rel)) {
                    relofs -= sizeof(*rel);
                    rel = (ElfW_Rel*)(ssec->reloc->data + relofs);
                    if (rel->r_offset >= esym->st_value + size)
                        continue;
                    if (rel->r_offset < esym->st_value)
                        break;
                    put_elf_reloca(symtab_section, sec,
                        c + rel->r_offset - esym->st_value,
                        ELFW(R_TYPE)(rel->r_info),
                        ELFW(R_SYM)(rel->r_info),
#if PTR_SIZE == 8
                        rel->r_addend
#else
                        0
#endif
                    );
                }
            }
        }
        else {
            if (type->t & VT_BITFIELD) {
                int bit_pos, bit_size, bits, n;
                unsigned char* p, v, m;
                bit_pos = BIT_POS(vtop->type.t);
                bit_size = BIT_SIZE(vtop->type.t);
                p = (unsigned char*)ptr + (bit_pos >> 3);
                bit_pos &= 7, bits = 0;
                while (bit_size) {
                    n = 8 - bit_pos;
                    if (n > bit_size)
                        n = bit_size;
                    v = val >> bits << bit_pos;
                    m = ((1 << n) - 1) << bit_pos;
                    *p = (*p & ~m) | (v & m);
                    bits += n, bit_size -= n, bit_pos = 0, ++p;
                }
            }
            else
                switch (bt) {
                case VT_BOOL:
                    *(char*)ptr = val != 0;
                    break;
                case VT_BYTE:
                    *(char*)ptr = val;
                    break;
                case VT_SHORT:
                    write16le(ptr, val);
                    break;
                case VT_FLOAT:
                    write32le(ptr, val);
                    break;
                case VT_DOUBLE:
                    write64le(ptr, val);
                    break;
                case VT_LDOUBLE:
#if defined TCC_IS_NATIVE_387
                    /* Host and target platform may be different but both have x87.
                       On windows, tcc does not use VT_LDOUBLE, except when it is a
                       cross compiler.  In this case a mingw gcc as host compiler
                       comes here with 10-byte long doubles, while msvc or tcc won't.
                       tcc itself can still translate by asm.
                       In any case we avoid possibly random bytes 11 and 12.
                    */
                    if (sizeof(long double) >= 10)
                        memcpy(ptr, &vtop->c.ld, 10);
#ifdef __TINYC__
                    else if (sizeof(long double) == sizeof(double))
                        __asm__("fldl %1\nfstpt %0\n" : "=m" (*ptr) : "m" (vtop->c.ld));
#endif
                    else
#endif
                        /* For other platforms it should work natively, but may not work
                           for cross compilers */
                        if (sizeof(long double) == LDOUBLE_SIZE)
                            memcpy(ptr, &vtop->c.ld, LDOUBLE_SIZE);
                        else if (sizeof(double) == LDOUBLE_SIZE)
                            *(double*)ptr = (double)vtop->c.ld;
                        else if (0 == memcmp(ptr, &vtop->c.ld, LDOUBLE_SIZE))
                            ; /* nothing to do for 0.0 */
#ifndef TCC_CROSS_TEST
                        else
                            tcc_error("long double �萔���N���X�R���p�C���ł��܂���");
#endif
                    break;

#if PTR_SIZE == 8
                    /* intptr_t may need a reloc too, see tcctest.c:relocation_test() */
                case VT_LLONG:
                case VT_PTR:
                    if (vtop->r & VT_SYM)
                        greloca(sec, vtop->sym, c, R_DATA_PTR, val);
                    else
                        write64le(ptr, val);
                    break;
                case VT_INT:
                    write32le(ptr, val);
                    break;
#else
                case VT_LLONG:
                    write64le(ptr, val);
                    break;
                case VT_PTR:
                case VT_INT:
                    if (vtop->r & VT_SYM)
                        greloc(sec, vtop->sym, c, R_DATA_PTR);
                    write32le(ptr, val);
                    break;
#endif
                default:
                    //tcc_internal_error("unexpected type");
                    break;
                }
        }
        vtop--;
    }
    else {
        /* C++ BUG-9: a reference declaration binds to the initializer
           lvalue, i.e. stores its ADDRESS.  Without this the value was
           stored into the pointer slot (scalar) or memcpy'd over it
           (struct), so later accesses dereferenced garbage. */
        if (tcc_state->cpp && (dtype.t & VT_REFERENCE)) {
            if ((vtop->r & VT_LVAL)
                && cpp_can_bind_lvalue_to_reference(&dtype, &vtop->type)) {
                gaddrof();
                vtop->type = dtype;
                /* plain pointer store; keeping VT_REFERENCE on the dest
                   would trigger the assign-through-reference path */
                vtop->type.t &= ~VT_REFERENCE;
                dtype.t &= ~VT_REFERENCE;
            } else {
                tcc_error("cannot bind reference to this initializer");
            }
        }
        vset(&dtype, VT_LOCAL | VT_LVAL, c);
        vswap();
        vstore();
        vpop();
    }
}

/* 't' contains the type and storage info. 'c' is the offset of the
   object in section 'sec'. If 'sec' is NULL, it means stack based
   allocation. 'flags & DIF_FIRST' is true if array '{' must be read (multi
   dimension implicit array init handling). 'flags & DIF_SIZE_ONLY' is true if
   size only evaluation is wanted (only for arrays). */
static void decl_initializer(init_params* p, CType* type, unsigned long c, int flags)
{
    int len, n, no_oblock, i;
    int size1, align1;
    Sym* s, * f;
    Sym indexsym;
    CType* t1;

    /* generate line number info */
    if (debug_modes && !(flags & DIF_SIZE_ONLY) && !p->sec)
        tcc_debug_line(tcc_state), tcc_tcov_check_line(tcc_state, 1);

    if (!(flags & DIF_HAVE_ELEM) && tok != '{' &&
        /* In case of strings we have special handling for arrays, so
           don't consume them as initializer value (which would commit them
           to some anonymous symbol).  */
        tok != TOK_LSTR && tok != TOK_STR &&
        (!(flags & DIF_SIZE_ONLY)
            /* a struct may be initialized from a struct of same type, as in
                    struct {int x,y;} a = {1,2}, b = {3,4}, c[] = {a,b};
               In that case we need to parse the element in order to check
               it for compatibility below */
            || (type->t & VT_BTYPE) == VT_STRUCT)
        ) {
        int ncw_prev = nocode_wanted;
        if ((flags & DIF_SIZE_ONLY) && !p->sec)
            ++nocode_wanted;
        parse_init_elem(!p->sec ? EXPR_ANY : EXPR_CONST);
        nocode_wanted = ncw_prev;
        flags |= DIF_HAVE_ELEM;
    }

    if (type->t & VT_ARRAY) {
        no_oblock = 1;
        if (((flags & DIF_FIRST) && tok != TOK_LSTR && tok != TOK_STR) ||
            tok == '{') {
            skip('{');
            no_oblock = 0;
        }

        s = type->ref;
        n = s->c;
        t1 = pointed_type(type);
        size1 = type_size(t1, &align1);

        /* only parse strings here if correct type (otherwise: handle
           them as ((w)char *) expressions */
        if ((tok == TOK_LSTR &&
#ifdef TCC_TARGET_PE
        (t1->t & VT_BTYPE) == VT_SHORT && (t1->t & VT_UNSIGNED)
#else
            (t1->t & VT_BTYPE) == VT_INT
#endif
            ) || (tok == TOK_STR && (t1->t & VT_BTYPE) == VT_BYTE)) {
            len = 0;
            cstr_reset(&initstr);
            if (size1 != (tok == TOK_STR ? 1 : sizeof(nwchar_t)))
                tcc_error("�����񃊃e�����̌����������ł��܂���");
            while (tok == TOK_STR || tok == TOK_LSTR) {
                if (initstr.size)
                    initstr.size -= size1;
                if (tok == TOK_STR)
                    len += tokc.str.size;
                else
                    len += tokc.str.size / sizeof(nwchar_t);
                len--;
                cstr_cat(&initstr, tokc.str.data, tokc.str.size);
                next();
            }
            if (tok != ')' && tok != '}' && tok != ',' && tok != ';'
                && tok != TOK_EOF) {
                /* Not a lone literal but part of a bigger expression.  */
                unget_tok(size1 == 1 ? TOK_STR : TOK_LSTR);
                tokc.str.size = initstr.size;
                tokc.str.data = initstr.data;
                goto do_init_array;
            }

            decl_design_flex(p, s, len);
            if (!(flags & DIF_SIZE_ONLY)) {
                int nb = n, ch;
                if (len < nb)
                    nb = len;
                if (len > nb)
                    tcc_warning("�z��̏����������񂪒������܂�");
                /* in order to go faster for common case (char
                   string in global variable, we handle it
                   specifically */
                if (p->sec && size1 == 1) {
                    init_assert(p, c + nb);
                    if (!NODATA_WANTED)
                        memcpy(p->sec->data + c, initstr.data, nb);
                }
                else {
                    for (i = 0; i < n; i++) {
                        if (i >= nb) {
                            /* only add trailing zero if enough storage (no
                               warning in this case since it is standard) */
                            if (flags & DIF_CLEAR)
                                break;
                            if (n - i >= 4) {
                                init_putz(p, c + i * size1, (n - i) * size1);
                                break;
                            }
                            ch = 0;
                        }
                        else if (size1 == 1)
                            ch = ((unsigned char*)initstr.data)[i];
                        else
                            ch = ((nwchar_t*)initstr.data)[i];
                        vpushi(ch);
                        init_putv(p, t1, c + i * size1);
                    }
                }
            }
        }
        else {

        do_init_array:
            indexsym.c = 0;
            f = &indexsym;

        do_init_list:
            /* zero memory once in advance */
            if (!(flags & (DIF_CLEAR | DIF_SIZE_ONLY))) {
                init_putz(p, c, n * size1);
                flags |= DIF_CLEAR;
            }

            len = 0;
            /* GNU extension: if the initializer is empty for a flex array,
               it's size is zero.  We won't enter the loop, so set the size
               now.  */
            decl_design_flex(p, s, len);
            while (tok != '}' || (flags & DIF_HAVE_ELEM)) {
                len = decl_designator(p, type, c, &f, flags, len);
                flags &= ~DIF_HAVE_ELEM;
                if (type->t & VT_ARRAY) {
                    ++indexsym.c;
                    /* special test for multi dimensional arrays (may not
                       be strictly correct if designators are used at the
                       same time) */
                    if (no_oblock && len >= n * size1)
                        break;
                }
                else {
                    if (s->type.t == VT_UNION)
                        f = NULL;
                    else
                        f = f->next;
                    if (no_oblock && f == NULL)
                        break;
                }

                if (tok == '}')
                    break;
                skip(',');
            }
        }
        if (!no_oblock)
            skip('}');

    }
    else if ((flags & DIF_HAVE_ELEM)
        /* Use i_c_parameter_t, to strip toplevel qualifiers.
           The source type might have VT_CONSTANT set, which is
           of course assignable to non-const elements.  */
        && is_compatible_unqualified_types(type, &vtop->type)) {
        goto one_elem;

    }
    else if ((type->t & VT_BTYPE) == VT_STRUCT) {
        no_oblock = 1;
        if ((flags & DIF_FIRST) || tok == '{') {
            skip('{');
            no_oblock = 0;
        }
        s = type->ref;
        f = s->next;
        n = s->c;
        size1 = 1;
        goto do_init_list;

    }
    else if (tok == '{') {
        if (flags & DIF_HAVE_ELEM)
            skip(';');
        next();
        decl_initializer(p, type, c, flags & ~DIF_HAVE_ELEM);
        skip('}');

    }
    else one_elem: if ((flags & DIF_SIZE_ONLY)) {
        /* If we supported only ISO C we wouldn't have to accept calling
           this on anything than an array if DIF_SIZE_ONLY (and even then
           only on the outermost level, so no recursion would be needed),
           because initializing a flex array member isn't supported.
           But GNU C supports it, so we need to recurse even into
           subfields of structs and arrays when DIF_SIZE_ONLY is set.  */
           /* just skip expression */
        if (flags & DIF_HAVE_ELEM)
            vpop();
        else
            skip_or_save_block(NULL);

    }
    else {
        if (!(flags & DIF_HAVE_ELEM)) {
            /* This should happen only when we haven't parsed
               the init element above for fear of committing a
               string constant to memory too early.  */
            if (tok != TOK_STR && tok != TOK_LSTR)
                expect("string constant");
            parse_init_elem(!p->sec ? EXPR_ANY : EXPR_CONST);
        }
        if (!p->sec && (flags & DIF_CLEAR) /* container was already zero'd */
            && (vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST
            && vtop->c.i == 0
            && btype_size(type->t & VT_BTYPE) /* not for fp constants */
            )
            vpop();
        else
            init_putv(p, type, c);
    }
}

/* parse an initializer for type 't' if 'has_init' is non zero, and
   allocate space in local or global data space ('r' is either
   VT_LOCAL or VT_CONST). If 'v' is non zero, then an associated
   variable 'v' of scope 'scope' is declared before initializers
   are parsed. If 'v' is zero, then a reference to the new object
   is put in the value stack. If 'has_init' is 2, a special parsing
   is done to handle string constants. */
static void decl_initializer_alloc(CType* type, AttributeDef* ad, int r,
    int has_init, int v, int global)
{
    int size, align, addr;
    TokenString* init_str = NULL;

    Section* sec;
    Sym* flexible_array;
    Sym* sym;
    int saved_nocode_wanted = nocode_wanted;
#ifdef CONFIG_TCC_BCHECK
    int bcheck = tcc_state->do_bounds_check && !NODATA_WANTED;
#endif
    init_params p = { 0 };
    int cpp_needs_vptr = 0;

    /* Always allocate static or global variables */
    if (v && (r & VT_VALMASK) == VT_CONST)
        nocode_wanted |= DATA_ONLY_WANTED;

    flexible_array = NULL;
    size = type_size(type, &align);

    /* exactly one flexible array may be initialized, either the
       toplevel array or the last member of the toplevel struct */

    if (size < 0) {
        // error out except for top-level incomplete arrays
        // (arrays of incomplete types are handled in array parsing)
        if (!(type->t & VT_ARRAY))
            tcc_error("�s���S�^�̏������ł�");

        /* If the base type itself was an array type of unspecified size
           (like in 'typedef int arr[]; arr x = {1};') then we will
           overwrite the unknown size by the real one for this decl.
           We need to unshare the ref symbol holding that size. */
        type->ref = sym_push(SYM_FIELD, &type->ref->type, 0, type->ref->c);
        p.flex_array_ref = type->ref;

    }
    else if (has_init && (type->t & VT_BTYPE) == VT_STRUCT) {
        Sym* field = type->ref->next;
        if (field) {
            while (field->next)
                field = field->next;
            if (field->type.t & VT_ARRAY && field->type.ref->c < 0) {
                flexible_array = field;
                p.flex_array_ref = field->type.ref;
                size = -1;
            }
        }
    }

    if (size < 0) {
        /* If unknown size, do a dry-run 1st pass */
        if (!has_init)
            tcc_error("�^�̃T�C�Y���s���ł�");
        if (has_init == 2) {
            /* only get strings */
            init_str = tok_str_alloc();
            while (tok == TOK_STR || tok == TOK_LSTR) {
                tok_str_add_tok(init_str);
                next();
            }
            tok_str_add(init_str, TOK_EOF);
        }
        else
            skip_or_save_block(&init_str);
        unget_tok(0);

        /* compute size */
        begin_macro(init_str, 1);
        next();
        decl_initializer(&p, type, 0, DIF_FIRST | DIF_SIZE_ONLY);
        /* prepare second initializer parsing */
        macro_ptr = init_str->str;
        next();

        /* if still unknown size, error */
        size = type_size(type, &align);
        if (size < 0)
            tcc_error("�^�̃T�C�Y���s���ł�");

        /* If there's a flex member and it was used in the initializer
           adjust size.  */
        if (flexible_array && flexible_array->type.ref->c > 0)
            size += flexible_array->type.ref->c
            * pointed_size(&flexible_array->type);
    }

    /* take into account specified alignment if bigger */
    if (ad->a.aligned) {
        int speca = 1 << (ad->a.aligned - 1);
        if (speca > align)
            align = speca;
    }
    else if (ad->a.packed) {
        align = 1;
    }

    if (!v && NODATA_WANTED)
        size = 0, align = 1;

    if ((r & VT_VALMASK) == VT_LOCAL) {
        sec = NULL;
#ifdef CONFIG_TCC_BCHECK
        if (bcheck && v) {
            /* add padding between stack variables for bound checking */
            loc -= align;
        }
#endif
        loc = (loc - size) & -align;
        addr = loc;
        p.local_offset = addr + size;
#ifdef CONFIG_TCC_BCHECK
        if (bcheck && v) {
            /* add padding between stack variables for bound checking */
            loc -= align;
        }
#endif
        if (v) {
            /* local variable */
#ifdef CONFIG_TCC_ASM
            if (ad->asm_label) {
                int reg = asm_parse_regvar(ad->asm_label);
                if (reg >= 0)
                    r = (r & ~VT_VALMASK) | reg;
            }
#endif
            sym = sym_push(v, type, r, addr);
            /* FEAT-5A: initialize the vptr of a local polymorphic object so
             * virtual calls dispatch through a valid vtable. */
            if (tcc_state->cpp
                && (type->t & VT_BTYPE) == VT_STRUCT
                && type->ref
                && type->ref->cpp_vtable_tok)
                cpp_init_local_vptr(sym);
            if (ad->cleanup_func) {
                Sym* cls = sym_push2(&all_cleanups,
                    SYM_FIELD | ++cur_scope->cl.n, 0, 0);
                cls->prev_tok = sym;
                cls->cleanup_func = ad->cleanup_func;
                cls->next = cur_scope->cl.s;
                cur_scope->cl.s = cls;
            }

            sym->a = ad->a;
        }
        else {
            /* push local reference */
            vset(type, r, addr);
        }
    }
    else {
        sym = NULL;
        cpp_needs_vptr = tcc_state->cpp && v && global
            && (type->t & VT_BTYPE) == VT_STRUCT && type->ref
            && type->ref->cpp_vtable_tok;
        if (v && global) {
            /* see if the symbol was already defined */
            sym = sym_find(v);
            if (sym) {
                if (p.flex_array_ref && (sym->type.t & type->t & VT_ARRAY)
                    && sym->type.ref->c > type->ref->c) {
                    /* flex array was already declared with explicit size
                            extern int arr[10];
                            int arr[] = { 1,2,3 }; */
                    type->ref->c = sym->type.ref->c;
                    size = type_size(type, &align);
                }
                patch_storage(sym, ad, type);
                /* we accept several definitions of the same global variable. */
                if (!has_init && sym->c && elfsym(sym)->st_shndx != SHN_UNDEF)
                    goto no_alloc;
            }
        }

        /* allocate symbol in corresponding section */
        sec = ad->section;
        if (!sec) {
            CType* tp = type;
            while ((tp->t & (VT_BTYPE | VT_ARRAY)) == (VT_PTR | VT_ARRAY))
                tp = &tp->ref->type;
            if (tp->t & VT_CONSTANT) {
                sec = rodata_section;
            }
            else if (has_init || cpp_needs_vptr) {
                sec = data_section;
                /*if (g_debug & 4)
                    tcc_warning("rw data: %s", get_tok_str(v, 0));*/
            }
            else if (tcc_state->nocommon)
                sec = bss_section;
        }

        if (sec) {
            addr = section_add(sec, size, align);
#ifdef CONFIG_TCC_BCHECK
            /* add padding if bound check */
            if (bcheck)
                section_add(sec, 1, 1);
#endif
        }
        else {
            addr = align; /* SHN_COMMON is special, symbol value is align */
            sec = common_section;
        }

        if (v) {
            if (!sym) {
                sym = sym_push(v, type, r | VT_SYM, 0);
                patch_storage(sym, ad, NULL);
            }
            /* update symbol definition */
            put_extern_sym(sym, sec, addr, size);
            /* FEAT-5A Phase 4: wire vptr slot for global polymorphic objects.
             * Phase 4: vptr reloc in data section (needs_vptr forces .data). */
            if (tcc_state->cpp
                && (type->t & VT_BTYPE) == VT_STRUCT
                && type->ref
                && type->ref->cpp_vtable_tok) {
                cpp_init_global_vptr(sym, sec, addr);
            }
        }
        else {
            /* push global reference */
            vpush_ref(type, sec, addr, size);
            sym = vtop->sym;
            vtop->r |= r;
        }

#ifdef CONFIG_TCC_BCHECK
        /* handles bounds now because the symbol must be defined
           before for the relocation */
        if (bcheck) {
            addr_t* bounds_ptr;

            greloca(bounds_section, sym, bounds_section->data_offset, R_DATA_PTR, 0);
            /* then add global bound info */
            bounds_ptr = section_ptr_add(bounds_section, 2 * sizeof(addr_t));
            bounds_ptr[0] = 0; /* relocated */
            bounds_ptr[1] = size;
        }
#endif
    }

    if (type->t & VT_VLA) {
        int a;

        if (NODATA_WANTED)
            goto no_alloc;

        /* save before-VLA stack pointer if needed */
        if (cur_scope->vla.num == 0) {
            if (cur_scope->prev && cur_scope->prev->vla.num) {
                cur_scope->vla.locorig = cur_scope->prev->vla.loc;
            }
            else {
                gen_vla_sp_save(loc -= PTR_SIZE);
                cur_scope->vla.locorig = loc;
            }
        }

        vpush_type_size(type, &a);
        gen_vla_alloc(type, a);
#if defined TCC_TARGET_PE && defined TCC_TARGET_X86_64
        /* on _WIN64, because of the function args scratch area, the
           result of alloca differs from RSP and is returned in RAX.  */
        gen_vla_result(addr), addr = (loc -= PTR_SIZE);
#endif
        gen_vla_sp_save(addr);
        cur_scope->vla.loc = addr;
        cur_scope->vla.num++;
    }
    else if (has_init) {
        p.sec = sec;
        decl_initializer(&p, type, addr, DIF_FIRST);
        /* patch flexible array member size back to -1, */
        /* for possible subsequent similar declarations */
        if (flexible_array)
            flexible_array->type.ref->c = -1;
    }

no_alloc:
    /* restore parse state if needed */
    if (init_str) {
        end_macro();
        next();
    }

    nocode_wanted = saved_nocode_wanted;
}

/* generate vla code saved in post_type() */
static void func_vla_arg_code(Sym* arg)
{
    int align;
    TokenString* vla_array_tok = NULL;

    if (arg->type.ref)
        func_vla_arg_code(arg->type.ref);

    if ((arg->type.t & VT_VLA) && arg->type.ref->vla_array_str) {
        loc -= type_size(&int_type, &align);
        loc &= -align;
        arg->type.ref->c = loc;

        unget_tok(0);
        vla_array_tok = tok_str_alloc();
        vla_array_tok->str = arg->type.ref->vla_array_str;
        begin_macro(vla_array_tok, 1);
        next();
        gexpr();
        end_macro();
        next();
        vpush_type_size(&arg->type.ref->type, &align);
        gen_op('*');
        vset(&int_type, VT_LOCAL | VT_LVAL, arg->type.ref->c);
        vswap();
        vstore();
        vpop();
    }
}

static void func_vla_arg(Sym* sym)
{
    Sym* arg;

    for (arg = sym->type.ref->next; arg; arg = arg->next)
        if ((arg->type.t & VT_BTYPE) == VT_PTR && (arg->type.ref->type.t & VT_VLA))
            func_vla_arg_code(arg->type.ref);
}

/* parse a function defined by symbol 'sym' and generate its code in
   'cur_text_section' */
static void gen_function(Sym* sym)
{
    struct scope f = { 0 };
    Sym *this_param;
    Sym *saved_param_next;
    cur_scope = root_scope = &f;
    nocode_wanted = 0;

    ind = cur_text_section->data_offset;
    if (sym->a.aligned) {
        size_t newoff = section_add(cur_text_section, 0,
            1 << (sym->a.aligned - 1));
        gen_fill_nops(newoff - ind);
    }

    funcname = get_tok_str(sym->v, NULL);
    func_ind = ind;
    func_vt = sym->type.ref->type;
    func_var = sym->type.ref->f.func_type == FUNC_ELLIPSIS;

    this_param = NULL;
    saved_param_next = NULL;
    cpp_this_sym = NULL;
    cpp_cur_func_class = sym->parent_class;
    if (sym->parent_class && tcc_state->cpp && !(sym->type.t & VT_STATIC)) {
        CType pt;

        /* Build `this` as a real pointer type via mk_pointer so that
           pointed_type() yields {VT_STRUCT, ref=class}.  Pointing ref
           directly at the class tag sym made pointed_type() return the
           tag's own type whose ref is NULL (struct_decl), crashing any
           field walk after indir() (`this->x`, `*this`) (BUG-8). */
        pt.t = VT_STRUCT;
        pt.ref = sym->parent_class;
        /* FEAT-6B-P2: a const method takes `const T* this`; the const
           must sit on the pointed-to type so that indir() propagates it
           to member lvalues (assignment then errors in
           verify_assign_cast).  It was previously set on the pointer
           itself, where nothing ever checked it. */
        if (sym->type.ref->f.func_const)
            pt.t |= VT_CONSTANT;
        mk_pointer(&pt);
        this_param = sym_malloc();
        /* sym_malloc() recycles pool memory without zeroing; r/c would
           otherwise carry stale data (BUG-6).  They are placeholders
           only: the authoritative `this` storage is the local pushed by
           gfunc_prolog, see below. */
        this_param->v = TOK_THIS;
        this_param->type = pt;
        this_param->r = 0;
        this_param->c = 0;
        saved_param_next = sym->type.ref->next;
        this_param->next = saved_param_next;
        sym->type.ref->next = this_param;
        cpp_this_sym = this_param;
    }

    /* NOTE: we patch the symbol size later */
    put_extern_sym(sym, cur_text_section, ind, 0);

    if (sym->type.ref->f.func_ctor)
        add_array(tcc_state, ".init_array", sym->c);
    if (sym->type.ref->f.func_dtor)
        add_array(tcc_state, ".fini_array", sym->c);

    /* put debug symbol */
    tcc_debug_funcstart(tcc_state, sym);

    /* push a dummy symbol to enable local sym storage */
    sym_push2(&local_stack, SYM_FIELD, 0, 0);
    local_scope = 1; /* for function parameters */
    nb_temp_local_vars = 0;
    gfunc_prolog(sym);
    tcc_debug_prolog_epilog(tcc_state, 0);

    /* gfunc_prolog pushed every entry of sym->type.ref->next (including
     * the injected `this`) on local_stack with its real storage (r/c).
     * this_param itself is only a type-list entry without storage, so
     * member accesses must go through the prolog-created local (BUG-6).
     * Precondition: this runs right after gfunc_prolog and before the
     * body is parsed, so sym_find(TOK_THIS) can only resolve to the
     * parameter just pushed -- no shadowing is possible yet. */
    if (this_param) {
        Sym *this_local = sym_find(TOK_THIS);
        if (!this_local)
            tcc_error("internal error: 'this' parameter not pushed by prolog");
        cpp_this_sym = this_local;
    }

    local_scope = 0;
    rsym = 0;
    func_vla_arg(sym);

    /* C++ constructor member-initializer list: expand `: a(x), b(y)` saved on
     * the ctor sym into `this->a = x; this->b = y;` instructions before the
     * body runs.  Base-class initializers call __cpp_ctor_Base on the
     * embedded base subobject (FEAT-4D). */
    if (tcc_state->cpp && sym->cpp_mem_init_list
        && cpp_this_sym && sym->parent_class) {
        TokenString *init_copy = tok_str_dup_for_default(sym->cpp_mem_init_list);
        if (init_copy) {
            Sym *class_sym = sym->parent_class;
            begin_macro(init_copy, 1);
            next();
            while (tok != TOK_EOF) {
                int member_tok;
                Sym *member_field;
                Sym *base_field;

                if (tok < TOK_IDENT)
                    expect("identifier");
                member_tok = tok;
                next();
                if (tok != '(')
                    expect("(");
                next();

                member_field = cpp_lookup_member_field(member_tok, class_sym);
                if (member_field
                    && (member_field->type.t & VT_BTYPE) != VT_FUNC) {
                    cpp_push_member_var(member_field);
                    if (tok != ')') {
                        expr_eq();
                        vstore();
                        vpop();
                    } else {
                        vpop();
                    }
                } else {
                    base_field = cpp_find_base_field(class_sym, member_tok);
                    if (base_field && base_field->type.ref) {
                        cpp_emit_base_ctor_call(base_field,
                                                base_field->type.ref);
                    } else {
                        /* unknown name: skip expr. */
                        int paren = 0;
                        while (tok != TOK_EOF) {
                            if (tok == '(') paren++;
                            else if (tok == ')') {
                                if (paren == 0) break;
                                paren--;
                            }
                            next();
                        }
                    }
                }

                if (tok != ')')
                    expect(")");
                next();
                if (tok == ',')
                    next();
            }
            end_macro();
            /* gen_inline_functions has already consumed the body's `{`
             * (tok == '{' when gen_function was entered).  After our
             * init-list expansion tok is TOK_EOF, so put back `{` to
             * keep block()'s entry contract. */
            unget_tok('{');
        }
    }

    block(0);
    gsym(rsym);

    nocode_wanted = 0;
    /* reset local stack */
    pop_local_syms(NULL, 0);
    tcc_debug_prolog_epilog(tcc_state, 1);
    gfunc_epilog();

    /* end of function */
    tcc_debug_funcend(tcc_state, ind - func_ind);

    /* patch symbol size */
    elfsym(sym)->st_size = ind - func_ind;

    cur_text_section->data_offset = ind;
    local_scope = 0;
    label_pop(&global_label_stack, NULL, 0);
    sym_pop(&all_cleanups, NULL, 0);

    /* It's better to crash than to generate wrong code */
    if (this_param) {
        sym->type.ref->next = saved_param_next;
        sym_free(this_param);
    }
    cpp_this_sym = NULL;
    cpp_cur_func_class = NULL;
    cur_text_section = NULL;
    funcname = ""; /* for safety */
    func_vt.t = VT_VOID; /* for safety */
    func_var = 0; /* for safety */
    ind = 0; /* for safety */
    func_ind = -1;
    nocode_wanted = DATA_ONLY_WANTED;
    check_vstack();

    /* do this after funcend debug info */
    next();
}

static void gen_inline_functions(TCCState* s)
{
    Sym* sym;
    int inline_generated, i;
    struct InlineFunc* fn;

    tcc_open_bf(s, ":inline:", 0);
    /* iterate while inline function are referenced */
    do {
        inline_generated = 0;
        for (i = 0; i < s->nb_inline_fns; ++i) {
            fn = s->inline_fns[i];
            sym = fn->sym;
            if (sym && (sym->c || !(sym->type.t & VT_INLINE))) {
                /* the function was used or forced (and then not internal):
                   generate its code and convert it to a normal function */
                fn->sym = NULL;
                tccpp_putfile(fn->filename);
                begin_macro(fn->func_str, 1);
                next();
                cur_text_section = text_section;
                gen_function(sym);
                end_macro();

                inline_generated = 1;
            }
        }
    } while (inline_generated);
    tcc_close();
}

static void free_inline_functions(TCCState* s)
{
    int i;
    /* free tokens of unused inline functions */
    for (i = 0; i < s->nb_inline_fns; ++i) {
        struct InlineFunc* fn = s->inline_fns[i];
        if (fn->sym)
            tok_str_free(fn->func_str);
    }
    dynarray_reset(&s->inline_fns, &s->nb_inline_fns);
}

static void do_Static_assert(void)
{
    int c;
    const char* msg;

    next();
    skip('(');
    c = expr_const();
    msg = "_Static_assert fail";
    if (tok == ',') {
        next();
        msg = parse_mult_str("string constant")->data;
    }
    skip(')');
    if (c == 0)
        tcc_error("%s", msg);
    skip(';');
}

/* 'l' is VT_LOCAL or VT_CONST to define default storage type
   or VT_CMP if parsing old style parameter list
   or VT_JMP if parsing c99 for decl: for (int i = 0, ...) */
static int decl(int l)
{
    int v, has_init, r, oldint, ooc_cls;
    CType type, btype;
    Sym* sym;
    AttributeDef ad, adbase;
    ElfSym* esym;

    while (1) {

        if (tcc_state->cpp && tok == TOK_EXTERN) {
            next();
            if (tok == TOK_STR) {
                const char *s = tokc.str.data;
                int len = tokc.str.size - 1;

                if (len == 1 && s[0] == 'C') {
                    next();
                    if (tok != '{') {
                        tcc_state->extern_c++;
                        tcc_state->lex_c++;
                        decl_once_flag = 1;
                        decl(l);
                        decl_once_flag = 0;
                        tcc_state->lex_c--;
                        tcc_state->extern_c--;
                        /* BUG-11: decl() already fetched the token after
                           ';' in C mode - re-promote it. */
                        cpp_repromote_stale_lookahead();
                        continue;
                    }
                    /* BUG-11: raise lex_c before consuming '{' so the
                       first token inside the block is lexed in C mode
                       (symmetry with the trailing lookahead fix). */
                    tcc_state->extern_c++;
                    tcc_state->lex_c++;
                    next();
                    while (tok != '}') {
                        if (tok == TOK_EOF)
                            tcc_error("unclosed extern C block");
                        decl(l);
                    }
                    next();
                    tcc_state->lex_c--;
                    tcc_state->extern_c--;
                    /* BUG-11: the next() above fetched the token after
                       '}' in C mode - re-promote it. */
                    cpp_repromote_stale_lookahead();
                    continue;
                }
                if (len == 3 && !memcmp(s, "C++", 3)) {
                    next();
                    tcc_error("Stage 1: extern C++ not supported");
                }
                tcc_error("unsupported linkage");
            } else {
                unget_tok(TOK_EXTERN);
            }
        }

        oldint = 0;
        ooc_cls = 0;
        if (tcc_state->cpp && cpp_peek_out_of_class_ctor(&ooc_cls)) {
            btype.t = VT_VOID;
            btype.ref = NULL;
            memset(&adbase, 0, sizeof adbase);
            cpp_qualified_class = struct_find(ooc_cls);
            if (!cpp_qualified_class)
                tcc_error("unknown class in qualified name");
        } else if (!parse_btype(&btype, &adbase, l == VT_LOCAL)) {
            if (l == VT_JMP)
                return 0;
            /* skip redundant ';' if not in old parameter decl scope */
            if (tok == ';' && l != VT_CMP) {
                next();
                continue;
            }
            if (tok == TOK_STATIC_ASSERT) {
                do_Static_assert();
                continue;
            }
            if (tcc_state->cpp && cpp_unget_scoped_expr()) {
                if (l != VT_CONST)
                    break;
            }
            if (l != VT_CONST)
                break;
            if (tok == TOK_ASM1 || tok == TOK_ASM2 || tok == TOK_ASM3) {
                /* global asm block */
                asm_global_instr();
                continue;
            }
            if (tok >= TOK_UIDENT) {
                /* special test for old K&R protos without explicit int
                   type. Only accepted when defining global data */
                int is_qual = 0;
                if (tcc_state->cpp) {
                    Sym *stsym = struct_find(tok);
                    if (!stsym) {
                        Sym *ts = sym_find(tok);
                        if (ts && ts->type.ref)
                            stsym = ts->type.ref;
                    }
                    if (stsym) {
                        int cls_tok = tok;
                        next();
                        if (tok == ':') {
                            next();
                            if (tok == ':') {
                                unget_tok(':');
                                unget_tok(':');
                                unget_tok(cls_tok);
                                is_qual = 1;
                            } else
                                unget_tok(':');
                        }
                        if (!is_qual)
                            unget_tok(cls_tok);
                    }
                }
                if (is_qual) {
                    if (l != VT_CONST)
                        break;
                } else {
                    btype.t = VT_INT;
                    oldint = 1;
                }
            }
            else {
                if (tok != TOK_EOF) {
                    if (tok == '}' && tcc_state->extern_c)
                        break;
                    expect("declaration");
                }
                break;
            }
        }

        if (tok == ';') {
            if ((btype.t & VT_BTYPE) == VT_STRUCT) {
                v = btype.ref->v;
                if (!(v & SYM_FIELD) && (v & ~SYM_STRUCT) >= SYM_FIRST_ANOM)
                    tcc_warning("�C���X�^���X���`���Ȃ����� struct/union �ł�");
                next();
                continue;
            }
            if (IS_ENUM(btype.t)) {
                next();
                continue;
            }
        }

        while (1) { /* iterate thru each declaration */
            type = btype;
            ad = adbase;
            /* FEAT-4G: global `Foo g;` / `Foo g(args);` with ctor.  Kept
               separate from the local FEAT-4B/4F rewrite: mixing both in one
               block left global decls in a bad token state (hang). */
            if (tcc_state->cpp
                && l == VT_CONST
                && (btype.t & VT_BTYPE) == VT_STRUCT
                && btype.ref
                && tok >= TOK_UIDENT
                && cpp_find_ctor_field(btype.ref)) {
                int saved_var_tok = tok;
                int obj_r = VT_LVAL | VT_CONST;
                Sym *obj_sym;
                next();
                if (tok == '(') {
                    next();
                    if (tok != ')') {
                        TokenString *arg_toks;
                        arg_toks = cpp_save_paren_expr_tokens();
                        decl_initializer_alloc(&type, &ad, obj_r,
                            0, saved_var_tok, 1);
                        obj_sym = sym_find(saved_var_tok);
                        cpp_register_global_dyn(obj_sym, arg_toks, 0);
                        next();
                        if (tok == ',') {
                            next();
                            continue;
                        }
                        skip(';');
                        break;
                    }
                    unget_tok('(');
                    unget_tok(saved_var_tok);
                } else if ((tok == ';' || tok == ',')
                           /* VT_STATIC is NOT excluded here: a file-scope
                              `static P g;` is a definition with static
                              storage duration whose default ctor must run
                              at startup (matching `static P g(args);` and
                              non-static `P g;`).  VT_EXTERN is a declaration
                              and VT_TYPEDEF is a type alias, so both stay
                              excluded.  (The local FEAT-4F gate below keeps
                              excluding VT_STATIC: function-local statics need
                              once-only guarded construction, not this path.) */
                           && !(btype.t & (VT_EXTERN | VT_TYPEDEF))
                           && cpp_class_has_default_ctor(btype.ref)) {
                    decl_initializer_alloc(&type, &ad, obj_r,
                        0, saved_var_tok, 1);
                    obj_sym = sym_find(saved_var_tok);
                    cpp_register_global_dyn(obj_sym, NULL, 0);
                    if (tok == ',') {
                        next();
                        continue;
                    }
                    skip(';');
                    break;
                } else {
                    unget_tok(saved_var_tok);
                }
            }
            /* C++ FEAT-4B: detect `ClassType ident(args);` ctor-call form.
               Only in C++ mode, when the base type is a class with a
               constructor, we are in a local scope, and an identifier is
               directly followed by '('. The variable is allocated like a
               plain `Foo f;` and `Foo f(args)` is rewritten into the
               existing member-call path `f.Foo(args)` via token unget. */
            if (tcc_state->cpp
                && l == VT_LOCAL
                && (btype.t & VT_BTYPE) == VT_STRUCT
                && btype.ref
                && tok >= TOK_UIDENT
                && cpp_find_ctor_field(btype.ref)) {
                int saved_var_tok = tok;
                int obj_r = VT_LVAL | VT_LOCAL;
                next();
                if (tok == '(') {
                    next(); /* peek first token inside the parens */
                    if (tok != ')') {
                        /* `Foo f(args);` -> `f.Foo(args);` (local) */
                        Sym *ctor_field = cpp_find_ctor_field(btype.ref);
                        int ctor_tok_v = ctor_field->v & ~SYM_FIELD;
                        decl_initializer_alloc(&type, &ad, obj_r,
                                               0, saved_var_tok, 0);
                        /* Rebuild the stream `var . Ctor ( <args...> )`.
                           unget is LIFO and saves the current tok (the first
                           argument), so push in reverse so next() yields
                           var_tok, '.', ctor_tok, '(', first_arg, ... */
                        unget_tok('(');
                        unget_tok(ctor_tok_v);
                        unget_tok('.');
                        unget_tok(saved_var_tok);
                        expr_eq();
                        vpop();
                        if (tok == ',') {
                            next();
                            continue;
                        }
                        skip(';');
                        break;
                    }
                    /* `Foo f();` is a function declaration (most vexing
                       parse): restore the stream and fall through. */
                    unget_tok('(');
                    unget_tok(saved_var_tok);
                } else if ((tok == ';' || tok == ',')
                           && !(btype.t & (VT_STATIC | VT_EXTERN | VT_TYPEDEF))
                           && cpp_class_has_default_ctor(btype.ref)) {
                    /* FEAT-4F: `Foo f;` with a user default ctor is
                       rewritten into `f.Foo()` (same unget trick as 4B;
                       the current ';' or ',' stays after the ')'). */
                    Sym *ctor_field = cpp_find_ctor_field(btype.ref);
                    int ctor_tok_v = ctor_field->v & ~SYM_FIELD;
                    decl_initializer_alloc(&type, &ad, obj_r,
                                           0, saved_var_tok, 0);
                    unget_tok(')');
                    unget_tok('(');
                    unget_tok(ctor_tok_v);
                    unget_tok('.');
                    unget_tok(saved_var_tok);
                    expr_eq();
                    vpop();
                    if (tok == ',') {
                        next();
                        continue;
                    }
                    skip(';');
                    break;
                } else {
                    unget_tok(saved_var_tok);
                }
            }

            type_decl(&type, &ad, &v, TYPE_DIRECT);
#if 0
            {
                char buf[500];
                type_to_str(buf, sizeof(buf), &type, get_tok_str(v, NULL));
                printf("type = '%s'\n", buf);
            }
#endif
            if ((type.t & VT_BTYPE) == VT_FUNC) {
                if ((type.t & VT_STATIC) && (l != VT_CONST))
                    tcc_error("�t�@�C���X�R�[�v�O�̊֐��� static �ɂł��܂���");
                /* if old style function prototype, we accept a
                   declaration list */
                sym = type.ref;
                if (sym->f.func_type == FUNC_OLD && l == VT_CONST) {
                    func_vt = type;
                    decl(VT_CMP);
                }

                if ((type.t & (VT_EXTERN | VT_INLINE)) == (VT_EXTERN | VT_INLINE)) {
                    /* always_inline functions must be handled as if they
                       don't generate multiple global defs, even if extern
                       inline, i.e. GNU inline semantics for those.  Rewrite
                       them into static inline.  */
                    if (tcc_state->gnu89_inline || sym->f.func_alwinl)
                        type.t = (type.t & ~VT_EXTERN) | VT_STATIC;
                    else
                        type.t &= ~VT_INLINE; /* always compile otherwise */
                }

            }
            else if (oldint) {
                tcc_warning("�^�͊���� int �ɂȂ�܂�");
            }

            if (gnu_ext && (tok == TOK_ASM1 || tok == TOK_ASM2 || tok == TOK_ASM3)) {
                ad.asm_label = asm_label_instr();
                /* parse one last attribute list, after asm label */
                parse_attribute(&ad);
#if 0
                /* gcc does not allow __asm__("label") with function definition,
                   but why not ... */
                if (tok == '{')
                    expect(";");
#endif
            }

#ifdef TCC_TARGET_PE
            if (ad.a.dllimport || ad.a.dllexport) {
                if (type.t & VT_STATIC)
                    tcc_error("static �w��� DLL �����P�[�W�͕��p�ł��܂���");
                if (type.t & VT_TYPEDEF) {
                    tcc_warning("'%s' attribute ignored for typedef",
                        ad.a.dllimport ? (ad.a.dllimport = 0, "dllimport") :
                        (ad.a.dllexport = 0, "dllexport"));
                }
                else if (ad.a.dllimport) {
                    if ((type.t & VT_BTYPE) == VT_FUNC)
                        ad.a.dllimport = 0;
                    else
                        type.t |= VT_EXTERN;
                }
            }
#endif
            if (tok == '{'
                || (tcc_state->cpp && tok == ':'
                    && (type.t & VT_BTYPE) == VT_FUNC
                    && cpp_qualified_class)) {
                Sym *qclass;
                int sym_tok;

                if (l != VT_CONST)
                    tcc_error("���[�J���֐����g�p�ł��܂���");
                if ((type.t & VT_BTYPE) != VT_FUNC)
                    expect("function definition");

                /* reject abstract declarators in function definition
                   make old style params without decl have int type */
                sym = type.ref;
                while ((sym = sym->next) != NULL) {
                    if (!(sym->v & ~SYM_FIELD))
                        expect("identifier");
                    if (sym->type.t == VT_VOID)
                        sym->type = int_type;
                }

                /* apply post-declaraton attributes */
                merge_funcattr(&type.ref->f, &ad.f);

                /* put function symbol */
                type.t &= ~VT_EXTERN;
                qclass = cpp_qualified_class;
                sym_tok = v;
                if (qclass
                    && (v & ~SYM_FIELD) == (qclass->v & ~SYM_STRUCT)) {
                    int ct = cpp_ctor_name_tok(v & ~SYM_FIELD);
                    if (ct)
                        sym_tok = ct;
                }
                sym = external_sym(sym_tok, &type, 0, &ad);
                if ((type.t & VT_BTYPE) == VT_FUNC)
                    cpp_set_func_mangle_label(sym, &type);
                if (qclass) {
                    sym->parent_class = qclass;
                    cpp_inherit_decl_defaults(sym);
                    cpp_qualified_class = NULL;
                }
                if (tok == ':')
                    cpp_save_mem_init_list(sym);

                /* static inline functions are just recorded as a kind
                   of macro. Their code will be emitted at the end of
                   the compilation unit only if they are used */
                if (sym->type.t & VT_INLINE) {
                    struct InlineFunc* fn;
                    fn = tcc_malloc(sizeof * fn + strlen(file->filename));
                    strcpy(fn->filename, file->filename);
                    fn->sym = sym;
                    dynarray_add(&tcc_state->inline_fns,
                        &tcc_state->nb_inline_fns, fn);
                    skip_or_save_block(&fn->func_str);
                }
                else {
                    /* compute text section */
                    cur_text_section = ad.section;
                    if (!cur_text_section)
                        cur_text_section = text_section;
                    else if (cur_text_section->sh_num > bss_section->sh_num)
                        cur_text_section->sh_flags = text_section->sh_flags;
                    gen_function(sym);
                }
                break;
            }
            else {
                if (l == VT_CMP) {
                    /* find parameter in function parameter list */
                    for (sym = func_vt.ref->next; sym; sym = sym->next)
                        if ((sym->v & ~SYM_FIELD) == v)
                            goto found;
                    tcc_error("�p�����[�^ '%s' �̐錾�ł����A���̂悤�ȃp�����[�^�͂���܂���",
                        get_tok_str(v, NULL));
                found:
                    if (type.t & VT_STORAGE) /* 'register' is okay */
                        tcc_error("'%s' �ɃX�g���[�W�N���X���w�肳��Ă��܂�",
                            get_tok_str(v, NULL));
                    if (sym->type.t != VT_VOID)
                        tcc_error("�p�����[�^ '%s' �̍Ē�`�ł�",
                            get_tok_str(v, NULL));
                    convert_parameter_type(&type);
                    sym->type = type;
                }
                else if (type.t & VT_TYPEDEF) {
                    /* save typedefed type  */
                    /* XXX: test storage specifiers ? */
                    sym = sym_find(v);
                    if (sym && sym->sym_scope == local_scope) {
                        if (!is_compatible_types(&sym->type, &type)
                            || !(sym->type.t & VT_TYPEDEF))
                            tcc_error("'%s' �̍Ē�`�͌݊���������܂���",
                                get_tok_str(v, NULL));
                        sym->type = type;
                    }
                    else {
                        sym = sym_push(v, &type, 0, 0);
                    }
                    sym->a = ad.a;
                    if ((type.t & VT_BTYPE) == VT_FUNC) {
                        merge_funcattr(&sym->type.ref->f, &ad.f);
                        cpp_set_func_mangle_label(sym, &type);
                    }
                    if (debug_modes)
                        tcc_debug_typedef(tcc_state, sym);
                }
                else if ((type.t & VT_BTYPE) == VT_VOID
                    && !(type.t & VT_EXTERN)) {
                    tcc_error("void �^�̃I�u�W�F�N�g��錾���邱�Ƃ͂ł��܂���");
                }
                else {
                    r = 0;
                    if ((type.t & VT_BTYPE) == VT_FUNC) {
                        /* external function definition */
                        /* specific case for func_call attribute */
                        merge_funcattr(&type.ref->f, &ad.f);
                    }
                    else if (!(type.t & VT_ARRAY)) {
                        /* not lvalue if array */
                        r |= VT_LVAL;
                    }
                    has_init = (tok == '=');
                    if (has_init && (type.t & VT_VLA))
                        tcc_error("�ϒ��z��͏������ł��܂���");

                    if (((type.t & VT_EXTERN) && (!has_init || l != VT_CONST))
                        || (type.t & VT_BTYPE) == VT_FUNC
                        /* as with GCC, uninitialized global arrays with no size
                           are considered extern: */
                        || ((type.t & VT_ARRAY) && !has_init
                            && l == VT_CONST && type.ref->c < 0)
                        ) {
                        /* external variable or function */
                        type.t |= VT_EXTERN;
                        sym = external_sym(v, &type, r, &ad);
                        if ((type.t & VT_BTYPE) == VT_FUNC)
                            cpp_set_func_mangle_label(sym, &type);
                        if (cpp_qualified_class && (type.t & VT_BTYPE) == VT_FUNC) {
                            sym->parent_class = cpp_qualified_class;
                            cpp_qualified_class = NULL;
                        }
                    }
                    else {
                        if (l == VT_CONST || (type.t & VT_STATIC))
                            r |= VT_CONST;
                        else
                            r |= VT_LOCAL;
                        if (has_init)
                            next();
                        else if (l == VT_CONST)
                            /* uninitialized global variables may be overridden */
                            type.t |= VT_EXTERN;
                        decl_initializer_alloc(&type, &ad, r, has_init, v, l == VT_CONST);
                    }

                    if (ad.alias_target && l == VT_CONST) {
                        /* Aliases need to be emitted when their target symbol
                           is emitted, even if perhaps unreferenced.
                           We only support the case where the base is already
                           defined, otherwise we would need deferring to emit
                           the aliases until the end of the compile unit.  */
                        esym = elfsym(sym_find(ad.alias_target));
                        if (!esym)
                            tcc_error("��s���� __alias__ �����̓T�|�[�g����Ă��܂���");
                        put_extern_sym2(sym_find(v), esym->st_shndx,
                            esym->st_value, esym->st_size, 1);
                    }
                }
                if (tok != ',') {
                    if (l == VT_JMP)
                        return 1;
                    skip(';');
                    break;
                }
                next();
            }
        }
        if (decl_once_flag)
            break;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
#undef gjmp_addr
#undef gjmp
/* ------------------------------------------------------------------------- */

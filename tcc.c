/*
 *  TCC - Tiny C Compiler
 * 
 *  Copyright (c) 2001-2004 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef ONE_SOURCE
# define ONE_SOURCE 1
#endif

#include "tcc.h"
#if ONE_SOURCE
# include "libtcc.c"
#endif
#include "tcctools.c"

static const char help[] =
    "Tiny C Compiler "TCC_VERSION" - Copyright (C) 2001-2006 Fabrice Bellard\n"
    "使い方: tcc [オプション...] [-o 出力ファイル] [-c] 入力ファイル...\n"
    "       tcc [オプション...] -run 入力ファイル (または --) [引数...]\n"
    "一般オプション:\n"
    "  -c           コンパイルのみ - オブジェクトファイルを生成する\n"
    "  -o outfile   出力ファイル名を指定する\n"
    "  -run         コンパイルしたソースを実行する\n"
    "  -fflag       フラグを設定またはリセットする（'no-' プレフィックスでリセット）(詳細は tcc -hh)\n"
    "  -Wwarning    警告を設定またはリセットする（'no-' プレフィックスでリセット）(詳細は tcc -hh)\n"
    "  -w           すべての警告を無効にする\n"
    "  -v --version バージョンを表示する\n"
    "  -vv          検索パスや読み込まれたファイルを表示する\n"
    "  -h -hh       簡易/詳細ヘルプを表示する\n"
    "  -bench       コンパイル統計を表示する\n"
    "  -            標準入力を入力ファイルとして使用する\n"
    "  @listfile    リストファイルから引数を読み込む\n"
    "プリプロセッサオプション:\n"
    "  -Idir        インクルードパス 'dir' を追加する\n"
    "  -Dsym[=val]  シンボル 'sym' を値 'val' で定義する\n"
    "  -Usym        シンボル 'sym' の定義を解除する\n"
    "  -E           前処理のみを実行する\n"
    "  -nostdinc    標準のシステムインクルードパスを使用しない\n"
    "リンカーオプション:\n"
    "  -Ldir        ライブラリパス 'dir' を追加する\n"
    "  -llib        動的または静的ライブラリ 'lib' とリンクする\n"
    "  -nostdlib    標準のCRTおよびライブラリとリンクしない\n"
    "  -r           再配置可能オブジェクトファイルを生成する\n"
    "  -rdynamic    すべてのグローバルシンボルを動的リンカに公開する\n"
    "  -shared      共有ライブラリ/DLL を生成する\n"
    "  -soname      実行時に使用する共有ライブラリ名を設定する\n"
    "  -Wl,-opt[=val]  リンカオプションを設定する (詳細は tcc -hh)\n"
    "デバッガーオプション:\n"
    "  -g           stab形式のランタイムデバッグ情報を生成する\n"
    "  -gdwarf[-x]  DWARF形式のランタイムデバッグ情報を生成する\n"
#ifdef TCC_TARGET_PE
    "  -g.pdb       .pdb デバッグデータベースを作成する\n"
#endif
#ifdef CONFIG_TCC_BCHECK
    "  -b           組み込みのメモリ/境界チェックでコンパイルする (暗黙に -g を含む)\n"
#endif
#ifdef CONFIG_TCC_BACKTRACE
    "  -bt[N]       バックトレース（スタックダンプ）サポートをリンクする [最大 N 呼び出し元を表示]\n"
#endif
    "その他のオプション:\n"
    "  -std=version __STDC_VERSION__ を version (例: c11/gnu11) に設定する\n"
    "  -x[c|a|b|n]  次の入力ファイルの種別を指定する (C, ASM, BIN, NONE)\n"
    "  -Bdir        tcc のプライベートなインクルード/ライブラリディレクトリを設定する\n"
    "  -M[M]D       Makefile 用の依存関係ファイルを生成する [システムファイルを無視]\n"
    "  -M[M]        上と同じだが他の出力は生成しない\n"
    "  -MF file     依存関係ファイル名を指定する\n"
#if defined(TCC_TARGET_I386) || defined(TCC_TARGET_X86_64)
    "  -m32/64      i386/x86_64 へのクロスコンパイルを委譲する\n"
#endif
    "ツール:\n"
    "  ライブラリ作成  : tcc -ar [crstvx] lib [files]\n"
#ifdef TCC_TARGET_PE
    "  .def ファイル作成 : tcc -impdef lib.dll [-v] [-o lib.def]\n"
#endif
    ;

static const char help2[] =
    "Tiny C Compiler "TCC_VERSION" - 詳細オプション\n"
    "特別なオプション:\n"
    "  -P -P1                        -E と共に: #line 出力を無効化または代替出力にする\n"
    "  -dD -dM                       -E と共に: #define ディレクティブを出力する\n"
    "  -pthread                      -D_REENTRANT と -lpthread を指定したのと同等\n"
    "  -On                           n > 0 の場合、-D__OPTIMIZE__ と同等\n"
    "  -Wp,-opt                      -opt と同じ効果\n"
    "  -include file                 各入力ファイルの前に 'file' をインクルードする\n"
    "  -nostdlib                     標準のCRT/ライブラリとリンクしない\n"
    "  -isystem dir                  指定したディレクトリをシステムインクルードパスに追加する\n"
    "  -static                       静的ライブラリにリンクする（推奨されない）\n"
    "  -dumpversion                  バージョンを表示する\n"
    "  -print-search-dirs            検索パスを表示する\n"
    "  -dt                           -run/-E と共に: 'test_...' マクロを自動定義する\n"
    "無視されるオプション:\n"
    "  -arch -C --param -pedantic -pipe -s -traditional\n"
    "-W[no-]... 警告:\n"
    "  all                           いくつかの警告(*)を有効にする\n"
    "  error[=warning]               最初の警告で停止する（任意または指定可）\n"
    "  write-strings                 文字列を const として扱う\n"
    "  unsupported                   無視されたオプションやプラグマ等に対して警告する\n"
    "  implicit-function-declaration プロトタイプがない場合に警告する(*)\n"
    "  discarded-qualifiers          const などが除去された場合に警告する(*)\n"
    "-f[no-]... フラグ:\n"
    "  unsigned-char                 デフォルトの char を符号なしにする\n"
    "  signed-char                   デフォルトの char を符号ありにする\n"
    "  common                        bss の代わりに common セクションを使用する\n"
    "  leading-underscore            外部シンボル名を修飾する\n"
    "  ms-extensions                 構造体内で匿名構造体を許可する\n"
    "  dollars-in-identifiers        C 記号内で '$' を許可する\n"
    "  reverse-funcargs              関数引数を右から左へ評価する\n"
    "  gnu89-inline                  'extern inline' を 'static inline' と同様に扱う\n"
    "  asynchronous-unwind-tables    eh_frame セクションを作成する\n"
    "  test-coverage                 コードカバレッジ用のコードを生成する\n"
    "-m... ターゲット固有オプション:\n"
    "  ms-bitfields                  MSVC 互換のビットフィールドレイアウトを使用する\n"
#ifdef TCC_TARGET_ARM
    "  float-abi                     ARM でのハード/ソフトFP ABI を指定する\n"
#endif
#ifdef TCC_TARGET_X86_64
    "  no-sse                        x86_64 上で SSE による浮動小数点を無効にする\n"
#endif
    "-Wl,... リンカーオプション:\n"
    "  -nostdlib                     標準ライブラリ検索パスを使用しない\n"
    "  -[no-]whole-archive           ライブラリを完全に/必要に応じて読み込む\n"
    "  -export-all-symbols           -rdynamic と同等\n"
    "  -export-dynamic               -rdynamic と同等\n"
    "  -image-base= -Ttext=          実行ファイルのベースアドレスを設定する\n"
    "  -section-alignment=           実行ファイルのセクション配置を設定する\n"
#ifdef TCC_TARGET_PE
    "  -file-alignment=              PE ファイルのアライメントを設定する\n"
    "  -stack=                       PE のスタック予約サイズを設定する\n"
    "  -large-address-aware          関連する PE オプションを設定する\n"
    "  -subsystem=[console/windows]  PE サブシステムを設定する\n"
    "  -oformat=[pe-* binary]        実行可能ファイルの出力形式を設定する\n"
    "定義済みマクロ:\n"
    "  tcc -E -dM - < nul\n"
#else
    "  -rpath=                       動的ライブラリの検索パスを設定する\n"
    "  -enable-new-dtags             DT_RPATH の代わりに DT_RUNPATH を設定する\n"
    "  -soname=                      DT_SONAME (ELF タグ) を設定する\n"
#if defined(TCC_TARGET_MACHO)
    "  -install_name=                DT_SONAME を設定する（macOS の soname 別名）\n"
#else
    "  -Ipath, -dynamic-linker=path  ELF インタプリタを path に設定する\n"
#endif
    "  -Bsymbolic                    DT_SYMBOLIC ELF タグを設定する\n"
    "  -oformat=[elf32/64-* binary]  実行ファイル出力形式を設定する\n"
    "  -init= -fini= -Map= -as-needed -O -z= (無視されます)\n"
    "定義済みマクロ:\n"
    "  tcc -E -dM - < /dev/null\n"
#endif
    "詳細はマニュアルを参照してください。\n"
    ;

static const char version[] =
    "tcc version "TCC_VERSION
#ifdef TCC_GITHASH
    " "TCC_GITHASH
#endif
    " ("
#ifdef TCC_TARGET_I386
        "i386"
#elif defined TCC_TARGET_X86_64
        "x86_64"
#elif defined TCC_TARGET_C67
        "C67"
#elif defined TCC_TARGET_ARM
        "ARM"
# ifdef TCC_ARM_EABI
        " eabi"
#  ifdef TCC_ARM_HARDFLOAT
        "hf"
#  endif
# endif
#elif defined TCC_TARGET_ARM64
        "AArch64"
#elif defined TCC_TARGET_RISCV64
        "riscv64"
#endif
#ifdef TCC_TARGET_PE
        " Windows"
#elif defined(TCC_TARGET_MACHO)
        " Darwin"
#elif TARGETOS_FreeBSD || TARGETOS_FreeBSD_kernel
        " FreeBSD"
#elif TARGETOS_OpenBSD
        " OpenBSD"
#elif TARGETOS_NetBSD
        " NetBSD"
#else
        " Linux"
#endif
    ")\n"
    ;

static void print_dirs(const char *msg, char **paths, int nb_paths)
{
    int i;
    printf("%s:\n%s", msg, nb_paths ? "" : "  -\n");
    for(i = 0; i < nb_paths; i++)
        printf("  %s\n", paths[i]);
}

static void print_search_dirs(TCCState *s)
{
    printf("install: %s\n", s->tcc_lib_path);
    /* print_dirs("programs", NULL, 0); */
    print_dirs("include", s->sysinclude_paths, s->nb_sysinclude_paths);
    print_dirs("libraries", s->library_paths, s->nb_library_paths);
    printf("libtcc1:\n  %s/%s\n", s->library_paths[0], CONFIG_TCC_CROSSPREFIX TCC_LIBTCC1);
#ifdef TCC_TARGET_UNIX
    print_dirs("crt", s->crt_paths, s->nb_crt_paths);
    printf("elfinterp:\n  %s\n",  DEFAULT_ELFINTERP(s));
#endif
}

static void set_environment(TCCState *s)
{
    char * path;

    path = getenv("C_INCLUDE_PATH");
    if(path != NULL) {
        tcc_add_sysinclude_path(s, path);
    }
    path = getenv("CPATH");
    if(path != NULL) {
        tcc_add_include_path(s, path);
    }
    path = getenv("LIBRARY_PATH");
    if(path != NULL) {
        tcc_add_library_path(s, path);
    }
}

static char *default_outputfile(TCCState *s, const char *first_file)
{
    char buf[1024];
    char *ext;
    const char *name = "a";

    if (first_file && strcmp(first_file, "-"))
        name = tcc_basename(first_file);
    if (strlen(name) + 4 >= sizeof buf)
        name = "a";
    strcpy(buf, name);
    ext = tcc_fileextension(buf);
#ifdef TCC_TARGET_PE
    if (s->output_type == TCC_OUTPUT_DLL)
        strcpy(ext, ".dll");
    else
    if (s->output_type == TCC_OUTPUT_EXE)
        strcpy(ext, ".exe");
    else
#endif
    if ((s->just_deps || s->output_type == TCC_OUTPUT_OBJ) && !s->option_r && *ext)
        strcpy(ext, ".o");
    else
        strcpy(buf, "a.out");
    return tcc_strdup(buf);
}

static unsigned getclock_ms(void)
{
#ifdef _WIN32
    return GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000 + (tv.tv_usec+500)/1000;
#endif
}

#ifdef _WIN32
// C1 (crash-prevention plan): last-resort crash net.  A compiler bug must
// never kill the process silently - BUG-35 died with 0xC00000FD, no output
// at all, and Windows' crash handling then kept the zombie process holding
// a lock on dev\tcc.exe, breaking every following build.  This handler
// turns stack overflow / access violation inside the COMPILER into a
// printed diagnostic + regular exit(1), and terminates the process itself
// so no crash-reporting machinery ever grabs it.
// It stays disarmed while tcc -run executes user code: tccrun.c installs
// SetUnhandledExceptionFilter for that, and a vectored handler would fire
// first and steal the user program's exceptions from it.
static volatile LONG tcc_crash_net_armed;

static LONG CALLBACK tcc_crash_net_veh(EXCEPTION_POINTERS *ep)
{
    // Static storage throughout: on stack overflow only one guard page of
    // stack remains, so the handler must not need any.  wsprintfA +
    // WriteFile proved to work in that state during the BUG-35
    // investigation; the CRT (fprintf/exit) is deliberately avoided.
    static void *frames[40];
    static char msg[2048];
    DWORD code, written;
    USHORT nf, i;
    int pos;
    HANDLE h;

    if (!tcc_crash_net_armed)
        return EXCEPTION_CONTINUE_SEARCH;
    code = ep->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_STACK_OVERFLOW && code != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    h = GetStdHandle(STD_ERROR_HANDLE);
    pos = wsprintfA(msg,
        "tcc: internal error: %s (this is a compiler bug, not an error in "
        "the source being compiled)\n",
        code == EXCEPTION_STACK_OVERFLOW
            ? "stack overflow (runaway recursion)" : "invalid memory access");
    WriteFile(h, msg, pos, &written, NULL);
    // Raw return addresses: with the linker .map (base below) they resolve
    // to functions even without a debugger attached.
    nf = RtlCaptureStackBackTrace(0, 40, frames, NULL);
    for (i = 0; i < nf; i++) {
        pos = wsprintfA(msg, "tcc:   #%02u %p\n", i, frames[i]);
        WriteFile(h, msg, pos, &written, NULL);
    }
    pos = wsprintfA(msg, "tcc:   module base %p\n",
                    (void *)GetModuleHandleA(NULL));
    WriteFile(h, msg, pos, &written, NULL);
    // exit(1), same as tcc_error: build scripts see an ordinary failure,
    // and no WER/zombie process is left holding the executable open.
    TerminateProcess(GetCurrentProcess(), 1);
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

int main(int argc0, char **argv0)
{
    TCCState *s, *s1;
    int ret, opt, n = 0, t = 0, done;
    unsigned start_time = 0, end_time = 0;
    const char *first_file;
    int argc; char **argv;
    FILE *ppfp = stdout;

#ifdef _WIN32
    AddVectoredExceptionHandler(1, tcc_crash_net_veh);
    tcc_crash_net_armed = 1;
#endif
redo:
    argc = argc0, argv = argv0;
    s = s1 = tcc_new();
    opt = tcc_parse_args(s, &argc, &argv);
    if (opt < 0)
        return 1;

    if (n == 0) {
        if (opt == OPT_HELP) {
            fputs(help, stdout);
            if (!s->verbose)
                return 0;
            ++opt;
        }
        if (opt == OPT_HELP2) {
            fputs(help2, stdout);
            return 0;
        }
        if (opt == OPT_M32 || opt == OPT_M64)
            return tcc_tool_cross(s, argv, opt);
        if (s->verbose)
            printf("%s", version);
        if (opt == OPT_AR)
            return tcc_tool_ar(s, argc, argv);
#ifdef TCC_TARGET_PE
        if (opt == OPT_IMPDEF)
            return tcc_tool_impdef(s, argc, argv);
#endif
        if (opt == OPT_V)
            return 0;
        if (opt == OPT_PRINT_DIRS) {
            /* initialize search dirs */
            set_environment(s);
            tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
            print_search_dirs(s);
            return 0;
        }

        if (s->nb_files == 0) {
            tcc_error_noabort("入力ファイルがありません");
        } else if (s->output_type == TCC_OUTPUT_PREPROCESS) {
            if (s->outfile && 0!=strcmp("-",s->outfile)) {
                ppfp = fopen(s->outfile, "wb");
                if (!ppfp)
                    tcc_error_noabort("書き込みできません: '%s'", s->outfile);
            }
        } else if (s->output_type == TCC_OUTPUT_OBJ && !s->option_r) {
            if (s->nb_libraries)
                tcc_error_noabort("-c と共にライブラリを指定できません");
            else if (s->nb_files > 1 && s->outfile)
                tcc_error_noabort("-c と複数ファイル時に出力ファイルを指定できません");
        }
        if (s->nb_errors)
            return 1;
        if (s->do_bench)
            start_time = getclock_ms();
    }

    set_environment(s);
    if (s->output_type == 0)
        s->output_type = TCC_OUTPUT_EXE;
    tcc_set_output_type(s, s->output_type);
    s->ppfp = ppfp;

    if ((s->output_type == TCC_OUTPUT_MEMORY
      || s->output_type == TCC_OUTPUT_PREPROCESS)
        && (s->dflag & 16)) { /* -dt option */
        if (t)
            s->dflag |= 32;
        s->run_test = ++t;
        if (n)
            --n;
    }

    /* compile or add each files or library */
    first_file = NULL;
    do {
        struct filespec *f = s->files[n];
        s->filetype = f->type;
        if (f->type & AFF_TYPE_LIB) {
            ret = tcc_add_library(s, f->name);
        } else {
            if (1 == s->verbose)
                printf("-> %s\n", f->name);
            if (!first_file)
                first_file = f->name;
            ret = tcc_add_file(s, f->name);
        }
    } while (++n < s->nb_files
            && 0 == ret
            && (s->output_type != TCC_OUTPUT_OBJ || s->option_r));

    if (s->do_bench)
        end_time = getclock_ms();

    if (s->run_test) {
        t = 0;
    } else if (s->output_type == TCC_OUTPUT_PREPROCESS) {
        ;
    } else if (0 == ret) {
        if (s->output_type == TCC_OUTPUT_MEMORY) {
#ifdef TCC_IS_NATIVE
#ifdef _WIN32
            // C1: user code is about to run in-process; its exceptions
            // belong to tccrun.c's unhandled-exception filter, and a
            // vectored handler would see them first - stand down.
            tcc_crash_net_armed = 0;
#endif
            ret = tcc_run(s, argc, argv);
#ifdef _WIN32
            tcc_crash_net_armed = 1;
#endif
#endif
        } else {
            if (!s->outfile)
                s->outfile = default_outputfile(s, first_file);
            if (!s->just_deps)
                ret = tcc_output_file(s, s->outfile);
            if (!ret && s->gen_deps)
                gen_makedeps(s, s->outfile, s->deps_outfile);
        }
    }

    done = 1;
    if (t)
        done = 0; /* run more tests with -dt -run */
    else if (ret) {
        if (s->nb_errors)
            ret = 1;
        /* else keep the original exit code from tcc_run() */
    } else if (n < s->nb_files)
        done = 0; /* compile more files with -c */
    else if (s->do_bench)
        tcc_print_stats(s, end_time - start_time);

    tcc_delete(s);

    if (!done)
        goto redo;
    if (ppfp && ppfp != stdout)
        fclose(ppfp);
    return ret;
}

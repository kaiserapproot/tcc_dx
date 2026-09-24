/* doc_source_refs_check.c - audit the source references in the C++ feature guide.
 *
 * Usage: doc_source_refs_check <repo-root>
 *
 * The guide cites implementation sites as `tccgen.c:1234` or `tccgen.c:12-34`,
 * often followed by the function name in backticks.  A bulk line-number update
 * is easy to get wrong (the first pass shifted only the start of every range
 * and left the end behind, producing impossible A>B spans), so every reference
 * is checked here instead of by eye:
 *
 *   - A <= B for a range
 *   - A and B are inside the cited file
 *   - when a `name()` follows the reference, that name really appears in the
 *     cited span (or just after it, for a single-line citation, or on the
 *     enclosing definition above it - a citation often points inside a
 *     function and names the function it sits in)
 *
 * Prints:
 *   DOC_SOURCE_REFS_TOTAL=<n>
 *   DOC_SOURCE_REFS_INVALID_RANGE=<n>
 *   DOC_SOURCE_REFS_OUT_OF_FILE=<n>
 *   DOC_SOURCE_REFS_NAME_CHECKED=<n>
 *   DOC_SOURCE_REFS_NAME_MISMATCH=<n>
 *   DOC_SOURCE_REFS_EXTERNAL=<n>      (files that live in another repo)
 *
 * EXTERNAL means the cited file is not under <repo-root>, which for this guide
 * is the amateras tree (base_inc and test/tcc).  Those get the range check but
 * not the bounds or name check, because the file is not here to read.
 * and exits non-zero if any invalid range, out-of-file or name mismatch is found.
 *
 * The guide's file name is not ASCII, so it is located by the ASCII marker
 * "doc-sample-qualification" the same way doc_sample_extract.c does it.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 8192
#define MAX_SRC_LINES 32768
#define DOC_MARKER "doc-sample-qualification"
#define NAME_WINDOW 40        /* how far past a single-line citation to look */
#define ENCLOSING_WINDOW 1200 /* how far back to look for the enclosing function */

static int g_total, g_bad_range, g_out_of_file, g_name_checked, g_name_bad, g_external;

/* ---- cached source file ------------------------------------------------- */
static char  g_cached_name[256];
static char *g_cached[MAX_SRC_LINES];
static int   g_cached_n = -1;   /* -1 = nothing cached, -2 = file not found */

static void free_cached(void)
{
    int i;
    if (g_cached_n > 0)
        for (i = 0; i < g_cached_n; i++)
            free(g_cached[i]);
    g_cached_n = -1;
    g_cached_name[0] = '\0';
}

/* Load <root>\<name>; returns line count, or -1 when the file is not here.
 * <name> may be a repo-relative path, so normalise the separators. */
static int load_source(const char *root, const char *name)
{
    char path[1024];
    char *p;
    FILE *f;
    char line[MAX_LINE];

    if (g_cached_n != -1 && strcmp(g_cached_name, name) == 0)
        return (g_cached_n == -2) ? -1 : g_cached_n;
    free_cached();
    strcpy(g_cached_name, name);

    sprintf(path, "%s\\%s", root, name);
    for (p = path; *p; p++)
        if (*p == '/') *p = '\\';
    f = fopen(path, "rb");
    if (!f) { g_cached_n = -2; return -1; }
    g_cached_n = 0;
    while (g_cached_n < MAX_SRC_LINES && fgets(line, sizeof line, f)) {
        size_t n = strlen(line);
        while (n > 0 && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = '\0';
        g_cached[g_cached_n] = (char *)malloc(n + 1);
        strcpy(g_cached[g_cached_n], line);
        g_cached_n++;
    }
    fclose(f);
    return g_cached_n;
}

static int is_name_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9') || c == '_';
}

/* Which suffixes count as a source file.  .cpp / .hpp / .cc / .cxx are here
 * because the guide cites a .cpp test (test/tcc/vec_quat/vec_quat_matrix_cpp.cpp);
 * with only .c / .h those three references were skipped silently, so "every
 * reference is audited" was not actually true. */
static int has_source_ext(const char *name)
{
    static const char *exts[] = { ".c", ".h", ".cpp", ".hpp", ".cc", ".cxx", 0 };
    size_t nlen = strlen(name);
    int i;
    for (i = 0; exts[i]; i++) {
        size_t elen = strlen(exts[i]);
        if (nlen > elen && strcmp(name + nlen - elen, exts[i]) == 0)
            return 1;
    }
    return 0;
}

/* A source file name holds name chars, dots, dashes and path separators.
 * Separators are kept so a citation like base_inc/cross_base.h:29 is read as a
 * path: dropping them would turn a repo-relative path into a bare file name and
 * a future dev/foo/bar.h:123 would be classed external and skip the checks.
 * Scan backwards from the ':' that starts the line number. */
static int grab_filename(const char *line, int colon, char *out, size_t outsz)
{
    int s = colon;
    int len;
    while (s > 0) {
        char c = line[s-1];
        if (is_name_char(c) || c == '.' || c == '-' || c == '/' || c == '\\') s--;
        else break;
    }
    len = colon - s;
    if (len <= 2 || (size_t)len + 1 > outsz) return 0;
    memcpy(out, line + s, len);
    out[len] = '\0';
    return has_source_ext(out);
}

/* After the reference, an identifier in backticks like `foo()` may follow. */
static int grab_func(const char *line, int from, char *out, size_t outsz)
{
    int i = from;
    int guard = 0;
    while (line[i] && guard < 8) {
        if (line[i] == '`') {
            int s = i + 1, e = s;
            while (is_name_char(line[e])) e++;
            if (e > s && line[e] == '(' && (size_t)(e - s + 1) <= outsz) {
                memcpy(out, line + s, e - s);
                out[e - s] = '\0';
                return 1;
            }
            guard++;
            i = e;
            continue;
        }
        if (line[i] == '\n') break;
        i++;
        guard = guard; /* keep scanning the rest of the line */
        if (i - from > 80) break;
    }
    return 0;
}

int main(int argc, char **argv)
{
    WIN32_FIND_DATAA fd;
    HANDLE h;
    char pattern[1024], guide[1024];
    FILE *in;
    char line[MAX_LINE];
    const char *root;
    int found = 0;

    if (argc < 2) { fprintf(stderr, "usage: doc_source_refs_check <repo-root>\n"); return 2; }
    root = argv[1];

    sprintf(pattern, "%s\\*.md", root);
    h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            char cand[1024];
            FILE *f;
            int n = 0;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            sprintf(cand, "%s\\%s", root, fd.cFileName);
            f = fopen(cand, "rb");
            if (!f) continue;
            while (n < 200 && fgets(line, sizeof line, f)) {
                n++;
                if (strstr(line, DOC_MARKER)) { strcpy(guide, cand); found = 1; break; }
            }
            fclose(f);
        } while (!found && FindNextFileA(h, &fd));
        FindClose(h);
    }
    if (!found) { fprintf(stderr, "guide not found under %s\n", root); return 2; }

    in = fopen(guide, "rb");
    if (!in) { fprintf(stderr, "cannot open the guide\n"); return 2; }

    while (fgets(line, sizeof line, in)) {
        int i = 0;
        while (line[i]) {
            char fname[256];
            int a, b, j, nlines;
            if (line[i] != ':' || line[i+1] < '0' || line[i+1] > '9') { i++; continue; }
            if (!grab_filename(line, i, fname, sizeof fname)) { i++; continue; }
            j = i + 1;
            a = 0;
            while (line[j] >= '0' && line[j] <= '9') { a = a * 10 + (line[j] - '0'); j++; }
            b = a;
            if (line[j] == '-' && line[j+1] >= '0' && line[j+1] <= '9') {
                j++;
                b = 0;
                while (line[j] >= '0' && line[j] <= '9') { b = b * 10 + (line[j] - '0'); j++; }
            }
            g_total++;
            if (a > b) {
                g_bad_range++;
                printf("  [INVALID RANGE] %s:%d-%d\n", fname, a, b);
            }
            nlines = load_source(root, fname);
            if (nlines < 0) {
                g_external++;
            } else {
                if (a < 1 || a > nlines || b < 1 || b > nlines) {
                    g_out_of_file++;
                    printf("  [OUT OF FILE] %s:%d-%d (file has %d lines)\n", fname, a, b, nlines);
                } else {
                    char fn[256];
                    if (grab_func(line, j, fn, sizeof fn)) {
                        int k, hit = 0;
                        int hi = (b > a) ? b : a + NAME_WINDOW;
                        if (hi > nlines) hi = nlines;
                        for (k = a; k <= hi; k++) {
                            if (strstr(g_cached[k-1], fn)) { hit = 1; break; }
                        }
                        /* A citation often points INSIDE a function and names
                         * the enclosing one, so the name is not on those lines.
                         * Accept that by looking back for the definition: a
                         * line starting at column 0 that mentions the name. */
                        if (!hit) {
                            int lo = a - ENCLOSING_WINDOW;
                            if (lo < 1) lo = 1;
                            for (k = a; k >= lo; k--) {
                                const char *s = g_cached[k-1];
                                if (s[0] == '\0' || s[0] == ' ' || s[0] == '\t') continue;
                                if (strstr(s, fn)) { hit = 1; break; }
                            }
                        }
                        g_name_checked++;
                        if (!hit) {
                            g_name_bad++;
                            printf("  [NAME MISMATCH] %s:%d-%d does not mention %s\n",
                                   fname, a, b, fn);
                        }
                    }
                }
            }
            i = j;
        }
    }
    fclose(in);
    free_cached();

    printf("DOC_SOURCE_REFS_TOTAL=%d\n", g_total);
    printf("DOC_SOURCE_REFS_INVALID_RANGE=%d\n", g_bad_range);
    printf("DOC_SOURCE_REFS_OUT_OF_FILE=%d\n", g_out_of_file);
    printf("DOC_SOURCE_REFS_NAME_CHECKED=%d\n", g_name_checked);
    printf("DOC_SOURCE_REFS_NAME_MISMATCH=%d\n", g_name_bad);
    printf("DOC_SOURCE_REFS_EXTERNAL=%d\n", g_external);

    return (g_bad_range || g_out_of_file || g_name_bad) ? 1 : 0;
}

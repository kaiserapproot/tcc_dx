/* doc_sample_extract.c - pull the runnable samples out of the C++ feature guide.
 *
 * Usage: doc_sample_extract <dir-holding-the-guide> <outdir>
 *
 * The guide's file name is not ASCII, and a batch file cannot hand such a name
 * to a child process without the console code page mangling it.  So the guide
 * is located here instead: every <dir>\*.md is scanned for the ASCII marker
 * "doc-sample-qualification" and the first hit is used.  The name then never
 * leaves this process.
 *
 * Every ```cpp / ```c fenced block that contains "int main" is written to
 * <outdir>\doc_sample_NN.cpp (or .c for a ```c block - that block is written
 * for C mode on purpose, e.g. the one that uses `class` as an identifier).
 * One line is printed per sample:
 *   <index> <first source line number> <path>
 * The last line is "TOTAL=<n>".
 *
 * Plain C so it builds with dev\tcc.exe itself; the guide claims its samples
 * are buildable, and this is what lets the harness check that.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 4096
#define MAX_BLOCK (256 * 1024)
#define DOC_MARKER "doc-sample-qualification"

static int starts_with(const char *s, const char *p)
{
    return strncmp(s, p, strlen(p)) == 0;
}

/* Does this file carry the marker? */
static int has_marker(const char *path)
{
    FILE *f;
    char line[MAX_LINE];
    int n = 0;

    f = fopen(path, "rb");
    if (!f)
        return 0;
    while (n < 200 && fgets(line, sizeof line, f)) {
        n++;
        if (strstr(line, DOC_MARKER)) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

static int find_guide(const char *dir, char *out, size_t outsz)
{
    WIN32_FIND_DATAA fd;
    HANDLE h;
    char pattern[1024];

    sprintf(pattern, "%s\\*.md", dir);
    h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return 0;
    do {
        char candidate[1024];
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        sprintf(candidate, "%s\\%s", dir, fd.cFileName);
        if (has_marker(candidate)) {
            if (strlen(candidate) + 1 > outsz)
                break;
            strcpy(out, candidate);
            FindClose(h);
            return 1;
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return 0;
}

int main(int argc, char **argv)
{
    FILE *in;
    char guide[1024];
    char line[MAX_LINE];
    char *block;
    size_t block_len;
    int in_fence = 0;
    int fence_is_code = 0;
    int fence_is_cpp = 0;
    int lineno = 0;
    int fence_start = 0;
    int index = 0;

    if (argc < 3) {
        fprintf(stderr, "usage: doc_sample_extract <dir> <outdir>\n");
        return 2;
    }
    if (!find_guide(argv[1], guide, sizeof guide)) {
        fprintf(stderr, "no .md carrying '%s' under %s\n", DOC_MARKER, argv[1]);
        return 2;
    }
    in = fopen(guide, "rb");
    if (!in) {
        fprintf(stderr, "cannot open the guide\n");
        return 2;
    }
    block = (char *)malloc(MAX_BLOCK);
    if (!block) { fclose(in); return 2; }
    block_len = 0;

    while (fgets(line, sizeof line, in)) {
        lineno++;
        if (starts_with(line, "```")) {
            if (!in_fence) {
                in_fence = 1;
                fence_start = lineno + 1;
                block_len = 0;
                fence_is_cpp = starts_with(line, "```cpp");
                fence_is_code = fence_is_cpp || starts_with(line, "```c\n")
                    || starts_with(line, "```c\r");
            } else {
                in_fence = 0;
                if (fence_is_code && block_len > 0) {
                    block[block_len] = '\0';
                    if (strstr(block, "int main")) {
                        char path[1024];
                        FILE *out;
                        index++;
                        sprintf(path, "%s\\doc_sample_%02d.%s", argv[2], index,
                                fence_is_cpp ? "cpp" : "c");
                        out = fopen(path, "wb");
                        if (!out) {
                            fprintf(stderr, "cannot write %s\n", path);
                            free(block);
                            fclose(in);
                            return 2;
                        }
                        fwrite(block, 1, block_len, out);
                        fclose(out);
                        printf("%d %d %s\n", index, fence_start, path);
                    }
                }
                fence_is_code = 0;
            }
            continue;
        }
        if (in_fence && fence_is_code) {
            size_t n = strlen(line);
            if (block_len + n + 1 < MAX_BLOCK) {
                memcpy(block + block_len, line, n);
                block_len += n;
            }
        }
    }
    printf("TOTAL=%d\n", index);
    free(block);
    fclose(in);
    return 0;
}

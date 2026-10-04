/*
 * verify_original.c - checks dxguid.c against the archive it was taken from.
 *
 * dxguid.c was written from the mingw-w64 COFF archive that used to be
 * dev\lib\libdxguid.a.  This tool reads that archive again and compares every
 * GUID with the DXGUID() lines of dxguid.c, in both directions.
 * verify_original.bat fetches the archive from git history and runs this.
 *
 * usage: verify_original <original libdxguid.a> <dxguid.c>
 * exit 0 when both sides hold the same names with the same 16 bytes.
 *
 * Archive layout relied on: an ar archive whose object members are COFF
 * x86-64 files, one section named ".rdata$<symbol>" with 16 bytes of raw data
 * per GUID (mingw declares them DECLSPEC_SELECTANY, one COMDAT section each).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_GUIDS 4096
#define NAME_MAX_LEN 128

typedef struct {
    char name[NAME_MAX_LEN];
    unsigned char b[16];
    int seen;
} guid_ent;

static guid_ent g_arch[MAX_GUIDS];
static guid_ent g_src[MAX_GUIDS];
static int g_narch;
static int g_nsrc;

static unsigned rd16(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static unsigned long rd32(const unsigned char *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8)
         | ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}

static unsigned char *read_file(const char *path, long *size)
{
    FILE *f;
    unsigned char *buf;
    long n;

    f = fopen(path, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (unsigned char *)malloc((size_t)n + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    if (n > 0 && fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fclose(f);
        free(buf);
        return NULL;
    }
    fclose(f);
    buf[n] = 0;
    *size = n;
    return buf;
}

/* One COFF object: collect every 16-byte ".rdata$<name>" section. */
static int scan_coff(const unsigned char *d, unsigned long size)
{
    unsigned nsec, opt, i;
    unsigned long symptr, nsym, strtab;
    const unsigned char *sec;

    if (size < 20)
        return 0;
    if (rd16(d) != 0x8664) {
        printf("ERROR: object member is not COFF x86-64 (machine=0x%04x)\n", rd16(d));
        return -1;
    }
    nsec = rd16(d + 2);
    symptr = rd32(d + 8);
    nsym = rd32(d + 12);
    opt = rd16(d + 16);
    strtab = symptr + nsym * 18UL;
    if (20UL + opt + (unsigned long)nsec * 40UL > size || strtab + 4 > size) {
        printf("ERROR: COFF headers run past the end of the member\n");
        return -1;
    }
    sec = d + 20 + opt;
    for (i = 0; i < nsec; i++) {
        const unsigned char *s = sec + (unsigned long)i * 40UL;
        char name[NAME_MAX_LEN];
        unsigned long rawsize = rd32(s + 16);
        unsigned long rawptr = rd32(s + 20);

        if (s[0] == '/') {
            /* long name: "/<decimal offset into the string table>" */
            char num[8];
            unsigned long off;
            size_t len;
            memcpy(num, s + 1, 7);
            num[7] = 0;
            off = strtoul(num, NULL, 10);
            if (strtab + off >= size)
                continue;
            len = strlen((const char *)d + strtab + off);
            if (len >= sizeof name)
                continue;
            memcpy(name, d + strtab + off, len + 1);
        } else {
            memcpy(name, s, 8);
            name[8] = 0;
        }
        if (strncmp(name, ".rdata$", 7) != 0 || rawsize != 16)
            continue;
        if (rawptr + 16 > size) {
            printf("ERROR: section data of %s runs past the end of the member\n", name);
            return -1;
        }
        if (g_narch >= MAX_GUIDS) {
            printf("ERROR: more than %d GUIDs in the archive\n", MAX_GUIDS);
            return -1;
        }
        strcpy(g_arch[g_narch].name, name + 7);
        memcpy(g_arch[g_narch].b, d + rawptr, 16);
        g_arch[g_narch].seen = 0;
        g_narch++;
    }
    return 0;
}

static int scan_archive(const unsigned char *buf, long size)
{
    long pos = 8;

    if (size < 8 || memcmp(buf, "!<arch>\n", 8) != 0) {
        printf("ERROR: not an ar archive\n");
        return -1;
    }
    while (pos + 60 <= size) {
        const unsigned char *h = buf + pos;
        char num[11];
        long msize;

        memcpy(num, h + 48, 10);
        num[10] = 0;
        msize = atol(num);
        if (h[58] != '`' || h[59] != '\n' || msize < 0 || pos + 60 + msize > size) {
            printf("ERROR: bad ar member header at offset %ld\n", pos);
            return -1;
        }
        /* "/" = symbol table, "//" = long-name table; "/<digits>" and plain
           names are object members. */
        if (!(h[0] == '/' && (h[1] == ' ' || h[1] == '/'))) {
            if (scan_coff(h + 60, (unsigned long)msize) != 0)
                return -1;
        }
        pos += 60 + msize;
        if (pos & 1)
            pos++;
    }
    return 0;
}

/* dxguid.c: DXGUID(name,0x........,0x....,0x....,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..); */
static int scan_source(char *text)
{
    char *line = text;

    while (line && *line) {
        char *next = strchr(line, '\n');
        if (next)
            *next++ = 0;
        if (strncmp(line, "DXGUID(", 7) == 0) {
            char *p = line + 7;
            char *comma = strchr(p, ',');
            unsigned long v[11];
            size_t len;
            int k;

            if (!comma) {
                printf("ERROR: malformed line: %s\n", line);
                return -1;
            }
            len = (size_t)(comma - p);
            if (len == 0 || len >= NAME_MAX_LEN || g_nsrc >= MAX_GUIDS) {
                printf("ERROR: bad name or too many entries: %s\n", line);
                return -1;
            }
            memcpy(g_src[g_nsrc].name, p, len);
            g_src[g_nsrc].name[len] = 0;
            p = comma + 1;
            for (k = 0; k < 11; k++) {
                char *end;
                v[k] = strtoul(p, &end, 0);
                if (end == p || (k < 10 && *end != ',') || (k == 10 && *end != ')')) {
                    printf("ERROR: malformed value %d in: %s\n", k + 1, g_src[g_nsrc].name);
                    return -1;
                }
                p = end + 1;
            }
            if (v[1] > 0xffffUL || v[2] > 0xffffUL) {
                printf("ERROR: 16-bit field out of range in: %s\n", g_src[g_nsrc].name);
                return -1;
            }
            g_src[g_nsrc].b[0] = (unsigned char)(v[0] & 0xff);
            g_src[g_nsrc].b[1] = (unsigned char)((v[0] >> 8) & 0xff);
            g_src[g_nsrc].b[2] = (unsigned char)((v[0] >> 16) & 0xff);
            g_src[g_nsrc].b[3] = (unsigned char)((v[0] >> 24) & 0xff);
            g_src[g_nsrc].b[4] = (unsigned char)(v[1] & 0xff);
            g_src[g_nsrc].b[5] = (unsigned char)((v[1] >> 8) & 0xff);
            g_src[g_nsrc].b[6] = (unsigned char)(v[2] & 0xff);
            g_src[g_nsrc].b[7] = (unsigned char)((v[2] >> 8) & 0xff);
            for (k = 0; k < 8; k++) {
                if (v[3 + k] > 0xffUL) {
                    printf("ERROR: byte out of range in: %s\n", g_src[g_nsrc].name);
                    return -1;
                }
                g_src[g_nsrc].b[8 + k] = (unsigned char)v[3 + k];
            }
            g_src[g_nsrc].seen = 0;
            g_nsrc++;
        }
        line = next;
    }
    return 0;
}

int main(int argc, char **argv)
{
    unsigned char *arch;
    unsigned char *src;
    long asize = 0, ssize = 0;
    int i, j;
    int match = 0, mismatch = 0, only_src = 0, only_arch = 0, dup_src = 0;

    if (argc != 3) {
        printf("usage: verify_original <original libdxguid.a> <dxguid.c>\n");
        return 2;
    }
    arch = read_file(argv[1], &asize);
    src = read_file(argv[2], &ssize);
    if (!arch || !src) {
        printf("ERROR: cannot read %s\n", arch ? argv[2] : argv[1]);
        return 2;
    }
    if (scan_archive(arch, asize) != 0 || scan_source((char *)src) != 0)
        return 2;

    for (i = 0; i < g_nsrc; i++) {
        for (j = 0; j < i; j++)
            if (strcmp(g_src[i].name, g_src[j].name) == 0) {
                printf("DUPLICATE_IN_SOURCE %s\n", g_src[i].name);
                dup_src++;
                break;
            }
        for (j = 0; j < g_narch; j++)
            if (strcmp(g_src[i].name, g_arch[j].name) == 0)
                break;
        if (j == g_narch) {
            printf("ONLY_IN_SOURCE %s\n", g_src[i].name);
            only_src++;
            continue;
        }
        g_arch[j].seen = 1;
        if (memcmp(g_src[i].b, g_arch[j].b, 16) == 0) {
            match++;
        } else {
            printf("VALUE_MISMATCH %s\n", g_src[i].name);
            mismatch++;
        }
    }
    for (j = 0; j < g_narch; j++)
        if (!g_arch[j].seen) {
            printf("ONLY_IN_ARCHIVE %s\n", g_arch[j].name);
            only_arch++;
        }

    printf("ARCHIVE_GUIDS=%d\n", g_narch);
    printf("SOURCE_GUIDS=%d\n", g_nsrc);
    printf("MATCH=%d\n", match);
    printf("VALUE_MISMATCH=%d\n", mismatch);
    printf("ONLY_IN_SOURCE=%d\n", only_src);
    printf("ONLY_IN_ARCHIVE=%d\n", only_arch);
    printf("DUPLICATE_IN_SOURCE=%d\n", dup_src);
    if (g_narch == 0 || mismatch || only_src || only_arch || dup_src) {
        printf("DXGUID_VERIFY=FAIL\n");
        return 1;
    }
    printf("DXGUID_VERIFY=PASS\n");
    return 0;
}

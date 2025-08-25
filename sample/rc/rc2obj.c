// rc2obj.c - Fixed version for proper RT_ICON/RT_GROUP_ICON support
// Build: tcc rc2obj.c -o rc2obj.exe

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>

// Minimal vector
typedef struct { unsigned char *data; size_t size, cap; } vecu8;
static void v_init(vecu8 *v)
{
    v->data = NULL;
    v->size = 0;
    v->cap = 0;
}

static void *xrealloc(void *p, size_t n)
{
    void *q = realloc(p, n);
    if (!q) {
        fprintf(stderr, "OOM\n");
        exit(1);
    }
    return q;
}

static void *xmalloc(size_t n)
{
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "OOM\n");
        exit(1);
    }
    return p;
}

static void v_reserve(vecu8 *v, size_t need)
{
    if (need > v->cap) {
        size_t c = v->cap ? v->cap * 2 : 1024;
        while (c < need) c *= 2;
        v->data = xrealloc(v->data, c);
        v->cap = c;
    }
}

static size_t v_append(vecu8 *v, const void *p, size_t n)
{
    size_t off = v->size;
    v_reserve(v, off + n);
    memcpy(v->data + off, p, n);
    v->size = off + n;
    return off;
}

static size_t v_append_zeros(vecu8 *v, size_t n)
{
    size_t off = v->size;
    v_reserve(v, off + n);
    memset(v->data + off, 0, n);
    v->size = off + n;
    return off;
}

static size_t pad(size_t off, size_t n)
{
    size_t m = (n - 1);
    return (off + m) & ~m;
}

static size_t v_align(vecu8 *v, size_t align)
{
    size_t newsize = pad(v->size, align);
    if (newsize > v->size) v_append_zeros(v, newsize - v->size);
    return v->size;
}

#pragma pack(push,1)
typedef struct { uint16_t Machine; uint16_t NumberOfSections; uint32_t TimeDateStamp; uint32_t PointerToSymbolTable; uint32_t NumberOfSymbols; uint16_t SizeOfOptionalHeader; uint16_t Characteristics; } COFF_HEADER;
typedef struct { char Name[8]; uint32_t VirtualSize; uint32_t VirtualAddress; uint32_t SizeOfRawData; uint32_t PointerToRawData; uint32_t PointerToRelocations; uint32_t PointerToLinenumbers; uint16_t NumberOfRelocations; uint16_t NumberOfLinenumbers; uint32_t Characteristics; } COFF_SECTION_HEADER;
typedef struct { uint32_t VirtualAddress; uint32_t SymbolTableIndex; uint16_t Type; } COFF_RELOCATION;
typedef struct { char Name[8]; uint32_t Value; uint16_t SectionNumber; uint16_t Type; uint8_t StorageClass; uint8_t NumberOfAuxSymbols; } COFF_SYMBOL;

typedef struct { uint32_t Characteristics; uint32_t TimeDateStamp; uint16_t MajorVersion; uint16_t MinorVersion; uint16_t NumberOfNamedEntries; uint16_t NumberOfIdEntries; } IMAGE_RESOURCE_DIRECTORY;
typedef struct { uint32_t Name; uint32_t OffsetToData; } IMAGE_RESOURCE_DIRECTORY_ENTRY;
typedef struct { uint32_t OffsetToData; uint32_t Size; uint32_t CodePage; uint32_t Reserved; } IMAGE_RESOURCE_DATA_ENTRY;

typedef struct { uint16_t idReserved; uint16_t idType; uint16_t idCount; } ICONDIR;
typedef struct { uint8_t bWidth; uint8_t bHeight; uint8_t bColorCount; uint8_t bReserved; uint16_t wPlanes; uint16_t wBitCount; uint32_t dwBytesInRes; uint32_t dwImageOffset; } ICONDIRENTRY;
typedef struct { uint8_t bWidth; uint8_t bHeight; uint8_t bColorCount; uint8_t bReserved; uint16_t wPlanes; uint16_t wBitCount; uint32_t dwBytesInRes; uint16_t nID; } GRPICONDIRENTRY;
#pragma pack(pop)

#define RT_ICON 3
#define RT_GROUP_ICON 14
#define MACHINE_X64 0x8664
#define IMAGE_SCN_CNT_INITIALIZED_DATA 0x00000040
#define IMAGE_SCN_MEM_READ 0x40000000

#define IMAGE_REL_AMD64_ADDR32NB 3
#define IMAGE_SYM_CLASS_STATIC 3

typedef struct { int id; size_t data_off; uint32_t size; } ResItem;
typedef struct { ResItem *icons; int n_icons,c_icons; ResItem *grp; int n_grp,c_grp; vecu8 blob; } ResCollector;

static void rc_init(ResCollector *rc)
{
    memset(rc, 0, sizeof(*rc));
    v_init(&rc->blob);
}

static void rc_add_item(ResItem **arr, int *n, int *c, int id, size_t off, uint32_t sz)
{
    if (*n == *c) {
        *c = *c ? (*c * 2) : 8;
        *arr = xrealloc(*arr, (*c) * sizeof(ResItem));
    }
    (*arr)[*n].id = id;
    (*arr)[*n].data_off = off;
    (*arr)[*n].size = sz;
    (*n)++;
}

static unsigned char *read_all(const char *path, size_t *out_sz)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "open: %s\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *buf = (unsigned char*)xmalloc((size_t)n);
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "read failed: %s\n", path);
        exit(1);
    }
    fclose(f);
    *out_sz = (size_t)n;
    return buf;
}

// ===== 省略: ユーティリティ (vecu8, xmalloc, etc.) は元と同じ =====


// --- 中略: rc_init, rc_add_item, read_all, v_* 系関数は元と同じ ---

// === 修正版 add_icon ===
static void add_icon(ResCollector *rc, int group_id, const char *ico_path)
{
    size_t sz;
    unsigned char *ico = read_all(ico_path, &sz);
    if (!ico) return;
    ICONDIR *hdr = (ICONDIR*)ico;
    ICONDIRENTRY *ents = (ICONDIRENTRY*)(ico + sizeof(ICONDIR));

    int base_icon_id = group_id * 100;  // グループごとに固有のIDを割り当て
    GRPICONDIRENTRY *grp = xmalloc(sizeof(GRPICONDIRENTRY) * hdr->idCount);

    for (int i = 0; i < hdr->idCount; i++) {
        ICONDIRENTRY *e = &ents[i];
        size_t off = v_align(&rc->blob, 4);
        v_append(&rc->blob, ico + e->dwImageOffset, e->dwBytesInRes);
        rc_add_item(&rc->icons, &rc->n_icons, &rc->c_icons,
                    base_icon_id + i, off, e->dwBytesInRes);

        grp[i].bWidth  = e->bWidth;
        grp[i].bHeight = e->bHeight;
        grp[i].bColorCount = e->bColorCount;
        grp[i].bReserved = 0;
        grp[i].wPlanes = e->wPlanes;
        grp[i].wBitCount = e->wBitCount;
        grp[i].dwBytesInRes = e->dwBytesInRes;
        grp[i].nID = (uint16_t)(base_icon_id + i); // RT_ICON と一致
    }

    ICONDIR ghdr = {0,1,hdr->idCount};
    size_t goff = v_align(&rc->blob, 4);
    v_append(&rc->blob, &ghdr, sizeof(ghdr));
    v_append(&rc->blob, grp, sizeof(GRPICONDIRENTRY) * hdr->idCount);
    uint32_t gsz = (uint32_t)(sizeof(ghdr) + sizeof(GRPICONDIRENTRY) * hdr->idCount);
    rc_add_item(&rc->grp, &rc->n_grp, &rc->c_grp, group_id, goff, gsz);

    free(grp);
    free(ico);
}

static int cmp_resitem(const void *a, const void *b)
{
    const ResItem *A = a;
    const ResItem *B = b;
    return A->id - B->id;
}

static size_t emit_directory(vecu8 *out, int n_entries)
{
    IMAGE_RESOURCE_DIRECTORY dir;
    memset(&dir, 0, sizeof(dir));
    dir.NumberOfIdEntries = (uint16_t)n_entries;
    size_t pos = v_align(out, 4);
    v_append(out, &dir, sizeof(dir));
    
    // エントリ用の空領域を予約
    IMAGE_RESOURCE_DIRECTORY_ENTRY zero;
    memset(&zero, 0, sizeof(zero));
    for (int i = 0; i < n_entries; i++) {
        v_append(out, &zero, sizeof(zero));
    }
    return pos;
}

static void build_rsrc_tree(ResCollector *rc, vecu8 *relocations)
{
    ResItem *icons = rc->icons; 
    int n_icons = rc->n_icons; 
    ResItem *grps = rc->grp; 
    int n_grps = rc->n_grp;
    
    if(n_icons) qsort(icons, n_icons, sizeof(ResItem), cmp_resitem);
    if(n_grps) qsort(grps, n_grps, sizeof(ResItem), cmp_resitem);

    vecu8 out; 
    v_init(&out);

    // RT_ICONを先に、RT_GROUP_ICONを後に配置（タイプIDの昇順）
    struct Type { uint32_t type; ResItem *items; int n; } types[2]; 
    int nt = 0;
    if(n_icons){ 
        types[nt].type = RT_ICON; 
        types[nt].items = icons; 
        types[nt].n = n_icons; 
        nt++; 
    }
    if(n_grps){ 
        types[nt].type = RT_GROUP_ICON; 
        types[nt].items = grps; 
        types[nt].n = n_grps; 
        nt++; 
    }

    // ルートディレクトリ
    size_t root_off = emit_directory(&out, nt);
    IMAGE_RESOURCE_DIRECTORY_ENTRY *root_entries = 
        (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(out.data + root_off + sizeof(IMAGE_RESOURCE_DIRECTORY));

    // データエントリのリスト
    typedef struct { size_t data_entry_offset; ResItem *ri; } DataEntryInfo;
    DataEntryInfo *data_entries = NULL;
    int n_data_entries = 0;
    int c_data_entries = 0;

    // 各タイプについてディレクトリを構築
    for(int ti = 0; ti < nt; ++ti) {
        struct Type *t = &types[ti];
        
        // タイプレベルディレクトリ
        size_t type_off = emit_directory(&out, t->n);
        root_entries[ti].Name = t->type;
        root_entries[ti].OffsetToData = 0x80000000u | (uint32_t)type_off;
        
        IMAGE_RESOURCE_DIRECTORY_ENTRY *id_entries = 
            (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(out.data + type_off + sizeof(IMAGE_RESOURCE_DIRECTORY));
        
        // 各IDについて
        for(int i = 0; i < t->n; i++) {
            // 言語レベルディレクトリ
            size_t lang_off = emit_directory(&out, 1);
            id_entries[i].Name = (uint32_t)t->items[i].id;
            id_entries[i].OffsetToData = 0x80000000u | (uint32_t)lang_off;
            
            IMAGE_RESOURCE_DIRECTORY_ENTRY *lang_entries = 
                (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(out.data + lang_off + sizeof(IMAGE_RESOURCE_DIRECTORY));
            
            // 言語エントリ（英語）
            lang_entries[0].Name = 0x0409; // LANG_ENGLISH_US
            
            // データエントリの位置を記録（後で埋める）
            if (n_data_entries == c_data_entries) {
                c_data_entries = c_data_entries ? c_data_entries * 2 : 16;
                data_entries = xrealloc(data_entries, c_data_entries * sizeof(DataEntryInfo));
            }
            data_entries[n_data_entries].data_entry_offset = 
                (size_t)&lang_entries[0].OffsetToData - (size_t)out.data;
            data_entries[n_data_entries].ri = &t->items[i];
            n_data_entries++;
        }
    }

    // データ部分を追加
    size_t directory_size = out.size;
    size_t data_start = v_align(&out, 4);
    v_append(&out, rc->blob.data, rc->blob.size);

    // データエントリを作成し、参照を修正
    for (int i = 0; i < n_data_entries; i++) {
        ResItem *ri = data_entries[i].ri;
        
        // IMAGE_RESOURCE_DATA_ENTRYを作成
        IMAGE_RESOURCE_DATA_ENTRY entry;
        entry.OffsetToData = (uint32_t)(data_start + ri->data_off); // セクション内オフセット
        entry.Size = ri->size;
        entry.CodePage = 0;
        entry.Reserved = 0;
        
        v_align(&out, 4);
        size_t entry_pos = v_append(&out, &entry, sizeof(entry));
        
        // 言語エントリのOffsetToDataを更新
        uint32_t *offset_ptr = (uint32_t*)(out.data + data_entries[i].data_entry_offset);
        *offset_ptr = (uint32_t)entry_pos;
        
        // リロケーションエントリを追加
        COFF_RELOCATION reloc;
        reloc.VirtualAddress = (uint32_t)(entry_pos);
        reloc.SymbolTableIndex = 0; // .rsrcセクションのシンボル
        reloc.Type = IMAGE_REL_AMD64_ADDR32NB;
        v_append(relocations, &reloc, sizeof(reloc));
    }

    free(data_entries);
    free(rc->blob.data);
    rc->blob = out;
}

static void write_coff_obj(const char *out_path, const unsigned char *rsrc, size_t rsrc_sz, vecu8 *relocations)
{
    // シンボルテーブル
    COFF_SYMBOL symbol;
    memset(&symbol, 0, sizeof(symbol));
    memcpy(symbol.Name, ".rsrc", 5);
    symbol.Value = 0;
    symbol.SectionNumber = 1;
    symbol.Type = 0;
    symbol.StorageClass = IMAGE_SYM_CLASS_STATIC;
    symbol.NumberOfAuxSymbols = 0;

    size_t symbol_table_size = sizeof(COFF_SYMBOL);
    size_t header_size = sizeof(COFF_HEADER) + sizeof(COFF_SECTION_HEADER);
    size_t reloc_table_size = relocations->size;
    
    COFF_HEADER ch;
    memset(&ch, 0, sizeof(ch));
    ch.Machine = MACHINE_X64;
    ch.NumberOfSections = 1;
    ch.NumberOfSymbols = 1;
    ch.PointerToSymbolTable = (uint32_t)(header_size + rsrc_sz + reloc_table_size);
    ch.SizeOfOptionalHeader = 0;
    ch.Characteristics = 0;

    COFF_SECTION_HEADER sh;
    memset(&sh, 0, sizeof(sh));
    memcpy(sh.Name, ".rsrc", 5);
    sh.VirtualSize = (uint32_t)rsrc_sz;
    sh.VirtualAddress = 0;
    sh.SizeOfRawData = (uint32_t)rsrc_sz;
    sh.PointerToRawData = (uint32_t)header_size;
    sh.PointerToRelocations = (uint32_t)(header_size + rsrc_sz);
    sh.NumberOfRelocations = (uint16_t)(reloc_table_size / sizeof(COFF_RELOCATION));
    sh.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;

    size_t total = ch.PointerToSymbolTable + symbol_table_size + 4; // +4 for string table size
    unsigned char *buf = (unsigned char*)xmalloc(total);
    
    size_t pos = 0;
    memcpy(buf + pos, &ch, sizeof(ch)); pos += sizeof(ch);
    memcpy(buf + pos, &sh, sizeof(sh)); pos += sizeof(sh);
    memcpy(buf + pos, rsrc, rsrc_sz); pos += rsrc_sz;
    memcpy(buf + pos, relocations->data, reloc_table_size); pos += reloc_table_size;
    memcpy(buf + pos, &symbol, sizeof(symbol)); pos += sizeof(symbol);
    
    // 文字列テーブルサイズ（4バイト）
    uint32_t string_table_size = 4;
    memcpy(buf + pos, &string_table_size, 4);

    FILE *f = fopen(out_path, "wb");
    if (!f) {
        fprintf(stderr, "open for write: %s\n", out_path);
        exit(1);
    }
    if (fwrite(buf, 1, total, f) != total) {
        fprintf(stderr, "write failed\n");
        exit(1);
    }
    fclose(f);
    free(buf);
    
    printf("Generated COFF object: %s (size: %zu bytes, %d relocations)\n", 
           out_path, total, sh.NumberOfRelocations);
}

static void parse_rc_and_build(const char *rc_path, const char *obj_path)
{
    ResCollector rc;
    rc_init(&rc);

    FILE *f = fopen(rc_path, "r");
    if (!f) {
        fprintf(stderr, "cannot open %s\n", rc_path);
        exit(1);
    }

    char line[1024];
    int icon_count = 0;
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) continue;
        if (*p == '#') continue;
        if (!isdigit((unsigned char)*p)) continue;
        
        char *endptr;
        long id = strtol(p, &endptr, 10);
        if (id <= 0) continue;
        
        char *kw = endptr;
        while (*kw && isspace((unsigned char)*kw)) kw++;
        if (_strnicmp(kw, "ICON", 4) == 0) {
            char *q = strchr(kw, '"');
            if (!q) continue;
            char *r = strchr(q + 1, '"');
            if (!r) continue;
            size_t len = (size_t)(r - (q + 1));
            char *fname = (char*)xmalloc(len + 1);
            memcpy(fname, q + 1, len);
            fname[len] = 0;
            printf("Adding ICON %ld -> %s\n", id, fname);
            add_icon(&rc, (int)id, fname);
            icon_count++;
            free(fname);
        }
    }

    fclose(f);

    if (icon_count == 0) {
        fprintf(stderr, "Warning: No ICON resources found in %s\n", rc_path);
        return;
    }

    printf("Found %d ICON resources, %d icon groups\n", rc.n_icons, rc.n_grp);

    vecu8 relocations;
    v_init(&relocations);
    
    build_rsrc_tree(&rc, &relocations);
    write_coff_obj(obj_path, rc.blob.data, rc.blob.size, &relocations);
    
    free(rc.icons);
    free(rc.grp);
    free(rc.blob.data);
    free(relocations.data);
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s input.rc output.obj\n", argv[0]);
        return 1;
    }
    parse_rc_and_build(argv[1], argv[2]);
    return 0;
}
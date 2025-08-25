#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// minimal xmalloc used by extractor
static void *xmalloc(size_t n)
{
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    return p;
}

#pragma pack(push,1)
typedef struct {
    uint16_t e_magic;
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    uint32_t e_lfanew;
} IMAGE_DOS_HEADER;

typedef struct {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} IMAGE_FILE_HEADER;

typedef struct {
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} IMAGE_SECTION_HEADER_COMMON;

typedef struct {
    uint32_t Characteristics;
    uint32_t TimeDateStamp;
    uint16_t MajorVersion;
    uint16_t MinorVersion;
    uint16_t NumberOfNamedEntries;
    uint16_t NumberOfIdEntries;
} IMAGE_RESOURCE_DIRECTORY;

typedef struct {
    uint32_t Name;
    uint32_t OffsetToData;
} IMAGE_RESOURCE_DIRECTORY_ENTRY;

typedef struct {
    uint32_t OffsetToData;
    uint32_t Size;
    uint32_t CodePage;
    uint32_t Reserved;
} IMAGE_RESOURCE_DATA_ENTRY;
#pragma pack(pop)

static uint32_t read_u32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t read_u16(const unsigned char *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s file.exe\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("open");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long flen = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *buf = (unsigned char*)malloc(flen);
    if (!buf) { fclose(f); return 1; }
    if (fread(buf, 1, flen, f) != (size_t)flen) { perror("read"); fclose(f); free(buf); return 1; }
    fclose(f);

    if (flen < 0x40) { fprintf(stderr, "file too small\n"); free(buf); return 1; }

    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER*)buf;
    if (dos->e_magic != 0x5A4D) { fprintf(stderr, "not MZ\n"); free(buf); return 1; }
    uint32_t pe_off = dos->e_lfanew;
    if (pe_off + 4 > (uint32_t)flen) { fprintf(stderr, "invalid pe offset\n"); free(buf); return 1; }

    uint32_t signature = read_u32(buf + pe_off);
    if (signature != 0x4550) { fprintf(stderr, "not PE\n"); free(buf); return 1; }

    IMAGE_FILE_HEADER *fh = (IMAGE_FILE_HEADER*)(buf + pe_off + 4);
    uint16_t nsec = fh->NumberOfSections;
    uint32_t sec_table = pe_off + 4 + sizeof(IMAGE_FILE_HEADER) + fh->SizeOfOptionalHeader;
    uint32_t rsrc_raw = 0, rsrc_size = 0, rsrc_va = 0;

    for (int i = 0; i < nsec; i++) {
        uint32_t off = sec_table + i * 40;
        char name[9]; memset(name, 0, 9); memcpy(name, buf + off, 8);
        uint32_t vs = read_u32(buf + off + 8);
        uint32_t va = read_u32(buf + off + 12);
        uint32_t sz = read_u32(buf + off + 16);
        uint32_t pr = read_u32(buf + off + 20);
        if (strncmp(name, ".rsrc", 6) == 0) { rsrc_raw = pr; rsrc_size = sz; rsrc_va = va; }
    }

    if (!rsrc_raw) { printf("no .rsrc section\n"); free(buf); return 0; }
    printf(".rsrc at file offset 0x%X size 0x%X\n", rsrc_raw, rsrc_size);

    unsigned char *rs = buf + rsrc_raw;

    // parse root directory
    IMAGE_RESOURCE_DIRECTORY *root = (IMAGE_RESOURCE_DIRECTORY*)rs;
    printf("root: named=%d id=%d\n", root->NumberOfNamedEntries, root->NumberOfIdEntries);
    IMAGE_RESOURCE_DIRECTORY_ENTRY *re = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(rs + sizeof(IMAGE_RESOURCE_DIRECTORY));

    for (int i = 0; i < (int)root->NumberOfIdEntries; i++) {
        uint32_t name = read_u32((unsigned char*)&re[i].Name);
        uint32_t off = read_u32((unsigned char*)&re[i].OffsetToData);
        int isdir = (off & 0x80000000) ? 1 : 0;
        off = off & 0x7FFFFFFF;
        printf("type entry %d: type=0x%X isdir=%d off=0x%X\n", i, name, isdir, off);
        if (!isdir) continue;

        IMAGE_RESOURCE_DIRECTORY *td = (IMAGE_RESOURCE_DIRECTORY*)(rs + off);
        IMAGE_RESOURCE_DIRECTORY_ENTRY *ide = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)((unsigned char*)td + sizeof(IMAGE_RESOURCE_DIRECTORY));

        for (int j = 0; j < (int)td->NumberOfIdEntries; j++) {
            uint32_t id = read_u32((unsigned char*)&ide[j].Name);
            uint32_t ioff = read_u32((unsigned char*)&ide[j].OffsetToData);
            int idir = (ioff & 0x80000000) ? 1 : 0;
            ioff &= 0x7FFFFFFF;
            printf("  id entry %d: id=0x%X off=0x%X isdir=%d\n", j, id, ioff, idir);
            if (!idir) continue;

            IMAGE_RESOURCE_DIRECTORY *ld = (IMAGE_RESOURCE_DIRECTORY*)(rs + ioff);
            IMAGE_RESOURCE_DIRECTORY_ENTRY *le = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)((unsigned char*)ld + sizeof(IMAGE_RESOURCE_DIRECTORY));

            for (int k = 0; k < (int)ld->NumberOfIdEntries; k++) {
                uint32_t lang = read_u32((unsigned char*)&le[k].Name);
                uint32_t dto = read_u32((unsigned char*)&le[k].OffsetToData);
                int dis = (dto & 0x80000000) ? 1 : 0;
                dto &= 0x7FFFFFFF;
                printf("    lang %d: lang=0x%X dto=0x%X isdir=%d\n", k, lang, dto, dis);
                if (!dis) {
                    IMAGE_RESOURCE_DATA_ENTRY *de = (IMAGE_RESOURCE_DATA_ENTRY*)(rs + dto);
                    uint32_t dataoff = de->OffsetToData;
                    uint32_t datasz = de->Size;
                    printf("      data at 0x%X size 0x%X\n", dataoff, datasz);
                }
            }
        }
    }

    // For convenience, try to find RT_GROUP_ICON (14) and dump its payload
    for (int i = 0; i < (int)root->NumberOfIdEntries; i++) {
        uint32_t name = read_u32((unsigned char*)&re[i].Name);
        if (name == 14) {
            uint32_t off = read_u32((unsigned char*)&re[i].OffsetToData) & 0x7FFFFFFF;
            IMAGE_RESOURCE_DIRECTORY *td = (IMAGE_RESOURCE_DIRECTORY*)(rs + off);
            IMAGE_RESOURCE_DIRECTORY_ENTRY *ide = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)((unsigned char*)td + sizeof(*td));
            for (int j = 0; j < (int)td->NumberOfIdEntries; j++) {
                uint32_t id = read_u32((unsigned char*)&ide[j].Name);
                uint32_t ioff = read_u32((unsigned char*)&ide[j].OffsetToData) & 0x7FFFFFFF;
                IMAGE_RESOURCE_DIRECTORY *ld = (IMAGE_RESOURCE_DIRECTORY*)(rs + ioff);
                IMAGE_RESOURCE_DIRECTORY_ENTRY *le = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)((unsigned char*)ld + sizeof(*ld));
                uint32_t dto = read_u32((unsigned char*)&le[0].OffsetToData) & 0x7FFFFFFF;
                IMAGE_RESOURCE_DATA_ENTRY *de = (IMAGE_RESOURCE_DATA_ENTRY*)(rs + dto);
                printf("GROUP_ICON id=0x%X dataoff=0x%X size=0x%X\n", id, de->OffsetToData, de->Size);
                unsigned char *gdata = rs + de->OffsetToData;
                int count = ((unsigned short*)gdata)[2];
                printf("  group count=%d\n", count);
                unsigned char *p = gdata + 6;
                for (int m = 0; m < count; m++) {
                    unsigned char w = p[0];
                    unsigned char h = p[1];
                    unsigned char cc = p[2];
                    unsigned short planes = *(unsigned short*)(p + 4);
                    unsigned short bpp = *(unsigned short*)(p + 6);
                    unsigned int bytes = *(unsigned int*)(p + 8);
                    unsigned short nid = *(unsigned short*)(p + 12);
                    printf("   entry %d: %dx%d cc=%d planes=%d bpp=%d bytes=%u id=%d\n", m, w==0?256:w, h==0?256:h, cc, planes, bpp, bytes, nid);
                    p += 14;
                }
            }
        }
    }

    // Also assemble a proper .ico file for the first found group icon
    // find first group icon resource and build ico
    for (int i = 0; i < (int)root->NumberOfIdEntries; i++) {
        uint32_t name = read_u32((unsigned char*)&re[i].Name);
        if (name != 14) continue;
        uint32_t off = read_u32((unsigned char*)&re[i].OffsetToData) & 0x7FFFFFFF;
        IMAGE_RESOURCE_DIRECTORY *td = (IMAGE_RESOURCE_DIRECTORY*)(rs + off);
        IMAGE_RESOURCE_DIRECTORY_ENTRY *ide = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)((unsigned char*)td + sizeof(*td));
        // take first id
        if (td->NumberOfIdEntries == 0) continue;
        uint32_t id = read_u32((unsigned char*)&ide[0].Name);
        uint32_t ioff = read_u32((unsigned char*)&ide[0].OffsetToData) & 0x7FFFFFFF;
        IMAGE_RESOURCE_DIRECTORY *ld = (IMAGE_RESOURCE_DIRECTORY*)(rs + ioff);
        IMAGE_RESOURCE_DIRECTORY_ENTRY *le = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)((unsigned char*)ld + sizeof(*ld));
        uint32_t dto = read_u32((unsigned char*)&le[0].OffsetToData) & 0x7FFFFFFF;
        IMAGE_RESOURCE_DATA_ENTRY *de = (IMAGE_RESOURCE_DATA_ENTRY*)(rs + dto);
        unsigned char *gdata = rs + de->OffsetToData;
        int count = ((unsigned short*)gdata)[2];
        unsigned char *p = gdata + 6;
        // allocate icon file buffer: ICONDIR + entries + data
        size_t icondir_sz = 6 + count * 16;
        size_t total_sz = icondir_sz;
        unsigned int *img_sizes = (unsigned int*)xmalloc(sizeof(unsigned int) * count);
        unsigned char **img_data = (unsigned char**)xmalloc(sizeof(unsigned char*) * count);
        for (int m = 0; m < count; m++) {
            unsigned int bytes = *(unsigned int*)(p + 8);
            unsigned short nid = *(unsigned short*)(p + 12);
            // find RT_ICON with id == nid
            // search type RT_ICON (3) in root
            unsigned char *icon_blob = NULL; unsigned int icon_sz = 0;
            for (int ti = 0; ti < (int)root->NumberOfIdEntries; ti++) {
                uint32_t tname = read_u32((unsigned char*)&re[ti].Name);
                if (tname != 3) continue;
                uint32_t toff = read_u32((unsigned char*)&re[ti].OffsetToData) & 0x7FFFFFFF;
                IMAGE_RESOURCE_DIRECTORY *tt = (IMAGE_RESOURCE_DIRECTORY*)(rs + toff);
                IMAGE_RESOURCE_DIRECTORY_ENTRY *tide = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)((unsigned char*)tt + sizeof(*tt));
                if (tt->NumberOfIdEntries == 0) continue;
                for (int jj = 0; jj < (int)tt->NumberOfIdEntries; jj++) {
                    uint32_t tid = read_u32((unsigned char*)&tide[jj].Name);
                    if (tid != nid) continue;
                    uint32_t tioff = read_u32((unsigned char*)&tide[jj].OffsetToData) & 0x7FFFFFFF;
                    IMAGE_RESOURCE_DIRECTORY *tld = (IMAGE_RESOURCE_DIRECTORY*)(rs + tioff);
                    IMAGE_RESOURCE_DIRECTORY_ENTRY *tle = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)((unsigned char*)tld + sizeof(*tld));
                    uint32_t tdto = read_u32((unsigned char*)&tle[0].OffsetToData) & 0x7FFFFFFF;
                    IMAGE_RESOURCE_DATA_ENTRY *tde = (IMAGE_RESOURCE_DATA_ENTRY*)(rs + tdto);
                    icon_blob = rs + tde->OffsetToData;
                    icon_sz = tde->Size;
                    break;
                }
                if (icon_blob) break;
            }
            img_sizes[m] = icon_sz;
            img_data[m] = icon_blob;
            p += 14;
            total_sz += icon_sz;
        }
        unsigned char *outbuf = (unsigned char*)xmalloc(total_sz);
        // write ICONDIR
        outbuf[0] = 0; outbuf[1] = 0; outbuf[2] = 1; outbuf[3] = 0; outbuf[4] = (unsigned char)(count & 0xFF); outbuf[5] = 0;
        size_t cur = icondir_sz;
        // second pass to write entries
        p = gdata + 6;
        for (int m = 0; m < count; m++) {
            unsigned char bWidth = p[0];
            unsigned char bHeight = p[1];
            unsigned char bColorCount = p[2];
            unsigned short wPlanes = *(unsigned short*)(p + 4);
            unsigned short wBitCount = *(unsigned short*)(p + 6);
            unsigned int bytes = img_sizes[m];
            unsigned short nid = *(unsigned short*)(p + 12);
            size_t ent_off = 6 + m * 16;
            outbuf[ent_off + 0] = bWidth;
            outbuf[ent_off + 1] = bHeight;
            outbuf[ent_off + 2] = bColorCount;
            outbuf[ent_off + 3] = 0;
            outbuf[ent_off + 4] = (unsigned char)(wPlanes & 0xFF);
            outbuf[ent_off + 5] = (unsigned char)((wPlanes >> 8) & 0xFF);
            outbuf[ent_off + 6] = (unsigned char)(wBitCount & 0xFF);
            outbuf[ent_off + 7] = (unsigned char)((wBitCount >> 8) & 0xFF);
            outbuf[ent_off + 8] = (unsigned char)(bytes & 0xFF);
            outbuf[ent_off + 9] = (unsigned char)((bytes >> 8) & 0xFF);
            outbuf[ent_off +10] = (unsigned char)((bytes >> 16) & 0xFF);
            outbuf[ent_off +11] = (unsigned char)((bytes >> 24) & 0xFF);
            // dwImageOffset
            outbuf[ent_off +12] = (unsigned char)(cur & 0xFF);
            outbuf[ent_off +13] = (unsigned char)((cur >> 8) & 0xFF);
            outbuf[ent_off +14] = (unsigned char)((cur >> 16) & 0xFF);
            outbuf[ent_off +15] = (unsigned char)((cur >> 24) & 0xFF);
            // copy image data
            memcpy(outbuf + cur, img_data[m], img_sizes[m]);
            cur += img_sizes[m];
            p += 14;
        }
        // write file
        // build output path in same directory as input exe
        const char *inpath = argv[1];
        const char *slash = strrchr(inpath, '\\');
        if (!slash) slash = strrchr(inpath, '/');
        char outname[1024];
        if (slash) {
            size_t dirlen = (size_t)(slash - inpath + 1);
            if (dirlen + 64 >= sizeof(outname)) dirlen = sizeof(outname) - 64;
            memcpy(outname, inpath, dirlen);
            outname[dirlen] = 0;
            snprintf(outname + dirlen, sizeof(outname) - dirlen, "extracted_group_%u.ico", id);
        } else {
            snprintf(outname, sizeof(outname), "extracted_group_%u.ico", id);
        }
        // diagnostic: ensure all img_data were found
        int missing = 0;
        for (int m = 0; m < count; m++) {
            printf("img %d: nid=? size=%u ptr=%p\n", m, img_sizes[m], (void*)img_data[m]);
            if (!img_data[m] || img_sizes[m] == 0) missing = 1;
        }
        if (missing) {
            fprintf(stderr, "warning: some RT_ICON blobs were not found or empty; aborting write\n");
        }
        printf("will write ico to: %s\n", outname);
        if (missing) {
            // don't write incomplete ico
            free(outbuf);
            free(img_sizes);
            free(img_data);
            continue;
        }
        FILE *of = fopen(outname, "wb");
        if (!of) {
            fprintf(stderr, "failed to open output file: %s\n", outname);
            free(outbuf);
            free(img_sizes);
            free(img_data);
            continue;
        }
        size_t wrote = fwrite(outbuf, 1, total_sz, of);
        fclose(of);
        printf("fwrite returned %zu (expected %zu)\n", wrote, total_sz);
        if (wrote == total_sz) printf("wrote %s\n", outname);
        free(outbuf);
        free(img_sizes);
        free(img_data);
        break; // only first group
    }

    free(buf);
    return 0;
}

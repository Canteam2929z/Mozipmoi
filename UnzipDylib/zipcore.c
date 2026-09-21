/*
 * zipcore.c — Bộ giải nén ZIP tối giản, tự chứa (chỉ cần zlib).
 * Hỗ trợ: method 0 (stored) và method 8 (deflate). Không hỗ trợ ZIP64.
 */

#include "zipcore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <zlib.h>

static inline uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static inline uint32_t rd32(const uint8_t *p) {
    return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24));
}

/* Tạo mọi thư mục cha trong 'path' (path có thể là file hoặc dir). */
static void mkdir_p(char *path) {
    for (char *p = path + 1; *p; p++) {
        if (*p == '/') { *p = '\0'; mkdir(path, 0755); *p = '/'; }
    }
    mkdir(path, 0755);   /* tao luon thanh phan cuoi cung */
}

static void mkdir_parent_of(const char *filePath) {
    char tmp[4096];
    strncpy(tmp, filePath, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    char *slash = strrchr(tmp, '/');
    if (slash) { *slash = '\0'; if (tmp[0]) mkdir_p(tmp); }
}

/* Giải nén một entry ra outPath. */
static int extract_entry(const uint8_t *src, uint32_t csize, uint32_t usize,
                         uint16_t method, const char *outPath) {
    if (method == 0) {                       /* stored */
        FILE *o = fopen(outPath, "wb");
        if (!o) return -1;
        if (csize) fwrite(src, 1, csize, o);
        fclose(o);
        return 0;
    }
    if (method != 8) return -2;              /* chỉ hỗ trợ stored & deflate */

    uint32_t cap = (usize ? usize : 64) + 64;
    uint8_t *out = malloc(cap);
    if (!out) return -3;

    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    zs.next_in   = (Bytef *)src;
    zs.avail_in  = csize;
    zs.next_out  = out;
    zs.avail_out = cap;

    if (inflateInit2(&zs, -MAX_WBITS) != Z_OK) { free(out); return -4; }
    int r = inflate(&zs, Z_FINISH);
    uint32_t produced = cap - zs.avail_out;
    inflateEnd(&zs);
    if (r != Z_STREAM_END) { free(out); return -5; }

    FILE *o = fopen(outPath, "wb");
    if (!o) { free(out); return -6; }
    if (produced) fwrite(out, 1, produced, o);
    fclose(o);
    free(out);
    return 0;
}

int zip_extract(const char *zipPath, const char *destRoot) {
    FILE *f = fopen(zipPath, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 22) { fclose(f); return -2; }

    uint8_t *buf = malloc((size_t)sz);
    if (!buf) { fclose(f); return -3; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return -4; }
    fclose(f);

    /* Tìm End Of Central Directory (0x06054b50), quét từ cuối lên. */
    long lo = sz - 22 - 65535; if (lo < 0) lo = 0;
    long eocd = -1;
    for (long i = sz - 22; i >= lo; i--) {
        if (rd32(buf + i) == 0x06054b50) { eocd = i; break; }
    }
    if (eocd < 0) { free(buf); return -5; }

    uint16_t nentries = rd16(buf + eocd + 10);
    uint32_t cdOff    = rd32(buf + eocd + 16);
    long p = cdOff;

    char outPath[4096];

    for (uint16_t k = 0; k < nentries; k++) {
        if (p + 46 > sz || rd32(buf + p) != 0x02014b50) break;

        uint16_t method = rd16(buf + p + 10);
        uint32_t csize  = rd32(buf + p + 20);
        uint32_t usize  = rd32(buf + p + 24);
        uint16_t fnLen  = rd16(buf + p + 28);
        uint16_t exLen  = rd16(buf + p + 30);
        uint16_t cmLen  = rd16(buf + p + 32);
        uint32_t lho    = rd32(buf + p + 42);
        char    *name   = (char *)(buf + p + 46);

        /* Bỏ qua entry ghi bằng đường dẫn tuyệt đối / thoát thư mục. */
        if (name[0] == '/' || strstr(name, "../") != NULL) {
            p += 46 + fnLen + exLen + cmLen;
            continue;
        }

        snprintf(outPath, sizeof(outPath), "%s/%.*s", destRoot, fnLen, name);

        int isDir = (fnLen > 0 && name[fnLen - 1] == '/');
        if (isDir) {
            mkdir_p(outPath);
        } else if (lho + 30 <= (uint32_t)sz && rd32(buf + lho) == 0x04034b50) {
            uint16_t lfn = rd16(buf + lho + 26);
            uint16_t lex = rd16(buf + lho + 28);
            uint32_t dataOff = lho + 30 + lfn + lex;
            if (dataOff + csize <= (uint32_t)sz) {
                mkdir_parent_of(outPath);
                extract_entry(buf + dataOff, csize, usize, method, outPath);
            }
        }

        p += 46 + fnLen + exLen + cmLen;
    }

    free(buf);
    return 0;
}

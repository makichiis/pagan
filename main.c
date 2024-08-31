#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "cursor.h"

typedef struct ChunkIHDR {
    uint32_t    width;
    uint32_t    height;
    uint8_t     bit_depth;
    uint8_t     color_type;
    uint8_t     compression_method;
    uint8_t     filter_method;
    uint8_t     interlace_method;
} ChunkIHDR;

typedef struct PNG {
    ChunkIHDR ihdr;
} PNG;

#define MAGIC_HEADER_SZ 8

Cursor_u8 Cursor_u8_from(uint8_t* ubuf, size_t sz) {
    return (struct Cursor_u8){ ubuf, ubuf + sz };
}

bool png_data_is_valid(Cursor_u8 data);
void png_from_bytes(PNG* png, Cursor_u8 data);

int main() {
    FILE* imgfp = fopen("tiny.png", "r");
    if (!imgfp) {
        perror("Failed to open file");
        return 1;
    }

    uint8_t* imgbuf = NULL;
    size_t   imgbufsz = 0L;

    fseek(imgfp, 0L, SEEK_END);
    imgbufsz = ftell(imgfp) - 8; // exclude magic header 
    fseek(imgfp, 0L, SEEK_SET);

    uint8_t magic[8];
    if (fread(magic, 1, 8, imgfp) != 8) {
        fprintf(stderr, "Could not read 8 byte header.\n");
        return 1;
    } else if (!png_data_is_valid(Cursor_u8_from(magic, 8))) {
        fprintf(stderr, "Image is not a valid PNG (mismatched magic).\n");
        return 1;
    }

    imgbuf = calloc(1, imgbufsz);
    if (fread(imgbuf, 1, imgbufsz, imgfp) != imgbufsz) {
        fprintf(stderr, "Failed to read image.");
        return 1;
    }

    PNG png;
    png_from_bytes(&png, Cursor_u8_from(imgbuf, imgbufsz));
    
    free(imgbuf);
    fclose(imgfp);
}

bool png_data_is_valid(Cursor_u8 data) {
    static union {
        uint64_t rx;
        uint8_t bytes[8];
    } magic = { .bytes = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a } };

    assert(data.end - data.begin >= 8);

    return magic.rx == *(uint64_t*)data.begin;
}

typedef struct Chunk {
    uint32_t            length;
    uint8_t             label[4];
    const uint8_t*      data;
    uint32_t            crc;
} Chunk;

#define szm(c_t, m) (sizeof (((c_t*)NULL)->m))
#define MIN_CHUNK_SZ szm(Chunk, length) + szm(Chunk, label) + szm(Chunk, crc)

void chunk_from_bytes(Chunk* chunk, Cursor_u8 data) {
    if (data.end - data.begin < MIN_CHUNK_SZ) {
        fprintf(stderr, "error: invalid chunk block.");
        exit(1);
    }

    chunk->length = data.begin[0] << 24
                  | data.begin[1] << 16
                  | data.begin[2] << 8
                  | data.begin[3];

    if (data.end - data.begin < MIN_CHUNK_SZ + chunk->length) {
        fprintf(stderr, "error: chunk length descriptor larger than scope.");
        exit(1);
    }
    data.begin += sizeof chunk->length;

    strncpy(chunk->label, data.begin, sizeof chunk->label);
    data.begin += sizeof chunk->label;

    chunk->data = data.begin;
    data.begin += chunk->length;
    
    chunk->crc = data.begin[0] << 24
               | data.begin[1] << 16
               | data.begin[2] << 8
               | data.begin[3];
}

void png_from_bytes(PNG* png, Cursor_u8 data) {
    Chunk ihdr;
    chunk_from_bytes(&ihdr, data);

    printf("chunk type: %.4s\n", ihdr.label);
    printf("chunk body (%dB):", ihdr.length);
    
    for (int i = 0; i < ihdr.length; ++i) {
        if (i % 8 == 0) printf("\n0x ");
        printf("%.2X ", ihdr.data[i]);
    } puts("");

    printf("chksm: %d\n", ihdr.crc);
}


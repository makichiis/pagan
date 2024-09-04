#ifndef PAGAN_CURSOR_H
#define PAGAN_CURSOR_H

typedef struct Cursor_u8 {
    const uint8_t*    begin;
    const uint8_t*    end;
} Cursor_u8;

/**
 * @brief Create a u8 cursor from a C-style array.
 * The beginning pointer is set to u8buf, and the
 * end pointer is set to u8buf + sz.
 */
Cursor_u8 Cursor_u8_from(uint8_t* u8buf, size_t sz);

#endif 

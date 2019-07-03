#ifndef __text_conv_h__
#define __text_conv_h__

#include <stdint.h>

enum
{
 CHAR_FLAG_SINGLE_BYTE = 1,
 CHAR_FLAG_FIRST_BYTE  = 2,
 CHAR_FLAG_SECOND_BYTE = 4
};

enum
{
 RESULT_VALID         = 0x01,
 RESULT_HAS_ERRORS    = 0x02,
 RESULT_BUF_ERROR     = 0x04,
 RESULT_HAS_BOM       = 0x08,
 RESULT_HAS_MULTIBYTE = 0x10, 
 RESULT_BAD_CHARSET   = 0x80
};

struct range_def
{
 uint16_t start;
 uint16_t end;
 const uint16_t *table;
};

int is_valid_utf8(const void *data, int size);
int is_valid_dbcs(const void *data, int size, const uint8_t char_flags[]);

int convert_latin1(void *out, int *out_size_ptr, const void *in, int *in_size_ptr);
int convert_sbcs(void *out, int *out_size_ptr, const void *in, int *in_size_ptr, char error_char, const uint16_t table[]);
int convert_dbcs(void *out, int *out_size_ptr, const void *in, int *in_size_ptr, char error_char,
                 const struct range_def ranges[], int range_count, const uint8_t char_flags[]);
int convert_utf16_be(void *out, int *out_size_ptr, const void *in, int *in_size_ptr, char error_char);

#endif /* __text_conv_h__ */

#ifndef __charsets_h__
#define __charsets_h__

enum
{
 CHARSET_NONE,
 CHARSET_UTF8,
 CHARSET_ISO8859_1,
 CHARSET_ISO8859_2,
 CHARSET_ISO8859_3,
 CHARSET_ISO8859_4,
 CHARSET_ISO8859_5,
 CHARSET_ISO8859_6,
 CHARSET_ISO8859_7,
 CHARSET_ISO8859_8,
 CHARSET_ISO8859_9,
 CHARSET_ISO8859_10,
 CHARSET_ISO8859_11,
 CHARSET_ISO8859_13,
 CHARSET_ISO8859_14,
 CHARSET_ISO8859_15,
 CHARSET_ISO8859_16,
 CHARSET_SHIFT_JIS,
 CHARSET_CP437,
 CHARSET_CP1250,
 CHARSET_CP1251,
 CHARSET_CP1252,
 CHARSET_CP1256,
 CHARSET_UTF16_BE,
 CHARSET_BIG5
};

int convert_to_utf8(void *out, int *out_size_ptr, const void *in, int *in_size_ptr, char err_char, int charset);
int is_valid_shiftjis(const void *data, int size);

#endif /* __charsets_h__ */

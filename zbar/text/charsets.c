#include "charsets.h"
#include "text_conv.h"
#include "single_byte.inc"
#include "shift_jis.inc"
#include <string.h>
#include <assert.h>

#define countof(a) (sizeof(a)/sizeof(a[0]))

int convert_to_utf8(void *out, int *out_size_ptr, const void *in, int *in_size_ptr, char err_char, int charset)
{
 assert(out != in);
 switch (charset)
 {
  case CHARSET_UTF8:
  {
   int result = 0;
   int size = *in_size_ptr;
   if (*out_size_ptr < size)
   {
    result = RESULT_BUF_ERROR;
    size = *out_size_ptr;
   }
   memcpy(out, in, size);
   *in_size_ptr = *out_size_ptr = size;
   return result;
  }
  
  case CHARSET_UTF16_BE:
   return convert_utf16_be(out, out_size_ptr, in, in_size_ptr, err_char);
  
#ifdef ENABLE_CHARSET_ISO8859
  case CHARSET_ISO8859_1:
   return convert_latin1(out, out_size_ptr, in, in_size_ptr);

  case CHARSET_ISO8859_2: 
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, iso8859_2_to_unicode);

  case CHARSET_ISO8859_3: 
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, iso8859_3_to_unicode);

  case CHARSET_ISO8859_4: 
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, iso8859_4_to_unicode);

  case CHARSET_ISO8859_5: 
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, iso8859_5_to_unicode);

  case CHARSET_ISO8859_6: 
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, iso8859_6_to_unicode);

  case CHARSET_ISO8859_7: 
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, iso8859_7_to_unicode);

  case CHARSET_ISO8859_8: 
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, iso8859_8_to_unicode);

  case CHARSET_ISO8859_9: 
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, iso8859_9_to_unicode);

  case CHARSET_ISO8859_10: 
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, iso8859_10_to_unicode);

  case CHARSET_ISO8859_11: 
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, iso8859_11_to_unicode);

  case CHARSET_ISO8859_13: 
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, iso8859_13_to_unicode);

  case CHARSET_ISO8859_14: 
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, iso8859_14_to_unicode);

  case CHARSET_ISO8859_15: 
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, iso8859_15_to_unicode);

  case CHARSET_ISO8859_16: 
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, iso8859_16_to_unicode);
#endif

#ifdef ENABLE_CHARSET_WINDOWS
  case CHARSET_CP1250:
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, cp1250_to_unicode);

  case CHARSET_CP1251:
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, cp1251_to_unicode);

  case CHARSET_CP1252:
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, cp1252_to_unicode);

  case CHARSET_CP1256:
   return convert_sbcs(out, out_size_ptr, in, in_size_ptr, err_char, cp1256_to_unicode);
#endif

  case CHARSET_SHIFT_JIS: 
   return convert_dbcs(out, out_size_ptr, in, in_size_ptr, err_char,
                       shiftjis_ranges, countof(shiftjis_ranges), shiftjis_flags);
 }
 *out_size_ptr = *in_size_ptr = 0;
 return RESULT_BAD_CHARSET;
}

int is_valid_shiftjis(const void *data, int size)
{
 return is_valid_dbcs(data, size, shiftjis_flags);
}

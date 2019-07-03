#include "text_conv.h"
#include "bits.h"

int is_valid_utf8(const void *data, int size)
{
 int result = 0;
 const uint8_t *src = data;
 if (size >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF)
 {
  result |= RESULT_HAS_BOM;
  size -= 3;
  src += 3;
 }
 while (size)
 {
  if (!(*src & 0x80))
  {
   src++;
   size--;
  } else
  {
   unsigned val;
   int count = clz32(*src ^ 0xFF) - 24;
   if (count > size) return 0;
   switch (count)
   {
    case 2:
     val = (src[0] & 0x1F)<<6 | (src[1] & 0x3F); 
     if (!(src[1] & 0x80) || val < 0x80) return 0;
     break;
    case 3:
     val = (src[0] & 0xF)<<12 | (src[1] & 0x3F)<<6 | (src[2] & 0x3F);
     if (!(src[1] & src[2] & 0x80) || val < 0x800) return 0;
     break;
    case 4:
     val = (src[0] & 0x7)<<18 | (src[1] & 0x3F)<<12 | (src[2] & 0x3F)<<6 | (src[3] & 0x3F);   
     if (!(src[1] & src[2] & src[3] & 0x80) || val < 0x10000) return 0;
     break;
    default:
     return 0;
   }
   src += count;
   size -= count;
   result |= RESULT_HAS_MULTIBYTE;
  }
 }
 return result | RESULT_VALID;
}

int is_valid_dbcs(const void *data, int size, const uint8_t char_flags[])
{
 int result = 0;
 const uint8_t *src = data;
 while (size)
 {
  if (char_flags[*src] & CHAR_FLAG_SINGLE_BYTE)
  {
   src++;
   size--;
  } else
  if (char_flags[*src] & CHAR_FLAG_FIRST_BYTE)
  {
   if (size < 2) return 0;
   if (!(char_flags[src[1]] & CHAR_FLAG_SECOND_BYTE)) return 0;
   src += 2;
   size -= 2;
   result |= RESULT_HAS_MULTIBYTE;
  } else return 0;
 }
 return result | RESULT_VALID;
}

int convert_latin1(void *out, int *out_size_ptr, const void *in, int *in_size_ptr)
{
 int result = 0;
 int in_size = *in_size_ptr;
 int out_size = *out_size_ptr;
 const uint8_t *src = in;
 uint8_t *dest = out;
 while (in_size)
 {
  if (*src < 0x80)
  {
   if (!out_size) { result |= RESULT_BUF_ERROR; break; }
   *dest++ = *src++;
   out_size--;
  } else
  {
   unsigned b = *src++;
   if (out_size < 2) { result |= RESULT_BUF_ERROR; break; }
   *dest++ = 0xC0 | b>>6;
   *dest++ = 0x80 | (b & 0x3F);
   out_size -= 2;
  }
  in_size--;
 }
 *in_size_ptr -= in_size;
 *out_size_ptr -= out_size;
 return result;
}

int convert_sbcs(void *out, int *out_size_ptr, const void *in, int *in_size_ptr, char error_char, const uint16_t table[])
{
 int result = 0;
 int in_size = *in_size_ptr;
 int out_size = *out_size_ptr;
 const uint8_t *src = in;
 uint8_t *dest = out;
 while (in_size)
 {
  if (*src < 0x80)
  {
   if (!out_size) { result |= RESULT_BUF_ERROR; break; }
   *dest++ = *src++;
   out_size--;
  } else
  {
   unsigned uc = table[*src++ - 0x80];
   if (!uc)
   {
    result |= RESULT_HAS_ERRORS;
    uc = error_char;
   }
   if (uc < 0x80)
   {
    if (!out_size) { result |= RESULT_BUF_ERROR; break; }
    *dest++ = (uint8_t) uc;
    out_size--;
   } else
   if (uc < 0x800)
   {
    if (out_size < 2) { result |= RESULT_BUF_ERROR; break; }
    *dest++ = 0xC0 | uc>>6;
    *dest++ = 0x80 | (uc & 0x3F);
    out_size -= 2;
   } else
   {
    if (out_size < 3) { result |= RESULT_BUF_ERROR; break; }
    *dest++ = 0xE0 | uc>>12;
    *dest++ = 0x80 | ((uc>>6) & 0x3F);
    *dest++ = 0x80 | (uc & 0x3F);
    out_size -= 3;
   }
  }
  in_size--;
 }
 *in_size_ptr -= in_size;
 *out_size_ptr -= out_size;
 return result;
}

int convert_dbcs(void *out, int *out_size_ptr, const void *in, int *in_size_ptr, char error_char,
                 const struct range_def ranges[], int range_count, const uint8_t char_flags[])
{
 int result = 0;
 int in_size = *in_size_ptr;
 int out_size = *out_size_ptr;
 const uint8_t *src = in;
 uint8_t *dest = out;
 while (in_size)
 {
  unsigned uc, b = *src++;
  if (!(char_flags[b] & (CHAR_FLAG_SINGLE_BYTE | CHAR_FLAG_FIRST_BYTE)))
  {
   result |= RESULT_HAS_ERRORS;
   uc = error_char;
  } else
  {
   int i;
   if (char_flags[b] & CHAR_FLAG_FIRST_BYTE)
   {
    if (in_size < 2) { result |= RESULT_BUF_ERROR; break; }
    b = b<<8 | *src++;
    in_size--;
   }
   uc = 0;
   for (i = 0; i < range_count; i++)
    if (b >= ranges[i].start && b < ranges[i].end)
    {
     uc = ranges[i].table[b - ranges[i].start];
     break;
    }
   if (!uc && b)
   {
    result |= RESULT_HAS_ERRORS;
    uc = error_char;
   }
  } 
  if (uc < 0x80)
  {
   if (!out_size) { result |= RESULT_BUF_ERROR; break; }
   *dest++ = (uint8_t) uc;
   out_size--;
  } else
  if (uc < 0x800)
  {
   if (out_size < 2) { result |= RESULT_BUF_ERROR; break; }
   *dest++ = 0xC0 | uc>>6;
   *dest++ = 0x80 | (uc & 0x3F);
   out_size -= 2;
  } else
  {
   if (out_size < 3) { result |= RESULT_BUF_ERROR; break; }
   *dest++ = 0xE0 | uc>>12;
   *dest++ = 0x80 | ((uc>>6) & 0x3F);
   *dest++ = 0x80 | (uc & 0x3F);
   out_size -= 3;
  }
  in_size--;
 }
 *in_size_ptr -= in_size;
 *out_size_ptr -= out_size;
 return result;
}

int convert_utf16_be(void *out, int *out_size_ptr, const void *in, int *in_size_ptr, char error_char)
{
 int result = 0;
 int in_size = *in_size_ptr;
 int out_size = *out_size_ptr;
 const uint8_t *src = in;
 uint8_t *dest = out;
 in_size &= ~1;
 while (in_size)
 {
  unsigned uc = src[0]<<8 | src[1];
  src += 2;
  in_size -= 2;
  if (uc >= 0xD800 && uc < 0xDC00)
  {
   if (in_size < 2)
   {
    result |= RESULT_HAS_ERRORS;
    uc = error_char;
   } else
   {
    unsigned uc_next = src[0]<<8 | src[1];
    if (uc_next >= 0xDC00 && uc_next < 0xE000)
    {
     uc = (uc & 0x3FF)<<10 | (uc_next & 0x3FF);
     src += 2;
     in_size -= 2;
    } else
    {
     result |= RESULT_HAS_ERRORS;
     uc = error_char;
    }
   }
  }
  if (uc < 0x80)
  {
   if (!out_size) { result |= RESULT_BUF_ERROR; break; }
   *dest++ = (uint8_t) uc;
   out_size--;
  } else
  if (uc < 0x800)
  {
   if (out_size < 2) { result |= RESULT_BUF_ERROR; break; }
   *dest++ = 0xC0 | uc>>6;
   *dest++ = 0x80 | (uc & 0x3F);
   out_size -= 2;
  } else
  if (uc < 0x10000)
  {
   if (out_size < 3) { result |= RESULT_BUF_ERROR; break; }
   *dest++ = 0xE0 | uc>>12;
   *dest++ = 0x80 | ((uc>>6) & 0x3F);
   *dest++ = 0x80 | (uc & 0x3F);
   out_size -= 3;
  } else
  {
   if (out_size < 4) { result |= RESULT_BUF_ERROR; break; }
   *dest++ = 0xF0 | uc>>18;
   *dest++ = 0x80 | ((uc>>12) & 0x3F);
   *dest++ = 0x80 | ((uc>>6) & 0x3F);
   *dest++ = 0x80 | (uc & 0x3F);
   out_size -= 4;
  }
 }
 *in_size_ptr -= in_size;
 *out_size_ptr -= out_size;
 return result;
}

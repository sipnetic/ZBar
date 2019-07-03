/*Copyright (C) 2008-2010  Timothy B. Terriberry (tterribe@xiph.org)
  You can redistribute this library and/or modify it under the terms of the
   GNU Lesser General Public License as published by the Free Software
   Foundation; either version 2.1 of the License, or (at your option) any later
   version.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qrcode.h"
#include "qrdec.h"
#include "util.h"
#include "image.h"
#include "error.h"
#include "img_scanner.h"
#include "text/charsets.h"
#include "text/text_conv.h"

#define ERROR_CHAR '?'

#define MOD(mod) (1 << (mod))

static int unescape_fnc1(char *out, int out_size, const void *in, int in_size, int *err)
{
 const uint8_t *src = in;
 int escape = 0;
 int result = 0;
 while (in_size)
 {
  if (*src == '%')
  {
   if (!escape)
   {
    escape = 1;
    in_size--;
    continue;
   }
   escape = 0;
  }
  if (escape)
  {
   if (out_size - result < 2) { *err = 1; return 0; }
   out[result++] = 0x1D; /* insert group separator */
   out[result++] = *src;
  } else
  {
   if (out_size <= result) { *err = 1; return 0; }
   out[result++] = *src;
  }
  in_size--;
 }
 if (escape) *err = 1;
 return result;
}

static int detect_charset(const void *data, int size)
{
 if (is_valid_utf8(data, size)) return CHARSET_UTF8;
 if (is_valid_shiftjis(data, size)) return CHARSET_SHIFT_JIS;
 return CHARSET_ISO8859_1;
}

static int eci_to_charset(int eci)
{
 switch (eci)
 {
  case QR_ECI_GLI0:
  case QR_ECI_CP437:
   return CHARSET_CP437;
  case QR_ECI_GLI1:
  case QR_ECI_ISO8859_1:
   return CHARSET_ISO8859_1;
  case QR_ECI_ISO8859_2:
   return CHARSET_ISO8859_2;
  case QR_ECI_ISO8859_3:
   return CHARSET_ISO8859_3;
  case QR_ECI_ISO8859_4:
   return CHARSET_ISO8859_4;
  case QR_ECI_ISO8859_5:
   return CHARSET_ISO8859_5;
  case QR_ECI_ISO8859_6:
   return CHARSET_ISO8859_6;
  case QR_ECI_ISO8859_7:
   return CHARSET_ISO8859_7;
  case QR_ECI_ISO8859_8:
   return CHARSET_ISO8859_8;
  case QR_ECI_ISO8859_9:
   return CHARSET_ISO8859_9;
  case QR_ECI_ISO8859_10:
   return CHARSET_ISO8859_10;
  case QR_ECI_ISO8859_11:
   return CHARSET_ISO8859_11;
  case QR_ECI_ISO8859_13:
   return CHARSET_ISO8859_13;
  case QR_ECI_ISO8859_14:
   return CHARSET_ISO8859_14;
  case QR_ECI_ISO8859_15:
   return CHARSET_ISO8859_15;
  case QR_ECI_ISO8859_16:
   return CHARSET_ISO8859_16;
  case QR_ECI_SHIFT_JIS:
   return CHARSET_SHIFT_JIS;
  case QR_ECI_CP1250:
   return CHARSET_CP1250;
  case QR_ECI_CP1251:
   return CHARSET_CP1251;
  case QR_ECI_CP1252:
   return CHARSET_CP1252;
  case QR_ECI_CP1256:
   return CHARSET_CP1256;
  case QR_ECI_UTF16_BE:
   return CHARSET_UTF16_BE;
  case QR_ECI_UTF8:
   return CHARSET_UTF8;
 }
 return CHARSET_NONE;
}

int qr_code_data_list_extract_text(const qr_code_data_list *qrlist,
                                   zbar_image_scanner_t *iscn,
                                   zbar_image_t *img)
{
 const qr_code_data *qrdata = qrlist->qrdata;
 int nqrdata = qrlist->nqrdata;
 uint8_t *mark = (uint8_t *) calloc(nqrdata, sizeof(*mark));
 int total_decoded = 0;
 int i;
 for (i = 0; i < nqrdata; i++)
  if (!mark[i])
  {
   const qr_code_data *qrdataj;
   const qr_code_data_entry *entry;
   int j, k;
   int sa[16];
   int sa_size;
   char *sa_text;
   int sa_ntext, sa_ctext;
   int fnc1;
   int fnc1_2ai;
   int eci;
   int err;
   zbar_symbol_t *syms = NULL, **sym = &syms;
   qr_point dir;
   int horiz;

   /*Step 0: Collect the other QR codes belonging to this S-A group.*/
   if (qrdata[i].sa_size)
   {
    unsigned sa_parity;
    sa_size = qrdata[i].sa_size;
    sa_parity = qrdata[i].sa_parity;
    for (j = 0; j < sa_size; j++) sa[j] = -1;
    for (j = i; j < nqrdata; j++)
     if (!mark[j])
     {
      /*TODO: We could also match version, ECC level, etc. if size and
        parity alone are too ambiguous.*/
      if (qrdata[j].sa_size == sa_size &&
          qrdata[j].sa_parity == sa_parity &&
          sa[qrdata[j].sa_index] < 0)
      {
       sa[qrdata[j].sa_index] = j;
       mark[j] = 1;
      }
     }
    /*TODO: If the S-A group is complete, check the parity.*/
   } else
   {
    sa[0] = i;
    sa_size = 1;
   }

   sa_ctext = 0;
   fnc1 = 0;
   fnc1_2ai = 0;
   /*Step 1: Detect FNC1 markers and estimate the required buffer size.*/
   for (j = 0; j < sa_size; j++)
    if (sa[j] >= 0)
    {
     qrdataj = qrdata + sa[j];
     for (k = 0; k < qrdataj->nentries; k++)
     {
      int mult = 0;
      entry = qrdataj->entries + k;
      switch (entry->mode)
      {
       /*FNC1 applies to the entire code and ignores subsequent markers.*/
       case QR_MODE_FNC1_1ST:
        if (!fnc1) fnc1 = MOD(ZBAR_MOD_GS1);
        break;

       case QR_MODE_FNC1_2ND:
        if (!fnc1)
        {
         fnc1 = MOD(ZBAR_MOD_AIM);
         fnc1_2ai = entry->payload.ai;
         sa_ctext += 2;
        }
        break;

       case QR_MODE_NUM:
       case QR_MODE_ALNUM:
        mult = 1;
        break;

       case QR_MODE_KANJI:
       case QR_MODE_BYTE:
        mult = 3;
        break;
      }
      if (mult) sa_ctext += entry->payload.data.len * mult;
     }
    } else sa_ctext++;

   /*Step 2: Convert the entries.*/
   sa_text = (char *) malloc(sa_ctext + 1);
   sa_ntext = 0;

   /*Add the encoded Application Indicator for FNC1 in the second position.*/
   if (fnc1 == MOD(ZBAR_MOD_AIM))
   {
    if (fnc1_2ai < 100)
    {
     /*The Application Indicator is a 2-digit number.*/
     sa_text[sa_ntext++] = '0' + fnc1_2ai / 10;
     sa_text[sa_ntext++] = '0' + fnc1_2ai % 10;
    }
    /*The Application Indicator is a single letter.
      We already checked that it lies in one of the ranges A...Z, a...z
      when we decoded it.*/
    else
     sa_text[sa_ntext++] = (char) (fnc1_2ai - 100);
   }

   eci = -1;
   err = 0;
   for (j = 0; j < sa_size && !err; j++, sym = &(*sym)->next)
   {
    *sym = _zbar_image_scanner_alloc_sym(iscn, ZBAR_QRCODE, 0);
    (*sym)->datalen = sa_ntext;
    if (sa[j] < 0)
    {
     /* generic placeholder for unfinished results */
     (*sym)->type = ZBAR_PARTIAL;

     /*Skip all contiguous missing segments.*/
     for (j++; j < sa_size && sa[j] < 0; j++);

     /*If there aren't any more, stop.*/
     if (j >= sa_size) break;

     /* mark break in data */
     sa_text[sa_ntext++] = 0;

     /* advance to next symbol */
     sym = &(*sym)->next;
     *sym = _zbar_image_scanner_alloc_sym(iscn, ZBAR_QRCODE, 0);
     (*sym)->datalen = sa_ntext;
    }

    qrdataj = qrdata + sa[j];
    /* expose bounding box */
    sym_add_point(*sym, qrdataj->bbox[0][0], qrdataj->bbox[0][1]);
    sym_add_point(*sym, qrdataj->bbox[2][0], qrdataj->bbox[2][1]);
    sym_add_point(*sym, qrdataj->bbox[3][0], qrdataj->bbox[3][1]);
    sym_add_point(*sym, qrdataj->bbox[1][0], qrdataj->bbox[1][1]);

    /* approx symbol "up" direction */
    dir[0] = (qrdataj->bbox[0][0] - qrdataj->bbox[2][0] +
              qrdataj->bbox[1][0] - qrdataj->bbox[3][0]);
    dir[1] = (qrdataj->bbox[2][1] - qrdataj->bbox[0][1] +
              qrdataj->bbox[3][1] - qrdataj->bbox[1][1]);
    horiz = abs(dir[0]) > abs(dir[1]);
    (*sym)->orient = horiz + 2 * (dir[1 - horiz] < 0);

    for (k = 0; k < qrdataj->nentries && !err; k++)
    {
     int len, charset, result, conv_size;
     entry = qrdataj->entries + k;
     switch (entry->mode)
     {
      case QR_MODE_NUM:
       len = entry->payload.data.len;
       if (len > sa_ctext - sa_ntext) { err = 1; break; }
       memcpy(sa_text + sa_ntext, entry->payload.data.buf, len);
       sa_ntext += len;
       break;

      case QR_MODE_ALNUM:
       len = entry->payload.data.len;
       if (fnc1)
       {
        len = unescape_fnc1(sa_text + sa_ntext, sa_ctext - sa_ntext, entry->payload.data.buf, len, &err);
        if (err) break;
       } else
       {
        if (len > sa_ctext - sa_ntext) { err = 1; break; }
        memcpy(sa_text + sa_ntext, entry->payload.data.buf, len);
       }
       sa_ntext += len;
       break;

      case QR_MODE_BYTE:
       len = entry->payload.data.len;
       if (eci < 0)
        charset = detect_charset(entry->payload.data.buf, len);
       else
        charset = eci_to_charset(eci);
       if (charset == CHARSET_NONE) continue; /* Can't convert, unknown encoding */
       conv_size = sa_ctext - sa_ntext;
       result = convert_to_utf8(sa_text + sa_ntext, &conv_size, entry->payload.data.buf, &len, ERROR_CHAR, charset);
       if (result & (RESULT_BUF_ERROR | RESULT_BAD_CHARSET))
        err = 1;
       else
        sa_ntext += conv_size;
       break;

      case QR_MODE_KANJI:
       len = entry->payload.data.len;
       conv_size = sa_ctext - sa_ntext;
       result = convert_to_utf8(sa_text + sa_ntext, &conv_size, entry->payload.data.buf, &len, ERROR_CHAR, CHARSET_SHIFT_JIS);
       if (result & (RESULT_BUF_ERROR | RESULT_BAD_CHARSET))
        err = 1;
       else
        sa_ntext += conv_size;
       break;

      case QR_MODE_ECI:
       eci = entry->payload.eci;
     }
     /*If eci should be reset between codes, do so.*/
     if (eci <= QR_ECI_GLI1) eci = -1;
    }
   }
   if (!err)
   {
    zbar_symbol_t *sa_sym;
    sa_text[sa_ntext++] = 0;
    if (sa_ctext + 1 > sa_ntext)
     sa_text = (char *) realloc(sa_text, sa_ntext * sizeof(*sa_text));

    if (sa_size == 1)
     sa_sym = syms;
    else
    {
     /* cheap out w/axis aligned bbox for now */
     int xmin = img->width, xmax = -2;
     int ymin = img->height, ymax = -2;

     /* create "virtual" container symbol for composite result */
     sa_sym = _zbar_image_scanner_alloc_sym(iscn, ZBAR_QRCODE, 0);
     sa_sym->syms = _zbar_symbol_set_create();
     sa_sym->syms->head = syms;

     /* fixup data references */
     for (; syms; syms = syms->next)
     {
      int next;
      _zbar_symbol_refcnt(syms, 1);
      if (syms->type == ZBAR_PARTIAL)
       sa_sym->type = ZBAR_PARTIAL;
      else
       for (j = 0; j < (int) syms->npts; j++)
       {
        int u = syms->pts[j].x;
        if (xmin >= u) xmin = u - 1;
        if (xmax <= u) xmax = u + 1;
        u = syms->pts[j].y;
        if (ymin >= u) ymin = u - 1;
        if (ymax <= u) ymax = u + 1;
       }
      syms->data = sa_text + syms->datalen;
      next = (syms->next) ? syms->next->datalen : sa_ntext;
      assert(next > (int) syms->datalen);
      syms->datalen = next - syms->datalen - 1;
     }
     if (xmax >= -1)
     {
      sym_add_point(sa_sym, xmin, ymin);
      sym_add_point(sa_sym, xmin, ymax);
      sym_add_point(sa_sym, xmax, ymax);
      sym_add_point(sa_sym, xmax, ymin);
     }
    }
    sa_sym->data = sa_text;
    sa_sym->data_alloc = sa_ntext;
    sa_sym->datalen = sa_ntext - 1;
    sa_sym->modifiers = fnc1;

    _zbar_image_scanner_add_sym(iscn, sa_sym);
    total_decoded++;
   } else
   {
    _zbar_image_scanner_recycle_syms(iscn, syms);
    free(sa_text);
   }
  }
 free(mark);
 return total_decoded;
}

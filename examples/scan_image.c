#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <png.h>
#include <zbar.h>

#if !defined(PNG_LIBPNG_VER) || \
    PNG_LIBPNG_VER < 10018 ||   \
    (PNG_LIBPNG_VER > 10200 &&  \
     PNG_LIBPNG_VER < 10209)
  /* Changes to Libpng from version 1.2.42 to 1.4.0 (January 4, 2010)
   * ...
   * 2. m. The function png_set_gray_1_2_4_to_8() was removed. It has been
   *       deprecated since libpng-1.0.18 and 1.2.9, when it was replaced with
   *       png_set_expand_gray_1_2_4_to_8() because the former function also
   *       expanded palette images.
   */
# define png_set_expand_gray_1_2_4_to_8 png_set_gray_1_2_4_to_8
#endif

static int get_data(const char *name, int *width, int *height, void **raw)
{
    FILE *file;
    png_structp png;
    png_infop info;
    int color, bits, row_bytes;
    png_byte **rows = NULL;
    int i;
    int result = 0;
    
    *raw = NULL;

    file = fopen(name, "rb");
    if (!file)
    {
        fprintf(stderr, "Can't open %s: %s\n", name, strerror(errno));
        goto cleanup;
    }
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
    {
        fprintf(stderr, "Can't read %s: not a valid PNG\n", name);
        goto cleanup;
    }
    if (setjmp(png_jmpbuf(png)))
    {
        goto cleanup;
    }
    info = png_create_info_struct(png);
    png_init_io(png, file);
    png_read_info(png, info);
    /* configure for 8bpp grayscale input */
    color = png_get_color_type(png, info);
    bits = png_get_bit_depth(png, info);
    if (color == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_palette_to_rgb(png);
        //png_read_update_info(png, info);
        //color = png_get_color_type(png, info);
        //bits = png_get_bit_depth(png, info);
        color |= PNG_COLOR_MASK_ALPHA;
    } else
    if (color == PNG_COLOR_TYPE_GRAY && bits < 8)
        png_set_expand_gray_1_2_4_to_8(png);
    if (bits == 16)
        png_set_strip_16(png);
    if (color & PNG_COLOR_MASK_COLOR)
        png_set_rgb_to_gray_fixed(png, 1, -1, -1);
    if (color & PNG_COLOR_MASK_ALPHA)
        png_set_strip_alpha(png);
    png_read_update_info(png, info);
    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    row_bytes = png_get_rowbytes(png, info);
    if (row_bytes != *width)
    {
        fprintf(stderr, "Can't convert %s to grayscale: something wrong with libPNG\n", name);
        goto cleanup;
    }
    /* allocate image */
    *raw = malloc(*width * *height);
    rows = malloc(*height * sizeof(png_byte*));
    for (i = 0; i < *height; i++)
        rows[i] = (png_byte *) *raw + (row_bytes * i);
    png_read_image(png, rows);
    result = 1;
    
    cleanup:
    png_destroy_read_struct(&png, NULL, NULL);
    if (!result)
    {
        free(*raw);
        *raw = NULL;
    }
    free(rows);
    fclose(file);
    return result;
}

int main(int argc, char **argv)
{
    zbar_image_scanner_t *scanner;
    zbar_image_t *image;
    const zbar_symbol_t *symbol;
    int width, height, n;
    void *raw;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <PNG_File>\n", argv[0]);
        return 1;
    }

    /* create a reader */
    scanner = zbar_image_scanner_create();

    /* configure the reader */
    //zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);

    /* obtain image data */
    if (!get_data(argv[1], &width, &height, &raw)) return 2;

    /* wrap image data */
    image = zbar_image_create();
    zbar_image_set_format(image, zbar_fourcc('Y','8','0','0'));
    zbar_image_set_size(image, width, height);
    zbar_image_set_data(image, raw, width * height, zbar_image_std_cleanup);

    /* scan the image for barcodes */
    n = zbar_scan_image(scanner, image);

    /* extract results */
    for (symbol = zbar_image_first_symbol(image); symbol; symbol = zbar_symbol_next(symbol))
    {
        /* do something useful with results */
        zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
        const char *data = zbar_symbol_get_data(symbol);
        printf("decoded %s symbol \"%s\"\n",
               zbar_get_symbol_name(typ), data);
    }

    /* clean up */
    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);

    return 0;
}

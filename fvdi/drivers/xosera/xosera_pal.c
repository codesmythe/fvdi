/* 
 * Palette handling routines for Xosera.
 */

#include "xosera.h"
#include "driver.h"
#include <stdint.h>

/* Xosera has 4 bits for each color channel. */

#define red_bits   4
#define green_bits 4
#define blue_bits  4

#define PIXEL                unsigned char

static unsigned char tos_colours[] = {0, 0xF, 1, 2, 4, 6, 3, 5, 7, 8, 9, 10, 12, 14, 11, 13};

long CDECL
c_get_colour(Virtual *UNUSED(vwk), long colour)
{
    uint16_t foreground, background;
    uint16_t fg_in = colour & 0xFF, bg_in = (colour >> 16) & 0xFF;

    if (fg_in < 16) foreground = tos_colours[fg_in & 0xF];
    else if (fg_in == 0xFF) foreground = 15;
    else foreground = fg_in;

    if (bg_in < 16) background = tos_colours[bg_in & 0xF];
    else if (bg_in == 0xFF) background = 15;
    else background = bg_in;

    long result = ((long) background << 16) | (long) foreground;
    return result;
}

uint8_t indices[16];

static void set_colour(long paletteIndex, long red, long green, long blue)
{
    //this code sets up a pallete index with the specified value
    uint8_t shortInd = (uint8_t) tos_colours[paletteIndex & 0xF] & 0xf;
    if (indices[shortInd] == 1) {
        //but after setting it up, it reset all the colors to 0 for some reason,
        // so i made it impossible for it to change a color after it has been set once
        return;
    }
    volatile xmreg_t *const xosera_ptr = xv_prep_dyn(me->device);
    uint16_t r = red & 0xF;
    uint16_t g = green & 0xF;
    uint16_t b = blue & 0xF;
    uint16_t c = (r << 8) | (g << 4) | b;
    xosera_palette_register_write(xosera_ptr, shortInd, c);
    indices[shortInd] = 1;
}

void CDECL
c_set_colours(Virtual *UNUSED(vwk), long start, long entries, unsigned short *requested, Colour palette[])
{
    unsigned long colour;
    unsigned short component;
    unsigned long tc_word;
    int i;
    // I have no idea what all of this does, i just copied it from another driver, crashes without it
    if ((long) requested & 1) {                        /* New entries? */
        //access->funcs.puts("c_set_colours: low bit of 'requested' is set.\n");
        requested = (unsigned short *) ((long) requested & 0xfffffffeL);
        for (i = 0; i < entries; i++) {
            requested++;                                /* First word is reserved */
            component = *requested++;
            palette[start + i].vdi.red = component;
            palette[start + i].hw.red = component;        /* Not at all correct */
            colour = component >> (16 - red_bits);        /* (component + (1 << (14 - red_bits))) */
            tc_word = colour << green_bits;
            component = *requested++;
            palette[start + i].vdi.green = component;
            palette[start + i].hw.green = component;        /* Not at all correct */
            colour = component >> (16 - green_bits);        /* (component + (1 << (14 - green_bits))) */
            tc_word |= colour;
            tc_word <<= blue_bits;
            component = *requested++;
            palette[start + i].vdi.blue = component;
            palette[start + i].hw.blue = component;        /* Not at all correct */
            colour = component >> (16 - blue_bits);                /* (component + (1 << (14 - blue_bits))) */
            tc_word |= colour;
            set_colour(start + i,
                         palette[start + i].vdi.red,
                         palette[start + i].vdi.green,
                         palette[start + i].vdi.blue);

            *(PIXEL *) &palette[start + i].real = (PIXEL) tc_word;
        }
    } else {
        //access->funcs.puts("c_set_colours: low bit of 'requested' is not set.\n");
        // In this case, we are being asked to set the hardware palette.
        // For each entry, we have the requested red, green and blue VDI values. A VDI
        // color value ranges from 0 to 1000.

        for (i = 0; i < entries; i++) {
            //PRINTF(("    entry %d: red = %d, green = %d, blue = %d\n", i, requested[0], requested[1], requested[2]));

            component = *requested++;
            palette[start + i].vdi.red = component;
            long temp = (component * ((1L << red_bits) - 1) + 500L) / 1000;
            palette[start + i].hw.red = (short) temp;
            tc_word = palette[start + i].hw.red << green_bits;

            component = *requested++;
            palette[start + i].vdi.green = component;
            temp = (component * ((1L << green_bits) - 1) + 500L) / 1000;
            palette[start + i].hw.green = (short) temp;

            tc_word |= temp;
            tc_word <<= blue_bits;

            component = *requested++;
            palette[start + i].vdi.blue = component;
            temp = (component * ((1L << blue_bits) - 1) + 500L) / 1000;
            palette[start + i].hw.blue = (short) temp;
            tc_word |= temp;

            //PRINTF(("        hw.red = %d, hw.blue = %d, hw.green = %d, truecolor = 0x%08lx\n",
            //    palette[start + i].hw.red, palette[start + i].hw.blue, palette[start + i].hw.green, tc_word));

            set_colour(start + i,
                         palette[start + i].hw.red,
                         palette[start + i].hw.green,
                         palette[start + i].hw.blue);

            *(PIXEL *) &palette[start + i].real = (PIXEL) tc_word;
        }
    }
}

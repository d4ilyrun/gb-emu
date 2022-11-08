/**
 * \file lcd.c
 * \authors Léo DUBOIN <leo@duboin.com>
 *
 * The Game Boy LCD is 160×144 pixels.
 *
 * It can display 4 gray shades in DMG/MGB/SGB and 15-bit depth colors in
 * CGB/AGB/AGS. It can show a background and a window (another background over
 * the first one), and up to 40 sprites (10 per line) of 8×8 or 8×16 (each one
 * the same size).
 *
 * Color palettes:
 * - The DMG has 1 palette of 4 gray shades for backgrounds and 2
 * palettes of 3 gray shades for sprites (color 0 is transparent).
 * - The GBC has 8 palettes of 4 colors for backgrounds and 8 palettes of 3
 * colors for sprites (color 0 is transparent).
 *
 * A vertical refresh happens every 70224 clocks (140448 in GBC double speed
 * mode): 59,7275 Hz
 */

#include "ppu/lcd.h"

#include <string.h>

#include "utils/error.h"
#include "utils/macro.h"

/// LCD context static variable
static struct lcd g_lcd;

typedef shade palette[4];

static palette default_palette = {
    0xFFFFFFFF, // White
    0xFFAAAAAA, // Light grey
    0xFF555555, // Dark grey
    0xFF000000, // Black
};

static union palettes {
    struct {
        palette bg;
        palette obj[2];
    } dmg;
    struct {
        // TODO: GBC
    } cgb;
} g_palettes;

// Forward definitions
static void lcd_update_palette(palette_name index, u8 data);

const struct lcd *get_lcd()
{
    return &g_lcd;
}

void init_lcd()
{
    // registers
    g_lcd.lcdc = 0x91;
    g_lcd.scx = 0;
    g_lcd.scy = 0;
    g_lcd.wx = 0;
    g_lcd.wy = 0;
    g_lcd.ly = 0;
    g_lcd.lyc = 0;

    // palettes
    g_lcd.dmg.bgp = 0xFC;
    g_lcd.dmg.obp[0] = 0xFF;
    g_lcd.dmg.obp[1] = 0xFF;

    // Set default palette values
    memcpy(g_palettes.dmg.bg, default_palette, sizeof(palette));
    memcpy(g_palettes.dmg.obj[0], default_palette, sizeof(palette));
    memcpy(g_palettes.dmg.obj[1], default_palette, sizeof(palette));

    // Stat
    g_lcd.stat = 0;
    g_lcd.stat |= (1 << 2); // Set LYC
}

void write_lcd(u16 address, u8 value)
{
    u8 *lcd_ptr = (u8 *)&g_lcd;

    switch (address) {
    case 0xFF41:
        // 3 lower bits are read only and bit 7 is always set
        g_lcd.stat &= 0x7;
        g_lcd.stat |= (value & 0xF8);
        g_lcd.stat |= 0x80;
        return;

    case 0xFF44:
        log_warn("LCD: Writing to read only address (" HEX "). Skipping.",
                 address);
        return;

    case 0xFF46:
        NOT_IMPLEMENTED("OAM DMA Transfer");
        return;

    case 0xFF47:
    case 0xFF48:
    case 0xFF49:
        lcd_update_palette(address - 0xFF47, value);
        __attribute__((fallthrough));

    default:
        if (address <= 0xFF4B) {
            lcd_ptr[address - 0xFF40] = value;
        } else if (BETWEEN(address, 0xFF68, 0xFF6B)) {
            // CGB 0xFF68-6B
            u8 *cgb_lcd_ptr = (u8 *)&g_lcd.cgb_colors;
            cgb_lcd_ptr[address - 0xFF68] = value;
        } else {
            log_warn("LCD: Invalid write address: " HEX ". Skipping.", address);
        }
    }

    // If changed LY or LYC, update LYC flag inside STAT
    if (BETWEEN(address, 0xFF44, 0xFF45)) {
        if (g_lcd.ly == g_lcd.lyc)
            g_lcd.stat |= (1 << 2); // Set LYC flag
        else
            g_lcd.stat &= ~(1 << 2); // Clear LYC flag
    }
}

u8 read_lcd(u16 address)
{
    ASSERT_MSG(BETWEEN(address, 0xFF40, 0xFF4A), "Invalid LCD read address: %d",
               address);

    return ((u8 *)&g_lcd)[address - 0xFF40];
}

shade *lcd_get_palette(palette_name index)
{
    ASSERT_MSG(index < INVALID, "LCD: Invalid palette index %d.", index);

    // Get palette from index
    void *palette_at_index_ptr = g_palettes.dmg.bg;
    palette_at_index_ptr += sizeof(palette);

    return palette_at_index_ptr;
}

/**
 * \brief Update a the color values of a palette
 *
 * Used when writing to 0xFF47-49.
 *
 * Color palettes:
 *  - The DMG has 1 palette of 4 gray shades for backgrounds and 2
 *    palettes of 3 gray shades for sprites (color 0 is transparent).
 *  - The GBC has 8 palettes of 4 colors for backgrounds and 8 palettes of 3
 *    colors for sprites (color 0 is transparent).
 *
 * A shade is a 32-bit value.
 *
 * The update is done by assigning shades from \c default_palette to the
 * selected palette. Each group of 2 bits inside the written 8-bit value
 * is used as an index to the corresponding shade inside \c default_palette.
 *
 * Example:
 *  - Writing 0xB3 -> 0b10110110
 *  - Resulting palette (4 shades):
 *      - 0xFF555555 (default_palette[0b10])
 *      - 0xFF000000 (default_palette[0b11])
 *      - 0xFFAAAAAA (default_palette[0b01])
 *      - 0xFF555555 (default_palette[0b10])
 *
 * Note:
 *  The lower 2 bits are ignored when updating OBJ palettes
 *  because color index 0 is transparent for OBJs.
 *
 * \param index The index of the palette
 * \param data The indexes used for udpating
 */
static void lcd_update_palette(palette_name name, u8 data)
{
    shade *palette_ptr = lcd_get_palette(name);

#define SHADE(_data, _index) default_palette[((_data) >> (2 * (_index))) & 0b11]

    palette_ptr[0] = SHADE(data, 0);
    palette_ptr[1] = SHADE(data, 1);
    palette_ptr[2] = SHADE(data, 2);

    // Lower 2 bits ignored when updating OBJS
    if (name == BG_PALETTE)
        palette_ptr[3] = SHADE(data, 3);

#undef SHADE
}

void lcd_set_mode(lcd_mode mode)
{
    // Change bits 0-1 of the stat register (mode flag)
    g_lcd.stat = (g_lcd.stat & 0xFC) | (mode & 0x3);
}

lcd_mode lcd_get_mode()
{
    return LCD_STAT_MODE_FLAG(g_lcd);
}

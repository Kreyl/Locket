/*
 * color.h
 *
 *  Created on: 05 апр. 2014 г.
 *      Author: Kreyl
 */

#pragma once

#include "inttypes.h"
#include <sys/cdefs.h>

// Mixing two colors
#define ClrMix(C, B, L)     ((C * L + B * (255 - L)) / 255)

struct Color_t {
    union {
        struct {
            uint8_t R, G, B;
        };
        uint32_t DWord32;
    };
    bool operator == (const Color_t &AColor) const { return ((R == AColor.R) and (G == AColor.G) and (B == AColor.B)); }
    bool operator != (const Color_t &AColor) const { return ((R != AColor.R) or  (G != AColor.G) or  (B != AColor.B)); }
    Color_t& operator = (const Color_t &Right) { R = Right.R; G = Right.G; B = Right.B; return *this; }
    void Adjust(const Color_t *PColor) {
        if     (R < PColor->R) R++;
        else if(R > PColor->R) R--;
        if     (G < PColor->G) G++;
        else if(G > PColor->G) G--;
        if     (B < PColor->B) B++;
        else if(B > PColor->B) B--;
    }
    void Set(uint8_t Red, uint8_t Green, uint8_t Blue) { R = Red; G = Green; B = Blue; }
    void Set(const Color_t* p) { DWord32 = p->DWord32; }
    void Get(uint8_t *PR, uint8_t *PG, uint8_t *PB) const { *PR = R; *PG = G; *PB = B; }
    uint8_t RGBTo565_HiByte() const {
        uint32_t rslt = R & 0b11111000;
        rslt |= G >> 5;
        return (uint8_t)rslt;
    }
    uint8_t RGBTo565_LoByte() const {
        uint32_t rslt = (G << 3) & 0b11100000;
        rslt |= B >> 3;
        return (uint8_t)rslt;
    }
    uint16_t RGBTo565() const {
        uint16_t rslt = ((uint16_t)(R & 0b11111000)) << 8;
        rslt |= ((uint16_t)(G & 0b11111100)) << 3;
        rslt |= ((uint16_t)B) >> 3;
        return rslt;
    }
    void MixOf(Color_t &Fore, Color_t &Back, uint32_t Brt) {
        R = ClrMix(Fore.R, Back.R, Brt);
        G = ClrMix(Fore.G, Back.G, Brt);
        B = ClrMix(Fore.B, Back.B, Brt);
    }
} __attribute__((packed));

#define RED_OF(c)           (((c) & 0xF800)>>8)
#define GREEN_OF(c)         (((c)&0x007E)>>3)
#define BLUE_OF(c)          (((c)&0x001F)<<3)

static inline uint16_t RGBTo565(uint16_t r, uint16_t g, uint16_t b) {
    uint16_t rslt = (r & 0b11111000) << 8;
    rslt |= (g & 0b11111100) << 3;
    rslt |= b >> 3;
    return rslt;
}

// Blend two colors according to the alpha;
// The alpha value (0-255). 0 is all background, 255 is all foreground.
__unused
static uint16_t ColorBlend(Color_t fg, Color_t bg, uint16_t alpha) {
    uint16_t fg_ratio = alpha + 1;
    uint16_t bg_ratio = 256 - alpha;
    uint16_t r, g, b;

    r = fg.R * fg_ratio;
    g = fg.G * fg_ratio;
    b = fg.B * fg_ratio;

    r += bg.R * bg_ratio;
    g += bg.G * bg_ratio;
    b += bg.B * bg_ratio;

    r >>= 8;
    g >>= 8;
    b >>= 8;

    return RGBTo565(r, g, b);
}

// ==== Colors ====
#define clBlack     ((Color_t){0,   0,   0})
#define clRed       ((Color_t){255, 0,   0})
#define clGreen     ((Color_t){0,   255, 0})
#define clBlue      ((Color_t){0,   0,   255})
#define clYellow    ((Color_t){255, 255, 0})
#define clMagenta   ((Color_t){255, 0, 255})
#define clCyan      ((Color_t){0, 255, 255})
#define clWhite     ((Color_t){255, 255, 255})

#define clDarkGreen ((Color_t){0, 54, 00})

#define clLightBlue ((Color_t){90, 90, 255})
#define clDarkBlue  ((Color_t){0, 0, 90})

#define clGrey      ((Color_t){126, 126, 126})
#define clLightGrey ((Color_t){180, 180, 180})
#define clDarkGrey  ((Color_t){54, 54, 54})

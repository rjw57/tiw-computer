#include "crtc6845.h"
#include "types.h"

u8 CRTC6845_MODE_1[16] = {
    0x68, // horiz total
    0x50, // horiz disp
    0x52, // hsync pos
    0x38, // vsync & hsync widths
    0x1F, // vert total
    0x04, // vert adjust
    0x1E, // vert displayed
    0x1E, // vsync pos
    0x00, // interlace mode
    0x0F, // max row addr
    0x00, // cursor start
    0x00, // cursor end
    0x00, // start addr (H)
    0x00, // start addr (L)
    0x00, // cursor (H)
    0x00, // cursor (L)
};

// Write a byte to a CRTC6845 register.
#define CRTC6845_WRITE_REG(reg, value) do { \
    __AX__ = ((u16)((reg & 0xff))); \
    __asm__("sta __CRTC6845_START__"); \
    __AX__ = ((u16)((value & 0xff))); \
    __asm__("sta __CRTC6845_START__ + 1"); \
} while(0)

void crt6845_init(u8 mode[]) {
    u8 i;
    for(i=0; i<=9; ++i) {
        CRTC6845_WRITE_REG(i, mode[i]);
    }
}

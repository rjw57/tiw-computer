#ifndef CRTC6845_H__
#define CRTC6845_H__

#include "types.h"

extern const u8 CRTC6845_MODE_1[];

void crt6845_init(const u8 mode[]);
void crt6845_set_screen_start(void* p);

#endif // CRTC6845_H__

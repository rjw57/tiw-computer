// ascii control codes
#ifndef ASCII_H__
#define ASCII_H__

#include "types.h"

#define ASCII_BEL   ((u8)0x07)
#define ASCII_BS    ((u8)0x08)
#define ASCII_LF    ((u8)0x0A)
#define ASCII_CR    ((u8)0x0D)
#define ASCII_DEL   ((u8)0x7F)

bool ascii_is_printable(u8 c);

#endif // ASCII_H__

#include "ascii.h"
#include "types.h"

bool ascii_is_printable(u8 c) {
    return (c >= 0x20) && (c < 0x7F);
}

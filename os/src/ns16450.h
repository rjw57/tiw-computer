#ifndef NS16450_H__
#define NS16450_H__

#include "types.h"

#define NS16450_REG_BUFFER          ((u8)0u) // Rx/Tx buffer
#define NS16450_REG_IRQ_ENABLE      ((u8)1u)
#define NS16450_REG_IRQ_ID          ((u8)2u)
#define NS16450_REG_LINE_CTRL       ((u8)3u)
#define NS16450_REG_MODEM_CTRL      ((u8)4u)
#define NS16450_REG_LINE_STATUS     ((u8)5u)
#define NS16450_REG_MODEM_STATUS    ((u8)6u)
#define NS16450_REG_SCRATCH         ((u8)7u)

#define NS16450_REG_DIVISOR_LSB     NS16450_REG_BUFFER
#define NS16450_REG_DIVISOR_MSB     NS16450_REG_IRQ_ENABLE

// Write a byte to a NS16450 register. Reg must be a compile time constant.
#define NS16450_A_WRITE_REG(reg, value) do { \
    __AX__ = ((u16)(value & 0xff)); \
    __asm__("sta __NS16450_A_START__ + %b", (u8)reg); \
} while(0)

// Read a byte from a NS16450 register. Reg must be a compile time constant.
#define NS16450_A_READ_REG(reg) ( \
    __asm__("lda __NS16450_A_START__ + %b", (u8)reg), \
    (u8)__AX__ \
)

// Initialise first NS16450 to 1 start bit, 1 stop bit, 8 bits, 19200 baud, no
// parity, no echo and with all interrupts disabled.
void ns16450_a_init(void);

#define ns16450_a_wait_txd_empty() do { \
    __asm__("lda __NS16450_A_START__ + %b", ((u8)NS16450_REG_LINE_STATUS)); \
    __asm__("and #$20"); \
} while(!(__AX__ & 0xFF))

#define ns16450_a_wait_rxd_full() do { \
    __asm__("lda __NS16450_A_START__ + %b", ((u8)NS16450_REG_LINE_STATUS)); \
    __asm__("and #$01"); \
} while(!(__AX__ & 0xFF))

// Send a byte via NS16450_a. Blocks until the byte is sent.
#define ns16450_a_send(val) do { \
    ns16450_a_wait_txd_empty(); \
    NS16450_A_WRITE_REG(NS16450_REG_BUFFER, (u8)(val)); \
} while(0)

// Receive a byte from NS16450_a. Returns -ve value if no byte to read.
#define ns16450_a_recv() ( \
    ( \
        __asm__("lda __NS16450_A_START__ + %b", ((u8)NS16450_REG_LINE_STATUS)), \
        __asm__("and #$01"), \
        __AX__ & 0xFF \
    ) ? ( \
        (i16)(NS16450_A_READ_REG(NS16450_REG_BUFFER)) \
    ) : ((i16)-1) \
)

#endif // NS16450_H__

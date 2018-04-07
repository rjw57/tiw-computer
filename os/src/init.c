#include "interrupt.h"
#include "ascii.h"
#include "types.h"
#include "cli.h"

#include "ns16450.h"

// Entry point for OS. When called interrupts are disabled, zero-page is
// initialised to zero and the stack pointer is set up.
//
// This function should not exit.
void init(void);

// Idle loop routine. Called repeatedly until the end of time.
void idle(void);

void init(void) {
    // initialise hw peripherals
    ns16450_a_init();

    // enable interrupts
    IRQ_ENABLE();

    ns16450_a_send('T');
    ns16450_a_send('I');
    ns16450_a_send('W');
    ns16450_a_send('\r');
    ns16450_a_send('\n');

    // loop forever
    while(1) { idle(); }
}

void println(const char* s) {
    for(; *s != '\0'; ++s) { ns16450_a_send(*s); }
    ns16450_a_send(ASCII_CR);
    ns16450_a_send(ASCII_LF);
}

u16 parse_hex(const char* s, u8* count) {
    u16 v = 0;
    for(*count=0; ; ++s, ++(*count)) {
        u8 c = *s;
        if(*count >= 4) { return v; }
        if((c >= '0') && (c <= '9')) {
            v <<= 4;
            v += c - '0';
        } else if((c >= 'A') && (c <= 'F')) {
            v <<= 4;
            v += c - ('A' - 0xA);
        } else if((c >= 'a') && (c <= 'f')) {
            v <<= 4;
            v += c - ('a' - 0xA);
        } else {
            return v;
        }
    }
}

void print_hex(u16 v, u8 w) {
    for(; w>0; --w) {
        u8 n = (v >> ((w-1)<<2)) & 0x0F;
        if(n <= 9) {
            ns16450_a_send('0' + n);
        } else {
            ns16450_a_send(('A' - 0x0A) + n);
        }
    }
}

void idle(void) {
    u8* buf;
    u16 arg1, arg2;
    u8 count = 0;

    cli_get();
    buf = cli_buf;

    switch(buf[0]) {
        case 'r':
            // read byte
            ++buf;
            for(; *buf == ' '; ++buf) { }
            arg1 = parse_hex(buf, &count); buf+=count;
            print_hex(*((u8*)(void*)arg1), 2);
            println("");
            break;
        case 'w':
            // write bytes
            ++buf;
            for(; *buf == ' '; ++buf) { }
            arg1 = parse_hex(buf, &count); buf+=count;
            while(1) {
                for(; *buf == ' '; ++buf) { }
                if(*buf == '\0') { break; }
                arg2 = parse_hex(buf, &count); buf+=count;
                *((u8*)(void*)(arg1++)) = (u8)arg2;
            }
            break;
        default:
            println("?");
    }
}

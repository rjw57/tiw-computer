#include "interrupt.h"
#include "types.h"

// Defined by linker
extern u8 *_DATA_RUN__, *_DATA_LOAD__;
extern u16 _DATA_SIZE__;

// Entry point for OS. When called interrupts are disabled, zero-page is
// initialised to zero and the stack pointer is set up.
//
// This function should not exit.
void init(void);

// Idle loop routine. Called repeatedly until the end of time.
void idle(void);

void init(void) {
    // initialise hw peripherals
//    acia1_init();
//    vdp1_init();

    // enable interrupts
//    IRQ_ENABLE();

//    puts("Hello, world\n");

    // loop forever
    while(1) { idle(); }
}

void idle(void) {
    // Poll ACIA
//    i16 recv_byte = acia1_recv();
//    if(recv_byte >= 0) {
//        acia1_send(recv_byte);
//    }
}

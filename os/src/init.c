#include "interrupt.h"
#include "types.h"

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

void idle(void) {
    ns16450_a_wait_rxd_full();
    ns16450_a_send(ns16450_a_recv());
}

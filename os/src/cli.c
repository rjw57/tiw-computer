#include "cli.h"
#include "ascii.h"
#include "types.h"

#include "ns16450.h"

char cli_buf[CLI_MAX_LEN + 1]; // +1 to allow for terminating \0
static u8 cli_insert = 0;

void cli_get(void) {
    u8 ch;  // input character
    u8 i;   // loop counter
    cli_insert = 0;

    // clear input buffer
    for(i=0; i<=CLI_MAX_LEN; ++i) {
        cli_buf[i] = '\0';
    }

    // show prompt
    ns16450_a_send(']');

    while(1) {
        ns16450_a_wait_rxd_full();
        ch = ns16450_a_recv();

        // Special character handling
        if(ch == ASCII_BS) {
            // backspace
            if(cli_insert > 0) {
                // if buffer isn't empty...
                cli_buf[cli_insert] = '\0';
                ns16450_a_send(ASCII_BS);
                ns16450_a_send(' ');
                ns16450_a_send(ASCII_BS);
                --cli_insert;
            } else {
                ns16450_a_send(ASCII_BEL);
                continue;
            }
        } else if(ch == ASCII_CR) {
            // return CLI in cli_buf
            ns16450_a_send(ASCII_CR);
            ns16450_a_send(ASCII_LF);
            return;
        } else if(ascii_is_printable(ch) && (cli_insert < CLI_MAX_LEN)) {
            ns16450_a_send(ch);
            cli_buf[cli_insert] = ch;
            ++cli_insert;
        } else {
            ns16450_a_send(ASCII_BEL);
        }
    }
}


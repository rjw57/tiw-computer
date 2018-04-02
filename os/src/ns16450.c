#include "ns16450.h"
#include "interrupt.h"
#include "types.h"

IRQ_ISR_BEGIN(ns16450_a)
IRQ_ISR_END(ns16450_a)

void ns16450_a_init(void)
{
    IRQ_REGISTER_ISR(ns16450_a);

    // Enable access to DRAB
    NS16450_A_WRITE_REG(NS16450_REG_LINE_CTRL, 0x80);

    // Set divisor for 9600 baud assuming 6.144MHz crystal = 40d = 28h
    NS16450_A_WRITE_REG(NS16450_REG_DIVISOR_LSB, 0x28);
    NS16450_A_WRITE_REG(NS16450_REG_DIVISOR_MSB, 0x00);

    // Set line mode, disabling DRAB access
    NS16450_A_WRITE_REG(NS16450_REG_LINE_CTRL, 0x03);
}

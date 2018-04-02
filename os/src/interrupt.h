#ifndef INTERRUPT_H__
#define INTERRUPT_H__

#define IRQ_DISABLE() do { __asm__("sei"); } while(0)
#define IRQ_ENABLE() do { __asm__("cli"); } while(0)

#define IRQ_ISR_BEGIN(tag) \
    static void (*irq_next_handler_##tag) (void); \
    void irq_handler_##tag(void) {

#define IRQ_ISR_RETURN(tag) \
    __asm__("jmp (%v)", irq_next_handler_##tag)

#define IRQ_ISR_END(tag) \
    IRQ_ISR_RETURN(tag); \
}

#define IRQ_REGISTER_ISR(tag) do { \
    irq_next_handler_##tag = first_isr; \
    first_isr = irq_handler_##tag; \
} while(0)

extern void (*first_isr) (void);

#endif // INTERRUPT_H__

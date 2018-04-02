; Hardware vector table.
.import reset

; A "NOP" handler which does nothing except return.
.proc nop_handler
        rti
.endproc

; IRQ handler. Jumps to the first interrupt service routine.
.proc isr_head
        pha                     ; save registers to stack
        txa
        pha
        tya
        pha

        cld                     ; ensure binary mode (in case it was changed)

        jmp (first_isr)
.endproc

; Tail of all ISRs
.export isr_tail
.proc isr_tail
        pla                     ; restore registers from stack
        tay
        pla
        tax
        pla

        rti
.endproc

.bss

; Address of isr service routine
first_isr:      .res 2
_first_isr=first_isr
.export first_isr, _first_isr

; 6502 vector table. This segment is placed at the very top of memory.

.segment "VECTORS"

.word nop_handler       ; (reserved)
.word nop_handler       ; (reserved)
.word nop_handler       ; COP
.word nop_handler       ; (reserved)
.word nop_handler       ; ABORT
.word nop_handler       ; NMI
.word reset             ; RESET
.word isr_head          ; IRQ/BRK

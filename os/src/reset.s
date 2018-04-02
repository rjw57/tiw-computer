; Reset handler.
.import first_isr, isr_tail
.import __DATA_LOAD__, __DATA_RUN__, __DATA_SIZE__
.importzp ptr1, ptr2, ptr3

; The reset handler. Initialises zero page to zero and transfers control to C
; init() function.
.export reset
.proc reset
        sei                     ; disable interrupts
        cld                     ; clear decimal mode flag

        ldx #$FF                ; initialise stack pointer to $01FF
        txs

        jsr copy_data           ; copy read-write initialised data to RAM

        ldx #0                  ; fill zero-page with zeros
        lda #0
@loop:  sta 0,X
        inx
        bne @loop

                                ; initialise C stack pointer to point to
                                ; top of stack
        .importzp sp
        .import __OSCSTACK_SIZE__, __OSCSTACK_START__
        lda #<(__OSCSTACK_START__ + __OSCSTACK_SIZE__)
        ldx #>(__OSCSTACK_START__ + __OSCSTACK_SIZE__)
        sta sp
        stx sp+1

        lda #<isr_tail          ; initialise first_isr to point to tail
        ldx #>isr_tail          ; of interrupt service routine
        sta first_isr
        stx first_isr+1

        .import _init           ; transfer control to C init() function
        jmp _init
.endproc

; Copy initialised data to RAM.
.export copy_data
.proc copy_data
        lda #<__DATA_LOAD__     ; ptr1 = __DATA_LOAD__
        ldx #>__DATA_LOAD__
        sta ptr1
        stx ptr1+1

        lda #<__DATA_RUN__      ; ptr2 = __DATA_RUN__
        ldx #>__DATA_RUN__
        sta ptr2
        stx ptr2+1

        lda #<__DATA_SIZE__     ; ptr3 = __DATA_SIZE__
        ldx #>__DATA_SIZE__     ; ptr3 stores number of bytes left to copy
        sta ptr3
        stx ptr3+1

@loop:
        lda ptr3                ; ptr3 == 0?
        bne @body
        lda ptr3
        bne @body
        rts                     ; yes, all done so return

@body:                          ; loop body

        ldx #0
        lda (ptr1,X)            ; *ptr2 = *ptr1
        sta (ptr2,X)

        lda #0                  ; ++ptr1
        clc
        inc ptr1
        adc ptr1 + 1

        lda #0                  ; ++ptr2
        clc
        inc ptr2
        adc ptr2 + 1

        lda #0                  ; --ptr3
        clc
        dec ptr3
        sbc ptr3 + 1

        clv
        bvc @loop               ; next iteration
.endproc



#define MFP_BASE 0xFFFFFA00


    .globl  _interrupt_counter
    .globl  _disable_interrupts
    .globl  _enable_interrupts
    .globl  _timer_b_interrupt_handler
    .extern _timer_b_sieve

    .bss
    .even
_interrupt_counter:
interrupt_counter:  .ds.l   1
cpu_status_save:    .ds.w   1

    .even
    .text

_disable_interrupts:
    move.w  sr,cpu_status_save
    move.w  #0x2700,sr
    rts

_enable_interrupts:
    move.w  cpu_status_save,sr
    rts

_timer_b_interrupt_handler:
    move.l	a0,-(sp)
    rol.w   _timer_b_sieve
    jpl     timerb_end
    addq.l  #1,interrupt_counter

    /* Call the vertical interrupt route through its vector. */
    move.l  (0x70),a0
    jsr     (a0)

timerb_end:
    /* Write the MFP ISR register to clear the interrupt */
    lea	    MFP_BASE,a0
    move.b  #0xFE,15(a0)
    move.l	(sp)+,a0
    rte

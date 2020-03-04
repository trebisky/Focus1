// *****************************************************************************/
// Tom Trebisky  12-17-2013
// *****************************************************************************/

.origin 0
.entrypoint start

#define CONST_PRUCFG	C4
#define CONST_PRUDRAM	C24

#define PRU0_ARM_INTERRUPT      19

#define GPIO0 0x44e07000
#define GPIO1 0x4804c000
#define GPIO2 0x481ac000
#define GPIO3 0x481ae000

// See page 4640-4666 in the reference manual
#define GPIO_DATAIN     0x138
#define GPIO_CLEARDATAOUT 0x190
#define GPIO_SETDATAOUT 0x194

// Here are the settings to blink the
// onboard usr0 LED
//#define GPIO    GPIO1
//#define PIN     21

// Here are the settings to send pulses
// to a motor via P8_11
#define GPIO    GPIO1
#define PIN     13

// watch for a limit status on this bit
#define LGPIO    GPIO1
#define LPIN     12

#define DELAY_COUNT 0x00f00000
// #define DELAY_COUNT 0x00800000

// Data RAM has commands and status
//  0 (00) = command
//  1 (04) = parameter
//  2 (08) = hi delay
//  3 (12) = lo delay
//  4 (16) = lower limit
//  5 (20) = upper limit
// ---
//  6 (24) = current position
//  7 (28) = status
//
// Command has low byte:
//  0 = idle
//  1 = continuous pulses
//  2 = count pulses as param
//  3 = match position as param
// Flags:
//  0x100 = reverse direction
//  0x200 = override hard limit

start:

    // If I don't do this, I don't get the
    //  interrupt when this finishes.
    // Enable OCP master port
    lbco r0, CONST_PRUCFG, 4, 4
    clr r0, r0, 4  // clear SYSCFG[STANDBY_INIT] to enable
    sbco r0, CONST_PRUCFG, 4, 4

    // get param from dataram
    lbco r1, CONST_PRUDRAM, 4, 4
    // get high delay from dataram
    lbco r5, CONST_PRUDRAM, 8, 4
    // get low delay from dataram
    lbco r7, CONST_PRUDRAM, 12, 4

    mov r2, 1<<PIN
    mov r3, GPIO | GPIO_SETDATAOUT
    mov r4, GPIO | GPIO_CLEARDATAOUT

    mov r8, LGPIO | GPIO_DATAIN

LLOOP:

    // get mode from dataram
    lbco r6, CONST_PRUDRAM, 0, 4
    and r6, r6, 0xff
    qbeq exit, r6, 0

    lbco r6, CONST_PRUDRAM, 0, 4
    qbbs nohwlim, r6, 9

// check for the hw limit before starting a pulse
    lbbo        r9, r8, 0, 4
    qbbs        exit, r9, LPIN

nohwlim:

    lbco r10, CONST_PRUDRAM, 0, 4        // mode
    qbbs        checklow, r10, 8

    // Check upper limit
    lbco r9, CONST_PRUDRAM, 24, 4       // position
    lbco r10, CONST_PRUDRAM, 20, 4      // ulim
    sub r9, r9, r10
    qbeq  exit, r9, 0
    jmp dopulse

checklow:
    // Check lower limit
    lbco r9, CONST_PRUDRAM, 24, 4       // position
    lbco r10, CONST_PRUDRAM, 16, 4      // llim
    sub r9, r9, r10
    qbeq  exit, r9, 0

dopulse:
    call pulse

    lbco r9, CONST_PRUDRAM, 24, 4       // position

    // Adjust position
    lbco r10, CONST_PRUDRAM, 0, 4        // mode
    qbbs        countdown, r10, 8
    add r9, r9, 1
    jmp  endcount
countdown:
    sub r9, r9, 1
endcount:
    sbco r9, CONST_PRUDRAM, 24, 4

    // get mode from dataram
    lbco r6, CONST_PRUDRAM, 0, 4
    and r6, r6, 0xff

    // maybe we are looping forever
    qbeq LLOOP, r6, 1

    // check total count
    sub r1, r1, 1
    qbne LLOOP, r1, 0

exit:
    // Send notification to Host for program completion
    mov       r31.b0, PRU0_ARM_INTERRUPT+16

    halt

// ----------------------------

pulse:
    sbbo r2, r3, 0, 4   // set bit high

// Commenting out the delay loop, I get pulses
// too narrow to measure with the 100 Mhz scope I
// have (the Tektronix 7603).  I see them, but their
// shape is distorted by probe and scope bandwidth

    // Delay keeping pin high
    mov r0, r5
DEL1:
    sub r0, r0, 1
    qbne DEL1, r0, 0

    sbbo r2, r4, 0, 4   // set bit low

    // Delay keeping pin low
    mov r0, r7
DEL2:
    sub r0, r0, 1
    qbne DEL2, r0, 0

    ret

// THE END

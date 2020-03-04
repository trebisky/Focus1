/******************************************************************************
 * Tom Trebisky 12-17-2013
 *
 * modeled after PRU_memAccess_DDR_PRUsharedRAM.c
 *
 ******************************************************************************/
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

// #include "prussdrv.h"
#include <prussdrv.h>
#include <pruss_intc_mapping.h>	 

unsigned int * pru_membase ( void );
void dump_dataram ( void );

void first_blink ( void );
void again ( void );
void blink_halt ( void );
void blink_wait ( void );

void motor_init ( int );

char *pru_binary_file = "./pru_eblink.bin";

unsigned int *dataram = NULL;

#ifdef notdef
#define FAST_DELAY	0x00800000
#define SLOW_DELAY	0x00f00000

// about 360 ms cycle (about 3 Hz)
//#define MOTOR_DELAY	0x01000000

// 21 ms cycle (47.6 Hz)
//#define MOTOR_DELAY	0x00100000

// 1.28 millisecond cycle (781 Hz)
// motor runs clean with full steps.
//#define MOTOR_DELAY	0x00010000

// too fast
//#define MOTOR_DELAY	0x0000c000
//#define MOTOR_DELAY	0x00008000
// #define MOTOR_DELAY	0x00004000

#define MOTOR_DELAY	20000

// should give 20 microsecond pulses
#define PULSE_DELAY	2000

int bh_delay = PULSE_DELAY;
int bl_delay = MOTOR_DELAY;
#endif

#define PRU_NUM 	 0

/* I can run an unloaded motor at 800 Hz,
   but not 1000 Hz
 */
#define MOTOR_HZ	400
// #define MOTOR_HZ	1000000

#define BL_HALT		0
#define BL_RUN		1
#define BL_COUNT	2

int blink_mode = BL_COUNT ;
int blink_count = 10;

int bh_delay;
int bl_delay;

int blink_repeat = 0;

/* Usage:
   pru_blink will blink LED 2x
   pru_blink b10 will blink LED 10x (and so forth)
   pru_blink r10 will blink led 5x, repeating this 10 times.
   pru_blink f will blink forever
   pru_blink h will halt an ongoing blink
   */

int
main (int argc, char **argv )
{
    unsigned int ret;
    int cmd = 'b';
    int val = 2;
    char *p;

    --argc;
    ++argv;
    if ( argc > 0 ) {
	blink_count = atoi ( *argv );
    }

    if ( argc > 0 ) {
	p = *argv;
	if ( *p && *p == '-' ) p++;
	if ( *p ) cmd = *p ++;
	if ( *p ) val = atoi ( p );
    }

    if ( val < 1 ) val = 2;
    if ( cmd == 'b' ) {
	blink_mode = BL_COUNT;
	blink_count = val;
    }

    if ( cmd == 'r' )
	blink_repeat = val;

    if ( cmd == 'f' )
	blink_mode = BL_RUN;

    if ( cmd == 'h' )
	blink_mode = BL_HALT;

    motor_init ( MOTOR_HZ );

    /* Initialize the PRU */
    ret = prussdrv_initialize ();		
    if (ret) {
        printf("prussdrv failed to initialize\n");
        printf(" maybe you need to run this as root\n");
        return (ret);
    }

    dataram = pru_membase ();
    if ( ! dataram ) {
	printf ( "Cannot map PRU dataram base address\n" );
	return -1;
    }

    dump_dataram ();

    if ( cmd == 't' ) {
	// prussdrv_pru_reset ( PRU_NUM );
	prussdrv_pru_enable ( PRU_NUM );
	exit (0);
    }

    if ( blink_mode != BL_HALT )
	prussdrv_load_program (PRU_NUM, pru_binary_file );

    if ( blink_repeat ) {
	first_blink ();
	--blink_repeat;
	while ( blink_repeat-- ) {
	    blink_wait ();
	    sleep ( 2 );
	    again ();
	}
	blink_wait ();
	return 0;
    }

    if ( blink_mode == BL_COUNT ) {
	first_blink ();
	blink_wait ();
    }

    if ( blink_mode == BL_RUN ) {
	first_blink ();
	/* leave the PRU running */
	prussdrv_exit ();
	return 0;
    }

    if ( blink_mode == BL_HALT ) {
	blink_halt ();
	blink_wait ();
    }

    /* Cleanup */
    prussdrv_pru_disable(PRU_NUM); 
    prussdrv_exit ();

    return 0;
}

/* The delay loops use 2 PRU instructions,
   so each count of the loop is 10 nanoseconds.
 */
void motor_init ( int hz )
{
    double nanos = 1000000000.0;
    int ldelay;

    nanos /= hz;	// nanos per tick
    nanos /= 10;	// loops per tick

    // loops per 20 microsecond pulse
    bh_delay = 25 * 1000 / 10;

    printf ( "Hi delay: %d\n", bh_delay );

    ldelay = nanos;
    printf ( "Nanos for %d: %d\n", hz, ldelay );

    if ( ldelay > bh_delay )
	bl_delay = ldelay - bh_delay;
    else
	bl_delay = ldelay;
    //printf ( "Lo delay: %d\n", bl_delay );
}

void again ( void )
{
    prussdrv_pru_reset ( PRU_NUM );
    prussdrv_pru_enable ( PRU_NUM );

    prussdrv_pru_wait_event (PRU_EVTOUT_0);
    prussdrv_pru_clear_event (PRU0_ARM_INTERRUPT,PRU_EVTOUT_0);
}

unsigned int *
pru_membase ( void )
{
    static void *vp = NULL;

    if ( ! vp ) {
	prussdrv_map_prumem ( PRUSS0_PRU0_DATARAM, &vp );
	/*
	printf ( "Data RAM pointer: %08x\n", (unsigned int)vp );
	*/
    }

    if ( ! vp )
	return NULL;

    return (unsigned int *) vp;
}

void dump_dataram ( void )
{
    printf ( "dataram[0] = %d\n", dataram[0] );
    printf ( "dataram[1] = %d\n", dataram[1] );
    printf ( "dataram[2] = %d\n", dataram[2] );
    printf ( "dataram[3] = %d\n", dataram[3] );
}

void pru_command ( int mode, int count, int hi_delay, int low_delay )
{
    dataram[0] = mode;
    dataram[1] = count;
    dataram[2] = hi_delay;
    dataram[3] = low_delay;
}

void
blink_halt ( void )
{
    pru_command ( BL_HALT, 0, 0, 0 );
}

void
blink_wait ( void )
{
    prussdrv_pru_wait_event (PRU_EVTOUT_0);
    prussdrv_pru_clear_event (PRU0_ARM_INTERRUPT, PRU_EVTOUT_0);
}

void
first_blink ( void )
{
    pru_command ( blink_mode, blink_count, bh_delay, bl_delay );
    // prussdrv_exec_program (PRU_NUM, pru_binary_file );
    prussdrv_pru_enable ( PRU_NUM );
}

/* THE END */

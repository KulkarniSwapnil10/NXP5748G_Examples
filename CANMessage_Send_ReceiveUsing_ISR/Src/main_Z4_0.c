/*****************************************************************************/
/* FILE NAME: main_z4_0.c             COPYRIGHT (c) NXP Semiconductors 2016  */
/*                                                      All Rights Reserved  */
/* PLATFORM: DEVKIT-MPC5748G												 */
/* DESCRIPTION: Main C program for core 0 (e200z4a) to call FlexCAN functions*/
/*				Continuously Listens on the CAN0 port for data.				 */
/*				Can be used with transmit project on another board			 */
/*                                                                           */
/*****************************************************************************/
/* REV      AUTHOR        DATE        DESCRIPTION OF CHANGE                  */
/* ---   -----------    ----------    ---------------------                  */
/* 1.0	 S Mihalik      19 Feb 2014   Initial Version                        */
/* 1.1   S Mihalik      12 Feb 2015   Removed unrequired SIUL ME_PCTL code 	 */
/* 1.1   K Shah			01 Mar 2016	  Ported to S23DS						 */
/*****************************************************************************/

#include "derivative.h" /* include peripheral declarations */
#include "can.h"
#include "mode.h"
#include "MPC5748G.h"
#include "pit.h"
#include "gpio.h"
#include "mode.h"

#define KEY_VALUE1 0x5AF0ul
#define KEY_VALUE2 0xA50Ful
#define LED_DS4 SIUL2.GPDO[PA10].B.PDO_4n
#define LED_DS5 SIUL2.GPDO[PA7].B.PDO_4n

extern void xcptn_xmpl(void);
void peri_clock_gating (void); /* Configure gating/enabling peripheral(CAN) clocks */

void peri_clock_gating (void) {
  MC_ME.RUN_PC[0].R = 0x00000000;  /* Gate off clock for all RUN modes */
  MC_ME.RUN_PC[1].R = 0x000000FE;  /* Configures peripheral clock for all RUN modes */

  MC_ME.PCTL[70].B.RUN_CFG = 0x1;   /* FlexCAN 0: select peri. cfg. RUN_PC[1] */
  MC_ME.PCTL[91].B.RUN_CFG = 0x1;  /* PIT: select peripheral configuration RUN_PC[1] */
}

void PIT0_ISR(void) {
	static uint8_t counter=0;	/* Increment ISR counter */

	counter++;
	Transmit_Fixed_Msg();
	LED_DS4 = ~LED_DS4;         /* Toggle DS4 LED port */
    if(counter == 4)
    {
    	counter = 0;
    	INTC_SSCIR1=0x02;
    }
    PIT.TIMER[0].TFLG.R = 1;  	/* Clear interrupt flag */
}

void hw_init(void)
{
#if defined(DEBUG_SECONDARY_CORES)
	uint32_t mctl = MC_ME.MCTL.R;
#if defined(TURN_ON_CPU1)
	/* enable core 1 in all modes */
	MC_ME.CCTL[2].R = 0x00FE;
	/* Set Start address for core 1: Will reset and start */
	MC_ME.CADDR[2].R = 0x11d0000 | 0x1;
#endif	
#if defined(TURN_ON_CPU2)
	/* enable core 2 in all modes */
	MC_ME.CCTL[3].R = 0x00FE;
	/* Set Start address for core 2: Will reset and start */
	MC_ME.CADDR[3].R = 0x13a0000 | 0x1;
#endif
	MC_ME.MCTL.R = (mctl & 0xffff0000ul) | KEY_VALUE1;
	MC_ME.MCTL.R =  mctl; /* key value 2 always from MCTL */
#endif /* defined(DEBUG_SECONDARY_CORES) */
}

__attribute__ ((section(".text")))

/************************************ My Main Logic ***********************************/
int main(void)
{
	xcptn_xmpl ();           /* Configure and Enable Interrupts */

    //Since We are using PIT- one of the peripherals. We need to enable peripheral clocks.
    peri_clock_gating();     /* Configure gating/enabling peripheral(PTI) clocks for modes*/
                             /* Configuration occurs after mode transition! */

    system160mhz();
    /* Sets clock dividers= max freq,
       calls PLL_160MHz function which:
       MC_ME.ME: enables all modes for Mode Entry module
       Connects XOSC to PLL
       PLLDIG: LOLIE=1, PLLCAL3=0x09C3_C000, no sigma delta, 160MHz
       MC_ME.DRUN_MC: configures sysclk = PLL
       Mode transition: re-enters DRUN which activates PLL=sysclk & peri clks
       */
    initCAN_0();
    initGPIO();         /* Initializes LED, buttons & other general purpose pins for NXP EVB */

    PIT.MCR.B.MDIS = 0; /* Enable PIT module. NOTE: PIT module must be       */
                        /* enabled BEFORE writing to it's registers.         */
                        /* Other cores will write to PIT registers so the    */
                        /* PIT is enabled here before starting other cores.  */
    PIT.MCR.B.FRZ = 1;  /* Freeze PIT timers in debug mode */

    PIT0_init(2000000); /* Initialize PIT channel 0 for desired SYSCLK counts*/
             	 	 	 /* timeout= 40M  PITclks x 4 sysclks/1 PITclk x 1 sec/160Msysck */
    					 /*        = 40M x 4 / 160M = 160/160 = 1 sec.  */
    for (;;)
    {
     CAN_Receive_And_ReTransmit();
     LED_DS5 = ~LED_DS5;         /* Toggle DS5 LED port */
    }

	return 0;
}
/******************************End of Main ***********************************/


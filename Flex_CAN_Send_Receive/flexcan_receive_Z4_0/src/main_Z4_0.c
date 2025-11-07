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

#define KEY_VALUE1 0x5AF0ul
#define KEY_VALUE2 0xA50Ful
#define LED_PIN 7U

extern void xcptn_xmpl(void);
void peri_clock_gating (void); /* Configure gating/enabling peripheral(CAN) clocks */

void LED_Init(void);
void LED_Toggle(void);

/* -------------------------------------------------------------------------- */
/* Initialize LED Pin                                                        */
/* -------------------------------------------------------------------------- */
void LED_Init(void)
{
    /* Configure PA7 as output */
    SIUL2.MSCR[PA7].B.OBE = 1;
    /* Set initial state (OFF) */
    SIUL2.GPDO[LED_PIN].R = 0;
}

/* -------------------------------------------------------------------------- */
/* Toggle LED                                                                */
/* -------------------------------------------------------------------------- */
void LED_Toggle(void)
{
    SIUL2.GPDO[LED_PIN].R ^= 1;
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

/************************************ Main ***********************************/

//int main(void)
//{
// 	uint32_t CAN_msg_count = 0;
//	xcptn_xmpl ();         /* Configure and Enable Interrupts */
//
//	peri_clock_gating();   /* Configure gating/enabling peripheral clocks(CANs) for modes*/
//	                       /* Configuration occurs after mode transition */
//
//	system160mhz();        /* sysclk=160MHz, dividers configured, mode trans*/
//
//	initCAN_0_rx();           /* Initialize FlexCAN 0 & one of its buffers for receive*/
//
//	while (1) {
//
//	  ReceiveMsg();           /* Transmit one message from a FlexCAN 0 buffer */
//	  CAN_msg_count++;         /* Increment CAN message counter */
//	  SIUL2.MSCR[PA7].B.OBE = 1;  /* Pad PA7 (7): OBE=1. On EVB enable low DS5 LED */
//	}
//
//	return 0;
//}

/************************************ My Main Logic ***********************************/
int main(void)
{
    xcptn_xmpl();

    peri_clock_gating();
    system160mhz();
    initCAN_0();  // Unified init for RX + TX
    LED_Init();   /* Initialize LED */

    for(;;)
    {


        CAN_Receive_And_ReTransmit(); // Wait → Modify → Send back

        /* Toggle LED after each receive+retransmit */
        LED_Toggle();
    }
}
/******************************End of Main ***********************************/

void peri_clock_gating (void) {		/* Configure gating/enabling peripheral(CAN) clocks */
  MC_ME.RUN_PC[0].R = 0x00000000;  	/* gate off clock for all RUN modes */
  MC_ME.RUN_PC[1].R = 0x000000FE;  	/* config. peri clock for all RUN modes */

  MC_ME.PCTL[70].B.RUN_CFG = 0x1;   /* FlexCAN 0: select peri. cfg. RUN_PC[1] */
}


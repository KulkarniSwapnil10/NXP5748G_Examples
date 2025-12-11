/*****************************************************************************/
	/* FILE NAME: can.c                   COPYRIGHT (c) NXP Semiconductors 2016  */
/*                                                      All Rights Reserved  */
/* DESCRIPTION: CAN Driver Functions							             */
/*                                                                           */
/*****************************************************************************/
/* REV      AUTHOR               DATE        DESCRIPTION OF CHANGE           */
/* ---   -----------          ----------    -----------------------          */
/* 1.0	 S Mihalik/H Osornio  07 Mar 2014   Initial Version                  */
/* 1.1	 S Mihalik            09 Mar 2015   Change from MPC5748G: 96 buffers.*/
/* 1.2	 K Shah				  01 Mar 2016	Ported to S32DS					 */
/*****************************************************************************/

#include "can.h"

uint32_t  RxCODE;              	/* Received message buffer code */
uint32_t  RxID;                	/* Received message ID */
uint32_t  RxLENGTH;            	/* Received message number of data bytes */
uint8_t   RxDATA[64];           	/* Received message data string*/
uint32_t  RxTIMESTAMP;         	/* Received message time */

/* ---- NEW VARIABLES ---- */
#define FIXED_TX_ID  0x600       /* ID for periodic message */
#define FIXED_PERIOD_MS 50        /* Transmit period (50 ms) */
#define MAX_RX_FRAMES 4  // optional, can handle multiple frames

/* ---- Initialization CAN Function ---- */
void initCAN_0(void)
{
    uint8_t i;

    /* ---------- FlexCAN Module Initialization ---------- */
    CAN_0.MCR.B.MDIS = 1;          /* Disable module before selecting clock source */
    CAN_0.CTRL1.B.CLKSRC = 0;      /* Clock Source = oscillator clock (40 MHz) */
    CAN_0.MCR.B.MDIS = 0;          /* Enable module for configuration */
    while (!CAN_0.MCR.B.FRZACK) {} /* Wait for freeze acknowledge to set */

    CAN_0.CTRL1.R = 0x04DB0086;    /* 500kbps, 40 MHz oscillator clock */

    /* ---------- Clear all message buffers ---------- */
    for (i = 0; i < 96; i++) {
        CAN_0.MB[i].CS.B.CODE = 0; /* In-activate all message buffers */
    }

    /* ---------- Configure RX mailbox (MB4) ---------- */
    CAN_0.MB[4].CS.B.IDE = 0;            /* Standard ID */
    CAN_0.MB[4].ID.B.ID_STD = 0x000;     /* Expected ID from logger */
    CAN_0.MB[4].CS.B.CODE = 4;           /* RX EMPTY (ready to receive) */
//    CAN_0.RXMGMASK.R = 0x1FFFFFFF;       /* Accept all bits for filtering */
    CAN_0.RXMGMASK.R = 0x00000000;       /* Accept all bits for filtering */

    /* ---------- Disable self reception ---------- */
    CAN_0.MCR.B.SRXDIS = 1;            /* Disable self-reception (no TX echo) */

    /* Configure MB0 as TX mailbox */
    CAN_0.MB[0].CS.B.CODE = 8;       /* TX INACTIVE */
    CAN_0.MB[0].CS.B.IDE  = 0;       /* Standard ID */

    /* ---- FIXED TX MB1 ---- */
    CAN_0.MB[5].CS.B.CODE = 8;        /* TX inactive */
    CAN_0.MB[5].CS.B.IDE = 0;
    CAN_0.MB[5].ID.B.ID_STD = FIXED_TX_ID;

    /* ---------- Pin multiplexing for CAN0 ---------- */
    SIUL2.MSCR[16].B.SSS = 1;  /* PB0 → CAN0_TX */
    SIUL2.MSCR[16].B.OBE = 1;  /* Output buffer enable */
    SIUL2.MSCR[16].B.SRC = 3;  /* Max slew rate */
    SIUL2.MSCR[17].B.IBE = 1;  /* PB1 → CAN0_RX */
    SIUL2.IMCR[188].B.SSS = 2; /* Link CAN0_RX to PB1 */

    /* ---------- Exit freeze mode (start CAN) ---------- */
    //CAN_0.MCR.R = 0x0000003F;   /* Enable CAN, 64 MBs */
    CAN_0.MCR.B.HALT = 0;
    while (CAN_0.MCR.B.FRZACK | CAN_0.MCR.B.NOTRDY) {}
}

/* ------------------------------------------------------------- */
/* Wait for standard CAN frame, modify ID, transmit back                      */
/* ------------------------------------------------------------- */
void CAN_Receive_And_ReTransmit(void)
{
    uint8_t j;
    uint32_t dummy;

//    /* ---- Wait for MB4 reception flag ---- */
//    while (CAN_0.IFLAG1.B.BUF4TO1I != 8) {}

    /* ---- Wait for MB4 reception flag ---- */
    while ((CAN_0.IFLAG1.R & (1<<4)) == 0) {}


    /* ---- Read received frame ---- */
    RxCODE      = CAN_0.MB[4].CS.B.CODE;
    RxID        = CAN_0.MB[4].ID.B.ID_STD;
    RxLENGTH    = CAN_0.MB[4].CS.B.DLC;
    RxTIMESTAMP = CAN_0.MB[4].CS.B.TIMESTAMP;

    for (j = 0; j < RxLENGTH; j++) {
        RxDATA[j] = CAN_0.MB[4].DATA.B[j];
    }

//    /* ---- Unlock MB and clear flag ---- */
//    dummy = CAN_0.TIMER.R;
//    CAN_0.IFLAG1.R = 0x00000010; /* Clear MB4 flag */

    /* ---- Unlock MB and clear only MB4 flag ---- */
    dummy = CAN_0.TIMER.R;
    CAN_0.IFLAG1.R = (1<<4);

    static uint32_t last_tx_id = 0xFFFFFFFF;
    if (RxID == last_tx_id) {
        return; /* Skip retransmitting self */
    }

    /* ---- Calculate new ID ---- */
    uint32_t new_id;
    if (RxID >= 0x7FF) {
        new_id = 0x000;        /* Wrap around for 0x7FF */
    } else {
        new_id = RxID + 1;     /* Increment ID by 1 */
    }

    /* ---- Setup TX MB (MB0) for retransmission ---- */
    CAN_0.MB[0].CS.B.CODE = 8;       /* TX INACTIVE */
    CAN_0.MB[0].CS.B.IDE = 0;          /* Standard ID */
    CAN_0.MB[0].ID.B.ID_STD = new_id;  /* Updated ID */
    CAN_0.MB[0].CS.B.RTR = 0;          /* Data frame */
    CAN_0.MB[0].CS.B.DLC = RxLENGTH;   /* Same length */

    for (j = 0; j < RxLENGTH; j++) {
        CAN_0.MB[0].DATA.B[j] = RxDATA[j]; /* Copy same payload */
    }

    CAN_0.MB[0].CS.B.SRR = 1;          /* Send as data frame */
    CAN_0.MB[0].CS.B.CODE = 0xC;       /* Start transmission */

    /* ---- Correct TX wait (wait until TX complete) ---- */
    while (CAN_0.MB[0].CS.B.CODE != 0x8) {}
}

/* ----------- Periodic Transmit (corrected) ---------- */
void Transmit_Fixed_Msg(void)
{
	uint8_t i;
    const uint8_t TxData[8] = {
        0x00, 0x06, 0x62, 0xA5, 0x01, 0xFF, 0xFF, 0xFF
    };

    CAN_0.MB[5].CS.B.IDE = 0;
    CAN_0.MB[5].ID.B.ID_STD = FIXED_TX_ID;
    CAN_0.MB[5].CS.B.RTR = 0;
    CAN_0.MB[5].CS.B.DLC = 8;

    for (i = 0; i < 8; i++)
        CAN_0.MB[5].DATA.B[i] = TxData[i];

    CAN_0.MB[5].CS.B.SRR = 1;
    CAN_0.MB[5].CS.B.CODE = 0xC;

    while (CAN_0.MB[5].CS.B.CODE != 0x8) {}
}


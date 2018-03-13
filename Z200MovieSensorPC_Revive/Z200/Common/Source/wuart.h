/****************************************************************************/
/*!
 *\MODULE              Wireless UART (Application Layer)
 *
 *\COMPONENT           $HeadURL: https://www.collabnet.nxp.com/svn/lprf_apps/Application_Notes/JN-AN-1069-IEEE-802.15.4-Serial-Cable-Replacement/Tags/Release_3v0-Public/Common/Source/wuart.h $
 *
 *\VERSION			   $Revision: 16555 $
 *
 *\REVISION            $Id: wuart.h 16555 2015-11-20 12:14:37Z nxa04494 $
 *
 *\DATED               $Date: 2015-11-20 12:14:37 +0000 (Fri, 20 Nov 2015) $
 *
 *\AUTHOR              $Author: nxa04494 $
 *
 *\DESCRIPTION         Wireless UART (Application Layer) - public interface.
 *
 * This module implements the wireless UART functionality.
 *
 * It transfers data between the UART and the radio system implementing the
 * Wireless UART.
 *
 * It also maintains the LCD display providing statistical feedback on the
 * wireless UART's operation.
 */
/*****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd 2009. All rights reserved
 *
 ****************************************************************************/

#ifndef WUART_H_INCLUDED
#define WUART_H_INCLUDED

/* Jennic includes */
#include "jendefs.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/** Debug trace defines */
#define WUART_TRACE 	TRUE					/**< Wuart */

/** Functionality */
#define WUART_STATS		TRUE					/**< Maintain statistics on performance */
#define WUART_LCD		FALSE					/**< LCD display of statistics */

/** WUART configuration */
#define WUART_UART 		E_AHI_UART_1			/**< Uart to use */
#define WUART_BAUD		115200					/**< Baud rate */
#define WUART_EVEN		FALSE					/**< Even parity */
#define WUART_PARITY	FALSE					/**< Enable parity */
#define WUART_WORDLEN	E_AHI_UART_WORD_LEN_8	/**< Word length */
#define WUART_ONESTOP	TRUE					/**< One stop bit */
#define WUART_RTSCTS	FALSE					/**< RTS/CTS flow control */
#define WUART_XONXOFF	FALSE					/**< XON/XOFF flow control */

/** WUART states */
#define WUART_STATE_NONE	 	   0			/**< Not in network */
#define WUART_STATE_IDLE	 	   1			/**< Idling */
#define WUART_STATE_DIAL	 	   2			/**< Dialling */
#define WUART_STATE_DCONNECT 	   3			/**< Dialled connecting */
#define WUART_STATE_ANSWER	 	   4			/**< Answering */
#define WUART_STATE_ACONNECT 	   5			/**< Answered connecting */
#define WUART_STATE_ONLINE	 	   6			/**< Online */
#define WUART_STATE_ACK	 	 	   7			/**< Ack expected */
#define WUART_STATES		 	   8			/**< Number of states */
#define WUART_STATES_PAIRED  	0xc0			/**< Bitmask of paired states */

/** WUART status flags */
#define WUART_STATUS_TX_ENABLED	0x01			/**< Radio tx is enabled */
#define WUART_STATUS_RX_ENABLED	0x02			/**< Radio rx is enabled */
#define WUART_STATUS_RX_CHANGE	0x04			/**< Radio rx has changed */
#define WUART_STATUS_PING		0x08			/**< Pinging paired node */

/** WUART acknowledge flags */
#define WUART_ACK_DATA_ACK		0x01			/**< Data ack */
#define WUART_ACK_DATA_NAK		0x02			/**< Data nak */
#define WUART_ACK_RX_CHANGE		0x04			/**< Radio rx change ack */
#define WUART_ACK_PING			0x08			/**< Pinging paired node ack */

/* Queue indicies */
#define WUART_QUEUE_TX 	  		   0			/**< Transmit queue */
#define WUART_QUEUE_RX 	  		   1			/**< Receive queue */
#define WUART_QUEUES   	  		   2			/**< Number of queues */

/* Queue data size (use a power of two) */
#define WUART_QUEUE_DATA		8192					/**< Size of queue */
#define WUART_QUEUE_DATA_LOW	 256 /*(WUART_QUEUE_DATA >> 2)*/ /**< Low level of free space */
#define WUART_QUEUE_DATA_HIGH	1024 /*(WUART_QUEUE_DATA >> 1)*/ /**< Hign level of free space */

/** Packet Format */
#define MAC_POS		0
#define TARGET_POS	2
#define SRC_POS		4
#define SEQ_POS		7
//#define RADIUS_POS	7

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/** Structure for Wuart */
typedef struct
{
	uint64 u64Online;			/**< Address of node online with */
	uint32 u32StateStart;	 	/**< State timer on entering state */
	uint32 u32StateDuration; 	/**< State duration before timing out */
#if WUART_STATS
	uint32 u32Txed;				/**< Transmitted characters over radio */
	uint32 u32Rxed;				/**< Received characters over radio */
	uint32 u32TxPackets;		/**< Transmitted data packets over radio */
	uint32 u32RxPackets;		/**< Received data packets over radio */
#endif
	uint16  u16Paired;		 	/**< Paired node */
	uint8   u8State;		 	/**< State */
	uint8   u8Retry;		 	/**< Retry */
	uint8   u8Uart;				/**< Uart being used */
	uint8   u8Status;			/**< Current status */
	uint8	u8Ack;				/**< Current acks */
	uint8	u8MsgSeqTx;			/**< Transmit message sequence */
	uint8	u8MsgSeqRx;			/**< Receive message sequence from paired node */
	uint8	u8DataSeqTx;		/**< Transmit data sequence */
	uint8	u8DataSeqRx;		/**< Receive data sequence from paired node */
} 	WUART_tsData;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void   WUART_vInit(void);
PUBLIC void   WUART_vTick(void);
PUBLIC void   WUART_vRxQ(uint64, uint16, uint8 *);
PUBLIC void   WUART_vTxAck(uint64, uint16, uint8 *);  //m by Hodge
PUBLIC void   WUART_vTx(uint64, uint16, uint8 *);
PUBLIC void   WUART_vState(uint8);
PUBLIC bool_t WUART_bRxEnabled (void);
PUBLIC bool_t WUART_bTxEnabled (void);
void initRFPacket(void);


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern PUBLIC WUART_tsData	WUART_sData;		/**< Wuart information */
extern PUBLIC uint8			gWaitUARTPacket;  //Don't wait too long if there is no further character from UART
extern uint8 gWaitDECount; //wait some time before disable DE
extern uint8 gTurn485ReceiveMode;
extern bool		gDoApplicationResend; //control the application resending timing
extern uint8	gApplicationResend; //control the application resend times
extern uint8 gRxdelay_count;
#endif /* WUART_H_INCLUDED */

typedef struct
{
	uint8 Data[45];
	uint8 Data_Head;
	uint8 Data_Tail;
	bool OutFlag;
} HRsensor_sData;


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

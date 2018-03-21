/****************************************************************************/
/*!
 *\MODULE              UART (Application Layer)
 *
 *\COMPONENT           $HeadURL: https://www.collabnet.nxp.com/svn/lprf_apps/Application_Notes/JN-AN-1069-IEEE-802.15.4-Serial-Cable-Replacement/Tags/Release_3v0-Public/Common/Source/uart.h $
 *
 *\VERSION			   $Revision: 16555 $
 *
 *\REVISION            $Id: uart.h 16555 2015-11-20 12:14:37Z nxa04494 $
 *
 *\DATED               $Date: 2015-11-20 12:14:37 +0000 (Fri, 20 Nov 2015) $
 *
 *\AUTHOR              $Author: nxa04494 $
 *
 *\DESCRIPTION         UART (Application Layer) - public interface.
 *
 * This module provides interrupt driven UART routines, using circular queues.
 */
/****************************************************************************
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

#ifndef  UART_H_INCLUDED
#define  UART_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
/* Jennic includes */
#include <jendefs.h>

/* Application includes */
#include "queue.h"
#include "config.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/* Debug trace defines */
#define UART_TRACE 				FALSE	/**< Uart */

/* Compile options */
#define UART_STATS	    		TRUE	/**< Compile uart statistics */
#define UART_PRINTF  			TRUE	/**< Compile printf support */
#define UART_JENOS        		FALSE	/**< Compile for JenOs */

/* Disabled / Enabled flags */
#define UART_ABLE_UARTIO  		0x01	/**< Disabled by UartIO module */
#define UART_ABLE_WUART_PAIR	0x02	/**< Disabled by Wuart module not paired */

/* Standard characters */
#define CHAR_XON				0x11
#define CHAR_XOFF				0x13
#define CHAR_LF					'\n'
#define CHAR_CR					'\r'

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

#ifdef CTSRTS
PUBLIC uint8 gRTS; //RTS active low
#endif


/****************************************************************************/
/***        Exported Function Prototypes                                  ***/
/****************************************************************************/
PUBLIC 	bool_t 		UART_bOpen (		/**< Open UART */
		uint8 	        u8Uart,			/**< Uart to open */
		uint32    	   u32BaudRate,		/**< Baud rate */
		bool_t   		 bEvenParity,	/**< Even parity */
		bool_t   		 bEnableParity,	/**< Enable parity */
		uint8      		u8WordLength,	/**< Word length, one of:\n
										 	 E_AHI_UART_WORD_LEN_5\n
										 	 E_AHI_UART_WORD_LEN_6\n
										 	 E_AHI_UART_WORD_LEN_7\n
										 	 E_AHI_UART_WORD_LEN_8 */
		bool_t   		 bOneStopBit,	/**< One stop bit */
		bool_t   		 bRtsCts,		/**< RTS/CTS flow control */
		bool_t   		 bXonXoff,		/**< XON/XOFF flow control */
		QUEUE_tsData   *psTxQueue,		/**< Transmit queue pointer */
		QUEUE_tsData   *psRxQueue);		/**< Receive queue pointer */

PUBLIC 	bool_t 		UART_bGetChar (		/**< Get character received by UART */
		uint8  	   		u8Uart,			/**< Uart to get character from */
		char  	  	   *pcChar);		/**< Character received pointer */

PUBLIC bool_t 		UART_bPutChar (		/**< Put character for transmission by UART */
		uint8  	   		u8Uart,			/**< Uart to get character from */
		char  	    	 cChar);		/**< Character to transmit */

PUBLIC bool_t 		UART_bWaitEmptyTx 	(uint8 u8Uart);
PUBLIC bool_t 		UART_bOpened 		(uint8 u8Uart);
PUBLIC bool_t 		UART_bRtsCts 		(uint8 u8Uart);
PUBLIC bool_t 		UART_bXonXoff 		(uint8 u8Uart);
PUBLIC bool_t 		UART_bRts 			(uint8 u8Uart);
PUBLIC bool_t 		UART_bCts 			(uint8 u8Uart);
PUBLIC bool_t 		UART_bRxEnable		(uint8 u8Uart, uint8 u8Enable);
PUBLIC bool_t 		UART_bRxDisable		(uint8 u8Uart, uint8 u8Disable);
PUBLIC bool_t 		UART_bRxEnabled 	(uint8 u8Uart);
PUBLIC bool_t 		UART_bTxEnable		(uint8 u8Uart, uint8 u8Enable);
PUBLIC bool_t 		UART_bTxDisable		(uint8 u8Uart, uint8 u8Disable);
PUBLIC bool_t 		UART_bTxEnabled		(uint8 u8Uart);
PUBLIC void   		UART_vStartTx		(uint8 u8Uart);

#if UART_PRINTF
PUBLIC 	bool_t 		UART_bPrintf (		/**< Format and print to UART */
		uint8 			u8Uart,			/**< Uart to output to */
		const char     *pcFormat,		/**< Formatting string */
		...);
PUBLIC 	bool_t UART_bPrintf_ap (
		uint8 			u8Uart,		/**< Uart to output to */
		const char     *pcFormat,	/**< Formatting string */
		va_list         ap);
#endif

#if UART_STATS
PUBLIC uint32 		UART_u32Txed 		(uint8 u8Uart);
PUBLIC uint32 		UART_u32Rxed 		(uint8 u8Uart);
PUBLIC uint32 		UART_u32TxDisabled 	(uint8 u8Uart);
PUBLIC uint32 		UART_u32RxDisabled 	(uint8 u8Uart);
#endif


void sendAChar(char achar); //send a char to air

#if defined __cplusplus
}
#endif

#endif  /* UART_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/



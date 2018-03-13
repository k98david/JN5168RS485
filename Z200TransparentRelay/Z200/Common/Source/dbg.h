/*****************************************************************************/
/*!
 *\MODULE              Debug (Application Layer)
 *
 *\COMPONENT           $HeadURL: https://www.collabnet.nxp.com/svn/lprf_apps/Application_Notes/JN-AN-1069-IEEE-802.15.4-Serial-Cable-Replacement/Tags/Release_3v0-Public/Common/Source/dbg.h $
 *
 *\VERSION			   $Revision: 16555 $
 *
 *\REVISION            $Id: dbg.h 16555 2015-11-20 12:14:37Z nxa04494 $
 *
 *\DATED               $Date: 2015-11-20 12:14:37 +0000 (Fri, 20 Nov 2015) $
 *
 *\AUTHOR              $Author: nxa04494 $
 *
 *\DESCRIPTION         Debug (Application Layer) - public interface.
 *
 * This module provides functionality common to all nodes.
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

#ifndef DBG_H_INCLUDED
#define DBG_H_INCLUDED

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
/* Jennic includes */
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppHardwareApi_JN516x.h>
#include <mac_pib.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/** WUART configuration */
#define DBG_UART 		E_AHI_UART_0	/**< Uart to use */
//#define DBG_UART 		0xFF  //E_AHI_UART_1	/**< Uart to use */
#define DBG_BAUD		115200					/**< Baud rate */
#define DBG_EVEN		FALSE					/**< Even parity */
#define DBG_PARITY		FALSE					/**< Enable parity */
#define DBG_WORDLEN		E_AHI_UART_WORD_LEN_8	/**< Word length */
#define DBG_ONESTOP		TRUE					/**< One stop bit */
#define DBG_RTSCTS		FALSE					/**< RTS/CTS flow control */
#define DBG_XONXOFF		FALSE					/**< XON/XOFF flow control */

/* Queue indicies */
#define DBG_QUEUE_TX 	  		   0			/**< Transmit queue */
#define DBG_QUEUE_RX 	  		   1			/**< Receive queue */
#define DBG_QUEUES   	  		   2			/**< Number of queues */

/* Queue data size (use a power of two) */
#define DBG_QUEUE_DATA		128 				  /**< Size of queue */
#define DBG_QUEUE_DATA_LOW	(DBG_QUEUE_DATA >> 2) /**< Low level of free space */
#define DBG_QUEUE_DATA_HIGH	(DBG_QUEUE_DATA >> 1) /**< Hign level of free space */

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void DBG_vUartInit(uint8 u8Uart, uint32 u32BaudRate);
PUBLIC void	DBG_vPrintf(bool_t bStream, const char *pcFormat, ...);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#endif /* DBG_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/






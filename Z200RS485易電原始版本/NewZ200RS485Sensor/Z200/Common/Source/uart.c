/****************************************************************************/
/*!
 *\MODULE              UART (Application Layer)
 *
 *\COMPONENT           $HeadURL: https://www.collabnet.nxp.com/svn/lprf_apps/Application_Notes/JN-AN-1069-IEEE-802.15.4-Serial-Cable-Replacement/Tags/Release_3v0-Public/Common/Source/uart.c $
 *
 *\VERSION			   $Revision: 16562 $
 *
 *\REVISION            $Id: uart.c 16562 2015-11-24 10:33:02Z nxa04494 $
 *
 *\DATED               $Date: 2015-11-24 10:33:02 +0000 (Tue, 24 Nov 2015) $
 *
 *\AUTHOR              $Author: nxa04494 $
 *
 *\DESCRIPTION         UART (Application Layer) - implementation.
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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
/* Jennic includes */
#include <jendefs.h>
#include <AppHardwareApi.h>

/* Application includes */
#include "dbg.h"
#include "node.h"
#include "queue.h"
#include "uart.h"
#include "wuart.h"
#include "QApp.h"
#include "config.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/* Register locations - dependent upon chip type */
#define UART_ADDRESS_0  	0x02003000UL 	   /**< UART register addresses */
#define UART_ADDRESS_1  	0x02004000UL 	   /**< UART register addresses */

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/** UART structure */
typedef struct
{
	QUEUE_tsData *psTxQueue;		/**< Transmit structure */
	QUEUE_tsData *psRxQueue;		/**< Receive structure */
	uint32    	 u32Address;		/**< Address of UART in memory */
#if UART_STATS
	uint32	  	 u32Txed;			/**< Number of transmitted characters */
	uint32	  	 u32Rxed;			/**< Number of received characters */
	uint32	  	 u32TxDisabled;    	/**< Number of times transmit disabled */
	uint32	  	 u32RxDisabled;		/**< Number of times receive disabled */
#endif
	uint8      	  u8TxDisable;		/**< Transmit disabled flags */
	uint8      	  u8RxDisable;		/**< Transmit disabled flags */
	bool_t	       bOpened;			/**< Port has been opened */
	bool_t         bRtsCts;			/**< RTS/CTS flow control */
	bool_t         bXonXoff;		/**< XON/XOFF flow control */
	bool_t	       bRts;			/**< Current RTS setting */
	bool_t	       bCts;			/**< Current CTS setting */
	bool_t         bTxInt;			/**< Tx interrupt is expected */
} 	tsUart;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void     UART_vInterrupt		(uint32, uint32);
PRIVATE void     UART_vRts  			(uint8, bool_t);
#if UART_PRINTF
PRIVATE bool_t   UART_bNum2String		(uint8, uint32, uint8, uint8, char, bool_t, char, bool_t);
PRIVATE bool_t   UART_bPad2Min			(uint8, uint8, uint8, char);
#endif

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
#ifdef CTSRTS
PUBLIC uint8 gRTS = 0; //RTS active low
#endif
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE tsUart	asUart[2];				 					 /**< Uart data */

/****************************************************************************/
/***        Local Constants                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Public Functions                                              ***/
/****************************************************************************/

/****************************************************************************/
/**
 * <b>UART_bOpen</b> &mdash; Opens UART.
 *
 * This function opens and configures a UART for input and output. Parameters
 * to this function allow the baud rate, parity, word length and stop bits to
 * be set. Additionally, hardware flow control using RTS/CTS or software flow
 * using XON/XOFF can be specified.
 */
/****************************************************************************/
PUBLIC 	bool_t 		UART_bOpen (
		uint8 	   	    u8Uart,			/**< Uart to open */
		uint32    	   u32BaudRate,		/**< Baud rate */
		bool_t   	     bEvenParity,	/**< Even parity */
		bool_t   	     bEnableParity,	/**< Enable parity */
		uint8      	    u8WordLength,	/**< Word length, one of:\n
										 	 E_AHI_UART_WORD_LEN_5\n
										 	 E_AHI_UART_WORD_LEN_6\n
										 	 E_AHI_UART_WORD_LEN_7\n
										 	 E_AHI_UART_WORD_LEN_8 */
		bool_t   	     bOneStopBit,	/**< One stop bit */
		bool_t   	     bRtsCts,		/**< RTS/CTS flow control */
		bool_t   	     bXonXoff,		/**< XON/XOFF flow control */
		QUEUE_tsData   *psTxQueue,		/**< Transmit queue pointer */
		QUEUE_tsData   *psRxQueue)		/**< Receive queue pointer */
{
    uint16 	  u16Divisor;		/* Baud rate divisor */
    uint32 	  u32Remainder;		/* Baud rate remainder */

	/* Valid uart ? */
	if (u8Uart < 2)
	{
		/* Uart 0 ? */
		if      (u8Uart == 0)
		{
			/* Set address in memory of UART */
			asUart[u8Uart].u32Address = UART_ADDRESS_0;
		}
		/* Uart 1 ? */
		else if (u8Uart == 1)
		{
			asUart[u8Uart].u32Address = UART_ADDRESS_1;
		}

		/* Note the queue pointers */
		asUart[u8Uart].psTxQueue = psTxQueue;
		asUart[u8Uart].psRxQueue = psRxQueue;

		/* Start not expected transmit interrupt */
		asUart[u8Uart].bTxInt = FALSE;

		/* Start with transmit and receive enabled */
		asUart[u8Uart].u8TxDisable = 0;
		asUart[u8Uart].u8RxDisable = 0;

		/* Start assuming RTS and CTS are on */
		asUart[u8Uart].bRts = TRUE;
		asUart[u8Uart].bCts = TRUE;

		/* Note passed in flow control settings */
		asUart[u8Uart].bRtsCts  = bRtsCts;
		asUart[u8Uart].bXonXoff = bXonXoff;
		/* Both RTS/CTS and XON/XOFF are specified ? */
		if (asUart[u8Uart].bRtsCts && asUart[u8Uart].bXonXoff)
		{
			/* Disable XON/XOFF control */
			asUart[u8Uart].bXonXoff = FALSE;
		}

		/* Start with port unopened */
		asUart[u8Uart].bOpened = FALSE;

	    /* Calculate divisor for baud rate = 16MHz / (16 x baud rate) */
    	u16Divisor   = (uint16)(16000000UL / (16UL * u32BaudRate));
	    /* Correct for rounding errors */
    	u32Remainder = (uint32)(16000000UL % (16UL * u32BaudRate));
	    if (u32Remainder >= ((16UL * u32BaudRate) / 2)) u16Divisor += 1;

		/* Enable UART */
		vAHI_UartEnable(u8Uart);
		/* Reset UART */
		vAHI_UartReset(u8Uart, TRUE, TRUE);
		vAHI_UartReset(u8Uart, FALSE, FALSE);
		/* Note we've opened the UART */
		asUart[u8Uart].bOpened = TRUE;
	    /* Allow use of RTS/CTS pins with UART if required */
   		vAHI_UartSetRTSCTS(u8Uart, asUart[u8Uart].bRtsCts);
		/* Set baud rate */
		vAHI_UartSetBaudDivisor(u8Uart, u16Divisor);
		/* Set control */
		vAHI_UartSetControl(u8Uart, bEvenParity, bEnableParity, u8WordLength, bOneStopBit, (asUart[u8Uart].u8RxDisable == 0) ? TRUE : FALSE);
		/* Using RTS/CTS flow control ? */
		if (asUart[u8Uart].bRtsCts)
		{
			/* Start with RTS on */
			UART_vRts(u8Uart, (asUart[u8Uart].u8RxDisable == 0) ? TRUE : FALSE);
			/* Is CTS bit set meaning CTS is off ? */
			if (u8AHI_UartReadModemStatus(u8Uart) & 0x10)
			{
				/* Start with transmit disabled */
				asUart[u8Uart].u8TxDisable |= UART_ABLE_UARTIO;
			}
		}
		/* Not JenOS ? */
		#if !UART_JENOS
		{
			/* Register callback for appropriate UART */
			if (u8Uart == E_AHI_UART_0) vAHI_Uart0RegisterCallback(UART_vInterrupt);
			if (u8Uart == E_AHI_UART_1) vAHI_Uart1RegisterCallback(UART_vInterrupt);
		}
		#endif

		/* Set UART interrupts */
		vAHI_UartSetInterrupt(u8Uart,
							  asUart[u8Uart].bRtsCts, 		  /* ModemStatus */
							  FALSE,						  /* RxLineStatus */
							  TRUE,							  /* TxFifoEmpty */
							  TRUE,							  /* RxData */
							  E_AHI_UART_FIFO_LEVEL_4);

		return TRUE;		   /** \retval TRUE Valid UART specified. */
	}

	return FALSE;			   /** \retval FALSE Invalid UART specified. */
}

/****************************************************************************/
/**
 * <b>UART_bGetChar</b> &mdash; Gets next received character from UART.
 *
 * Once a port is opened data received by the UART is internally placed in the
 * receive queue and can be retrieved a character at a time using
 * this function.
 *
 * When the receive queue is near to full, the
 * specified flow control method will be used to halt the flow of data into
 * the UART.
 */
/****************************************************************************/
PUBLIC 	bool_t 		UART_bGetChar (
		uint8  	        u8Uart,		 		/**< Uart to get character from */
		char  	       *pcChar)				/**< Character received pointer */
{
	bool_t bRem = FALSE;					/* Removed from queue */

	/* Uart has been opened ? */
	if (u8Uart < 2 && asUart[u8Uart].bOpened)
	{
		/* Try to remove character from receive queue */
		bRem = QUEUE_bGet(asUart[u8Uart].psRxQueue, (uint8 *) pcChar);
		/* Removed ? */
		if (bRem)
		{
			/* Reached a high amount of free space ? */
			if (QUEUE_bHighReached(asUart[u8Uart].psRxQueue))
			{
				/* Enable receive there is now room in the queue */
				UART_bRxEnable(u8Uart, UART_ABLE_UARTIO);
			}
		}
	}

	return bRem;	/** \retval TRUE character received. */
					/** \retval FALSE no character received. */
}

/****************************************************************************/
/**
 * <b>UART_bPutChar</b> &mdash; Sets next character for transmission by the UART.
 *
 * Data to be transmitted is placed in the transmit queue one character at a time
 * using this function.
 *
 * Data in the transmit queue is output by the UART, unless transmission is
 * disabled by the specified flow control method.
 */
/****************************************************************************/
PUBLIC bool_t UART_bPutChar (
	   uint8  	  u8Uart,					/**< Uart to get character from */
	   char  	   cChar)					/**< Character to transmit */
{
	bool_t bAdd = FALSE;					/* Added to queue */

	/* Uart has been opened ? */
	if (u8Uart < 2 && asUart[u8Uart].bOpened)
	{
		/* Try to add item to queue */
		bAdd = QUEUE_bPut(asUart[u8Uart].psTxQueue, (uint8) cChar);
		#if 0
		/* Added ? */
		if (bAdd)
		{
			/* If interrupts are not being serviced start transmitting */
			if (asUart[u8Uart].bTxInt == FALSE) UART_vStartTx(u8Uart);
		}
		#endif
	}

	return bAdd;	/** \retval TRUE character added for transmission */
					/** \retval FALSE invalid UART or character not added for transmission */
}

/****************************************************************************/
/**
 * <b>UART_bWaitEmptyTx</b> &mdash; Waits for all pending data to be transmitted.
 *
 * This is particularly useful for flushing the UART transmit buffer of any outstanding data
 * before putting an End Device to sleep.
 */
/****************************************************************************/
PUBLIC bool_t UART_bWaitEmptyTx (uint8 u8Uart)
{
	/* Uart has been opened and is allowed to transmit ? */
	if (u8Uart < 2 && asUart[u8Uart].bOpened && asUart[u8Uart].u8TxDisable == 0)
	{
		/* Wait for transmit queue to be empty */
		while (! QUEUE_bEmpty(asUart[u8Uart].psTxQueue));

		/* Wait for tx fifo and shift register to be empty */
		while ((u8AHI_UartReadLineStatus(u8Uart) & (E_AHI_UART_LS_THRE | E_AHI_UART_LS_TEMT)) !=
			(E_AHI_UART_LS_THRE | E_AHI_UART_LS_TEMT));

		return TRUE;	/* \retval TRUE queue has emptied */
	}
	else
	{
		return FALSE;	/* \retval FALSE invalid UART or transmit is disabled */
	}
}

/****************************************************************************/
/**
 * <b>UART_bOpened</b> &mdash; Returns if specified UART has been opened.
 */
/****************************************************************************/
PUBLIC bool_t UART_bOpened (uint8 u8Uart)
{
	return ((u8Uart < 2) ? asUart[u8Uart].bOpened : FALSE);
}

/****************************************************************************/
/**
 * <b>UART_bRtsCts</b> &mdash; Returns if specified UART is using RTS/CTS flow control.
 */
/****************************************************************************/
PUBLIC bool_t UART_bRtsCts (uint8 u8Uart)
{
	return ((u8Uart < 2) ? asUart[u8Uart].bRtsCts : FALSE);
}

/****************************************************************************/
/**
 * <b>UART_bXonXoff</b> &mdash; Returns if specified UART is using XON/XOFF flow control.
 */
/****************************************************************************/
PUBLIC bool_t UART_bXonXoff (uint8 u8Uart)
{
	return ((u8Uart < 2) ? asUart[u8Uart].bXonXoff : FALSE);
}

/****************************************************************************/
/**
 * <b>UART_bRts</b> &mdash; Returns if specified UART has RTS on.
 */
/****************************************************************************/
PUBLIC bool_t UART_bRts (uint8 u8Uart)
{
	return ((u8Uart < 2) ? asUart[u8Uart].bRts : FALSE);
}

/****************************************************************************/
/**
 * <b>UART_bCts</b> &mdash; Returns if specified UART has CTS on.
 */
/****************************************************************************/
PUBLIC bool_t UART_bCts (uint8 u8Uart)
{
	return ((u8Uart < 2) ? asUart[u8Uart].bCts : FALSE);
}

/****************************************************************************/
/**
 * <b>UART_vRxEnable</b> &mdash; Sets receive enabled status.
 *
 * This function set whether the UART is able to receive data taking action
 * appropriate for the specified flow control method.
 */
/****************************************************************************/
PUBLIC bool_t UART_bRxEnable(uint8 u8Uart, uint8 u8Enable)
{
	uint8 u8Old;
	bool_t bReturn = FALSE;

    /* Valid and open uart ? */
    if (u8Uart < 2 && asUart[u8Uart].bOpened)
    {
    	/* Note current disabled flags */
    	u8Old = asUart[u8Uart].u8RxDisable;
		/* Clear reason */
		asUart[u8Uart].u8RxDisable &= (~u8Enable);

		/* Debug */
		if (u8Uart != DBG_UART) DBG_vPrintf(UART_TRACE, "\r\n%d UART < UART_bRxEnable(%d, %x) old=%x new=%x", NODE_sData.u32Timer, u8Uart, u8Enable, u8Old, asUart[u8Uart].u8RxDisable);

		/* Were we disabled and have now removed all reasons ? */
		if (u8Old != 0 && asUart[u8Uart].u8RxDisable == 0)
		{
			/* Debug */
			if (u8Uart != DBG_UART) DBG_vPrintf(UART_TRACE, " ENABLED RX");

			/* Controlling RTS from application ? */
			if (asUart[u8Uart].bRtsCts)
			{
				/* Set RTS manually */
				UART_vRts(u8Uart, TRUE);
			}
			/* Controlling XON/XOFF from application ? */
			else if (asUart[u8Uart].bXonXoff == TRUE)
			{
				/* Output XON character direct to UART */
				vAHI_UartWriteData(u8Uart, CHAR_XON);
				/* Expect a tx interrupt */
				asUart[u8Uart].bTxInt = TRUE;
			}
		}

		/* Success */
		bReturn = TRUE;
	}

	return bReturn;
}

/****************************************************************************/
/**
 * <b>UART_vRxDisable</b> &mdash; Sets receive enabled status.
 *
 * This function set whether the UART is able to receive data taking action
 * appropriate for the specified flow control method.
 */
/****************************************************************************/
PUBLIC bool_t UART_bRxDisable(uint8 u8Uart, uint8 u8Disable)
{
	uint8 u8Old;
	bool_t bReturn = FALSE;

    /* Valid and open uart ? */
    if (u8Uart < 2 && asUart[u8Uart].bOpened)
    {
    	/* Note current disabled flags */
    	u8Old = asUart[u8Uart].u8RxDisable;
		/* Set reason */
		asUart[u8Uart].u8RxDisable |= u8Disable;

		/* Debug */
		if (u8Uart != DBG_UART) DBG_vPrintf(UART_TRACE, "\r\n%d UART < UART_bRxDisable(%d, %x) old=%x new=%x", NODE_sData.u32Timer, u8Uart, u8Disable, u8Old, asUart[u8Uart].u8RxDisable);

		/* Were we enabled and have now added a reason ? */
		if (u8Old == 0 && asUart[u8Uart].u8RxDisable != 0)
		{
			/* Debug */
			if (u8Uart != DBG_UART) DBG_vPrintf(UART_TRACE, " DISABLED RX");

			/* Controlling RTS from application ? */
			if (asUart[u8Uart].bRtsCts)
			{
				/* Set RTS manually */
				UART_vRts(u8Uart, FALSE);
			}
			/* Controlling XON/XOFF from application ? */
			else if (asUart[u8Uart].bXonXoff == TRUE)
			{
				/* Output XOFF character direct to UART */
				vAHI_UartWriteData(u8Uart, CHAR_XOFF);
				/* Expect a tx interrupt */
				asUart[u8Uart].bTxInt = TRUE;
			}
			/* Stats ? */
			#if UART_STATS
			{
				/* Increment disabled receive counter */
				asUart[u8Uart].u32RxDisabled++;
			}
			#endif
		}
		/* Success */
		bReturn = TRUE;
	}

	return bReturn;
}

/****************************************************************************/
/**
 * <b>UART_bRxEnabled</b> &mdash; Returns if specified UART has receive enabled
 */
/****************************************************************************/
PUBLIC bool_t UART_bRxEnabled (uint8 u8Uart)
{
	return ((u8Uart < 2 && asUart[u8Uart].bOpened && asUart[u8Uart].u8RxDisable == 0) ? TRUE : FALSE);
}


/****************************************************************************/
/**
 * <b>UART_bTxEnable</b> &mdash; Sets transmit enabled status.
 *
 * This function set whether the UART is able to transmit data taking action
 * appropriate for the specified flow control method.
 */
/****************************************************************************/
PUBLIC bool_t UART_bTxEnable(uint8 u8Uart, uint8 u8Enable)
{
	uint8 u8Old;
	bool_t bReturn = FALSE;

    /* Valid and open uart ? */
    if (u8Uart < 2 && asUart[u8Uart].bOpened)
    {
    	/* Note current disabled flags */
    	u8Old = asUart[u8Uart].u8TxDisable;
		/* Clear reason */
		asUart[u8Uart].u8TxDisable &= (~u8Enable);

		/* Debug */
		if (u8Uart != DBG_UART) DBG_vPrintf(UART_TRACE, "\r\n%d UART < UART_bTxEnable(%d, %x) old=%x new=%x", NODE_sData.u32Timer, u8Uart, u8Enable, u8Old, asUart[u8Uart].u8TxDisable);

		/* Were we disabled and have now removed all reasons ? */
		if (u8Old != 0 && asUart[u8Uart].u8TxDisable == 0)
		{
			/* Debug */
			if (u8Uart != DBG_UART) DBG_vPrintf(UART_TRACE, " ENABLED TX");

			/* If interrupts are not being serviced start transmitting */
			if (asUart[u8Uart].bTxInt == FALSE) UART_vStartTx(u8Uart);
		}
		/* Success */
		bReturn = TRUE;
	}

	return bReturn;
}

/****************************************************************************/
/**
 * <b>UART_bTxDisable</b> &mdash; Sets transmit enabled status.
 *
 * This function set whether the UART is able to transmit data taking action
 * appropriate for the specified flow control method.
 */
/****************************************************************************/
PUBLIC bool_t UART_bTxDisable(uint8 u8Uart, uint8 u8Disable)
{
	uint8 u8Old;
	bool_t bReturn = FALSE;

    /* Valid and open uart ? */
    if (u8Uart < 2 && asUart[u8Uart].bOpened)
    {
    	/* Note current disabled flags */
    	u8Old = asUart[u8Uart].u8TxDisable;
		/* Set reason */
		asUart[u8Uart].u8TxDisable |= u8Disable;

		/* Debug */
		if (u8Uart != DBG_UART) DBG_vPrintf(UART_TRACE, "\r\n%d UART < UART_bTxDisable(%d, %x) old=%x new=%x", NODE_sData.u32Timer, u8Uart, u8Disable, u8Old, asUart[u8Uart].u8TxDisable);

		/* Were we enabled and have now added a reason ? */
		if (u8Old == 0 && asUart[u8Uart].u8TxDisable != 0)
		{
			/* Debug */
			if (u8Uart != DBG_UART) DBG_vPrintf(UART_TRACE, " DISABLED TX");
		}

		/* Stats ? */
		#if UART_STATS
		{
			/* Increment disabled transmit counter */
			asUart[u8Uart].u32TxDisabled++;
		}
		#endif
		/* Success */
		bReturn = TRUE;
	}

	return bReturn;
}

/****************************************************************************/
/**
 * <b>UART_bTxEnabled</b> &mdash; Returns if specified UART has transmit enabled
 */
/****************************************************************************/
PUBLIC bool_t UART_bTxEnabled (uint8 u8Uart)
{
	return ((u8Uart < 2 && asUart[u8Uart].bOpened && asUart[u8Uart].u8TxDisable == 0) ? TRUE : FALSE);
}

#if UART_PRINTF == TRUE
/****************************************************************************/
/**
 * <b>UART_bPrintf</b> &mdash; Transmit formatted output to UART.
 *
 * Data can formatted and placed in the transmit queue using
 * this function.
 *
 * The formatting syntax available is as follows:
 *
 * \%[<i>flags</i>][<i>min width</i>]<i>specifier</i>
 *
 * <i>Specifiers:</i>
 * <ul>
 * <li> d &mdash; unsigned decimal
 * <li> x &mdash; unsigned hex
 * <li> b &mdash; unsigned binary
 * <li> i &mdash; signed integer
 * <li> c &mdash; character
 * <li> s &mdash; string
 * <li> m &mdash; MAC address uint64, (always outputs 16 digits).
 * </ul>
 *
 * <i>Flags</i>
 * <ul>
 * <li> \# (hash)      &mdash; leading base indicator 0x or 0b, (for x, b, m specifiers).
 * <li> -  (minus)     &mdash; left aligned within width, (for d, x, b, i, s specifiers).
 * <li> +  (plus)      &mdash; leading + on positive integers, (for i specifier).
 * <li> ' ' (space)    &mdash; leading space on positive integers, (for i specifier).
 * <li> : (colon)      &mdash; colon every two chars on MAC address (for m specifier).
 * <li> 0 (zero)       &mdash; pad to width with 0s, (for d, x, b, i, s specifiers).
 * <li> _ (underscore) &mdash; pad to width with underscores, (for d, x, b, i, s specifiers).
 * </ul>
 *
 * <i>Min Width</i>
 * <ul>
 * <li> * (asterisk) &mdash; use next argument as uint8 for min width.
 * </ul>\n
 */
/****************************************************************************/
PUBLIC 	bool_t UART_bPrintf (
		uint8 			u8Uart,		/**< Uart to output to */
		const char     *pcFormat,	/**< Formatting string */
		...)
{
	va_list ap;

	/* Initialise argument pointer */
	va_start(ap, pcFormat);
	/* Call the worker function to do the work */
	return UART_bPrintf_ap (u8Uart, pcFormat, ap);
}
#endif

#if UART_PRINTF == TRUE
/****************************************************************************/
/**
 * <b>vUartIO_Printf</b> &mdash; Format data and output to UART with argument pointer
 */
/****************************************************************************/

PUBLIC 	bool_t UART_bPrintf_ap (
		uint8 			u8Uart,		/**< Uart to output to */
		const char     *pcFormat,	/**< Formatting string */
		va_list         ap)
{
	/* Uart has been opened ? */
	if (u8Uart < 2 && asUart[u8Uart].bOpened)
	{
		char *bp = (char *)pcFormat;
		char c;
		char *p, *t;
		int32 i;
		uint8 u8Min, u8Len;
		char   cPad;
		char   cSign;
		bool_t bPrefix;
		bool_t bLeft;
		uint64 u64Address;

		while ((c = *bp++)) {
			if (c != '%') {
				UART_bPutChar(u8Uart, c);
				continue;
			}

			/* Flags ? */
			bPrefix = FALSE;
			bLeft   = FALSE;
			cSign   = 0;
			cPad   = ' ';
			while (*bp == '#' || *bp == '-' || *bp == '+' || *bp == ':' || *bp == ' ' || *bp == '0' || *bp == '_')
			{
				switch (*bp)
				{
				case '#': bPrefix = TRUE; break; /* Number base flag  */
				case '-': bLeft   = TRUE; break; /* left align flag  */
				case '+': cSign   = '+';  break; /* sign on positive number flag  */
				case ' ': cSign   = ' ';  break; /* space on positive number flag  */
				case ':': cSign   = ':';  break; /* colons in mac address flag */
				case '0': cPad    = '0';  break; /* pad with 0 flag  */
				case '_': cPad    = '_';  break; /* pad with underscore flag */
				}
				bp++;
			}

			/* Minimum width ? */
			u8Min = 0;
			if (*bp == '*')
			{
				u8Min = (uint8) va_arg(ap, uint32);
				bp++;
			}
			else
			{
				while (*bp >= '0' && *bp <= '9')
				{
					u8Min=u8Min*10+(*bp-'0');
					bp++;
				}
			}

			switch ((c = *bp++)) {

			/* %d - show a decimal value */
			case 'd':
				UART_bNum2String(u8Uart, va_arg(ap, uint32), 10, u8Min, cPad, bLeft, 0, bPrefix);
				break;

			/* %x - show a value in hex */
			case 'x':
				UART_bNum2String(u8Uart, va_arg(ap, uint32), 16, u8Min, cPad, bLeft, 0, bPrefix);
				break;

			/* %m - show mac address in hex */
			case 'm':
				if (cSign != ':') cSign = 0;
				u64Address = va_arg(ap, uint64);
				UART_bNum2String(u8Uart, (uint32)(u64Address >> 32),        16, 8, '0', FALSE, cSign, bPrefix);
				if (cSign != 0) UART_bPutChar(u8Uart, cSign);
				UART_bNum2String(u8Uart, (uint32)(u64Address & 0xffffffff), 16, 8, '0', FALSE, cSign, FALSE);
				break;

			/* %b - show a value in binary */
			case 'b':
				UART_bNum2String(u8Uart, va_arg(ap, uint32), 2, u8Min, cPad, bLeft, cSign, bPrefix);
				break;

			case 'i':
				i = va_arg(ap, int32);
				if(i < 0){
					cSign = '-';
					UART_bNum2String(u8Uart, (~i)+1, 10, u8Min, cPad, bLeft, cSign, bPrefix);
				} else {
					if (cSign == ':') cSign = 0;
					UART_bNum2String(u8Uart, i, 10, u8Min, cPad, bLeft, cSign, bPrefix);
				}
				break;

			/* %c - show a character */
			case 'c':
				UART_bPutChar(u8Uart, va_arg(ap, int));
				break;

			/* %s - show a string */
			case 's':
				p = va_arg(ap, char *);
				t = p;
				u8Len = 0;
				while (*t++) ++u8Len;
				if (!bLeft && u8Min>u8Len) UART_bPad2Min(u8Uart, 0, u8Min-u8Len, cPad);
				do {
					UART_bPutChar(u8Uart, *p++);
				} while (*p);
				if (bLeft && u8Min>u8Len) UART_bPad2Min(u8Uart, u8Len, u8Min, cPad);
				break;

			/* %% - show a % character */
			case '%':
				UART_bPutChar(u8Uart, '%');
				break;

			/* %something else not handled ! */
			default:
				UART_bPutChar(u8Uart, '?');
				UART_bPutChar(u8Uart, c);
				break;
			}
		}

		/* If interrupts are not being serviced start transmitting */
		if (asUart[u8Uart].bTxInt == FALSE) UART_vStartTx(u8Uart);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
#endif

/****************************************************************************/
/***        Private Functions                                              ***/
/****************************************************************************/

/****************************************************************************/
/**
 * <b>UART_vIsr0</b> &mdash; UART 0 interrupt service routine.
 */
/****************************************************************************/
#if UART_JENOS
OS_ISR(UART_vIsr0)
{
	/* Have we opened UART0 ? */
	if (asUart[0].bOpened)
	{
		uint8 u8IntStatus;

		u8IntStatus = u8AHI_UartReadInterruptStatus(E_AHI_UART_0);

		#if 0
		/* Echo character */
		vAHI_UartWriteData(0, u8AHI_UartReadData(0));
		#else
		/* Pass on to full interrupt routine */
		UART_vInterrupt(E_AHI_DEVICE_UART0, ((u8IntStatus >> 1) & 0x7));
		#endif
	}
}
#endif

/****************************************************************************/
/**
 * <b>UART_vIsr1</b> &mdash; UART 1 interrupt service routine.
 */
/****************************************************************************/
#if UART_JENOS
OS_ISR(UART_vIsr1)
{
	/* Have we opened UART1 ? */
	if (asUart[1].bOpened)
	{
		uint8 u8IntStatus;

		u8IntStatus = u8AHI_UartReadInterruptStatus(E_AHI_UART_1);

		#if 0
		/* Echo character */
		vAHI_UartWriteData(1, u8AHI_UartReadData(1));
		#else
		/* Pass on to full interrupt routine */
		UART_vInterrupt(E_AHI_DEVICE_UART1, ((u8IntStatus >> 1) & 0x7));
		#endif
	}
}
#endif

/****************************************************************************/
/**
 * <b>UART_vInterrupt</b> &mdash; Uart interrupt handler.
 *
 * Adds received character to receive serial queue, if available.
 *
 * Transmits character from transmit serial queue, if available.
 *
 * Reacts to change in CTS status when not using automatic flow control.
 */
/****************************************************************************/
PRIVATE void UART_vInterrupt(uint32 u32Device, 	 	/**< Interrupting device */
					         uint32 u32ItemBitmap) 	/**< Interrupt bitmask */
{
	uint8 u8ModemStatus;
	uint8 u8RxChar;
	uint8 u8Uart = 0xFF;
	uint8 u8Data;
	bool_t bAdd;

	/* Which uart ? */
	if (u32Device == E_AHI_DEVICE_UART0) u8Uart = E_AHI_UART_0;
	if (u32Device == E_AHI_DEVICE_UART1) u8Uart = E_AHI_UART_1;

	/* Interrupt is for a valid uart and it is open? */
    if (u8Uart < 2 && asUart[u8Uart].bOpened)
    {
        /* Ready to transmit ? */
        if ((u32ItemBitmap & 0xFF) == E_AHI_UART_INT_TX)
        {
			/* We will service this interrupt but might generate another */
			asUart[u8Uart].bTxInt = FALSE;
        }
		/* Modem status changed ? */
		if ((u32ItemBitmap & 0xFF) == E_AHI_UART_INT_MODEM)
		{
			/* Read modem status */
			u8ModemStatus = u8AHI_UartReadModemStatus(u8Uart);
			/* Has CTS changed ? */
			if (u8ModemStatus & E_AHI_UART_MS_DCTS)
			{
				/* Controlling flow from application using CTS ? */
				if (asUart[u8Uart].bRtsCts == TRUE)
				{
					/* Note current CTS setting (CTS bit set means CTS has cleared) */
					asUart[u8Uart].bCts = (u8ModemStatus & 0x10) ? FALSE : TRUE;
					/* Enabling transmit ? */
					if (asUart[u8Uart].bCts)
					{
						UART_bTxEnable(u8Uart, UART_ABLE_UARTIO);
					}
					/* Disabling transmit ? */
					else
					{
						UART_bTxDisable(u8Uart, UART_ABLE_UARTIO);
					}
				}
			}
		}

    	/* Data to receive ? */
        if ((u32ItemBitmap & 0xFF) == E_AHI_UART_INT_RXDATA ||
        	(u32ItemBitmap & 0xFF) == E_AHI_UART_INT_TIMEOUT)
        {
        	if ((u32ItemBitmap & 0xFF) == E_AHI_UART_INT_TIMEOUT){
					//LED2Off
				gWaitUARTPacket = FALSE;
				gCheckRXTimeout = FALSE;
				//DBG_vPrintf(1, "Ti04\r\n");
					//LED2On
        	}else{
     			gCheckTimeoutCounter = gRxdelay_count;
				gWaitUARTPacket = TRUE;
				gCheckRXTimeout = TRUE;
				//DBG_vPrintf(1, "Rx04\r\n");
         	}
			/* While there is data in the receive fifo */
			while (u8AHI_UartReadLineStatus(u8Uart) & E_AHI_UART_LS_DR)
			{
				/* Receive character from UART */
				u8RxChar = u8AHI_UartReadData(u8Uart);

				/* Using XON/XOFF flow control and this is an XON/XOFF character ? */
				if (asUart[u8Uart].bXonXoff == TRUE &&
				   (u8RxChar == CHAR_XON || u8RxChar == CHAR_XOFF))
				{
					/* Enabling transmit ? */
					if (u8RxChar == CHAR_XON)
					{
						UART_bTxEnable(u8Uart, UART_ABLE_UARTIO);
					}
					/* Disabling transmit ? */
					else
					{
						UART_bTxDisable(u8Uart, UART_ABLE_UARTIO);
					}
				}
				else
				{
					//gWaitUARTPacketCount = 0;  //New character from UART, keeps waiting.
					/* Stats ? */
					#if UART_STATS
					{
						/* Increment received character counter */
						asUart[u8Uart].u32Rxed++;
					}
					#endif
					/* Add character to receive queue */
					bAdd = QUEUE_bPut(asUart[u8Uart].psRxQueue, u8RxChar);
					/* Added ? */
					if (bAdd)
					{
						/* Reached a low amount of free space ? */
						if (QUEUE_bLowReached(asUart[u8Uart].psRxQueue))
						{
							/* Disable receive there is now room in the queue */
							UART_bRxDisable(u8Uart, UART_ABLE_UARTIO);
						}
					}
				}
			}
        }
        /* Ready to transmit ? */
        if ((u32ItemBitmap & 0xFF) == E_AHI_UART_INT_TX)
        {
        	//LED3Off
			/* Did we not service the last tx interrupt ? */
			if (asUart[u8Uart].bTxInt == FALSE)
			{
				/* Allowed to transmit ? */
				if(asUart[u8Uart].u8TxDisable == 0)
				{
					/* Got some data in the queue ? */
					if(! QUEUE_bEmpty_Int(asUart[u8Uart].psTxQueue))
					//if(asUart[u8Uart].psTxQueue->u16Tail != asUart[u8Uart].psTxQueue->u16Head)
					{
						uint8 u8Write;
						bool_t bGet = TRUE;

						/* Write up to 16 bytes of data to fifo */
						for (u8Write = 0;
							 u8Write < 16 && bGet == TRUE;
							 u8Write++)
						{
							/* Attempt to get byte from queue */
							bGet = QUEUE_bGet(asUart[u8Uart].psTxQueue, &u8Data);
							/* Got a byte ? */
							if (bGet == TRUE)
							{
								/* Output to UART */
								vAHI_UartWriteData(u8Uart, u8Data);
								/* Note we serviced the interrupt, tx interrupts will continue */
								asUart[u8Uart].bTxInt = TRUE;
								/* Stats ? */
								#if UART_STATS
								{
									/* Increment transmitted character counter */
									asUart[u8Uart].u32Txed++;
								}
								#endif
							}
						}
					}
#if (hardware == Z200485) || (hardware == Z200Old485)
					else{
						if (u8Uart == WUART_UART){
							gWaitDECount =2;
							gTurn485ReceiveMode = 1;
						}
					}
#endif
				}
			}
        	//LED3On
        }
    }
}

/****************************************************************************/
/**
 * <b>UART_vRts</b> &mdash; Set or clear RTS.
 *
 * Directly using MCR register.
 */
/****************************************************************************/
PRIVATE void UART_vRts(uint8 u8Uart,	/**< Uart to set RTS for */
					   bool_t bEnable)	/**< RTS status */
{
    /* Valid and open uart ? */
    if (u8Uart < 2 && asUart[u8Uart].bOpened)
    {
		vAHI_UartSetRTS(u8Uart, !bEnable);
		/* Note rts value */
		asUart[u8Uart].bRts  = bEnable;
	}
}

/****************************************************************************/
/**
 * <b>UART_vStartTx</b> &mdash; Start transmitting from UART.
 *
 * If we are not currently servicing transmit interrupts and we have data
 * to transmit, begin transmission by writing the initial character to
 * the transmit buffer.
 */
/****************************************************************************/
PUBLIC void UART_vStartTx(uint8 u8Uart)
{
    uint8 u8Data;
#ifdef CTSRTS
    if (gRTS==0){
#endif
	/* Did we not service the last tx interrupt ? */
	if (asUart[u8Uart].bTxInt == FALSE)
	{
		//DBG_vPrintf(TRUE, "\r\n===TxInt===");
	    /* Has interrupt driven transmit stalled (tx fifo is empty) */
    	if (u8AHI_UartReadLineStatus(u8Uart) & E_AHI_UART_LS_THRE)
    	{
			/* Allowed to transmit ? */
			if(asUart[u8Uart].u8TxDisable == 0)
			{
				/* Got some data in the queue ? */
				if(! QUEUE_bEmpty(asUart[u8Uart].psTxQueue))
				{
					uint8 u8Write;
					bool_t bGet = TRUE;
#if (hardware == Z200485) | (hardware == Z200Old485)
					if (u8Uart == WUART_UART){
						DEOn
						REOn
					}
#endif
					/* Write up to 16 bytes of data to fifo */
					for (u8Write = 0;
						 u8Write < 16 && bGet == TRUE;
						 u8Write++)
					{

						/* Attempt to get byte from queue */
						bGet = QUEUE_bGet(asUart[u8Uart].psTxQueue, &u8Data);
						/* Got a byte ? */
						if (bGet == TRUE)
						{
							/* Output to UART */
							vAHI_UartWriteData(u8Uart, u8Data);
							/* Note we serviced the interrupt, tx interrupts will continue */
							asUart[u8Uart].bTxInt = TRUE;
							/* Stats ? */
							#if UART_STATS
							{
								/* Increment transmitted character counter */
								asUart[u8Uart].u32Txed++;
							}
							#endif
						}
					}
				}
			}
    	}
    }
#ifdef CTSRTS
    }
#endif
}

#if UART_PRINTF
/****************************************************************************/
/**
 * <b>UART_bNum2String</b> &mdash; Convert a number to a string.
 */
/****************************************************************************/
PRIVATE bool_t UART_bNum2String(uint8 u8Uart, uint32 u32Number, uint8 u8Base, uint8 u8Min, char cPad, bool_t bLeft, char cSign, bool_t bPrefix)
{
	/* Uart has been opened ? */
	if (u8Uart < 2 && asUart[u8Uart].bOpened)
	{
		char buf[33];
		char *p = buf + 33;
		uint32 c, n;
		uint8 u8Width=0;
		uint8 u8Pos=0;

		if (u8Min > 32) u8Min=32;

		*--p = '\0';
		do {
			n = u32Number / u8Base;
			c = u32Number - (n * u8Base);
			if (c < 10) {
				*--p = '0' + c;
			} else {
				*--p = 'A' + (c - 10);
			}
			u32Number /= u8Base;
			u8Width++;
		} while (u32Number != 0);

		if (cPad == '0')
		{
			while (u8Width < u8Min)
			{
				*--p = '0';
				u8Width++;
			}
		}

		if (cPad != '0' && bLeft == FALSE)
		{
			UART_bPad2Min(u8Uart, u8Width, u8Min, cPad);
		}

		if(cSign != 0 && cSign != ':') UART_bPutChar(u8Uart, cSign);

		if (bPrefix)
		{
			switch (u8Base)
			{
				case 16:
					UART_bPutChar(u8Uart, '0');
					UART_bPutChar(u8Uart, 'x');
					break;
				case 2:
					UART_bPutChar(u8Uart, '0');
					UART_bPutChar(u8Uart, 'b');
					break;
			}
		}

		while (*p){
			UART_bPutChar(u8Uart, *p);
			p++;
			if (cSign == ':')
			{
				if ((u8Pos%2)==1 && (*p))
				{
					UART_bPutChar(u8Uart, cSign);
				}
				u8Pos++;
			}
		}

		if (cPad != '0' && bLeft == TRUE)
		{
			UART_bPad2Min(u8Uart, u8Width, u8Min, cPad);
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
#endif /* UART_PRINTF */

#if UART_PRINTF
/****************************************************************************/
/**
 * <b>UART_bPad2Min</b> &mdash; Convert a number to a string
 */
/****************************************************************************/
PRIVATE bool_t UART_bPad2Min(uint8 u8Uart, uint8 u8Width, uint8 u8Min, char cPad)
{
	/* Uart has been opened ? */
	if (u8Uart < 2 && asUart[u8Uart].bOpened)
	{
		while (u8Width < u8Min)
		{
			UART_bPutChar(u8Uart, cPad);
			u8Width++;
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
#endif /* UART_PRINTF */

#if UART_STATS
/****************************************************************************/
/**
 * <b>UART_u32Txed</b> &mdash; Returns number of transmitted chars
 */
/****************************************************************************/
PUBLIC uint32 UART_u32Txed (uint8 u8Uart)
{
	return ((u8Uart < 2 && asUart[u8Uart].bOpened) ? asUart[u8Uart].u32Txed : 0);
}
#endif /* UART_STATS */

#if UART_STATS
/****************************************************************************/
/**
 * <b>UART_u32Rxed</b> &mdash; Returns number of received chars
 */
/****************************************************************************/
PUBLIC uint32 UART_u32Rxed (uint8 u8Uart)
{
	return ((u8Uart < 2 && asUart[u8Uart].bOpened) ? asUart[u8Uart].u32Rxed : 0);
}
#endif /* UART_STATS */

#if UART_STATS
/****************************************************************************/
/**
 * <b>UART_u32TxDisabled</b> &mdash; Returns number of times transmit disabled
 */
/****************************************************************************/
PUBLIC uint32 UART_u32TxDisabled (uint8 u8Uart)
{
	return ((u8Uart < 2 && asUart[u8Uart].bOpened) ? asUart[u8Uart].u32TxDisabled : 0);
}
#endif /* UART_STATS */

#if UART_STATS
/****************************************************************************/
/**
 * <b>UART_u32RxDisabled</b> &mdash; Returns number of receive disabled
 */
/****************************************************************************/
PUBLIC uint32 UART_u32RxDisabled (uint8 u8Uart)
{
	return ((u8Uart < 2 && asUart[u8Uart].bOpened) ? asUart[u8Uart].u32RxDisabled : 0);
}
#endif /* UART_STATS */

void sendAChar(char achar){
	bool bAdd=FALSE;

	bAdd = QUEUE_bPut(asUart[WUART_UART].psRxQueue, achar);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

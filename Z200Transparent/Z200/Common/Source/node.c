/*****************************************************************************/
/*!
 *\MODULE              IEEE 802.15.4 Node (Application Layer)
 *
 *\COMPONENT           $HeadURL: https://www.collabnet.nxp.com/svn/lprf_apps/Application_Notes/JN-AN-1069-IEEE-802.15.4-Serial-Cable-Replacement/Tags/Release_3v0-Public/Common/Source/node.c $
 *
 *\VERSION			   $Revision: 16555 $
 *
 *\REVISION            $Id: node.c 16555 2015-11-20 12:14:37Z nxa04494 $
 *
 *\DATED               $Date: 2015-11-20 12:14:37 +0000 (Fri, 20 Nov 2015) $
 *
 *\AUTHOR              $Author: nxa04494 $
 *
 *\DESCRIPTION         IEEE 802.15.4 Node (Application Layer) - implementation.
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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
/* Standard includes */
#include <string.h>

/* Stack includes */
#include <AppApi.h>
#include <Button.h>
#include <mac_pib.h>

/* Application includes */
#include "dbg.h"
#include "node.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PUBLIC NODE_tsData	NODE_sData;						  /**< Node information */

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: NODE_vInit
 *
 * DESCRIPTION:
 * Initialises Zigbee stack, hardware and application.
 *
 ****************************************************************************/
PUBLIC void NODE_vInit(void)
{
	/* Debug */
	DBG_vPrintf(NODE_TRACE, "\r\n%d NODE < NODE_vInit()", NODE_sData.u32Timer);

	/* No state yet */
	NODE_sData.eNwkState = NODE_NWKSTATE_NONE;

	/* Initialise non-zero node structure members */
	NODE_sData.u64Address   = *(uint64 *) pvAppApiGetMacAddrLocation();	/**< Address of node */
	NODE_sData.u16Address   = 0xffff;									/**< Short address of node */
	NODE_sData.u64Parent    = NODE_ADDRESS_NULL;						/**< Extended address of node's parent */
	NODE_sData.u16Parent    = 0xffff;									/**< Short address of node's parent */

	/* Initialise node structure pointer members */
	NODE_sData.pvMac = (void *) pvAppApiGetMacHandle();
	NODE_sData.psPib = MAC_psPibGetHandle(NODE_sData.pvMac);

	/* Note initial MinBe */
	NODE_sData.u8DefPibMinBe = NODE_sData.psPib->u8MinBe_ReadOnly;
	NODE_sData.u8DefPibMinBe = NODE_sData.psPib->u8_MIN_BE_READ_ONLY;

}

/****************************************************************************
 *
 * NAME: NODE_vTick
 *
 * DESCRIPTION:
 * Called regularly off a timer
 *
 ****************************************************************************/
PUBLIC void NODE_vTick (void)
{
	/* Increment node tick counters */
	NODE_sData.u32Timer++;
	NODE_sData.u32TimerSync++;
	DBG_vPrintf(1,"\r\n%d NODE < NODE_TickTimerTask()");
	/* Debug this tick ? */
	if ((NODE_sData.u32Timer & NODE_TICKTRACE) == 0)
	{
		/* Debug */
		DBG_vPrintf(1, "\r\n%d NODE < NODE_TickTimerTask()", NODE_sData.u32Timer);
	}
}

/****************************************************************************
 *
 * NAME: NODE_vTxAck
 *
 * DESCRIPTION:
 * Transmit data over the radio
 *
 ****************************************************************************/
PUBLIC void NODE_vTxAck(uint64 u64DstAddr, uint16 u16Size, uint8 *pu8Payload)
{
    MAC_McpsReqRsp_s  sMcpsReqRsp;
    MAC_McpsSyncCfm_s sMcpsSyncCfm;

	/* Debug */
	DBG_vPrintf(NODE_DATA_TRACE, "\r\n%d DATA < NODE_vTxAx\ck(%x:%x %d %s, %#x)",
		NODE_sData.u32Timer,
		(uint32)(u64DstAddr >> 32),
		(uint32)(u64DstAddr & 0xffffffff),
		u16Size,
		pu8Payload,NODE_sData.u16PanId);

    /* Create frame transmission request */
    sMcpsReqRsp.u8Type = MAC_MCPS_REQ_DATA;
    sMcpsReqRsp.u8ParamLength = sizeof(MAC_McpsReqData_s);
    /* Set handle so we can match confirmation to request */
    sMcpsReqRsp.uParam.sReqData.u8Handle = 1;
    /* Use short address for source */
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u8AddrMode = 2;
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u16PanId = NODE_sData.u16PanId;
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.u16Short = NODE_sData.u16Address;
	/* Short address ? */
	if (u64DstAddr <= 0xffffULL)
	{
		/* Use short address for destination */
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode = 2;
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u16PanId = NODE_sData.u16PanId;
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.u16Short = (uint16) u64DstAddr;
	}
	/* Extended address ? */
	else
	{
		/* Use short address for destination */
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode = 3;
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u16PanId = NODE_sData.u16PanId;
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.sExt.u32H	= (uint32)(u64DstAddr >> 32);
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.sExt.u32L = (uint32)(u64DstAddr &  0xffffffffULL);
	}

    /* Frame requires ack but not security, indirect transmit or GTS */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8TxOptions = 1;//MAC_TX_OPTION_ACK;

	/* Copy passed in payload */
	memcpy(sMcpsReqRsp.uParam.sReqData.sFrame.au8Sdu, pu8Payload, u16Size);

    /* Set frame length */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8SduLength = u16Size;

	/* Lower MinBe while we send this for speed */
	MAC_vPibSetMinBe(NODE_sData.pvMac, 1);

    /* Request transmit */
    vAppApiMcpsRequest(&sMcpsReqRsp, &sMcpsSyncCfm);

	/* Restore default MinBe now we've sent */
	MAC_vPibSetMinBe(NODE_sData.pvMac, NODE_sData.u8DefPibMinBe);
}

/****************************************************************************
 *
 * NAME: NODE_vTx
 *
 * DESCRIPTION:
 * Transmit data over the radio
 *
 ****************************************************************************/
PUBLIC void NODE_vTx(uint64 u64DstAddr, uint16 u16Size, uint8 *pu8Payload)
{
    MAC_McpsReqRsp_s  sMcpsReqRsp;
    MAC_McpsSyncCfm_s sMcpsSyncCfm;

	/* Debug */
	DBG_vPrintf(NODE_DATA_TRACE, "\r\n%d DATA < NODE_vTx(%x:%x %d %s %#x)",
		NODE_sData.u32Timer,
		(uint32)(u64DstAddr >> 32),
		(uint32)(u64DstAddr & 0xffffffff),
		u16Size,
		pu8Payload,NODE_sData.u16PanId );

    /* Create frame transmission request */
    sMcpsReqRsp.u8Type = MAC_MCPS_REQ_DATA;
    sMcpsReqRsp.u8ParamLength = sizeof(MAC_McpsReqData_s);
    /* Set handle so we can match confirmation to request */
    sMcpsReqRsp.uParam.sReqData.u8Handle = 1;
    /* Use short address for source */
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u8AddrMode = 2;
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u16PanId = NODE_sData.u16PanId;
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.u16Short = NODE_sData.u16Address;
	/* Short address ? */
	if (u64DstAddr <= 0xffffULL)
	{
		/* Use short address for destination */
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode = 2;
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u16PanId = NODE_sData.u16PanId;
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.u16Short = (uint16) u64DstAddr;
	}
	/* Extended address ? */
	else
	{
		/* Use short address for destination */
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode = 3;
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u16PanId = NODE_sData.u16PanId;
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.sExt.u32H	= (uint32)(u64DstAddr >> 32);
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.sExt.u32L = (uint32)(u64DstAddr &  0xffffffffULL);
	}

    /* Frame requires ack but not security, indirect transmit or GTS */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8TxOptions = 0;//MAC_TX_OPTION_NoACK;

	/* Copy passed in payload */
	memcpy(sMcpsReqRsp.uParam.sReqData.sFrame.au8Sdu, pu8Payload, u16Size);

    /* Set frame length */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8SduLength = u16Size;

	/* Lower MinBe while we send this for speed */
	MAC_vPibSetMinBe(NODE_sData.pvMac, 1);

    /* Request transmit */
    vAppApiMcpsRequest(&sMcpsReqRsp, &sMcpsSyncCfm);

	/* Restore default MinBe now we've sent */
	MAC_vPibSetMinBe(NODE_sData.pvMac, NODE_sData.u8DefPibMinBe);
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/


#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <LedControl.h>
#include <mac_sap.h>
#include <mac_pib.h>
/* Common defines */
#include "config.h"
#include "dbg.h"
#include "node.h"
#include "rnd.h"
#include "wuart.h"
#include "QApp.h"
#include "QEvent_Process.h"
#ifdef SWDEBUG
    #include <gdb.h>
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define ED_TRACE TRUE

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct
{
	uint32  		u32ScanChannels;
} ED_tsData;


extern void ED_vReqAssociate(void);
void ED_vDcfmAssociate(MAC_MlmeDcfmInd_s *psMlmeInd);



/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE ED_tsData ED_sData;
PRIVATE uint8 u8Bit;
PRIVATE uint8 u8Channel;
PRIVATE bool bMoreChannels;
PRIVATE uint8 u8BeaconCounter = 0;
PRIVATE uint8 u8Beacon;
PRIVATE uint16 au8RejectedBeacons[500]; /* 500 potential short MAC addresses */
PRIVATE bool bAlreadySaved = FALSE;
PRIVATE uint8 u8SavedBeacons = 0;
PRIVATE bool bAssociate = FALSE;

extern bool onetooneSlave_Flag;
//===== End Device Part =====
PUBLIC void ED_vReqStartEnddevice(void)
{
    /* Structures used to hold data for MLME request and response */
    MAC_MlmeReqRsp_s   sMlmeReqRsp;
    MAC_MlmeSyncCfm_s  sMlmeSyncCfm;

	/* Network is now up */
    NODE_sData.eNwkState = NODE_NWKSTATE_UP;

	/* Debug */
	DBG_vPrintf(ED_TRACE, "\r\n%d ED < ED_vReqStartEnddevice()",
		NODE_sData.u32Timer);

    /* Start end device */
    sMlmeReqRsp.u8Type = MAC_MLME_REQ_START;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqStart_s);
    sMlmeReqRsp.uParam.sReqStart.u16PanId = NODE_sData.u16PanId;
    sMlmeReqRsp.uParam.sReqStart.u8Channel = NODE_sData.u8Channel;
    sMlmeReqRsp.uParam.sReqStart.u8BeaconOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8SuperframeOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8PanCoordinator = TRUE;
    sMlmeReqRsp.uParam.sReqStart.u8BatteryLifeExt = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8Realignment = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8SecurityEnable = FALSE;

    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);

	/* Wuart not running ? */
	if (WUART_sData.u8State == WUART_STATE_NONE)
	{
		/* Put wuart into idle state */
		WUART_vState(WUART_STATE_IDLE);
	}
}


PUBLIC void ED_vReqActiveScan(void)
{
    MAC_MlmeReqRsp_s  sMlmeReqRsp;
    MAC_MlmeSyncCfm_s sMlmeSyncCfm;

	/* Debug */
	DBG_vPrintf(ED_TRACE, "\r\n%d ED < ED_vReqActiveScan() Scan=%#x",
		NODE_sData.u32Timer,
		ED_sData.u32ScanChannels);

   	/* Invalidate channel and addresses */
    NODE_sData.u8Channel  = 0;
    NODE_sData.u16Address =	0xffff;
    NODE_sData.u16Parent  =	0xffff;

    NODE_sData.eNwkState = NODE_NWKSTATE_DISCOVER;

    /* Request scan */
    sMlmeReqRsp.u8Type = MAC_MLME_REQ_SCAN;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqScan_s);
    sMlmeReqRsp.uParam.sReqScan.u8ScanType = MAC_MLME_SCAN_TYPE_ACTIVE;
    sMlmeReqRsp.uParam.sReqScan.u32ScanChannels = SCAN_CHANNELS;
    sMlmeReqRsp.uParam.sReqScan.u8ScanDuration = ACTIVE_SCAN_DURATION;

    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}

PUBLIC void ED_vDcfmScanActive(MAC_MlmeDcfmInd_s *psMlmeInd)
{
	DBG_vPrintf(ED_TRACE, "\r\n%d ED < ED_vDcfmScanActive()",	NODE_sData.u32Timer);
/*
	if (gSerialMode == CommandMode_c){
		return;
	}
	*/
    if (psMlmeInd->uParam.sDcfmScan.u8ScanType == MAC_MLME_SCAN_TYPE_ACTIVE)
    {
		/* Debug */
		DBG_vPrintf(ED_TRACE, " Status=%d Unscanned=%#x",
			psMlmeInd->uParam.sDcfmScan.u8Status,
			psMlmeInd->uParam.sDcfmScan.u32UnscannedChannels);

        if (psMlmeInd->uParam.sDcfmScan.u8Status == MAC_ENUM_SUCCESS)
        {
		    uint8 u8Result;
	    	MAC_PanDescr_s *psPanDesc;
	    	//DBG_vPrintf(ED_TRACE, "\r\nED < ED_1");
			/* Loop through results */
			for (u8Result = 0; u8Result < psMlmeInd->uParam.sDcfmScan.u8ResultListSize; u8Result++)
            {
            	/* Get pointer to this result */
                psPanDesc = &psMlmeInd->uParam.sDcfmScan.uList.asPanDescr[u8Result];

                /* Reset saved flag */
				bAlreadySaved = FALSE;
		    	//DBG_vPrintf(ED_TRACE, "\r\nED < ED_2");

				if(u8BeaconCounter == 0) /* first beacon ? */
				{
					/* Matching PAN ID and
					   short addressing used and
					   accepting association requests ? */
			    	//DBG_vPrintf(ED_TRACE, "\r\nED < ED_3");
	                if ((psPanDesc->sCoord.u16PanId   == NODE_sData.u16PanId) &&
	                    (psPanDesc->sCoord.u8AddrMode == 2) &&
	                    (psPanDesc->u16SuperframeSpec & 0x8000))
	                {
	                	/* Attempt association */
	                    NODE_sData.u8Channel = psPanDesc->u8LogicalChan;
	                    NODE_sData.u16Parent = psPanDesc->sCoord.uAddr.u16Short;
						/* Debug */
						DBG_vPrintf(ED_TRACE, " 0-Chan=%d Coord=%#x",
							psPanDesc->u8LogicalChan, psPanDesc->sCoord.uAddr.u16Short);
						bAssociate = TRUE;
						ED_vReqAssociate();
						break;//Associate only once.
	                }
	                else
	                {
	                	if(psPanDesc->sCoord.u8AddrMode == 2) /* short address */
						{
							au8RejectedBeacons[u8BeaconCounter] = psPanDesc->sCoord.uAddr.u16Short;
						}
						else
						{
							au8RejectedBeacons[u8BeaconCounter] = (uint16) psPanDesc->sCoord.uAddr.sExt.u32L;
						}
						u8BeaconCounter++;
						u8SavedBeacons++;
	                }
				}
				else
				{
					/* check if already saved */
    	//DBG_vPrintf(ED_TRACE, "\r\nED < ED_4");
					for(u8Beacon = 0; u8Beacon < u8BeaconCounter; u8Beacon++)
					{
						if(psPanDesc->sCoord.u8AddrMode == 2) /* short address */
						{
							if(psPanDesc->sCoord.uAddr.u16Short == au8RejectedBeacons[u8Beacon])
							{
								bAlreadySaved = TRUE;
								break;
							}
						}
						else if(psPanDesc->sCoord.u8AddrMode == 3) /* ext address */
						{
							if(((uint16) psPanDesc->sCoord.uAddr.sExt.u32L) == au8RejectedBeacons[u8Beacon])
							{
								bAlreadySaved = TRUE;
								break;
							}
						}
					}

					if(!bAlreadySaved)
					{
				    	//DBG_vPrintf(ED_TRACE, "\r\nED < ED_A");
						/* Matching PAN ID and
						   short addressing used and
						   accepting association requests ? */
						if ((psPanDesc->sCoord.u16PanId   == NODE_sData.u16PanId) &&
							(psPanDesc->sCoord.u8AddrMode == 2) &&
							(psPanDesc->u16SuperframeSpec & 0x8000))
						{
							/* Attempt association */
							NODE_sData.u8Channel = psPanDesc->u8LogicalChan;
							NODE_sData.u16Parent = psPanDesc->sCoord.uAddr.u16Short;
							/* Debug */
							DBG_vPrintf(ED_TRACE, " 1-Chan=%d Coord=%#x",
								psPanDesc->u8LogicalChan, psPanDesc->sCoord.uAddr.u16Short);
							bAssociate = TRUE;
							ED_vReqAssociate();
						}
						else /* save beacon's addr */
						{
					    	//DBG_vPrintf(ED_TRACE, "\r\nED < ED_B");
							if(psPanDesc->sCoord.u8AddrMode == 2) /* short address */
							{
								au8RejectedBeacons[u8BeaconCounter] = psPanDesc->sCoord.uAddr.u16Short;
							}
							else
							{
								au8RejectedBeacons[u8BeaconCounter] = (uint16) psPanDesc->sCoord.uAddr.sExt.u32L;
							}
							u8BeaconCounter++;
							u8SavedBeacons++;
						}
					}
				}
            }

			if(bAssociate)
			{
				bAssociate = FALSE;
			}
			else if(u8SavedBeacons > 0) /* unique beacons ? */
			{
				u8SavedBeacons = 0;
				/* Do another scan of the same channel */
				ED_vReqActiveScan();
			}
			else /* no unique beacons */
			{
		    //DBG_vPrintf(ED_TRACE, "\r\nED < ED_D");
				//ED_sData.u32ScanChannels &= ~(1 << u8Channel); /* clear bit */
				for(u8Bit = (u8Channel+1); u8Bit <= 26; u8Bit++)
				{
					if(SCAN_CHANNELS & (1 << u8Bit))
					{
						u8Channel = u8Bit;
						bMoreChannels = TRUE;
						break;
					}
				}
				if(bMoreChannels)
				{
			    	//DBG_vPrintf(ED_TRACE, "\r\nED < ED_E");
					bMoreChannels = FALSE;
					ED_sData.u32ScanChannels |= (1 << u8Channel);
					/* Start the next scan */
					ED_vReqActiveScan();
				}
				else
				{
			    	//DBG_vPrintf(ED_TRACE, "\r\nED < ED_LoopAgain");
					u8SavedBeacons = 0;
					u8BeaconCounter = 0;
					/* Go to sleep */
					NODE_sData.eNwkState = NODE_NWKSTATE_RESCAN;
					ED_vReqActiveScan();
				}
			}
        }
        else if (psMlmeInd->uParam.sDcfmScan.u8Status == MAC_ENUM_NO_BEACON)
        {
        	/* No beacons found on this channel, go to next one */
  	//DBG_vPrintf(ED_TRACE, "\r\nED < ED_5");
			ED_sData.u32ScanChannels &= ~(1 << u8Channel); /* clear bit */
			for(u8Bit = (u8Channel+1); u8Bit <= 26; u8Bit++)
			{
				if(SCAN_CHANNELS & (1 << u8Bit))
				{
					u8Channel = u8Bit;
					bMoreChannels = TRUE;
					break;
				}
			}
			if(bMoreChannels)
			{
    	//DBG_vPrintf(ED_TRACE, "\r\nED < ED_6");
				bMoreChannels = FALSE;
				ED_sData.u32ScanChannels |= (1 << u8Channel);
				/* Start the next scan */
				ED_vReqActiveScan();
			}
			else
			{
    	//DBG_vPrintf(ED_TRACE, "\r\nED < ED_7");
				u8SavedBeacons = 0;
				u8BeaconCounter = 0;
				/* Go to sleep */
				NODE_sData.eNwkState = NODE_NWKSTATE_RESCAN;
				ED_vReqActiveScan();
			}
        }
        else
        {
  	DBG_vPrintf(ED_TRACE, "\r\nED < ED_else");
        	/* Something went wrong */
			/* Attempt another active scan */
			ED_vReqActiveScan();
        }
    }
}

PUBLIC void ED_vReqAssociate(void)
{
    MAC_MlmeReqRsp_s  sMlmeReqRsp;
    MAC_MlmeSyncCfm_s sMlmeSyncCfm;

    NODE_sData.eNwkState = NODE_NWKSTATE_JOIN;

	/* Debug */
	DBG_vPrintf(ED_TRACE, "\r\n%d ED < ED_vReqAssociate()",
		NODE_sData.u32Timer);

    /* Create associate request. We know short address and PAN ID of
       coordinator as this is preset and we have checked that received
       beacon matched this */

    sMlmeReqRsp.u8Type = MAC_MLME_REQ_ASSOCIATE;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqAssociate_s);
    sMlmeReqRsp.uParam.sReqAssociate.u8LogicalChan = NODE_sData.u8Channel;
    sMlmeReqRsp.uParam.sReqAssociate.u8Capability = 0x80; /* We want short address, other features off */
    sMlmeReqRsp.uParam.sReqAssociate.u8SecurityEnable = FALSE;
    sMlmeReqRsp.uParam.sReqAssociate.sCoord.u8AddrMode = 2;
    sMlmeReqRsp.uParam.sReqAssociate.sCoord.u16PanId = NODE_sData.u16PanId;
    sMlmeReqRsp.uParam.sReqAssociate.sCoord.uAddr.u16Short = NODE_sData.u16Parent;

    /* Put in associate request and check immediate confirm. Should be
       deferred, in which case response is handled by event handler */
    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}



/****************************************************************************
 *
 * NAME: ED_vDcfmAssociate
 *
 * DESCRIPTION:
 * Handle the response generated by the stack as a result of the associate
 * start request.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  psMlmeInd
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PUBLIC void ED_vDcfmAssociate(MAC_MlmeDcfmInd_s *psMlmeInd)
{
	uint8 EEPROMSegmentDataLength=0;
	/* Debug */
	DBG_vPrintf(ED_TRACE, "\r\n%d ED < ED_vDcfmAssociate() Status=%d",
		NODE_sData.u32Timer, psMlmeInd->uParam.sDcfmAssociate.u8Status);

    /* If successfully associated with network coordinator */
    if (psMlmeInd->uParam.sDcfmAssociate.u8Status == MAC_ENUM_SUCCESS)
    {
    	if (gWorkingMode == wmPairingMode_c){
			gWorkingMode = wmNormalMode_c;
			if (gConfigData.settings.verbose){
				//printMsg("@Pairing succeeds.\r\n");
				LED1On
			}
    	}
    	/* Note our short address */
    	if (psMlmeInd->uParam.sDcfmAssociate.u16AssocShortAddr== 0xFFFE){
    		//Don't change short address. But it was set to 0xFFFF previously. Reset. Use configured address.
    		NODE_sData.u16Address = gConfigData.settings.shortAddr;
    	}else{
    		NODE_sData.u16Address = psMlmeInd->uParam.sDcfmAssociate.u16AssocShortAddr;
    	}
        /* Network is now up */
        NODE_sData.eNwkState = NODE_NWKSTATE_UP;
        gConfigData.settings.targetAddr = NODE_sData.u16Parent;
		/* Debug */
		//DBG_vPrintf(ED_TRACE, " Short=%#x", psMlmeInd->uParam.sDcfmAssociate.u16AssocShortAddr);

		/* Wuart not running ? */
		if (WUART_sData.u8State == WUART_STATE_NONE)
		{
			/* Put wuart into idle state */
			WUART_vState(WUART_STATE_IDLE);
		}

	    //MAC_vPibSetShortAddr(NODE_sData.pvMac, NODE_sData.u16Address);
		/* Set pan id in pib */
	    //MAC_vPibSetPanId(NODE_sData.pvMac, NODE_sData.u16PanId);
	    /* Enable receiver to be on when idle */
	    //MAC_vPibSetRxOnWhenIdle(NODE_sData.pvMac, TRUE, FALSE);
	    //NODE_sData.psPib->bAssociationPermit = 0;
	    if (gConfigData.settings.deviceType == dtOne2OneSlave){
	    	if (gConfigData.settings.verbose){
	    		wuart_vPrintf(1, "Starting as One2One Slave on channel 0x%x.\r\n",	NODE_sData.u8Channel);
	    	}
			u16AHI_InitialiseEEP(&EEPROMSegmentDataLength);
			iAHI_EraseEEPROMsegment(CONFIG_EEPROM_SECTOR_NUM);
			iAHI_WriteDataIntoEEPROMsegment(CONFIG_EEPROM_SECTOR_NUM,0,gConfigData.data,64);
	    }else{
	    	if (gConfigData.settings.verbose){
	    		wuart_vPrintf(1, "Starting as end-device on channel 0x%x.\r\n",	NODE_sData.u8Channel);
	    	}
	    }
    	if (gConfigData.settings.verbose){
			wuart_vPrintf(1, "Started with PAN ID %#x, short address %#x, parent address %#x\r\n", NODE_sData.u16PanId,	NODE_sData.u16Address, gConfigData.settings.targetAddr);
			//printMsg("Ready to send and receive data.\r\n");
/*
			DBG_vPrintf(1, "evRfLsList_Event\r\n");
			sendEvent(evRfLsList_Event);

			DBG_vPrintf(1, "evRfLsCMD_Event\r\n");
			sendEvent(evRfLsCMD_Event);
			*/
    	}

   	}
	/* Got some channels left to scan ? */
	else if (ED_sData.u32ScanChannels)
	{
		/* Start the next scan */
		ED_vReqActiveScan();
	}
	/* No channels left to scan ? */
	else
	{
		/* Go into rescan state */
		NODE_sData.eNwkState = NODE_NWKSTATE_RESCAN;
	}

    if(gConfigData.settings.deviceType == dtOne2OneSlave){
		//if(!onetooneSlave_Flag){
	    	vAHI_SwReset();
		//}
    }
}



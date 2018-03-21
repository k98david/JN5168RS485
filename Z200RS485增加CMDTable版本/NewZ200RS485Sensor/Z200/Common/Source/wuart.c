/*****************************************************************************/
/*!
 *\MODULE              Wireless UART (Application Layer)
 *
 *\COMPONENT           $HeadURL: https://www.collabnet.nxp.com/svn/lprf_apps/Application_Notes/JN-AN-1069-IEEE-802.15.4-Serial-Cable-Replacement/Tags/Release_3v0-Public/Common/Source/wuart.c $
 *
 *\VERSION			   $Revision: 16555 $
 *
 *\REVISION            $Id: wuart.c 16555 2015-11-20 12:14:37Z nxa04494 $
 *
 *\DATED               $Date: 2015-11-20 12:14:37 +0000 (Fri, 20 Nov 2015) $
 *
 *\AUTHOR              $Author: nxa04494 $
 *
 *\DESCRIPTION         Wireless UART (Application Layer) - implementation.
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

/* Jennic includes */
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <LedControl.h>

/* Stack includes */
#include <dbg.h>
#include <mac_pib.h>

/* Application includes */
#include "stdio.h"
#include "dbg.h"
#include "wuart.h"
#include "lcd.h"
#include "node.h"
#include "queue.h"
#include "rnd.h"
#include "uart.h"
#include "QSensorDataFormat.h"
#include "config.h"
#include "QApp.h"
#include "enddevice.h"
#include "QSensor.h"
#include "QEvent_Process.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/* Positions of various bytes in message packets */
#define WUART_MSG_START 	0
#define WUART_MSG_SEQ_TX   	7
#define WUART_MSG_SEQ_RX   	2
#define WUART_MSG_CMD		3
/* Positions of various bytes in data packet */
#define WUART_DATA_STATUS 	4
#define WUART_DATA_ACK 		5
#define WUART_DATA_SEQ  	6
#define WUART_DATA_DATA		7

#define ZPS_E_BROADCAST_RX_ON 0xffff

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE bool_t WUART_bTxData(void);
PRIVATE bool_t WUART_bStatePaired(uint8 u8State);
uint8  WUART_u8MsgSeqTxNew(void);
PRIVATE uint8  WUART_u8DataSeqTxNew(void);
void sendTestData(uint8 signature);
extern bool ResetModeNoPairkey;
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PUBLIC WUART_tsData	WUART_sData;		/**< Wuart information */

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE uint32			  au32StateTimeout[2][WUART_STATES];	 	 	 /**< State timeouts */
PRIVATE QUEUE_tsData		asQueue[WUART_QUEUES];					 	 /**< Queues */
PRIVATE uint8              au8QueueData[WUART_QUEUES][WUART_QUEUE_DATA]; /**< Queue data buffers */
/* Command strings (4th char must be different in each command) */
uint8			   au8CmdData[NODE_PAYLOAD] = "!  D  "; //This is where packet payload was placed.
PRIVATE uint8 			   au8CmdIdle[]     		= "01234567890";

PRIVATE uint8 			   au8CmdError[] 			= "!  ERROR";
/* Command lengths */
PRIVATE uint8 			    u8CmdData      	= WUART_DATA_DATA;
PRIVATE uint8 			    u8CmdError 	 	= 9;

//PUBLIC	uint8			gWaitUARTPacket = TRUE; // wait until a whole packet from UART is completely received and then send Over the Air
PUBLIC 	uint8			gWaitUARTPacket = TRUE;  //Don't wait too long if there is no further character from UART
PUBLIC	uint8			gWaitDECount=0; //wait until the last byte was transmit out of UART TX line.
PUBLIC 	uint8			gTurn485ReceiveMode = 0; //when this is set, turn DE/RE into low level when gWaitDECount is zero

PUBLIC bool		gDoApplicationResend = FALSE;
PUBLIC uint8	gApplicationResend=0; //control the application resend times
uint16	gPairingPeriodCounter =0;
uint8 gRxdelay_count=10;
/*Mode Bus Data*/
uint8 ModeBusData[ModeBusArrLen]={0};
uint8 ModeBusData96use[ModeBusArrLen]={0};
uint8 ModeBusData96_Count=0;
bool ModeBusData96_Flag=0;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/**
 * <b>WUART_vInit</b> &mdash; Initialise WUART
 */
/****************************************************************************/
PUBLIC void WUART_vInit(void)
{
	uint8_t i;
	/* Debug */
	DBG_vPrintf(WUART_TRACE, "\r\n%d WUART< WUART_vInit()", NODE_sData.u32Timer);

	/* Initialise data */
	WUART_sData.u64Online        = NODE_ADDRESS_NULL;
	WUART_sData.u32StateDuration = 0xffffffff;
	WUART_sData.u16Paired 	     = 0xffff;
	WUART_sData.u8State          = WUART_STATE_NONE;
	WUART_sData.u8Uart           = 0xff;
	WUART_sData.u8Status         = 0;
	WUART_sData.u8Ack            = 0;
	WUART_sData.u8MsgSeqTx       = (uint8) RND_u32GetRand('!', '~');
	WUART_sData.u8MsgSeqRx       = ' ';
	WUART_sData.u8DataSeqTx      = (uint8) RND_u32GetRand('!', '~');
	WUART_sData.u8DataSeqRx      = ' ';

	/* Initiliase state timeouts */
	au32StateTimeout[0][WUART_STATE_NONE	] = 0xffffffff;
	au32StateTimeout[1][WUART_STATE_NONE	] = 0xffffffff;
	au32StateTimeout[0][WUART_STATE_IDLE	] =	512;
	au32StateTimeout[1][WUART_STATE_IDLE	] =	1024;
	au32StateTimeout[0][WUART_STATE_DIAL	] =	64;
	au32StateTimeout[1][WUART_STATE_DIAL	] =	128;
	au32StateTimeout[0][WUART_STATE_DCONNECT] =	64;
	au32StateTimeout[1][WUART_STATE_DCONNECT] =	128;
	au32StateTimeout[0][WUART_STATE_ANSWER	] =	64;
	au32StateTimeout[1][WUART_STATE_ANSWER	] =	128;
	au32StateTimeout[0][WUART_STATE_ACONNECT] =	64;
	au32StateTimeout[1][WUART_STATE_ACONNECT] =	128;
	au32StateTimeout[0][WUART_STATE_ONLINE	] =	512;
	au32StateTimeout[1][WUART_STATE_ONLINE	] =	1024;
	au32StateTimeout[0][WUART_STATE_ACK	 	] =	16;
	au32StateTimeout[1][WUART_STATE_ACK	 	] =	32;

	/* Initialise the queues */
	QUEUE_bInit(&asQueue[WUART_QUEUE_TX], WUART_QUEUE_DATA, WUART_QUEUE_DATA_LOW, WUART_QUEUE_DATA_HIGH, au8QueueData[WUART_QUEUE_TX]);
	QUEUE_bInit(&asQueue[WUART_QUEUE_RX], WUART_QUEUE_DATA, WUART_QUEUE_DATA_LOW, WUART_QUEUE_DATA_HIGH, au8QueueData[WUART_QUEUE_RX]);

	/*
					  WUART_BAUD,
					  WUART_EVEN,
					  WUART_PARITY,
					  WUART_WORDLEN,
					  WUART_ONESTOP,
					  */
	if (UART_bOpen (WUART_UART,
					  gConfigData.settings.baudrate,
					  gConfigData.settings.parityType,
					  gConfigData.settings.parity,
					  gConfigData.settings.dataBit,
					  gConfigData.settings.stopBit,
					  WUART_RTSCTS,
					  WUART_XONXOFF,
					  &asQueue[WUART_QUEUE_TX],
					  &asQueue[WUART_QUEUE_RX]))
	{
		/* Note uart being used */
		WUART_sData.u8Uart = WUART_UART;
		/* Disable uart transmit and receive as we are not paired */
		//UART_bTxDisable(WUART_sData.u8Uart, UART_ABLE_WUART_PAIR);
		//UART_bRxDisable(WUART_sData.u8Uart, UART_ABLE_WUART_PAIR);
	}

    switch(gConfigData.settings.baudrate){    //for Rx Fifo 4 Byte Time_out Count use to over Uart_int_time_out
		case 1200:    //1200
			gRxdelay_count=96;
			break;
		case 2400:    //38400
			gRxdelay_count=48;
			break;
		case 4800:    //38400
			gRxdelay_count=24;
			break;
		case 9600:    //38400
			gRxdelay_count=12;
			break;
		case 19200:    //38400
			gRxdelay_count=6;
			break;
		case 38400:    //38400
			gRxdelay_count=6;
			break;
		case 57600:    //57600
			gRxdelay_count=3;
			break;
		case 115200:    //115200
			gRxdelay_count=3;
			break;
    }

	//=== Init packet ===
	au8CmdData[MAC_POS]=0;
	au8CmdData[MAC_POS+1]=0x05;
	au8CmdData[TARGET_POS]=(uint64)gConfigData.settings.targetAddr>>8;
	au8CmdData[TARGET_POS+1]=(uint64)gConfigData.settings.targetAddr;
	au8CmdData[SRC_POS]=NODE_sData.u16Address>>8;
	au8CmdData[SRC_POS+1]=NODE_sData.u16Address;
	au8CmdData[6]=0x00;
	au8CmdData[SEQ_POS]=0xFF;
	//au8CmdData[RADIUS_POS]=0x31;
	gApplicationResend = 0;

	//=== Init PSW CMD packet ===
	PswCmdData[MAC_POS]=1;
	PswCmdData[MAC_POS+1]=0x05;
	PswCmdData[TARGET_POS]=0xFF;
	PswCmdData[TARGET_POS+1]=0xFF;
	PswCmdData[SRC_POS]=0x12;
	PswCmdData[SRC_POS+1]=0x34;
	PswCmdData[6]=0x00;
	PswCmdData[SEQ_POS]=0xFF;
	//au8CmdData[RADIUS_POS]=0x31;
	gApplicationResend = 0;

	//=== Init sensor packet ===
	sensor_RF_Packet[MAC_POS]=0;
	sensor_RF_Packet[MAC_POS+1]=0x05;
	sensor_RF_Packet[TARGET_POS]=(uint64)gConfigData.settings.targetAddr>>8;
	sensor_RF_Packet[TARGET_POS+1]=(uint64)gConfigData.settings.targetAddr;
	sensor_RF_Packet[SRC_POS]=NODE_sData.u16Address>>8;
	sensor_RF_Packet[SRC_POS+1]=NODE_sData.u16Address;
	sensor_RF_Packet[6]=0x00;
	sensor_RF_Packet[SEQ_POS]=0xFF;
	sensor_RF_Packet[Time_POS]=0x00;
	sensor_RF_Packet[Address_POS]=0x00;
	sensor_RF_Packet[Type_POS]=0x00;
	sensor_RF_Packet[DataSeQ_POS]=0x00;
	sensor_RF_Packet[Length_POS]=0x00;
}

/****************************************************************************/
/**
 * <b>WUART_vTick</b> &mdash; Called regularly for timing purposes
 *
 * Samples the uart receive buffer and transmit any data over radio
 *
 * Samples the queue's fill levels and activates radio flow control if required
 *
 * Monitors WUART states for time out conditions
 *
 * Updates LCD if used
 */
/****************************************************************************/
uint16 kk=0;
uint16 testCounter=0,TempRx=0;
PUBLIC void WUART_vTick(void)
{
	//DBG_vPrintf(WUART_TRACE, "\r\n%d WUART> WUART_vTick() Timeout state=%d", NODE_sData.u32Timer, WUART_sData.u8State);

	vAHI_WatchdogRestart();

	if (gSerialMode == TransparentMode_c){
		if (NODE_sData.u32Timer - WUART_sData.u32StateStart > WUART_sData.u32StateDuration)
		{
			/* Set sequence */
			//au8CmdIdle[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
			//au8CmdIdle[WUART_MSG_SEQ_RX] = ' ';
			/* Re-enter idle state, broadcast idle request */
			//WUART_vState(WUART_STATE_IDLE); //Only reset timer
			//WUART_vTx((uint64) ZPS_E_BROADCAST_RX_ON, u8CmdIdle, au8CmdIdle);
			WUART_sData.u32StateStart = NODE_sData.u32Timer;
			WUART_sData.u32StateDuration = 1200;
			//WUART_vTx(gConfigData.settings.targetAddr, u8CmdIdle, au8CmdIdle);
			//DBG_vPrintf(TRUE, "\r\n===vTick===");
		}
	}

#ifdef CTSRTS
	if (u32AHI_DioReadInput() & (1<<RTSpin)){
		vAHI_DioSetOutput(1<<LED1pin, 0); //
		gRTS=1;
	}else{
		vAHI_DioSetOutput(0, 1<< LED1pin); //set CTS low
		gRTS=0;
	}

#endif
	/* Start transmit from uart if appropriate */
	UART_vStartTx(WUART_sData.u8Uart);
	/* Send data packet if appropriate */
	//LED2Off
	if (gDoApplicationResend ){
		DBG_vPrintf(WUART_TRACE, "\r\n%d WUART< Resend() gApplic=%d", NODE_sData.u32Timer,gApplicationResend);
		if (gConfigData.settings.targetAddr == 0xFFFF){
			WUART_vTx(0xFFFF, u8CmdData, au8CmdData);
			if (gApplicationResend>0) {
				//gDoApplicationResend=TRUE;
				gApplicationResend--;
			} else {
				gDoApplicationResend=FALSE;
			}
		}else{
			uint64 targetAddr =gConfigData.settings.targetAddr;
			DBG_vPrintf(WUART_TRACE, "\r\n%d WUART< target= %#x gApplic=%d", NODE_sData.u32Timer, targetAddr,gApplicationResend);
			WUART_vTxAck(targetAddr, u8CmdData, au8CmdData);
			gDoApplicationResend=FALSE;
			if (gApplicationResend>0) {
			    gApplicationResend--;
			}
		}
		//gApplicationResend--;

	}else{
		//TempRx=QUEUE_u16Used(&asQueue[WUART_QUEUE_RX]);
		//DBG_vPrintf(WUART_TRACE, "asQueue[WUART_QUEUE_RX] %x \r\n ",TempRx);
		//DBG_vPrintf(WUART_TRACE, "gWaitUARTPacket %x \r\n ",gWaitUARTPacket);
		//DBG_vPrintf(WUART_TRACE, "L \r\n ");
		if (((QUEUE_bEmpty(&asQueue[WUART_QUEUE_RX])==FALSE)&&(!gWaitUARTPacket))||(QUEUE_u16Used(&asQueue[WUART_QUEUE_RX])>88)){
		//if (QUEUE_bEmpty(&asQueue[WUART_QUEUE_RX])==FALSE){
			//LED1Off
			//LED1On
			DBG_vPrintf(WUART_TRACE, "tick%d \r\n", QUEUE_u16Used(&asQueue[WUART_QUEUE_RX]));
			if (gSerialMode == TransparentMode_c){
				gApplicationResend = gConfigData.settings.applicationResend;
				WUART_sData.u32StateStart = NODE_sData.u32Timer;
				WUART_sData.u32StateDuration = 1200;
			}
			gWaitUARTPacket=TRUE; //clear to prevent re-entrance
			gCheckRXTimeout = FALSE;
			gCheckTimeoutCounter=gRxdelay_count;
			WUART_bTxData();

			TestCount=TRUE;
		}else{
			gTurn485ReceiveMode=1;
		}
	}
	//LED2On
	if (kk>=30){
		uint32 bitmap=0;
	 //LED2Off
		 kk=0;
		 if (gWorkingMode == wmPairingMode_c){
			 if (u32AHI_DioReadInput() & (1<< LED1pin)){
				 LED1Off
			 }else{
				 LED1On
			 }
			 if (gPairingPeriodCounter >=120){   //32's
				 gWorkingMode = wmNormalMode_c;
				 DBG_vPrintf(1,"Pairing fail\r\n");
				 if (gConfigData.settings.verbose){
					// printMsg("@Pairing fails.\r\n");
				 }
			 }else{
				 gPairingPeriodCounter++;
			 }
#ifdef SendTestUse
		     gApplicationResend = gConfigData.settings.applicationResend;
			 testCounter++;
			 testCounter= testCounter %10;
			 sendTestData(testCounter);
#endif
		 }
	 //LED2On
	}else{
		if (gFlashDataLED){
			if (kk==29){
			//if (dataLEDCounter >60){
				 if (u32AHI_DioReadInput() & (1<< LED2pin)){
					 LED2Off
				 }else{
					 LED2On
				 }
				 //dataLEDCounter=0;
				 dataLEDDuration++;
				if (dataLEDDuration>6){
					gFlashDataLED=FALSE;
					dataLEDDuration=0;
					 LED2On
				}
			}else{
				//dataLEDCounter++;
			}
		}

		if (gFlashDataLEDG){
			if (kk==29){
			//if (dataLEDCounter >60){
				 if (u32AHI_DioReadInput() & (1<< LED1pin)){
					 LED1Off
				 }else{
					 LED1On
				 }
				 //dataLEDCounter=0;
				 dataLEDDuration++;
				if (dataLEDDuration>6){
					gFlashDataLEDG=FALSE;
					dataLEDDuration=0;
					 LED1On
				}
			}else{
				//dataLEDCounter++;
			}
		}
		kk++;
	}
/*
	if(!(u32AHI_DioReadInput() & (1<< KEY0pin))){
		gKey0Pressed = TRUE;
	    //DBG_vPrintf(WUART_TRACE, "\r\n gKey0Pressed %x", gKey0Pressed);
	}
	if (gKey0Pressed&& ResetModeNoPairkey!=FALSE){
		if (u32AHI_DioReadInput() & (1<< KEY0pin)){
			gKey0Pressed = FALSE;
			gKey0DebounceCount=0;
		    DBG_vPrintf(WUART_TRACE, "\r\n gKey0false %x", gKey0Pressed);
		}else{
			if (gKey0DebounceCount>140){
				DBG_vPrintf(WUART_TRACE, "\r\n wmPairingMode_c");
				gWorkingMode = wmPairingMode_c;
				gPairingPeriodCounter=0;
				if (gConfigData.settings.deviceType == dtOne2OneSlave){
			    	ED_vReqActiveScan();
				}
				 if (gConfigData.settings.verbose){
					 printMsg("Enter Paring mode.\r\n");
				 }
				gKey0Pressed=FALSE;
				gKey0DebounceCount=0;
				testCounter=0;
			}else{
				gKey0DebounceCount++;
			}
		}
	}
*/
}

/****************************************************************************/
/**
 * <b>WUART_bRxEnabled</b> &mdash; Returns if radio receive is enabled.
 */
/****************************************************************************/
PUBLIC bool_t WUART_bRxEnabled (void)
{
	return (WUART_sData.u8Status & WUART_STATUS_RX_ENABLED) ? TRUE : FALSE;
}

/****************************************************************************/
/**
 * <b>WUART_bTxEnabled</b> &mdash; Returns if radio transmit is enabled.
 */
/****************************************************************************/
PUBLIC bool_t WUART_bTxEnabled (void)
{
	return (WUART_sData.u8Status & WUART_STATUS_TX_ENABLED) ? TRUE : FALSE;
}

/****************************************************************************/
/**
 * <b>WUART_vRx</b> &mdash; Handles data received over radio
 */
/****************************************************************************/
PUBLIC void WUART_vRxQ(uint64 u64RxAddr, uint16 u16RxSize, uint8 *pu8RxPayload)
{
	bool_t   bHandled = FALSE;	/**< Assume we won't handle the message */
	uint16 u16Pos;
    uint8 y=0,x=0,z=0,h=0;
	//uint8 STM51[7]={0x41,0x54,0x54,0x58,0x3D,0x46,0x64};
	uint8 STM32[4]={"ATX="};
	uint8 OutData[NODE_PAYLOAD]={0};
	uint16 AddAddress=0;
	char Hex2ASCII[10]={0};
	char Hex2ASCII2[10]={0};
	char *Temp;

#if Sensor_Receiver
		char str[30]="Fd sdata eata678901234567890abcde";
		str[2]= 22;//packet langth
		str[POS_SourceAddress]= pu8RxPayload[11];
		str[POS_SourceAddress+1]= pu8RxPayload[12];
		str[POS_PacketType]=0;
		str[POS_Serial]= pu8RxPayload[14];
		str[POS_SensorType] = 0x01;
		str[POS_SensorType+1] = 0x02; //temperature& Humidity sensor
		str[POS_Status] = pu8RxPayload[10];
		str[POS_SensorValue+5]=0x0D;
		str[POS_SensorValue+6]=0x0A;


		memcpy(str + POS_SensorValue, pu8RxPayload+15,5);
		/* Loop through received data */
		for (u16Pos = 0; u16Pos < 25; u16Pos++)
		//for (u16Pos = WUART_DATA_DATA; u16Pos < u16RxSize-1; u16Pos++)
		{
			/* Get uart to transmit this byte */
			UART_bPutChar(WUART_sData.u8Uart, str[u16Pos]);
			//memcpy(str, pu8RxPayload+10, )
			//UART_bPutChar(WUART_sData.u8Uart, pu8RxPayload[u16Pos]);
		}
		UART_vStartTx(WUART_sData.u8Uart);
#endif
#if Z200
		/* Is this a repeated message from the node we are online with ? */
		if (WUART_sData.u64Online  != u64RxAddr || WUART_sData.u8MsgSeqRx != pu8RxPayload[7])
		{
			//DBG_vPrintf(WUART_TRACE, "\r\n== Bingo ==");
			if (pu8RxPayload[0] == 0x00){ //this is a data packet. Indicated by the first byte of Network Layer header.

				memcpy(OutData,STM32,4);

				if (strncmp(&pu8RxPayload[8],"LIST",4)==0){
					memcpy(&OutData[6],"LIST#",5);
				}else if(strncmp(&pu8RxPayload[8],"CMDT",4)==0){
					memcpy(&OutData[6],"CMDT#",5);
				}else if(strncmp(&pu8RxPayload[8],"DATA",4)==0){
					memcpy(&OutData[6],"DATA#",5);
				}

				//OutData[9]= pu8RxPayload[4]; //source addresss LO
				//OutData[10]= pu8RxPayload[5]; //source addresss HI

		        for(z=0;z<=1;z++){
		             Hex2ASCII[x++]=convertHex2ASCII_HighNibble(pu8RxPayload[z+4]);
		             Hex2ASCII[x++]=convertHex2ASCII_LowNibble(pu8RxPayload[z+4]);
		        }
		        x=0;
				memcpy(&OutData[11],Hex2ASCII,4);   //Source Address to ASCII 4 Byte

				for (u16Pos = 12; u16Pos < u16RxSize; u16Pos++)
				{
					OutData[u16Pos+3]=pu8RxPayload[u16Pos];
				}

		        for(z=0;z<=1;z++){
		        	OutData[4]=convertHex2ASCII_HighNibble(u16RxSize-5);
		        	OutData[5]=convertHex2ASCII_LowNibble(u16RxSize-5);
		        }
/*
				for(y=0;y<=u16RxSize+2;y++){
					DBG_vPrintf(1, "OutData[%d]=%02x\r\n", y,OutData[y]);
				}
*/
				for (u16Pos = 0; u16Pos <=u16RxSize+2; u16Pos++)
				{
					UART_bPutChar(WUART_sData.u8Uart, OutData[u16Pos]);
				}

				UART_vStartTx(WUART_sData.u8Uart);

/*
		        for(z=13;z<=u16RxSize-2;z++){
		             Hex2ASCII[x++]=convertHex2ASCII_HighNibble(OutData[z]);
		             Hex2ASCII[x++]=convertHex2ASCII_LowNibble(OutData[z]);
		        }

		        Hex2ASCII[x]=0x0D;
		        Hex2ASCII[x+1]=0x0A;

				memcpy(&OutData[13],Hex2ASCII,x+2);   //Source Address to ASCII 4 Byte

				for (u16Pos = 0; u16Pos < x+2+13; u16Pos++)
				{
					UART_bPutChar(WUART_sData.u8Uart, OutData[u16Pos]);
				}

				UART_vStartTx(WUART_sData.u8Uart);
*/
				/*
				memcpy(OutData,STM32,4);

				OutData[5]= pu8RxPayload[4]; //source addresss LO
				OutData[6]= pu8RxPayload[5]; //source addresss HI
				OutData[7]= pu8RxPayload[2]; //Dest addresss LO
				OutData[8]= pu8RxPayload[3]; //Dest addresss HI
				OutData[4]=u16RxSize-4;  //len

				for (u16Pos = 8; u16Pos < u16RxSize; u16Pos++)
				{
					OutData[u16Pos+1]=pu8RxPayload[u16Pos];
				}
				//OutData[u16Pos+1]=0x0D;
				//OutData[u16Pos+2]=0x0A;
				//OutData[12]=0x01;

				DBG_vPrintf(1, "OutData \r\n");
				for(y=0;y<=u16RxSize;y++){
					DBG_vPrintf(1, "%02x\r\n", OutData[y]);
				}

		        for(z=4;z<=u16RxSize;z++){
		             Hex2ASCII[x++]=convertHex2ASCII_HighNibble(OutData[z]);
		             Hex2ASCII[x++]=convertHex2ASCII_LowNibble(OutData[z]);
		        }
		        Hex2ASCII[x]=0x0D;
		        Hex2ASCII[x+1]=0x0A;
				DBG_vPrintf(1, "OutDataHex \r\n");
				for(y=0;y<=x+2;y++){
					DBG_vPrintf(1, "%02x\r\n", Hex2ASCII[y]);
				}

				memcpy(Hex2ASCII,STM32,4);

				for (u16Pos = 0; u16Pos < x+2; u16Pos++)
				{
					UART_bPutChar(WUART_sData.u8Uart, Hex2ASCII[u16Pos]);
				}

				UART_vStartTx(WUART_sData.u8Uart);
*/
/*
				wuart_vPrintf(1, "ATX=%d%x%x%x%x,",u16RxSize-4,pu8RxPayload[4],pu8RxPayload[5],pu8RxPayload[2],pu8RxPayload[3]);

				pu8RxPayload[u16RxSize]=0x0D;
				pu8RxPayload[u16RxSize+1]=0x0A;
				for (u16Pos = 8; u16Pos < u16RxSize+2; u16Pos++)
				{
					UART_bPutChar(WUART_sData.u8Uart, pu8RxPayload[u16Pos]);
				}
				UART_vStartTx(WUART_sData.u8Uart);
*/
			}else if(pu8RxPayload[0] == 0x01){    //for RF PSW CMD
				PSW_CMD_flag=TRUE;
				parseCommand(pu8RxPayload+8, u16RxSize);
			}else if(pu8RxPayload[0] == 0x03){    //for Reper
/*
				memcpy(OutData,HRhead,2);
				if(pu8RxPayload[8]==0){
					OutData[3]= pu8RxPayload[4]; //source addresss LO
					OutData[4]= pu8RxPayload[5]; //source addresss HI
					OutData[5]= pu8RxPayload[2]; //Dest addresss LO
					OutData[6]= pu8RxPayload[3]; //Dest addresss HI
					OutData[2]=u16RxSize-4;  //len
					for (u16Pos = 8; u16Pos < u16RxSize; u16Pos++)
					{
						OutData[u16Pos-1]=pu8RxPayload[u16Pos];
					}

					for (u16Pos = 0; u16Pos < u16RxSize-1; u16Pos++)
					{
						UART_bPutChar(WUART_sData.u8Uart, OutData[u16Pos]);
					}
				}else if(pu8RxPayload[8]==2){    //show Add Data
					for (u16Pos = 8; u16Pos < u16RxSize; u16Pos++)
					{
						UART_bPutChar(WUART_sData.u8Uart, pu8RxPayload[u16Pos]);
					}
					*/
			}
		}
		WUART_sData.u64Online  = u64RxAddr;
		WUART_sData.u8MsgSeqRx = pu8RxPayload[7];
#endif
	//}

	/* Didn't handle message above ? */
	//if (bHandled == FALSE)
	if ( FALSE) //m by Hodge
	{
		/* Set sequence */
		au8CmdError[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
		au8CmdError[WUART_MSG_SEQ_RX] = pu8RxPayload[WUART_MSG_SEQ_TX];;
		/* Not in idle state and from node we are online with ? */
		if (WUART_sData.u8State != WUART_STATE_IDLE && u64RxAddr == WUART_sData.u64Online)
		{
			/* Return to idle state transmit error */
			WUART_vState(WUART_STATE_IDLE);
		}
		/* Transmit error response */
		WUART_vTx(u64RxAddr, u8CmdError, au8CmdError);
	}

}


/****************************************************************************/
/**
 * <b>WUART_vTx</b> &mdash; Transmit WUART data message over the radio, adds message sequence
 */
/****************************************************************************/
PUBLIC void WUART_vTx(uint64 u64TxAddr, uint16 u16TxSize, uint8 *pu8TxPayload)
{
	/* Got data and address to transmit ? */
	if (pu8TxPayload != (uint8 *) NULL && u64TxAddr != NODE_ADDRESS_NULL)
	{
		/* Length indicates a string - get data length */
		if (u16TxSize == 0xffff) u16TxSize = strlen((char *) pu8TxPayload)+1;
		DBG_vPrintf(Debug, "WUART_vTxData()");

		/* Debug*/
		/*
		DBG_vPrintf(WUART_TRACE, "\r\n%d WUART< WUART_vTx(%x:%x %d %s)",
			NODE_sData.u32Timer,
			(uint32)(u64TxAddr >> 32),
			(uint32)(u64TxAddr & 0xffffffff),
			u16TxSize,
			pu8TxPayload);
*/
		/* Got data ? */
		if (u16TxSize > 0)
		{
			/* Send data */
			NODE_vTx(u64TxAddr, u16TxSize, (uint8 *) pu8TxPayload); //m by Hodge, no Ack
		}
	}
}

PUBLIC void WUART_vTxAck(uint64 u64TxAddr, uint16 u16TxSize, uint8 *pu8TxPayload)
{
	/* Got data and address to transmit ? */
	if (pu8TxPayload != (uint8 *) NULL && u64TxAddr != NODE_ADDRESS_NULL)
	{
		/* Length indicates a string - get data length */
		if (u16TxSize == 0xffff) u16TxSize = strlen((char *) pu8TxPayload)+1;

		/* Debug */
		DBG_vPrintf(WUART_TRACE, "\r\n%d WUART< WUART_vTxAck(%x:%x %d %s)",
			NODE_sData.u32Timer,
			(uint32)(u64TxAddr >> 32),
			(uint32)(u64TxAddr & 0xffffffff),
			u16TxSize,
			pu8TxPayload);

		/* Got data ? */
		if (u16TxSize > 0)
		{
			/* Send data */
			NODE_vTxAck(u64TxAddr, u16TxSize, (uint8 *) pu8TxPayload); //m by Hodge
		}
	}
}

/****************************************************************************/
/**
 * <b>WUART_vState</b> &mdash; Sets state for WUART, sets timers, transmits associated message
 */
/****************************************************************************/
PUBLIC void WUART_vState(uint8   u8State)
{
	/* Valid state ? */
	if (u8State < WUART_STATES)
	{
		/* Debug */
		DBG_vPrintf(WUART_TRACE, "\r\n%d WUART< WUART_vState(%d) old=%d", NODE_sData.u32Timer, u8State, WUART_sData.u8State);

		/* Invalidate state timeout to prevent timeouts while processing the new state */
		WUART_sData.u32StateDuration = 0xffffffff;

		/* State different ? */
		//if (0)  //m by hodge. This is useless in Z200 application
		if (WUART_sData.u8State != u8State)
		{
			/* Have we just paired ? */
			if (WUART_bStatePaired(u8State)             == TRUE &&
				WUART_bStatePaired(WUART_sData.u8State) == FALSE)
			{
				/* Enable transmit and receive as we are now paired */
				UART_bTxEnable(WUART_sData.u8Uart, UART_ABLE_WUART_PAIR);
				UART_bRxEnable(WUART_sData.u8Uart, UART_ABLE_WUART_PAIR);

				/* Invalidate expected receive sequence */
				WUART_sData.u8DataSeqRx = ' ';
			}
			/* Have we just unpaired ? */
			else if (WUART_bStatePaired(u8State)             == FALSE &&
				     WUART_bStatePaired(WUART_sData.u8State) == TRUE)
			{
				/* Disable transmit and receive as we are no longer paired */
				UART_bTxDisable(WUART_sData.u8Uart, UART_ABLE_WUART_PAIR);
				UART_bRxDisable(WUART_sData.u8Uart, UART_ABLE_WUART_PAIR);
				/* Invalidate online number */
				WUART_sData.u64Online = NODE_ADDRESS_NULL;
			}
			/* Note new state */
			WUART_sData.u8State = u8State;
		}

		/* Debug */
		//DBG_vPrintf(WUART_TRACE, " tx=%d rx=%d", UART_bTxEnabled(WUART_sData.u8Uart), UART_bTxEnabled(WUART_sData.u8Uart));

		/* Note time of entry to state */
		WUART_sData.u32StateStart = NODE_sData.u32Timer;

		/* Random duration ? */
		if (au32StateTimeout[1][WUART_sData.u8State] > au32StateTimeout[0][WUART_sData.u8State])
		{
			/* Set random duration */
			WUART_sData.u32StateDuration = RND_u32GetRand(au32StateTimeout[0][WUART_sData.u8State], au32StateTimeout[1][WUART_sData.u8State]);
		}
		/* Fixed duration ? */
		else
		{
			/* Set fixed duration */
			WUART_sData.u32StateDuration = au32StateTimeout[0][WUART_sData.u8State];
		}
	}
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/**
 * <b>WUART_bTxData</b> &mdash; Check if we should transmit data packet, build if required
 */
/****************************************************************************/
PRIVATE bool_t WUART_bTxData(void)
{
	bool_t bTxData  = FALSE;
	//uint8 u8TxState = WUART_STATE_NONE;
	uint8 got1B = 0;
    uint8 x=0,y=0;

		u8CmdData 	=  8; //The first 8 data is Network layer Header
		//u8CmdData 	=  WUART_DATA_DATA;
		while (QUEUE_bEmpty(&asQueue[WUART_QUEUE_RX])  == FALSE)
		{
			uint8 u8Char;
			bool_t bGetChar = TRUE;
			bGetChar = UART_bGetChar(WUART_sData.u8Uart, (char *) &u8Char);
			if (bGetChar) {
				switch (got1B){
				case  0:
					if (u8Char == 0x1B) {
						got1B = 1;
					}else{
						got1B = 3;  //not a enter config command
					}
					break;
				case 1:
					if (u8Char == 0x1B) {
						got1B = 2;
					}else{
						got1B = 3; //not a enter config command
					}
					break;
				case 2:
					if (u8Char == 0x1B) {
						got1B = 3;
					}else{
						got1B = 3; //not a enter config command
					}
					break;
				default:
					break;  //not a enter config command
				}
				au8CmdData[u8CmdData++] = u8Char;
			}
		}
		au8CmdData[u8CmdData]='\0';
		//DBG_vPrintf(WUART_TRACE, "\r\nWUART< au8CMDData %s.",au8CmdData);
		if (got1B == 2){
			gSerialMode = CommandMode_c;
			printMsg("Config mode entered.\r\n");
			DBG_vPrintf(WUART_TRACE, "\r\nWUART< Config mode entered.");
			return bTxData;
		}

		if(sensor_mode==EnableMode_c){   //if in sensor mode don't use sendRF put data to log buffer

			DBG_vPrintf(WUART_TRACE, "\r\n UartLen=%d\r\n",u8CmdData-8);

			if((au8CmdData[9]&0x80)!=1){  // not erro CMD
				if((u8CmdData-8)==(au8CmdData[10]+5)&&(!ModeBusData96_Flag)){    //check Modbus len Right     if Uart len < 96 use
					memcpy(ModeBusData,&au8CmdData[8],(au8CmdData[10]+5));   //for cut to sList

					Sensor485State=ReciveData;

					if(RS485CMD_Count_Flag){   //for use delay CMD count
						RS485CMD_Count_Timer_Flag=TRUE;
						RS485CMD_Count_Flag=FALSE;
					}

					DBG_vPrintf(WUART_TRACE, "\r\n sensorDataLen=%d\r\n",au8CmdData[10]);

					for(y=0;y<au8CmdData[10]+3;y++){
						DBG_vPrintf(Debug, "%02x", ModeBusData[y]);
					}
					gFlashDataLED=TRUE;
					sendEvent(evUpdate_DataEvent);
				}else if(((u8CmdData-8)==96)||ModeBusData96_Flag){   //if Uart Len > 96
					ModeBusData96_Flag=TRUE;    //USE switch
					memcpy(&ModeBusData96use[ModeBusData96_Count],&au8CmdData[8],u8CmdData-8);
					DBG_vPrintf(WUART_TRACE, "\r\n ModeBusData96_Count=%d\r\n",ModeBusData96_Count);
					ModeBusData96_Count=(u8CmdData-8)+ModeBusData96_Count;
					DBG_vPrintf(WUART_TRACE, "\r\n ModeBusData96_Count[2]=%d\r\n",ModeBusData96use[2]);
					if(ModeBusData96_Count==(ModeBusData96use[2]+5)){    //check Modbus len Right
						DBG_vPrintf(WUART_TRACE, "\r\n ModeBusData96_Countif=%d\r\n",ModeBusData96_Count);
						Sensor485State=ReciveData;

						if(RS485CMD_Count_Flag){   //for use delay CMD count
							RS485CMD_Count_Timer_Flag=TRUE;
							RS485CMD_Count_Flag=FALSE;
						}
						for(y=0;y<ModeBusData96use[2]+3;y++){
							DBG_vPrintf(Debug, "%02x", ModeBusData96use[y]);
						}
						memcpy(ModeBusData,ModeBusData96use,ModeBusData96_Count);   //for cut to sList
						ModeBusData96_Count=0;
						gFlashDataLED=TRUE;
						sendEvent(evUpdate_DataEvent);
						ModeBusData96_Flag=FALSE;    //USE switch
					}

				}
			}
		}
		/* Data transmit packet is not empty and
		   and we are allowed to transmit */
		if(sensor_mode!=EnableMode_c){
			if (gSerialMode == TransparentMode_c)
			{
				/* Debug */
				DBG_vPrintf(WUART_TRACE, "\r\n%d WUART< WUART_vTxData(), target=%#x", NODE_sData.u32Timer, gConfigData.settings.targetAddr);
				/* Set message sequence */
				//au8CmdData[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
				au8CmdData[SEQ_POS] = WUART_u8MsgSeqTxNew();
				/* Set current status, ack values for this packet */
				//au8CmdData[WUART_DATA_STATUS] = WUART_sData.u8Status + '0';
				//au8CmdData[WUART_DATA_ACK] 	  = WUART_sData.u8Ack    + '0';
				/* Clear our outstanding acks if the ack fails will get a resend anyway */
				//WUART_sData.u8Ack = 0;
				/* Change to state and transmit */
				//WUART_vState(u8TxState);
				//WUART_sData.u64Online=0x8812;
				if (gConfigData.settings.targetAddr == 0xFFFF){
					WUART_vTx(0xFFFF, u8CmdData, au8CmdData);
					DBG_vPrintf(WUART_TRACE, "WUART_vTxData() gApplic=%d TXseq=%d", gApplicationResend,au8CmdData[SEQ_POS]);
					if (gApplicationResend>0){
						gDoApplicationResend = TRUE;
						gApplicationResend--;
					}
				}else{
					uint64 targetAddr = (uint64)gConfigData.settings.targetAddr;
					WUART_vTxAck(targetAddr, u8CmdData, au8CmdData);
					//WUART_vTx(targetAddr, u8CmdData, au8CmdData);
				}
				bTxData = TRUE;
			}else{
				parseCommand(au8CmdData+8, u8CmdData-8);
				//DBG_vPrintf(WUART_TRACE, "\r\n%d WUART< parseData() %s", NODE_sData.u32Timer, au8CmdData+8);

			}
		}
	return bTxData;
}


/****************************************************************************/
/**
 * <b>WUART_bStatePaired</b> &mdash; Returns if node is paired in specified state
 */
/****************************************************************************/
PRIVATE bool_t WUART_bStatePaired(uint8 u8State)
{
	return ((1 << u8State) & WUART_STATES_PAIRED) ? TRUE : FALSE;
}

/****************************************************************************/
/**
 * <b>WUART_u8MsgSeqTxNew</b> &mdash; Generates and returns new message transmit sequence number
 */
/****************************************************************************/
uint8 WUART_u8MsgSeqTxNew(void)
{
	/* Increment sequence */
	WUART_sData.u8MsgSeqTx++;
	/* Invalid ? */
	if (WUART_sData.u8MsgSeqTx < '!' || WUART_sData.u8MsgSeqTx > '~')
	{
		/* Wrap to first valid value */
		WUART_sData.u8MsgSeqTx = '!';
	}

	return WUART_sData.u8MsgSeqTx;
}

/****************************************************************************/
/**
 * <b>WUART_u8DataSeqTxNew</b> &mdash; Generates and returns new message transmit sequence number
 */
/****************************************************************************/
PRIVATE uint8 WUART_u8DataSeqTxNew(void)
{
	/* Increment sequence */
	WUART_sData.u8DataSeqTx++;
	/* Invalid ? */
	if (WUART_sData.u8DataSeqTx < '!' || WUART_sData.u8DataSeqTx > '~')
	{
		/* Wrap to first valid value */
		WUART_sData.u8DataSeqTx = '!';
	}

	return WUART_sData.u8DataSeqTx;
}

void initRFPacket(){
	au8CmdIdle[8]=0x12;
	au8CmdIdle[9]=0x96;
	au8CmdIdle[0]=0x01;
	au8CmdIdle[1]=0;
	au8CmdIdle[2]=0xFF;
	au8CmdIdle[3]=0xFF;
	au8CmdIdle[4]=gConfigData.settings.shortAddr >>8;
	au8CmdIdle[5]=gConfigData.settings.shortAddr &0xFF;

}


void sendTestData(uint8 signature){
	{
		bool_t bTxData  = FALSE;
		//uint8 u8TxState = WUART_STATE_NONE;
		uint8 got1B = 0;

			u8CmdData 	=  8; //The first 8 data is Network layer Header
			for (u8CmdData=8; u8CmdData<45; u8CmdData++){
				au8CmdData[u8CmdData]=signature+0x30;
			}
			if (signature %2 ){
				au8CmdData[u8CmdData++]=0x0D;
				au8CmdData[u8CmdData++]=0x0A;
			}
			/* Data transmit packet is not empty and
			   and we are allowed to transmit */
			if (gSerialMode == TransparentMode_c)
			{
				/* Debug */
				DBG_vPrintf(WUART_TRACE, "\r\n%d WUART< sendTestData(), target=%#x", NODE_sData.u32Timer, gConfigData.settings.targetAddr);
				/* Set message sequence */
				//au8CmdData[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
				au8CmdData[SEQ_POS] = WUART_u8MsgSeqTxNew();
				/* Set current status, ack values for this packet */
				//au8CmdData[WUART_DATA_STATUS] = WUART_sData.u8Status + '0';
				//au8CmdData[WUART_DATA_ACK] 	  = WUART_sData.u8Ack    + '0';
				/* Clear our outstanding acks if the ack fails will get a resend anyway */
				//WUART_sData.u8Ack = 0;
				/* Change to state and transmit */
				//WUART_vState(u8TxState);
				//WUART_sData.u64Online=0x8812;
				if (gConfigData.settings.targetAddr == 0xFFFF){
					WUART_vTx(0xFFFF, u8CmdData, au8CmdData);
					if (gApplicationResend>0){
						gDoApplicationResend = TRUE;
						gApplicationResend--;
					}
				}else{
					uint64 targetAddr = (uint64)gConfigData.settings.targetAddr;
					WUART_vTxAck(targetAddr, u8CmdData, au8CmdData);
				}
				bTxData = TRUE;
			}

	}


}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/


/* Jennic includes */
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <string.h>

/* Application includes */
#include "dbg.h"
#include "node.h"
#include "queue.h"
#include "uart.h"
#include "QApp.h"
#include "wuart.h"
#include "config.h"
#include "Qsensor.h"
#include "QEvent_Process.h"

ConfigData_t gConfigData;
SerialMode_t gSerialMode  = TransparentMode_c;
CommandType_t CommandList[MAX_CMD_NUMBER];
PUBLIC bool gKey0Pressed = FALSE; //variable for key debouncing
PUBLIC uint8 gKey0DebounceCount=0;
PUBLIC bool gKey1Pressed = FALSE; //variable for key debouncing
PUBLIC bool gFlashDataLED = FALSE;
PUBLIC bool gFlashDataLEDG = FALSE;
PRIVATE bool gGotCR = FALSE;

uint8 PswCmdData[150];
PRIVATE uint8 gCmdIndex=0;

PUBLIC uint8 gKey0ResetCount=0;
PUBLIC uint8 gKey0Count=0;
PUBLIC uint8 gKey1Count=0;

bool ResetGledMode=FALSE;
bool ResetModeNoPairkey=FALSE;
uint8 ResetLEDCounter=0;
bool_t gCheckRXTimeout = FALSE;
uint8 gCheckTimeoutCounter =0;
PUBLIC uint8 dataLEDDuration =0;
uint8 ATSLen =0;

extern bool Timer0_Watchdog_flag;
uint8 convertASCII2HexByte(char * aWord);

PRIVATE void ISR_handleSysCtrl(uint32 u32Device, 	 	/**< Interrupting device */
					         uint32 u32ItemBitmap); 	/**< Interrupt bitmask */


void initGPIO(void){
	vAHI_DioSetDirection(0,1<<LED1pin); //output
	vAHI_DioSetOutput(0,1<<LED1pin);
	vAHI_DioSetDirection(0,1<<LED2pin);
	vAHI_DioSetOutput(0,1<<LED2pin);
	//vAHI_DioSetOutput(0, 1<<LED1pin);
	//vAHI_DioSetDirection(0,1<<LED3pin);
	//vAHI_DioSetOutput(1<<LED3pin,0);
    //vAHI_DioSetPullup(0x000F, 0);
	//vAHI_DioSetOutput(0, 0); //set D17 low


#if hardware == Z200USB
	/*
	vAHI_DioSetDirection(0,1<< RTSpin);//set RTS as input
	vAHI_DioSetOutput(0, 1<< RTSpin); //set RTS high

	vAHI_DioSetDirection(0,1<<CTSpin);//set CTS as output
	*/
	//vAHI_DioSetOutput(1<<CTSpin,0); //set CTS hi
	vAHI_SysCtrlRegisterCallback(ISR_handleSysCtrl);
	vAHI_DioSetDirection(1<<KEY0pin,0);//set key0 as input
	vAHI_DioInterruptEdge(0, 1<<KEY0pin);
	vAHI_DioInterruptEnable(1<< KEY0pin,0);

	vAHI_DioSetDirection(1<<KEY1pin,0);//set key0 as input
	vAHI_DioInterruptEdge(0, 1<<KEY1pin);
	vAHI_DioInterruptEnable(1<< KEY1pin,0);
#endif
#if (hardware == Z200Old485) | (hardware == Z200485)
	vAHI_DioSetDirection( 0, 1<<REpin);//set RE as output
	vAHI_DioSetOutput(0, 1<<REpin); //set RE low to enable receive
	vAHI_DioSetDirection(0, 1<< DEpin);//set DE as output
	vAHI_DioSetOutput(0, 1<< DEpin); //set DE low to disable line drive
	vAHI_SysCtrlRegisterCallback(ISR_handleSysCtrl);

	vAHI_DioSetDirection(1<<KEY0pin,0);//set key0 as input
	vAHI_DioInterruptEdge(0, 1<<KEY0pin);
	vAHI_DioInterruptEnable(1<< KEY0pin,0);

	vAHI_DioSetDirection(1<<KEY1pin,0);//set key0 as input
	vAHI_DioInterruptEdge(0, 1<<KEY1pin);
	vAHI_DioInterruptEnable(1<< KEY1pin,0);
#endif

}

void parseCommand(uint8* pCommandstr, uint8 pLength){
	static bool gotCommand = FALSE;
    uint8 length = 0;
	uint8 datax;
    char gCommand[250];
	DBG_vPrintf(1,"gCmdIndex %d pLength %d length=%d\r\n", gCmdIndex,pLength,length);
	while (length < pLength){

		datax= pCommandstr[length++];
		gCommand[gCmdIndex]=datax;
		//DBG_vPrintf(1, "gCommand[%d]=%x \r\n", gCmdIndex,gCommand[gCmdIndex]);
		gCmdIndex++;
		switch (datax){
		case 0x0D:
			gGotCR=1;
			break;
		case 0x0A:
			if (gGotCR){
				gotCommand=1;
				gGotCR=0;
				gCommand[gCmdIndex-2]='\0';
				break;
			}
			break;
		default:
			gGotCR=0; //CR should be followed immediately by a LF
			break;
		}

	}
	//DBG_vPrintf(1,"W gCmdIndex %d pLength %d length=%d\r\n", gCmdIndex,pLength,length);
	//=== Parse command
	if (gotCommand){
		uint8 i=0,y=0;
		char * pch;

		DBG_vPrintf(1,"Got command: %s", gCommand);
		if (strlen(gCommand)>1){
			pch = strtok(gCommand,"= \r");
			for (i=0; i< MAX_CMD_NUMBER; i++){
				if (strcmp(pch, CommandList[i].commandName)==0){
					//UART0SendStr("got one command\r\n");
					pch = strtok(NULL, " ");
					ATSLen=gCmdIndex-6;
					//memcpy(pch, &gCommand[4],gCmdIndex-7);
					//if (!gProtectionTimer.enabled){  //Execute when there is no 45 seconds protection
					CommandList[i].commandFunction(pch);
						//LPC_GPIO_PORT->MPIN[0] = ~DEBUG_PIN;
					//}else{
						//UART0Send("#LED G 0\r\n", 10);  //for DA go sleep
					//}
					break;
				}
			}
			if (i == MAX_CMD_NUMBER){
				//printMsg("ATX=Error\r\n");
			}else{
				//printMsg("ATX=Ok\r\n");
			}
		}
		gCmdIndex = 0;
		gotCommand =0;
	}

}

//register commands received from BLE/Zigbee module
uint8_t registerCommand(uint8_t cmdIndex, char* commandName, uint8_t (*funcPtr)(char *)){

	uint8_t result=1;
	strcpy(CommandList[cmdIndex].commandName, commandName);
	CommandList[cmdIndex].commandFunction=funcPtr;
	return result;

}

/****************************************************************************/
void initCommands(void){
	registerCommand(0, "ATCHPN", processPANID);
	registerCommand(1, "ATCHBR", processBaud);
	registerCommand(2, "ATCHPR", processParity);
	registerCommand(3, "ATCHPT", processParityType);
	registerCommand(4, "ATCHDB", processDataBit);
	registerCommand(5, "ATCHBM", processBitMode);
	registerCommand(6, "ATCHAD", processOwnAddress);
	registerCommand(7, "ATCHRC", processRFChannel);
	registerCommand(8, "ATCHFT", processMasterSlave);
	registerCommand(9, "ATSV", processSaveFlash);
	registerCommand(10, "ATEXIT", processExit);
	registerCommand(11, "ATRST", processReset);
	registerCommand(12, "ATSWCF", processSwcf);
	registerCommand(13, "ATCHDN", processDestAddress);
	registerCommand(14, "ATCHRT", processRepeat);
	registerCommand(15, "ATCHST", processStopBit);
	//registerCommand(16, "ATSensorE", processSensorE);
	//registerCommand(17, "ATSensorD", processSensorD);
	registerCommand(16, "XLIS", processXLIS);
	registerCommand(17, "XCMD", processXCMD);
	registerCommand(18, "ATS", processPsW);
	registerCommand(19, "SLIS", processSlis);
	registerCommand(20, "DLIS", processDLIS);
	registerCommand(21, "ELOO", processRfEnableLoop);
	registerCommand(22, "DLOO", processRfDisableLoop);
	registerCommand(23, "TIME", processRfLoopTime);
	registerCommand(24, "LIST", processlist);
	registerCommand(25, "EREA", processATEreaseEE);
	registerCommand(26, "SCMD", processSCMD);
	registerCommand(27, "CMD1", processCMD1);
	registerCommand(28, "DCMD", processDCMD);
	//registerCommand(29, "Test", processTest);
	registerCommand(29, "CMD2", processCMD2);
	registerCommand(30, "PAIR", processPAIR);
	registerCommand(31, "CMDR", processCMDR);



}

/////ATCommand
//=== Calibrate sensor. Format: calibrate sensorID current value(decimal integer)
uint8 processPANID(char * cmdStr){
	char * sName;
    uint8 temp;
    uint8 x=0;
    uint16 panid16=0;
	sName = strtok(cmdStr, " ");

	for(x=0;x<=1;x++){
		temp=convertASCII2HexByte(sName+x*2);
		panid16 = panid16 * 256 + temp;
	}
	gConfigData.settings.panID=panid16;

	NODE_sData.u16PanId=panid16;

	DBG_vPrintf(1,"ATCHPN %x \r\n",panid16);
	return 1;
}

uint8 processBaud(char * cmdStr){
	char * sName;

	sName = strtok(cmdStr, " ");
	switch (sName[0]-0x30){
		case 0x00:
			gConfigData.settings.baudrate=1200;
			break;
		case 0x01:
			gConfigData.settings.baudrate=2400;
			break;
		case 0x02:
			gConfigData.settings.baudrate=4800;
			break;
		case 0x03:
			gConfigData.settings.baudrate=9600;
			break;
		case 0x04:
			gConfigData.settings.baudrate=19200;
			break;
		case 0x05:
			gConfigData.settings.baudrate=38400;
			break;
		case 0x06:
			gConfigData.settings.baudrate=57600;
			break;
		case 0x07:
			gConfigData.settings.baudrate=115200;
			break;
	}

	DBG_vPrintf(1,"ATCHBR %d \r\n",gConfigData.settings.baudrate);
	return 1;
}

uint8 processParity(char * cmdStr){
	char * sName;

	sName = strtok(cmdStr, " ");

	switch (sName[0]-0x30){
		case 0x00:
			gConfigData.settings.parity=0;
			break;
		case 0x01:
			gConfigData.settings.parity=1;
			break;
	}
	DBG_vPrintf(1,"ATCHPR %d \r\n",gConfigData.settings.parity);
	return 1;
}
uint8 processParityType(char * cmdStr){
	char * sName;

	sName = strtok(cmdStr, " ");

	switch (sName[0]-0x30){
		case 0x00:
			gConfigData.settings.parityType=1;
			break;
		case 0x01:
			gConfigData.settings.parityType=0;
			break;
	}
	DBG_vPrintf(1,"ATCHPT %d \r\n",gConfigData.settings.parityType);
	return 1;
}

uint8 processDataBit(char * cmdStr){
	char * sName;

	sName = strtok(cmdStr, " ");

	switch (sName[0]-0x30){
		case 0x07:
			gConfigData.settings.dataBit=2;  //7bit
			break;
		case 0x08:
			gConfigData.settings.dataBit=3;  //8bit
			break;
	}
	DBG_vPrintf(1,"ATCHBM %d \r\n",gConfigData.settings.dataBit);
	return 1;
}

uint8 processStopBit(char * cmdStr){
	char * sName;

	sName = strtok(cmdStr, " ");

	switch (sName[0]-0x30){
		case 0x01:
			gConfigData.settings.stopBit=E_AHI_UART_1_STOP_BIT;    //Stop 1
			break;
		case 0x02:
			gConfigData.settings.stopBit=E_AHI_UART_2_STOP_BITS;  //Stop 2
			break;
	}
	DBG_vPrintf(1,"ATCHST %d \r\n",gConfigData.settings.dataBit);
	return 1;
}


uint8 processPsW(char * cmdStr){
	//char * sName;
	uint8_t Len,y=0;
    uint16 RMdata16=0;
    uint8 temp;
    uint8 x=0;
/*
	for(y=0;y<ATSLen;y++){
		DBG_vPrintf(1, "cmdStr[%d]=%02x\r\n",y,cmdStr[y]);
	}
*/
	for(x=0;x<=1;x++){
		temp=convertASCII2HexByte(cmdStr+x*2);
		RMdata16 = RMdata16 * 256 + temp;
	}
	DBG_vPrintf(1, "RMdata16 %x\r\n", RMdata16);

	//Len=strlen(cmdStr);
	Len=ATSLen;
	DBG_vPrintf(1, "ATSLen %x\r\n", ATSLen);

	ATSLen=0;
	memcpy(&PswCmdData[8], &cmdStr[4],Len-4);
	PswCmdData[Len+4]=0x0D;
	PswCmdData[Len+5]=0x0A;

	PswCmdData[TARGET_POS]=RMdata16>>8;
	PswCmdData[TARGET_POS+1]=RMdata16;

	gWaitUARTPacket = FALSE;
	PswCmdData[SEQ_POS] = WUART_u8MsgSeqTxNew();
    if(RMdata16==0xffff){
    	WUART_vTx(0xffff, 10+Len-4, PswCmdData);
    }else{
    	WUART_vTxAck(RMdata16, 10+Len-4, PswCmdData);
    }


/*
	Len=strlen(cmdStr);

	memcpy(&PswCmdData[8], cmdStr,Len);

	//DBG_vPrintf(1, "PswCmdData\r\n");
	PswCmdData[Len+8]=0x0D;
	PswCmdData[Len+9]=0x0A;
	for(y=0;y<30;y++){
		DBG_vPrintf(1, "PswCmdData[%d]=%x \r\n", y,PswCmdData[y]);
	}

	gWaitUARTPacket = FALSE;
	PswCmdData[SEQ_POS] = WUART_u8MsgSeqTxNew();

	if (gConfigData.settings.targetAddr == 0xFFFF){
		WUART_vTx(0xFFFF, 10+Len, PswCmdData);
	}else{
		uint64 targetAddr = (uint64)gConfigData.settings.targetAddr;
		WUART_vTxAck(targetAddr, 10+Len, PswCmdData);
	}
*/
	return 0;
}

uint8 processBitMode(char * cmdStr){
	char * sName;
	//Do nothing.
	DBG_vPrintf(1,"ATCHBM %d \r\n",gConfigData.settings.dataBit);
	printMsg("\r\nATCHBM is deprecated, please use ATCHDB=n instead. 'n' is 7 for 7 data bits, 8 for 8 data bits.\r\n");
	return 0;
}

uint8 processOwnAddress(char * cmdStr){
	char * sName;
    uint8 temp;
    uint16 address16=0;
    uint8 x=0;

	sName = strtok(cmdStr, " ");

	for(x=0;x<=1;x++){
		temp=convertASCII2HexByte(sName+x*2);
		address16 = address16 * 256 + temp;
	}
	gConfigData.settings.shortAddr=address16;

	//gConfigZigbee.ShortAddress=(Temp[0]-0x30)+(Temp[1]-0x30);
	NODE_sData.u16Address = address16 ;
	DBG_vPrintf(1,"ATCHAD %x \r\n",address16);
	return 1;
}

uint8 processDestAddress(char * cmdStr){
	char * sName;
    uint8 temp;
    uint16 address16=0;
    uint8 x=0;

	sName = strtok(cmdStr, " ");
	for(x=0;x<=1;x++){
		temp=convertASCII2HexByte(sName+x*2);
		address16 = address16 * 256 + temp;
	}
	gConfigData.settings.targetAddr=address16;

	DBG_vPrintf(1,"ATCHDN %x \r\n",address16);
	return 1;
}

uint8 processRFChannel(char * cmdStr){
	char * sName;
    uint8 temp;

	sName = strtok(cmdStr, " ");

	temp =convertASCII2HexByte(sName);

	gConfigData.settings.channel= temp;
	DBG_vPrintf(1,"ATCHRC %x \r\n",gConfigData.settings.channel);
	return 1;
}

uint8 processMasterSlave(char * cmdStr){
	char * sName;

	sName = strtok(cmdStr, " ");

	switch (sName[0]-0x30){
		case 0x01:
			gConfigData.settings.deviceType=dtMaster;
			break;
		case 0x02:
			gConfigData.settings.deviceType=dtSlave;
			break;
		case 0x05:
			gConfigData.settings.deviceType=dtOne2OneMaster;
			break;
		case 0x06:
			gConfigData.settings.deviceType=dtOne2OneSlave;
			break;
	}


	DBG_vPrintf(1,"ATCHFT %d \r\n",	gConfigData.settings.deviceType);
	return 1;
}

uint8 processSaveFlash(char * cmdStr){
	char * sName;
	uint8 EEPROMSegmentDataLength;
	sName = strtok(cmdStr, " \r");
	u16AHI_InitialiseEEP(&EEPROMSegmentDataLength);

	iAHI_EraseEEPROMsegment(CONFIG_EEPROM_SECTOR_NUM);
	//DBG_vPrintf(1, "EEP_W \n");
	//DBG_vPrintf(1,"musk %d \n",gConfigData.settings.mask);
	//DBG_vPrintf(1,"ATCHBR %d \n",gConfigData.settings.baudrate);
	//DBG_vPrintf(1,"ATCHPR %d \n",gConfigData.settings.parity);
	//DBG_vPrintf(1,"ATCHPT %d \n",gConfigData.settings.parityType);
	//DBG_vPrintf(1,"ATCHBM %d \n",gConfigData.settings.dataBit);
	//DBG_vPrintf(1,"ATSV=%s . PANID %#x \n",cmdStr, gConfigData.settings.panID[1]);
	iAHI_WriteDataIntoEEPROMsegment(CONFIG_EEPROM_SECTOR_NUM,0,gConfigData.data,64);
	DBG_vPrintf(1,"ATSV %s \n",sName);
	return 1;
}

uint8 processExit(char * cmdStr){
	char * sName;

	sName = strtok(cmdStr, " ");
	gSerialMode=TransparentMode_c;    //TransparentMode_c
	printMsg("Exit Config Mode.\r\n");
	DBG_vPrintf(1,"ATEXIT %s \r\n",sName);
	if (gConfigData.settings.deviceType == dtSlave){
		ED_vReqActiveScan();
	}
	return 1;
}

uint8 processReset(char * cmdStr){
	DBG_vPrintf(1,"ATRST \r\n");
	vAHI_SwReset();
	return 1;
}

uint8 processRepeat(char * cmdStr){ //change application repeat send times
	char * sName;

	sName = strtok(cmdStr, " ");
	gConfigData.settings.applicationResend = sName[0]-0x30;
	DBG_vPrintf(1,"ATCHRT %d \r\n",	gConfigData.settings.applicationResend);
	return 1;
}

uint8 processSwcf(char * cmdStr){
	uint8 temp[32]={0};
	uint8 y=0;
	//DBG_vPrintf(1,"ATSWCF \r\n");
	temp[0]=0x1B;

    switch(gConfigData.settings.baudrate){
		case 1200:    //2400
			temp[1]=0x01;
			temp[2]=0xA0;
			break;
		case 2400:    //4800
			temp[1]=0xd0;
			temp[2]=0x00;
			break;
		case 9600:   //9600
			temp[1]=0x68;
			temp[2]=0x00;
			break;
		case 19200:    //19200
			temp[1]=0x34;
			temp[2]=0x00;
			break;
		case 38400:    //38400
			temp[1]=0x1A;
			temp[2]=0x00;
			break;
		case 57600:    //57600
			temp[1]=0x11;
			temp[2]=0x00;
			break;
		case 115200:    //57600
			temp[1]=0x11;
			temp[2]=0x00;
			break;
    }

	temp[3]=NODE_sData.u8Channel;
	temp[4]=gConfigData.settings.deviceType;


	temp[5]=gConfigData.settings.shortAddr&0xFF;
	temp[6]=gConfigData.settings.shortAddr>>8;

	temp[7]=gConfigData.settings.panID & 0xFF;
	temp[8]=gConfigData.settings.panID >>8;

	temp[9]=gConfigData.settings.verbose;
	temp[10]=gConfigData.settings.targetAddr&0xFF;
	temp[11]=gConfigData.settings.targetAddr>>8;
	temp[12]=gConfigData.settings.applicationResend; //Application level resend
	temp[13]= 0xFF; //0XFF for compatibility
	temp[14]= (gConfigData.settings.parity & 0x0F)+ ((gConfigData.settings.parityType & 0x0F)<<4);
	temp[15]= gConfigData.settings.dataBit;
	temp[16]= gConfigData.settings.stopBit;
	temp[17]= gConfigData.settings.fwVersion;
	temp[18]= gConfigData.settings.parentAddr&0xFF;
	temp[19]= gConfigData.settings.parentAddr>>8;
	temp[20]= gConfigData.settings.outputPower;
	temp[30]= 0x0D;
	temp[31]= 0x0A;


	for(y=0;y<32;y++){
		UART_bPutChar(1, temp[y]);
	}

	UART_vStartTx(1);



	//for test
/*
	sensorList[0].info.sensorAddress=0x01;
	sensorList[0].info.sensorType=XMT_288FC;
	sensorList[0].info.sensorCmdNum[0].sensorCmd[0]=0x01;    //sensor address
	sensorList[0].info.sensorCmdNum[0].sensorCmd[1]=0x03;
	sensorList[0].info.sensorCmdNum[0].sensorCmd[2]=0x00;
	sensorList[0].info.sensorCmdNum[0].sensorCmd[3]=0x00;
	sensorList[0].info.sensorCmdNum[0].sensorCmd[4]=0x00;
	sensorList[0].info.sensorCmdNum[0].sensorCmd[5]=0x01;
	calc_crc(sensorList[0].info.sensorCmdNum[0].sensorCmd,6);

	sensorList[0].info.Enable=1;
	sensorList[0].info.mask=0x88;
	sensor_mode=EnableMode_c;
*/
	return 1;
}

uint8 processATEreaseEE(char * cmdStr){
uint8 L=0,x=0;
                for(x=0;x<20;x++){
    				iAHI_EraseEEPROMsegment(x+EEpromAddress);
                }

                for(x=0;x<10;x++){
    				iAHI_EraseEEPROMsegment(x+EEpromCMDTable+EEpromAddress);
                }

				gConfigData.settings.mask=0x88;
				//gConfigData.settings.baudrate=9600;    //9600
				//gConfigData.settings.baudrate=115200;    //9600
				gConfigData.settings.parity=E_AHI_UART_PARITY_DISABLE;      //enable
				//gConfigData.settings.parity=E_AHI_UART_PARITY_ENABLE;      //enable
				gConfigData.settings.parityType=E_AHI_UART_EVEN_PARITY;  //even
				gConfigData.settings.dataBit=E_AHI_UART_WORD_LEN_8;
				gConfigData.settings.stopBit=E_AHI_UART_1_STOP_BIT;

#if  Gatway
			    gConfigData.settings.baudrate=115200;    //9600
			    gConfigData.settings.deviceType=dtOne2OneMaster; //Master
#else
				gConfigData.settings.baudrate=9600;    //9600
			    gConfigData.settings.deviceType=dtOne2OneSlave;
#endif
				//gConfigData.settings.deviceType=dtOne2OneMaster; //Master
				gConfigData.settings.panID=PAN_ID;
				gConfigData.settings.shortAddr=NODE_sData.u64Address & 0xFFFF;
				gConfigData.settings.targetAddr=0xFFFF;
				gConfigData.settings.channel=Default_Channel;
				gConfigData.settings.verbose =1;
				gConfigData.settings.applicationResend=DefaultApplicationResendTimes;
				gConfigData.settings.fwVersion=Z200_Version_Value;
				gConfigData.settings.outputPower=0xFF; //maximum
				gConfigData.settings.parentAddr=0xFFFF;

				iAHI_WriteDataIntoEEPROMsegment(0,0,gConfigData.data,60);


			    NODE_sData.u8Channel = gConfigData.settings.channel;
				NODE_sData.u16Address = gConfigData.settings.shortAddr;
				NODE_sData.u16PanId = gConfigData.settings.panID;

				DBG_vPrintf(1, "\r\nBaud=%d", gConfigData.settings.baudrate);
				DBG_vPrintf(1, "\r\nPANID= %#x", NODE_sData.u16PanId);
				DBG_vPrintf(1, "\r\nShortAddr= %#x, targetAddr= %#x", NODE_sData.u16Address, gConfigData.settings.targetAddr);
				DBG_vPrintf(1, "\r\nChannel= %d", NODE_sData.u8Channel);
				WUART_vInit();
				//printMsg("ATX=%dRestore factory settings\r\n",26);
				L=26;
	    		wuart_vPrintf(1, "ATX=%dRestore factory settings\r\n",L);
				/* Set short address in pib */
			    MAC_vPibSetShortAddr(NODE_sData.pvMac, NODE_sData.u16Address);
				/* Set pan id in pib */
			    MAC_vPibSetPanId(NODE_sData.pvMac, NODE_sData.u16PanId);
			    /* Enable receiver to be on when idle */
			    MAC_vPibSetRxOnWhenIdle(NODE_sData.pvMac, TRUE, FALSE);
			    /* Allow nodes to associate */
			    NODE_sData.psPib->bAssociationPermit = 1;

				PSW_CMD_flag=FALSE;  //restart RS485 Loop
	return 1;
}

uint8 processPAIR(char * cmdStr){ //processPAIR

	DBG_vPrintf(1,"processPAIR\r\n");
	gWorkingMode = wmPairingMode_c;
	gPairingPeriodCounter=0;
	return 1;
}



/////Hex2ASCII
uint8 convertASCII2HexByte(char * aWord){

	uint8 abyte=0;;

	if ( (aWord[0]>=0x30) & (aWord[0]<=0x39)){
		abyte = aWord[0]-0x30;
		abyte = abyte <<4;
	}else{
		if ((aWord[0]>='A')&(aWord[0]<='F')){
			abyte = aWord[0]-'A'+10;
			abyte = abyte <<4;
		}else if((aWord[0]>='a')&(aWord[0]<='f')){
			abyte = aWord[0]-'a'+10;
		}
	}
	if ( (aWord[1]>0x30) & (aWord[1]<=0x39)){
		abyte += aWord[1]-0x30;
	}else{
		if ((aWord[1]>='A')&(aWord[1]<='F')){
			abyte += aWord[1]-'A'+10;
		}
	}
	return abyte;
}

void printMsg(uint8 * msg){
/*
	uint8 i =0;
	while ((msg[i] !=0)&&(i<100)){
		UART_bPutChar(WUART_sData.u8Uart,  msg[i]);
		i++;
	}
	*/
}

PUBLIC void	wuart_vPrintf(bool_t bStream, const char *pcFormat, ...)
{
	/* Stream is anabled and uart is opened ? */
	if (bStream == TRUE)
	{
		va_list ap;

		/* Initialise argument pointer */
		va_start(ap, pcFormat);
		/* Call the worker function to do the work */
		UART_bPrintf_ap(WUART_UART, pcFormat, ap);
		while((u8AHI_UartReadLineStatus(WUART_UART) & E_AHI_UART_LS_THRE ) == 0);
	}
}

PUBLIC void timer0Callback(uint32 u32Device, uint32 u32ItemBitmap){
	//LED3Off

    Timer0_Watchdog_flag=TRUE;
#if ((hardware == Z200485)||(hardware == Z200Old485))
	if (gTurn485ReceiveMode){
		if ((u8AHI_UartReadLineStatus(WUART_UART)&E_AHI_UART_LS_TEMT) !=0){
			DEOff
			REOff
			gTurn485ReceiveMode = 0;
			//DBG_vPrintf(1,"Timer0DERE\r\n");
		}
	}
	//LED3On
#endif

	if (gCheckRXTimeout ){
		if (gCheckTimeoutCounter==0){ // Remedy for  E_AHI_UART_FIFO_LEVEL_14 interrupt
			gCheckRXTimeout = FALSE;
			gWaitUARTPacket = FALSE;
			gCheckTimeoutCounter=gRxdelay_count;
			DBG_vPrintf(Debug,"counter %d", gCheckTimeoutCounter);
		}else {
			gCheckTimeoutCounter--;
		}
	}

/*
	if(RS485CMD_Count_Timer_Flag){
		RS485CMD_Count++;
		if(RS485CMD_Count>100){
			sendEvent(evFindSensor_CountEvent);
			RS485CMD_Count_Timer_Flag=FALSE;
			RS485CMD_Count=0;
		}
	}
	*/
#if 0
	if (gFlashDataLED){
		if (dataLEDCounter >60){
			 if (u32AHI_DioReadInput() & (1<< LED2pin)){
				 LED2Off
			 }else{
				 LED2On
			 }
			 dataLEDCounter=0;
			 dataLEDDuration++;
			if (dataLEDDuration>6){
				gFlashDataLED=FALSE;
				dataLEDDuration=0;
				 LED2On
			}
		}else{
			dataLEDCounter++;
		}
	}
#endif
}

PUBLIC void Restorekey0_timer1Callback(uint32 u32Device, uint32 u32ItemBitmap){
uint8_t x=0;

	if(!(u32AHI_DioReadInput() & (1<< KEY0pin))){
		gKey0Pressed = TRUE;
	    DBG_vPrintf(Debug, "\r\n gKey0PressedTimer1Count %x", gKey0ResetCount);
	}
	if (gKey0Pressed){
		if (u32AHI_DioReadInput() & (1<< KEY0pin)){
			ResetGledMode=FALSE;
			ResetModeNoPairkey=TRUE;
			gKey0Pressed=FALSE;
			gKey0ResetCount=0;
			DBG_vPrintf(Debug, "\r\n gKey0falseTimer1 %x", gKey0ResetCount);
			vAHI_DioSetOutput(1<<LED1pin,0);    //LED on
			vAHI_DioSetOutput(1<<LED2pin,0);
		}else{
			if (gKey0ResetCount>10){
				ResetGledMode=TRUE;
				vAHI_DioSetOutput(1<<LED2pin,0);   //LED R ON

				gKey0ResetCount=0;
				DBG_vPrintf(Debug, "\r\n Reset5sec");
                for(x=0;x<20;x++){
    				iAHI_EraseEEPROMsegment(x+EEpromAddress);
                }

                for(x=0;x<10;x++){
    				iAHI_EraseEEPROMsegment(x+EEpromCMDTable+EEpromAddress);
                }
				gConfigData.settings.mask=0x88;
				//gConfigData.settings.baudrate=9600;    //9600
				//gConfigData.settings.baudrate=115200;    //9600
				gConfigData.settings.parity=E_AHI_UART_PARITY_DISABLE;      //enable
				//gConfigData.settings.parity=E_AHI_UART_PARITY_ENABLE;      //enable
				gConfigData.settings.parityType=E_AHI_UART_EVEN_PARITY;  //even
				gConfigData.settings.dataBit=E_AHI_UART_WORD_LEN_8;
				gConfigData.settings.stopBit=E_AHI_UART_1_STOP_BIT;
#if  Gatway
			    gConfigData.settings.baudrate=115200;    //9600
			    gConfigData.settings.deviceType=dtOne2OneMaster; //Master
#else
				gConfigData.settings.baudrate=9600;    //9600
			    gConfigData.settings.deviceType=dtOne2OneSlave;
#endif
				//gConfigData.settings.deviceType=dtOne2OneSlave; //Master
				//gConfigData.settings.deviceType=dtOne2OneMaster; //Master
				gConfigData.settings.panID=PAN_ID;
				gConfigData.settings.shortAddr= NODE_sData.u64Address & 0xFFFF;
				gConfigData.settings.targetAddr=0xFFFF;
				gConfigData.settings.channel= Default_Channel;
				gConfigData.settings.verbose =1;
				gConfigData.settings.applicationResend=DefaultApplicationResendTimes;
				gConfigData.settings.fwVersion=Z200_Version_Value;
				gConfigData.settings.outputPower=0xFF; //maximum
				gConfigData.settings.parentAddr=0xFFFF;
				DBG_vPrintf(Debug, "\r\nRestore factory settings.");
				iAHI_WriteDataIntoEEPROMsegment(0,0,gConfigData.data,60);


			    NODE_sData.u8Channel = gConfigData.settings.channel;
				NODE_sData.u16Address = gConfigData.settings.shortAddr;
				NODE_sData.u16PanId = gConfigData.settings.panID;

				DBG_vPrintf(Debug, "\r\nBaud=%d", gConfigData.settings.baudrate);
				DBG_vPrintf(Debug, "\r\nPANID= %#x", NODE_sData.u16PanId);
				DBG_vPrintf(Debug, "\r\nShortAddr= %#x, targetAddr= %#x", NODE_sData.u16Address, gConfigData.settings.targetAddr);
				DBG_vPrintf(Debug, "\r\nChannel= %d", NODE_sData.u8Channel);
				WUART_vInit();
				printMsg("\r\nRestore factory settings.");

				/* Set short address in pib */
			    MAC_vPibSetShortAddr(NODE_sData.pvMac, NODE_sData.u16Address);
				/* Set pan id in pib */
			    MAC_vPibSetPanId(NODE_sData.pvMac, NODE_sData.u16PanId);
			    /* Enable receiver to be on when idle */
			    MAC_vPibSetRxOnWhenIdle(NODE_sData.pvMac, TRUE, FALSE);
			    /* Allow nodes to associate */
			    NODE_sData.psPib->bAssociationPermit = 1;
			}else{
				gKey0ResetCount++;
			}
		}

	}


	if(ResetGledMode){    //if in ResetMode delay 1 sec to turn GLED
		ResetLEDCounter++;
		DBG_vPrintf(Debug, "\r\n ResetModeTimer1 %x", ResetLEDCounter);
		if(ResetLEDCounter>1){
			vAHI_DioSetOutput(1<<LED1pin,0);    //LED G on
			ResetLEDCounter=0;
		}
	}

	if(ResetModeNoPairkey){
		DBG_vPrintf(Debug, "\r\n ResetModeNoPairkey");
		vAHI_TimerDisable(E_AHI_TIMER_1);
	}

}


PUBLIC void Key_timer3Callback(uint32 u32Device, uint32 u32ItemBitmap){

	DBG_vPrintf(Debug, "\r\n Timer3 Count gKey0Pressed=%d gKey1Pressed=%d",gKey0Pressed,gKey1Pressed);

	if (gKey0Pressed){
		if (u32AHI_DioReadInput() & (1<< KEY0pin)){
			gKey0Pressed=FALSE;
			DBG_vPrintf(Debug, "\r\n gKey0falseTimer2 %x", gKey0Count);
			gKey0Count=0;
			vAHI_TimerDisable(E_AHI_TIMER_3);
		}else{
			if (gKey0Count>10){
				sendEvent(evKey0PressedEvent);
				vAHI_TimerDisable(E_AHI_TIMER_3);
				gKey0Count=0;
				gKey0Pressed=FALSE;
			}else{
				gKey0Count++;
			}
		}

	}

	if (gKey1Pressed){
		if (u32AHI_DioReadInput() & (1<< KEY1pin)){
			gKey1Pressed=FALSE;
			DBG_vPrintf(Debug, "\r\n gKey1falseTimer2 %x", gKey1Count);
			gKey1Count=0;
			vAHI_TimerDisable(E_AHI_TIMER_3);
		}else{
			if (gKey1Count>10){
				sendEvent(evKey1PressedEvent);
				vAHI_TimerDisable(E_AHI_TIMER_3);
				gKey1Count=0;
				gKey1Pressed=FALSE;
			}else{
				gKey1Count++;
			}
		}

	}

}
PRIVATE void ISR_handleSysCtrl(uint32 u32Device, 	 	/**< Interrupting device */
					         uint32 u32ItemBitmap) 	/**< Interrupt bitmask */
{
	//if (u32Device == E_AHI_DEVICE_SYSCTRL) {}
//LED2Off
#if (hardware == Z200485)
	 if (u32ItemBitmap  == 	E_AHI_DIO11_INT ){
#endif
#if(hardware == Z200USB)
		 if (u32ItemBitmap  == 	E_AHI_DIO4_INT ){
#endif

		 //DBG_vPrintf(WUART_TRACE, "\r\nKey0 Pressed %#x", u32ItemBitmap);
		 /*
	     if ((gConfigData.settings.deviceType== dtOne2OneMaster) |(gConfigData.settings.deviceType == dtOne2OneSlave)){
			gKey0Pressed = TRUE;
			gKey0DebounceCount=0;
		 }
		*/
		DBG_vPrintf(Debug, "\r\n ISR gKey0Pressed=%d gKey1Pressed=%d",gKey0Pressed,gKey1Pressed);

	    gKey0Pressed = TRUE;
	    gKey0DebounceCount=0;
	    gKey0Count=0;
		vAHI_TimerClockSelect(E_AHI_TIMER_3, FALSE, FALSE);    //Send CMD sensor timer
		vAHI_TimerEnable(E_AHI_TIMER_3, 16 ,FALSE, TRUE,FALSE);  //409.6ms
		vAHI_Timer3RegisterCallback(Key_timer3Callback);
		vAHI_TimerStartRepeat(E_AHI_TIMER_3, 0,125);
		DBG_vPrintf(Debug, "\r\n gKey0Pressed");
	}else if(u32ItemBitmap  == 	E_AHI_DIO1_INT){
		gKey1Pressed = TRUE;
		gKey1Count=0;
		vAHI_TimerClockSelect(E_AHI_TIMER_3, FALSE, FALSE);    //Send CMD sensor timer
		vAHI_TimerEnable(E_AHI_TIMER_3, 16 ,FALSE, TRUE,FALSE);  //409.6ms
		vAHI_Timer3RegisterCallback(Key_timer3Callback);
		vAHI_TimerStartRepeat(E_AHI_TIMER_3, 0,125);
		DBG_vPrintf(Debug, "\r\n gKey1Pressed");
	}
//LED2On
}



#define Z200_Version "V250\r\n"
#define Z200_Version_Value 0x50

#define BufferLen 150
#define CommandLength 80
#define MAX_CMD_NUMBER 31

#define ModeBusArrLen 150
//=== Serial Data Working Mode

#define LED1On	vAHI_DioSetOutput(1<<LED1pin,0);
#define LED1Off	vAHI_DioSetOutput(0,1<<LED1pin);
#define LED2On	vAHI_DioSetOutput(1<<LED2pin,0);
#define LED2Off	vAHI_DioSetOutput(0,1<<LED2pin);
#define LED3On	vAHI_DioSetOutput(1<<LED3pin,0);
#define LED3Off	vAHI_DioSetOutput(0,1<<LED3pin);
#define DEOn	vAHI_DioSetOutput(1<<DEpin,0);
#define DEOff	vAHI_DioSetOutput(0,1<<DEpin);
#define REOn	vAHI_DioSetOutput(1<<REpin,0);
#define REOff	vAHI_DioSetOutput(0,1<<REpin);
#define CONFIG_EEPROM_SECTOR_NUM	0

typedef enum
{
	CommandMode_c = 0,
	TransparentMode_c
} SerialMode_t;

#define wmNormalMode_c	1
#define wmPairingMode_c	2

#define	dtMaster 	1
#define	dtSlave 	2
#define dtOne2OneMaster 	3
#define dtOne2OneSlave 		4


typedef struct {
	char commandName[15];
	uint8 (*commandFunction)(char* parameterStr);
}CommandType_t;

//Uart Config struct
typedef struct {
	uint8 mask;
	uint32 baudrate;
	uint8 parity;
	uint8 parityType;
	uint8 dataBit;
	uint8 stopBit;

	uint16 panID;
	uint8 verbose;
	uint16 shortAddr;
	uint16 targetAddr;
	uint8 channel;
	uint8 deviceType;
	uint8 applicationResend;
	uint8 fwVersion;
	uint16 parentAddr;
	uint8 outputPower;
}UartConfig_t;

typedef  union  {
	uint8_t data[64];
	UartConfig_t settings;
	//UartConfig_t ports[2];
}ConfigData_t;

extern ConfigData_t gConfigData;
extern SerialMode_t gSerialMode; //TransparentMode_c;
extern uint8 gWorkingMode;
extern bool gKey0Pressed;
extern uint8 gKey0DebounceCount;
extern bool gFlashDataLED;
extern bool gFlashDataLEDG;
extern uint8 dataLEDDuration;
extern uint8 PswCmdData[150];
extern bool_t gCheckRXTimeout;
extern uint8 gCheckTimeoutCounter;


void initGPIO(void);
void initCommands(void);


void processRX1(void);
uint8 registerCommand(uint8 cmdIndex, char* commandName, uint8 (*funcPtr)(char *));
void parseCommand(uint8* commandstr, uint8 length);
void printMsg(uint8 * msg);
PUBLIC void	wuart_vPrintf(bool_t bStream, const char *pcFormat, ...);

uint8 processPANID(char * cmdStr);
uint8 processBaud(char * cmdStr);
uint8 processParity(char * cmdStr);
uint8 processParityType(char * cmdStr);
uint8 processDataBit(char * cmdStr);
uint8 processStopBit(char * cmdStr);
uint8 processBitMode(char * cmdStr);
uint8 processOwnAddress(char * cmdStr);
uint8 processRFChannel(char * cmdStr);
uint8 processMasterSlave(char * cmdStr);
uint8 processSaveFlash(char * cmdStr);
uint8 processExit(char * cmdStr);
uint8 processReset(char * cmdStr);
uint8 processSwcf(char * cmdStr);
uint8 processDestAddress(char * cmdStr);
uint8 processRepeat(char * cmdStr); //change application repeat send times
uint8 processPsW(char * cmdStr);
uint8 processATEreaseEE(char * cmdStr);
uint8 processTest(char * cmdStr);
uint8 processXLIS(char * cmdStr);
uint8 processXCMD(char * cmdStr);




void timer0Callback(uint32 u32Device, uint32 u32ItemBitmap);
void Restorekey0_timer1Callback(uint32 u32Device, uint32 u32ItemBitmap);
void Key_timer3Callback(uint32 u32Device, uint32 u32ItemBitmap);

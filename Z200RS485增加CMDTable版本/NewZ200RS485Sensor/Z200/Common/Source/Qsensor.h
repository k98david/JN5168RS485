/*
 * Qsensor.h
 *
 *  Created on: 2017/5/25
 *      Author: K98David
 */

#ifndef QSENSOR_H_
#define QSENSOR_H_

#define MaxValueNumber 10
#define MaxSensorNumber	20
#define SensorCMDNumber 10

#define EEpromAddress 1
#define EEpromCMDTable 40
typedef enum
{
	Error = 0,
	SendCMD,
	ReciveData
}Sensor485State_t;

typedef enum
{
	DisableMode_c = 0,
	EnableMode_c
}SensorMode_t;


typedef enum
{
	XMT_288FC = 1,
	XMT_E,
	XMT_E2,
	XMT_E3
}SensorType_t;

typedef struct{
	uint8_t sensorCmdLen;
	uint8_t sensorCmd[15];
}SensorCMD_t;


typedef struct{
	uint8_t mask;
	SensorType_t sensorType;
	uint8_t sensorAddress;
	bool Enable;
	//SensorCMD_t sensorCmdNum[3];
	uint8_t CMD_Table_number[1];
	uint8_t CmdCount;
	uint16_t LoopTime;
	uint16_t CountTime;
}SensorInfo_t;

typedef struct{
	SensorInfo_t info;
}SensorLog_t;

typedef struct{
	SensorCMD_t sensorCmdTable_Num[10];
}SensorCMDTable_t;

typedef struct{
	SensorCMD_t sensorCmdTable_Num[4];
}SensorEECMDTable_t;


typedef struct{
	uint8_t oldIndex;
}SensorOldIndex_t;

typedef struct{
	uint8_t Index;
	uint8_t data[15];
	uint8_t len;
}FifoBuffer_t;

void calc_crc(unsigned char *buf,int len);
void printCmd(uint8 * msg,uint8 len);
void timer1_SensorCMD_Callback(uint32 u32Device, uint32 u32ItemBitmap);
void timer1_Polling_Callback(uint32 u32Device, uint32 u32ItemBitmap);
void timer4_Polling_Callback(uint32 u32Device, uint32 u32ItemBitmap);
//void timer4_SendRF_Callback(uint32 u32Device, uint32 u32ItemBitmap);
uint8 processSensorE(char * cmdStr);
uint8 processSensorD(char * cmdStr);
uint8 processSlis(char * cmdStr);
uint8 processDLIS(char * cmdStr);
uint8 processRfEnableLoop(char * cmdStr); //change application repeat send times
uint8 processRfDisableLoop(char * cmdStr);
uint8 processRfLoopTime(char * cmdStr);
uint8 processlist(char * cmdStr);
uint8 processCMD1(char * cmdStr);
uint8 processCMD2(char * cmdStr);
uint8 processDCMD(char * cmdStr);
uint8 processPAIR(char * cmdStr);
uint8 processCMDR(char * cmdStr);


void readEEpromSensor(void);
uint8_t getSensorCMD(uint8_t *sData);
uint8_t processSCMD(char *sData);

uint8_t getSensorNumber(void);
extern Sensor485State_t Sensor485State;
extern uint8_t rF_Log_len;
extern uint8_t rF_Log_Count;
extern uint8_t sensor_RF_Packet_len;
extern SensorMode_t sensor_mode;
extern uint8 sensor_RF_Packet[150];
extern uint8 EEPROMDataLength;
extern uint8 totalSegment;
extern SensorLog_t sensorList[MaxSensorNumber];
extern bool RS485CMD_Count_Flag;
extern bool RS485CMD_Count_Timer_Flag;
extern bool PSW_CMD_flag;
extern uint8_t RS485CMD_Count;
extern uint8_t gRXRS485Sensor_Count;
extern uint8_t gRXRS485SensorCMDIndex;
extern SensorCMDTable_t SensorCMD_Table;
extern SensorEECMDTable_t SensorEECMD_Table[SensorCMDNumber];
extern bool PowerUpData;
//Ls list Start end*/
extern uint8 StarlsList_Number;
extern uint8 EndlsList_Number;

void CpydataFifo(uint8_t Number);
extern FifoBuffer_t Fifo_Data[30];
extern uint8_t Oldindex;
extern uint8_t FifoHead,FifoTail,CMD_Table_Index;
//extern SensorLog_t sensorListTest[MaxSensorNumber];
#endif /* QSENSOR_H_ */

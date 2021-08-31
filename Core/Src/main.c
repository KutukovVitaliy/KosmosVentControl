/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "socket.h"
#include "MQTTClient.h"
#include <string.h>
#include <stdlib.h>
#include "dhcp.h"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//Socket number defines
#define TCP_SOCKET	1
#define DHCP_SOCKET	0
//Receive Buffer Size define
#define BUFFER_SIZE			2048
#define _MODBUS_MES_SIZE	64
#define	_U3_MES_SIZE		10
#define FLG_SETPOINT		1
#define FLG_STATE			2
#define FLG_FAN				3
typedef struct {
	uint16_t rxIndex;
	uint8_t rxBuf[_MODBUS_MES_SIZE];
	uint32_t rxTime;
} myMes;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
//Global variables
uint16_t new_setpoint=0;
uint16_t new_state=0;
uint16_t new_fan =1;
uint8_t flag_arrived=0;
unsigned char targetIP[4] = {194,8,130,147}; // mqtt server IP
unsigned int targetPort = 1883; // mqtt server port
//uint8_t mac_address[6] = {};
wiz_NetInfo gWIZNETINFO = { //.mac = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef}, //user MAC
							.mac = {0x00, 0x18, 0x7c, 0x01, 0x9c, 0x2a}, //user MAC
							//.ip = /*{192,168,0,70},//*/{185,126,182,24}, //user IP
							//.sn = {255,255,255,0},
							//.gw = {185,126,182,1},
							.dhcp = NETINFO_DHCP
							};

// 1K should be enough, see https://forum.wiznet.io/t/topic/1612/2
uint8_t dhcp_buffer[1024];
volatile bool ip_assigned = false;

unsigned char tempBuffer[BUFFER_SIZE] = {};

struct opts_struct
{
	char* clientid;
	int nodelimiter;
	char* delimiter;
	enum QoS qos;
	char* username;
	char* password;
	char* host;
	int port;
	int showtopics;
} opts ={ (char*)"stdout-subscriber", 0, (char*)"\n", QOS0, (char*)"kvit", (char*)"20281174", (char*)targetIP, 1883, 0 };
char msg[60];

myMes m1;
myMes m2;
uint8_t b1;
uint8_t b2;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// включить модуль W5500 сигналом SCNn=0
void cs_sel()
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); //CS LOW
}

// выключить модуль W5500 сигналом SCNn=1
void cs_desel()
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); //CS HIGH
}

// принять байт через SPI
uint8_t spi_rb(void)
{
    uint8_t rbuf;
    HAL_SPI_Receive(&hspi2, &rbuf, 1, 0xFFFFFFFF);
    return rbuf;
}

// передать байт через SPI
void spi_wb(uint8_t b)
{
    HAL_SPI_Transmit(&hspi2, &b, 1, 0xFFFFFFFF);
}

void W5500_ReadBuff(uint8_t* buff, uint16_t len) {
    HAL_SPI_Receive(&hspi2, buff, len, HAL_MAX_DELAY);
}

void W5500_WriteBuff(uint8_t* buff, uint16_t len) {
    HAL_SPI_Transmit(&hspi2, buff, len, HAL_MAX_DELAY);
}

void Callback_IPAssigned(void) {
    //UART_Printf("Callback: IP assigned! Leased time: %d sec\r\n", getDHCPLeasetime());
    ip_assigned = true;
}

void Callback_IPConflict(void) {
    //UART_Printf("Callback: IP conflict!\r\n");
}

void messageArrived(MessageData* md)
{
	//unsigned char testbuffer[100];
	MQTTMessage* message = md->message;
	char tmp_buf[30];

	memset(tmp_buf,0,30);
	memcpy(tmp_buf, md->topicName->lenstring.data,md->topicName->lenstring.len);
	if(memcmp(tmp_buf, "Vent1/set/setpoint", md->topicName->lenstring.len) == 0)
	{
		memset(tmp_buf,0,30);
		memcpy(tmp_buf, message->payload, message->payloadlen);
		flag_arrived = FLG_SETPOINT;
		new_setpoint = (uint16_t)(atof(tmp_buf)*10);
	}
	else if(memcmp(tmp_buf, "Vent1/set/state", md->topicName->lenstring.len) == 0)
		{
			memset(tmp_buf,0,30);
			memcpy(tmp_buf, message->payload, message->payloadlen);
			flag_arrived = FLG_STATE;
			new_state = (uint16_t)(atof(tmp_buf));
		}
	else if(memcmp(tmp_buf, "Vent1/set/fan", md->topicName->lenstring.len) == 0)
			{
				memset(tmp_buf,0,30);
				memcpy(tmp_buf, message->payload, message->payloadlen);
				flag_arrived = FLG_FAN;
				new_fan = (uint16_t)(atof(tmp_buf));
			}

}

void trace(int nb, const char* buf)
{
	if(nb < 0)
	{
		//HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 0xFFFF);
	}
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{

	switch((uint32_t)UartHandle->Instance)
	{
		case (uint32_t)USART1:


		  if(m1.rxIndex <_MODBUS_MES_SIZE - 1)
		      {
			  	m1.rxIndex++;
			    HAL_UART_Receive_IT(&huart1, &m1.rxBuf[m1.rxIndex],1);


		      }
		      else
		      {
		    	  HAL_UART_Receive_IT(&huart1, &b1,1);
		      }
		    	m1.rxTime = HAL_GetTick();

		break;

		case (uint32_t)USART2:

				if(m2.rxIndex <_MODBUS_MES_SIZE - 1)
				{
					m2.rxIndex++;
					HAL_UART_Receive_IT(&huart2, &m2.rxBuf[m2.rxIndex],1);


				}
				else
				{
					HAL_UART_Receive_IT(&huart2, &b2,1);
				}
				m2.rxTime = HAL_GetTick();
				//xQueueSendFromISR( RecQHandleHandle, &m, &xHigherPriorityTaskWoken );
		break;


	}
}

uint16_t receiveRaw(UART_HandleTypeDef *huart, uint32_t timeout)
{
  uint32_t startTime = HAL_GetTick();
  while(1)
  {
    if(HAL_GetTick() - startTime > timeout)
      return 0;
    if(huart == &huart1)
    {
    	if((m1.rxIndex > 0) && (HAL_GetTick() - m1.rxTime > 2))
      return m1.rxIndex;
    }
    else if(huart == &huart2)
    {
    	if((m2.rxIndex > 0) && (HAL_GetTick() - m2.rxTime > 2))
      return m2.rxIndex;
    }
    HAL_Delay(1);
  }
}
//////////////CRC CALC/////////////////////
static const uint16_t wCRCTable[] =
{
  0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
  0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
  0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
  0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
  0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
  0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
  0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
  0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
  0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
  0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
  0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
  0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
  0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
  0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
  0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
  0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
  0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
  0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
  0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
  0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
  0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
  0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
  0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
  0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
  0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
  0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
  0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
  0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
  0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
  0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
  0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
  0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};
uint16_t modbus_crc16(const uint8_t *nData, uint16_t wLength)
{
  uint8_t nTemp;
  uint16_t wCRCWord = 0xFFFF;
  while (wLength--)
  {
    nTemp = *nData++ ^ wCRCWord;
    wCRCWord >>= 8;
    wCRCWord  ^= wCRCTable[nTemp];
  }
  return wCRCWord;
}
/////////////////////////////////
uint8_t set_setpoint(uint16_t setpoint)
{
	if(setpoint < 0 || setpoint > 350)
	{
		return 0;
	}
	uint8_t txData[8];
	uint8_t recByte=0;
	uint8_t equal = 0;
	txData[0] = 0x01;//slave address
	txData[1] = 0x06; // function code
	txData[2] = 0x00;// Hi byte of register address
	txData[3] = 0x90;// Low byte of register address
	txData[4] = (setpoint & 0xFF00) >> 8;// Hi byte of register value
	txData[5] = (setpoint & 0x00FF);// Low byte of register address
	static uint16_t  crc;
	crc = modbus_crc16(txData, 6);
	txData[6] = (crc & 0x00FF);
	txData[7] = (crc & 0xFF00) >> 8;
	HAL_UART_Transmit(&huart2, txData, 8 , 160);
	recByte = receiveRaw(&huart2,100);
	if(recByte==8)
	{

		for(int i=0;i<recByte; ++i)
		{
			if(m2.rxBuf[i] != txData[i])
			{
				equal = 1;
				break;
			}
		}
	}
	m2.rxIndex = 0;
	HAL_UART_Receive_IT(&huart2, &m2.rxBuf[m2.rxIndex],1);
	return equal;
}
uint8_t set_state(uint16_t state)
{
	if(state < 0 || state >2)
	{
		return 0;
	}
	uint8_t txData[8];
	uint8_t recByte=0;
	uint8_t equal = 0;
	txData[0] = 0x01;//slave address
	txData[1] = 0x06; // function code
	txData[2] = 0x00;// Hi byte of register address
	txData[3] = 0x8F;// Low byte of register address
	txData[4] = (state & 0xFF00) >> 8;// Hi byte of register value
	txData[5] = (state & 0x00FF);// Low byte of register address
	static uint16_t  crc;
	crc = modbus_crc16(txData, 6);
	txData[6] = (crc & 0x00FF);
	txData[7] = (crc & 0xFF00) >> 8;
	HAL_UART_Transmit(&huart2, txData, 8 , 160);
	recByte = receiveRaw(&huart2,100);
	if(recByte==8)
	{

		for(int i=0;i<recByte; ++i)
		{
			if(m2.rxBuf[i] != txData[i])
			{
				equal = 1;
				break;
			}
		}
	}
	m2.rxIndex = 0;
	HAL_UART_Receive_IT(&huart2, &m2.rxBuf[m2.rxIndex],1);
	return equal;
}
uint8_t set_fan(uint16_t fan)
{
	if(fan < 1 || fan >3)
	{
		return 0;
	}
	uint8_t txData[8];
	uint8_t recByte=0;
	uint8_t equal = 0;
	txData[0] = 0x01;//slave address
	txData[1] = 0x06; // function code
	txData[2] = 0x00;// Hi byte of register address
	txData[3] = 0x8D;// Low byte of register address
	txData[4] = (fan & 0xFF00) >> 8;// Hi byte of register value
	txData[5] = (fan & 0x00FF);// Low byte of register address
	static uint16_t  crc;
	crc = modbus_crc16(txData, 6);
	txData[6] = (crc & 0x00FF);
	txData[7] = (crc & 0xFF00) >> 8;
	HAL_UART_Transmit(&huart2, txData, 8 , 160);
	recByte = receiveRaw(&huart2,100);
	if(recByte==8)
	{

		for(int i=0;i<recByte; ++i)
		{
			if(m2.rxBuf[i] != txData[i])
			{
				equal = 1;
				break;
			}
		}
	}
	m2.rxIndex = 0;
	HAL_UART_Receive_IT(&huart2, &m2.rxBuf[m2.rxIndex],1);
	return equal;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  m1.rxIndex = 0;
  HAL_UART_Receive_IT(&huart1, &m1.rxBuf[m1.rxIndex],1);
  m2.rxIndex = 0;
  HAL_UART_Receive_IT(&huart2, &m2.rxBuf[m2.rxIndex],1);
  reg_wizchip_cs_cbfunc(cs_sel, cs_desel);
  reg_wizchip_spi_cbfunc(spi_rb, spi_wb);
  reg_wizchip_spiburst_cbfunc(W5500_ReadBuff, W5500_WriteBuff);
  uint8_t bufSize[] = {2, 2, 2, 2, 2, 2, 2, 2};
  wizchip_init(bufSize, bufSize);
  ////////////////////////////////////////////////////////////
  // set MAC address before using DHCP
      setSHAR(gWIZNETINFO.mac);
      DHCP_init(DHCP_SOCKET, dhcp_buffer);

      //UART_Printf("Registering DHCP callbacks...\r\n");
      reg_dhcp_cbfunc(
          Callback_IPAssigned,
          Callback_IPAssigned,
          Callback_IPConflict
      );

      //UART_Printf("Calling DHCP_run()...\r\n");
      // actually should be called in a loop, e.g. by timer
      uint32_t ctr = 10000;
      while((!ip_assigned) && (ctr > 0)) {
          DHCP_run();
          HAL_Delay(200);
          ctr--;
      }
     /* if(!ip_assigned) {
          //UART_Printf("\r\nIP was not assigned :(\r\n");
          return;
      }*/

      getIPfromDHCP(gWIZNETINFO.ip);
      getGWfromDHCP(gWIZNETINFO.gw);
      getSNfromDHCP(gWIZNETINFO.sn);

      /*uint8_t dns[4];
      getDNSfromDHCP(dns);

      UART_Printf("IP:  %d.%d.%d.%d\r\nGW:  %d.%d.%d.%d\r\nNet: %d.%d.%d.%d\r\nDNS: %d.%d.%d.%d\r\n",
          net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3],
          net_info.gw[0], net_info.gw[1], net_info.gw[2], net_info.gw[3],
          net_info.sn[0], net_info.sn[1], net_info.sn[2], net_info.sn[3],
          dns[0], dns[1], dns[2], dns[3]
      );

      UART_Printf("Calling wizchip_setnetinfo()...\r\n");
      wizchip_setnetinfo(&net_info);*/
  ///////////////////////////////////////////////////////////
  wizchip_setnetinfo(&gWIZNETINFO);
  wizchip_getnetinfo(&gWIZNETINFO);

  	Network n;
  	MQTTClient c;
  	unsigned char buf[100];
  	int rc = 0;
  	NewNetwork(&n, TCP_SOCKET);
  	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  	uint32_t startTime;
m1:	startTime = HAL_GetTick();//today
  	do{
  		ConnectNetwork(&n, targetIP, targetPort);
  		MQTTClientInit(&c,&n,1000,buf,100,tempBuffer,2048);


  		data.willFlag = 0;
  		data.MQTTVersion = 3;
  		data.clientID.cstring = opts.clientid;
  		data.username.cstring = opts.username;
  		data.password.cstring = opts.password;

  		data.keepAliveInterval = 60;
  		data.cleansession = 1;

  		rc = MQTTConnect(&c, &data);
  		if((HAL_GetTick() - startTime) > 10)//today
  		{
  			break;
  		}
  		//memset(msg,0,60);
  		//sprintf(msg,"Connected %d\r\n", rc);
  		//trace(-1, (const char*)msg);
  	}while(rc);
  	opts.showtopics = 1;

  	//memset(msg,0,60);
  	//sprintf(msg,"Subscribing to %s\r\n", "hello/wiznet");
  	//trace(-1, (const char*)msg);
  	if(!rc)
  	{
		startTime = HAL_GetTick();//today
  		do{
			rc = MQTTSubscribe(&c, "Vent1/set/#", opts.qos, messageArrived);
			//memset(msg,0,60);
			//sprintf(msg,"Subscribed %d\r\n", rc);
			//trace(-1, (const char*)msg);
			if((HAL_GetTick() - startTime) > 10)//today
			{
			 	break;
			}
		}while(rc);
	}
  	if(rc){
  		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
  	}else{
  		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  	}
  	MQTTMessage mes;
  	char payload[32]={0};
  	mes.payload = payload;
  	sprintf(payload, "IP:%3d.%3d.%3d.%3d", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
  	//memcpy(payload,"Connected!",strlen("Connected!"));
  	mes.payloadlen = 19;//strlen("Connected!");
  	if(!rc)//today
  	{
  		MQTTPublish(&c, "hello/stm32", &mes);
  	}
  	mes.qos = QOS0;
  	uint32_t count =0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  //MQTTPublish(&c, "hello/stm32", &mes);
	  //HAL_Delay(1000);
	  uint16_t recByte = receiveRaw(&huart1,100);
	  if(recByte){
		  if(!rc)
		  {
			  if(m1.rxBuf[0] == 0x01 && m1.rxBuf[1] == 0x06 && m1.rxBuf[2] == 0x00 && m1.rxBuf[3] == 0xB5)
			  {
				  sprintf(payload, "%d.%d", ((int16_t) (m1.rxBuf[4] << 8) | m1.rxBuf[5])/10, ((int16_t) (m1.rxBuf[4] << 8) | m1.rxBuf[5])%10);
				  mes.payloadlen = strlen((const char*)payload);
				  rc = MQTTPublish(&c, "Vent1/Ts", &mes);
			  }
			  if(m1.rxBuf[0] == 0x01 && m1.rxBuf[1] == 0x06 && m1.rxBuf[2] == 0x00 && m1.rxBuf[3] == 0xB9)
					  {
						  sprintf(payload, "%d.%d", ((int16_t) (m1.rxBuf[4] << 8) | m1.rxBuf[5])/10, ((int16_t) (m1.rxBuf[4] << 8) | m1.rxBuf[5])%10);
						  mes.payloadlen = strlen((const char*)payload);
						  rc = MQTTPublish(&c, "Vent1/Tf", &mes);
					  }
			  if(m1.rxBuf[0] == 0x01 && m1.rxBuf[1] == 0x06 && m1.rxBuf[2] == 0x00 && m1.rxBuf[3] == 0xC2)
					  {
						  sprintf(payload, "%d.%d", ((int16_t) (m1.rxBuf[4] << 8) | m1.rxBuf[5])/10,((int16_t) (m1.rxBuf[4] << 8) | m1.rxBuf[5])%10 );
						  mes.payloadlen = strlen((const char*)payload);
						  rc = MQTTPublish(&c, "Vent1/To", &mes);
					  }
			  if(m1.rxBuf[0] == 0x01 && m1.rxBuf[1] == 0x06 && m1.rxBuf[2] == 0x00 && m1.rxBuf[3] == 0x9D)
					  {
						  sprintf(payload, "%d.%d", ((int16_t) (m1.rxBuf[4] << 8) | m1.rxBuf[5])/10, ((int16_t) (m1.rxBuf[4] << 8) | m1.rxBuf[5])%10);
						  mes.payloadlen = strlen((const char*)payload);
						  rc = MQTTPublish(&c, "Vent1/Tlcd", &mes);
					  }
			  if(m1.rxBuf[0] == 0x01 && m1.rxBuf[1] == 0x06 && m1.rxBuf[2] == 0x00 && m1.rxBuf[3] == 0xC6)
					  {
						  sprintf(payload, "%d", (int16_t) (m1.rxBuf[4] << 8) | m1.rxBuf[5]);
						  mes.payloadlen = strlen((const char*)payload);
						  rc = MQTTPublish(&c, "Vent1/Alarm", &mes);
					  }
		  }
		  HAL_UART_Transmit(&huart2, (uint8_t *)&m1.rxBuf[0], m1.rxIndex , 160);
		  m1.rxIndex = 0;
		  HAL_UART_Receive_IT(&huart1, &m1.rxBuf[m1.rxIndex],1);
	  }
	  recByte = receiveRaw(&huart2,100);
	  if(recByte)
	  {
		  if(!rc)
		  {
				  if(m2.rxBuf[0] == 0x01 && m2.rxBuf[1] == 0x03 && m2.rxBuf[2] == 0x1C)
					  {
						  sprintf(payload, "%d.%d", ((int16_t) (m2.rxBuf[29] << 8) | m2.rxBuf[30])/10,((int16_t) (m2.rxBuf[29] << 8) | m2.rxBuf[30])%10);
						  mes.payloadlen = strlen((const char*)payload);
						  rc = MQTTPublish(&c, "Vent1/Tset", &mes);
					  }
				if(m2.rxBuf[0] == 0x01 && m2.rxBuf[1] == 0x03 && m2.rxBuf[2] == 0x1C)
					  {
						  sprintf(payload, "%d", (int16_t) (m2.rxBuf[27] << 8) | m2.rxBuf[28]);
						  mes.payloadlen = strlen((const char*)payload);
						  rc = MQTTPublish(&c, "Vent1/State", &mes);
					  }
				if(m2.rxBuf[0] == 0x01 && m2.rxBuf[1] == 0x03 && m2.rxBuf[2] == 0x1C)
					  {
						  sprintf(payload, "%d", (int16_t) (m2.rxBuf[23] << 8) | m2.rxBuf[24]);
						  mes.payloadlen = strlen((const char*)payload);
						  rc = MQTTPublish(&c, "Vent1/Fan", &mes);
					  }
				if(m2.rxBuf[0] == 0x01 && m2.rxBuf[1] == 0x03 && m2.rxBuf[2] == 0x1C)
					  {
						  sprintf(payload, "%d", (int16_t) (m2.rxBuf[21] << 8) | m2.rxBuf[22]);
						  mes.payloadlen = strlen((const char*)payload);
						  rc = MQTTPublish(&c, "Vent1/Mode", &mes);
					  }
		  }
	  	  HAL_UART_Transmit(&huart1, (uint8_t *)&m2.rxBuf[0], m2.rxIndex , 160);
	  	  m2.rxIndex = 0;
	  	  HAL_UART_Receive_IT(&huart2, &m2.rxBuf[m2.rxIndex],1);
	  }
	  if(!rc) MQTTYield(&c, data.keepAliveInterval);//today
	  if(flag_arrived == FLG_SETPOINT)
	  {
		  if(set_setpoint(new_setpoint) == 0)
		  {
			  flag_arrived =0;
		  }
	  }
	  else if(flag_arrived == FLG_STATE)
	  {
		  if(set_state(new_state) == 0)
		  {
			  flag_arrived =0;
		  }
	  }
	  else if(flag_arrived == FLG_FAN)
	  	  {
	  		  if(set_fan(new_fan) == 0)
	  		  {
	  			  flag_arrived =0;
	  		  }
	  	  }
	  ++count;
	  if(rc){
	    		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
	    	}else{
	    		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
	    	}
	  if(rc && (count > 255)) goto m1;//today 2^25

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

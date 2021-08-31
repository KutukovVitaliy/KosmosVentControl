/*
 * tcp.c
 *
 *  Created on: Feb 28, 2021
 *      Author: kvit
 */
//#include "main.h"
//#include "stm32f4xx_hal.h"
//#include "usart.h"
// внимание: это никакой не системный файл, а входит в состав библиотеки W5500
//#include "socket.h"
//#include <string.h>
//char msg[60];
// это будем посылать tcp клиенту, когда он к нам приконнектится
//const char MSG[] = "Hello World";
//const char MSGRx[] = "Received: ";


// простой tcp сервер. Данные не принимаем, только посылаем строчку Hello World.
// вы можете самостоятельно дополнить функцию для получения данных от клиента
//void tcp_server()
//{
  /*uint8_t retVal, sockStatus, size, retVal1;

  // а вот это как раз моя функция вывода сообщений через модуль UART-USB
  // если вместо -1 поставить число, она и его отобразит.
  // в недрах этой функции работающая строка будет
  // HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 0xFFFF);
  trace(-1,"Try open socket\r\n");

  // открываем сокет 0 как TCP_SOCKET, порт 5000
  if((retVal = socket(0, Sn_MR_TCP, 5000, 0)) != 0)
  {
    trace(-1, "Error open socket\r\n");
    return;
  }
  trace(-1,"Socket opened, try listen\r\n");

  // устанавливаем сокет в режим LISTEN. Так будет создан tcp сервер
  if((retVal = listen(0)) != SOCK_OK)
  {
    trace(-1, "Error listen socket\r\n");
    return;
  }

  trace(-1,"Socked listened, wait for input connection\r\n");
  // ждем входящих соединений. здесь мы немножко крутимся в бесконечном цикле
  // и чтобы не заскучать одновременно мигаем светодиодом
  while((sockStatus = getSn_SR(0)) == SOCK_LISTEN)
  {
    HAL_Delay(200);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
  }

  // раз мы попали сюда, значит выскочили из цикла. входящее соединение!
  trace(-1,"Input connection\r\n");

  if((sockStatus = getSn_SR(0)) != SOCK_ESTABLISHED)
  {
    trace(-1, "Error socket status\r\n");
    return;
  }
  // из сокета вытаскиваем информацию: кто к нам пришел, откуда
  // можете также отобразить инфу в трассировке
  uint8_t remoteIP[4];
  uint16_t remotePort;
  getsockopt(0, SO_DESTIP, remoteIP);
  getsockopt(0, SO_DESTPORT, (uint8_t*)&remotePort);
  uint8_t buf[128];
  // посылаем клиенту приветствие и закрываем сокет
  if((retVal = send(0, (uint8_t*)MSG, strlen(MSG))) == (int16_t)strlen(MSG))
  {
    trace(-1, "Msg sent\r\n");
  }
  else
    trace(-1, "Error socket send\r\n");
  while(1)
  {
	  if((size = getSn_RX_RSR(0)) > 0) // Sn_RX_RSR: Socket n Received Size Register, Receiving data length
	  {
		  retVal = recv(0, buf, size); // Data Receive process (H/W Rx socket buffer -> User's buffer)
		  if(retVal <= 0)  // If the received data length <= 0, receive failed and process end
		  {
			  trace(-1, "Error socket recv\r\n");
		  }
		  HAL_UART_Transmit(&huart1, (uint8_t*)buf, retVal, 0xFFFF);
		  if((retVal1 = send(0, (uint8_t*)MSGRx, strlen(MSGRx))) != (int16_t)strlen(MSGRx))
		    {
		      trace(-1, "Error socket send\r\n");
		    }
		  if((retVal1 = send(0, (uint8_t*)buf, retVal)) != retVal)
		  	{
			  trace(-1, "Error socket send\r\n");
		  	}
		  if(buf[0]=='9' && buf[1]=='9' && buf[2]== '9' && buf[3]=='9')
		  {
			  break;
		  }
	  }
  }
  // закрываемся. когда нас снова вызовут, мы всегда готовы кработе
  disconnect(0);
  close(0);*/
//}



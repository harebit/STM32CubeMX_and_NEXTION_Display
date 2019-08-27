/**
 * @file NexHardware.cpp
 *
 * The implementation of base API for using Nextion. 
 *
 * @author  Wu Pengfei (email:<pengfei.wu@itead.cc>)
 * @date    2015/8/11
 * @copyright 
 * Copyright (C) 2014-2015 ITEAD Intelligent Systems Co., Ltd. \n
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */
#include "NexHardware.h"
#include "stm32f1xx_hal.h"
#include "string"

using namespace std;

#define NEX_RET_CMD_FINISHED            	(0x01) // used
#define NEX_RET_EVENT_LAUNCHED          	(0x88)
#define NEX_RET_EVENT_UPGRADED          	(0x89)
#define NEX_RET_EVENT_TOUCH_HEAD            (0x65) // used
#define NEX_RET_EVENT_POSITION_HEAD         (0x67)
#define NEX_RET_EVENT_SLEEP_POSITION_HEAD   (0x68)
#define NEX_RET_CURRENT_PAGE_ID_HEAD        (0x66)
#define NEX_RET_STRING_HEAD                 (0x70) // used
#define NEX_RET_NUMBER_HEAD                 (0x71) // used

#define NEX_RET_INVALID_CMD             (0x00)
#define NEX_RET_INVALID_COMPONENT_ID    (0x02)
#define NEX_RET_INVALID_PAGE_ID         (0x03)
#define NEX_RET_INVALID_PICTURE_ID      (0x04)
#define NEX_RET_INVALID_FONT_ID         (0x05)
#define NEX_RET_INVALID_BAUD            (0x11)
#define NEX_RET_INVALID_VARIABLE        (0x1A)
#define NEX_RET_INVALID_OPERATION       (0x1B)

uint8_t Rx_indx;
volatile uint8_t Transfer_len, cnt_0xff;
char Rx_data[2];
volatile char Transfer_cplt;
volatile char Rx_Buffer[64], Transfer_Buffer[64];
volatile uint32_t timeout, timeoutcallback, time;

/*
 * Receive uint32_t data. 
 * 
 * @param number - save uint32_t data. 
 * @param timeout - set timeout time. 
 *
 * @retval true - success. 
 * @retval false - failed.
 *
 */
bool recvRetNumber(uint32_t *number)
{
    bool ret = false;

    if(*number >=0) // check argument
    {
    	timeout = 100000;
    	while (timeout)
		{
			timeout--;
    		if (Transfer_cplt == 1) // receive complete
			{

				if (Transfer_Buffer[0] == NEX_RET_NUMBER_HEAD) // (0x71 = 113) Returned when get command to return a number
				{
					*number = ((uint32_t)Transfer_Buffer[4] << 24) | ((uint32_t)Transfer_Buffer[3] << 16) | (Transfer_Buffer[2] << 8) | (Transfer_Buffer[1]);
					ret = true;
					timeout = 0;
				}
				else
				{
					timeout = 0;
				}
			}
		}
    }
	Transfer_cplt = 0;
    return ret;
}


/*
 * Receive string data. 
 * 
 * @param buffer - save string data. 
 * @param len - string buffer length. 
 * @param timeout - set timeout time. 
 *
 * @return the length of string buffer.
 *
 */

uint16_t recvRetString(char *buffer, uint16_t len)
{
    uint16_t ret = false;
    uint8_t i;
    string temp = "";

    if (buffer > 0 && len > 0) // check argument
    {
    	timeout = 100000;
    	while (timeout)
        {
			timeout--;
    		if (Transfer_cplt == 1) // receive complete
			{
				if(NEX_RET_STRING_HEAD == Transfer_Buffer[0]	// (0x70 = 112) Returned when using get command for a string. Each byte is converted to char
				   && len >= (Transfer_len-4)) 			// Transfer_len without 0x70 .... and 0xff,0xff,0xff
				{
					for (i=1; i<(Transfer_len-3);  i++)
					{
						temp += (char)Transfer_Buffer[i];
					}
					ret = temp.length();
					strncpy(buffer, temp.c_str(), ret);
					timeout = 0;
				}
				else
				{
					timeout = 0;
				}
    		}
        }
    }
    Transfer_cplt=0;
    return ret;
}

/*
 * Send command to Nextion.
 *
 * @param cmd - the string of command.
 */
void sendCommand(const char* cmd)
{
	uint8_t len;
	char cmd_buff[63];

	len=sprintf(cmd_buff,"%s%s",cmd,"ÿÿÿ"); //sprintf will return the length of 'buffer'

	HAL_UART_Transmit(&huart1, (uint8_t *)cmd_buff, len, 10);
	HAL_UART_Receive_IT(&huart1, (uint8_t*)Rx_data, 1);   //activate UART receive interrupt
}


/*
 * Command is executed successfully. 
 *
 * @param timeout - set timeout time.
 *
 * @retval true - success.
 * @retval false - failed. 
 *
 */
bool recvRetCommandFinished(void)
{    
    bool ret = false;
    
    timeout = 100000;
    while (timeout)
    {
    	timeout--;
    	if(Transfer_cplt == 1)
		{

				if (Transfer_Buffer[0] == NEX_RET_CMD_FINISHED	//(0x01 = 1) Returned when instruction sent by user was successful
					&& 4 == Transfer_len)						// 0x01 0xff 0xff 0xff = 4byte
				{
					ret = true;
					timeout = 0;
				}
				else
				{
					timeout = 0;
				}

		}
/*		if (ret == true)
		{
			break;
		}*/
    }
    Transfer_cplt=0;
    return ret;
}


bool nexInit(void)
{
    bool ret1 = false;
    bool ret2 = false;
    
	Transfer_cplt = 0;
	Rx_indx = 0;
	cnt_0xff = 0;

    sendCommand("");
    HAL_Delay(100);
    recvRetCommandFinished();
    sendCommand("bkcmd=1");
    ret1 = recvRetCommandFinished();
    sendCommand("page 0");
    ret2 = recvRetCommandFinished();
    return ret1 && ret2;



}

void nexLoop(NexTouch *nex_listen_list[])
{
	bool ret = false;

	if (Transfer_cplt == 1)
	{
		if (Transfer_Buffer[0] == NEX_RET_EVENT_TOUCH_HEAD  // (0x65 = 101) Returned when Touch occurs and component’s corresponding Send Component ID is checked in the users HMI design.
			 && Transfer_len ==7)
		{
			Transfer_cplt = 0; //unblocking buffer of Transfer_Buffer
			NexTouch::iterate(nex_listen_list, Transfer_Buffer[1], Transfer_Buffer[2], (int32_t)Transfer_Buffer[3]);
		}
		else
		{
			Transfer_cplt = 0; //unblocking buffer of Transfer_Buffer
			sendCommand("sys1=0");
			ret += recvRetCommandFinished();
			sendCommand("page3.n1.val=0");
			ret += recvRetCommandFinished();
		}

	}
}

//Interrupt callback routine
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uint8_t i;
	bool ret = false;

    if (huart->Instance == USART1)  //current UART
    {
        if (Rx_indx==0) // init
		{
			//for (i=0;i<(sizeof(Rx_Buffer));i++) Rx_Buffer[i]=0; //clear Rx_Buffer before receiving new data
			timeoutcallback = 100000;
		}
        if (timeoutcallback)  // timelimit
        {
        timeoutcallback--;
        Rx_Buffer[Rx_indx++]=Rx_data[0];    //add data to Rx_Buffer

			if (Rx_data[0] == 'ÿ') //if received data equal to ascii 255 (ÿ)
			{
				cnt_0xff++;

				   if (cnt_0xff == 3 && (
						   Rx_Buffer[0] == NEX_RET_CMD_FINISHED ||			// (0x01 = 1) successful
						   Rx_Buffer[0] == NEX_RET_EVENT_TOUCH_HEAD ||		// (0x65 = 101) touch
						   Rx_Buffer[0] == NEX_RET_STRING_HEAD )			// (0x70 = 112) string
				   	   	   )
				   {
					   if(Transfer_cplt==0)
					   {
						   for (i=0;i<Rx_indx;i++)
						   {
							   Transfer_Buffer[i]=Rx_Buffer[i];
						   }

					   Transfer_len = Rx_indx;
					   Transfer_cplt=1;
					   Rx_indx = 0;
					   cnt_0xff = 0;

					   }
				   }
				   else if(cnt_0xff == 3 && Rx_Buffer[0] == NEX_RET_NUMBER_HEAD)	// (0x71 = 113) number) // 0x71 0x01 0x02 0x03 0x04 0xFF 0xFF 0xFF = 8byte, number has 4 byte with 32-bit value in little endian order
				   {
					   if (Rx_indx == 8)
					   {
						   if(Transfer_cplt==0)
						   {
							   for (i=0;i<Rx_indx;i++)
							   {
								   Transfer_Buffer[i]=Rx_Buffer[i];
							   }

						   Transfer_len = Rx_indx;
						   Transfer_cplt=1;
						   Rx_indx = 0;
						   cnt_0xff = 0;

						   }
					   }
					   else
					   {
						   cnt_0xff--;
					   }
				   }
				   else if (cnt_0xff >= 3)// transfer was not successful
				   {
						Transfer_cplt=1;
						Rx_indx = 0;
						cnt_0xff = 0;
						sendCommand("sys1=0");
						ret += recvRetCommandFinished();
						sendCommand("page3.n1.val=0");
						ret += recvRetCommandFinished();
				   }
			}
			else
			{
				cnt_0xff=0; // only if 3 terminators in serie
			}
        }
        else //termination by time
        {
			cnt_0xff = 0;
			Rx_indx = 0;
        }
        HAL_UART_Receive_IT(&huart1, (uint8_t*)Rx_data, 1);   //activate UART receive interrupt every time
    }
 }

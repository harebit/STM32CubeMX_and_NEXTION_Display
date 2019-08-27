/*
 * example.cpp
 *
 *  Created on: 03.03.2019
 *      Author: harebit
 */



#include "example.h"

/*
 * Declare a text object [page id:2,component id:4, component name: "t3"].
 */
NexText p2t3 = NexText(2, 4, "page2.t3");

/*
 * Declare a button object [page id:2,component id:5, component name: "b0"].
 */
NexButton p2b0 = NexButton(2, 5, "b0");

/*
 * Declare a number object [page id:3,component id:4, component name: "n0"].
 */
NexNumber p3n0 = NexNumber(3, 4, "page3.n0");

/*
 * Declare a button object [page id:3,component id:5, component name: "b0"].
 */
NexButton p3b0 = NexButton(3, 5, "b0");

char buffer[64] = {0};


/*
 * Register object p2b0 and p3b0 to the touch event list.
 */
NexTouch *nex_listen_list[] =
{
  	&p2b0,
    &p3b0,
    NULL
};

/*
 * Button0 component on page 2 pop callback function.
 * In this example,the value of the text component will plus one every time when button0 is released.
 */
void p2b0PopCallback(void *ptr)
{
    uint16_t len, i;
    uint8_t ret = 0;
    char greeting[] = "Hello World";

    memset(buffer, 0, sizeof(buffer));
    len = p2t3.getText(buffer, sizeof(buffer));

 // len can be 0 and 1024, there is no possibility to catch an error

		len += 1;
		memset(buffer, 0, sizeof(buffer));

		if (len >= sizeof(greeting))
		{
			len = 0;
		}
		for (i=0;i<len;i++)
		{
			buffer[i] = greeting[i];
		}

		p2t3.setText(buffer);
		sendCommand("sys1=0");
		ret += recvRetCommandFinished();
		sendCommand("page3.n1.val=0");
		ret += recvRetCommandFinished();

	if (ret<2) HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}

/*
 * Button0 component on page 3 pop callback function.
 * In this example,the value of the number component will plus one every time when button0 is released.
 */
void p3b0PopCallback(void *ptr)
{
    uint32_t number;
    uint8_t ret = 0;

    ret = p3n0.getValue(&number);

    if (ret == 1) // all others are wrong
    {
		if (number >= 255)
		{
			number = 0;
		}
		else
		{
			number += 5;
		}
		ret += p3n0.setValue(number);
		sendCommand("sys1=0");
		ret += recvRetCommandFinished();
		sendCommand("page3.n1.val=0");
		ret += recvRetCommandFinished();
    }
    else
    {

		sendCommand("sys1=0");
		ret += recvRetCommandFinished();
		sendCommand("page3.n1.val=0");
		ret += recvRetCommandFinished();
    }
    if (ret != 4) 	HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}




void setup(void)
{
	uint8_t ret = 0;

    /* Set communication with Nextion screen. */
    ret = nexInit();
	if (ret == 0) HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

    /* set text component. */
    ret += p2t3.setText("");

    /* Register the pop event callback function of the current button0 component. */
    p2b0.attachPop(p2b0PopCallback);

    /* set number component. */
    ret += p3n0.setValue(0);

    /* Register the pop event callback function of the current button0 component. */
    p3b0.attachPop(p3b0PopCallback);

    if (ret<=3)	HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}

void loop(void)
{
    nexLoop(nex_listen_list);
}



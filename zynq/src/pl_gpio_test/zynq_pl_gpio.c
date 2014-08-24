/*
 * Copyright (c) 2009-2012 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

/***************************** Include Files *********************************/
#include "xparameters.h"
#include "xgpio.h"
#include "sleep.h"

/************************** Variable Defintions ******************************/
XGpio Gpio_leds;
XGpio Gpio_keys;
/* Instance For GPIO */
XGpio GpioOutput;

int pl_gpio_key_init ()
{

    int Status;
    u32 DataRead;
    int i = 0;
    
    Status = XGpio_Initialize(&Gpio_keys, XPAR_KEY_IO_DEVICE_ID);
    if (Status != XST_SUCCESS) 
    {
        return XST_FAILURE;
    }

    XGpio_SetDataDirection(&Gpio_keys, 1, 0xFFFFFFFF);

    Status = XGpio_Initialize(&Gpio_leds, XPAR_LED_IO_DEVICE_ID);
    if (Status != XST_SUCCESS) 

    {
        return XST_FAILURE;
    }

    XGpio_SetDataDirection(&Gpio_leds, 1, 0x0);

    printf("\r\n");
    for (i = 0; i < 10; i++)
    {
        printf("****** 10 times testing, you have the %d times left\r\n", (9-i));
        DataRead = XGpio_DiscreteRead(&Gpio_keys, 1);
        sleep(1);
        printf("Gpio_keys:0x%x\r\n",DataRead);
        XGpio_DiscreteWrite(&Gpio_leds, 1, DataRead);
    }
    
    return 0;
}



int pl_gpio_led_init()
{

	u32 Delay;
	u32 Ledwidth;
    int i = 0;
    XGpio_Initialize(&GpioOutput, XPAR_LED_IO_DEVICE_ID);
    XGpio_SetDataDirection(&GpioOutput, 1, 0x0);
    XGpio_DiscreteWrite(&GpioOutput, 1, 0x0);
    
    for (i = 0; i < 3; i++)
    {
        for (Ledwidth = 0x0; Ledwidth < 5; Ledwidth++)
        {
             XGpio_DiscreteWrite(&GpioOutput, 1, 1 << Ledwidth);
             usleep(500000);
             XGpio_DiscreteClear(&GpioOutput, 1, 1 << Ledwidth);
        }
    }
    
    return 0;
}


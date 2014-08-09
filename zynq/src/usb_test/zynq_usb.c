/*
 * Driver for the Zynq-7000 PSS I2C controller
 * IP from Cadence (ID T-CS-PE-0007-100, Version R1p10f2)
 *
 * Author: Joe Hershberger <joe.hershberger@ni.com>
 * Copyright (c) 2012 Joe Hershberger.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#include <common.h>
#include <asm/errno.h>

#include "xusbps_ch9.h"
#include "xusbps.h"


#include "xdevcfg_hw.h"
#include "xdevcfg.h"
#include "fsbl.h"
#include "xil_io.h"


#include <malloc.h>


static XUsbPs UsbInstance;	/* The instance of the USB Controller */
u8 Buffer[65535];


void usb_phy_init()
{
    
	int	Status;
	XUsbPs_Config		*UsbConfigPtr;
    /* printf("usb_phy_init begin***\n"); */
	u8	*MemPtr = NULL;
    
	u32	MemSize;
    XUsbPs_DeviceConfig  DeviceConfig;
    u16 UsbDeviceId = XPAR_XUSBPS_0_DEVICE_ID;
    XUsbPs *UsbInstancePtr = &UsbInstance;
    
	const u8 NumEndpoints = 2;
    
	UsbConfigPtr = XUsbPs_LookupConfig(UsbDeviceId);
	if (NULL == UsbConfigPtr) {  
        printf("usb_phy_init step1***\n");
		return;
	}
    
	Status = XUsbPs_CfgInitialize(UsbInstancePtr,
				       UsbConfigPtr,
				       UsbConfigPtr->BaseAddress);
	if (XST_SUCCESS != Status) {
        printf("usb_phy_init step2***\n");
		return;
	}

    
	DeviceConfig.EpCfg[0].Out.Type		= XUSBPS_EP_TYPE_CONTROL;
	DeviceConfig.EpCfg[0].Out.NumBufs	= 2;
	DeviceConfig.EpCfg[0].Out.BufSize	= 64;
	DeviceConfig.EpCfg[0].Out.MaxPacketSize	= 64;
	DeviceConfig.EpCfg[0].In.Type		= XUSBPS_EP_TYPE_CONTROL;
	DeviceConfig.EpCfg[0].In.NumBufs	= 2;
	DeviceConfig.EpCfg[0].In.MaxPacketSize	= 64;

	DeviceConfig.EpCfg[1].Out.Type		= XUSBPS_EP_TYPE_BULK;
	DeviceConfig.EpCfg[1].Out.NumBufs	= 16;
	DeviceConfig.EpCfg[1].Out.BufSize	= 512;
	DeviceConfig.EpCfg[1].Out.MaxPacketSize	= 512;
	DeviceConfig.EpCfg[1].In.Type		= XUSBPS_EP_TYPE_BULK;
	DeviceConfig.EpCfg[1].In.NumBufs	= 16;
	DeviceConfig.EpCfg[1].In.MaxPacketSize	= 512;

	DeviceConfig.NumEndpoints = NumEndpoints;
	MemSize = XUsbPs_DeviceMemRequired(&DeviceConfig);

	MemPtr = &Buffer[0];
	memset(&Buffer[0],0,65535);
	Xil_DCacheFlushRange((unsigned int)&Buffer,65535);


	DeviceConfig.DMAMemVirt = (u32) MemPtr;
	DeviceConfig.DMAMemPhys = (u32) MemPtr;

	Status = XUsbPs_ConfigureDevice(UsbInstancePtr, &DeviceConfig);
	if (XST_SUCCESS != Status) {
        printf("usb_phy_init step4***\n");
		return;
	}
    
	/* Enable the interrupts. */
	XUsbPs_IntrEnable(UsbInstancePtr, XUSBPS_IXR_UR_MASK |
					   XUSBPS_IXR_UI_MASK);

	/* Start the USB engine */
	XUsbPs_Start(UsbInstancePtr);
    
    /* printf("usb_phy_init end***\n"); */
    
    return;
}






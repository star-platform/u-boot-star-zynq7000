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

#include "hdmi_header.h"
#include <malloc.h>
#include "xiicps.h"
#include "i2c.h"

#define IIC_DEVICE_ID	XPAR_XIICPS_1_DEVICE_ID
#define INTC_DEVICE_ID	XPAR_SCUGIC_SINGLE_DEVICE_ID
#define IIC_INTR_ID	    XPAR_XIICPS_1_INTR
#define IIC_SCLK_RATE		100000


#define CF_CLKGEN_BASEADDR  XPAR_AXI_CLKGEN_0_BASEADDR
#define CFV_BASEADDR        XPAR_AXI_HDMI_TX_24B_0_BASEADDR
#define CFA_BASEADDR        0x75c00000
#define DDR_BASEADDR        0x00000000
#define UART_BASEADDR       0xe0001000
#define VDMA_BASEADDR       0x43000000
#define ADMA_BASEADDR       0x40400000
#define H_STRIDE            1920
#define H_COUNT             2200
#define H_ACTIVE            1920
#define H_WIDTH             44
#define H_FP                88
#define H_BP                148
#define V_COUNT             1125
#define V_ACTIVE            1080
#define V_WIDTH             5
#define V_FP                4
#define V_BP                36
#define A_SAMPLE_FREQ       48000
#define A_FREQ              1400

#define H_DE_MIN (H_WIDTH+H_BP)
#define H_DE_MAX (H_WIDTH+H_BP+H_ACTIVE)
#define V_DE_MIN (V_WIDTH+V_BP)
#define V_DE_MAX (V_WIDTH+V_BP+V_ACTIVE)
#define VIDEO_LENGTH  (H_ACTIVE*V_ACTIVE)
#define AUDIO_LENGTH  (A_SAMPLE_FREQ/A_FREQ)
#define VIDEO_BASEADDR DDR_BASEADDR + 0x2000000
#define AUDIO_BASEADDR DDR_BASEADDR + 0x1000000





void ddr_video_wr() {

  u32 n;
  u32 d;
  u32 dcnt;

  dcnt = 0;
  printf("DDR write: started (length %d)\n\r", IMG_LENGTH);
  for (n = 0; n < IMG_LENGTH; n++) {
    for (d = 0; d < ((IMG_DATA[n]>>24) & 0xff); d++) {
      Xil_Out32((VIDEO_BASEADDR+(dcnt*4)), (IMG_DATA[n] & 0xffffff));
      dcnt = dcnt + 1;
    }
  }
  //flush_cache((u32)IMG_DATA, 4*IMG_LENGTH);
  //flush_dcache_all();
  //Xil_DCacheFlush();
  printf("DDR write: completed (total %d)\n\r", dcnt);
}


int HDMI_init( void )
{
    u32 data;
    int Status;
    u8 ReadBuffer[2];

    
    si9134_i2c_init();

    /*for logicvc,*/
    ddr_video_wr();
        
    data = Xil_In32(CF_CLKGEN_BASEADDR + (0x1f*4));
    if ((data & 0x1) == 0x0) {
      printf("CLKGEN (148.5MHz) out of lock (0x%04x)\n\r", data);
      return(0);
    }
    
        
    printf("******before setting vdma, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
    	Xil_In32((VDMA_BASEADDR + 0x0)), Xil_In32((VDMA_BASEADDR + 0x5c)), Xil_In32((VDMA_BASEADDR + 0x60)),
        Xil_In32((VDMA_BASEADDR + 0x64)), Xil_In32((VDMA_BASEADDR + 0x58)), Xil_In32((VDMA_BASEADDR + 0x54)),
        Xil_In32((VDMA_BASEADDR + 0x50)));

	//**********configure VDMA**************//
	Xil_Out32((VDMA_BASEADDR + 0x000), 0x00000003); // enable circular mode
	Xil_Out32((VDMA_BASEADDR + 0x05c), VIDEO_BASEADDR); // start address
	Xil_Out32((VDMA_BASEADDR + 0x060), VIDEO_BASEADDR); // start address
	Xil_Out32((VDMA_BASEADDR + 0x064), VIDEO_BASEADDR); // start address
	Xil_Out32((VDMA_BASEADDR + 0x058), (H_STRIDE*4)); // h offset (2048 * 4) bytes
	Xil_Out32((VDMA_BASEADDR + 0x054), (H_ACTIVE*4)); // h size (1920 * 4) bytes
	Xil_Out32((VDMA_BASEADDR + 0x050), V_ACTIVE); // v size (1080)

    
    printf("******after setting vdma, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
    	Xil_In32((VDMA_BASEADDR + 0x0)), Xil_In32((VDMA_BASEADDR + 0x5c)), Xil_In32((VDMA_BASEADDR + 0x60)),
        Xil_In32((VDMA_BASEADDR + 0x64)), Xil_In32((VDMA_BASEADDR + 0x58)), Xil_In32((VDMA_BASEADDR + 0x54)),
        Xil_In32((VDMA_BASEADDR + 0x50)));
    

    

    printf("******before setting hdmi_tx_24b, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
    	Xil_In32((CFV_BASEADDR + 0x08)), Xil_In32((CFV_BASEADDR + 0x0c)), Xil_In32((CFV_BASEADDR + 0x10)),
        Xil_In32((CFV_BASEADDR + 0x14)), Xil_In32((CFV_BASEADDR + 0x04)));

    
    
	//**********configure axi_hdmi_tx_24b**************//
	Xil_Out32((CFV_BASEADDR + 0x08), ((H_WIDTH << 16) | H_COUNT));		//HSYNC Width
	Xil_Out32((CFV_BASEADDR + 0x0c), ((H_DE_MIN << 16) | H_DE_MAX));	//HSYNC DE MAX
	Xil_Out32((CFV_BASEADDR + 0x10), ((V_WIDTH << 16) | V_COUNT));		//VSYNC width
	Xil_Out32((CFV_BASEADDR + 0x14), ((V_DE_MIN << 16) | V_DE_MAX));	//VSYC DE MAX
	Xil_Out32((CFV_BASEADDR + 0x04), 0x00000002);						//bit2=0: disable test pattern,Bypass the CSC, Video output disable
	Xil_Out32((CFV_BASEADDR + 0x04), 0x00000003);						//Video output enable
    
    printf("******after setting hdmi_tx_24b, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
    	Xil_In32((CFV_BASEADDR + 0x08)), Xil_In32((CFV_BASEADDR + 0x0c)), Xil_In32((CFV_BASEADDR + 0x10)),
        Xil_In32((CFV_BASEADDR + 0x14)), Xil_In32((CFV_BASEADDR + 0x04)));
    
    
	printf("hdmi enable\n");

#if 0
    /**********************************************************/
    /*SiI9134 register initial                                */
    /**********************************************************/
      IicPsMasterPolled_Init(IIC_DEVICE_ID);
      
      IicPsMasterPolled_Write(0x39, 0x05, 0x01);        //soft reset
      IicPsMasterPolled_Write(0x39, 0x05, 0x00);        //soft reset
      IicPsMasterPolled_Write(0x39, 0x08, 0xfd);        //PD#=1,power on mode

      IicPsMasterPolled_Write(0x3d, 0x2f, 0x21);        //HDMI mode enable, 24bit per pixel(8 bits per channel; no packing)
      IicPsMasterPolled_Write(0x3d, 0x3e, 0x03);        //Enable AVI infoFrame transmission, Enable(send in every VBLANK period)
      IicPsMasterPolled_Write(0x3d, 0x40, 0x82);
      IicPsMasterPolled_Write(0x3d, 0x41, 0x02);
      IicPsMasterPolled_Write(0x3d, 0x42, 0x0d);
      IicPsMasterPolled_Write(0x3d, 0x43, 0xf7);
      IicPsMasterPolled_Write(0x3d, 0x44, 0x10);
      IicPsMasterPolled_Write(0x3d, 0x45, 0x68);
      IicPsMasterPolled_Write(0x3d, 0x46, 0x00);
      IicPsMasterPolled_Write(0x3d, 0x47, 0x00);
      IicPsMasterPolled_Write(0x3d, 0x3d, 0x07);
        
      //display Device ID information
      Status = IicPsMasterPolled_Read(0x39, 0x02,ReadBuffer);

      if (Status != XST_SUCCESS)  {
          return XST_FAILURE;
      }
      else{
          printf("Read Device ID of (%d)\n\r", ReadBuffer[0]);
      }

        
      Status = IicPsMasterPolled_Read(0x39, 0x03);
      if (Status != XST_SUCCESS)  {
          return XST_FAILURE;
      }
      else    {
          printf("Read Device ID of (%d)\n\r", ReadBuffer[0]);
      }

#endif   



}

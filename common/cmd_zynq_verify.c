/*
 * (C) Copyright 2000, 2001
 * Rich Ireland, Enterasys Networks, rireland@enterasys.com.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

#include <common.h>
#include <command.h>
#include <malloc.h>

#include "xdevcfg_hw.h"
#include "xdevcfg.h"
#include "fsbl.h"
/* Local functions */

#include "memory_config.h"
#include "xil_testmem.h"

#include "xparameters.h"
#include "xil_cache.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "scugic_header.h"
#include "xdmaps.h"
#include "dmaps_header.h"
#include "xemacps.h"
#include "xemacps_example.h"
#include "emacps_header.h"
#include "xiicps.h"
#include "iicps_header.h"
#include "xqspips.h"
#include "qspips_header.h"
#include "xdevcfg.h"
#include "devcfg_header.h"
#include "xscutimer.h"
#include "scutimer_header.h"
#include "xscuwdt.h"
#include "scuwdt_header.h"
#include "xgpiops.h"
#include "xgpio.h"
#include "sd.h"

enum {
    PS_GPIO_TEST = 1,
	PS_MEMORY_TEST,
	PS_SD_TEST,
	PS_QSPI_TEST,
	PS_USB_TEST,
	PS_GMAC_TEST,
    SCU_GIC_SELF_TEST,
    SCU_GIC_INT_SETUP,
    /* EMAC_PS_INT_TEST */
    IIC0_PS_SELF_TEST,
    IIC1_PS_SELF_TEST,
    QSPI_PS_SELF_TEST,
    SCU_TIMER_POLL_TEST,
    PL_OLED_TEST,
    PL_HDMI_TEST,
    EEPROM_TEST,
    PL_LED_TEST
};


#define OUTPUT_PIN		    47	
#define GPIO_LED_PIN        0    /* Pin connected to LED/Output */
#define GPIO_DEVICE_ID		XPAR_XGPIOPS_0_DEVICE_ID

char *opnum2opstr(int op_num)
{
    char tmp_op[100];
    switch (op_num)
    {

	case PS_GPIO_TEST:
		strncpy(tmp_op, "PS_GPIO_LED_test", 100);
		break;
	case PS_MEMORY_TEST:
		strncpy(tmp_op, "PS_Memory_test", 100);
		break;
    case PS_SD_TEST:
		strncpy(tmp_op, "PS_SD_test", 100);
        break;    
    case PS_QSPI_TEST:
		strncpy(tmp_op, "PS_QSPI_test", 100);
        break;
    case PS_USB_TEST:
		strncpy(tmp_op, "PS_USB_test", 100);
        break;
    case PS_GMAC_TEST:
        strncpy(tmp_op, "PS_GMAC_test", 100);
        break;
    case IIC0_PS_SELF_TEST:        
		strncpy(tmp_op, "I2C0_self_test", 100);
        break;
            
    case IIC1_PS_SELF_TEST:
		strncpy(tmp_op, "I2C1_self_test", 100);
        break;

    case QSPI_PS_SELF_TEST:       
		strncpy(tmp_op, "QSPI_self_test", 100);
        break;
        
    case SCU_TIMER_POLL_TEST:        
		strncpy(tmp_op, "Scu_Timer_Poll_test", 100);
        break;
    case PL_OLED_TEST:        
        strncpy(tmp_op, "OLED_Test", 100);
        break;
    case PL_HDMI_TEST:        
		strncpy(tmp_op, "HDMI_Test", 100);
        break;
    case EEPROM_TEST:        
		strncpy(tmp_op, "EEPROM_Test", 100);
        break;
    case PL_LED_TEST:        
		strncpy(tmp_op, "PL_LED_Test", 100);
        break;
	default:
		printf("invalid zynq verification operation\n");
		break;
    }
    return tmp_op;
}


struct memory_range_s memory_ranges[] = {
	/* ps7_qspi_linear_0 memory will not be tested since it is a flash memory */
	/* ps7_qspi_linear_0 memory will not be tested since it is a flash memory */
	{
		"ps7_ddr_0",
		"ps7_ddr",
		0x00100000,
		1072693248,
	},
	/* ps7_ram_0 memory will not be tested since application resides in the same memory */
	{
		"ps7_ram_1",
		"ps7_ram",
		0xffff0000,
		65024,
	},
};

int n_memory_ranges = 2;


void test_memory_range(struct memory_range_s *range) {
    XStatus status;

    /* This application uses print statements instead of xil_printf/printf
     * to reduce the text size.
     *
     * The default linker script generated for this application does not have
     * heap memory allocated. This implies that this program cannot use any
     * routines that allocate memory on heap (printf is one such function).
     * If you'd like to add such functions, then please generate a linker script
     * that does allocate sufficient heap memory.
     */

    printf("Testing memory region: %s\r\n", range->name);
    printf("    Memory Controller: %s\r", range->ip);
    printf("         Base Address: 0x%x\r\n", range->base);
    printf("                 Size: 0x0x%x\r\n", range->size);
    
    status = Xil_TestMem32((u32*)range->base, 1024, 0xAAAA5555, XIL_TESTMEM_ALLMEMTESTS);
    printf("          32-bit test: %s\r\n", status == XST_SUCCESS? "PASSED!":"FAILED!");
    
    status = Xil_TestMem16((u16*)range->base, 2048, 0xAA55, XIL_TESTMEM_ALLMEMTESTS);
    printf("          16-bit test: %s\r\n", status == XST_SUCCESS? "PASSED!":"FAILED!");
    
    status = Xil_TestMem8((u8*)range->base, 4096, 0xA5, XIL_TESTMEM_ALLMEMTESTS);
    printf("          8-bit test: %s\r\n", status == XST_SUCCESS? "PASSED!":"FAILED!");
}


int zynq_ps_gpio_led_test()
{
    int i;
	XGpioPs_Config *ConfigPtr;
    int Status;
    XGpioPs Gpio;
    
    printf("---Starting PS GPIO LED Test Application---\n\r");
    

	/*
	 * Initialize the GPIO driver.
	 */
	ConfigPtr = XGpioPs_LookupConfig(GPIO_DEVICE_ID);
	Status = XGpioPs_CfgInitialize(&Gpio, ConfigPtr,
					ConfigPtr->BaseAddr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


    /* set GPIO_LED_PIN*/
	XGpioPs_SetDirectionPin(&Gpio, GPIO_LED_PIN, 1);
	XGpioPs_SetOutputEnablePin(&Gpio, GPIO_LED_PIN, 1);

    /* led off-> on three times */
    for (i = 0; i < 3; i++)
    {
        /* Set the GPIO output to be hign, led off */
        XGpioPs_WritePin(&Gpio, 0, 0x1);
        mdelay(500);
        /* Set the GPIO output to be hign, led on */
        XGpioPs_WritePin(&Gpio, 0, 0x0);
        /* delay 1s */
        printf("GPIO LED On %i times\r\n", i+1);
        mdelay(500);
    }
#if 0
    /*
    * Set the direction for the pin to be output and
    * Enable the Output enable for the LED Pin.
    */
    XGpioPs_SetDirectionPin(&Gpio, OUTPUT_PIN, 1);
    XGpioPs_SetOutputEnablePin(&Gpio, OUTPUT_PIN, 1);
    /*
    * Set the GPIO output to be low.
    */
    XGpioPs_WritePin(&Gpio, OUTPUT_PIN, 0x1);
#endif
    
    
    printf("---PS GPIO LED Test Application Complete---\n\r");
    printf("\r\n");
     
    return 0;
}


int zynq_ps_memory_test()
{
    int i;
    printf("---Starting Memory Test Application---\n\r");
    printf("NOTE: This application runs with D-Cache disabled.");
    printf("As a result, cacheline requests will not be generated\n\r");

    for (i = 0; i < n_memory_ranges; i++) 
    {
        mdelay(1000);
        test_memory_range(&memory_ranges[i]);
    }
    
    printf("---Memory Test Application Complete---\n\r");
    printf("\r\n");
    return 0;
}


int zynq_ps_sd_test()
{
    int Status;
    
    printf("---Starting SD Test Application---\n\r");
    
    Status = Get_Image_Info_From_SD();

    if (Status == 0) 
    {
        printf("---SD Test Application Complete---\n\r\r\n");
    }
    else 
    {
        printf("---SD Test Application Failed---\n\r\r\n");
    }
    
    
    return 0;
}



int zynq_ps_qspi_test()
{
    int Status;
    int argc;
    int i;
    int ddr_wr_baseaddr = 0x00200000;
    int ddr_rd_baseaddr = 0x00300000;
    
	static const char *const arg_probe[] = { "probe", "0", "0", "0"};
	static const char *const arg_erase[] = { "erase", "0x01FFF000", "0x001000"};
	static const char *const arg_write[] = { "write", "0x00200000", "0x01FFF000", "0x001000"};
	static const char *const arg_read[] = { "read", "0x00300000", "0x01FFF000", "0x001000"};
    
    printf("---Starting QSPI Test Application---\n\r");
    
    argc = 4;    
    /* probe 0 0 0 */
    Status = spi_op(argc, arg_probe);   
    if (Status == 0) 
    {
        printf("---QSPI Probe Test Application Complete---\n\r\r\n");
    }
    else 
    {
        printf("---QSPI Probe Test Application Failed---\n\r\r\n");
    }
    /* probe erase 0x01FFF000 0x001000  erase the last 1KB */
    argc = 3;
    Status = spi_op(argc, arg_erase);   
    if (Status == 0) 
    {
        printf("---QSPI Erase Test Application Complete---\n\r\r\n");
    }
    else 
    {
        printf("---QSPI Erase Test Application Failed---\n\r\r\n");
    }
    
    /* sf write 0x00200000 0x01FFF000 0x001000 */
    
    for (i = 0; i < 0x1000; i++) 
    {
        Xil_Out32((ddr_wr_baseaddr+(i*4)), (i+1));
    }
    argc = 4;
    Status = spi_op(argc, arg_write);   
    if (Status == 0) 
    {
        printf("---QSPI Write Test Application Complete---\n\r\r\n");
    }
    else 
    {
        printf("---QSPI Write Test Application Failed---\n\r\r\n");
    }
    
    /* sf read 0x00300000 0x01FFF000 0x001000 */
    argc = 4;
    Status = spi_op(argc, arg_read);   
    if (Status == 0)
    {
        printf("---QSPI Read Test Application Complete---\n\r\r\n");
    }
    else 
    {
        printf("---QSPI Read Test Application Failed---\n\r\r\n");
    }
    if (memcmp(ddr_wr_baseaddr, ddr_rd_baseaddr, 0x1000) == 0) 
    {
        printf("---QSPI Test Complete---\n\r\r\n");
    }
    else
    {
        printf("---QSPI Test Failed ---\n\r\r\n");
    }
    return 0;
}


int zynq_ps_usb_test()
{
    int Status;

    printf("---Starting USB Test Application---\n\r");
    Status = ulpi_phy_init();
    if (Status == 0) 
    {
        printf("---USB Test Application Complete---\n\r\r\n");
    }
    else 
    {
        printf("---USB Test Application Failed---\n\r\r\n");
    }
    return 0;
}

int zynq_ps_Gmac_test()
{
    int Status;
    char *server_ip;
    printf("---Starting GMAC Test Application---\n\r");
    Status = Xgmac_init(NULL, NULL);
    if (Status == 0 || Status == 1) 
    {
        printf("---GMAC Test Application Complete---\n\r\r\n");
    }
    else 
    {
        printf("---GMAC Test Application Failed, status:%d---\n\r\r\n", Status);
    }
    
    printf("---Starting Ethernet Test Application---\n\r");
    /* ping to check the ethernet */
    if ((server_ip= getenv ("serverip")) == NULL) {
        printf("###set serverip first\r\n");
        return -1;
    }
    else
    {
        printf("serverip:%s\r\n", server_ip);
        NetPingIP = string_to_ip(server_ip);
        if (NetPingIP == 0)
            return CMD_RET_USAGE;
        
        if (NetLoop(PING) < 0) {
            printf("ping failed; host %s is not alive\r\n\r\n", server_ip);
            return -1;
        }       
        printf("host %s is alive\r\n\r\n", server_ip);
    }
    return 0;
}





int ScuGicSelfTest()
{
    int Status;

    printf("\r\n Running ScuGicSelfTestExample() for ps7_scugic_0...\r\n");
    
    Status = ScuGicSelfTestExample(XPAR_PS7_SCUGIC_0_DEVICE_ID);

    if (Status == 0) 
    {
        printf("ScuGicSelfTestExample PASSED\r\n");
    }
    else 
    {
        printf("ScuGicSelfTestExample FAILED\r\n");
    }
}

int ScuGicIntSetup()
{
    int Status;
    static XScuGic intc;
    Status = ScuGicInterruptSetup(&intc, XPAR_PS7_SCUGIC_0_DEVICE_ID);
    if (Status == 0) 
    {
        printf("ScuGic Interrupt Setup PASSED\r\n");
    } 
    else 
    {
        printf("ScuGic Interrupt Setup FAILED\r\n");
    } 
}

#if 0
int EmacPsIntrTest()
{
    int Status;    
    static XScuGic intc;
    static XEmacPs ps7_ethernet_0;
    
    printf("\r\n Running Interrupt Test  for ps7_ethernet_0...\r\n");
    
    Status = EmacPsDmaIntrExample(&intc, &ps7_ethernet_0, \
             XPAR_PS7_ETHERNET_0_DEVICE_ID, \
             XPAR_PS7_ETHERNET_0_INTR);

    if (Status == 0) 
    {
        printf("EmacPsDmaIntrExample PASSED\r\n");
    } 
    else 
    {
        printf("EmacPsDmaIntrExample FAILED\r\n");
    }

}
#endif


int IICPS_SelfTest(int device_id)
{
      int Status;
      
      printf("\r\n Running IicPsSelfTestExample() for ps7_i2c_0...\r\n");
      
      Status = IicPsSelfTestExample(device_id);
      
      if (Status == 0) {
         printf("IicPsSelfTestExample PASSED\r\n");
      }
      else {
         printf("IicPsSelfTestExample FAILED\r\n");
      }
      return 0;
}

int QspiPS_SelfTest()
{
    int Status;

    printf("\r\n Running QspiSelfTestExample() for ps7_qspi_0...\r\n");

    Status = QspiPsSelfTestExample(XPAR_PS7_QSPI_0_DEVICE_ID);

    if (Status == 0) 
    {
        printf("QspiPsSelfTestExample PASSED\r\n");
    }
    else 
    {
        printf("QspiPsSelfTestExample FAILED\r\n");
    }
    return 0;
}
#if 0
int Dcfg_Self_Test()
{
    int Status;

    printf("\r\n Running DcfgSelfTestExample() for ps7_dev_cfg_0...\r\n");

    Status = DcfgSelfTestExample(XPAR_PS7_DEV_CFG_0_DEVICE_ID);

    if (Status == 0) 
    {
        printf("DcfgSelfTestExample PASSED\r\n");
    }
    else 
    {
        printf("DcfgSelfTestExample FAILED\r\n");
    }
    return 0;
}
#endif


int ScuTimer_Poll_Test()
{
    int Status;

    printf("\r\n Running ScuTimerPolledExample() for ps7_scutimer_0...\r\n");
    
    Status = ScuTimerPolledExample(XPAR_PS7_SCUTIMER_0_DEVICE_ID);
    
    if (Status == 0) 
    {
        printf("ScuTimerPolledExample PASSED\r\n");
    }
    else 
    {
        printf("ScuTimerPolledExample FAILED\r\n");
    }
}

void PL_OLED_Test()
{
    printf("---Starting OLED Test Application---\n\r");
    oled_init();
    printf("---OLED Test Application Complete--\n\r");
    printf("\r\n");
    return;
}


void PL_HDMI_Test()
{
    printf("--Starting HDMI Test Application--\n\r");
    HDMI_init();
    printf("--HDMI Test Application Complete--\n\r");
    printf("\r\n");
    return;
}



/*****************************************************************************/
/**
* Main function to call the Iic EEPROM polled example.
*
* @param	None.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None.
*
******************************************************************************/
int IICPs_Eeprom_test(void)
{
	int Status;

    printf("--Starting EEPROM Test Application--\n\r");

	/*
	 * Run the Iic EEPROM Polled Mode example.
	 */
	Status = IicPsEepromPolledExample();
	if (Status != XST_SUCCESS) {
		printf("IIC EEPROM Polled Mode Example Test Failed\r\n");
		return XST_FAILURE;
	}
    
    printf("--EEPROM Test Application Complete--\n\r");
	return XST_SUCCESS;
}


int PL_Led_test(void)
{
	u32 Delay;
	u32 Ledwidth;
	u32 i;
    XGpio GpioOutput;
    // TBD
    //XGpio_Initialize(&GpioOutput, XPAR_PL_LED4_DEVICE_ID);
    XGpio_SetDataDirection(&GpioOutput, 1, 0x0);
    XGpio_DiscreteWrite(&GpioOutput, 1, 0x0);

    while (1)
    {
        for (Ledwidth = 0x0; Ledwidth < 4; Ledwidth++)
        {
              XGpio_DiscreteWrite(&GpioOutput, 1, 1 << Ledwidth);
              udelay(1000000);
              XGpio_DiscreteClear(&GpioOutput, 1, 1 << Ledwidth);
        }
    }
    
    return 0;
}

/* ------------------------------------------------------------------------- */
/* command form:
 *   fpga <op> <device number> <data addr> <datasize>
 * where op is 'load', 'dump', or 'info'
 * If there is no device number field, the fpga environment variable is used.
 * If there is no data addr field, the fpgadata environment variable is used.
 * The info command requires no data address field.
 */
int do_zynq_verify (cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    unsigned int op_num;
    char *op_str;
    /* do_zynq_verify no.*/
    
	switch (argc) 
    {
	case 2:		
		op_num = simple_strtoul (argv[1], NULL, 10);
        op_str = opnum2opstr(op_num);
		printf("%s: op_num: 0x%x, operation:%s\n", __func__, op_num, op_str);
		break;
        
	default:
		debug("%s: Too many or too few args (%d)\n",__func__, argc);
        op_num = -1;
		break;
	}
    
	switch (op_num) 
    {
    case PS_GPIO_TEST:
        zynq_ps_gpio_led_test();
        break;
	case PS_MEMORY_TEST: 
        zynq_ps_memory_test();
		break;
    case PS_SD_TEST:
        zynq_ps_sd_test();
        break;
    case PS_QSPI_TEST:
        zynq_ps_qspi_test();
        break;
    case PS_USB_TEST:
        zynq_ps_usb_test();
        break;
    case PS_GMAC_TEST:
        zynq_ps_Gmac_test();
        break;
    case SCU_GIC_SELF_TEST:
        ScuGicSelfTest();
    case SCU_GIC_INT_SETUP:
        ScuGicIntSetup();
        break;
    case IIC0_PS_SELF_TEST:
        IICPS_SelfTest(0);
        break;
    case IIC1_PS_SELF_TEST:
        IICPS_SelfTest(1);
        break;
    case QSPI_PS_SELF_TEST:
        QspiPS_SelfTest();
        break;
    case SCU_TIMER_POLL_TEST:
        ScuTimer_Poll_Test();
        break;
    case PL_OLED_TEST:
        PL_OLED_Test();
        break;
    case PL_HDMI_TEST:
        PL_HDMI_Test();
        break;
    case EEPROM_TEST:
        IICPs_Eeprom_test();
        break;
    case PL_LED_TEST:
        PL_Led_test();
        break;
#if 0
    case DEVCFG_SELF_TEST:
        Dcfg_Self_Test();
        break;
        
    /* delete because mmu needed */
    case EMAC_PS_INT_TEST:
        EmacPsIntrTest();
        break;       
#endif
        
    default:
        printf("invalid parameter, no zynq verification\n");        
        printf("***************************************\n");   
        printf("\r\n");
		return cmd_usage(cmdtp);
	}
    
	return 0;
}

U_BOOT_CMD (zynq_verify, 3, 1, do_zynq_verify,
	"verify star-zynq7000 board function",
	"[do_zynq_verify] [number]\n"
	"zynq verify operations no:operation\n"
	"  1\t  PS GPIO LED test\n"
	"  2\t  PS Memory test\n"
	"  3\t  PS SD test\n"
	"  4\t  PS QSPI test\n"
	"  5\t  PS USB test\n"
	"  6\t  PS GMAC test\n"
	"  5\t  I2C0 test\n"
	"  6\t  I2C1 test\n"
	"  7\t  QSPI test\n"
	"  8\t  Scu Timer Poll test\n"
    "  9\t  OLED test\n"
    "  10\t HDMI test\n"    
    "  11\t I2C EEPROM test\n"
    "  12\t PL LED test\n"
);


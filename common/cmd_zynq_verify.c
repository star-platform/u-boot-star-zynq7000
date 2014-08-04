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

enum {
	MEMORY_TEST	= 1,
    SCU_GIC_SELF_TEST,
    SCU_GIC_INT_SETUP,
    /* EMAC_PS_INT_TEST */
    IIC0_PS_SELF_TEST,
    IIC1_PS_SELF_TEST,
    QSPI_PS_SELF_TEST,
    SCU_TIMER_POLL_TEST
};

char *opnum2op(int op_num)
{
    char tmp_op[100];
    switch (op_num)
    {
	case MEMORY_TEST:
		memcpy(tmp_op, "memory_test", 100);
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


int zynq_memory_test()
{
    int i;
    printf("--Starting Memory Test Application--\n\r");
    printf("NOTE: This application runs with D-Cache disabled.");
    printf("As a result, cacheline requests will not be generated\n\r");

    for (i = 0; i < n_memory_ranges; i++) {
        test_memory_range(&memory_ranges[i]);
    }

    printf("--Memory Test Application Complete--\n\r");
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
		op_num = simple_strtoul (argv[1], NULL, 16);
        op_str = opnum2op(op_num);
		printf("%s: op_num: 0x%x, operation:%s\n", op_str);
		break;
            
	default:
		debug("%s: Too many or too few args (%d)\n",__func__, argc);
        op_num = -1;
		break;
	}
          
	switch (op_num) 
    {
	case MEMORY_TEST: 
        zynq_memory_test();
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
	}
    
	return 0;
}

U_BOOT_CMD (zynq_verify, 6, 1, do_zynq_verify,
	"verify star-zynq7000 board function",
	"[do_zynq_verify] [number]\n"
	"zynq verify operations in detail:\n"
	"  1\t DDR test\n"
	"  2\t \n"
	"  \n"
	"  loadb\t[dev] [address] [size]\t"
	"\tsubimage unit name in the form of addr:<subimg_uname>"
);

/******************************************************************************
*
* (c) Copyright 2012-2013 Xilinx, Inc. All rights reserved.
*
* This file contains confidential and proprietary information of Xilinx, Inc.
* and is protected under U.S. and international copyright and other
* intellectual property laws.
*
* DISCLAIMER
* This disclaimer is not a license and does not grant any rights to the
* materials distributed herewith. Except as otherwise provided in a valid
* license issued to you by Xilinx, and to the maximum extent permitted by
* applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL
* FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS,
* IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
* MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE;
* and (2) Xilinx shall not be liable (whether in contract or tort, including
* negligence, or under any other theory of liability) for any loss or damage
* of any kind or nature related to, arising under or in connection with these
* materials, including for any direct, or any indirect, special, incidental,
* or consequential loss or damage (including loss of data, profits, goodwill,
* or any type of loss or damage suffered as a result of any action brought by
* a third party) even if such damage or loss was reasonably foreseeable or
* Xilinx had been advised of the possibility of the same.
*
* CRITICAL APPLICATIONS
* Xilinx products are not designed or intended to be fail-safe, or for use in
* any application requiring fail-safe performance, such as life-support or
* safety devices or systems, Class III medical devices, nuclear facilities,
* applications related to the deployment of airbags, or any other applications
* that could lead to death, personal injury, or severe property or
* environmental damage (individually and collectively, "Critical
* Applications"). Customer assumes the sole risk and liability of any use of
* Xilinx products in Critical Applications, subject only to applicable laws
* and regulations governing limitations on product liability.
*
* THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE
* AT ALL TIMES.
*
*******************************************************************************/
/*****************************************************************************/
/**
*
* @file qspi.c
*
* Contains code for the QSPI FLASH functionality.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver	Who	Date		Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00a ecm	01/10/10 Initial release
* 3.00a mb  25/06/12 InitQspi, data is read first and required config bits
*                    are set
* 4.00a sg	02/28/13 Cleanup
* 					 Removed LPBK_DLY_ADJ register setting code as we use
* 					 divisor 8
* 5.00a sgd	05/17/13 Added Flash Size > 128Mbit support
* 					 Dual Stack support
*					 Fix for CR:721674 - FSBL- Failed to boot from Dual
*					                     stacked QSPI
* 6.00a kc  08/30/13 Fix for CR#722979 - Provide customer-friendly
*                                        changelogs in FSBL
*                    Fix for CR#739711 - FSBL not able to read Large QSPI
*                    					 (512M) in IO Mode
* </pre>
*
* @note
*
******************************************************************************/

/***************************** Include Files *********************************/

#include "qspi.h"
#include "image_mover.h"

#ifdef XPAR_PS7_QSPI_LINEAR_0_S_AXI_BASEADDR
#include "xqspips_hw.h"
#include "xqspips.h"

/************************** Constant Definitions *****************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#define QSPI_DEVICE_ID		XPAR_XQSPIPS_0_DEVICE_ID

/*
 * The following constants define the commands which may be sent to the FLASH
 * device.
 */
#define ENTER_QPI_MODE			0x38
#define EXIT_QPI_MODE			0xff

#define DUAL_READ_CMD		0x3B

#define QUAD_READ_CMD		0x6B
#define READ_ID_CMD			0x9F

#define WRITE_ENABLE_CMD	0x06
#define BANK_REG_RD			0x16
#define BANK_REG_WR			0x17
/* Bank register is called Extended Address Reg in Micron */
#define EXTADD_REG_RD		0xC8
#define EXTADD_REG_WR		0xC5

#define COMMAND_OFFSET		0 /* FLASH instruction */
#define ADDRESS_1_OFFSET	1 /* MSB byte of address to read or write */
#define ADDRESS_2_OFFSET	2 /* Middle byte of address to read or write */
#define ADDRESS_3_OFFSET	3 /* LSB byte of address to read or write */
#define DATA_OFFSET			4 /* Start of Data for Read/Write */
#define DUMMY_OFFSET		4 /* Dummy byte offset for fast, dual and quad
				     reads */
#define DUMMY_SIZE			1 /* Number of dummy bytes for fast, dual and
				     quad reads */
#define RD_ID_SIZE			4 /* Read ID command + 3 bytes ID response */
#define BANK_SEL_SIZE		2 /* BRWR or EARWR command + 1 byte bank value */
#define WRITE_ENABLE_CMD_SIZE	1 /* WE command */
/*
 * The following constants specify the extra bytes which are sent to the
 * FLASH on the QSPI interface, that are not data, but control information
 * which includes the command and address
 */
#define OVERHEAD_SIZE		4

/*
 * The following constants specify the max amount of data and the size of the
 * the buffer required to hold the data and overhead to transfer the data to
 * and from the FLASH.
 */
#define DATA_SIZE		4096

/*
 * The following defines are for dual flash interface.
 */
#define LQSPI_CR_FAST_QUAD_READ		0x0000006B /* Fast Quad Read output */
#define LQSPI_CR_1_DUMMY_BYTE		0x00000100 /* 1 Dummy Byte between
						     address and return data */

#define SINGLE_QSPI_CONFIG_QUAD_READ	(XQSPIPS_LQSPI_CR_LINEAR_MASK | \
					 LQSPI_CR_1_DUMMY_BYTE | \
					 LQSPI_CR_FAST_QUAD_READ)

#define DUAL_QSPI_CONFIG_QUAD_READ	(XQSPIPS_LQSPI_CR_LINEAR_MASK | \
					 XQSPIPS_LQSPI_CR_TWO_MEM_MASK | \
					 XQSPIPS_LQSPI_CR_SEP_BUS_MASK | \
					 LQSPI_CR_1_DUMMY_BYTE | \
					 LQSPI_CR_FAST_QUAD_READ)

#define DUAL_STACK_CONFIG_READ	(XQSPIPS_LQSPI_CR_TWO_MEM_MASK | \
					 LQSPI_CR_1_DUMMY_BYTE | \
					 LQSPI_CR_FAST_QUAD_READ)

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

/************************** Variable Definitions *****************************/

XQspiPs QspiInstance;
XQspiPs *QspiInstancePtr;
u32 QspiFlashSize;
u32 QspiFlashMake;
extern u32 FlashReadBaseAddress;
extern u8 LinearBootDeviceFlag;

/*
 * The following variables are used to read and write to the eeprom and they
 * are global to avoid having large buffers on the stack
 */
u8 ReadBuffer[DATA_SIZE + DATA_OFFSET + DUMMY_SIZE];
u8 WriteBuffer[DATA_OFFSET + DUMMY_SIZE];

/******************************************************************************/
/**
*
* This function initializes the controller for the QSPI interface.
*
* @param	None
*
* @return	None
*
* @note		None
*
****************************************************************************/
u32 InitQspi(void)
{
	XQspiPs_Config *QspiConfig;
	int Status;

	QspiInstancePtr = &QspiInstance;

	/*
	 * Set up the base address for access
	 */
	FlashReadBaseAddress = XPS_QSPI_LINEAR_BASEADDR;

	/*
	 * Initialize the QSPI driver so that it's ready to use
	 */
	QspiConfig = XQspiPs_LookupConfig(QSPI_DEVICE_ID);
	if (NULL == QspiConfig) {
		return XST_FAILURE;
	}

	Status = XQspiPs_CfgInitialize(QspiInstancePtr, QspiConfig,
					QspiConfig->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Set Manual Chip select options and drive HOLD_B pin high.
	 */
	XQspiPs_SetOptions(QspiInstancePtr, XQSPIPS_FORCE_SSELECT_OPTION |
			XQSPIPS_HOLD_B_DRIVE_OPTION);

	/*
	 * Set the prescaler for QSPI clock
	 */
	XQspiPs_SetClkPrescaler(QspiInstancePtr, XQSPIPS_CLK_PRESCALE_8);

	/*
	 * Assert the FLASH chip select.
	 */
	XQspiPs_SetSlaveSelect(QspiInstancePtr);

	/*
	 * Read Flash ID and extract Manufacture and Size information
	 */
	Status = FlashReadID();
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	if (XPAR_PS7_QSPI_0_QSPI_MODE == SINGLE_FLASH_CONNECTION) {

		fsbl_printf(DEBUG_INFO,"QSPI is in single flash connection\r\n");
		/*
		 * For Flash size <128Mbit controller configured in linear mode
		 */
		if (QspiFlashSize <= FLASH_SIZE_16MB) {
			LinearBootDeviceFlag = 1;

			/*
			 * Enable linear mode
			 */
			XQspiPs_SetOptions(QspiInstancePtr,  XQSPIPS_LQSPI_MODE_OPTION |
					XQSPIPS_HOLD_B_DRIVE_OPTION);

			/*
			 * Single linear read
			 */
			XQspiPs_SetLqspiConfigReg(QspiInstancePtr, SINGLE_QSPI_CONFIG_QUAD_READ);

			/*
			 * Enable the controller
			 */
			XQspiPs_Enable(QspiInstancePtr);
		}
	}

	if (XPAR_PS7_QSPI_0_QSPI_MODE == DUAL_PARALLEL_CONNECTION) {

		fsbl_printf(DEBUG_INFO,"QSPI is in Dual Parallel connection\r\n");
		/*
		 * For Single Flash size <128Mbit controller configured in linear mode
		 */
		if (QspiFlashSize <= FLASH_SIZE_16MB) {
			/*
			 * Setting linear access flag
			 */
			LinearBootDeviceFlag = 1;

			/*
			 * Enable linear mode
			 */
			XQspiPs_SetOptions(QspiInstancePtr,  XQSPIPS_LQSPI_MODE_OPTION |
					XQSPIPS_HOLD_B_DRIVE_OPTION);

			/*
			 * Dual linear read
			 */
			XQspiPs_SetLqspiConfigReg(QspiInstancePtr, DUAL_QSPI_CONFIG_QUAD_READ);

			/*
			 * Enable the controller
			 */
			XQspiPs_Enable(QspiInstancePtr);
		}

		/*
		 * Total flash size is two time of single flash size
		 */
		QspiFlashSize = 2 * QspiFlashSize;
	}

	/*
	 * It is expected to same flash size for both chip selection
	 */
	if (XPAR_PS7_QSPI_0_QSPI_MODE == DUAL_STACK_CONNECTION) {

		fsbl_printf(DEBUG_INFO,"QSPI is in Dual Stack connection\r\n");

		QspiFlashSize = 2 * QspiFlashSize;

		/*
		 * Enable two flash memories on separate buses
		 */
		XQspiPs_SetLqspiConfigReg(QspiInstancePtr, DUAL_STACK_CONFIG_READ);
	}

	return XST_SUCCESS;
}

/******************************************************************************
*
* This function reads serial FLASH ID connected to the SPI interface.
* It then deduces the make and size of the flash and obtains the
* connection mode to point to corresponding parameters in the flash
* configuration table. The flash driver will function based on this and
* it presently supports Micron and Spansion - 128, 256 and 512Mbit and
* Winbond 128Mbit
*
* @param	none
*
* @return	XST_SUCCESS if read id, otherwise XST_FAILURE.
*
* @note		None.
*
******************************************************************************/
u32 FlashReadID(void)
{
	u32 Status;

	/*
	 * Read ID in Auto mode.
	 */
	WriteBuffer[COMMAND_OFFSET]   = READ_ID_CMD;
	WriteBuffer[ADDRESS_1_OFFSET] = 0x00;		/* 3 dummy bytes */
	WriteBuffer[ADDRESS_2_OFFSET] = 0x00;
	WriteBuffer[ADDRESS_3_OFFSET] = 0x00;

	Status = XQspiPs_PolledTransfer(QspiInstancePtr, WriteBuffer, ReadBuffer,
				RD_ID_SIZE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	fsbl_printf(DEBUG_INFO,"Single Flash Information\r\n");

	fsbl_printf(DEBUG_INFO,"FlashID=0x%x 0x%x 0x%x\r\n", ReadBuffer[1],
			ReadBuffer[2],
			ReadBuffer[3]);

	/*
	 * Deduce flash make
	 */
	if (ReadBuffer[1] == MICRON_ID) {
		QspiFlashMake = MICRON_ID;
		fsbl_printf(DEBUG_INFO, "MICRON ");
	} else if(ReadBuffer[1] == SPANSION_ID) {
		QspiFlashMake = SPANSION_ID;
		fsbl_printf(DEBUG_INFO, "SPANSION ");
	} else if(ReadBuffer[1] == WINBOND_ID) {
		QspiFlashMake = WINBOND_ID;
		fsbl_printf(DEBUG_INFO, "WINBOND ");
	}

	/*
	 * Deduce flash Size
	 */
	if (ReadBuffer[3] == FLASH_SIZE_ID_128M) {
		QspiFlashSize = FLASH_SIZE_128M;
		fsbl_printf(DEBUG_INFO, "128M Bits\r\n");
	} else if (ReadBuffer[3] == FLASH_SIZE_ID_256M) {
		QspiFlashSize = FLASH_SIZE_256M;
		fsbl_printf(DEBUG_INFO, "256M Bits\r\n");
	} else if (ReadBuffer[3] == FLASH_SIZE_ID_512M) {
		QspiFlashSize = FLASH_SIZE_512M;
		fsbl_printf(DEBUG_INFO, "512M Bits\r\n");
	} else if (ReadBuffer[3] == FLASH_SIZE_ID_1G) {
		QspiFlashSize = FLASH_SIZE_1G;
		fsbl_printf(DEBUG_INFO, "1G Bits\r\n");
	}

	return XST_SUCCESS;
}


/******************************************************************************
*
* This function reads from the  serial FLASH connected to the
* QSPI interface.
*
* @param	Address contains the address to read data from in the FLASH.
* @param	ByteCount contains the number of bytes to read.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void FlashRead(u32 Address, u32 ByteCount)
{
	/*
	 * Setup the write command with the specified address and data for the
	 * FLASH
	 */
	/* modify by star-star */
	WriteBuffer[COMMAND_OFFSET]   = DUAL_READ_CMD;/*QUAD_READ_CMD;*/
	WriteBuffer[ADDRESS_1_OFFSET] = (u8)((Address & 0xFF0000) >> 16);
	WriteBuffer[ADDRESS_2_OFFSET] = (u8)((Address & 0xFF00) >> 8);
	WriteBuffer[ADDRESS_3_OFFSET] = (u8)(Address & 0xFF);
	
	ByteCount += DUMMY_SIZE;

	/*
	 * Send the read command to the FLASH to read the specified number
	 * of bytes from the FLASH, send the read command and address and
	 * receive the specified number of bytes of data in the data buffer
	 */
	XQspiPs_PolledTransfer(QspiInstancePtr, WriteBuffer, ReadBuffer,
				ByteCount + OVERHEAD_SIZE);
}

/******************************************************************************/
/**
*
* This function provides the QSPI FLASH interface for the Simplified header
* functionality.
*
* @param	SourceAddress is address in FLASH data space
* @param	DestinationAddress is address in DDR data space
* @param	LengthBytes is the length of the data in Bytes
*
* @return
*		- XST_SUCCESS if the write completes correctly
*		- XST_FAILURE if the write fails to completes correctly
*
* @note	none.
*
****************************************************************************/
u32 QspiAccess( u32 SourceAddress, u32 DestinationAddress, u32 LengthBytes)
{
	u8	*BufferPtr;
	u32 Length = 0;
	u32 BankSel = 0;
	u32 LqspiCrReg;
	u32 Status;
	u8 BankSwitchFlag = 1;

	/*
	 * Linear access check
	 */
	if (LinearBootDeviceFlag == 1) {
		/*
		 * Check for non-word tail, add bytes to cover the end
		 */
		if ((LengthBytes%4) != 0){
			LengthBytes += (4 - (LengthBytes & 0x00000003));
		}

		memcpy((void*)DestinationAddress,
		      (const void*)(SourceAddress + FlashReadBaseAddress),
		      (size_t)LengthBytes);
	} else {
		/*
		 * Non Linear access
		 */
		BufferPtr = (u8*)DestinationAddress;

		/*
		 * Dual parallel connection actual flash is half
		 */
		if (XPAR_PS7_QSPI_0_QSPI_MODE == DUAL_PARALLEL_CONNECTION) {
			SourceAddress = SourceAddress/2;
		}

		while(LengthBytes > 0) {
			/*
			 * Local of DATA_SIZE size used for read/write buffer
			 */
			if(LengthBytes > DATA_SIZE) {
				Length = DATA_SIZE;
			} else {
				Length = LengthBytes;
			}

			/*
			 * Dual stack connection
			 */
			if (XPAR_PS7_QSPI_0_QSPI_MODE == DUAL_STACK_CONNECTION) {
				/*
				 * Get the current LQSPI configuration value
				 */
				LqspiCrReg = XQspiPs_GetLqspiConfigReg(QspiInstancePtr);

				/*
				 * Select lower or upper Flash based on sector address
				 */
				if (SourceAddress >= (QspiFlashSize/2)) {
					/*
					 * Set selection to U_PAGE
					 */
					XQspiPs_SetLqspiConfigReg(QspiInstancePtr,
							LqspiCrReg | XQSPIPS_LQSPI_CR_U_PAGE_MASK);

					/*
					 * Subtract first flash size when accessing second flash
					 */
					SourceAddress = SourceAddress - (QspiFlashSize/2);
					
					fsbl_printf(DEBUG_INFO, "stacked - upper CS \n\r");

					/*
					 * Assert the FLASH chip select.
					 */
					XQspiPs_SetSlaveSelect(QspiInstancePtr);
				}
			}

			/*
			 * Select bank
			 */
			if ((SourceAddress >= FLASH_SIZE_16MB) && (BankSwitchFlag == 1)) {
				BankSel = SourceAddress/FLASH_SIZE_16MB;

				fsbl_printf(DEBUG_INFO, "Bank Selection %d\n\r", BankSel);

				Status = SendBankSelect(BankSel);
				if (Status != XST_SUCCESS) {
					fsbl_printf(DEBUG_INFO, "Bank Selection Failed\n\r");
					return XST_FAILURE;
				}

				BankSwitchFlag = 0;
			}

			/*
			 * If data to be read spans beyond the current bank, then
			 * calculate length in current bank else no change in length
			 */
			if (XPAR_PS7_QSPI_0_QSPI_MODE == DUAL_PARALLEL_CONNECTION) {
				/*
				 * In dual parallel mode, check should be for half
				 * the length.
				 */
				if((SourceAddress & BANKMASK) != ((SourceAddress + (Length/2)) & BANKMASK))
				{
					Length = (SourceAddress & BANKMASK) + FLASH_SIZE_16MB - SourceAddress;
					/*
					 * Above length calculated is for single flash
					 * Length should be doubled since dual parallel
					 */
					Length = Length * 2;
					BankSwitchFlag = 1;
				}
			} else {
				if((SourceAddress & BANKMASK) != ((SourceAddress + Length) & BANKMASK))
				{
					Length = (SourceAddress & BANKMASK) + FLASH_SIZE_16MB - SourceAddress;
					BankSwitchFlag = 1;
				}
			}

			/*
			 * Copying the image to local buffer
			 */
			FlashRead(SourceAddress, Length);

			/*
			 * Moving the data from local buffer to DDR destination address
			 */
			memcpy(BufferPtr, &ReadBuffer[DATA_OFFSET + DUMMY_SIZE], Length);

			/*
			 * Updated the variables
			 */
			LengthBytes -= Length;

			/*
			 * For Dual parallel connection address increment should be half
			 */
			if (XPAR_PS7_QSPI_0_QSPI_MODE == DUAL_PARALLEL_CONNECTION) {
				SourceAddress += Length/2;
			} else {
				SourceAddress += Length;
			}

			BufferPtr = (u8*)((u32)BufferPtr + Length);
		}

		/*
		 * Reset Bank selection to zero
		 */
		Status = SendBankSelect(0);
		if (Status != XST_SUCCESS) {
			fsbl_printf(DEBUG_INFO, "Bank Selection Reset Failed\n\r");
			return XST_FAILURE;
		}

		if (XPAR_PS7_QSPI_0_QSPI_MODE == DUAL_STACK_CONNECTION) {

			/*
			 * Reset selection to L_PAGE
			 */
			XQspiPs_SetLqspiConfigReg(QspiInstancePtr,
					LqspiCrReg & (~XQSPIPS_LQSPI_CR_U_PAGE_MASK));

			fsbl_printf(DEBUG_INFO, "stacked - lower CS \n\r");

			/*
			 * Assert the FLASH chip select.
			 */
			XQspiPs_SetSlaveSelect(QspiInstancePtr);
		}
	}

	return XST_SUCCESS;
}



/******************************************************************************
*
* This functions selects the current bank
*
* @param	BankSel is the bank to be selected in the flash device(s).
*
* @return	XST_SUCCESS if bank selected
*			XST_FAILURE if selection failed
* @note		None.
*
******************************************************************************/
u32 SendBankSelect(u8 BankSel)
{
	u32 Status;

	/*
	 * bank select commands for Micron and Spansion are different
	 */
	if ((QspiFlashMake == MICRON_ID) || (QspiFlashMake == WINBOND_ID))	{
		/*
		 * For micron command WREN should be sent first
		 * except for some specific feature set
		 */
		WriteBuffer[COMMAND_OFFSET] = WRITE_ENABLE_CMD;
		Status = XQspiPs_PolledTransfer(QspiInstancePtr, WriteBuffer, NULL,
				WRITE_ENABLE_CMD_SIZE);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}

		/*
		 * Send the Extended address register write command
		 * written, no receive buffer required
		 */
		WriteBuffer[COMMAND_OFFSET]   = EXTADD_REG_WR;
		WriteBuffer[ADDRESS_1_OFFSET] = BankSel;
		Status = XQspiPs_PolledTransfer(QspiInstancePtr, WriteBuffer, NULL,
				BANK_SEL_SIZE);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	}	
	if (QspiFlashMake == SPANSION_ID) {
		WriteBuffer[COMMAND_OFFSET]   = BANK_REG_WR;
		WriteBuffer[ADDRESS_1_OFFSET] = BankSel;

		/*
		 * Send the Extended address register write command
		 * written, no receive buffer required
		 */
		Status = XQspiPs_PolledTransfer(QspiInstancePtr, WriteBuffer, NULL,
				BANK_SEL_SIZE);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	}

	/*
	 * For testing - Read bank to verify
	 */
	if (QspiFlashMake == SPANSION_ID){
		WriteBuffer[COMMAND_OFFSET]   = BANK_REG_RD;
		WriteBuffer[ADDRESS_1_OFFSET] = 0x00;

		/*
		 * Send the Extended address register write command
		 * written, no receive buffer required
		 */
		Status = XQspiPs_PolledTransfer(QspiInstancePtr, WriteBuffer, ReadBuffer,
				BANK_SEL_SIZE);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	}
	
	if ((QspiFlashMake == MICRON_ID) || (QspiFlashMake == WINBOND_ID))	{	
		WriteBuffer[COMMAND_OFFSET]   = EXTADD_REG_RD;
		WriteBuffer[ADDRESS_1_OFFSET] = 0x00;

		/*
		 * Send the Extended address register write command
		 * written, no receive buffer required
		 */
		Status = XQspiPs_PolledTransfer(QspiInstancePtr, WriteBuffer, ReadBuffer,
				BANK_SEL_SIZE);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	}

	if (ReadBuffer[1] != BankSel) {
		fsbl_printf(DEBUG_INFO, "BankSel %d != Register Read %d\n\r", BankSel,
				ReadBuffer[1]);
		return XST_FAILURE;
		/* modify by star-star */
		/* return XST_SUCCESS; */
	}

	return XST_SUCCESS;
}


#define STATUS_REGISTER_2_RD 0x35
#define STATUS_REGISTER_2_WR 0x31
#define QUAD_ENABLE 0x2
/******************************************************************************
*
* This functions set Quad mode
*
* @param	
*
* @return	XST_SUCCESS if bank selected
*			XST_FAILURE if selection failed
* @note		None.
*
******************************************************************************/
u32 QSpi_set_quad_mode()
{
	u32 Status;
	
	/*
	 * For testing - Read reg2
	 */
	WriteBuffer[COMMAND_OFFSET]   = STATUS_REGISTER_2_RD;
	WriteBuffer[ADDRESS_1_OFFSET] = 0x00;
	
	/*
	* Send the Extended address register write command
	* written, no receive buffer required
	*/
	Status = XQspiPs_PolledTransfer(QspiInstancePtr, WriteBuffer, ReadBuffer,
			BANK_SEL_SIZE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	
	fsbl_printf(DEBUG_INFO, "Register 2:0x%x, 0x%x\n\r", ReadBuffer[0], ReadBuffer[1]);
	if ((ReadBuffer[1] & QUAD_ENABLE) == 0)
	{
		u8 stat_reg2;
		stat_reg2 = ReadBuffer[1];
		stat_reg2 = stat_reg2 | QUAD_ENABLE;
		fsbl_printf(DEBUG_INFO, "stat_reg2:0x%x\n\r", stat_reg2);
		
		WriteBuffer[COMMAND_OFFSET] = WRITE_ENABLE_CMD;
		Status = XQspiPs_PolledTransfer(QspiInstancePtr, WriteBuffer, NULL,
				WRITE_ENABLE_CMD_SIZE);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
		
		/*
		 * Send the Extended address register write command
		 * written, no receive buffer required
		 */
		WriteBuffer[COMMAND_OFFSET]   = STATUS_REGISTER_2_WR;
		WriteBuffer[ADDRESS_1_OFFSET] = stat_reg2;
		Status = XQspiPs_PolledTransfer(QspiInstancePtr, WriteBuffer, NULL,
				BANK_SEL_SIZE);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
		
		WriteBuffer[COMMAND_OFFSET]   = STATUS_REGISTER_2_RD;
		WriteBuffer[ADDRESS_1_OFFSET] = 0x00;
		
		/*
		* Send the Extended address register write command
		* written, no receive buffer required
		*/
		Status = XQspiPs_PolledTransfer(QspiInstancePtr, WriteBuffer, ReadBuffer,
				BANK_SEL_SIZE);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
		
		fsbl_printf(DEBUG_INFO, "After set Register 2:0x%x, 0x%x\n\r", ReadBuffer[0], ReadBuffer[1]);

		
	}
	return XST_SUCCESS;
}


/******************************************************************************
*
* This functions set Quad mode
*
* @param	
*
* @return	XST_SUCCESS if bank selected
*			XST_FAILURE if selection failed
* @note		None.
*
******************************************************************************/
u32 QSpi_enter_QPI_mode()
{
	u32 Status;
	WriteBuffer[COMMAND_OFFSET] = ENTER_QPI_MODE;
	Status = XQspiPs_PolledTransfer(QspiInstancePtr, WriteBuffer, NULL,
			WRITE_ENABLE_CMD_SIZE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}	
	return XST_SUCCESS;
}

u32 QSpi_exit_QPI_mode()
{
	u32 Status;
	WriteBuffer[COMMAND_OFFSET] = EXIT_QPI_MODE;
	Status = XQspiPs_PolledTransfer(QspiInstancePtr, WriteBuffer, NULL,
			WRITE_ENABLE_CMD_SIZE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}	
	return XST_SUCCESS;
}

#endif


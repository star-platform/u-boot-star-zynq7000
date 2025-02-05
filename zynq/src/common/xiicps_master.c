/******************************************************************************
*
* (c) Copyright 2010-2013 Xilinx, Inc. All rights reserved.
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
******************************************************************************/
/*****************************************************************************/
/**
*
* @file xiicps_master.c
*
* Handles master mode transfers.
*
* <pre> MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---  -------- ---------------------------------------------
* 1.00a jz   01/30/10 First release
* 1.00a sdm  09/21/11 Updated the XIicPs_SetupMaster to not check for
*		      Bus Busy condition when the Hold Bit is set.
* 1.01a sg   03/30/12 Fixed an issue in XIicPs_MasterSendPolled where a
*		      check for transfer completion is added, which indicates
			 the completion of current transfer.
*
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/

#include "xiicps.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/
int TransmitFifoFill(XIicPs *InstancePtr);

static int XIicPs_SetupMaster(XIicPs *InstancePtr, int Role);
static void MasterSendData(XIicPs *InstancePtr);

/************************* Variable Definitions *****************************/

/*****************************************************************************/
/**
* This function initiates an interrupt-driven send in master mode.
*
* It tries to send the first FIFO-full of data, then lets the interrupt
* handler to handle the rest of the data if there is any.
*
* @param	InstancePtr is a pointer to the XIicPs instance.
* @param	MsgPtr is the pointer to the send buffer.
* @param	ByteCount is the number of bytes to be sent.
* @param	SlaveAddr is the address of the slave we are sending to.
*
* @return	None.
*
* @note		This send routine is for interrupt-driven transfer only.
*
 ****************************************************************************/
void XIicPs_MasterSend(XIicPs *InstancePtr, u8 *MsgPtr, int ByteCount,
		 u16 SlaveAddr)
{
	u32 BaseAddr;

	/*
	 * Assert validates the input arguments.
	 */
	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(MsgPtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);
	Xil_AssertVoid(XIICPS_ADDR_MASK >= SlaveAddr);


	BaseAddr = InstancePtr->Config.BaseAddress;
	InstancePtr->SendBufferPtr = MsgPtr;
	InstancePtr->SendByteCount = ByteCount;
	InstancePtr->RecvBufferPtr = NULL;

	/*
	 * Setup as a master sending role.
	 */
	XIicPs_SetupMaster(InstancePtr, SENDING_ROLE);

	/*
	 * Set repeated start if sending more than FIFO of data.
	 */
	if (ByteCount > XIICPS_FIFO_DEPTH) {
		XIicPs_WriteReg(BaseAddr, XIICPS_CR_OFFSET,
			XIicPs_ReadReg(BaseAddr, XIICPS_CR_OFFSET) |
				XIICPS_CR_HOLD_MASK);
	}

	/*
	 * Do the address transfer to notify the slave.
	 */
	XIicPs_WriteReg(BaseAddr, XIICPS_ADDR_OFFSET, SlaveAddr);

	TransmitFifoFill(InstancePtr);

	XIicPs_EnableInterrupts(BaseAddr,
		XIICPS_IXR_NACK_MASK | XIICPS_IXR_TO_MASK |
		XIICPS_IXR_COMP_MASK | XIICPS_IXR_ARB_LOST_MASK);
}

/*****************************************************************************/
/**
* This function initiates an interrupt-driven receive in master mode.
*
* It sets the transfer size register so the slave can send data to us.
* The rest of the work is managed by interrupt handler.
*
* @param	InstancePtr is a pointer to the XIicPs instance.
* @param	MsgPtr is the pointer to the receive buffer.
* @param	ByteCount is the number of bytes to be received.
* @param	SlaveAddr is the address of the slave we are receiving from.
*
* @return	None.
*
* @note		This receive routine is for interrupt-driven transfer only.
*
****************************************************************************/
void XIicPs_MasterRecv(XIicPs *InstancePtr, u8 *MsgPtr, int ByteCount,
		 u16 SlaveAddr)
{
	u32 BaseAddr;

	/*
	 * Assert validates the input arguments.
	 */
	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(MsgPtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);
	Xil_AssertVoid(XIICPS_ADDR_MASK >= SlaveAddr);

	BaseAddr = InstancePtr->Config.BaseAddress;
	InstancePtr->RecvBufferPtr = MsgPtr;
	InstancePtr->RecvByteCount = ByteCount;
	InstancePtr->SendBufferPtr = NULL;

	/*
	 * Initialize for a master receiving role.
	 */
	XIicPs_SetupMaster(InstancePtr, RECVING_ROLE);

	XIicPs_EnableInterrupts(BaseAddr,
		XIICPS_IXR_NACK_MASK | XIICPS_IXR_TO_MASK |
		XIICPS_IXR_DATA_MASK |XIICPS_IXR_RX_OVR_MASK |
		XIICPS_IXR_COMP_MASK | XIICPS_IXR_ARB_LOST_MASK);

	/*
	 * Do the address transfer to signal the slave.
	 */
	XIicPs_WriteReg(BaseAddr, XIICPS_ADDR_OFFSET, SlaveAddr);

	/*
	 * Setup the transfer size register so the slave knows how much
	 * to send to us.
	 */
	if (ByteCount > XIICPS_FIFO_DEPTH) {
		XIicPs_WriteReg(BaseAddr, XIICPS_TRANS_SIZE_OFFSET,
			 XIICPS_FIFO_DEPTH);
	} else {
		XIicPs_WriteReg(BaseAddr, XIICPS_TRANS_SIZE_OFFSET,
				ByteCount);
	}
}

/*****************************************************************************/
/**
* This function initiates a polled mode send in master mode.
*
* It sends data to the FIFO and waits for the slave to pick them up.
* If slave fails to remove data from FIFO, the send fails with
* time out.
*
* @param	InstancePtr is a pointer to the XIicPs instance.
* @param	MsgPtr is the pointer to the send buffer.
* @param	ByteCount is the number of bytes to be sent.
* @param	SlaveAddr is the address of the slave we are sending to.
*
* @return
*		- XST_SUCCESS if everything went well.
*		- XST_FAILURE if timed out.
*
* @note		This send routine is for polled mode transfer only.
*
****************************************************************************/
int XIicPs_MasterSendPolled(XIicPs *InstancePtr, u8 *MsgPtr,
		 int ByteCount, u16 SlaveAddr)
{
	u32 IntrStatusReg;
	u32 StatusReg;
	u32 BaseAddr;
	u32 Intrs;

	/*
	 * Assert validates the input arguments.
	 */
	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(MsgPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);
	Xil_AssertNonvoid(XIICPS_ADDR_MASK >= SlaveAddr);

	BaseAddr = InstancePtr->Config.BaseAddress;
	InstancePtr->SendBufferPtr = MsgPtr;
	InstancePtr->SendByteCount = ByteCount;

	XIicPs_SetupMaster(InstancePtr, SENDING_ROLE);

	XIicPs_WriteReg(BaseAddr, XIICPS_ADDR_OFFSET, SlaveAddr);

	/*
	 * Intrs keeps all the error-related interrupts.
	 */
	Intrs = XIICPS_IXR_ARB_LOST_MASK | XIICPS_IXR_TX_OVR_MASK |
			XIICPS_IXR_TO_MASK | XIICPS_IXR_NACK_MASK;

	/*
	 * Clear the interrupt status register before use it to monitor.
	 */
	IntrStatusReg = XIicPs_ReadReg(BaseAddr, XIICPS_ISR_OFFSET);
	XIicPs_WriteReg(BaseAddr, XIICPS_ISR_OFFSET, IntrStatusReg);

	/*
	 * Transmit first FIFO full of data.
	 */
	TransmitFifoFill(InstancePtr);

	IntrStatusReg = XIicPs_ReadReg(BaseAddr, XIICPS_ISR_OFFSET);

	/*
	 * Continue sending as long as there is more data and
	 * there are no errors.
	 */
	while ((InstancePtr->SendByteCount > 0) &&
		((IntrStatusReg & Intrs) == 0)) {
		StatusReg = XIicPs_ReadReg(BaseAddr, XIICPS_SR_OFFSET);

		/*
		 * Wait until transmit FIFO is empty.
		 */
		if ((StatusReg & XIICPS_SR_TXDV_MASK) != 0) {
			IntrStatusReg = XIicPs_ReadReg(BaseAddr,
					XIICPS_ISR_OFFSET);
			continue;
		}

		/*
		 * Send more data out through transmit FIFO.
		 */
		TransmitFifoFill(InstancePtr);
	}
	
	/*
	 * Check for completion of transfer.
	 */
	while ((XIicPs_ReadReg(BaseAddr, XIICPS_ISR_OFFSET) &
		XIICPS_IXR_COMP_MASK) != XIICPS_IXR_COMP_MASK);

	/*
	 * If there is an error, tell the caller.
	 */
	if (IntrStatusReg & Intrs) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
* This function initiates a polled mode receive in master mode.
*
* It repeatedly sets the transfer size register so the slave can
* send data to us. It polls the data register for data to come in.
* If slave fails to send us data, it fails with time out.
*
* @param	InstancePtr is a pointer to the XIicPs instance.
* @param	MsgPtr is the pointer to the receive buffer.
* @param	ByteCount is the number of bytes to be received.
* @param	SlaveAddr is the address of the slave we are receiving from.
*
* @return
*		- XST_SUCCESS if everything went well.
*		- XST_FAILURE if timed out.
*
* @note		This receive routine is for polled mode transfer only.
*
****************************************************************************/
int XIicPs_MasterRecvPolled(XIicPs *InstancePtr, u8 *MsgPtr,
				int ByteCount, u16 SlaveAddr)
{
	u32 IntrStatusReg;
	u32 Intrs;
	u32 StatusReg;
	u32 BaseAddr;
	int BytesToRecv;
	int BytesToRead;
	int TransSize;
	int Tmp;

	/*
	 * Assert validates the input arguments.
	 */
	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(MsgPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);
	Xil_AssertNonvoid(XIICPS_ADDR_MASK >= SlaveAddr);

	BaseAddr = InstancePtr->Config.BaseAddress;
	InstancePtr->RecvBufferPtr = MsgPtr;
	InstancePtr->RecvByteCount = ByteCount;

	XIicPs_SetupMaster(InstancePtr, RECVING_ROLE);

	XIicPs_WriteReg(BaseAddr, XIICPS_ADDR_OFFSET, SlaveAddr);

	/*
	 * Intrs keeps all the error-related interrupts.
	 */
	Intrs = XIICPS_IXR_ARB_LOST_MASK | XIICPS_IXR_RX_OVR_MASK |
			XIICPS_IXR_RX_UNF_MASK | XIICPS_IXR_TO_MASK |
			XIICPS_IXR_NACK_MASK;

	/*
	 * Clear the interrupt status register before use it to monitor.
	 */
	IntrStatusReg = XIicPs_ReadReg(BaseAddr, XIICPS_ISR_OFFSET);
	XIicPs_WriteReg(BaseAddr, XIICPS_ISR_OFFSET, IntrStatusReg);

	/*
	 * Set up the transfer size register so the slave knows how much
	 * to send to us.
	 */
	if (ByteCount > XIICPS_FIFO_DEPTH) {
		XIicPs_WriteReg(BaseAddr, XIICPS_TRANS_SIZE_OFFSET,
			 XIICPS_FIFO_DEPTH);
	}else {
		XIicPs_WriteReg(BaseAddr, XIICPS_TRANS_SIZE_OFFSET,
			 ByteCount);
	}

	/*
	 * Pull the interrupt status register to find the errors.
	 */
	IntrStatusReg = XIicPs_ReadReg(BaseAddr, XIICPS_ISR_OFFSET);
	while ((InstancePtr->RecvByteCount > 0) &&
			((IntrStatusReg & Intrs) == 0)) {
		StatusReg = XIicPs_ReadReg(BaseAddr, XIICPS_SR_OFFSET);

		/*
		 * If there is no data in the FIFO, check the interrupt
		 * status register for error, and continue.
		 */
		if ((StatusReg & XIICPS_SR_RXDV_MASK) == 0) {
			IntrStatusReg = XIicPs_ReadReg(BaseAddr,
					XIICPS_ISR_OFFSET);
			continue;
		}

		/*
		 * The transfer size register shows how much more data slave
		 * needs to send to us.
		 */
		TransSize = XIicPs_ReadReg(BaseAddr,
		XIICPS_TRANS_SIZE_OFFSET);

		BytesToRead = InstancePtr->RecvByteCount;

		/*
		 * If expected number of bytes is greater than FIFO size,
		 * the master needs to wait for data comes in and set the
		 * transfer size register for slave to send more.
		 */
		if (InstancePtr->RecvByteCount > XIICPS_FIFO_DEPTH) {
			/* wait slave to send data */
			while ((TransSize > 2) &&
				((IntrStatusReg & Intrs) == 0)) {
				TransSize = XIicPs_ReadReg(BaseAddr,
						XIICPS_TRANS_SIZE_OFFSET);
				IntrStatusReg = XIicPs_ReadReg(BaseAddr,
							XIICPS_ISR_OFFSET);
			}

			/*
			 * If timeout happened, it is an error.
			 */
			if (IntrStatusReg & XIICPS_IXR_TO_MASK) {
				return XST_FAILURE;
			}
			TransSize = XIicPs_ReadReg(BaseAddr,
						XIICPS_TRANS_SIZE_OFFSET);

			/*
			 * Take trans size into account of how many more should
			 * be received.
			 */
			BytesToRecv = InstancePtr->RecvByteCount -
					XIICPS_FIFO_DEPTH + TransSize;

			/* Tell slave to send more to us */
			if (BytesToRecv > XIICPS_FIFO_DEPTH) {
				XIicPs_WriteReg(BaseAddr,
					XIICPS_TRANS_SIZE_OFFSET,
					XIICPS_FIFO_DEPTH);
			} else{
				XIicPs_WriteReg(BaseAddr,
					XIICPS_TRANS_SIZE_OFFSET, BytesToRecv);
			}

			BytesToRead = XIICPS_FIFO_DEPTH - TransSize;
		}

		Tmp = 0;
		IntrStatusReg = XIicPs_ReadReg(BaseAddr, XIICPS_ISR_OFFSET);
		while ((Tmp < BytesToRead) &&
				((IntrStatusReg & Intrs) == 0)) {
			StatusReg = XIicPs_ReadReg(BaseAddr,
					XIICPS_SR_OFFSET);
			IntrStatusReg = XIicPs_ReadReg(BaseAddr,
					XIICPS_ISR_OFFSET);

			if ((StatusReg & XIICPS_SR_RXDV_MASK) == 0) {
				/* No data in fifo */
				continue;
			}
			XIicPs_RecvByte(InstancePtr);
			Tmp ++;
		}
	}

	if (IntrStatusReg & Intrs) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
* This function enables the slave monitor mode.
*
* It enables slave monitor in the control register and enables
* slave ready interrupt. It then does an address transfer to slave.
* Interrupt handler will signal the caller if slave responds to
* the address transfer.
*
* @param	InstancePtr is a pointer to the XIicPs instance.
* @param	SlaveAddr is the address of the slave we want to contact.
*
* @return	None.
*
* @note		None.
*
****************************************************************************/
void XIicPs_EnableSlaveMonitor(XIicPs *InstancePtr, u16 SlaveAddr)
{
	u32 BaseAddr;

	Xil_AssertVoid(InstancePtr != NULL);

	BaseAddr = InstancePtr->Config.BaseAddress;

	/*
	 * Enable slave monitor mode in control register.
	 */
	XIicPs_WriteReg(BaseAddr, XIICPS_CR_OFFSET,
	XIicPs_ReadReg(BaseAddr, XIICPS_CR_OFFSET) |
				XIICPS_CR_MS_MASK |
				XIICPS_CR_NEA_MASK |
				XIICPS_CR_SLVMON_MASK );

	/*
	 * Set up interrupt flag for slave monitor interrupt.
	 */
	XIicPs_EnableInterrupts(BaseAddr, XIICPS_IXR_TO_MASK |
		XIICPS_IXR_NACK_MASK | XIICPS_IXR_SLV_RDY_MASK);

	/*
	 * Initialize the slave monitor register.
	 */
	XIicPs_WriteReg(BaseAddr, XIICPS_SLV_PAUSE_OFFSET, 0xF);

	/*
	 * Set the slave address to start the slave address transmission.
	 */
	XIicPs_WriteReg(BaseAddr, XIICPS_ADDR_OFFSET, SlaveAddr);

	return;
}

/*****************************************************************************/
/**
* This function disables slave monitor mode.
*
* @param	InstancePtr is a pointer to the XIicPs instance.
*
* @return	None.
*
* @note		None.
*
****************************************************************************/
void XIicPs_DisableSlaveMonitor(XIicPs *InstancePtr)
{
	u32 BaseAddr;

	Xil_AssertVoid(InstancePtr != NULL);

	BaseAddr = InstancePtr->Config.BaseAddress;

	/*
	 * Clear slave monitor control bit.
	 */
	XIicPs_WriteReg(BaseAddr, XIICPS_CR_OFFSET,
		XIicPs_ReadReg(BaseAddr, XIICPS_CR_OFFSET)
			& (~XIICPS_CR_SLVMON_MASK));

	/*
	 * Clear interrupt flag for slave monitor interrupt.
	 */
	XIicPs_DisableInterrupts(BaseAddr, XIICPS_IXR_SLV_RDY_MASK);

	return;
}

/*****************************************************************************/
/**
* The interrupt handler for the master mode. It does the protocol handling for
* the interrupt-driven transfers.
*
* Completion events and errors are signaled to upper layer for proper handling.
*
* <pre>
* The interrupts that are handled are:
* - DATA
*	This case is handled only for master receive data.
*	The master has to request for more data (if there is more data to
*	receive) and read the data from the FIFO .
*
* - COMP
*	If the Master is transmitting data and there is more data to be
*	sent then the data is written to the FIFO. If there is no more data to
*	be transmitted then a completion event is signalled to the upper layer
*	by calling the callback handler.
*
*	If the Master is receiving data then the data is read from the FIFO and
*	the Master has to request for more data (if there is more data to
*	receive). If all the data has been received then a completion event
*	is signalled to the upper layer by calling the callback handler.
*	It is an error if the amount of received data is more than expected.
*
* - NAK and SLAVE_RDY
*	This is signalled to the upper layer by calling the callback handler.
*
* - All Other interrupts
*	These interrupts are marked as error. This is signalled to the upper
*	layer by calling the callback handler.
*
* </pre>
*
* @param	InstancePtr is a pointer to the XIicPs instance.
*
* @return	None.
*
* @note 	None.
*
****************************************************************************/
void XIicPs_MasterInterruptHandler(XIicPs *InstancePtr)
{
	u32 IntrStatusReg;
	u32 IsSend = 0;
	u32 StatusEvent = 0;
	u32 BaseAddr;
	int Tmp;
	int BytesToRecv;

	/*
	 * Assert validates the input arguments.
	 */
	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

	BaseAddr = InstancePtr->Config.BaseAddress;

	/*
	 * Read the Interrupt status register.
	 */
	IntrStatusReg = XIicPs_ReadReg(BaseAddr,
					 XIICPS_ISR_OFFSET);

	/*
	 * Write the status back to clear the interrupts so no events are missed
	 * while processing this interrupt.
	 */
	XIicPs_WriteReg(BaseAddr, XIICPS_ISR_OFFSET, IntrStatusReg);

	/*
	 * Use the Mask register AND with the Interrupt Status register so
	 * disabled interrupts are not processed.
	 */
	IntrStatusReg &= ~(XIicPs_ReadReg(BaseAddr, XIICPS_IMR_OFFSET));

	/*
	 * Data interrupt.
	 *
	 * In master mode, this means master receiving needs to put more data
	 * into the FIFO. In order to avoid slave times out waiting for ack,
	 * transfer size register must be set before data is processed.
	 *
	 */
	if (0 != (IntrStatusReg & XIICPS_IXR_DATA_MASK)) {

		/*
		 * Only greater than FIFO size is handled here, otherwise, the
		 * COMP interrupt will be triggered shortly, and we will handle
		 * those receives there.
		 */
		if ((InstancePtr->RecvByteCount) > XIICPS_FIFO_DEPTH){
			/* First find out how many bytes slave has sent us */
			BytesToRecv = XIICPS_FIFO_DEPTH -
					XIicPs_ReadReg(BaseAddr,
						XIICPS_TRANS_SIZE_OFFSET);

			if ((InstancePtr->RecvByteCount - BytesToRecv)
					> XIICPS_FIFO_DEPTH) {
				XIicPs_WriteReg(BaseAddr,
					XIICPS_TRANS_SIZE_OFFSET,
					XIICPS_FIFO_DEPTH);
			} else {
				XIicPs_WriteReg(BaseAddr,
					XIICPS_TRANS_SIZE_OFFSET,
					(InstancePtr->RecvByteCount -
					BytesToRecv));
			}

			/*
			 * Receive the data out of the FIFO.
			 */
			for(Tmp = 0; Tmp < BytesToRecv; Tmp ++) {
				XIicPs_RecvByte(InstancePtr);
			}

			/*
			 * For receiving of larger than FIFO size, this is all
			 * the handling we need to do.
			 */
			return;
	 	}
	}

	/*
	 * Determine whether the device is sending.
	 */
	if (InstancePtr->RecvBufferPtr == NULL) {
		IsSend = 1;
	}

	/*
	 * Complete flag.
	 */
	if (0 != (IntrStatusReg & XIICPS_IXR_COMP_MASK)) {
		if (IsSend) {
			if (InstancePtr->SendByteCount > 0) {
				MasterSendData(InstancePtr);
			} else {
				StatusEvent |= XIICPS_EVENT_COMPLETE_SEND;
			}
		} else {
			/*
			 * Get the data out of FIFO first, if not done,
			 * tell the slave to send more.
			 */
			while (XIicPs_ReadReg(BaseAddr, XIICPS_SR_OFFSET) &
					XIICPS_SR_RXDV_MASK) {
				XIicPs_RecvByte(InstancePtr);
			}

			/*
			 * Continue to tell slave to send data if not done.
			 */
			if (InstancePtr->RecvByteCount > XIICPS_FIFO_DEPTH) {

				XIicPs_WriteReg(
					InstancePtr->Config.BaseAddress,
					XIICPS_TRANS_SIZE_OFFSET,
					XIICPS_FIFO_DEPTH);

			} else if (InstancePtr->RecvByteCount > 0) {

				XIicPs_WriteReg(
					InstancePtr->Config.BaseAddress,
					XIICPS_TRANS_SIZE_OFFSET,
					InstancePtr->RecvByteCount);
			}

			/*
			 * If all done, tell the application.
			 */
			if (InstancePtr->RecvByteCount == 0){
				StatusEvent |= XIICPS_EVENT_COMPLETE_RECV;
			}

			/*
			 * If received more than expected, it is an error.
			 */
			if (InstancePtr->RecvByteCount < 0){
				StatusEvent |= XIICPS_EVENT_ERROR;
			}
		}
	}

	/*
	 * Slave ready interrupt, it is only meaningful for master mode.
	 */
	if (0 != (IntrStatusReg & XIICPS_IXR_SLV_RDY_MASK)) {
		StatusEvent |= XIICPS_EVENT_SLAVE_RDY;
	}

	if (0 != (IntrStatusReg & XIICPS_IXR_NACK_MASK)) {
		StatusEvent |= XIICPS_EVENT_NACK;
	}

	/*
	 * All other interrupts are treated as error.
	 */
	if (0 != (IntrStatusReg & (XIICPS_IXR_TO_MASK | XIICPS_IXR_NACK_MASK |
			XIICPS_IXR_ARB_LOST_MASK | XIICPS_IXR_RX_UNF_MASK |
			XIICPS_IXR_TX_OVR_MASK | XIICPS_IXR_RX_OVR_MASK))) {
		StatusEvent |= XIICPS_EVENT_ERROR;
	}

	/*
	 * Signal application if there are any events.
	 */
	if (0 != StatusEvent) {
		InstancePtr->StatusHandler(InstancePtr->CallBackRef,
					   StatusEvent);
	}

}

/*****************************************************************************/
/*
* This function prepares a device to transfers as a master.
*
* @param	InstancePtr is a pointer to the XIicPs instance.
*
* @param	Role specifies whether the device is sending or receiving.
*
* @return
*		- XST_SUCCESS if everything went well.
*		- XST_FAILURE if bus is busy.
*
* @note		Interrupts are always disabled, device which needs to use
*		interrupts needs to setup interrupts after this call.
*
****************************************************************************/
static int XIicPs_SetupMaster(XIicPs *InstancePtr, int Role)
{
	u32 ControlReg;
	u32 BaseAddr;
	u32 EnabledIntr = 0x0;

	Xil_AssertNonvoid(InstancePtr != NULL);

	BaseAddr = InstancePtr->Config.BaseAddress;
	ControlReg = XIicPs_ReadReg(BaseAddr, XIICPS_CR_OFFSET);


	/*
	 * Only check if bus is busy when repeated start option is not set.
	 */
	if ((ControlReg & XIICPS_CR_HOLD_MASK) == 0) {
		if (XIicPs_BusIsBusy(InstancePtr)) {
			return XST_FAILURE;
		}
	}

	/*
	 * Set up master, AckEn, nea and also clear fifo.
	 */
	ControlReg |= XIICPS_CR_ACKEN_MASK | XIICPS_CR_CLR_FIFO_MASK |
		 	XIICPS_CR_NEA_MASK | XIICPS_CR_MS_MASK;

	if (Role == RECVING_ROLE) {
		ControlReg |= XIICPS_CR_RD_WR_MASK;
		EnabledIntr = XIICPS_IXR_DATA_MASK |XIICPS_IXR_RX_OVR_MASK;
	}else {
		ControlReg &= ~XIICPS_CR_RD_WR_MASK;
	}
	EnabledIntr |= XIICPS_IXR_COMP_MASK | XIICPS_IXR_ARB_LOST_MASK;

	XIicPs_WriteReg(BaseAddr, XIICPS_CR_OFFSET, ControlReg);

	XIicPs_DisableAllInterrupts(BaseAddr);

	return XST_SUCCESS;
}

/*****************************************************************************/
/*
* This function handles continuation of sending data. It is invoked
* from interrupt handler.
*
* @param	InstancePtr is a pointer to the XIicPs instance.
*
* @return	None.
*
* @note		None.
*
****************************************************************************/
static void MasterSendData(XIicPs *InstancePtr)
{
	TransmitFifoFill(InstancePtr);

	/*
	 * Clear repeated start if done, so stop can be sent out.
	 */
	if (InstancePtr->SendByteCount == 0) {

		/*
		 * If user has enabled repeated start as an option,
		 * do not disable it.
		 */
		if ((InstancePtr->Options & XIICPS_REP_START_OPTION) == 0) {

			XIicPs_WriteReg(InstancePtr->Config.BaseAddress,
				XIICPS_CR_OFFSET,
				XIicPs_ReadReg(InstancePtr->Config.BaseAddress,
				XIICPS_CR_OFFSET) & ~ XIICPS_CR_HOLD_MASK);
		}
	}

	return;
}




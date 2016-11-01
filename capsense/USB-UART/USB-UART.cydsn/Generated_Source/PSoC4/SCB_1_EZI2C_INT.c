/*******************************************************************************
* File Name: SCB_1_EZI2C_INT.c
* Version 1.20
*
* Description:
*  This file provides the source code to the Interrupt Service Routine for
*  the SCB Component in EZI2C mode.
*
* Note:
*
********************************************************************************
* Copyright 2013-2014, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "SCB_1_PVT.h"
#include "SCB_1_EZI2C_PVT.h"

#if(SCB_1_EZI2C_SCL_STRETCH_ENABLE_CONST)
    /*******************************************************************************
    * Function Name: SCB_1_EZI2C_STRETCH_ISR
    ********************************************************************************
    *
    * Summary:
    *  Handles the Interrupt Service Routine for the SCB EZI2C mode. The clock stretching is
    *  used during operation.
    *
    * Parameters:
    *  None
    *
    * Return:
    *  None
    *
    *******************************************************************************/
    CY_ISR_PROTO(SCB_1_EZI2C_STRETCH_ISR)
    {
        static uint16 locBufSize;
        uint32 locIndex;
        uint32 locStatus;

        uint32 endTransfer;
        uint32 fifoIndex;
        uint32 locByte;

        uint32 locIntrCause;
        uint32 locIntrSlave;

    #if(SCB_1_SECONDARY_ADDRESS_ENABLE_CONST)
        /* Variable intended to be used with either buffer */
        static volatile uint8 * SCB_1_dataBuffer; /* Pointer to data buffer              */
        static uint16 SCB_1_bufSizeBuf;           /* Size of buffer1 in bytes            */
        static uint16 SCB_1_protectBuf;           /* Start index of write protected area */

        static uint8 activeAddress;
        uint32 ackResponse;

        ackResponse = SCB_1_EZI2C_ACK_RECEIVED_ADDRESS;
    #endif /* (SCB_1_SECONDARY_ADDRESS_ENABLE_CONST) */

    #if !defined (CY_REMOVE_SCB_1_CUSTOM_INTR_HANDLER)
        if(NULL != SCB_1_customIntrHandler)
        {
            SCB_1_customIntrHandler();
        }
    #else
        CY_SCB_1_CUSTOM_INTR_HANDLER();
    #endif /* !defined (CY_REMOVE_SCB_1_CUSTOM_INTR_HANDLER) */

        /* Make local copy of global variable */
        locIndex = SCB_1_EZI2C_GET_INDEX(activeAddress);

        /* Get interrupt sources */
        locIntrSlave = SCB_1_GetSlaveInterruptSource();
        locIntrCause = SCB_1_GetInterruptCause();

        /* INTR_SLAVE.I2C_ARB_LOST and INTR_SLAVE_I2C.BUS_ERROR */
        /* Handles errors on the bus. There are cases when both bits are set.
        * The error recovery is common: re-enable the scb IP. The content of the RX FIFO is lost.
        */
        if(0u != (locIntrSlave & (SCB_1_INTR_SLAVE_I2C_ARB_LOST |
                                  SCB_1_INTR_SLAVE_I2C_BUS_ERROR)))
        {
            SCB_1_CTRL_REG &= (uint32) ~SCB_1_CTRL_ENABLED; /* Disable SCB block */

        #if(SCB_1_CY_SCBIP_V0)
            if(0u != ((uint8) SCB_1_EZI2C_STATUS_BUSY & SCB_1_curStatus))
        #endif /* (SCB_1_CY_SCBIP_V0) */
            {
                SCB_1_curStatus &= (uint8) ~SCB_1_EZI2C_STATUS_BUSY;
                SCB_1_curStatus |= (uint8)  SCB_1_EZI2C_STATUS_ERR;

                /* INTR_TX_EMPTY is enabled on address phase to receive data */
                if(0u == (SCB_1_GetTxInterruptMode() & SCB_1_INTR_TX_EMPTY))
                {
                    /* Write complete */
                    if(SCB_1_indexBuf1 != SCB_1_offsetBuf1)
                    {
                        SCB_1_curStatus |= (uint8) SCB_1_INTR_SLAVE_I2C_WRITE_STOP;
                    }
                }
                else
                {
                    /* Read complete */
                    SCB_1_curStatus |= (uint8) SCB_1_INTR_SLAVE_I2C_NACK;
                }
            }

            SCB_1_DISABLE_SLAVE_AUTO_DATA;
            
            /* Disable TX and RX interrupt sources */
            SCB_1_SetRxInterruptMode(SCB_1_NO_INTR_SOURCES);
            SCB_1_SetTxInterruptMode(SCB_1_NO_INTR_SOURCES);

        #if(SCB_1_CY_SCBIP_V0)
            /* Clear interrupt sources as they are not automatically cleared after SCB is disabled */
            SCB_1_ClearRxInterruptSource(SCB_1_INTR_RX_ALL);
            SCB_1_ClearSlaveInterruptSource(SCB_1_INTR_SLAVE_ALL);
        #endif /* (SCB_1_CY_SCBIP_V0) */

            SCB_1_fsmState = SCB_1_EZI2C_FSM_IDLE;

            SCB_1_CTRL_REG |= (uint32) SCB_1_CTRL_ENABLED;  /* Enable SCB block */
        }
        else
        {
            /* INTR_I2C_EC_WAKE_UP */
            /* Wakes up device from deep sleep */
            if(0u != (locIntrCause & SCB_1_INTR_CAUSE_I2C_EC))
            {
                /* Disables wakeup interrupt source but do not clear it. It is cleared in INTR_SLAVE_I2C_ADDR_MATCH */
                SCB_1_SetI2CExtClkInterruptMode(SCB_1_NO_INTR_SOURCES);
            }

            if(0u != (locIntrCause & (SCB_1_INTR_CAUSE_RX | SCB_1_INTR_CAUSE_SLAVE)))
            {
                /* INTR_RX.NOT_EMPTY */
                /* Receives data byte-by-byte. Do not use RX FIFO capabilities */
                if (0u != (SCB_1_GetRxInterruptSourceMasked() & SCB_1_INTR_RX_NOT_EMPTY))
                {
                #if (SCB_1_SECONDARY_ADDRESS_ENABLE_CONST)
                    /* If I2C_STOP service is delayed to I2C_ADDR_MATCH the address byte is in the RX FIFO and
                    * RX_NOT_EMPTY is enabled. The address byte has to stay into RX FIFO therefore 
                    * RX.NOT_EMPTY service has to be skipped. The address byte has to be read by I2C_ADDR_MATCH.
                    */
                    if (0u == (locIntrCause & SCB_1_INTR_CAUSE_SLAVE))
                #endif /* (SCB_1_SECONDARY_ADDRESS_ENABLE_CONST) */
                    {
                        locByte = SCB_1_RX_FIFO_RD_REG;

                        switch(SCB_1_fsmState)
                        {
                        case SCB_1_EZI2C_FSM_BYTE_WRITE:
                            if(0u != locBufSize)
                            {
                                /* Store data byte and ACK */
                                SCB_1_I2C_SLAVE_GENERATE_ACK;

                                SCB_1_dataBuffer1[locIndex] = (uint8) locByte;
                                locIndex++;
                                locBufSize--;
                            }
                            else
                            {
                                /* Discard data byte and NACK */
                                SCB_1_I2C_SLAVE_GENERATE_NACK;
                            }
                            break;

                    #if(SCB_1_SUB_ADDRESS_SIZE16_CONST)
                        case SCB_1_EZI2C_FSM_OFFSET_HI8:

                            SCB_1_I2C_SLAVE_GENERATE_ACK;

                            /* Store offset most significant byre */
                            locBufSize = (uint16) ((uint8) locByte);

                            SCB_1_fsmState = SCB_1_EZI2C_FSM_OFFSET_LO8;

                            break;
                    #endif /* (SCB_1_SUB_ADDRESS_SIZE16_CONST) */

                        case SCB_1_EZI2C_FSM_OFFSET_LO8:

                            #if(SCB_1_SUB_ADDRESS_SIZE16)
                            {
                                /* Collect 2 bytes offset */
                                locByte = ((uint32) ((uint32) locBufSize << 8u)) | locByte;
                            }
                            #endif

                            /* Check offset against buffer size */
                            if(locByte < (uint32) SCB_1_bufSizeBuf1)
                            {
                                SCB_1_I2C_SLAVE_GENERATE_ACK;

                                /* Update local buffer index with new offset */
                                locIndex = locByte;

                                /* Get available buffer size to write */
                                locBufSize = (uint16) ((locByte < SCB_1_protectBuf1) ?
                                                       (SCB_1_protectBuf1 - locByte) : (0u));

                            #if (SCB_1_CY_SCBIP_V0)
                                
                                if(locBufSize < SCB_1_FIFO_SIZE)
                                {
                                    /* Set FSM state to receive byte by byte */
                                    SCB_1_fsmState = SCB_1_EZI2C_FSM_BYTE_WRITE;
                                }
                                /* Receive use RX FIFO chunks */
                                else if(locBufSize == SCB_1_FIFO_SIZE)
                                {
                                    SCB_1_ENABLE_SLAVE_AUTO_DATA; /* NACK when RX FIFO is full */
                                    SCB_1_SetRxInterruptMode(SCB_1_NO_INTR_SOURCES);
                                }
                                else
                                {
                                    SCB_1_ENABLE_SLAVE_AUTO_DATA_ACK; /* Stretch when RX FIFO is full */
                                    SCB_1_SetRxInterruptMode(SCB_1_INTR_RX_FULL);
                                }
                                
                            #else
                                
                                #if(SCB_1_SECONDARY_ADDRESS_ENABLE)
                                {
                                    /* Set FSM state to receive byte by byte.  
                                    * The byte by byte receive is always chosen for two addreses. Ticket ID#175559.
                                    */
                                    SCB_1_fsmState = SCB_1_EZI2C_FSM_BYTE_WRITE;
                                }
                                #else
                                {
                                    if(locBufSize < SCB_1_FIFO_SIZE)
                                    {
                                        /* Set FSM state to receive byte by byte */
                                        SCB_1_fsmState = SCB_1_EZI2C_FSM_BYTE_WRITE;
                                    }
                                    /* Receive use RX FIFO chunks */
                                    else if(locBufSize == SCB_1_FIFO_SIZE)
                                    {
                                        SCB_1_ENABLE_SLAVE_AUTO_DATA; /* NACK when RX FIFO is full */
                                        SCB_1_SetRxInterruptMode(SCB_1_NO_INTR_SOURCES);
                                    }
                                    else
                                    {
                                        SCB_1_ENABLE_SLAVE_AUTO_DATA_ACK; /* Stretch when RX FIFO is full */
                                        SCB_1_SetRxInterruptMode(SCB_1_INTR_RX_FULL);
                                    }
                                }
                                #endif
                                
                            #endif /* (SCB_1_CY_SCBIP_V0) */

                                /* Store local offset into global variable */
                                SCB_1_EZI2C_SET_OFFSET(activeAddress, locIndex);
                            }
                            else
                            {
                                /* Discard offset byte and NACK */
                                SCB_1_I2C_SLAVE_GENERATE_NACK;
                            }
                            break;

                        default:
                            CYASSERT(0u != 0u); /* Should never get there */
                            break;
                        }

                        SCB_1_ClearRxInterruptSource(SCB_1_INTR_RX_NOT_EMPTY);
                    }
                }
                /* INTR_RX.FULL, INTR_SLAVE.I2C_STOP */
                /* Receive use FIFO chunks: auto data ACK is enabled */
                else if (0u != (SCB_1_I2C_CTRL_REG & SCB_1_I2C_CTRL_S_READY_DATA_ACK))
                {
                    /* Slave interrupt (I2C_STOP or I2C_ADDR_MATCH) leads to completion of read. */
                    /* The completion event has a high priority than the FIFO full. Read the remaining data from the RX FIFO. */
                    if(0u != (locIntrCause & SCB_1_INTR_CAUSE_SLAVE))
                    {
                        /* Read remaining bytes from RX FIFO */
                        fifoIndex = SCB_1_GET_RX_FIFO_ENTRIES;

                        #if(SCB_1_SECONDARY_ADDRESS_ENABLE)
                        {
                            /* Update with current address match */
                            if(SCB_1_CHECK_INTR_SLAVE_MASKED(SCB_1_INTR_SLAVE_I2C_ADDR_MATCH))
                            {
                                /* Update RX FIFO entries as address byte is there now */
                                fifoIndex = SCB_1_GET_RX_FIFO_ENTRIES;

                                /* If the SR is valid, the RX FIFO is full and the address is in the SHIFTER: read 8 entries and
                                * leave the address in the RX FIFO for further processing.
                                * If the SR is invalid, the address is already in the RX FIFO: read (entries-1).
                                */
                                fifoIndex -= ((0u != SCB_1_GET_RX_FIFO_SR_VALID) ? (0u) : (1u));
                            }
                        }
                        #endif

                        SCB_1_DISABLE_SLAVE_AUTO_DATA;
                        endTransfer = SCB_1_EZI2C_CONTINUE_TRANSFER;
                    }
                    else
                    /* INTR_RX_FULL */
                    /* Continue transfer or disable INTR_RX_FULL to catch completion event. */
                    {
                        /* Calculate buffer size available to write data into */
                        locBufSize -= (uint16) SCB_1_FIFO_SIZE;

                        if(locBufSize <= SCB_1_FIFO_SIZE)
                        {
                            /* Send NACK when RX FIFO overflow */
                            fifoIndex   = locBufSize;
                            endTransfer = SCB_1_EZI2C_COMPLETE_TRANSFER;
                        }
                        else
                        {
                            /* Continue  transaction */
                            fifoIndex   = SCB_1_FIFO_SIZE;
                            endTransfer = SCB_1_EZI2C_CONTINUE_TRANSFER;
                        }
                    }

                    for(; (0u != fifoIndex); fifoIndex--)
                    {
                        /* Store data in the buffer */
                        SCB_1_dataBuffer1[locIndex] = (uint8) SCB_1_RX_FIFO_RD_REG;
                        locIndex++;
                    }

                    /* Complete transfer sending NACK when RX FIFO overflow */
                    if(SCB_1_EZI2C_COMPLETE_TRANSFER == endTransfer)
                    {
                        SCB_1_ENABLE_SLAVE_AUTO_DATA_NACK;

                        /* Disable INTR_RX_FULL while last RX FIFO chunk reception */
                        SCB_1_SetRxInterruptMode(SCB_1_NO_INTR_SOURCES);
                    }

                    SCB_1_ClearRxInterruptSource(SCB_1_INTR_RX_FULL |
                                                            SCB_1_INTR_RX_NOT_EMPTY);
                }
                else
                {
                    /* Exit for slave interrupts which do not active for RX direction:
                    * INTR_SLAVE.I2C_ADDR_MATCH and INTR_SLAVE.I2C_STOP while byte-by-byte reception.
                    */
                }
            }

            if(0u != (locIntrCause & SCB_1_INTR_CAUSE_SLAVE))
            {
                /* INTR_SLAVE.I2C_STOP */
                /* Catch Stop condition: completion of write or read transfer */
            #if(!SCB_1_SECONDARY_ADDRESS_ENABLE_CONST)
                if(0u != (locIntrSlave & SCB_1_INTR_SLAVE_I2C_STOP))
            #else
                /* Prevent triggering when matched address was NACKed */
                if((0u != (locIntrSlave & SCB_1_INTR_SLAVE_I2C_STOP)) &&
                   (0u != ((uint8) SCB_1_EZI2C_STATUS_BUSY & SCB_1_curStatus)))
            #endif
                {
                    /* Disable TX and RX interrupt sources */
                    SCB_1_SetRxInterruptMode(SCB_1_NO_INTR_SOURCES);
                    SCB_1_SetTxInterruptMode(SCB_1_NO_INTR_SOURCES);

                    /* Set read completion mask */
                    locStatus = SCB_1_INTR_SLAVE_I2C_NACK;

                    /* Check if buffer content was modified: the address phase resets the locIndex */
                    if(locIndex != SCB_1_EZI2C_GET_OFFSET(activeAddress))
                    {
                        locStatus |= SCB_1_INTR_SLAVE_I2C_WRITE_STOP;
                    }

                    /* Complete read or write transaction */
                    locStatus &= locIntrSlave;
                    SCB_1_EZI2C_UPDATE_LOC_STATUS(activeAddress, locStatus);
                    locStatus |= (uint32)  SCB_1_curStatus;
                    locStatus &= (uint32) ~SCB_1_EZI2C_STATUS_BUSY;
                    SCB_1_curStatus = (uint8) locStatus;

                    SCB_1_fsmState = SCB_1_EZI2C_FSM_IDLE;

                    #if(SCB_1_SECONDARY_ADDRESS_ENABLE)
                    {
                        /* Store local index into global variable, before addrss phase */
                        SCB_1_EZI2C_SET_INDEX(activeAddress, locIndex);
                    }
                    #endif
                }

                /* INTR_SLAVE.I2C_ADDR_MATCH */
                /* The matched address is received: the slave starts its operation.
                * The INTR_SLAVE_I2C_STOP updates the buffer index before the address phase for two addresses mode.
                * This is done to update buffer index correctly before the address phase changes it.
                */
                if(0u != (locIntrSlave & SCB_1_INTR_SLAVE_I2C_ADDR_MATCH))
                {
                    #if(SCB_1_SECONDARY_ADDRESS_ENABLE)
                    {
                        /* Read address byte from RX FIFO */
                        locByte = SCB_1_GET_I2C_7BIT_ADDRESS(SCB_1_RX_FIFO_RD_REG);

                        SCB_1_ClearRxInterruptSource(SCB_1_INTR_RX_NOT_EMPTY);

                        /* Check received address against device addresses */
                        if(SCB_1_addrBuf1 == locByte)
                        {
                            /* Set buffer exposed to primary slave address */
                            SCB_1_dataBuffer = SCB_1_dataBuffer1;
                            SCB_1_bufSizeBuf = SCB_1_bufSizeBuf1;
                            SCB_1_protectBuf = SCB_1_protectBuf1;

                            activeAddress = SCB_1_EZI2C_ACTIVE_ADDRESS1;
                        }
                        else if(SCB_1_addrBuf2 == locByte)
                        {
                            /* Set buffer exposed to secondary slave address */
                            SCB_1_dataBuffer = SCB_1_dataBuffer2;
                            SCB_1_bufSizeBuf = SCB_1_bufSizeBuf2;
                            SCB_1_protectBuf = SCB_1_protectBuf2;

                            activeAddress = SCB_1_EZI2C_ACTIVE_ADDRESS2;
                        }
                        else
                        {
                            /* Address does not match */
                            ackResponse = SCB_1_EZI2C_NACK_RECEIVED_ADDRESS;
                        }
                    }
                    #endif

                #if(SCB_1_SECONDARY_ADDRESS_ENABLE_CONST)
                    if(SCB_1_EZI2C_NACK_RECEIVED_ADDRESS == ackResponse)
                    {
                        /* Clear interrupt sources before NACK address */
                        SCB_1_ClearI2CExtClkInterruptSource(SCB_1_INTR_I2C_EC_WAKE_UP);
                        SCB_1_ClearSlaveInterruptSource(SCB_1_INTR_SLAVE_ALL);

                    #if(!SCB_1_CY_SCBIP_V0)
                        /* Disable INTR_I2C_STOP to not trigger after matched address is NACKed. Ticket ID#156094 */
                        SCB_1_DISABLE_INTR_SLAVE(SCB_1_INTR_SLAVE_I2C_STOP);
                    #endif /* (!SCB_1_CY_SCBIP_V0) */

                        /* NACK address byte: it does not match neither primary nor secondary */
                        SCB_1_I2C_SLAVE_GENERATE_NACK;
                    }
                    else
                #endif /* (SCB_1_SECONDARY_ADDRESS_ENABLE_CONST) */
                    {

                    #if(SCB_1_SCB_MODE_UNCONFIG_CONST_CFG)
                        if(!SCB_1_SECONDARY_ADDRESS_ENABLE)
                        {
                            /* Set buffer exposed to primary slave address */
                            SCB_1_dataBuffer = SCB_1_dataBuffer1;
                            SCB_1_bufSizeBuf = SCB_1_bufSizeBuf1;
                            SCB_1_protectBuf = SCB_1_protectBuf1;

                            activeAddress = SCB_1_EZI2C_ACTIVE_ADDRESS1;
                        }
                    #endif /* (SCB_1_SCB_MODE_UNCONFIG_CONST_CFG) */

                        /* Bus becomes busy after address is received */
                        SCB_1_curStatus |= (uint8) SCB_1_EZI2C_STATUS_BUSY;

                        /* Slave is read or written: set current offset */
                        locIndex = SCB_1_EZI2C_GET_OFFSET(activeAddress);

                        /* Check transaction direction */
                        if(SCB_1_CHECK_I2C_STATUS(SCB_1_I2C_STATUS_S_READ))
                        {
                            /* Calculate slave buffer size */
                            locBufSize = SCB_1_bufSizeBuf1 - (uint16) locIndex;

                            /* Clear TX FIFO to start fill from offset */
                            SCB_1_CLEAR_TX_FIFO;
                            SCB_1_SetTxInterruptMode(SCB_1_INTR_TX_EMPTY);
                        }
                        else
                        {
                            /* Master writes: enable reception interrupt. The FSM state was set in INTR_SLAVE_I2C_STOP */
                            SCB_1_SetRxInterruptMode(SCB_1_INTR_RX_NOT_EMPTY);
                        }

                        /* Clear interrupt sources before ACK address */
                        SCB_1_ClearI2CExtClkInterruptSource(SCB_1_INTR_I2C_EC_WAKE_UP);
                        SCB_1_ClearSlaveInterruptSource(SCB_1_INTR_SLAVE_ALL);

                    #if(!SCB_1_CY_SCBIP_V0)
                        /* Enable STOP to trigger after address match is ACKed. Ticket ID#156094 */
                        SCB_1_ENABLE_INTR_SLAVE(SCB_1_INTR_SLAVE_I2C_STOP);
                    #endif /* (!SCB_1_CY_SCBIP_V0) */

                        /* ACK the address byte */
                        SCB_1_I2C_SLAVE_GENERATE_ACK;
                    }
                }

                /* Clear slave interrupt sources */
                SCB_1_ClearSlaveInterruptSource(locIntrSlave);
            }

            /* INTR_TX.EMPTY */
            /* Transmits data to the master: loads data into the TX FIFO. The 0xFF sends out if the master reads? out the buffer.
            * The address reception with a read flag clears the TX FIFO to be loaded with data.
            */
            if(0u != (SCB_1_GetInterruptCause() & SCB_1_INTR_CAUSE_TX))
            {
                /* Put data into TX FIFO until there is a room */
                do
                {
                    /* Check transmit buffer range: locBufSize calculates after address reception */
                    if(0u != locBufSize)
                    {
                        SCB_1_TX_FIFO_WR_REG = (uint32) SCB_1_dataBuffer1[locIndex];
                        locIndex++;
                        locBufSize--;
                    }
                    else
                    {
                        SCB_1_TX_FIFO_WR_REG = SCB_1_EZI2C_OVFL_RETURN;
                    }
                }
                while(SCB_1_FIFO_SIZE != SCB_1_GET_TX_FIFO_ENTRIES);

                SCB_1_ClearTxInterruptSource(SCB_1_INTR_TX_EMPTY);
            }
        }

        /* Store local index copy into global variable */
        SCB_1_EZI2C_SET_INDEX(activeAddress, locIndex);
    }
#endif /* (SCB_1_EZI2C_SCL_STRETCH_ENABLE_CONST) */


#if(SCB_1_EZI2C_SCL_STRETCH_DISABLE_CONST)
    /*******************************************************************************
    * Function Name: SCB_1_EZI2C_NO_STRETCH_ISR
    ********************************************************************************
    *
    * Summary:
    *  Handles the Interrupt Service Routine for the SCB EZI2C mode. Clock stretching is
    *  NOT used during operation.
    *
    * Parameters:
    *  None
    *
    * Return:
    *  None
    *
    *******************************************************************************/
    CY_ISR_PROTO(SCB_1_EZI2C_NO_STRETCH_ISR)
    {
    #if(SCB_1_SUB_ADDRESS_SIZE16_CONST)
        static uint8 locOffset;
    #endif /* (SCB_1_SUB_ADDRESS_SIZE16_CONST) */

        uint32 locByte;
        uint32 locStatus;
        uint32 locIntrSlave;
        uint32 locIntrCause;

    #if !defined (CY_REMOVE_SCB_1_CUSTOM_INTR_HANDLER)
        /* Calls registered customer routine to manage interrupt sources */
        if(NULL != SCB_1_customIntrHandler)
        {
            SCB_1_customIntrHandler();
        }
    #else
        CY_SCB_1_CUSTOM_INTR_HANDLER();
    #endif /* !defined (CY_REMOVE_SCB_1_CUSTOM_INTR_HANDLER) */

        locByte = 0u;

        /* Get copy of triggered slave interrupt sources */
        locIntrSlave = SCB_1_GetSlaveInterruptSource();
        locIntrCause = SCB_1_GetInterruptCause();

        /* INTR_SLAVE.I2C_ARB_LOST and INTR_SLAVE.I2C_BUS_ERROR */
        /* Handles errors on the bus: There are cases when both bits are set.
        * The error recovery is common: re-enable the scb IP. The content of the RX FIFO is lost.
        */
        if(0u != (locIntrSlave & (SCB_1_INTR_SLAVE_I2C_ARB_LOST |
                                  SCB_1_INTR_SLAVE_I2C_BUS_ERROR)))
        {
            SCB_1_CTRL_REG &= (uint32) ~SCB_1_CTRL_ENABLED; /* Disable SCB block */

        #if(SCB_1_CY_SCBIP_V0)
            if(0u != (locIntrSlave & SCB_1_INTR_SLAVE_I2C_ADDR_MATCH))
        #endif /* (SCB_1_CY_SCBIP_V0) */
            {
                SCB_1_curStatus |= (uint8) SCB_1_EZI2C_STATUS_ERR;

                if(0u != (SCB_1_EZI2C_FSM_WRITE_MASK & SCB_1_fsmState))
                {
                    /* Write complete */
                    if(SCB_1_indexBuf1 != SCB_1_offsetBuf1)
                    {
                        SCB_1_curStatus |= (uint8) SCB_1_INTR_SLAVE_I2C_WRITE_STOP;
                    }
                }
                else
                {
                    /* Read complete */
                    SCB_1_curStatus |= (uint8) SCB_1_INTR_SLAVE_I2C_NACK;
                }
            }

            /* Clean-up interrupt sources */
            SCB_1_SetTxInterruptMode(SCB_1_NO_INTR_SOURCES);

        #if(SCB_1_CY_SCBIP_V0)
            /* Clear interrupt sources as they are not automatically cleared after SCB is disabled */
            SCB_1_ClearRxInterruptSource(SCB_1_INTR_RX_NOT_EMPTY);
            SCB_1_ClearSlaveInterruptSource(SCB_1_INTR_SLAVE_ALL);
        #endif /* (SCB_1_CY_SCBIP_V0) */

            SCB_1_fsmState = SCB_1_EZI2C_FSM_IDLE;

            SCB_1_CTRL_REG |= (uint32) SCB_1_CTRL_ENABLED;  /* Enable SCB block */
        }
        else
        {
            /* INTR_RX.NOT_EMPTY */
            /* The slave receives data from the master: accepts into the RX FIFO. At least one entry is available to be read.
            * The offset is written first and all the following bytes are data (expected to be put in the buffer).
            * The slave ACKs all bytes, but it discards them if they do not match the write criteria.
            * the slave NACKs the bytes in case of the RX FIFO overflow.
            */
            if(0u != (locIntrCause & SCB_1_INTR_CAUSE_RX))
            {
                /* Read all entries available in RX FIFO */
                do
                {
                    locByte = SCB_1_RX_FIFO_RD_REG;

                    switch(SCB_1_fsmState)
                    {

                    case SCB_1_EZI2C_FSM_BYTE_WRITE:
                        /* Check buffer index against protect area */
                        if(SCB_1_indexBuf1 < SCB_1_protectBuf1)
                        {
                            /* Stores received byte into the buffer */
                            SCB_1_dataBuffer1[SCB_1_indexBuf1] = (uint8) locByte;
                            SCB_1_indexBuf1++;
                        }
                        else
                        {
                            /* Discard current byte and sets FSM state to discard following bytes */
                            SCB_1_fsmState = SCB_1_EZI2C_FSM_WAIT_STOP;
                        }

                        break;

                #if(SCB_1_SUB_ADDRESS_SIZE16_CONST)
                    case SCB_1_EZI2C_FSM_OFFSET_HI8:

                        /* Store high byte of offset */
                        locOffset = (uint8) locByte;

                        SCB_1_fsmState  = SCB_1_EZI2C_FSM_OFFSET_LO8;

                        break;
                #endif /* (SCB_1_SUB_ADDRESS_SIZE16_CONST) */

                    case SCB_1_EZI2C_FSM_OFFSET_LO8:

                        #if(SCB_1_SUB_ADDRESS_SIZE16)
                        {
                            /* Append the offset with high byte */
                            locByte = ((uint32) ((uint32) locOffset << 8u)) | locByte;
                        }
                        #endif

                        /* Check if offset within buffer range */
                        if(locByte < (uint32) SCB_1_bufSizeBuf1)
                        {
                            /* Store and sets received offset */
                            SCB_1_offsetBuf1 = (uint16) locByte;
                            SCB_1_indexBuf1  = (uint16) locByte;

                            /* Move FSM to data receive state */
                            SCB_1_fsmState = SCB_1_EZI2C_FSM_BYTE_WRITE;
                        }
                        else
                        {
                            /* Reset index due to TX FIFO fill */
                            SCB_1_indexBuf1 = (uint16) SCB_1_offsetBuf1;

                            /* Discard current byte and sets FSM state to default to discard following bytes */
                            SCB_1_fsmState = SCB_1_EZI2C_FSM_WAIT_STOP;
                        }

                        break;

                    case SCB_1_EZI2C_FSM_WAIT_STOP:
                        /* Clear RX FIFO to discard all received data */
                        SCB_1_CLEAR_RX_FIFO;

                        break;

                    default:
                        CYASSERT(0u != 0u); /* Should never get there */
                        break;
                    }

                }
                while(0u != SCB_1_GET_RX_FIFO_ENTRIES);

                SCB_1_ClearRxInterruptSource(SCB_1_INTR_RX_NOT_EMPTY);
            }


            /* INTR_SLAVE.I2C_START */
            /* Catches start of transfer to trigger TX FIFO update event */
            if(0u != (locIntrSlave & SCB_1_INTR_SLAVE_I2C_START))
            {
            #if(!SCB_1_CY_SCBIP_V0)
                #if(SCB_1_EZI2C_EC_AM_ENABLE)
                {
                    /* Manage INTR_I2C_EC.WAKE_UP as slave busy status */
                    SCB_1_ClearI2CExtClkInterruptSource(SCB_1_INTR_I2C_EC_WAKE_UP);
                }
                #else
                {
                    /* Manage INTR_SLAVE.I2C_ADDR_MATCH as slave busy status */
                    SCB_1_ClearSlaveInterruptSource(SCB_1_INTR_SLAVE_I2C_ADDR_MATCH);
                }
                #endif
            #else
                /* Manage INTR_SLAVE.I2C_ADDR_MATCH as slave busy status */
                SCB_1_ClearSlaveInterruptSource(SCB_1_INTR_SLAVE_I2C_ADDR_MATCH);
            #endif /* (SCB_1_CY_SCBIP_V0) */

                /* Clear TX FIFO and put a byte */
                SCB_1_CLEAR_TX_FIFO;
                SCB_1_TX_FIFO_WR_REG = (uint32) SCB_1_dataBuffer1[SCB_1_offsetBuf1];

                /* Store buffer index to be handled by INTR_SLAVE.I2C_STOP */
                locByte = (uint32) SCB_1_indexBuf1;

                /* Update index: one byte is already in the TX FIFO */
                SCB_1_indexBuf1 = (uint16) SCB_1_offsetBuf1 + 1u;

                /* Enable INTR_TX.NOT_FULL to load TX FIFO */
                SCB_1_SetTxInterruptMode(SCB_1_INTR_TX_TRIGGER);

                /* Clear locIntrSlave after INTR.TX_TRIGGER is enabled */
                SCB_1_ClearSlaveInterruptSource(locIntrSlave);

                locIntrCause |= SCB_1_INTR_CAUSE_TX;
            }


            /* INTR_TX.NOT_FULL */
            /* Transmits data to the master: loads data into the TX FIFO. At least one empty entry in the TX FIFO available to load.
            * If there is data to transmit, the 0xFF send out.
            * At the Start of any transfer the TX FIFO is loaded with data from the current buffer offset.
            * This action is intended to reduce probability of clock stretching if the TX FIFO and SHIRTER are empty.
            */
            if(0u != (locIntrCause & SCB_1_INTR_CAUSE_TX))
            {
                /* Put data into TX FIFO until there is a room */
                do
                {
                    /* Check transmit buffer range */
                    if(SCB_1_indexBuf1 < SCB_1_bufSizeBuf1)
                    {
                        SCB_1_TX_FIFO_WR_REG = (uint32) SCB_1_dataBuffer1[SCB_1_indexBuf1];
                        SCB_1_indexBuf1++;
                    }
                    else
                    {
                        SCB_1_TX_FIFO_WR_REG = SCB_1_EZI2C_OVFL_RETURN;
                    }

                }
                while(SCB_1_TX_LOAD_SIZE != SCB_1_GET_TX_FIFO_ENTRIES);

                SCB_1_ClearTxInterruptSource(SCB_1_INTR_TX_TRIGGER);
            }


            /* INTR_SLAVE.I2C_STOP */
            /* Catch completion of write or read transfer. */
            if(0u != (locIntrSlave & SCB_1_INTR_SLAVE_I2C_STOP))
            {
                if(0u == (locIntrSlave & SCB_1_INTR_SLAVE_I2C_START))
                {
                #if(!SCB_1_CY_SCBIP_V0)
                    #if(SCB_1_EZI2C_EC_AM_ENABLE)
                    {
                        /* Manage INTR_I2C_EC.WAKE_UP as slave busy status */
                        SCB_1_ClearI2CExtClkInterruptSource(SCB_1_INTR_I2C_EC_WAKE_UP);
                    }
                    #endif
                #endif /* (!SCB_1_CY_SCBIP_V0) */

                    /* Manage INTR_SLAVE.I2C_ADDR_MATCH as slave busy status */
                    SCB_1_ClearSlaveInterruptSource(locIntrSlave);

                    /* Read current buffer index */
                    locByte = (uint32) SCB_1_indexBuf1;
                }

                /* Set read completion mask */
                locStatus = SCB_1_INTR_SLAVE_I2C_NACK;

                if((locByte != SCB_1_offsetBuf1) &&
                   (0u != (SCB_1_EZI2C_FSM_WRITE_MASK & SCB_1_fsmState)))
                {
                    /* Set write completion mask */
                    locStatus |= SCB_1_INTR_SLAVE_I2C_WRITE_STOP;
                }

                /* Set completion flags in the status variable */
                SCB_1_curStatus |= (uint8) (locStatus & locIntrSlave);

                SCB_1_fsmState = SCB_1_EZI2C_FSM_IDLE;
            }


        #if(!SCB_1_CY_SCBIP_V0)
            #if(SCB_1_EZI2C_EC_AM_ENABLE)
            {
                /* INTR_I2C_EC.WAKE_UP */
                /* Wake up device from deep sleep on address match event. The matched address is NACKed */
                if(0u != (locIntrCause & SCB_1_INTR_CAUSE_I2C_EC))
                {
                    SCB_1_I2C_SLAVE_GENERATE_NACK; /* NACK in active mode */
                    SCB_1_ClearI2CExtClkInterruptSource(SCB_1_INTR_I2C_EC_WAKE_UP);
                }
            }
            #endif
        #endif /* (!SCB_1_CY_SCBIP_V0) */
        }
    }
#endif /* (SCB_1_EZI2C_SCL_STRETCH_DISABLE_CONST) */


/* [] END OF FILE */

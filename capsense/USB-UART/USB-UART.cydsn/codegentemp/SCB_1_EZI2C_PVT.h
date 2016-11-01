/*******************************************************************************
* File Name: .h
* Version 1.20
*
* Description:
*  This private file provides constants and parameter values for the
*  SCB Component in _EZI2C mode.
*  Please, do not use this file or its content in your project.
*
* Note:
*
********************************************************************************
* Copyright 2013-2014, Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_SCB_EZI2C_PVT_SCB_1_H)
#define CY_SCB_EZI2C_PVT_SCB_1_H

#include "SCB_1_EZI2C.h"


/***************************************
*      EZI2C Private Vars
***************************************/

extern volatile uint8 SCB_1_curStatus;
extern uint8 SCB_1_fsmState;

/* Variables intended to be used with Buffer1: Primary slave address */
extern volatile uint8 * SCB_1_dataBuffer1;
extern uint16 SCB_1_bufSizeBuf1;
extern uint16 SCB_1_protectBuf1;
extern uint16 SCB_1_offsetBuf1;
extern uint16 SCB_1_indexBuf1;

#if(SCB_1_SECONDARY_ADDRESS_ENABLE_CONST)
    extern uint8 SCB_1_addrBuf1;
    extern uint8 SCB_1_addrBuf2;

    /* Variables intended to be used with Buffer1: Primary slave address */
    extern volatile uint8 * SCB_1_dataBuffer2;
    extern uint16 SCB_1_bufSizeBuf2;
    extern uint16 SCB_1_protectBuf2;
    extern uint16 SCB_1_offsetBuf2;
    extern uint16 SCB_1_indexBuf2;
#endif /* (SCB_1_SECONDARY_ADDRESS_ENABLE_CONST) */


/***************************************
*     Private Function Prototypes
***************************************/

#if(SCB_1_SCB_MODE_EZI2C_CONST_CFG)
    void SCB_1_EzI2CInit(void);
#endif /* (SCB_1_SCB_MODE_EZI2C_CONST_CFG) */

void SCB_1_EzI2CStop(void);
#if(SCB_1_EZI2C_WAKE_ENABLE_CONST)
    void SCB_1_EzI2CSaveConfig(void);
    void SCB_1_EzI2CRestoreConfig(void);
#endif /* (SCB_1_EZI2C_WAKE_ENABLE_CONST) */

#endif /* (CY_SCB__EZI2C_PVT_SCB_1_H) */


/* [] END OF FILE */

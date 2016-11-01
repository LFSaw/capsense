/*******************************************************************************
* File Name: SCB_1_scl.c  
* Version 2.0
*
* Description:
*  This file contains API to enable firmware control of a Pins component.
*
* Note:
*
********************************************************************************
* Copyright 2008-2014, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "cytypes.h"
#include "SCB_1_scl.h"

#define SetP4PinDriveMode(shift, mode)  \
    do { \
        SCB_1_scl_PC =   (SCB_1_scl_PC & \
                                (uint32)(~(uint32)(SCB_1_scl_DRIVE_MODE_IND_MASK << (SCB_1_scl_DRIVE_MODE_BITS * (shift))))) | \
                                (uint32)((uint32)(mode) << (SCB_1_scl_DRIVE_MODE_BITS * (shift))); \
    } while (0)


/*******************************************************************************
* Function Name: SCB_1_scl_Write
********************************************************************************
*
* Summary:
*  Assign a new value to the digital port's data output register.  
*
* Parameters:  
*  prtValue:  The value to be assigned to the Digital Port. 
*
* Return: 
*  None 
*  
*******************************************************************************/
void SCB_1_scl_Write(uint8 value) 
{
    uint8 drVal = (uint8)(SCB_1_scl_DR & (uint8)(~SCB_1_scl_MASK));
    drVal = (drVal | ((uint8)(value << SCB_1_scl_SHIFT) & SCB_1_scl_MASK));
    SCB_1_scl_DR = (uint32)drVal;
}


/*******************************************************************************
* Function Name: SCB_1_scl_SetDriveMode
********************************************************************************
*
* Summary:
*  Change the drive mode on the pins of the port.
* 
* Parameters:  
*  mode:  Change the pins to one of the following drive modes.
*
*  SCB_1_scl_DM_STRONG     Strong Drive 
*  SCB_1_scl_DM_OD_HI      Open Drain, Drives High 
*  SCB_1_scl_DM_OD_LO      Open Drain, Drives Low 
*  SCB_1_scl_DM_RES_UP     Resistive Pull Up 
*  SCB_1_scl_DM_RES_DWN    Resistive Pull Down 
*  SCB_1_scl_DM_RES_UPDWN  Resistive Pull Up/Down 
*  SCB_1_scl_DM_DIG_HIZ    High Impedance Digital 
*  SCB_1_scl_DM_ALG_HIZ    High Impedance Analog 
*
* Return: 
*  None
*
*******************************************************************************/
void SCB_1_scl_SetDriveMode(uint8 mode) 
{
	SetP4PinDriveMode(SCB_1_scl__0__SHIFT, mode);
}


/*******************************************************************************
* Function Name: SCB_1_scl_Read
********************************************************************************
*
* Summary:
*  Read the current value on the pins of the Digital Port in right justified 
*  form.
*
* Parameters:  
*  None 
*
* Return: 
*  Returns the current value of the Digital Port as a right justified number
*  
* Note:
*  Macro SCB_1_scl_ReadPS calls this function. 
*  
*******************************************************************************/
uint8 SCB_1_scl_Read(void) 
{
    return (uint8)((SCB_1_scl_PS & SCB_1_scl_MASK) >> SCB_1_scl_SHIFT);
}


/*******************************************************************************
* Function Name: SCB_1_scl_ReadDataReg
********************************************************************************
*
* Summary:
*  Read the current value assigned to a Digital Port's data output register
*
* Parameters:  
*  None 
*
* Return: 
*  Returns the current value assigned to the Digital Port's data output register
*  
*******************************************************************************/
uint8 SCB_1_scl_ReadDataReg(void) 
{
    return (uint8)((SCB_1_scl_DR & SCB_1_scl_MASK) >> SCB_1_scl_SHIFT);
}


/* If Interrupts Are Enabled for this Pins component */ 
#if defined(SCB_1_scl_INTSTAT) 

    /*******************************************************************************
    * Function Name: SCB_1_scl_ClearInterrupt
    ********************************************************************************
    *
    * Summary:
    *  Clears any active interrupts attached to port and returns the value of the 
    *  interrupt status register.
    *
    * Parameters:  
    *  None 
    *
    * Return: 
    *  Returns the value of the interrupt status register
    *  
    *******************************************************************************/
    uint8 SCB_1_scl_ClearInterrupt(void) 
    {
		uint8 maskedStatus = (uint8)(SCB_1_scl_INTSTAT & SCB_1_scl_MASK);
		SCB_1_scl_INTSTAT = maskedStatus;
        return maskedStatus >> SCB_1_scl_SHIFT;
    }

#endif /* If Interrupts Are Enabled for this Pins component */ 


/* [] END OF FILE */

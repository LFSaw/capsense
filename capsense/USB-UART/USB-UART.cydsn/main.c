/******************************************************************************
* Project Name		: USB-UART
* File Name			: main.c
* Version 			: 1.0
* Device Used		: CY8C4245AXI-483
* Software Used		: PSoC Creator 2.2 SP1
* Compiler    		: ARMGCC 4.4.1
* Related Hardware	: CY8CKIT-042 PSoC 4 Pioneer Kit 
*
******************************************************************************
* ========================================
*
The following firmware was developed by Cypress Semiconductor
This work is licensed under a Creative Commons Attribution 3.0 Unported License.
http://creativecommons.org/licenses/by/3.0/deed.en_US
You are free to:
-To Share — to copy, distribute and transmit the work 
-To Remix — to adapt the work 
-To make commercial use of the work
* ========================================
*
******************************************************************************
*                           THEORY OF OPERATION
* This project demonstrates UART communication over the kit USB-UART Bridge. 
* The project uses the SCB based UART component which sends the string
* "CY8CKIT-042 USB-UART" and loops back the data sent over the COM terminal software.
* 
* UART is configured with
* UART Basic
* 	Mode: Standard
* 	Direction: TX+RX
* 	Baud rate (kbps): 9600
* 	Data bits: 8 bits
* 	Parity: None
* 	Stop bits: 1 bit
* 	Physical connections needed:
* 	Jumper between P0[5] and PSoC 5LP Header J8 P12[6]
* UART Advanced
* 	RX buffer size: 8
* 	TX buffer size: 8
* 	Interrupt: None
*
* Physical Connetion
* Connect jumper wire from the UART RX (P0[4]) of PSoC 4 to J8_10 (P12[7]) of PSoC 5LP
* Connect jumper wire from the UART TX (P0[5]) of PSoC 4 to J8_9 (P12[6]) of PSoC 5LP
* ******************************************************************************/ 
#include <device.h>
#include <stdlib.h>

#define numSense 4

uint16 capValue[numSense];
int k;
char buffer[64];

void main()
{   
    uint8 ch;

    /* Start SCB UART TX+RX operation */    
    UART_Start(); 
    CyGlobalIntEnable;
    CapSense_Start();
   // CapSense_TunerStart();
    // go into TunerComm 
    /*for(;;){
CapSense_TunerComm();
    };
    */
    // return our values from SensePins via buffer
    for(;;)
    {
        CapSense_ScanEnabledWidgets();

        // active wait
        while(CapSense_IsBusy() != 0){
            ;
        } 
        
        for(k=0; k < numSense; k++){
            capValue[k] = CapSense_ReadSensorRaw(k);
        };

        sprintf(buffer, "%u %u %u %u", capValue[0], capValue[1], capValue[2], capValue[3]);
        
        // return values
        UART_UartPutString(buffer);
        UART_UartPutChar(10);
        //UART_UartPutChar(13);
   
    }
}

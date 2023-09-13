/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.8
        Device            :  PIC18F25K80
        Driver Version    :  2.00
 */

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
 */

#include "mcc_generated_files/mcc.h"

//Flag pour le timer
uint8_t flag_timer_1s = 0;
uint8_t Etat = 0;

void timer_1s(void) {
    flag_timer_1s = 1;
}

void main(void) {
    // Initialize the device
    SYSTEM_Initialize();

    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global and Peripheral Interrupts
    // Use the following macros to:

    TMR1_SetInterruptHandler(timer_1s);
    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();

    RXB0CON = 0x60; // Accept all messages

    uCAN_MSG txCan;
    uCAN_MSG rxCan;

    txCan.frame.id = 0x1A0; //CAN transmit
    txCan.frame.dlc = 1;
    txCan.frame.data0 = 0;

    rxCan.frame.id = 0; //CAN Reçu
    rxCan.frame.dlc = 0;
    rxCan.frame.data0 = 0;

    EUSART1_Write(0xFE); //initialisation de l'écran
    EUSART1_Write(0x41);
    EUSART1_Write(0xFE);
    EUSART1_Write(0x51);
    EUSART1_Write(0xFE);
    EUSART1_Write(0x45);
    EUSART1_Write(0x00);
    printf("CAN RECEIVER");
    __delay_ms(1000);

    while (1) {

        //etat de l'onduleur
        if (IO_RA0_GetValue() == 1) {//off
            IO_RC3_SetLow(); //indicateur led
            IO_RC2_SetHigh();
        } else {//on
            IO_RC2_SetLow(); //indicateur led
            IO_RC3_SetHigh();

        }
        
        //Envoyer l'état de l'ondulaire
        if (flag_timer_1s == 1) {
            txCan.frame.data0 = IO_RA0_GetValue();
            CAN_transmit(&txCan);
            flag_timer_1s = 0;
        }
        //recevoir commande et messsage des autres noeuds
        if (CAN_receive(&rxCan) >= 1) {

            if (rxCan.frame.id == 0x100) {//Protection
                if (rxCan.frame.data0 == 0xFF) {//protection

                    EUSART1_Write(0xFE);
                    EUSART1_Write(0x45);
                    EUSART1_Write(0x00);
                    printf("Mode: Protection");
                } else if (rxCan.frame.data0 == 0x00) {//non-protection
                    EUSART1_Write(0xFE);
                    EUSART1_Write(0x45);
                    EUSART1_Write(0x00);
                    printf("Mode: Normal");
                }

            } else if (rxCan.frame.id == 0x120) {//Interface
                if (rxCan.frame.data0 == 0xFF) {//etat off
                    EUSART1_Write(0xFE);
                    EUSART1_Write(0x45);
                    EUSART1_Write(0x40);
                    printf("Etat: ON");
                } else if (rxCan.frame.data0 == 0x00) {//etat on
                    EUSART1_Write(0xFE);
                    EUSART1_Write(0x45);
                    EUSART1_Write(0x40);
                    printf("Etat: OFF");
                }
                
            } else if (rxCan.frame.id == 0x180) {//Batterie
                uint8_t batterie = rxCan.frame.data0;
                EUSART1_Write(0xFE);
                EUSART1_Write(0x45);
                EUSART1_Write(0x14);
                printf("Batterie: %d%", batterie);
            }
        }
    }
}
/**
 End of File
 */
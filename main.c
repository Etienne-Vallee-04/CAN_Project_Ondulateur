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
int Switch = 0;
int Protection = 0;
int Systeme = 0;
int Batterie = 0;
uint8_t charge_LSB = 0;
uint8_t charge_MSB = 0;

enum etat {
    OffSwitch, On, Off
};
int etat = Off;

void timer_1s(void) {
    flag_timer_1s = 1;
}

void main(void) {
    // Initialize the device
    SYSTEM_Initialize();

    TMR1_SetInterruptHandler(timer_1s);
    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    RXB0CON = 0x60; // Accept all messages
    //    RXM0SIDH = 0x07
    //    RXM0SIDL = 0x2F
    //    RXF0SIDH = 0x01
    //    RXF0SIDL = 0x00

    uCAN_MSG txCan;
    uCAN_MSG rxCan;

    txCan.frame.id = 0x1A0; //CAN transmit
    txCan.frame.dlc = 3;
    txCan.frame.data0 = 0;

    rxCan.frame.id = 0; //CAN Re�u
    rxCan.frame.dlc = 0;
    rxCan.frame.data0 = 0;

    EUSART1_Write(0xFE); //initialisation de l'�cran
    EUSART1_Write(0x41);
    EUSART1_Write(0xFE);
    EUSART1_Write(0x51);
    EUSART1_Write(0xFE);
    EUSART1_Write(0x46);
    printf("CAN RECEIVER");
    __delay_ms(1000);

    while (1) {

        //recevoir commande et messsage des autres noeuds
        if (CAN_receive(&rxCan) >= 1) {
            if (rxCan.frame.id == 0x100) {//Protection donn�e
                if (rxCan.frame.data0 != 0x00) {//protection
                    Protection = 1;
                } else {//non-protection
                    Protection = 0;
                }
            } else if (rxCan.frame.id == 0x120) {//Interface donn�e
                if (rxCan.frame.data0 != 0x00) {//alimenter
                    Systeme = 0;
                } else {//non-alimenter
                    Systeme = 1;
                }
            } else if (rxCan.frame.id == 0x180) {//Batterie donn�e
                uint8_t batterie = rxCan.frame.data0;
                if (batterie <= 2) {
                    Batterie = 0;
                } else if (batterie <= 20 || batterie >= 2) {
                    Batterie = 2;
                } else {
                    Batterie = 1;
                }
            }
        }

        //etat de l'onduleur sur la switch
        if (IO_RA0_GetValue() == 1) {//off
            Switch = 0; //�tat of
        } else {//on
            Switch = 1; //etat on
        }

        //Envoyer l'�tat de l'ondulaire avec une Machine � �tat
        switch (etat) {
            case On://transmisssion avec les autres noeuds
                IO_RC2_SetLow(); //led verte allum�e
                if (Batterie == 2) {//avertissement batterie faible
                    IO_RC3_Toggle();
                }else {
                    IO_RC3_SetHigh();
                }
                if (flag_timer_1s == 1) {
                    txCan.frame.data0 = 0xFF; //ON
                    uint32_t charge = ADC_GetConversion(channel_AN8); //convertir la valeur de la charge
                    charge = (charge * 300); //mettre la valeur de l'adc de 0 � 300
                    charge = (charge / 4096);
                    txCan.frame.data1 = charge >> 8;
                    txCan.frame.data2 = charge; //
                    CAN_transmit(&txCan);
                    flag_timer_1s = 0;
                }

                //conditions de changement d'�tat
                if (Protection == 1 || Systeme == 0 || Batterie == 0) {//Off par les noeud
                    etat = Off;
                } else if (Switch == 0) {// off par la switch
                    etat = OffSwitch;
                }
                break;

            case Off://sans impact de la switch
                IO_RC2_SetHigh(); //led rouge allum�e
                IO_RC3_SetLow();
                //conditions de changement d'�tat
                if (Protection == 0 && Systeme == 1 && Batterie == 1) {//Off par les noeud
                    if (Switch == 1) {
                        etat = On;
                    } else {
                        etat = OffSwitch;
                    }
                }
                break;
            case OffSwitch://impact de la switch
                IO_RC2_SetHigh(); //led rouge allum�e
                IO_RC3_SetLow();
                //envoie de l'�tat
                if (flag_timer_1s == 1) {
                    txCan.frame.data0 = 0x00; //OFF
                    CAN_transmit(&txCan);
                    flag_timer_1s = 0;
                }
                //conditions de changement d'�tat
                if (Protection == 1 || Systeme == 0 || Batterie == 0) {//Off par les noeud
                    etat = Off;
                } else if (Switch == 1) {// off par la switch
                    etat = On;
                }
                break;
        }
    }
}
/**
 End of File
 */
//if (rxCan.frame.data0 != 0x00) {//protection
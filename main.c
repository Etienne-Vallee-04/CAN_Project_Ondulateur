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

#include "mcc_generated_files/mcc.h"

//Flag pour le timer
uint8_t flag_timer_1s = 0; //fr�quence de transmission
int Switch = 0; // Etat de l'Int�rrupteur
int Protection = 0; // �tat du Syst�me de Protection
int Systeme = 0; // �tat du Syst�me de l'Interface
int Batterie = 0; // �tat de la Batterie
uint8_t charge_LSB = 0; //niveau de la charge: bit lsb
uint8_t charge_MSB = 0; // niveau de la charge: bit msb

enum etat {
    OFF_Passif, On, Off //etat du noeud
};
int etat = Off;

void timer_1s(void) { //� chaque seconde un flag est activ�e
    flag_timer_1s = 1;
}

void main(void) {
    // Initialize the device
    SYSTEM_Initialize();

    TMR1_SetInterruptHandler(timer_1s); // int�ruption � chaque seconde 
    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    RXB0CON = 0x60; // Accept all messages
    
    //    RXM0SIDH = 0x07 //mask
    //    RXM0SIDL = 0x2F
    //    RXF0SIDH = 0x01 //filtre
    //    RXF0SIDL = 0x00

    uCAN_MSG txCan;
    uCAN_MSG rxCan;

    txCan.frame.id = 0x1A0; //CAN transmit
    txCan.frame.dlc = 3;
    txCan.frame.data0 = 0;

    rxCan.frame.id = 0; //CAN Re�u
    rxCan.frame.dlc = 0;
    rxCan.frame.data0 = 0;

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
                    Batterie = 0; // batterie charg�e
                } else {
                    Batterie = 1; //batterie morte
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
                //afficher leds
                IO_RC2_SetLow(); //led verte allum�e
                IO_RC3_SetHigh();

                //transmission � chaque seconde
                if (flag_timer_1s == 1) { // � chaque seconde
                    txCan.frame.data0 = 0xFF; //ON
                    uint32_t charge = ADC_GetConversion(channel_AN8); //convertir la valeur de la charge
                    charge = (charge * 300); //mettre la valeur de l'adc de 0 � 300
                    charge = (charge / 4096);
                    txCan.frame.data1 = charge >> 8; //metttre la valeur de la charge dans les variables
                    txCan.frame.data2 = charge;
                    CAN_transmit(&txCan); //transimission des donn�es vers l'interface
                    flag_timer_1s = 0;
                }

                //conditions de changement d'�tat
                if (Protection == 1 || Systeme == 0) {//Off par les noeud
                    etat = Off;
                } else if (Switch == 0 || Batterie == 0) {// off par la switch
                    etat = OFF_Passif;
                }
                break;

            case Off://sans impact de la switch
                //afficher leds
                IO_RC2_SetHigh(); //led rouge allum�e
                IO_RC3_SetLow();
                //conditions de changement d'�tat
                if (Protection == 0 && Systeme == 1) {//Off par les noeud
                    if (Switch == 1 && Batterie == 1) { //l'�tat selon l'int�rrupteur
                        etat = On;
                    } else {
                        etat = OFF_Passif;
                    }
                }
                break;
                
            case OFF_Passif://syst�me qui transmet et en off
                //afficher leds
                IO_RC2_SetHigh(); //led rouge allum�e
                IO_RC3_SetLow();
                //envoie de l'�tat
                if (flag_timer_1s == 1) {
                    txCan.frame.data0 = 0x00; //OFF
                    CAN_transmit(&txCan); // transmission des donn�es vers l'interface
                    flag_timer_1s = 0;
                }
                //conditions de changement d'�tat
                if (Protection == 1 || Systeme == 0) {//Off par les noeud
                    etat = Off;
                } else if (Switch == 1 && Batterie == 1) {// off par la switch
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
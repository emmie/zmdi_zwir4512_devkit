/** **************************************************************************
 *
 *  Copyright (c) 2012 Zentrum Mikroelektronik Dresden AG (ZMD AG). All rights
 *  reserved.
 *
 *  This Software is property of Zentrum Mikroelektronik Dresden AG and is
 *  protected under national and international laws ZMD AG shall retain all
 *  intellectual property rights to the Software submitted to the customer.
 *  Any information and documentation in connection with the Software may not
 *  be distributed at any time nor may it be rented or in any other way made
 *  available to third parties unless expressly authorized by ZMD AG. It is
 *  strictly prohibited to modify the Software, to disassemble, reverse
 *  engineer or decompile the Software or separate any part thereof.
 *  Notwithstanding the foregoing, the Source Code provided with this Software
 *  may be modified subject to the condition that the Source Code will be
 *  exclusively used with ZMD AG hardware. Copies may only be made by users
 *  expressly authorized by ZMD AG.
 *
 *  DISCLAIMER. This Software is provided "as is" and without any express or
 *  implied warranties whatsoever as to performance or merchantability. ZMD AG
 *  shall not be liable for any damages arising out of defects resulting from
 *  delivered Software, non-observance of instructions in or misuse, abuse or
 *  use under abnormal conditions in connection with the Software. To the
 *  extent permitted by law ZMD AG hereby expressly disclaims and user
 *  expressly waives any and all claims and warranties, whether express,
 *  implied or statutory, including, without limitation, implied warranties of
 *  merchantability and of fitness for a particular purpose, statutory
 *  warranty of non-infringement and other warranty that may arise by reason
 *  of usage of trade, custom or course of dealing.
 *
 *
 *  \file   ethernt_gateway.c
 *  \brief  Ethernet Gateway basic functions and callback handler
 * 
 ** ***************************************************************************/


#include <stdio.h>
#include <string.h> 
#include <stm32f10x.h>
#include <ZWIR45xx-UART1.h>
#include <ZWIR451x-Devkit.h>
#include <ZWIR45xx-OTAU.h>
#include <ethernet_to_zwir.h>
#include "enc28j60.h"

ZWIR_UART1_PRINTF 

static  int  channel = 0;
static  int  modulation = 0;

void SPI1_Init(void);
void ZMDI_Delay(uint32_t);



uint32_t  ZWIR_vendorID         = 0xe966;
uint16_t  ZWIR_productID        = 0x10;
uint8_t   ZWIR_firmwareMajorVersion     = 1;
uint8_t   ZWIR_firmwareMinorVersion     = 1;
uint16_t  ZWIR_firmwareVersionExtension = 0x1eb3;


void init_ENC28J60() {

  // set Port declarations for ENC28J60 

  GPIOA->CRL = (GPIOA->CRL &~(GPIO_CRL_MODE4| GPIO_CRL_CNF4)) | (GPIO_CRL_MODE4_1); // ENC Chip Select
  GPIOA->ODR |= GPIO_ODR_ODR4;

  GPIOA->CRL = (GPIOA->CRL &~(GPIO_CRL_MODE3| GPIO_CRL_CNF3)) | (GPIO_CRL_MODE3_1); // ENC Reset  
  GPIOA->ODR &= ~GPIO_ODR_ODR3;
  ZMDI_Delay(2);
  GPIOA->ODR |= GPIO_ODR_ODR3;

  // setup exti1
  ZWIR_RegisterAppEventHandler(4, ENC28J60_vInterruptEventHandler);
  ZWIR_RegisterAppEventHandler(0, ENC28J60_vRXEventHandler);

  GPIOA->CRL = (GPIOA->CRL &~(GPIO_CRL_MODE1| GPIO_CRL_CNF1) )| (GPIO_CRL_CNF1_0); // ENC Reset  
  AFIO->EXTICR[1] &=~ AFIO_EXTICR1_EXTI1;

  EXTI->FTSR |= EXTI_FTSR_TR1;
  EXTI->EMR |= EXTI_EMR_MR1;
  EXTI->IMR |= EXTI_IMR_MR1;
  NVIC -> ISER [ 0 ] = 1 << 7;

  initMAC();
}


bool
ZWIR_Error ( int  e ) {
  if ( (e == ZWIR_UART1_eOvfl) || 
       (e == ZWIR_UART1_eParity) || 
       (e == ZWIR_UART1_eFrame) || 
       (e == ZWIR_UART1_eNoise) || 
       (e == ZWIR_UART1_eOvfl) ||
       (e == ZWIR_eHostUnreachable) ) {
    return true;
  }
  ZWIR_Reset ( );
  return false;
}



//-------------------------------------------------------------------------
//  

void ZWIR_AppInitNetwork ( ZWIR_ResetReason_t  reason ) {
  SPI1_Init();
  InitEthToZwir();
  init_ENC28J60();

}

//-------------------------------------------------------------------------

void ZWIR_AppInitNetworkDone ( ZWIR_ResetReason_t  reason ) {
  ZWIR_OTAU_Register ( 1357 );
}

//-------------------------------------------------------------------------


void ZWIR_AppInitHardware ( ZWIR_ResetReason_t  reason ) {
  
  ZWIR_SetParameter(ZWIR_spRoutingTableSize, 255);


  ZWIR_SetOperatingMode(ZWIR_omGateway, (ZWIR_RadioReceiveCallback_t)ZWIRToEthernet);

  IWDG->KR = 0x55555;                 // enable register access
  IWDG->PR = 0x07;                    // 40kHz/256 -> about 156 Hz
  IWDG->RLR= 0x0fff & 782;            // 782: (1/40000s)/256 * 782 => 5 s
  IWDG->KR = 0xaaaa;                  // load counter
  IWDG->KR = 0xcccc;                  // start watchdog
}

//-------------------------------------------------------------------------
static int button1HoldTime = 0;
static int button2HoldTime = 0;

void ZWIR_Main100ms(void) {
  if ( !button1HoldTime ) {
    ZWIR_SetLED1(0);
    ZWIR_SetLED4(0);
    ZWIR_SetLED2(0);
    ZWIR_SetLED3(0);
  }
}

void
LEDEncode ( int  number ) {
  switch ( number ) {
  case 100:
    number = 11;
    break;

  case 101:
    number = 12;
    break;

  case 102:
    number = 13;
    break;
  }
  ZWIR_SetLED1 ( number & 8 );
  ZWIR_SetLED4 ( number & 1 );
  ZWIR_SetLED2 ( number & 4 );
  ZWIR_SetLED3 ( number & 2 );
  
}


void
NextChannel ( ) {
  if ( channel == 10 )
    channel = 100;
  else if ( channel > 101 )
    channel = 0;
  else
    ++channel;

  ZWIR_SetChannel ( channel );    
}


void ZWIR_Main10ms ( ) {
  static bool button1Stop = false;

  if ( ZWIR_GetButton1 ( ) ) {
    if ( !button1Stop ) ++button1HoldTime;
  }
  else {
    button1HoldTime = 0;
    button1Stop = false;
  }

  if ( ZWIR_GetButton2 ( ) ) {
    ++ button2HoldTime;
    button1Stop = true;
    if ( ( button1HoldTime > 0 ) && ( button2HoldTime == 1 ) ) {
      if ( button1HoldTime < 200 ) {
        modulation = ! modulation;
        ZWIR_SetModulation ( modulation );
      }
      else {
        NextChannel ( );
      }
    }
  }
  else {
    button2HoldTime = 0;
  }

  if ( button1HoldTime ) {
    if ( button1HoldTime < 200 ) {
      LEDEncode ( modulation + 1 );
    }
    else {
      LEDEncode ( channel );
    }
  }
}

//-------------------------------------------------------------------------

void SentToZWIR() {
  if ( ! button1HoldTime )
    ZWIR_SetLED1(1);
}

void DroppedToZWIR() {
  if ( ! button1HoldTime )
    ZWIR_SetLED2(1);
}

void SentToEthernet() {
  if ( ! button1HoldTime )
    ZWIR_SetLED3(1);
}

void DroppedToEthernet() {
  if ( ! button1HoldTime )
    ZWIR_SetLED4(1);
}




//-------------------------------------------------------------------------



void ZWIR_Main1000ms(void) {

  IWDG->KR = 0xaaaa;
  ZWIR_TRXStatistic_t trxStatistic = ZWIR_GetTRXStatistic();
  if( ENC28J60_IsAllive() == false )
    ZWIR_Reset ( );

}

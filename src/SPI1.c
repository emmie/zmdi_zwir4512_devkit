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
 *  \file   SPI1.c
 *  \brief  Function for communicating via SPI 1
 * 
 ** ***************************************************************************/

#include "stm32f10x.h"
#define uint8_t unsigned char


/***********************************************************************/
/** \brief Initialise the SP1.
 *
 */
/**********************************************************************/
void SPI1_Init(void) {
  /* SPI init */


  GPIOA->CRL = (GPIOA->CRL &~(GPIO_CRL_MODE5| GPIO_CRL_CNF5)) | (GPIO_CRL_CNF5_1| GPIO_CRL_MODE5_0); //MOSI
  GPIOA->CRL = (GPIOA->CRL &~(GPIO_CRL_MODE6| GPIO_CRL_CNF6)) | (GPIO_CRL_CNF6_0); //MISO
  GPIOA->CRL = (GPIOA->CRL &~(GPIO_CRL_MODE7| GPIO_CRL_CNF7)) | (GPIO_CRL_CNF7_1| GPIO_CRL_MODE7_0); //SCK

  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN; /* SPI1 PeriphClock enabled */

  SPI1->CR1 = 0x0104 | 0x0008;


  SPI1->CR2 |= SPI_CR2_SSOE;

  SPI1->CR1 |= SPI_CR1_SPE;


}

/***********************************************************************/
/** \brief Denitialise the SP1.
 *
 */
/**********************************************************************/
void SPI1_DeInit(void) {
  RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN; /* SPI1 PeriphClock disabled */
}

/***********************************************************************/
/** \brief send byte over SPI1 and receive a byte
 *
 */
/**********************************************************************/


void SPI1_TRXByte(uint8_t input, uint8_t *output) {
  uint8_t *outData;
  while ((SPI1->SR &SPI_SR_TXE) == 0);   // Wait for SPI1 Tx buffer empty
  SPI1->DR = input;                            // Send SPI1 data
  outData = output ? output : &input;
  while ((SPI1->SR &SPI_SR_RXNE) == 0);  // Wait for SPI1 data reception 
  *outData = SPI1->DR;                         // read input data
}

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
 *  \file   enc28j60.c
 *  \brief  ENC28J60 interfacing functions
 * 
 ** ***************************************************************************/


#include "ZWIR45xx-6LoWPAN.h"
#include <ethernet_to_zwir.h>
#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "enc28j60.h"


void ZMDI_Delay(uint32_t);
void SPI1_TRXByte(uint8_t input, uint8_t *output);

/******************************************************************************/
/** \file enc28j60.c
 *  \brief Driver code for enc28j60.
 */
/*******************************************************************************/
static uint16_t ReadMacBuffer(void *buffer, uint16_t byteCount);

extern LinkAddress_t myEthernetAddress;

uint8_t flowControl = 0;

static MACPacket_t ethRxBuffer;
static MACPacket_t ethTxBuffer;
unsigned short sizeTx;
unsigned short size;

static bool txRequest = false;
static bool rxRequest = false;

void SPI1_Enable() {
  IO_SET_SPI1_CS(); // ENC CS low
  ZMDI_Delay(2); // Wait some us
}

//-------------------------------------------------------------------------

void SPI1_Disable() {
  ZMDI_Delay(2); // Wait some us
  IO_CLR_SPI1_CS(); // ENC CS high
}



/***********************************************************************/
/** \brief Initialise the MAC.
 *
 * Description: \n
 * a) Setup SPI device. Assume Reb B5 for sub 8MHz operation \n
 * b) Setup buffer ptrs to devide memory in In and Out mem \n
 * c) Setup receive filters (accept only unicast).\n
 * d) Setup MACON registers (MAC control registers)\n
 * e) Setup MAC address
 * f) Setup Phy registers
 */
/**********************************************************************/
extern void initMAC(void) {

  // setup memory by defining ERXST and ERXND
  BankSel(0); // select bank 0

  //wait for OSC
  while (!(ReadETHReg(ESTAT) &ESTAT_CLKRDY));

  WriteCtrReg(ETXSTL, (unsigned char)(TXSTART &0x00ff)); // write ptr to start of Tx packet
  WriteCtrReg(ETXSTH, (unsigned char)((TXSTART &0xff00) >> 8));
  
  WriteCtrReg(ERXSTL, (uint8_t)(RXSTART &0x00ff));
  WriteCtrReg(ERXSTH, (uint8_t)((RXSTART &0xff00) >> 8));
  WriteCtrReg(ERXNDL, (uint8_t)(RXEND &0x00ff));
  WriteCtrReg(ERXNDH, (uint8_t)((RXEND &0xff00) >> 8));
  // Make sure Rx Read ptr is at the start of Rx segment
  WriteCtrReg(ERXRDPTL, (uint8_t)((RXEND) &0x00ff));
  WriteCtrReg(ERXRDPTH, (uint8_t)((RXEND &0xff00) >> 8));

  // set receive filter 
  BankSel(1);
  // set offset to 0
  WriteCtrReg(EPMOL, 0x00);
  WriteCtrReg(EPMOH, 0x00);
  // set bit 0 and bit1 for first 2 Bytes in DEST Address
  WriteCtrReg(EPMM0, 0x00);
  WriteCtrReg(EPMM1, 0x30);
  WriteCtrReg(EPMM2, 0x00);
  WriteCtrReg(EPMM3, 0x00);
  WriteCtrReg(EPMM4, 0x00);
  WriteCtrReg(EPMM5, 0x00);
  WriteCtrReg(EPMM6, 0x00);
  WriteCtrReg(EPMM7, 0x00);
  // set checksumme see calculation section 13.2 in ENC datasheet 
  WriteCtrReg(EPMCSH, 0x86^0xff);
  WriteCtrReg(EPMCSL, 0xdd^0xff);
  // select bank 1


  WriteCtrReg(ERXFCON, ERXFCON_PMEN + ERXFCON_CRCEN);// + ERXFCON_UCEN);


  // Initialise the MAC registers
  BankSel(2); // select bank 2


  WriteCtrReg(MACON1, MACON1_MARXEN + MACON1_TXPAUS + MACON1_RXPAUS); // Enable reception of frames and enable pause functions
  WriteCtrReg(MACLCON2, 63);
  WriteCtrReg(MACON3, MACON3_FRMLNEN +  // Type / len field will be checked
  MACON3_TXCRCEN +  // MAC will append valid CRC
  MACON3_PADCFG0); // All small packets will be padded


  SetBitField(MACON4, MACON4_DEFER);
  WriteCtrReg(MAMXFLL, (uint8_t)(MAXFRAMELEN &0x00ff)); // set max frame len
  WriteCtrReg(MAMXFLH, (uint8_t)((MAXFRAMELEN &0xff00) >> 8));
  WriteCtrReg(MABBIPG, 0x12); // back to back interpacket gap. set as per data sheet
  WriteCtrReg(MAIPGL, 0x12); // non back to back interpacket gap. set as per data sheet
  WriteCtrReg(MAIPGH, 0x0C);
  //Program own MAC address

  BankSel(3);
  WriteCtrReg(MAADR1, myEthernetAddress[5]);
  WriteCtrReg(MAADR2, myEthernetAddress[4]);
  WriteCtrReg(MAADR3, myEthernetAddress[3]);
  WriteCtrReg(MAADR4, myEthernetAddress[2]);
  WriteCtrReg(MAADR5, myEthernetAddress[1]);
  WriteCtrReg(MAADR6, myEthernetAddress[0]);

  // Initialise the PHY registes

  WriteCtrReg(EPAUSL, 0x4b);
  WriteCtrReg(EPAUSH, 0x00); //wait for 

  WritePhyReg(PHCON1, 0x00);
  WritePhyReg(PHCON2, PHCON2_HDLDIS);
  WriteCtrReg(ECON1, ECON1_RXEN); //Enable the chip for reception of packets
  WriteCtrReg(EIE, EIE_PKTIE | EIE_INTIE | EIE_TXIE | EIE_TXERIE | EIE_RXERIE);
  

}

/*------------------------Private Functions-----------------------------*/

/***********************************************************************/
/** \brief ReadETHReg.
 *
 * Description: Reads contents of the addressed ETH reg over SPI bus. Assumes correct bank selected.
 *     
 * \param bytAddress Address of register to be read
 * \return byte Value of register.
 */
/**********************************************************************/
unsigned char ReadETHReg(unsigned char bytAddress) {
  unsigned char result;

  if (bytAddress > 0x1F)
    return FALSE;
  // address invalid, [TO DO] 
  //NVIC -> ICER [ 0 ] = 1 << 7;
  SPI1_Enable();
  SPI1_TRXByte(bytAddress, NULL);
  SPI1_TRXByte(0, &result);
  SPI1_Disable();
  //NVIC -> ISER [ 0 ] = 1 << 7;

  return result;
}

/***********************************************************************/
/** \brief ReadMacReg.
 *
 * Description: Read contents of addressed MAC register over SPI bus. Assumes correct bank selected.
 *      
 * \param bytAddress Address of register to read.
 * \return byte Contens of register just read.
 */
/**********************************************************************/
static unsigned char ReadMacReg(unsigned char bytAddress) {
  unsigned char bytData;

  if (bytAddress > 0x1F)
    return FALSE;

  SPI1_Enable();
  SPI1_TRXByte(bytAddress, NULL);
  SPI1_TRXByte(0, &bytData);
  SPI1_TRXByte(0, &bytData);
  SPI1_Disable();

  return bytData;
}

/***********************************************************************/
/** \brief Write to phy Reg. 
 *
 * Description:  Writing to PHY registers is different to writing the other regeisters in that
 *               the registers can not be accessed directly. This function wraps up the requirements
 *               for writing to the PHY reg. \n \n
 *                
 *               1) Write address of phy reg to MIREGADR. \n
 *               2) Write lower 8 bits of data to MIWRL. \n
 *               3) Write upper 8 bits of data to MIWRL. \n \n            
 *    
 * \param address
 * \param data
 * \return byte
 */
/**********************************************************************/
static unsigned char WritePhyReg(unsigned char address, unsigned short data) {
  if (address > 0x14)
    return FALSE;

  BankSel(2);
  WriteCtrReg(MIREGADR, address); // Write address of Phy reg 
  WriteCtrReg(MIWRL, (unsigned char)data); // lower phy data 
  WriteCtrReg(MIWRH, ((unsigned char)(data >> 8))); // Upper phydata

  return TRUE;
}

/***********************************************************************/
/** \brief Read from PHY reg.
 *
 * Description: No direct access allowed to phy registers so the folling process must take place. \n \n
 *              1) Write address of phy reg to read from into MIREGADR. \n
 *              2) Set MICMD.MIIRD bit and wait 10.4uS. \n
 *              3) Clear MICMD.MIIRD bit. \n
 *              4) Read data from MIRDL and MIRDH reg. \n
 * \param address
 */
/**********************************************************************/
static unsigned short ReadPhyReg(unsigned char address) {
  unsigned short uiData;
  unsigned char bytStat;

  BankSel(2);
  WriteCtrReg(MIREGADR, address); // Write address of phy register to read
  SetBitField(MICMD, MICMD_MIIRD); // Set rd bit
  do {
    bytStat = ReadMacReg(MISTAT);
  } while (bytStat &MISTAT_BUSY);

  ClrBitField(MICMD, MICMD_MIIRD); // Clear rd bit
  uiData = (unsigned short)ReadMacReg(MIRDL); // Read low data byte
  uiData |= ((unsigned short)ReadMacReg(MIRDH) << 8); // Read high data byte

  return uiData;
}

/***********************************************************************/
/** \brief Write to a control reg .
 *
 * Description: Writes a byte to the address register. Assumes that correct bank has
 *              all ready been selected 
 * \param bytAddress Address of register to be written to. 
 * \param bytData    Data to be written. 
 * \returns byte  
 */
/**********************************************************************/
static unsigned char WriteCtrReg(unsigned char bytAddress, unsigned char bytData) {
  if (bytAddress > 0x1f) {
    return FALSE;
  }



  bytAddress |= WCR_OP; // Set opcode
  SPI1_Enable();
  SPI1_TRXByte(bytAddress, NULL);
  SPI1_TRXByte(bytData, NULL);
  SPI1_Disable();
  return TRUE;
}

/***********************************************************************/
/** \brief Read bytes from MAC data buffer.
 *
 * Description: Reads a number of bytes from the ENC28J60 internal memory. Assumes auto increment
 *              is on. 
 * \param bytBuffer  Buffer to place data in.  
 * \param byt_length Number of bytes to read.
 * \return uint  Number of bytes read.
 */
/**********************************************************************/
static uint16_t ReadMacBuffer(void *buffer, uint16_t byteCount) {

  uint8_t bytOpcode = RBM_OP;
  uint32_t i;
  uint8_t t;
  bool biggerThanMTU = false;

  if (byteCount > sizeof(MACPacket_t))
    biggerThanMTU = true;
  SPI1_Enable();
  SPI1_TRXByte(bytOpcode, NULL);
  for (i = 0; i < byteCount; ++i) {
    if (biggerThanMTU)
      SPI1_TRXByte(0, &t);
    else
      SPI1_TRXByte(0, buffer + i);
  }
  SPI1_Disable();
  if (biggerThanMTU)
    return 0;
  return byteCount;
}


/***********************************************************************/
/** \brief Write bytes to MAC data buffer.[UNTESTED]
 *
 * Description: Reads a number of bytes from the ENC28J60 internal memory. Assumes auto increment
 *              is on.
 * \param bytBuffer
 * \param ui_len
 * \return uint
 */
/**********************************************************************/
static unsigned short WriteMacBuffer(unsigned char *bytBuffer, unsigned short ui_len) {

  uint32_t i;
  uint8_t bytOpcode = WBM_OP;
  SPI1_Enable();
  SPI1_TRXByte(bytOpcode, NULL);
  for (i = 0; i < ui_len; ++i)
    SPI1_TRXByte(bytBuffer[i], NULL);
  SPI1_Disable();
  return ui_len;
}

/***********************************************************************/
/** \brief Set bit field. 
 *
 * Description: Sets the bit/s at the address register. Assumes correct bank has been selected.
 * 
 * \param bytAddress Address of registed where bit is to be set
 * \param bytData    Sets all the bits high.
 * \return byte      True or false
 */
/**********************************************************************/
static unsigned char SetBitField(unsigned char bytAddress, unsigned char bytData) {
  if (bytAddress > 0x1f)
    return FALSE;

  bytAddress |= BFS_OP; // Set opcode
  SPI1_Enable();
  SPI1_TRXByte(bytAddress, NULL);
  SPI1_TRXByte(bytData, NULL);
  SPI1_Disable();

  return TRUE;
}

/***********************************************************************/
/** \brief Clear bit field on ctr registers.
 *
 * Description: Sets the bit/s at the address register. Assumes correct bank has been selected.
 *       
 * \param bytAddress Address of registed where bit is to be set
 * \param bytData    Sets all the bits high.
 * \return byte      True or false.
 */
/**********************************************************************/
static unsigned char ClrBitField(unsigned char bytAddress, unsigned char bytData) {
  if (bytAddress > 0x1f)
    return FALSE;

  bytAddress |= BFC_OP; // Set opcode
  SPI1_Enable();
  SPI1_TRXByte(bytAddress, NULL);
  SPI1_TRXByte(bytData, NULL);
  SPI1_Disable();

  return TRUE;
}

/***********************************************************************/
/** \brief Bank Select.
 *
 * Description: Select the required bank within the ENC28J60
 *              
 *              
 * \param bank Value between 0 and 3.
 */
/**********************************************************************/
void BankSel(unsigned char bank) {
  unsigned char temp;

  if (bank > 3)
    return ;

  temp = ReadETHReg(ECON1); // Read ECON1 register
  temp &= ~(ECON1_BSEL|ECON1_TXRTS); // mask off the BSEL bits and the TXRTS bit
  temp |= bank; // set BSEL bits
  WriteCtrReg(ECON1, temp); // write new values back to ENC28J60   
}

/***********************************************************************/
/** \brief ResetMac.
 *
 * Description: Sends command to reset the MAC.
 * 
 */
/**********************************************************************/
extern void ResetMac(void) {
  unsigned char bytOpcode = RESET_OP;
  SPI1_Enable();
  SPI1_TRXByte(bytOpcode, NULL);
  SPI1_Disable();
}

/***********************************************************************/
/** \brief SetRXHashTableEntry.
 *
 * Description: Set MAC Address in ENC28j60 ( HASHTABLE Filter)
 */
/**********************************************************************/

void SetRXHashTableEntry(MAC_ADDR DestMACAddr) {
  DWORD_VAL crc =  {
    0xFFFFFFFF
  };
  unsigned char HTRegister;
  unsigned char i, j;

  // Calculate a CRC-32 over the 6 byte MAC address 
  // using polynomial 0x4C11DB7
  for (i = 0; i < 6; i++) {
    unsigned char crcnext;

    // shift in 8 bits
    for (j = 0; j < 8; j++) {
      crcnext = 0;
      if (((BYTE_VAL*) &(crc.v[3]))->bits.b7)
        crcnext = 1;
      crcnext ^= (((BYTE_VAL*) &DestMACAddr.v[i])->bits.b0);

      crc.Val <<= 1;
      if (crcnext)
        crc.Val ^= 0x4C11DB7;
      // next bit
      DestMACAddr.v[i] >>= 1;
    }
  }

  // CRC-32 calculated, now extract bits 28:23
  // Bits 25:23 define where within the Hash Table byte the bit needs to be set
  // Bits 28:26 define which of the 8 Hash Table bytes that bits 25:23 apply to
  i = crc.v[3] &0x1F;
  HTRegister = (i >> 2) + (unsigned char)EHT0;
  i = (i << 1) &0x06;
  ((BYTE_VAL*) &i)->bits.b0 = ((BYTE_VAL*) &crc.v[2])->bits.b7;

  // Set the proper bit in the Hash Table
  BankSel(1);
  SetBitField(HTRegister, 1 << i);
}

//-------------------------------------------------------------------------

void enable_filter_Hashtable() {
  unsigned char reg;
  BankSel(1);
  reg = ReadETHReg(ERXFCON);
  WriteCtrReg(ERXFCON, ERXFCON_PMEN + reg);
}

//-------------------------------------------------------------------------

void disable_filter_Hashtable() {
  unsigned char reg;
  BankSel(1);
  reg = ReadETHReg(ERXFCON);
  WriteCtrReg(ERXFCON, reg - ERXFCON_PMEN);
}

/***********************************************************************/
/** \brief Tries to read a packet from the ENC28J60.
 *
 * Description: If a valid packet is available in the ENC28J60 this function reads the packet into a
 *              buffer. The memory within the ENC28J60 will then be released. This version of the
driver does not use interrupts so this function needs to be polled.\n \n
 * 
 * 1) Read packet count register. If >0 then continue else return. \n
 * 2) Read the current ERXRDPTR value. \n           
 * 3) Write this value into ERDPT.     \n
 * 4) First two bytes contain the ptr to the start of next packet. Read this value in. \n
 * 5) Calculate length of packet. \n
 * 6) Read in status byte into private variable. \n
 * 7) Read in packet and place into buffer. \n
 * 8) Free up memory in the ENC. \n
 *
 * \param ptrBuffer ptr to buffer of bytes where the packet should be read into. 
 * \return unsigned char , the number of complete packets in the buffer -1.
 */
/**********************************************************************/
extern uint16_t MACRead(void *buffer) {
  uint16_t size;
  bool readError = false;
  static ENCRxStatus_t rxStatus =  {
    (RXSTART) &0xff, (RXSTART) >> 8, 
  };

  BankSel(1);
  if (!ReadETHReg(EPKTCNT))
    return 0;
  // No full packets received

  BankSel(0);
  // Set the read pointer for packet to be received
  WriteCtrReg(ERDPTL, rxStatus . nextPacketPtr . bytes[0]);
  WriteCtrReg(ERDPTH, rxStatus . nextPacketPtr . bytes[1]);

  // read packet status
  ReadMacBuffer(&rxStatus, 6);
  // decrement packet counter
  SetBitField(ECON2, ECON2_PKTDEC);

  if (!ReadMacBuffer(buffer, rxStatus . byteCount))
    readError = true;
  uint16_t erxrdpt = rxStatus . nextPacketPtr . word;
  

    if (erxrdpt == RXSTART)
      erxrdpt = RXEND;
    else
      erxrdpt--;  

  
  // Free the ENC receive buffer by setting the read register to next packet
  WriteCtrReg(ERXRDPTL, erxrdpt&0xff);
  WriteCtrReg(ERXRDPTH, erxrdpt>>8);

  // Drop packet if errors are found
  if (((*(uint16_t*) &rxStatus . bits) &0x31) || readError) 
    return 0;
  return rxStatus . byteCount;
}

/***********************************************************************/
/** \brief Writes a packet to the ENC28J60.
 *
 * Description: Writes ui_len bytes of data from ptrBufffer into ENC28J60.
 *              puts the necessary padding around the packet to make it a legit
MAC packet.\n \n 
1) Program ETXST.   \n
2) Write per packet control byte.\n
3) Program ETXND.\n
4) Set ECON1.TXRTS.\n
5) Check ESTAT.TXABRT. \n
 * \param ptrBuffer ptr to byte buffer. 
 * \param ui_Len Number of bytes to write from buffer. 

 */
/**********************************************************************/
void TransmitToEthernet(unsigned char *ptrBuffer, unsigned short ui_Len) {

  if (ui_Len > sizeof(MACPacket_t))
    return;

  volatile unsigned short address = TXSTART;
  unsigned char bytControl = 0x00;

  if ((ReadETHReg(ECON1) &(ECON1_TXRTS))) {
    if (txRequest){
      //reset transmitter
      SetBitField ( ECON1, ECON1_TXRST );       
      ClrBitField ( ECON1, ECON1_TXRST );  
      txRequest = false;
    } else {
    txRequest = true;
    sizeTx = ui_Len;
    memcpy(&ethTxBuffer,ptrBuffer,ui_Len);
    return; //transmitting is in progress
    }
  }
  
  BankSel(0); // select bank 0


  WriteCtrReg(EWRPTL, (unsigned char)(TXSTART &0x00ff)); // Set write buffer to point to start of Tx Buffer
  WriteCtrReg(EWRPTH, (unsigned char)((TXSTART &0xff00) >> 8));

  WriteMacBuffer(&bytControl, 1); // write per packet control byte
  address++;
  address += WriteMacBuffer(ptrBuffer, ui_Len); // write packet. Assume correct formating src, dst, type  + data

  WriteCtrReg(ETXNDL, (unsigned char)(address &0x00ff)); // Tell MAC when the end of the packet is
  WriteCtrReg(ETXNDH, (unsigned char)((address &0xff00) >> 8));

  SetBitField(ECON1, ECON1_TXRTS); // begin transmitting;

  return;

}

/***********************************************************************/
/** \brief ENC_FORWARD_ZWIR_RETRY.
 *
 * Description: This function will be called periodicly if networkbuffer
 * is not empty and packet can not be sent out.
 */
/**********************************************************************/

static bool retry=false;
uint16_t retries = 0;

void ENC_FORWARD_ZWIR_RETRY(void *data) {
  retry = true;
  ENC28J60_vRXEventHandler();
  retry = false;
}




extern void ENC28J60_vRXEventHandler(void) {
  unsigned short ERXWRPTc;
  unsigned short ERXRDPTc;
  long space1;

  if (retry)
    retries++;


  //try to send frame to ZWIR
  if ( (EtherntToZWIR(&ethRxBuffer, size, retry) == 0) && (retries<300) ) {
    ZWIR_StartCallbackTimer(SENDRETRYMS, ENC_FORWARD_ZWIR_RETRY, (void*)1, 0); //start retry sending to ZWIR timer 
    return ;
  }

  retries = 0;
  //check available enc input buffer
  BankSel(0);
  ERXRDPTc = (ReadETHReg(ERXRDPTH) << 8) | ReadETHReg(ERXRDPTL);
  ERXWRPTc = (ReadETHReg(ERXWRPTH) << 8) | ReadETHReg(ERXWRPTL);

  if (ERXWRPTc > ERXRDPTc)
    space1 = (RXEND - RXSTART) - (ERXWRPTc - ERXRDPTc);
  else if (ERXWRPTc == ERXRDPTc)
    space1 = (RXEND - RXSTART);
  else
    space1 = ERXRDPTc - ERXWRPTc - 1;

  //if enough space available stop flow control
  if (space1 > STOPFLOWCONTROL) {
    BankSel(3);
    WriteCtrReg(EFLOCON, 0x00); //stop flow control
    flowControl = 0;
  }
  else
    flowControl = 1;
  rxRequest = false;
  SetBitField(EIE, EIE_PKTIE); //enable RX irq
  ZWIR_TriggerAppEvent(4);
}

/***********************************************************************/
/** \brief ENC28J60_vInterruptEventHandler.
 *
 * Description: InterrupHandler for ENC28J60.
 * 
 */
/**********************************************************************/

//unsigned char reasonBuffer[256];
//unsigned char reasonPointer;

//unsigned char tmp;

extern void ENC28J60_vInterruptEventHandler(void) {
  unsigned char bReason;

  BankSel(1);
  unsigned char packCnt = ReadETHReg(EPKTCNT);
  
  BankSel(3);
  ClrBitField(EIE, EIE_INTIE); //disable all interrupts
  bReason = ReadETHReg(EIR);
  
  //reasonBuffer[reasonPointer++]=bReason;
  //tmp =  ReadETHReg(EFLOCON);
  
  if ( packCnt && !rxRequest ) {
    rxRequest = true;
    ClrBitField(EIR, EIR_PKTIF);
    ClrBitField(EIE, EIE_PKTIE); //disable RX irq

    BankSel(3);
    WriteCtrReg(EFLOCON, 0x03); //start flow control
    flowControl = 1;
    //read frame from enc
    size = MACRead(&ethRxBuffer);



    //
    ZWIR_TriggerAppEvent(0);


  }
  if (bReason &EIR_DMAIF) {

    ClrBitField(EIR, EIR_DMAIF);
  }
  if (bReason &EIR_LINKIF) {

    ClrBitField(EIR, EIR_LINKIF);
  }

  
  
  if (bReason &EIR_TXERIF) {

    if (!txRequest)
       SetBitField(ECON1, ECON1_TXRTS);//retry sending      
    ClrBitField(EIR, EIR_TXERIF);
  }
  if (bReason &EIR_TXIF) {
    
    /** Workaround for issue 12.*/
    //workaround
    SetBitField ( ECON1, ECON1_TXRST );       
    ClrBitField ( ECON1, ECON1_TXRST );       
    ClrBitField ( EIR, EIR_TXERIF | EIR_TXIF );
    
    if (txRequest) {  //send buffered packet
      TransmitToEthernet((uint8_t*)&ethTxBuffer,sizeTx);      
      txRequest = false;
    }
  }

  
  
  if (bReason &EIR_RXERIF) {
    //buffer full
    BankSel(3);
    ClrBitField(EIR, EIR_RXERIF);
  }

  SetBitField(EIE, EIE_INTIE); //enable all interrupts
}

/***********************************************************************/
/** \brief ISR for EXTI1.
 *
 * Description: ISR for ENC28J60.
 * 
 */
/**********************************************************************/

void __attribute__((interrupt("IRQ")))ZWIR_ISR_EXTI1(void) {
  EXTI->PR = EXTI_PR_PR1;
  ZWIR_TriggerAppEvent(4); 

}


bool ENC28J60_IsAllive() {
  if(flowControl) {
    flowControl++;
    if( flowControl > 5 )
      return false;  //flow control is active more than 5 sec for one packet
  }
  else {
    BankSel(3);
    if (ReadETHReg(EFLOCON)& 0x03)
      return false; //flow control is still active but no RX handling is ongoing --> ENC error
    if (!(ReadETHReg(ECON1) & 0x04))
      return false; //RX is not enable anymore
  }
  
  return true;
}

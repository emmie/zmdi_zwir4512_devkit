
/* ***************************************************************************
 *
 *  Copyright (c) 2011 Zentrum Mikroelektronik Dresden AG
 *
 *  This software is property of Zentrum Mikroelektronik Dresden AG (ZMD AG)
 *  and is protected under national and international laws. Any information
 *  and documentation delivered with this software shall exclusively be used
 *  in combination with ZMD AG products and may not be distributed at any
 *  time nor may it be rented or in any other way made available unless
 *  expressly authorized by ZMD AG. It is strictly prohibited to modify the
 *  software or documentation, to disassemble, reverse engineer or decompile
 *  the software or separate any part thereof. Copies may only be made by
 *  users expressly authorized by ZMD AG.
 *  
 *  Unless otherwise agreed upon under a separate agreement, ZMD AG shall not
 *  be liable for any damages arising out of defects resulting from delivered
 *  hard- and software, non-observance of instructions or misuse, abuse or use
 *  under abnormal conditions in connection with the ZWIR4512. To the extent
 *  permitted by law ZMD AG hereby expressly disclaims and user expressly
 *  waives any and all claims and warranties, whether express, implied or
 *  statutory, including, without limitation, implied warranties of
 *  merchantability and of fitness for a particular purpose, statutory
 *  warranty of non-infringement and other warranty that may arise by reason
 *  of usage of trade, custom or course of dealing.
 *
 *
 *  This is an example to get started with ZWIR451x modules
 *
 *  The program sends packets every 5 seconds to all other devices in the
 *   network. LED2 is flashing when a packet is sent out. The receivers of
 *   the packet will signal the reception by flashing LED3 and printing a
 *   notification on UART.
 *  Besides automatic transmission of blink packet, the program demonstrates
 *   data transmission. For that, data is collected on the UART interface and
 *   sent out when a carriage return is received on the UART or when 32
 *   characters are received. LED1 is flashing when a data packet is sent out
 *   and LED4 is flashing when data is received.
 * ***************************************************************************/

#include <stdio.h>
#include "ZWIR45xx-6LoWPAN.h"
#include "ZWIR45xx-UART1.h"
#include "ZWIR451x-Devkit.h"


// provide printf over UART1
ZWIR_UART1_PRINTF

// define addresses for sending and receiving
ZWIR_IPv6Address_t  addrAll = { IPV6_ALLNODES };
ZWIR_IPv6Address_t  addrUnsp = { IPV6_UNSPECIFIED };

unsigned int   ZWIR_productID                = 0x60;
unsigned char  ZWIR_firmwareMajorVersion     = 1;
unsigned char  ZWIR_firmwareMinorVersion     = 0;
unsigned short ZWIR_firmwareVersionExtension = 0x1ec3;


/* ***************************************************************************\
 *  This is the handler for incoming UART packets.
 *
 *  Incoming data is collected in a buffer and sent out when a carriage return
 *   is received or if the buffer is full.
 * ***************************************************************************/
void SerialReceiveHandler ( void ) {
  // declare buffer variables
  static char buffer [ 32 ];
  static char bufferPt = 0;

  // Read Byte from UART buffer
  while ( ZWIR_UART1_ReadByte ( &buffer [ bufferPt ] ) ) {
    bufferPt++;

    // check if last received byte is a carriage return or if buffer is full
    if ( ( buffer [ bufferPt - 1 ] == '\r' ) || ( bufferPt == 32 ) ) {

      // send data packet to the all-nodes multicast address, port 1000
      ZWIR_SendUDP2 ( buffer, bufferPt, &addrAll, 1000 );

      // switch LED 1 on
      ZWIR_SetLED1(1);

      // reset buffer pointer
      bufferPt = 0;

      // Return from this function to allow the operating system sendign out
      //  the packet. This function will be called again if there are still
      //  bytes in the receive buffer.
      return;
    }
  }
}


/*****************************************************************************\
 *  This is the handler for incoming data packets (UDP packets on port 1000).
 *   It was registered in ZWIR_AppInitNetwork with a call to ZWIR_OpenSocket.
 *
 *  Incoming data are indicated by a message with sender information on UART.
 *   Furthermore, data are written out.
\*****************************************************************************/
void DataCallback ( unsigned char* data, unsigned short size ) {
  int a;

  // Indicate data reception by switching on LED3
  ZWIR_SetLED3 ( 1 );

  // Query and store the address of the sender of this packet
  ZWIR_IPv6Address_t* sender = ZWIR_GetPacketSenderAddress ( );

  // Print out data reception indication
  printf("Received data from ");
  for ( a = 0; a < 16; a += 2 )
    printf ( "%02x%02x:", sender -> u8 [ a ], sender -> u8 [ a + 1 ] );
  printf("\r\n");

  // Print out received characters
  for ( a = 0; a < size; ++ a )
    __putchar( data[a]  );
  printf("\r\n");
}


/*****************************************************************************\
  This is the handler for incoming blink packets (UDP packets on port 1001).
   It was registered in ZWIR_AppInitNetwork with a call to ZWIR_OpenSocket.

  Incoming packets are indicated by a message with sender information over
   UART.
\*****************************************************************************/
void BlinkCallback (unsigned char* data, unsigned short size) {
  int a;

  // Indicate blink reception by switching on LED4
  ZWIR_SetLED4(1);

  // Query and store the address of the sender of this packet
  ZWIR_IPv6Address_t *sender = ZWIR_GetPacketSenderAddress ( );

  // Print out data reception indication
  printf ( "Received blink from " );
  for ( a = 0; a < 16; a += 2 )
    printf ( "%02x%02x:", sender -> u8 [ a ], sender -> u8 [ a + 1 ] );
  printf ( " %i\r\n", * ( ( int* ) data ) );
}


/*****************************************************************************\
  This is the initialization hook which is called from the operating system
   during startup. Here we configure our device:
   - Register the receive handler for the UART
   - Open network sockets for data reception
   - Print out the MAC address of the device
\*****************************************************************************/
void ZWIR_AppInitNetwork ( ZWIR_ResetReason_t resetReason ) {

  ZWIR_SetTransmitPower ( 5 );

  // Register an handler for incoming UART packets.
  //  This will automatically enable the UART receiver.
  ZWIR_UART1_SetRXCallback ( SerialReceiveHandler );

  // Open two sockets listening to any address, at port 1000 & 1001
  // The unspecified address is used to enable reception of data from all
  //  remote devices
  ZWIR_OpenSocket ( &addrUnsp, 1000, 1000, DataCallback );
  ZWIR_OpenSocket ( &addrUnsp, 1001, 1001, BlinkCallback );

  // print the PAN address of this device
  ZWIR_PANAddress_t* panAddr = ZWIR_GetPANAddress ( );
  for ( int i = 0; i < 8; ++ i )
    printf ( "%02x",  ( *panAddr ) [ i ] );

}


/*****************************************************************************\
  This hook is called every 100 milliseconds. Here we switch off all LEDs.
   The result is, that LEDs are flashing in response to events.
\*****************************************************************************/
void  ZWIR_Main100ms ( void ) {
  ZWIR_SetLED1 ( 0 );
  ZWIR_SetLED2 ( 0 );
  ZWIR_SetLED3 ( 0 );
  ZWIR_SetLED4 ( 0 );
}


/*****************************************************************************\
  This hook is called every second. Here we send out a blink packet
   automatically every 5 seconds.
\*****************************************************************************/
void ZWIR_Main1000ms ( void ) {
  static char i = 0;

  // send a blink-packet all 5 seconds
  if ( 0 == ( ( i ++ ) % 5 ) ) {
    // send blink packet to the all-nodes multicast address, port 1001
    ZWIR_SendUDP2 ( &i, 4, &addrAll, 1001 );
    ZWIR_SetLED2 ( 1 );
    // emmie - se manda msg por UART
    printf("Se envió paquete blink %d\r\n", i);
  }
}

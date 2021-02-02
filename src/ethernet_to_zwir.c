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
 *  \file   ethernet_to_zwir.c
 *  \brief  Interface between Ethernet and 6LoWPAN
 * 
 ** ***************************************************************************/


#include <ZWIR45xx-6LoWPAN.h>
#include <ethernet_to_zwir.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define  MAC_ADDRESS_COUNT  255

extern void TransmitToEthernet(unsigned char *, unsigned short);

extern void SentToZWIR();
extern void DroppedToZWIR();
extern void SentToEthernet();
extern void DroppedToEthernet();


LinkAddress_t myEthernetAddress;
LinkAddress_t my6LoWPANAddress;


unsigned short  chksum ( unsigned short, unsigned char*, unsigned short );
unsigned short  htons  ( unsigned short );
unsigned char   StackOutput( unsigned char*  data, unsigned short  size, ZWIR_PANAddress_t  *address ); 


/** *************************************************************************
 *  Select one of the following statements - the first one disables the port
 *  filter, the second one configures the enabled ports.
 ** *************************************************************************/
uint16_t*  udpFilterAllowedPorts = NULL;
//uint16_t   udpFilterAllowedPorts [] = { 4,
                                        //1356,
                                        //1357,
                                        //20000,
                                        //0 };    // this zero MUST BE KEPT!
                                     

void
InitEthToZwir ( ) {
  ZWIR_CreateAlternativeAddressList (MAC_ADDRESS_COUNT );

  ZWIR_GatewaySetOutputFunction ( StackOutput );
  memcpy(my6LoWPANAddress, ZWIR_GetPANAddress(), 8);
  myEthernetAddress[0] = my6LoWPANAddress[0];
  myEthernetAddress[1] = my6LoWPANAddress[1];
  myEthernetAddress[2] = my6LoWPANAddress[2];
  myEthernetAddress[3] = my6LoWPANAddress[5];
  myEthernetAddress[4] = my6LoWPANAddress[6];
  myEthernetAddress[5] = my6LoWPANAddress[7];
  myEthernetAddress[6] = 0;
  myEthernetAddress[7] = 0;

}



unsigned short ComputeICMPv6Checksum(IPv6Packet_t *packet) {
  uint8_t i;

  packet-> payload . icmpv6Packet. chksum = 0;

  uint16_t chk = packet->nextHeader + packet->payloadLength;
  chk = chksum(chk, packet->srcAddress, 32);
  chk = chksum(chk, packet->payload . data, packet->payloadLength);
  chk = htons(~(chk &0xffff));
  return chk;
}



//-------------------------------------------------------------------------

/**
 *  This function is required as IEEE 802.3 and IEEE 802.15.4 have
 *   have different link-layer address types. IEEE 802.15.3 uses EUI48 and
 *   IEEE 802.15.4 uses EUI64 address types. These types have to be
 *   converted during packet forwarding from PAN to Ethernet and vice versa.
 *  For the conversion we assume that bits 3 and 4 of the EUI64 identifier
 *   in IEEE 802.15.4 addresses are 0. Otherwise the gateway will not
 *   operate correctly.
 */

unsigned char
HandleAddressResolution ( IPv6Packet_t*  ipPacket,
                          LinkType_t     destinationLink ) {

  if ( ipPacket->nextHeader != ipICMPv6 )
    return 1;



  uint16_t offset;

  switch (ipPacket-> payload . icmpv6Packet. type) {
  case icmpNeighborSolicitation:
    offset = 24;
    break;
  case icmpNeighborAdvertisement:
    offset = 24;
    break;
  case icmpRouterSolicitation:
    offset = 8;
    break;
  case icmpRouterAdvertisement:
    offset = 16;
    break;
  default:
    return 1;
  }

  ipPacket -> payloadLength = htons ( ipPacket -> payloadLength );

  uint16_t chkSum = ipPacket->payload . icmpv6Packet . chksum;
  if ( chkSum != ComputeICMPv6Checksum(ipPacket))
    return 0;

  // iterate over the ICMP option fields and find source and destination
  //  link-layer address information - 
  while ( offset < ipPacket->payloadLength ) {
    
    uint8_t *llOption = ipPacket -> payload . data + offset;    
    
    uint16_t optionLen = llOption [ 1 ] << 3;

    if ( *llOption == 1 || *llOption == 2 ) {
      LinkType_t link;
      if ( destinationLink == ltEthernet ) 
        link = lt6LoWPAN;
      else
        link = ltEthernet;
      
      if( llOption [ 1 ] == 2 ) {
        if ( link != lt6LoWPAN)
          return 0; // wrong packet
        ZWIR_AddAlternativeAddress( ( unsigned char (*) [8] ) (llOption + 2), ZWIR_aatEUI64 );
      
        llOption [ 1 ] = 1;
        memmove ( llOption + 5, llOption + 7, 
                  ipPacket -> payloadLength - offset - 7 );
      }
      else if( llOption [ 1 ] == 1 ){
        if ( link !=ltEthernet)
          return 0; // wrong packet

        ZWIR_PANAddress_t  addr64;
        memcpy ( addr64, llOption + 2, 3 );
        addr64 [ 3 ] = 0xff;
        addr64 [ 4 ] = 0xfe;
        memcpy ( addr64 + 5, llOption + 2 + 3, 3 );

        ZWIR_AddAlternativeAddress( ( unsigned char (*) [8] ) addr64, ZWIR_aatEUI48 );
        llOption [ 1 ] = 2;
        memmove ( llOption + 7, llOption + 5,
                  ipPacket -> payloadLength - offset - 5 );
        llOption [ 5 ] = 0xff;
        llOption [ 6 ] = 0xfe;
      }
      
    }
    uint16_t l = llOption [ 1 ] << 3;
    ipPacket -> payloadLength += l - optionLen;
    if(l)
       offset +=l ;
    else
       return 0;
   
  }

  chkSum = ComputeICMPv6Checksum(ipPacket);
  ipPacket->payload . icmpv6Packet . chksum =  chkSum;

  ipPacket->payloadLength = htons ( ipPacket -> payloadLength );

  return 1;
}


bool
FilterPacket ( IPv6Packet_t*  ipPacket ) {
  //if ( ( ipPacket -> nextHeader == ipUDP )
  //  && ( ( ipPacket -> payload . udpPacket . dstPort == htons ( 0xe76 ) )     // WS-Discovery
  //    || ( ipPacket -> payload . udpPacket . dstPort == htons ( 0x223 ) ) ) ) // DHCP
  //  return 0;

  if ( ! udpFilterAllowedPorts || ipPacket -> nextHeader != ipUDP )
    return 1;
  
  uint16_t*  filterPort = udpFilterAllowedPorts;
  while ( * filterPort ) {
    if ( ipPacket -> payload . udpPacket . srcPort == htons ( *filterPort ) ||
         ipPacket -> payload . udpPacket . dstPort == htons ( *filterPort ) )
      return 1;
    ++ filterPort;
  }
  return 0;
}

/*****************************************************************************
 * \brief  Forward the packets from Ethernet to PAN
 * 
 *  The forwarding procedure is as follows:
 *   1. Exit if we don't handle an IPv6 packet
 *   2. Check, if we find a matching link-layer address for the IPv6-address.
 *      If no address is found, we can't forward the packet.
 *   3. Check if the packet is for address resolution - If so, store IPv6 -
 *      link-layer address pairs in the internal data-base. Link-layer
 *      addresses are extracted from the link-layer address options of the
 *      neighbor-discovery-protocol packets. Replace the link-layer address
 *      with our own address. If packet is not for address resolution, simply
 *      forward without any changes.
 *****************************************************************************/
bool 
EtherntToZWIR ( MACPacket_t*    ethPacket,
                unsigned short  size,
                bool retry) {
  
  static LinkAddress_t address;

  if(!retry) {
    // We only need to handle IPv6 packages here
    if ( ! size || ( ethPacket -> type != IPV6_TYPE ) )
      return true;

    if ( ! FilterPacket ( & ethPacket -> payload . ipv6Packet ) )
      return true;

    if ( ! HandleAddressResolution ( &ethPacket -> payload . ipv6Packet, lt6LoWPAN ) )
      return true;

    ZWIR_GatewayPrepareSecondaryInput ( );
    if ( ! ZWIR_GatewayProcessPacket ( ethPacket -> payload . data,
                                       htons ( ethPacket -> payload . ipv6Packet . payloadLength ) + 40 ) ) {
      return true;
    }

    // Just forward packet which are addressed to a node
    //  in the PAN or a multicast destination:
    if ( ethPacket -> dstAddress [ 0 ] == 0x33 && ethPacket -> dstAddress [ 1 ] == 0x33 )
      memset ( address, 0xff, 8 ); // here we handle multicast destinations
    else {
      memcpy ( address, ethPacket -> dstAddress, 3 );
      memcpy ( address + 5, ethPacket -> dstAddress + 3, 3 );
      if ( !  ZWIR_IsAlternativeAddress ( ( unsigned char (*) [8] ) address, ZWIR_aatEUI64 ) ) {
      // if we don't find the address of the destination device, we drop the packet
      DroppedToZWIR ( );
      return true;
      }
    }
  }

  SentToZWIR();
  
  uint16_t ipLength = htons ( ethPacket -> payload . ipv6Packet . payloadLength ) + 40;

  if((ipLength < 40)||(ipLength>MTU))
    return true;

  return ZWIR_Send6LoWPAN ( (ZWIR_PANAddress_t*) address, 
                            ethPacket -> payload . data,
                            ipLength );
}



  
void
SendToEthernet ( IPv6Packet_t*   data, unsigned short  size, ZWIR_PANAddress_t *address) {
  MACPacket_t ethPacket;

  // Adapt link layer options to the needs of Ethernet
  if (!HandleAddressResolution ( data, ltEthernet ))
    return;

  if ((address==NULL) || ( *(uint16_t*)address == 0xffff) ) { 
    ethPacket . dstAddress [ 0 ] = ethPacket . dstAddress [ 1 ] = 0x33;
    memcpy ( ethPacket . dstAddress + 2, data -> dstAddress + 12, 4 );
  }
  else {
    ethPacket . dstAddress[0] = (*address)[0];
    ethPacket . dstAddress[1] = (*address)[1];
    ethPacket . dstAddress[2] = (*address)[2];
    ethPacket . dstAddress[3] = (*address)[5];
    ethPacket . dstAddress[4] = (*address)[6];
    ethPacket . dstAddress[5] = (*address)[7];    
  }


  // Complete the MAC header - populate source address and next header field.
  // The destination address is already set at this point.
  ethPacket . type = IPV6_TYPE;
  memcpy(ethPacket . srcAddress, myEthernetAddress, 6);


  // Copy the complete IP packet into the MAC payload
  memcpy(ethPacket . payload . data, data, size);

  SentToEthernet();
  
  TransmitToEthernet((uint8_t*) &ethPacket, size + 14); 
}

unsigned char
StackOutput( unsigned char*  data, unsigned short  size, ZWIR_PANAddress_t  *address ) {
  
  // check if broadcast
  if (address == NULL){
    ZWIR_Send6LoWPAN(address,data,size);
    SendToEthernet((IPv6Packet_t*)data,size,address);
    return true;
  }
  
  ZWIR_AlternativeAddressType_t type = ZWIR_IsAlternativeAddress(address,ZWIR_aatAny);
  if (type==ZWIR_aatEUI48) {
    SendToEthernet((IPv6Packet_t*)data,size,address);
  }
  else if (type==ZWIR_aatEUI64) {
    ZWIR_Send6LoWPAN(address,data,size);
  }
  
  return true;
}




/***********************************************************************/
/** \brief ZWIRToEthernet.
 *
 * Description: This function forward the packets from PAN TO
 *              Ethernet
 * 
 */
/**********************************************************************/

void
ZWIRToEthernet ( IPv6Packet_t*   data, unsigned short  size) {

  uint8_t buffer[MTU];

  if(size>MTU)
    return;  
  

  //buffer packet
  memcpy(buffer, data, size);

  if ( ! ZWIR_GatewayProcessPacket ( ( uint8_t* ) data, size ) ) {
    return;
  }

  SendToEthernet((IPv6Packet_t*)buffer,size,ZWIR_GetDestinationPANAddress());
}
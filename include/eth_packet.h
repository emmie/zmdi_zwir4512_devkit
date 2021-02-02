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
 *  \file   eth_packet.h
 *  \brief  
 * 
 ** ***************************************************************************/
 
#ifndef __ETH_PACKET__
#define __ETH_PACKET__

#include <stdint.h>
#include "ZWIR45xx-6LoWPAN.h"

#define MTU 1280

typedef enum __attribute__ (( __packed__ )) { 
               icmpEchoRequest = 128,
               icmpEchoReply = 129,
               icmpRouterSolicitation = 133,
               icmpRouterAdvertisement = 134,
               icmpNeighborSolicitation = 135,
               icmpNeighborAdvertisement = 136,
               icmpRedirect = 137 }   ICMPv6Type_t;

typedef enum __attribute__ (( __packed__ )) { ipUDP = 17, ipICMPv6 = 58 } IPNextHeader_t;

typedef struct __attribute__ (( __packed__ )) {
  uint16_t  srcPort;
  uint16_t  dstPort;
  uint16_t  length;
  uint16_t  chksum;
} UDPPacket_t;

typedef struct __attribute__ (( __packed__ )) {
  uint32_t  ignore_ShouldBeZero;
  uint8_t   targetAddress [ 16 ];
  uint8_t   sourceLLAddressOption [ 8 ];
} ICMPv6ns_t;


typedef struct __attribute__ (( __packed__ )) {
  uint8_t   flags;
  uint8_t   reserved_ShouldBeZeros [ 3 ];
  uint8_t   targetAddress [ 16 ];
  uint8_t   targetLLAddressOption [ 8 ];
} ICMPv6na_t;


typedef struct __attribute__ (( __packed__ )) {
  uint32_t  ignore_ShouldBeZero;
  uint8_t   sourceLLAddressOption [ 8 ];
} ICMPv6rs_t;


typedef struct __attribute__ (( __packed__ )) {
  uint8_t   hopLimit;
  uint8_t   flags;
  uint16_t  routerLifetime;
  uint32_t  reachabilityTimeout;
  uint32_t  resolveTimeout;
  uint8_t   targetLLAddressOption [ 8 ];
} ICMPv6ra_t;



typedef struct __attribute__ (( __packed__ )) {
  ICMPv6Type_t  type;
  uint8_t       code;
  uint16_t      chksum;
  union {
    ICMPv6ns_t  neighborSolicitation;
    ICMPv6na_t  neighborAdvertisement;
    ICMPv6rs_t  routerSolicitation;
    ICMPv6ra_t  routerAdvertisement;
  } payload;
} ICMP6Packet_t;

typedef struct __attribute__ (( __packed__ )) {
  uint8_t       typeTraffic;
  uint8_t       trafficFlow;
  uint16_t      flowLabel;
  uint16_t      payloadLength;
  IPNextHeader_t  nextHeader;
  uint8_t       hopLimit;
  uint8_t       srcAddress [ 16 ];
  uint8_t       dstAddress [ 16 ];
  union {
    uint8_t        data [ MTU - 40 ];
    UDPPacket_t    udpPacket;
    ICMP6Packet_t  icmpv6Packet;
  } payload;
} IPv6Packet_t;

typedef struct __attribute__ (( __packed__ )) {
  uint8_t   dstAddress [ 6 ];
  uint8_t   srcAddress [ 6 ];
  uint16_t  type;
  union {
    uint8_t       data [ MTU ];
    IPv6Packet_t  ipv6Packet;
  } payload;
  uint32_t  fcs;
} MACPacket_t;


typedef ZWIR_IPv6Address_t    IPv6Address_t;
typedef uint8_t               LinkAddress_t [ 8 ];

typedef union __attribute__ (( __packed__ )) {
  LinkAddress_t  link;
  IPv6Address_t  ipv6;
} Address_t;



typedef struct __attribute__ (( __packed__ ))  MAC_ADDR
{
    unsigned char v[6];
} MAC_ADDR;



#endif



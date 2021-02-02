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
 *  \file   ethernet_to_zwir.h
 *  \brief  
 * 
 ** ***************************************************************************/

 
 #ifndef __ETHERNET_TO_ZWIR_H__
  #define __ETHERNET_TO_ZWIR_H__

  #include <stdint.h>
  #include <stdbool.h>
  #include "ZWIR45xx-6LoWPAN.h"
  #include <eth_packet.h>
  
  #define MAX_NODE_COUNT  128
  
  
  #define IPV6_TYPE             0xdd86
  #define IMCPV6_PACKET_BEGIN   40
  #define ETHDIR                0
  #define ZWIRDIR               1
  #define MACDEST_START         0
  #define MACSRC_START          8
  #define IPDEST_START          24
  #define IPSRC_START           8

  #define ICMPV6CHKSUM0         42
  #define ICMPV6CHKSUM1         43
  #define LLADDRESSLENGTH       65

  #define IPV6PAYLOAD0          4
  #define IPV6PAYLOAD1          5

  // Multicast Listener --> not implemented
  #define ICMP6_MCLv2           143 
  #define TO_ENC                0
  #define TO_ZWIR               1

  #define IN                0    // means from ENC TO ZWIR
  #define OUT               1    // means from ZWIR TO ENC

  #define SIZEOF_IPADRESSES     30
  #define MAXFILTERADDRESS 30
  #define ICMP6_RS                        133  /**< Router Solicitation */
  #define ICMP6_RA                        134  /**< Router Advertisement */
  #define ICMP6_NS                        135  /**< Neighbor Solicitation */
  #define ICMP6_NA                        136  /**< Neighbor advertisement */
  #define ICMP6_REDIRECT                  137  /**< Redirect */

  

  typedef enum __attribute__ (( __packed__ ))  {
    ltEthernet, lt6LoWPAN
  } LinkType_t;


  typedef struct __attribute__ (( __packed__ ))  {
    IPv6Address_t ip;
    LinkAddress_t link;
    int16_t next: 14;
    LinkType_t type: 2;
  } NodeAddresses_t;

  typedef struct __attribute__ (( __packed__ ))  {
    unsigned char address[16];
    unsigned char LLaddress_to[8];
    unsigned char LLaddress_from[8];
  } tipMacPair;

  typedef struct __attribute__ (( __packed__ ))  {
    unsigned char address[16];
    unsigned char LLaddress_to[6];
    unsigned char LLaddress_from[6];
  } tipMacPair_ND;

  typedef struct __attribute__ (( __packed__ ))  {
    unsigned char Filter_MAC[6];
  } Filter_MAC;

  void InitEthToZwir();
  bool EtherntToZWIR(MACPacket_t *packet, unsigned short size, bool retry);
  void ZWIRToEthernet(IPv6Packet_t *data, unsigned short size);  
  
#endif

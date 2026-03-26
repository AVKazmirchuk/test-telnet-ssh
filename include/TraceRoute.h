//
// Created by Alexander on 26.03.2026.
//

#include <winsock2.h>
#include <ws2tcpip.h>

//TraceRoute.h
#ifndef TRACEROUTE_H_INCLUDED
#define TRACEROUTE_H_INCLUDED

#pragma pack(1)
//IP data header
typedef struct
{
    unsigned char hdr_len :4; // 4-bit header length
    unsigned char version :4; // 4-bit version
    unsigned char tos; // 8-bit service type
    unsigned short total_len; // 16-bit total length (number of bytes)
    unsigned short identifier; // 16-bit identifier
    unsigned short frag_and_flags; // 3-bit flag 13-bit chip offset
    unsigned char ttl; // 8-bit survival time
    unsigned char protocol; // 8-bit protocol (TCP, UDP etc)
    unsigned short checksum; // 16-bit header checksum
    unsigned long sourceIP; // 32-bit original address
    unsigned long destIP; // 32-bit destination IP address
} IP_HEADER;
//ICMP data header
typedef struct
{
    BYTE type; //8-bit type
    BYTE code; //8-bit code
    USHORT cksum; //16-bit checksum
    USHORT id; //16-bit identifier
    USHORT seq; //16-bit serial number
} ICMP_HEADER;
//Decoding result
typedef struct
{
    USHORT usSeqNo; //Package serial number
    DWORD dwRoundTripTime; //Round trip time
    in_addr dwIPaddr; //Peer IP address
} DECODE_RESULT;
#pragma pack()
//ICMP type field
const BYTE ICMP_ECHO_REPLY = 0; //Echo response
const BYTE ICMP_ECHO_REQUEST = 8; //Request echo
const BYTE ICMP_TIMEOUT = 11; //Transmission timeout
const DWORD DEF_ICMP_TIMEOUT = 3000; //default timeout time, unit ms
const int DEF_ICMP_DATA_SIZE = 32; //default ICMP data part length
const int MAX_ICMP_PACKET_SIZE = 1024; //The size of the largest ICMP datagram
const int DEF_MAX_HOP = 30; //Maximum number of hops
USHORT GenerateChecksum(USHORT* pBuf, int iSize);
BOOL DecodeIcmpResponse(char* pBuf, int iPacketSize, DECODE_RESULT& stDecodeResult);



#endif // TRACEROUTE_H_INCLUDED
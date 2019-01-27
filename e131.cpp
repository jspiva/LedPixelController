/*
* E131.cpp
*
* Project: E131 - E.131 (sACN) library for Arduino
* Copyright (c) 2015 Shelby Merrick
* http://www.forkineye.com
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/

#include "E131.h"
#include <string.h>

/* E1.17 ACN Packet Identifier */
const byte E131::ACN_ID[12] = { 0x41, 0x53, 0x43, 0x2d, 0x45, 0x31, 0x2e, 0x31, 0x37, 0x00, 0x00, 0x00 };

/* Constructor */
E131::E131() {
  
    // Setup buffer
    memset(packet.raw, 0, sizeof(packet.raw));

    // Zero the stats
    stats.num_packets = 0;
    stats.sequence_errors = 0;
    stats.packet_errors = 0;
}

void E131::begin() {
    udp.begin(E131_DEFAULT_PORT);

    Serial.print(F("E1.31 Unicast port: "));
    Serial.println(E131_DEFAULT_PORT);
}

void E131::dumpError(e131_error_t error) {
    switch (error) {
        case ERROR_ACN_ID:
            Serial.print(F("INVALID PACKET ID: "));
            for (int i = 0; i < sizeof(ACN_ID); i++)
                Serial.print(packet.acn_id[i], HEX);
            Serial.println("");
            break;
        case ERROR_PACKET_SIZE:
            Serial.println(F("INVALID PACKET SIZE: "));
            break;
        case ERROR_VECTOR_ROOT:
            Serial.print(F("INVALID ROOT VECTOR: 0x"));
            Serial.println(htonl(packet.root_vector), HEX);
            break;
        case ERROR_VECTOR_FRAME:
            Serial.print(F("INVALID FRAME VECTOR: 0x"));
            Serial.println(htonl(packet.frame_vector), HEX);
            break;
        case ERROR_VECTOR_DMP:
            Serial.print(F("INVALID DMP VECTOR: 0x"));
            Serial.println(packet.dmp_vector, HEX);
            break;
    }
}

/**
 * Projekt IPK3
 * Autor: David Krym
 * Datum: 22.4.2011
 */

#ifndef RDTSERVER_H
#define RDTSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "udt.h"
#include "list.h"
#include <stdint.h>


#define PROGRAM_INFO "RDT Server\n\n"
#define PROGRAM "rtdserver"
#define MAXCHARS 80


/////fce
char *createPacket(int num, char *text);
uint32_t fletcher32( char *data, size_t len );
int packetCheck(char *pkt, int len);
int makeConn(void);

#endif

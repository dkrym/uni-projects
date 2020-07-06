/**
 * Projekt IPK3
 * Autor: David Krym
 * Datum: 22.4.2011
 */

#ifndef RDTCLIENT_H
#define RDTCLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "udt.h"
#include <stdint.h>
#include <sys/time.h>
#include <signal.h>


#define PROGRAM_INFO "RDT Client\n\n"
#define PROGRAM "rtdclient"
#define MAXCHARS 80
#define SWINDOWSIZE 10
#define TIMER 1 // second


/////fce
char *createPacket(int num, char *text);
uint32_t fletcher32(char *data, size_t len);
int packetCheck(char *pkt, int len);
int makeConn(void);
void sigalrm_handler(int sig);
void handshake(void);
void endshake(void);

#endif

/*
 * File:   ndpv6_printer.h
 * Author: David Krym
 */

#ifndef NDPV6_PRINTER_H
#define	NDPV6_PRINTER_H

#include <net/ethernet.h> // struct ether_addr
#include <netinet/in.h>   // struct in6_addr
#include "ndpv6_if.h" // struct ifs


#define OUTPUT_NA "NA"
#define OUTPUT_NS "NS"
#define OUTPUT_PTB "PTB"
#define OUTPUT_IN "IN "
#define OUTPUT_OUT "OUT"


/** Struktura pro predani udaju pro vypis. */
struct output
{
    char *start_str;
    struct timeval *ts;
    struct ether_addr *mac_src;
    struct ether_addr *mac_dest;
    struct in6_addr *ip_src;
    struct in6_addr *ip_dest;
    char *type_str;
    struct in6_addr *ip_target;
    struct ether_addr *mac_slla;
};


/**
 * Vytiskne radek na vystup. Tiskne zpravy "IN: ..." a "OUT: ...".
 * Format:
 * IN/OUT: time, srcmac > dstmac, ipv6src > ipv6dst
 * IN/OUT: time, srcmac > dstmac, ipv6src > ipv6dst, ICMPv6[NS|NA] target, slla
 * @param p Ukazatel na strukturu s informacema pro tisk.
 * @return void.
 */
void print_out_ln(struct output *p);


/**
 * Vytiskne informace o rozhranich, na kterych se bude naslouchat.
 * @param ifs_prms Ukazatel na obe lokalni rozhrani.
 * @param if_in Nazev rozhrani in.
 * @param if_out Nazev rozhrani out.
 * @return void.
 */
void print_ifaces(char *if_in, char *if_out, struct ifs *ifs_prms);


#endif	/* NDPV6_PRINTER_H */


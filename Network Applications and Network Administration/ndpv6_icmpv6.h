/*
 * File:   ndpv6_icmpv6.h
 * Author: David Krym
 */

#ifndef NDPV6_ICMPV6_H
#define	NDPV6_ICMPV6_H

#include "ndpv6_if.h" // struct {ifs, in6_addr, pcap_pkthdr}


/** Struktura s hlavickou pro ICMPv6 Packet Too Big. */
struct icmp6_ptb
{
    uint8_t type;
    uint8_t code;
    uint16_t cksum;
    uint32_t mtu;
    /* Payload data */
};


/**
 * Vypocita ICMPv6 checksum.
 * Pocitano dle pseudokodu z RFC1071, sekce 4.1.
 * @param src Adresa IPv6 zdroje.
 * @param dest Adresa IPv6 cile.
 * @param data Ukazatel na payload.
 * @param len Velikost payload dat.
 * @return Vraci 16b checksum v network order.
 */
uint16_t icmpv6_cksum( struct in6_addr *src,
                       struct in6_addr *dest,
                       void *data, size_t len );


/**
 * Sestavi paket ICMPv6 Packet Too Big a odesle ho.
 * @param ifs_prms Ukazatel na strukturu s lokalnima rozhranima.
 * @param header Ukazatel na hlavicku od pcapu.
 * @param data Ukazatel na payload.
 * @return Vraci 0 kdyz vporadku, -1 pri chybe.
 */
int icmpv6_send_ptb( struct ifs *ifs_prms,
                     const struct pcap_pkthdr *header,
                     const u_char *packet );


#endif	/* NDPV6_ICMPV6_H */


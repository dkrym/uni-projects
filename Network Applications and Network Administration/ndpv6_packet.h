/*
 * File:   ndpv6_packet.h
 * Author: David Krym
 */

#ifndef NDPV6_PACKET_H
#define	NDPV6_PACKET_H

#include <pcap.h>
#include "ndpv6_if.h"
#include "ndpv6_printer.h"

/**
 * Vstupni bod po prijeti jakehokoliv IPv6 paketu.
 * Pokud je prichozi paket urcen lokalne, je zahozen (kernel ho sam zpracuje).
 * Pokud ne, je podle typu predan dalsi funkci na zpracovani.
 * @param arg Ukazatel na obe lokalni rozhrani.
 * @param header Ukazatel na hlavicku od pcapu.
 * @param packet Ukazatel na samotny packet.
 * @return void.
 */
void packet_rcvd( u_char *arg,
                  const struct pcap_pkthdr *header,
                  const u_char *packet );


/**
 * Byl prijat paket, ktery neni NA nebo NS.
 * Zjisti se, zda je paket unicast, nebo multicast - dle toho se zavola funkce.
 * @param ifs_prms Ukazatel na obe lokalni rozhrani.
 * @param header Ukazatel na hlavicku od pcapu.
 * @param packet Ukazatel na samotny packet.
 * @return void.
 */
void packet_ipv6( struct ifs *ifs_prms,
                  const struct pcap_pkthdr *header,
                  const u_char *packet );


/**
 * Byl prijat paket, ktery neni NA nebo NS a je multicast.
 * Zmeni se jen zdrojova MAC adresa a paket se preposle.
 * @param out Ukazatel na strukturu s informace pro vypis.
 * @param ifs_prms Ukazatel na obe lokalni rozhrani.
 * @param header Ukazatel na hlavicku od pcapu.
 * @param packet Ukazatel na samotny packet.
 * @return void.
 */
void packet_ipv6_multic( struct output *out,
                         struct ifs *ifs_prms,
                         const struct pcap_pkthdr *header,
                         const u_char *packet );


/**
 * Byl prijat paket, ktery neni NA nebo NS a je unicast.
 * Zmeni se jen zdrojova MAC adresa a paket se preposle.
 * @param out Ukazatel na strukturu s informace pro vypis.
 * @param ifs_prms Ukazatel na obe lokalni rozhrani.
 * @param header Ukazatel na hlavicku od pcapu.
 * @param packet Ukazatel na samotny packet.
 * @return void.
 */
void packet_ipv6_unic( struct output *out,
                       struct ifs *ifs_prms,
                       const struct pcap_pkthdr *header,
                       const u_char *packet );


/**
 * Byl prijat paket, ktery je NA nebo NS.
 * @param ifs_prms Ukazatel na obe lokalni rozhrani.
 * @param header Ukazatel na hlavicku od pcapu.
 * @param packet Ukazatel na samotny packet.
 * @return void.
 */
void packet_NA_or_NS( struct ifs *ifs_prms,
                      const struct pcap_pkthdr *header,
                      const u_char *packet );


/**
 * Byl prijat paket, ktery je NA.
 * Upravi se ethernetova hlavicka a pripadne i MAC adresa v payloadu.
 * @param out Ukazatel na strukturu s informace pro vypis.
 * @param ifs_prms Ukazatel na obe lokalni rozhrani.
 * @param header Ukazatel na hlavicku od pcapu.
 * @param packet Ukazatel na samotny packet.
 * @return void.
 */
void packet_ND_NA( struct output *out,
                   struct ifs *ifs_prms,
                   const struct pcap_pkthdr *header,
                   const u_char *packet );


/**
 * Byl prijat paket, ktery je NS.
 * Upravi se ethernetova hlavicka a pripadne i MAC adresa v payloadu.
 * @param out Ukazatel na strukturu s informace pro vypis.
 * @param ifs_prms Ukazatel na obe lokalni rozhrani.
 * @param header Ukazatel na hlavicku od pcapu.
 * @param packet Ukazatel na samotny packet.
 * @return void.
 */
void packet_ND_NS( struct output *out,
                   struct ifs *ifs_prms,
                   const struct pcap_pkthdr *header,
                   const u_char *packet );


#endif	/* NDPV6_PACKET_H */


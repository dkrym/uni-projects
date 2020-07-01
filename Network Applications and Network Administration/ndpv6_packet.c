/*
 * File:   ndpv6_packet.c
 * Author: David Krym
 */

#include <stdio.h>
#include <stdlib.h>
#include <netinet/ip6.h> // struct ip6_hdr
#include <netinet/icmp6.h> // struct icmp6
#include <net/ethernet.h> // struct ether_header
#include "ndpv6_ipv6.h" // is_ipv6_local()
#include "ndpv6_icmpv6.h" // icmpv6_cksum()
#include "ndpv6_packet.h"
#include "ndpv6_if.h"

#define MIN_ND_LEN 78 // minimalni delka ND paketu
#define ND_LEN_WITH_OPT 86 // delka i s options
#define NA_NS_HDR_LEN 32 // velikost NA/NS hlavicky i s TLLA/SLLA


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
                  const u_char *packet )
{
    if (header->caplen > header->len)
        return; // poskozeny paket

    struct ifs *ifs_prms = (struct ifs *) arg; // parametry rozhranich
    struct ether_header *ehdr = (struct ether_header *) packet; // eth header
    struct ip6_hdr *ipv6hdr = NULL; // ipv6 header
    struct icmp6_hdr *icmpv6hdr = NULL; // icmpv6 header

    if (ntohs(ehdr->ether_type) == ETHERTYPE_IPV6)
    { // ethernet typ je IPv6
        ipv6hdr = (struct ip6_hdr *) (packet + sizeof (struct ether_header));

        // (multicast a nevysilam ho ja) || (unicast ktery neni pro me)
        if ( ( IN6_IS_ADDR_MULTICAST(&(ipv6hdr->ip6_dst))
               && (!is_ipv6_local(ifs_prms, &(ipv6hdr->ip6_src))) )
             || (!is_ipv6_local(ifs_prms, &(ipv6hdr->ip6_dst))) )
        { // neni urcena lokalne, nebo je to multicast
            // z prichoziho paketu zkusime ulozit MAC a IPv6
            NC_update( ifs_prms->src->nc_tbl,
                       (struct ether_addr *) &(ehdr->ether_shost),
                       &(ipv6hdr->ip6_src) );

            if (ipv6hdr->ip6_nxt == IPPROTO_ICMPV6)
            { // nasleduje icmpv6 hlavicka
                icmpv6hdr = (struct icmp6_hdr *)
                            ( packet + sizeof(struct ether_header)
                              + sizeof(struct ip6_hdr) );

                if ( (icmpv6hdr->icmp6_type == ND_NEIGHBOR_ADVERT)
                     || (icmpv6hdr->icmp6_type == ND_NEIGHBOR_SOLICIT) )
                { // NA nebo NS
                    packet_NA_or_NS(ifs_prms, header, packet);
                }
                else
                { // neco jineho
                    packet_ipv6(ifs_prms, header, packet);
                }
            }
            else
            { // cokoliv jineho nez icmpv6
                packet_ipv6(ifs_prms, header, packet);
            }
        }
    }

    return;
}


/**
 * Byl prijat paket, ktery neni NA nebo NS. Vypise, ze paket prisel.
 * Zjisti se, zda je paket unicast, nebo multicast - dle toho se zavola funkce.
 * @param ifs_prms Ukazatel na obe lokalni rozhrani.
 * @param header Ukazatel na hlavicku od pcapu.
 * @param packet Ukazatel na samotny packet.
 * @return void.
 */
void packet_ipv6( struct ifs *ifs_prms,
                  const struct pcap_pkthdr *header,
                  const u_char *packet )
{
   //printf("in ipv6 fnc\n");

    struct ether_header *ehdr = (struct ether_header *) packet; // eth header
    struct ip6_hdr *ipv6hdr = (struct ip6_hdr *)
                              (packet + sizeof (struct ether_header));
    struct output out; // info pro vypis

    // naplneni informacemi pro vypis
    out.start_str = OUTPUT_IN;
    out.type_str = NULL;
    out.ip_dest = &(ipv6hdr->ip6_dst);
    out.ip_src = &(ipv6hdr->ip6_src);
    out.mac_dest = (struct ether_addr *) &(ehdr->ether_dhost);
    out.mac_src = (struct ether_addr *) &(ehdr->ether_shost);
    out.ts = (struct timeval *) &(header->ts);
    print_out_ln(&out); // vytiskne radek IN

    if (ifs_prms->dest->mtu < header->len)
    { // rozhrani, na ktere ma jit paket ma mensi MTU nez je delka paketu
       icmpv6_send_ptb(ifs_prms, header, packet);
       return;
    }

    // multicast nebo unicast
    if (IN6_IS_ADDR_MULTICAST(&(ipv6hdr->ip6_dst)))
        packet_ipv6_multic(&out, ifs_prms, header, packet);
    else
        packet_ipv6_unic(&out, ifs_prms, header, packet);

    return;
}


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
                         const u_char *packet )
{
    struct ether_header *ehdr = (struct ether_header *) packet; // eth header

    // zmena MAC (dest,src) v ethernetovy hlavicce
    copy_mac( (struct ether_addr *) &(ehdr->ether_shost),
              &(ifs_prms->dest->mac) );

    out->start_str = OUTPUT_OUT;
    print_out_ln(out); // vytiskne radek OUT

    // odesle upraveny paket na druhe rozhrani nez prisel
    pcap_inject(ifs_prms->dest->pcap_ptr, packet, header->len);

    return;
}


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
                       const u_char *packet )
{
    struct ether_header *ehdr = (struct ether_header *) packet; // eth header
    struct ip6_hdr *ipv6hdr = (struct ip6_hdr *)
                              (packet + sizeof (struct ether_header));
    struct ether_addr *dest_eaddr = NULL;

    if ( (dest_eaddr = NC_get_mac( ifs_prms->dest->nc_tbl,
                                   &(ipv6hdr->ip6_dst)) ) != NULL )
    { // v NC tabulce byl nalezen zaznam, zmeni se MAC prijemce
        // kopirovani MAC (dest,src)
        copy_mac((struct ether_addr *) &(ehdr->ether_dhost), dest_eaddr);
        copy_mac( (struct ether_addr *) &(ehdr->ether_shost),
                  &(ifs_prms->dest->mac) );

        out->start_str = OUTPUT_OUT;
        print_out_ln(out); // vytiskne radek OUT

        // odesle upraveny paket na druhe rozhrani nez prisel
        pcap_inject(ifs_prms->dest->pcap_ptr, packet, header->len);
    }

    return;
}


/**
 * Byl prijat paket, ktery je NA nebo NS.
 * @param ifs_prms Ukazatel na obe lokalni rozhrani.
 * @param header Ukazatel na hlavicku od pcapu.
 * @param packet Ukazatel na samotny packet.
 * @return void.
 */
void packet_NA_or_NS( struct ifs *ifs_prms,
                      const struct pcap_pkthdr *header,
                      const u_char *packet )
{
    if (header->len < MIN_ND_LEN)
        return; // chyba, nema minimalne velikost

    struct ether_header *ehdr = (struct ether_header *) packet; // eth header
    struct ip6_hdr *ipv6hdr = (struct ip6_hdr *)
                              (packet + sizeof(struct ether_header));
    struct icmp6_hdr *icmpv6hdr = (struct icmp6_hdr *)
                                  ( packet + sizeof(struct ether_header)
                                  + sizeof(struct ip6_hdr) );
    struct output out;

    // naplneni informacemi pro vypis
    out.start_str = OUTPUT_IN;
    out.ip_dest = &(ipv6hdr->ip6_dst);
    out.ip_src = &(ipv6hdr->ip6_src);
    out.mac_dest = (struct ether_addr *) &(ehdr->ether_dhost);
    out.mac_src = (struct ether_addr *) &(ehdr->ether_shost);
    out.ts = (struct timeval *) &(header->ts);

    if (ifs_prms->dest->mtu < header->len)
    { // rozhrani, na ktere ma jit paket ma mensi MTU nez je delka paketu
       icmpv6_send_ptb(ifs_prms, header, packet);
       return;
    }

    if  (icmpv6hdr->icmp6_type == ND_NEIGHBOR_ADVERT)
    { // NA
        packet_ND_NA(&out, ifs_prms, header, packet);
    }
    else if (icmpv6hdr->icmp6_type == ND_NEIGHBOR_SOLICIT)
    { // NS
        packet_ND_NS(&out, ifs_prms, header, packet);
    }

    return;
}


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
                   const u_char *packet )
{
    //printf("in NA fnc\n");

    struct ether_header *ehdr = (struct ether_header *) packet; // eth header
    struct ip6_hdr *ipv6hdr = (struct ip6_hdr *)
                              (packet + sizeof(struct ether_header));
    struct nd_neighbor_advert *na6hdr = (struct nd_neighbor_advert *)
                                        (packet + sizeof(struct ether_header)
                                        + sizeof(struct ip6_hdr));
    struct ether_addr *dest_eaddr = NULL;
    struct ether_addr *tlla_eaddr = NULL; // SLLA/TLLA mac adresa

    if (header->len >= ND_LEN_WITH_OPT)
    { // paket ma SLLA/TLLA
        tlla_eaddr = (struct ether_addr *)
                     (packet + MIN_ND_LEN + sizeof(struct nd_opt_hdr));

        // ulozime MAC adresu z TLLA do NC tabulky - jen pokud je jina
        NC_update(ifs_prms->src->nc_tbl, tlla_eaddr, &(ipv6hdr->ip6_src));
    }

    out->type_str = OUTPUT_NA;
    out->ip_target = &(na6hdr->nd_na_target);
    out->mac_slla = (tlla_eaddr == NULL) ? NULL : tlla_eaddr;
    print_out_ln(out); // vytiskne radek IN

    if (IS_MAC_MULTICAST((struct ether_addr *) &(ehdr->ether_dhost)))
    { /* multicast, cilova MAC se nemeni */ }
    else if ( (dest_eaddr = NC_get_mac( ifs_prms->dest->nc_tbl,
                                        &(ipv6hdr->ip6_dst) )) != NULL )
    { // unicast, ipv6 adresa nalezena v NC, zmena cilove MAC
        copy_mac((struct ether_addr *) &(ehdr->ether_dhost), dest_eaddr);
    }
    else
        return; // unicast, nebyl nalezen zaznam v NC tabulce

    // MAC (dest,src)
    copy_mac( (struct ether_addr *) &(ehdr->ether_shost),
              &(ifs_prms->dest->mac) );

    if (tlla_eaddr != NULL) 
    { // uprava TLLA MAC adresy, novy checksum
        copy_mac(tlla_eaddr, &(ifs_prms->dest->mac)); // MAC (dest,src)
        na6hdr->nd_na_hdr.icmp6_cksum = 0;
        uint16_t cksum = icmpv6_cksum( &(ipv6hdr->ip6_src), &(ipv6hdr->ip6_dst),
                                       (void *)na6hdr, NA_NS_HDR_LEN );
        na6hdr->nd_na_hdr.icmp6_cksum = cksum;
    }

    out->start_str = OUTPUT_OUT;
    print_out_ln(out); // vytiskne radek OUT

    // odesle upraveny paket na druhe rozhrani nez prisel
    pcap_inject(ifs_prms->dest->pcap_ptr, packet, header->len);

    return;
}


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
                   const u_char *packet )
{
    //printf("in NS fnc\n");

    struct ether_header *ehdr = (struct ether_header *) packet; // eth header
    struct ip6_hdr *ipv6hdr = (struct ip6_hdr *)
                              (packet + sizeof (struct ether_header));
    struct nd_neighbor_solicit *ns6hdr = (struct nd_neighbor_solicit *)
                                         (packet + sizeof(struct ether_header)
                                         + sizeof(struct ip6_hdr));
    struct ether_addr *dest_eaddr = NULL;
    struct ether_addr *slla_eaddr = NULL; // SLLA/TLLA mac adresa

    if (header->len >= ND_LEN_WITH_OPT)
    { // paket ma SLLA/TLLA
        slla_eaddr = (struct ether_addr *)
                     (packet + MIN_ND_LEN + sizeof(struct nd_opt_hdr));

        // ulozime MAC adresu do NC tabulky - jen pokud je jina nez ulozena
        NC_update(ifs_prms->src->nc_tbl, slla_eaddr, &(ipv6hdr->ip6_src));
    }

    out->type_str = OUTPUT_NS;
    out->ip_target = &(ns6hdr->nd_ns_target);
    out->mac_slla = (slla_eaddr == NULL) ? NULL : slla_eaddr;
    print_out_ln(out); // vytiskne radek IN

    if (IS_MAC_MULTICAST((struct ether_addr *) &(ehdr->ether_dhost)))
    { /* multicast, cilova mac se nemeni */ }
    else if ( (dest_eaddr = NC_get_mac( ifs_prms->dest->nc_tbl,
                                        &(ipv6hdr->ip6_dst) )) != NULL )
    { // unicast, ipv6 adresa nalezena v NC
        copy_mac((struct ether_addr *) &(ehdr->ether_dhost), dest_eaddr);
    }
    else
        return; // unicast, nebyl nalezen zaznam v NC tabulce

    // paket je multicast, nebo unicast se zaznamem v NC tabulce
    copy_mac( (struct ether_addr *) &(ehdr->ether_shost),
              &(ifs_prms->dest->mac) );

    if (slla_eaddr != NULL)
    {  // uprava SLLA mac adresy
        copy_mac(slla_eaddr, &(ifs_prms->dest->mac));
        ns6hdr->nd_ns_hdr.icmp6_cksum = 0;
        uint16_t cksum = icmpv6_cksum( &(ipv6hdr->ip6_src), &(ipv6hdr->ip6_dst),
                                       (void *)ns6hdr, NA_NS_HDR_LEN );
        ns6hdr->nd_ns_hdr.icmp6_cksum = cksum;
    }

    out->start_str = OUTPUT_OUT;
    print_out_ln(out); // vytiskne radek OUT

    // odesle upraveny paket na druhe rozhrani nez prisel
    pcap_inject(ifs_prms->dest->pcap_ptr, packet, header->len);

    return;
}



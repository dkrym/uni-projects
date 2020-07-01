/*
 * File:   ndpv6_icmpv6.c
 * Author: David Krym
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset, memcpy
#include <netinet/ip6.h> // struct ip6_hdr
#include "ndpv6_icmpv6.h"
#include "ndpv6_printer.h"


#define MIN_MTU 1280 // minimalni MTU pro rozhrani
// maximalni delka payloadu bez hlavicek
#define PTB_MAX_PL_LEN 1232 // min_mtu-ipv6_header_len-icmp6_ptb_len
// delka hlavicek
#define PTB_HDRS_LEN 62 // ether_hdr+ipv6_hdr+icmp6_ptb_hdr
#define PTB_ICMP6_TYPE 2 // type v icmp6 hlavicce
#define IPV6_ADDR16_CNT 8 // kolik ma ipv6 2B


/**
 * Sestavi paket ICMPv6 Packet Too Big a odesle ho.
 * @param ifs_prms Ukazatel na strukturu s lokalnima rozhranima.
 * @param header Ukazatel na hlavicku od pcapu.
 * @param data Ukazatel na payload.
 * @return Vraci 0 kdyz vporadku, -1 pri chybe.
 */
int icmpv6_send_ptb( struct ifs *ifs_prms,
                     const struct pcap_pkthdr *header,
                     const u_char *packet )
{
    u_char *ptb; // pro alokovane misto na paket
    int i; // pocitadlo
    struct ether_header *ehdr, *pkt_ehdr;
    struct ip6_hdr *ipv6hdr, *pkt_ipv6hdr;
    struct icmp6_ptb *icmp6;
    struct output out; // vystup

    // velikost paketu bez ether_hdr -> velikost payloadu icmp6 Packet Too Big
    int pl_size = header->len - ETHER_HDR_LEN;
    // velikost celyho paketu musi byt max 1280, pripadne zkratime payload
    int payload_len = (pl_size > PTB_MAX_PL_LEN) ? PTB_MAX_PL_LEN : pl_size;

    // alokovat velikost pro cely paket
    if ((ptb = (u_char *) malloc(PTB_HDRS_LEN + payload_len)) == NULL)
        return -1;

    // ETHERNET hlavicka
    ehdr = (struct ether_header *) ptb;
    pkt_ehdr = (struct ether_header *) packet;

    copy_mac((struct ether_addr *) &(ehdr->ether_shost), &(ifs_prms->src->mac));
    copy_mac( (struct ether_addr *) &(ehdr->ether_dhost),
              (struct ether_addr *) &(pkt_ehdr->ether_shost) ); // (dest,src)
    ehdr->ether_type = htons(ETHERTYPE_IPV6);

    // IPV6 hlavicka
    ipv6hdr = (struct ip6_hdr *) (ptb + ETHER_HDR_LEN);
    pkt_ipv6hdr = (struct ip6_hdr *) (packet + ETHER_HDR_LEN);
    memset(ipv6hdr, 0, sizeof(struct ip6_hdr)); // vynuluje IPv6 hlavicku

    ipv6hdr->ip6_vfc = 0x06 << 4; // verze
    // velikost payloadu+icmp6_hdr
    ipv6hdr->ip6_plen = htons(payload_len + sizeof(struct icmp6_ptb));
    ipv6hdr->ip6_nxt = IPPROTO_ICMPV6;
    ipv6hdr->ip6_hlim = 255;
    for (i = 0; i < IPV6_ADDR16_CNT; i++)
    { // kopie ÏPv6 adres
        ipv6hdr->ip6_dst.s6_addr16[i] = pkt_ipv6hdr->ip6_src.s6_addr16[i];
        ipv6hdr->ip6_src.s6_addr16[i] = pkt_ipv6hdr->ip6_dst.s6_addr16[i];
    }

    // ICMPv6 hlavicka
    icmp6 = (struct icmp6_ptb *) (ptb + ETHER_HDR_LEN + sizeof(struct ip6_hdr));

    icmp6->type = PTB_ICMP6_TYPE;
    icmp6->code = 0;
    icmp6->cksum = 0;
    // MTU rozhrani, pres ktere nelze projit (druhe lokalni)
    icmp6->mtu = htonl(ifs_prms->dest->mtu);

    u_char *pl = (u_char *) (ptb + PTB_HDRS_LEN); // adresa, kde zacina payload
    memcpy((u_char *) pl, (u_char *) pkt_ipv6hdr, payload_len); // kopie payload

    uint16_t cksum = icmpv6_cksum( &(ipv6hdr->ip6_src),
                                   &(ipv6hdr->ip6_dst), (void *) icmp6,
                                   (payload_len + sizeof(struct icmp6_ptb)) );
    icmp6->cksum = cksum; // spocitany ICMPv6 checksum

    out.start_str = OUTPUT_OUT;
    out.type_str = OUTPUT_PTB;
    out.ip_dest = &(ipv6hdr->ip6_dst);
    out.ip_src = &(ipv6hdr->ip6_src);
    out.mac_dest = (struct ether_addr *) &(ehdr->ether_dhost);
    out.mac_src = (struct ether_addr *) &(ehdr->ether_shost);
    out.ts = (struct timeval *) &(header->ts);
    print_out_ln(&out);
    
    // poslat na rozhrani, ze ktereho paket zachytil
    pcap_inject(ifs_prms->src->pcap_ptr, ptb, (PTB_HDRS_LEN + payload_len));
    
    free(ptb);

    return 0;
}


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
                       void *data, size_t len )
{
    union units
    {
        uint32_t dword;
        uint16_t word[2];
        uint8_t byte[4];
    } unit;

    int i;
    uint32_t cksum = 0;
    // scitani po 16b

    // 16B+16B - velikost dvou ipv6
    for (i = 0; i < IPV6_ADDR16_CNT; i++)
        cksum += (src->s6_addr16[i] + dest->s6_addr16[i]); // 2B+2B

    // 4B - velikost dat
    unit.dword = htonl(len);
    cksum += (unit.word[0] + unit.word[1]); // 2B+2B

    // 4B - typ dalsi hlavicky
    unit.dword = htonl(IPPROTO_ICMPV6); // cislo 58 = icmpv6
    cksum += (unit.word[0] + unit.word[1]); // 2B+2B

    while (len > 1)
    { // icmpv6 data
        cksum += *((uint16_t *) data);
        data = (uint16_t *) data + 1;
        len -= 2; // posun o 2B
    }

    if (len > 0)
        cksum += *((uint8_t *) data);

    while (cksum >> 16 != 0)
        cksum = (cksum & 0xffff) + (cksum >> 16);

    return ((uint16_t) (~cksum)); // provest NOT
}

/*
 * File:   ndpv6_printer.c
 * Author: David Krym
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h> // strftime
#include <string.h> // strcmp
#include <netinet/ether.h> // ether_ntoa_r
#include <arpa/inet.h> // inet_ntop
#include "ndpv6_printer.h"


#define TIME_STR_LEN 9 // delka retezce hodin ve formatu HH:MM:SS
#define MAC_STR_LEN 18 // delka retezce MAC adresy ve formatu aa:bb:cc:dd:ee:ff


/**
 * Vytiskne radek na vystup. Tiskne zpravy "IN: ..." a "OUT: ...".
 * Format:
 * IN/OUT: time, srcmac > dstmac, ipv6src > ipv6dst
 * IN/OUT: time, srcmac > dstmac, ipv6src > ipv6dst, ICMPv6[NS|NA] target, slla
 * @param p Ukazatel na strukturu s informacema pro tisk.
 * @return void.
 */
void print_out_ln(struct output *p)
{
    char str_time[TIME_STR_LEN];
    char ip_str_src[INET6_ADDRSTRLEN];
    char ip_str_dest[INET6_ADDRSTRLEN];
    char mac_str_src[MAC_STR_LEN];
    char mac_str_dest[MAC_STR_LEN];

    // cas do formatu HH:MM:SS
    strftime(str_time, sizeof(str_time), "%T", localtime(&(p->ts->tv_sec)));

    // ip prevod na text
    inet_ntop(AF_INET6, p->ip_src, ip_str_src, INET6_ADDRSTRLEN);
    inet_ntop(AF_INET6, p->ip_dest, ip_str_dest, INET6_ADDRSTRLEN);
    // mac prevod na text
    ether_ntoa_r(p->mac_src, mac_str_src);
    ether_ntoa_r(p->mac_dest, mac_str_dest);

    if (p->type_str != NULL)
    { // Neighbour discovery, delsi vypis
        if (strcmp(p->type_str, OUTPUT_PTB) == 0)
        { // vypis Packet Too Big
            printf( "%s: %s, %s > %s, %s > %s, ICMPv6[%s]\n",
                    p->start_str, str_time, mac_str_src, mac_str_dest,
                    ip_str_src, ip_str_dest, p->type_str );
        }
        else
        { // vypis NA nebo NS
            char ip_str_target[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, p->ip_target, ip_str_target, INET6_ADDRSTRLEN);

            printf( "%s: %s, %s > %s, %s > %s, ICMPv6[%s] %s, %s\n",
                    p->start_str, str_time, mac_str_src, mac_str_dest,
                    ip_str_src, ip_str_dest, p->type_str, ip_str_target,
                   ((p->mac_slla == NULL) ? "none" : ether_ntoa(p->mac_slla)) );
        }

    }
    else
    { // kratsi vypis
        printf( "%s: %s, %s > %s, %s > %s\n",
                p->start_str, str_time, mac_str_src, mac_str_dest,
                ip_str_src, ip_str_dest );
    }

    return;
}


/**
 * Vytiskne informace o rozhranich, na kterych se bude naslouchat.
 * @param ifs_prms Ukazatel na obe lokalni rozhrani.
 * @param if_in Nazev rozhrani in.
 * @param if_out Nazev rozhrani out.
 * @return void.
 */
void print_ifaces(char *if_in, char *if_out, struct ifs *ifs_prms)
{
    int i; // pocitadlo
    char ip_str[INET6_ADDRSTRLEN];

    printf( "interface in : %-5s | MTU: %-6d | MAC: %-18s\n", if_in,
            ifs_prms->src->mtu, (char *) ether_ntoa(&(ifs_prms->src->mac)) );
    printf( "  IPv6 adresy:");
    for (i = 0; i < ifs_prms->src->ipv6_cnt; i++)
    {
        inet_ntop( AF_INET6, &(ifs_prms->src->ipv6[i]),
                   ip_str, INET6_ADDRSTRLEN );
        printf( ((i==0) ? "  %s\n" : "                %s\n"), ip_str);
    }
    printf("\n");

    printf( "interface out: %-5s | MTU: %-6d | MAC: %-18s\n", if_out,
            ifs_prms->dest->mtu, (char *) ether_ntoa(&(ifs_prms->dest->mac)) );
    printf( "  IPv6 adresy:");
    for (i = 0; i < ifs_prms->dest->ipv6_cnt; i++)
    {
        inet_ntop( AF_INET6, &(ifs_prms->dest->ipv6[i]),
                   ip_str, INET6_ADDRSTRLEN );
        printf( ((i==0) ? "  %s\n" : "                %s\n"), ip_str);
    }
    printf("\n\n");

    return;
}
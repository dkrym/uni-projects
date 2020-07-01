/*
 * File:   ndpv6_ipv6.c
 * Author: David Krym
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h> // getifaddr
#include "ndpv6_ipv6.h"

#define TRUE 1
#define FALSE 0


/**
 * Ziska vsechny IPv6 adresy pro zadane lokalni rozhrani.
 * @param if_name Jmeno rozhrani.
 * @param ip Ukazatel na pole IPv6 adres.
 * @return Vraci pocet nalezenych IPv6 adres pro zadane rozhrani.
 */
int get_if_ipv6(char *if_name, struct in6_addr **ip)
{
    struct ifaddrs *ifa, *ifa_list;
    struct in6_addr *ip_tmp, *ip_tmp2;
    int i;
    int cnt = 0; // ipv6 count

    if (getifaddrs(&ifa_list) == -1)
        return -1; // chyba

    for (ifa = ifa_list; ifa != NULL; ifa = ifa->ifa_next)
    { // prochazeni seznamu adres vsech rozhranich
        if ( (ifa->ifa_addr->sa_family == AF_INET6)
              && (strcmp(ifa->ifa_name, if_name) == 0) )
        { // je to ipv6 naseho rozhrani?
            ip_tmp = &((struct sockaddr_in6 *) ifa->ifa_addr)->sin6_addr;
            cnt++;
            ip_tmp2 = (struct in6_addr *) // zvetsi se pole
                      realloc(*ip, sizeof (struct in6_addr) * cnt);

            for (i = 0; i < INET_ADDRSTRLEN; i++) // kopie adresy
                ip_tmp2[cnt - 1].s6_addr[i] = ip_tmp->s6_addr[i];

            *ip = ip_tmp2;
        }
    }

    freeifaddrs(ifa_list);
    return cnt;
}


/**
 * Zadanou IPv6 porovna s adresama na lokalnich rozhranich.
 * @param ifaces Ukazatele na obe lokalni rozhrani..
 * @param ip Adresa, pro kterou se urcuje, zda nalezi lokalnimu rozhrani.
 * @return Vraci 1, pokud se jedna o adresu lokalniho rozhrani, jinak 0.
 */
int is_ipv6_local(struct ifs *ifaces, struct in6_addr *ip)
{
    int i;
    // kazde rozhrani muze mit ruzny pocet IPv6, proto radeji 2 cykly

    for (i = 0; i < ifaces->src->ipv6_cnt; i++)
    {
        if (IN6_ARE_ADDR_EQUAL(ip, &(ifaces->src->ipv6[i])))
            return TRUE; // ipv6 patri nasemu rozhrani
    }
    
    for (i = 0; i < ifaces->dest->ipv6_cnt; i++)
    {
        if (IN6_ARE_ADDR_EQUAL(ip, &(ifaces->dest->ipv6[i])))
            return TRUE; // ipv6 patri nasemu rozhrani
    }

    return FALSE;
}
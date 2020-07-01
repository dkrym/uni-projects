/*
 * File:   ndpv6_neigh_cache.c
 * Author: David Krym
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h> // inet_ntop()
#include <netinet/ether.h> // ether_ntoa()
#include "ndpv6_if.h" // makra pro MAC
#include "ndpv6_neigh_cache.h"


/**
 * Prohleda NC tabulku a vraci MAC adresu k zadane IPv6 arese.
 * @param list Ukazatel na NC tabulku.
 * @param ipv6 Ukazatel na IPv6 adresu.
 * @return Ukazatel na MAC adresu, NULL pri nenalezeni.
 */
struct ether_addr *NC_get_mac(struct nc_table *list, struct in6_addr *ipv6)
{
    struct nc_entry *entry;
    struct ether_addr *mac = NULL;

    for (entry = list->first; entry != NULL; entry = entry->next)
    {
        if (IN6_ARE_ADDR_EQUAL(ipv6, &(entry->ipv6)))
        {
            mac = &(entry->mac); // nalezena pozadovana ipv6
            break;
        }
    }

    return mac;
}


/**
 * Vytvoreni noveho zaznamu v NC tabulce.
 * @param list Ukazatel na NC tabulku.
 * @param mac  Ukazatel na MAC adresu.
 * @param ipv6 Ukazatel na IPv6 adresu.
 * @return void.
 */
void NC_insert( struct nc_table *list,
                struct ether_addr *mac,
                struct in6_addr *ipv6 )
{
    int i; // pocitadlo
    struct nc_entry *pitem = NULL;
    pitem = malloc(sizeof (struct nc_entry)); // polozka v NC

    if (pitem == NULL)
        perror("Nelze alokovat pamet pro zaznam NC tabulky");
    else
    {
        for (i = 0; i < INET_ADDRSTRLEN; i++) // INET_ADDRSTRLEN=16
        {
            pitem->ipv6.s6_addr[i] = ipv6->s6_addr[i];
        }

        copy_mac(&(pitem->mac), mac); // zkopirovani MAC (dest,src)

        pitem->next = list->first;
        list->first = pitem; // zaradi zaznam na zacatek
    }

    return;
}


/**
 * Aktualizuje MAC adresu pro danou IPv6 adresu v NC tabulce.
 * Pokud neexistuje zaznam s danou IPv6 adresou, vytvori se zaznam novy.
 * @param list Ukazatel na NC tabulku.
 * @param mac  Ukazatel na MAC adresu.
 * @param ipv6 Ukazatel na IPv6 adresu.
 * @return void.
 */
void NC_update( struct nc_table *list,
                struct ether_addr *mac,
                struct in6_addr *ipv6 )
{
    if ( IN6_IS_ADDR_UNSPECIFIED(ipv6)
         || IS_MAC_MULTICAST(mac)
         || IN6_IS_ADDR_MULTICAST(ipv6) )
    { // nevkladat do tabulky multicast a unspecified adresy
        return; 
    }

    struct nc_entry *entry;
    for (entry = list->first; entry != NULL; entry = entry->next)
    { // prohledani NC tabulky
        if (IN6_ARE_ADDR_EQUAL(ipv6, &(entry->ipv6)))
        { // ipv6 uz existuje
            if (!MAC_ARE_ADDR_EQUAL(mac, &(entry->mac)))
            { // ulozena MAC je jina, aktualizujeme
                copy_mac(&(entry->mac), mac); // zkopiruje MAC (dest,src)
            }

            // tisk obsahu NC tabulky
            //NC_print(list);
            return;
        }
    }

    NC_insert(list, mac, ipv6); // IPv6 nenalezena, vlozi novou polozku

    // tisk obsahu NC tabulky
    //NC_print(list);
    return;
}


/**
 * Vytiskne celou NC tabulku.
 * @param list Ukazatel na NC tabulku.
 * @return void.
 */
void NC_print(struct nc_table *list)
{
    struct nc_entry *entry;
    char ip_str[INET6_ADDRSTRLEN];

    printf("\n");
    for (entry = list->first; entry != NULL; entry = entry->next)
    {
        inet_ntop(AF_INET6, &(entry->ipv6), ip_str, INET6_ADDRSTRLEN);
        printf( "| %-50s | %-18s |\n", ip_str,
                (char *) ether_ntoa(&(entry->mac)) );
    }

    return;
}
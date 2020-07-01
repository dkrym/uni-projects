/*
 * File:   ndpv6_neigh_cache.h
 * Author: David Krym
 */

#ifndef NDPV6_NEIGH_CACHE_H
#define	NDPV6_NEIGH_CACHE_H

#include <net/ethernet.h> // struct ether_addr
#include <netinet/in.h>   // struct in6_addr


/** Jeden zaznam NC tabulky. */
struct nc_entry
{
    struct nc_entry *next;
    struct ether_addr mac;
    struct in6_addr ipv6;
};


/** NC tabulka. */
struct nc_table
{
    struct nc_entry *first;
};


/**
 * Prohleda NC tabulku a vraci MAC adresu k zadane IPv6 arese.
 * @param list Ukazatel na NC tabulku.
 * @param ipv6 Ukazatel na IPv6 adresu.
 * @return Ukazatel na MAC adresu, NULL pri nenalezeni.
 */
struct ether_addr *NC_get_mac(struct nc_table *list, struct in6_addr *ipv6);


/**
 * Vytvoreni noveho zaznamu v NC tabulce.
 * @param list Ukazatel na NC tabulku.
 * @param mac  Ukazatel na MAC adresu.
 * @param ipv6 Ukazatel na IPv6 adresu.
 * @return void.
 */
void NC_insert( struct nc_table *list,
                struct ether_addr *mac,
                struct in6_addr *ipv6 );


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
                struct in6_addr *ipv6 );


/**
 * Vytiskne celou NC tabulku.
 * @param list Ukazatel na NC tabulku.
 * @return void.
 */
void NC_print(struct nc_table *list);


#endif	/* NDPV6_NEIGH_CACHE_H */


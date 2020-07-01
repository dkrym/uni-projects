/*
 * File:   ndpv6_if.h
 * Author: David Krym
 */

#ifndef NDPV6_IF_H
#define	NDPV6_IF_H

#include <net/ethernet.h> // struct ether_addr
#include <netinet/in.h>   // struct in6_addr
#include <pcap.h> // struct pcap
#include "ndpv6_neigh_cache.h" // struct nc_table


/** Makro pro porovnani dvou MAC adres. */
#define MAC_ARE_ADDR_EQUAL(a,b) \
	((((__const uint16_t *) (a))[0] == ((__const uint16_t *) (b))[0])     \
	 && (((__const uint16_t *) (a))[1] == ((__const uint16_t *) (b))[1])  \
	 && (((__const uint16_t *) (a))[2] == ((__const uint16_t *) (b))[2]))


/** Makro pro zjisteni, zda MAC adresa znaci IPv6 multicast */
#define IS_MAC_MULTICAST(a) (((__const uint16_t *) (a))[0] == 0x3333)


/** Struktura s informacemi o lokalnim rozhranim. */
struct if_params
{
    struct ether_addr mac;
    int mtu;
    struct in6_addr *ipv6; // pointer to array
    int ipv6_cnt;
    struct nc_table *nc_tbl;
    struct pcap *pcap_ptr;
};


/** Struktura s ukazatelama na dve lokalni rozhrani. */
struct ifs
{
    struct if_params *src;
    struct if_params *dest;
};


/**
 * Zkopiruje MAC adresu.
 * @param dest Ukazatel, kam se MAC zkopiruje.
 * @param src Ukazatel na MAC adresu, ktera bude zkopirovana.
 * @return void.
 */
void copy_mac(struct ether_addr *dest, struct ether_addr *src);


/**
 * Ziska MAC adresu zadaneho lokalniho rozhrani a jeho MTU hodnotu.
 * @param s Desktriptor socketu.
 * @param if_name Nazev lokalniho rozhrani.
 * @param mac Ukazatel na MAC adresu, kam se ulozi nalezena adresa.
 * @param mtu Ukazatel na promennou, kam se ulozi MTU hodnota rozhrani.
 * @return Vraci 0 pri uspechu, -1 pri chybe.
 */
int get_if_mac_mtu(int s, char *if_name, struct ether_addr *mac, int *mtu);


#endif	/* NDPV6_IF_H */


/*
 * File:   ndpv6_ipv6.h
 * Author: David Krym
 */

#ifndef NDPV6_IPV6_H
#define	NDPV6_IPV6_H

#include <netinet/in.h> // struct in6_addr
#include "ndpv6_if.h" // struct ifs

/**
 * Ziska vsechny IPv6 adresy pro zadane lokalni rozhrani.
 * @param if_name Jmeno rozhrani.
 * @param ip Ukazatel na pole IPv6 adres.
 * @return Vraci pocet nalezenych IPv6 adres pro zadane rozhrani.
 */
int get_if_ipv6(char *if_name, struct in6_addr **ip);


/**
 * Zadanou IPv6 porovna s adresama na lokalnich rozhranich.
 * @param ifaces Ukazatele na obe lokalni rozhrani..
 * @param ip Adresa, pro kterou se urcuje, zda nalezi lokalnimu rozhrani.
 * @return Vraci 1, pokud se jedna o adresu lokalniho rozhrani, jinak 0.
 */
int is_ipv6_local(struct ifs *ifaces, struct in6_addr *ip);


#endif	/* NDPV6_IPV6_H */


/*
 * File:   ndpv6_if.c
 * Author: David Krym
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // close()
#include <net/if.h> // struct ifreq
#include <sys/ioctl.h> // ioctl
#include <sys/types.h> // socket, uvedeno jen pro kompatibilitu
#include <sys/socket.h> // socket
#include <string.h> // strcpy
#include "ndpv6_if.h"


/**
 * Ziska MAC adresu zadaneho lokalniho rozhrani a jeho MTU hodnotu.
 * @param s Desktriptor socketu.
 * @param if_name Nazev lokalniho rozhrani.
 * @param mac Ukazatel na MAC adresu, kam se ulozi nalezena adresa.
 * @param mtu Ukazatel na promennou, kam se ulozi MTU hodnota rozhrani.
 * @return Vraci 0 pri uspechu, -1 pri chybe.
 */
int get_if_mac_mtu(int s, char *if_name, struct ether_addr *mac, int *mtu)
{
    struct ifreq ifr;

    strcpy(ifr.ifr_name, if_name);

    // ziskani MTU
    if ((s == -1) || (ioctl(s, SIOCGIFMTU, &ifr) == -1))
    { // chyba
        close(s);
        return -1;
    }
    *mtu = ifr.ifr_mtu;

    // ziskani MAC adresy
    if ((ioctl(s, SIOCGIFHWADDR, &ifr) == -1))
    { // chyba
        close(s);
        return -1;
    }

    copy_mac(mac, (struct ether_addr *) &(ifr.ifr_hwaddr.sa_data)); // kopie MAC

    return 0;
}


/**
 * Zkopiruje MAC adresu.
 * @param dest Ukazatel, kam se MAC zkopiruje.
 * @param src Ukazatel na MAC adresu, ktera bude zkopirovana.
 * @return void.
 */
void copy_mac(struct ether_addr *dest, struct ether_addr *src)
{
    int i;
    for (i = 0; i < ETH_ALEN; i++) // ETH_ALEN=6
        dest->ether_addr_octet[i] = src->ether_addr_octet[i];

    return;
}
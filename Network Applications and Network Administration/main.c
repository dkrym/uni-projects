/*
 * File:   main.c
 * Author: David Krym
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // getopt()
#include <pcap.h>
#include <pcap/pcap.h> // fileno
#include <unistd.h> // select()
#include <signal.h> // signal()
#include <netinet/ether.h> // ether_ntoa()
#include <arpa/inet.h> // inet_ntop()
#include "ndpv6_if.h" // struct ifs
#include "ndpv6_neigh_cache.h" // struct nc_table
#include "ndpv6_packet.h" // packet_rcv()
#include "ndpv6_ipv6.h" // get_if_ipv6()


// funkcni deklarace
int capturing_loop(char *if_in, char *if_out);
void sig_handler(int sig);
int init_pcap( const char *if_in, struct pcap **pcap_in,
               const char *if_out,struct pcap **pcap_out );


/**
 * Globalni promenna s ukazateli na obe rozhrani.
 * Pouziva ji jedina funkce, a to funkce "sig_handler()" pri prichodu signalu,
 * aby funkce mohla dealokovat dynamicky pridelenou pamet a ukoncit program.
 */
struct ifs g_interfaces;


/**
 * Vstupni bod aplikace.
 */
int main(int argc, char** argv)
{
    char ch;
    char *if_in = NULL; // vnitrni rozhrani
    char *if_out = NULL; // vnejsi rozhrani

    // zaregistrovani obsluzne funkce pri prijeti signalu
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGQUIT, sig_handler);

    while ((ch = getopt(argc, argv, "i:o:")) != -1)
    { // zpracovani parametru
        switch (ch)
        {
            case 'i':
                if_in = optarg;
                break;
            case 'o':
                if_out = optarg;
                break;
        }
    }

    if (if_in == NULL || if_out == NULL)
    { // nebyly zadany spravne parametry
        printf("Usage: \n  %s -i interface1 -o interface2\n", argv[0]);
        return (EXIT_FAILURE);
    }

    // funkce se smyckou na zachytavani paketu
    if (capturing_loop(if_in, if_out) == -1) 
        return (EXIT_FAILURE);

    return (EXIT_SUCCESS);
}


/**
 * Hlavni funkce pro zachytavani paketu.
 * Nasloucha na obou zadanych rozhranich v promiskuitnim modu.
 * @param if_in Nazev prvniho rozhrani
 * @param if_out Nazev druheho rozhrani
 * @return Chybovy kod, -1 pri chybe.
 */
int capturing_loop(char *if_in, char *if_out)
{
    struct pcap *pcap_in = NULL, *pcap_out = NULL; // ukazatele na pcap
    struct if_params if_in_prms, if_out_prms; // parametry rozhrani
    struct ifs interfaces = {NULL, NULL}; // ukazatel na SRC a DEST interfaces
    struct nc_table NCtable_in = {NULL}; // NC tabulka
    struct nc_table NCtable_out = {NULL}; // NC tabulka
    int fd_in, fd_out, max_fd_num; // file descriptory
    fd_set read_fds; // set file descriptoru
    int sel_ret; // return kod selectu

    // inicializace pcapu na obou rozhranich a nastaveni filtru
    if (init_pcap(if_in, &pcap_in, if_out, &pcap_out) == -1)
        return -1;

    // realne filedeskriptory
    fd_in = pcap_fileno(pcap_in);
    fd_out = pcap_fileno(pcap_out);
    max_fd_num = ((fd_in > fd_out) ? fd_in : fd_out) + 1; // max file desc +1

    // prvni rozhrani
    if (get_if_mac_mtu( fd_in, if_in, &(if_in_prms.mac),
                        &(if_in_prms.mtu) )  == -1)
    {
        fprintf( stderr,
                 "Chyba pri ziskani MAC adresy nebo MTU rozhrani: %s\n", if_in);
        return -1;
    }

    if_in_prms.pcap_ptr = pcap_in; // ukazatel na pcap relaci pro dany rozhrani
    if_in_prms.nc_tbl = &NCtable_in; // NC tabulka
    if_in_prms.ipv6 = NULL;

    if ((if_in_prms.ipv6_cnt = get_if_ipv6(if_in, &(if_in_prms.ipv6))) <= 0)
    { // chyba pri ziskani ipv6 adresy/rozhrani nema ipv6 adresu
        fprintf(stderr, "Chyba pri ziskani IPv6 adres rozhrani: %s\n", if_in);
        return -1;
    }

    // druhe rozhrani
    if (get_if_mac_mtu( fd_out, if_out,
                        &(if_out_prms.mac), &(if_out_prms.mtu) ) == -1)
    {
        fprintf( stderr,
                 "Chyba pri ziskani MAC adresy nebo MTU rozhrani: %s\n", if_out);
        return -1;
    }

    if_out_prms.pcap_ptr = pcap_out; // ukazatel na pcap pro dany rozhrani
    if_out_prms.nc_tbl = &NCtable_out; // NC tabulka
    if_out_prms.ipv6 = NULL;

    if ((if_out_prms.ipv6_cnt = get_if_ipv6(if_out, &(if_out_prms.ipv6))) <= 0)
    { // chyba pri ziskani ipv6 adresy/rozhrani nema ipv6 adresu
        fprintf(stderr, "Chyba pri ziskani IPv6 adres rozhrani: %s\n", if_out);
        return -1;
    }

    // globalni promenna pro dealokaci pameti pri ukonceni na signal
    g_interfaces.src = &if_in_prms;
    g_interfaces.dest = &if_out_prms;

    interfaces.src = &if_in_prms;
    interfaces.dest = &if_out_prms;
    print_ifaces(if_in, if_out, &interfaces); // info o rozhranich vytisknout

    while (1)
    { // prijimani paketu na obou rozhranich

        FD_ZERO(&read_fds);
        FD_SET(fd_in, &read_fds);
        FD_SET(fd_out, &read_fds);

        // pockat dokud neco neprijde
        sel_ret = select(max_fd_num, &read_fds, NULL, NULL, NULL);

        if (sel_ret == -1)
        {
            perror("Chyba selectu:");
            return -1;
        }
        else
        { // FD pripraven
            if (FD_ISSET(fd_in, &read_fds))
            { // out
                interfaces.src = &if_in_prms;
                interfaces.dest = &if_out_prms;

                pcap_dispatch(pcap_in, 1, packet_rcvd, (u_char *) &interfaces);
            }
            else if (FD_ISSET(fd_out, &read_fds))
            { // out
                interfaces.src = &if_out_prms;
                interfaces.dest = &if_in_prms;
                pcap_dispatch(pcap_out, 1, packet_rcvd, (u_char *) &interfaces);
            }
        }
    }

    pcap_close(pcap_in);
    pcap_close(pcap_out);

    return 1;
}


/**
 * Funkce pro uvolneni dynamicky alokovane pameti pri prichodu signalu.
 * Zpracuje signaly SIGTERM, SIGQUIT, SIGINT a ukonci program.
 * @param sig Cislo prijateho signalu
 * @return Ukonci program.
 */
void sig_handler(int sig)
{ // brano z globalni promenne g_interfaces
    
    // ukonceni zachytavani
    pcap_close(g_interfaces.src->pcap_ptr);
    pcap_close(g_interfaces.dest->pcap_ptr);

    // vymazani NC tabulky
    struct nc_entry *entry;
    while (g_interfaces.src->nc_tbl->first != NULL)
    {
        entry = g_interfaces.src->nc_tbl->first;
        g_interfaces.src->nc_tbl->first = entry->next;
        free(entry);
    }
    while (g_interfaces.dest->nc_tbl->first != NULL)
    {
        entry = g_interfaces.dest->nc_tbl->first;
        g_interfaces.dest->nc_tbl->first = entry->next;
        free(entry);
    }

    // uvolni pamet pro ulozeni lokalnich ipv6 adres
    free(g_interfaces.src->ipv6);
    free(g_interfaces.dest->ipv6);

    exit(EXIT_SUCCESS);
}


/**
 * Funkce inicializuje pcap na obou rozhranich a nastavi filtr.
 * @param if_in Nazev vnitrniho rozhrani.
 * @param pcap_in Ukazatel na pcap strukturu pro vnitrni rozhrani.
 * @param if_out Nazev vnejsiho rozhrani.
 * @param pcap_out Ukazatel na pcap strukturu pro vnejsi rozhrani.
 * @param fp Ukazatel na zkompilovany filter
 * @return Vraci -1 pri chybe.
 */
int init_pcap( const char *if_in, struct pcap **pcap_in,
               const char *if_out,struct pcap **pcap_out )
{
    char errbuf[PCAP_ERRBUF_SIZE];
    struct bpf_program fp; // zkompilovany filter pro pcap
    char filter_exp[] = "ip6"; // zachytavat jen ipv6 pakety
    bpf_u_int32 net = 0;

    if ((*pcap_in = pcap_open_live(if_in, BUFSIZ, 1, -1, errbuf)) == NULL)
    {
        fprintf(stderr, "Nelze otevrit \"%s\": %s\n", if_in, errbuf);
        return -1;
    }

    if ((*pcap_out = pcap_open_live(if_out, BUFSIZ, 1, -1, errbuf)) == NULL)
    {
        fprintf(stderr, "Nelze otevrit \"%s\": %s\n", if_in, errbuf);
        return -1;
    }

    if (pcap_compile(*pcap_in, &fp, filter_exp, 0, net) == -1)
    {
        fprintf( stderr, "Nelze rozparsovat filter %s: %s\n",
                 filter_exp, pcap_geterr(*pcap_in) );
        return -1;
    }

    if (pcap_setfilter(*pcap_in, &fp) == -1)
    {
        fprintf( stderr, "Nelze nastavit filter %s: %s\n",
                 filter_exp, pcap_geterr(*pcap_in) );
        return -1;
    }

    if (pcap_setfilter(*pcap_out, &fp) == -1)
    {
        fprintf( stderr, "Nelze nastavit filter %s: %s\n",
                 filter_exp, pcap_geterr(*pcap_out) );
        return -1;
    }

    if (pcap_setnonblock(*pcap_in, 1, errbuf) == -1)
    {
        fprintf( stderr, "Nelze nastavit \"%s\" jako neblokujici: %s\n",
                 if_in, errbuf );
        return -1;
    }

    if (pcap_setnonblock(*pcap_out, 1, errbuf) == -1)
    {
        fprintf( stderr, "Nelze nastavit \"%s\" jako neblokujici: %s\n",
                 if_out, errbuf );
        return -1;
    }

    pcap_freecode(&fp);

    return 0;
}
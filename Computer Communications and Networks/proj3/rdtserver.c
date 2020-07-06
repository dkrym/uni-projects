/**
 * Projekt IPK3
 * Autor: David Krym
 * Datum: 22.4.2011
 */

#include "rdtserver.h"

#define TMPCHARS 10
//#define DEBUGPRINT

in_addr_t remote_addr = 0x7f000001;
in_port_t src_port = 0;
in_port_t dest_port = 0;

enum states
{
  HANDSHAKE,
  RECIEVEDER
};


/**
 * Main.
 * @return Navratovy kod
 */
int main(int argc, char **argv )
{
  // zpracovani parametru
  char ch;
  while ((ch = getopt(argc,argv,"s:d:h")) != -1)
  {
    switch(ch)
    {
      case 's':
        src_port = atol(optarg);
        break;
      case 'd':
        dest_port = atol(optarg);
        break;

      case 'h':
        fprintf(stdout, PROGRAM_INFO);
        fprintf(stdout, "usage: rdtclient -s src_port -d dest_port\n\n");
        fprintf(stdout, "  s port    : Local port.\n" );
        fprintf(stdout, "  d port    : Server port.\n" );
        exit(EXIT_SUCCESS);
    }
  }
  if (src_port == 0 || dest_port == 0)
  {
    fprintf(stderr, "Missing required arguments! Type '%s -h' for help.\n", PROGRAM);
    exit(EXIT_FAILURE);
  }

  int retcode = makeConn();

  return retcode;
}


/**
 * Vypocita kontrolni soucet.
 * @param data Retezec, ze ktereho se bude pocitat.
 * @param len Delka tohoto retezce.
 * @return Kontrolni soucet
 */
uint32_t fletcher32( char *data, size_t len )
{
    uint32_t sum1 = 0xffff, sum2 = 0xffff;
    while (len)
    {
        unsigned tlen = len > 360 ? 360 : len;
        len -= tlen;
        do {
        sum1 += *data++;
        sum2 += sum1;
        } while (--tlen);
        sum1 = (sum1 & 0xffff) + (sum1 >> 16);
        sum2 = (sum2 & 0xffff) + (sum2 >> 16);
    }
    /* Second reduction step to reduce sums to 16 bits */
    sum1 = (sum1 & 0xffff) + (sum1 >> 16);
    sum2 = (sum2 & 0xffff) + (sum2 >> 16);
    return sum2 << 16 | sum1;
}


/**
 * Vytvori paket ve formatu: checksum|packet_id|message
 * @param num Cislo paketu.
 * @param text Zprava na poslani.
 * @return Vraci vysledny paket
 */
char *createPacket(int num, char *text)
{
  char *pkt;
  //char tmp[TMPCHARS] = {0};
  char tmp_pkt[MAXCHARS*2] = {0};
  
  if ((pkt = (char *)malloc(sizeof(char) * MAXCHARS * 2)) == NULL) // 160 znaku
  {
    perror("Malloc error");
    exit(EXIT_FAILURE);
  }
  
  pkt[0] = '\0';
  sprintf(tmp_pkt, "|%d|", num); // cislo paketu
  strcat(tmp_pkt, text); // zprava k cislu paketu
  
  //printf("fletcher: %s : %d\n", tmp_pkt, strlen(tmp_pkt));
  
  uint32_t checksum = fletcher32(tmp_pkt, strlen(tmp_pkt));
  
  sprintf(pkt, "%u", checksum);
  strcat(pkt, tmp_pkt);
  
  
  return pkt;
}


/**
 * Zkontroluje paket na spravnost.
 * @param pkt Prijaty paket.
 * @param len Delka paketu.
 * @return Pokud byl paket vporadku, vrati jeho cislo. Jinak -1.
 */
int packetCheck(char *pkt, int len)
{
  int i = 0;
  char tmp[MAXCHARS*2] = {0}; // docasne uloziste pro checksum
  char tmp_pkt[MAXCHARS*2] = {0}; // paket bez checksum
  
  // vse pred prvnim svislitkem je checksum
  while (pkt[i] != '|' && i < len)
  {
    tmp[i] = pkt[i];
    ++i;
  }
  
  strcpy(tmp_pkt, pkt+i); // zprava bez checksum
  uint32_t checksum = fletcher32(tmp_pkt, strlen(tmp_pkt));
  char *token = strtok(tmp_pkt, "|"); // cislo paketu
  char *token_msg = strtok(NULL, "|"); // zprava
    
  if (checksum == (unsigned)atoll(tmp))
  { // paket vporadku
    if (token != NULL && token_msg != NULL)
    {
      strcpy(pkt, token_msg); // zprava
      return atol(token);
    }
  }
  
  return -1;
}

/**
 * Vytvori spojeni a resi prijem vsech paketu a odeslani potvrzeni.
 * @return Navratovy kod.
 */
int makeConn(void)
{
  char recvline[MAXCHARS*2];
  int expected_pkt = 0;
  char *packet;
  
  // inicializace udt
  int udt = udt_init(src_port);

  fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // make stdin reading non-clocking

  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(udt, &readfds);
  
  tDLList list;
  DLInitList(&list); // init listu
  
  enum states state = HANDSHAKE; // pocatecni stav
  
  while (select(udt+1, &readfds, NULL, NULL, NULL))
  {
    if (state == HANDSHAKE)
    { // ceka na handshake
      if (FD_ISSET(udt, &readfds))
      { // prijat paket
        int n = udt_recv(udt, recvline, MAXCHARS*2, NULL, NULL);
        recvline[n] = 0;
        int pkt_num = packetCheck(recvline, n);
        if (pkt_num != -1)
        { // paket vporadku
          if (strcmp(recvline, "rdtHANDSHAKE") == 0)
          { state = RECIEVEDER; }
          
          char *packet = createPacket(0, "rdtACK"); // vytvori paket
          if (!udt_send(udt, remote_addr, dest_port, packet, strlen(packet)))
          {
            perror("Chyba pri posilani: ");
          }
          free(packet);
        }
      }
      
      FD_ZERO(&readfds);
      FD_SET(udt, &readfds);
    }
    else if (state == RECIEVEDER)
    { // recieve paketu
      if (FD_ISSET(udt, &readfds))
      { // prijat paket
        int n = udt_recv(udt, recvline, MAXCHARS*2, NULL, NULL);
        recvline[n] = 0;

        //kontrola paketu na spravnost, cislo
        int pkt_num = packetCheck(recvline, n);

        if (pkt_num != -1)
        { // paket vporadku
          if (pkt_num == expected_pkt)
          { // ocekavany paket
            if (strcmp(recvline, "rdtENDSHAKE") == 0)
            { break; }
            fputs(recvline, stdout); fflush(stdout);
            expected_pkt++; // cislo dalsiho paketu
          }
          
          // jiny paket
          if (pkt_num > expected_pkt)
          { // chce jen pakety, ktere budeme potrebovat
            DLInsertLast(&list, pkt_num, recvline); // paket do listu
            
            #ifdef DEBUGPRINT
              DLFirst(&list);
              int a; char*b;
              fprintf(stderr, "----------\n");
              while (DLActive(&list))
              {
                DLCopy(&list, &a, &b);
                fprintf(stderr, "num: %3d %s", a, b);
                DLSucc(&list);
              }
              fprintf(stderr, "----------\n");
            #endif
            
            DLFirst(&list); // na zacatek listu
            int num;
            char *data;
            while (DLActive(&list))
            { // projit seznam, zda neobsahuje potrebne pakety
              DLCopy(&list, &num, &data);
              
              if (num < expected_pkt)
              { // nepotrebny paket msazat ze seznamu
                DLSucc(&list);
                if (DLActive(&list))
                  DLPreDelete(&list);
                else // jediny prvek
                  DLDeleteLast(&list);            
              }
              else if (num == expected_pkt)
              { // mame ulozen pozadovany paket
                fputs(data, stdout); fflush(stdout);
                expected_pkt++; // cislo dalsiho paketu
                DLSucc(&list);
                if (DLActive(&list))
                  DLPreDelete(&list);
                else // jediny prvek
                  DLDeleteLast(&list);
                DLFirst(&list);
              }
              else
                DLSucc(&list);
            }
          }
        }
        
        // send potvrzeni
        packet = createPacket(expected_pkt, "rdtACK"); // vytvori paket
        #ifdef DEBUGPRINT
          fprintf(stderr, "exp: %d\n", expected_pkt); fflush(stderr);
        #endif
        if (!udt_send(udt, remote_addr, dest_port, packet, strlen(packet)))
        {
          perror("Chyba pri posilani: "); // some error
        }
        free(packet);
      }
      
      // nastaveni pro opakovane cteni
      FD_ZERO(&readfds);
      FD_SET(udt, &readfds);
    }
  }
  
  fflush(stdout);
  DLDisposeList(&list);
  return EXIT_SUCCESS;
}

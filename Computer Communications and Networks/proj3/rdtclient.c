/**
 * Projekt IPK3
 * Autor: David Krym
 * Datum: 22.4.2011
 */

#include "rdtclient.h"

#define TMPCHARS 10
//#define DEBUGPRINT


// globalni promenne
char *swindow[SWINDOWSIZE] = {0}; // sliding window
int s_base = 0; // zacatek okna
int s_max = SWINDOWSIZE-1; // konec okna
int s_act = 0; // aktualni cislo
int udt;
struct itimerval itv;
in_addr_t remote_addr = 0x7f000001;
in_port_t src_port = 0;
in_port_t dest_port = 0;


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
  
  strcpy(tmp_pkt, pkt+i); // bez checksum
  uint32_t checksum = fletcher32(tmp_pkt, strlen(tmp_pkt));
  char *token = strtok(tmp_pkt, "|"); // cislo paketu
  char *token_msg = strtok(NULL, "|"); // zprava
  
  if (checksum == (unsigned)atoll(tmp))
  { // paket vporadku
    //printf("sedi\n");
    if (token != NULL && token_msg != NULL)
    {
      strcpy(pkt, token_msg); // zprava
      return atol(token);
    }
  }
  
  return -1;
}


/**
 * Vytvori spojeni a resi odeslani vsech paketu a prijem potvrzeni.
 * @return Navratovy kod.
 */
int makeConn(void)
{
  char sendline[MAXCHARS+1];
  char recvline[MAXCHARS*2];
  char *packet = NULL;
  int eof = 0; // eof souboru
  
  // udt inicializace
  udt = udt_init(src_port);

  fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);	// make stdin reading non-clocking

  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(udt, &readfds);
  FD_SET(STDIN_FILENO, &readfds);
  
  signal(SIGALRM, sigalrm_handler);
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGALRM);

  itv.it_interval.tv_sec = TIMER; 	// sets an interval of the timer
  itv.it_interval.tv_usec = 0;	
  itv.it_value.tv_sec = TIMER;		// sets an initial value
  itv.it_value.tv_usec = 0;
  
  handshake(); // handshake
  
  while (select(udt+1, &readfds, NULL, NULL, NULL))
  {
    if ((s_base <= s_act) && (s_act <= s_max))
    { // pokud je misto v SW
      
      // send
      if (FD_ISSET(STDIN_FILENO, &readfds) && (fgets(sendline, MAXCHARS+1, stdin)!=NULL))
      { // prectena zprava, udelame paket a poslem
        packet = createPacket(s_act, sendline); // vytvori paket
        swindow[s_act%SWINDOWSIZE] = packet; // vlozeni do SW
        
        #ifdef DEBUGPRINT
          fprintf(stderr, "base:max:act %d:%d:%d\n", s_base, s_max, s_act);fflush(stderr);
        #endif
        
        if (!udt_send(udt, remote_addr, dest_port, packet, strlen(packet)))
        { // posle packet
          perror("Chyba pri posilani: ");	// some error
        }
        
        if (s_act == s_base)
        { // s prvnim packetem spustit casovac
          sigprocmask(SIG_UNBLOCK, &sigmask, NULL);
          setitimer(ITIMER_REAL, &itv, NULL);
        }
        
        ++s_act; // aktualni cislo v SW
      }
      else
      { // je eof a prazdny SW?
        if (feof(stdin))
        {
          eof = 1;
          if (s_act == s_base)
          { // endshake pri prazdnem okne
            endshake();
            break;
          }
        }
      }
    }
    
    // recieve
    if (FD_ISSET(udt, &readfds))
    { // prisel paket
      int n = udt_recv(udt, recvline, MAXCHARS*2, NULL, NULL);
      recvline[n] = '\0'; // ukonceni zpravy
      
      //kontrola paketu na spravnost, cislo -- chodi zados o dalsi, tj s_act
      int pkt_num = packetCheck(recvline, n);
      if (pkt_num != -1)
      { // paket vporadku
        int i;
        #ifdef DEBUGPRINT
          fprintf(stderr, "base:max:ack -- %d:%d:%d\n", s_base,s_max,pkt_num);fflush(stderr);
        #endif
          
        for (i = (s_base); i < (pkt_num); i++)
        { // uvolnit nepotrabne pakety
          if (swindow[i%SWINDOWSIZE] != NULL)
          {
            free(swindow[i%SWINDOWSIZE]);
            swindow[i%SWINDOWSIZE] = NULL;
          }
        }
         
        if (pkt_num >= s_base)
        { // pokud je ACK vetsi nez aktualni base SW          
          if (s_base == pkt_num)
          { // znovu poslat stejny paket
            if (!udt_send(udt, remote_addr, dest_port, swindow[pkt_num%SWINDOWSIZE], strlen(swindow[pkt_num%SWINDOWSIZE])))
            { // posle packet
              perror("Chyba pri posilani: ");
            }
          }
          else
          { // jinak posunout SW
            s_max += pkt_num - s_base; // nove max v SW
            s_base = pkt_num; // nove min v SW
          }
          
          if ((s_act == s_base) && !eof) // potvrzeny vsechny pakety, vypnu casovac
            sigprocmask(SIG_BLOCK, &sigmask, NULL);
          else
          { // s posunutim s_base spoustim znovu casovac
            sigprocmask(SIG_UNBLOCK, &sigmask, NULL);
            setitimer(ITIMER_REAL, &itv, NULL);
          }
          
          if ((s_act == s_base) && eof)
          { // endshake
            endshake();
            break;
          }
        }
      }
    }
    
    // and again!
    FD_ZERO(&readfds);
    FD_SET(udt, &readfds);
    FD_SET(STDIN_FILENO, &readfds);
  }
  
  return EXIT_SUCCESS;
}


/**
 * Zkontroluje paket na spravnost.
 * @param sig Signal, nepotrebny.
 */
void sigalrm_handler(int sig)
{
  int i = sig; // jen kvuli warningu, nepotrebny parametr
  for (i = s_base; i < s_act; i++)
  {
    if (swindow[i%SWINDOWSIZE] != NULL)
    {
      if (!udt_send(udt, remote_addr, dest_port, swindow[i%SWINDOWSIZE], strlen(swindow[i%SWINDOWSIZE])))
      { // posle packet
        perror("Chyba pri posilani: ");
      }
    }
  }
  
  // Reinstall handler.
  signal(SIGALRM, sigalrm_handler);
}

/**
 * Odesila handshake a ceka na prijem potvrzeni.
 */
void handshake(void)
{
  s_act = 1;
  char recvline[MAXCHARS*2];
  char *packet = NULL;
  packet = createPacket(0, "rdtHANDSHAKE"); // vytvori handshake paket
  swindow[0] = packet; // na prvni pozici okna
  
  // spustit casovas, ktery kazdou 1s odesle handshake
  signal(SIGALRM, sigalrm_handler);
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGALRM);
  sigprocmask(SIG_UNBLOCK, &sigmask, NULL);
  setitimer(ITIMER_REAL, &itv, NULL);
  
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(udt, &readfds);
  
  while (select(udt+1, &readfds, NULL, NULL, NULL))
  {
    if (FD_ISSET(udt, &readfds))
    { // prisel paket
      int n = udt_recv(udt, recvline, MAXCHARS*2, NULL, NULL);
      recvline[n] = '\0'; // ukonceni zpravy
      
      int pkt_num = packetCheck(recvline, n);
      if (pkt_num != -1)
      { // paket vpohode
        if (pkt_num == 0 || pkt_num == 1)
          break;
      }
    }
  }
    
  sigprocmask(SIG_BLOCK, &sigmask, NULL); // vypnout casovac
  free(packet);
  s_act = 0;
}

/**
 * Odesle 5x endshake.
 */
void endshake(void)
{
  char *packet = createPacket(s_act, "rdtENDSHAKE"); // vytvori paket
  int i;
  for (i = 0; i < (SWINDOWSIZE/2); i++) // 5x ENDSHAKE pro pripad ztraty
  {
    udt_send(udt, remote_addr, dest_port, packet, strlen(packet));
  }
  free(packet);
}

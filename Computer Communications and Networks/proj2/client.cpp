/**
 * IPK - Projekt 2
 * @author David Krym
 * @file server.cpp
 */
 
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <string>
#include <cctype>
#include "error.h"

using namespace std;

// ladici vypisy
//#define DEBUGPRINT

//#define string std::string

enum Pcode
{ // navratovy kody
  POK = 5,
  PERR
};

// prototypy
Pcode createConn(const string &server, const string &port, const int argc, char **argv);
Pcode rcv(string &msg, const int &soc);
Pcode showParsed(const string &msg);

/**
 * Program.
 */
int main (int argc, char **argv)
{
  if (argc < 3)
  { // min parametru nez pozadovano
    printf("%s hostname:port psc [psc]\n", argv[0]);
    return EXIT_FAILURE;
  }

  size_t foundport;
  string server;
  string port;
  server.append(argv[1]); // server
  
  if ((foundport = server.find_first_of(':')) == string::npos)
  { // nenalezen port
    printErr(ESRV);
    return EXIT_FAILURE;
  }
  
  port.append(server.substr(foundport+1)); // bez dvojtecky, az do konce
  server.erase(foundport); // odmaze se port

  #ifdef DEBUGPRINT
    cout << "  server: " << server << endl;
    cout << "  port: " << port <<endl;
  #endif

  // v argv se predaji jen psc (tj bez jm. programu a zadaneho serveru)
  if (createConn(server, port, argc-2, argv+2) != POK)
    return EXIT_FAILURE;
  
  return EXIT_SUCCESS;
}



/**
 * Vytvari spojeni a nasloucha na portu.
 * @param server Retezec se zadanym serverem.
 * @param port Retezec se zadanym portem.
 * @param argc Pocet zadanych psc.
 * @param argv pole zadanych psc.
 */
Pcode createConn(const string &server, const string &port, const int argc, char **argv)
{
  istringstream istr(port);
  int port_num;
  istr >> port_num;
  if (port_num < 1 || port_num > 65535)
  { // chyba portu
    printErr(EPORT);
    return PERR;
  }

  struct sockaddr_in sin;
  struct hostent *hptr;
  string message;
  string response;
  int s; // deskriptor socketu
  
  sin.sin_family = PF_INET;  // ipv4 protokol family    
  sin.sin_port = htons(port_num);  // cislo portu
  
  // 100ms, kvuli spusteni zaroven se serverem, jinak haze chybu pripojeni
  usleep(1000);
  
  if ((hptr = gethostbyname(server.c_str())) == NULL)
  { // jmeno na ip chyba
    printErr(EHOSTBYNAME);
    return PERR;
  }
  
  if ((s = socket(PF_INET, SOCK_STREAM, 0 )) < 0)
  { /* create socket*/
    printErr(ESOCKET);
    return PERR;
  }
  
  memcpy(&sin.sin_addr, hptr->h_addr, hptr->h_length); // adresa
  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  { // pripojeni na socket chyba
    printErr(ECONN);
    close(s);
    return PERR;
  }

  message.append("G\n"); // generovani zpravy na poslani
  for (int i = 0; i < argc; i++)
  {
    message.append(argv[i]);
    message.push_back('\n');
  }
  message.push_back('\n');
  
  #ifdef DEBUGPRINT
    cout << "---msg to send:---" << message << "---msg---";
  #endif
  
  if (write(s, message.data(), (size_t)message.size()) < 0)
  {  // chyba pri posilani
    printErr(ESEND);
    close(s);
    return PERR;
  }
  
  if (rcv(response, s) != POK)
  {  // chyba pri prijmu
    close(s);
    return PERR;
  }
 
  #ifdef DEBUGPRINT
    cout << "---rcved msg:---" << response << "---msg---";
  #endif
 
  if (showParsed(response) != POK)
  {  // chyba v prijate zprave
    close(s);
    return PERR;
  }
 
  if (close(s) < 0)
  { // zavre socket
    printErr(ECLOSE);
    return PERR;
  }

  return POK;
}



/**
 * Prijima data ze zadaneho socketu.
 * @param msg Retezec pro ulozeni prijate zpravy.
 * @param soc Deskriptor socketu.
 */
Pcode rcv(string &msg, const int &soc)
{
  int cnter = 0; // prislo LF?
  char buf = 0; // prijaty znak
  
  while (cnter < 2)
  { // dokud neprijde LF-LF
    if (read(soc, &buf, (size_t)1) < 0)
    { // chyba pri cteni 
      printErr(ERCVD);
      return PERR;      
    }   
       
    if (buf == '\n')
    { // LF
        cnter++;
    }
    else // jakykoliv znak krom LF
      cnter = 0;

    if (cnter < 2)
      msg += buf;
  }
  
  return POK;
}



/**
 * Vypise odpoved od serveru.
 * @param msg Retezec s prichozim pozadavkem.
 */
Pcode showParsed(const string &msg)
{
  if (msg[0] != 'R' && msg[1] != '\n')
  {
    printErr(EMSGF);
    return PERR;
  }
  // psc cisla a jmena obci zacinaji od msg[2]
  
  string city;
  size_t semicolon;
  size_t start = 2;
  
  size_t foundline = msg.find_first_of('\n', start); // az treti znak zpravy
  
  while (foundline != string::npos)
  { // nasel se LF, mame radek ve tvaru: PSC;mesto\n
    city.clear();
    semicolon = msg.find_first_of(';', start);
    city.append(msg.substr(semicolon+1, foundline-(semicolon+1))); // vybere mesto
    
    if (city.size() == 0)
    { // mesto nenalezeno
      cerr << "Chyba: Nezname psc " << msg.substr(start, semicolon-start) << endl;
    }
    else
    { // nalezeno
      cout << city << endl;
    }
    
    start = foundline + 1; // start novyho radek
    foundline = msg.find_first_of('\n', start);
  }

  
  return POK;
}

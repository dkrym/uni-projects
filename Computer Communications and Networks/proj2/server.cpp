/**
 * IPK - Projekt 2
 * @author David Krym
 * @file server.cpp
 */
 
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <string>
#include <cctype>
#include <signal.h>
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
Pcode createConn(const string &port, const string &db_name);
int mainChild(const string &db, struct sockaddr_in sin, const int &soc);
Pcode parser(const string &db, const string &msg, string &response);
Pcode rcv(string &msg, const int &soc);



/**
 * Program.
 */
int main (int argc, char **argv)
{
  int ch;
  bool f_port = false;
  bool f_db = false;
  string port;
  string db;
  
  if (argc < 5)
  { // min parametru nez pozadovano
    printf("%s -p port -d database\n", argv[0]);
    return EXIT_FAILURE;
  }

  while ((ch = getopt(argc, argv, "p:d:")) != -1) 
  {
    switch (ch)
    { // uklada parametry
      case 'p':
          port = optarg;
          f_port = true;
          break;
      case 'd':
          db = optarg;
          f_db = true;
          break;
      case '?':
          printErr(EPARAM);
          return EXIT_FAILURE;
          break;    
      default:
          break;
    }
  }
  
  argc -= optind;
  argv += optind;
  
  if (!f_port || !f_db || argc > 0)
  { // nezadany povinne parametry
    printErr(EPARAM);
    return EXIT_FAILURE;
  }

  if (createConn(port, db) != POK)
    return EXIT_FAILURE;
  
  return EXIT_SUCCESS;
}



/**
 * Vytvari spojeni a nasloucha na portu.
 * @param port Retezec se zadanym portem.
 * @param db Retezec se jmenem souboru.
 */
Pcode createConn(const string &port, const string &db_name)
{
  istringstream istr(port);
  int port_num;
  istr >> port_num;
  if (port_num < 1 || port_num > 65535)
  { // chyba portu
    printErr(EPORT);
    return PERR;
  }
  
  string db("\n"); // obsah souboru
  string line; // jeden radek
  ifstream file(db_name.c_str());
  
  if (file.is_open())
  { // otevren
    while (file.good())
    { // lze cist
      getline(file, line);
      db.append(line);
      db.push_back('\n');
    }
    file.close();
    db.erase(db.end()-1);
  }
  else
  { // chyba pri otevreni
    printErr(EFILEO);
    return PERR;
  }
  
  int s, s2, pid;
  struct sockaddr_in sin;

  if ((s = socket(PF_INET, SOCK_STREAM, 0 )) < 0)
  { // chyba pri vytvoreni socketu
    printErr(ESOCKET);
    return PERR;
  }
  
  sin.sin_family = PF_INET; // ipv4
  sin.sin_port = htons(port_num);  // port
  sin.sin_addr.s_addr  = INADDR_ANY;
  size_t sin_len = sizeof(sin);
  
  if (bind(s, (struct sockaddr *)&sin, sin_len) < 0)
  { // chyba pri navazani na lokalni adresu
    printErr(EBIND);
    return PERR;
  }
  
  if (listen(s, 5) != 0)
  { // socket bude cekat na pripojeni
    printErr(ELISTEN);
    return PERR;
  }
  
  signal(SIGCHLD, SIG_IGN); // aby nevznikali zombie procesy
  while (1)
  { // cekani na klienty
    if ((s2 = accept(s, (struct sockaddr *)&sin, &sin_len)) < 0 )
    { // chyba pri vytvoreni socketu pro novyho klienta
      printErr(EACCEPT);
      return PERR;
    }
    
    pid = fork(); // vytvoreni potomka
    if (pid == 0)
    { // dite
      mainChild(db, sin, s2);
    }
    else if (pid == -1)
    { // CHYBA
      printErr(EFORK);
    }
  }
  
  if (close(s) < 0)
  {
    printErr(ECLOSE);
    return PERR;
  }
  
  return POK;
}


 
/**
 * Hlavni funkce potomka.
 * @param db  Retezec s psc a jmenama obci.
 * @param sin Struktura s informacema o vytvorenem socketu.
 * @param soc Descriptor socketu.
 */
int mainChild(const string &db, struct sockaddr_in sin, const int &soc)
{
  
  #ifdef DEBUGPRINT
    struct hostent * hp;
    hp = (struct hostent *)gethostbyaddr((char *)&sin.sin_addr, 4, AF_INET);
    printf("From %s (%s):%d.\n", inet_ntoa(sin.sin_addr), hp->h_name, ntohs(sin.sin_port));
  #endif
  
  string text; // prijata zprava
  string response; // zprava na odeslani
  
  if (rcv(text, soc) != POK)
  { // chyba pri prijmu zpravy
    close(soc);
    exit(EXIT_FAILURE);
  }
  
  #ifdef DEBUGPRINT
    cout << "---rcved msg:---" << text << "---msg---";
  #endif

  if (parser(db, text, response) != POK)
  { // chyba pri parsovani zpravy, spatny format
    close(soc);
    exit(EXIT_FAILURE);
  }  
  
  #ifdef DEBUGPRINT
    cout << "---msg to send:---" << response << "---msg---";
  #endif
  
  if (write(soc, response.data(), (size_t)response.size()) < 0)
  { // chyba pri posilani
    printErr(ESEND);
    close(soc);
    exit(EXIT_FAILURE);
  }
  
  if (close(soc) < 0)
  { // chyba pri zavirani socketu
    printErr(ECLOSE);
    exit(EXIT_FAILURE);
  } 
  
  #ifdef DEBUGPRINT
    sleep(10);
  #endif
  
  exit(EXIT_SUCCESS);
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
 * Vyhleda zadana psc a vytvori odpoved.
 * @param db Retezec obsahujici soubor s psc.
 * @param msg Retezec s prichozim pozadavkem.
 * @param response Retezec s odpovedi na pozadavek.
 */
Pcode parser(const string &db, const string &msg, string &response)
{
  if (msg[0] != 'G' && msg[1] != '\n')
  { // chybny format
    printErr(EMSGF);
    return PERR;
  }
  // psc cisla zacinaji od msg[2]
  
  response.append("R\n"); // znaci odpoved, nasledujou data
  
  string psc;
  size_t foundcity;
  size_t start = 2;
  size_t foundpsc = msg.find_first_of('\n', start); // az treti znak zpravy
  
  while (foundpsc != string::npos)
  { // nasel se LF
    psc.clear();
    psc.push_back('\n');
    psc.append(msg.substr(start, foundpsc-start)); // vybere psc
    psc.push_back(';'); // v psc je: PSC; (pro presnejsi vyhledavani)
    
    if ((foundcity = db.find(psc)) != string::npos)
    { // vyriznuti i ze znakem \n
      response.append(db.substr(foundcity+1, db.find_first_of('\n', foundcity+1)-(foundcity+1)+1));
    }
    else
    { // nebylo nalezeno
      psc.erase(psc.begin(),psc.begin()+1);
      response.append(psc);
      response.append("\n");
    }
    
    start = foundpsc + 1; // najde nova psc
    foundpsc = msg.find_first_of('\n', start);
  }
  // na konec se prida LF
  response.push_back('\n');
  
  return POK;
}

/**
 * IPK - Projekt 1
 * @author David Krym
 * @file webinfo.cpp
 */
 
#include "webinfo.h"

// ladici vypisy
//#define DEBUGPRINT

#define FLAGCNT 4 // pocet moznych parametru
#define string std::string
#define REDIRECT_NUM 5 // max pocet redirectu

enum Pcode
{ // navratovy kody
  POK = 5,
  PREDIRECT,
  PERR
};

struct Turl
{ // rozsekana url
  string host;
  string port;
  string path;
};


// prototypy funkci
Pcode createConn(string &urlparam, const int cnt_flag, const int flag[]);
void showParsed(string &header, const int cnt_flag, const int flag[]);
Pcode recieve(int s, string &text, const int cnt_flag, const int flag[]);
Pcode parseURL(const string &url_str, Turl &myurl, const int &redir_counter);
void unsafeChars(string &path);



/**
 * Program pro vypis HEAD informaci ze serveru
 */
int main (int argc, char **argv)
{
  int flag[FLAGCNT] = {0}; // poradi flagu
  int cnt_flag = 0;  // pocet zadanych flagu
  
  int ch ;

  if ((argc < 2) || (argc > (FLAGCNT+2)))
  { // spatny pocet parametru
    printErr(EPARAM);
    return EXIT_FAILURE;
  }
  
  while ((ch = getopt(argc, argv, "lsmt")) != -1) 
  {
    switch (ch)
    { // uklada parametry v presnem poradi
      case 'l':
      case 's':
      case 'm':
      case 't':
          if (cnt_flag < FLAGCNT)
            flag[cnt_flag++] = ch;
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

  if (argc != 1)
  { // spatny pocet parametru
    printErr(EPARAM);
    return EXIT_FAILURE;
  }

  string urlparam;
  urlparam.append(argv[0]); // url addresa

  if (createConn(urlparam, cnt_flag, flag) != POK)
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}



/**
 * Vytvari spojeni se serverem.
 * Prijima odpoved od serveru a v pripade presmerovani vytvori nove spojeni.
 * @param urlparam Retezec se zadanou URL adresou.
 * @param cnt_flag Pocet zadanych parametru.
 * @param flag Poradi zadanych parametru.
 */
Pcode createConn(string &urlparam, const int cnt_flag, const int flag[])
{
  using namespace std;
  struct sockaddr_in sin;
  struct hostent *hptr;
  string htext;
  int redir_counter = 1; // pocet redirectu 
  Turl myurl;
  int s; // deskriptor
  
  sin.sin_family = PF_INET;  // ipv4 protokol family
  
  while (redir_counter <= REDIRECT_NUM)
  { // zarizuje redirecty, pokud neni redirect, vyskoci z cyklu
    if (parseURL(urlparam, myurl, redir_counter) != POK)
      return PERR; // chybna url
    
    #ifdef DEBUGPRINT
      std::cout << "  --createConn: url parsed--"
                << "\n    Host: " << myurl.host
                << "\n    Path: " << myurl.path
                << "\n    Port: " << myurl.port << "\n  --createConn--\n";
    #endif
    
    sin.sin_port = htons(atoi(myurl.port.c_str()));  // cislo portu
    if ((hptr = gethostbyname(myurl.host.c_str())) == NULL)
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
    
    // hlavicka na poslani
    htext = "HEAD "+ myurl.path +" HTTP/1.1\r\nHost: "+ myurl.host +"\r\nConnection: close\r\n\r\n";
    
    if (write(s, htext.data(), (size_t)htext.size()) < 0)
    {  // chyba pri posilani
      printErr(ESEND);
      close(s);
      return PERR;
    }
    
    htext.clear();
    if (recieve(s, htext, cnt_flag, flag) == PREDIRECT) // prijem odpovedi
    { // presmerovani
      size_t found;
      string loc = "Location: "; // hledame novou adresu
      redir_counter++;
      urlparam.clear();
      if ((found = htext.find(loc)) != string::npos)
      { // vyriznuti adresy
        urlparam.append(htext.substr(found+loc.size(), htext.find_first_of('\n', found)-(found+loc.size())));
      }
      else
      { // nenasla se adresa pro presmerovani
        printErr(EREDADDR);
        close(s);
        return PERR;
      }
      
      if (close(s) < 0)
      { // zavre socket
        printErr(ECLOSE);
        return PERR;
      }
    }
    else // vse vporadku, bez presmerovani
    {
      if (close(s) < 0)
      { // zavre socket
        printErr(ECLOSE);
        return PERR;
      }
      break;
    }
      
  }
  
  return POK;
}



/**
 * Parsuje zadanou adresu a rozdeli ji na jednotlive casti.
 * @param url_str Retezec s URL adresou.
 * @param myurl Struktura pro URL: host, port, cesta.
 * @param redir_counter Pocitadlo presmerovani.
 */
Pcode parseURL(const string &url_str, Turl &myurl, const int &redir_counter)
{
  regmatch_t pmatch[8];
  regex_t re;
  string pattern = "^([^:/?#]+\\://)(([a-zA-Z0-9]+[a-zA-Z0-9\\-\\.]*\\.[a-zA-Z]{2,4})|([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}))(\\:([0-9]{1,5}))?(.+)?"; 
  //printf("str: %s\n", pattern);
  int status;
  
  myurl.host.clear();
  myurl.path.clear();
  myurl.port.clear();
  
  if(regcomp(&re, pattern.c_str(), REG_EXTENDED) != 0)
  { // kompilace regexpu
    printErr(EREGCOMP);
    return PERR;
  }
  
  status = regexec(&re, url_str.c_str(), 8, pmatch, 0);
  if (status == 0)
  { // neco se naslo
    int offsize;
    
    if (pmatch[1].rm_so != -1 && pmatch[1].rm_eo != -1)
    { // protokol
      string protocol;
      offsize = pmatch[1].rm_eo-pmatch[1].rm_so;
      protocol.append(url_str, pmatch[1].rm_so, offsize);
      if (protocol.compare("http://") != 0)
      { // spatnej protokol
        if (redir_counter == 1)
          printErr(EPROT);
        else
          printErr(EREDPROT);
        regfree(&re);
        return PERR;
      }
    }
    else
    { // nezadan protokol
      printErr(EPROT);
      regfree(&re);
      return PERR;
    }
    
    if (pmatch[2].rm_so != -1 && pmatch[2].rm_eo != -1)
    { // host
      offsize = pmatch[2].rm_eo-pmatch[2].rm_so;
      myurl.host.append(url_str, pmatch[2].rm_so, offsize);
    }
    else
    { // nezadan host
      printErr(EHOST);
      regfree(&re);
      return PERR;
    }
    
    if (pmatch[6].rm_so != -1 && pmatch[6].rm_eo != -1)
    { // port
      offsize = pmatch[6].rm_eo-pmatch[6].rm_so;
      myurl.port.append(url_str, pmatch[6].rm_so, offsize);
    }
    else // default 80
      myurl.port.append("80");
      
    if (pmatch[7].rm_so != -1 && pmatch[7].rm_eo != -1)
    { // cesta
      offsize = pmatch[7].rm_eo-pmatch[7].rm_so;
      if (url_str[pmatch[7].rm_so] != '/')
      { // chybne zadana cast za TLD
        printErr(EURL);
        regfree(&re);
        return PERR;
      }
      myurl.path.append(url_str, pmatch[7].rm_so, offsize);
      unsafeChars(myurl.path); // prevod unsafe znaku
    }
    else // default cesta je /
      myurl.path.append("/");
      
  }
  else
  {
    if (redir_counter == 1)
      printErr(EURL);
    else
      printErr(EREDURL);
      
    regfree(&re);
    return PERR;
  }  
  
  regfree(&re);
  
  return POK;
}



/**
 * Vytvari spojeni se serverem.
 * Prijima odpoved od serveru a v pripade presmerovani vytvori nove spojeni.
 * @param s Deskriptor socketu.
 * @param text Odpoved od serveru (hlavicka).
 * @param cnt_flag Pocet zadanych parametru.
 * @param flag Poradi zadanych parametru.
 */
Pcode recieve(int s, string &text, const int cnt_flag, const int flag[])
{ // prijem dat

  int cnter = 0; // prislo LF?
  char buf = 0; // prijaty znak
  string headcode; // prvni radek hlavicky bez HTTP/x.x
  //size_t found; 
  char numcode = 0; // typ http hlavicky 4xx,5xx atd
  
  while (cnter < 2)
  { // dokud neprijde CR-LF-CR-LF nebo LF-LF
    if (read(s, &buf, (size_t)1) < 0)
    { // chyba pri cteni 
      printErr(ERCVD);
      return PERR;      
    }   
       
    if (buf == '\n' || buf == '\r')
    { // CR-LF
      if (buf == '\n')
        cnter++;
    }
    else // jakykoliv znak krom CR-LF
      cnter = 0;

    if (buf != '\r' && cnter < 2)
      text += buf;
      
  }
    
  #ifdef DEBUGPRINT
    std::cout << "  --recieve: whole header--\n" << text << "\n  --recieve--\n";
  #endif
    
  // vezmeme jen prvni radek hlavicky, az za HTTP/x.x
  size_t httpcode = text.find_first_of(' ');
  headcode = text.substr(httpcode, text.find_first_of('\n')-httpcode);
  numcode = headcode[1]; // prvni cislo kodu
  
  if (numcode == '3') // 3xx redirect
    return PREDIRECT;
  else if (numcode == '4' || numcode == '5')
  { // zprava na strerr, vypsat hlavicku
    printErr(headcode);
    showParsed(text, cnt_flag, flag);
  }
  else // jinak je to asi ok, vypsat hlavicku dle parametru
    showParsed(text, cnt_flag, flag);

  return POK;
}



/**
 * Vypisuje pozadovane informace z hlavicky.
 * @param header Odpoved od serveru (hlavicka), ze ktere se budu vypisovat.
 * @param cnt_flag Pocet zadanych parametru.
 * @param flag Poradi zadanych parametru.
 */
void showParsed(string &header, const int cnt_flag, const int flag[])
{
  using namespace std;
  size_t found;
  
  #ifdef DEBUGPRINT
    std::cout << "  --showParsed: whole header--\n" << header << "\n  --showParsed--\n";
  #endif
  
  if (cnt_flag == 0)
  { // vypsat cely header
    cout << header;
    return;
  }
  
 // specifikovany parametry
  for (int i = 0; i < cnt_flag; i++)
  {
    if (flag[i] == 'l')
    {
      if ((found = header.find("Content-Length:")) != string::npos)
      { // vyriznuti
        cout << header.substr(found, header.find_first_of('\n', found)-found+1);
      }
      else
        cout << "Content-Length: N/A" << endl;
    }
    else if (flag[i] == 's')
    {
      if ((found = header.find("Server:")) != string::npos)
      { // vyriznuti
        cout << header.substr(found, header.find_first_of('\n', found)-found+1);
      }
      else
        cout << "Server: N/A" << endl;
    }
    else if (flag[i] == 'm')
    {
      if ((found = header.find("Last-Modified:")) != string::npos)
      { // vyriznuti
        cout << header.substr(found, header.find_first_of('\n', found)-found+1);
      }
      else
        cout << "Last-Modified: N/A" << endl;
    }
    else if (flag[i] == 't')
    {
      if ((found = header.find("Content-Type:")) != string::npos)
      { // vyriznuti
        cout << header.substr(found, header.find_first_of('\n', found)-found+1);
      }
      else
        cout << "Content-Type: N/A" << endl;
    }
  }
  
  return;
}



/**
 * Hleda a nahrazuje "unsafe" znaky z URL adresy.
 * @param path Cesta z URL adresy.
 */
void unsafeChars(string &path)
{
  string tmpstr; // novy string
  bool percent = false;
  bool hexadig = false;
  char safe[5]; // tmp pro novy safe znak
  size_t len = path.length(); // delka

  for (size_t i = 0; i < len; i++)
  { // pro kazdy znak
    hexadig = isxdigit(path[i]);
  
    if (path[i] == '%' && !percent)
    { // prislo prvni procento
      percent = true;
      continue;
    }
    else if (path[i] == '%' && percent)
    { // vic procent
      sprintf(safe, "%%%x", '%');
      tmpstr.append(safe);
      continue;
    }
    else if (percent && !hexadig) // neni to jiz prekodovany znak
    { // procento se bude prekodovavat
      percent = false;
      sprintf(safe, "%%%x", '%');
      tmpstr.append(safe);
    }
    else if (percent && hexadig)
    {
      percent = false;
      tmpstr.append("%");
    }
  
    switch (path[i])
    { // unsafe znaky
      case ' ':
      case ';':
      case '<':
      case '>':
      case '"':
      case '#':
      case '{':
      case '}':
      case '|':
      case '\\': 
      case '^':
      case '~':
      case '[':
      case ']':
      case '`':
        sprintf(safe, "%%%x", path[i]);
        tmpstr.append(safe);
        break;        
      default:
        tmpstr.push_back(path[i]);
    }
  }
  path.assign(tmpstr);
  
  return;
}

/**
 * IPK - Projekt 1
 * @author David Krym
 * @file error.cpp
 */

#include "error.h"


/** Chybova hlaseni. */
const char *EMSG[] =
{
  "Server byl zadan ve spatnem tvaru.",
  "Zadan spatny port. Zadejte cislo 1-65535.",
  "Chyba pri vytvareni socketu.",
  "Chyba pri prekladu url na ip.",
  "Chyba pripojeni.",
  "Chyba pri odeslani dat.",
  "Chyba pri prijmu dat.",
  "Chyba pri zavreni socketu.",
  "Chybny format prijate zpravy",
  
  "Chybne zadane parametry.",
  "Chyba pri otevreni souboru s PSC.",
  "Chyba pri bindu portu.",
  "Chyba pri naslouchani.",
  "Chyba pri vytvoreni socketu potomka.",
  "Chyba pri vytvoreni potomka."
  
  "Neznama chyba!"
};

using namespace std;  

/**
 * Tiskne hlaseni na standarni chybovy vystup.
 * @param errMsg Typ chyboveho hlaseni.
 */
void printErr(tErrorMessage errMsg)
{
  cerr << EMSG[errMsg] << endl;
}

void printErr(const string &errMsg)
{

  cerr << "Chyba:" << errMsg << endl;
}

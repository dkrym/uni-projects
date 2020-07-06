/**
 * IPK - Projekt 1
 * @author David Krym
 * @file error.cpp
 */

#include "error.h"


/** Chybova hlaseni. */
const char *EMSG[] =
{
  "Zadany spatne parametry: ./webinfo [-l -m -t -s] URL",
  "Chyba pri vytvareni socketu.",
  "Chyba pri prekladu url na ip.",
  "Chyba pripojeni.",
  "Chyba pri odeslani na server.",
  "Chyba pri prijmu od serveru.",
  "Chyba pri zavreni socketu.",
  "Chyba pri kompilaci regularniho vyrazu.",
  "Nebyl zadan http:// protokol", // EPROT
  "Nebyl spravne zadan host.",
  "Byla zadana nespravna url.",
  "Chyba pri presmerovani, nenalezena nova absolutni adresa.",
  "Pri presmerovani nebyl nalezen http:// protokol.", 
  "Chyba pri presmerovani.",
  
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

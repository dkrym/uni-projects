/**
 * IPK - Projekt 1
 * @author David Krym
 * @file error.h
 */

#ifndef __ERROR_H__
#define __ERROR_H__

#include <iostream>


/** Typy chybovych hlaseni. */
typedef enum tErrorMessage
{
  EPARAM, // spatny pocet parametru
  ESOCKET, // chyba pri vytvareni socketu
  EHOSTBYNAME, // chyba pri prekladu ip
  ECONN, // pripojeni
  ESEND, // poslani
  ERCVD, // prijmuti
  ECLOSE, // zavreni socketu
  EREGCOMP, // compilace regexpu
  EPROT, // spatny protokol
  EHOST, // nezadan host
  EURL, // spatna url
  EREDURL, // spatna adresa pri presmerovani
  EREDPROT, // chybny protokol pri redirectu
  EREDADDR, // nenasla se adresa pro presmerovani

  EUNDEFINED // unknow
} tErrorMessage;

/**
 * Tiskne hlaseni na standarni chybovy vystup.
 * @param errMsg Typ chyboveho hlaseni, pripadne retezec na vypis.
 */
void printErr(tErrorMessage errMsg);
void printErr(const std::string &errMsg);



#endif

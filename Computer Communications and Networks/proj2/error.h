/**
 * IPK - Projekt 2
 * @author David Krym
 * @file error.h
 */

#ifndef __ERROR_H__
#define __ERROR_H__

#include <iostream>


/** Typy chybovych hlaseni. */
typedef enum tErrorMessage
{
  ESRV, // server zadan ve saptnem tvaru
  EPORT, // port ve spatnem tvaru
  ESOCKET, // chyba pri vytvareni socketu
  EHOSTBYNAME, // chyba pri prekladu ip
  ECONN, // pripojeni
  ESEND, // poslani
  ERCVD, // prijmuti
  ECLOSE, // zavreni socketu
  EMSGF, // chybny format prijaty zpravy

  EPARAM, // chyba parametru
  EFILEO, // chyba pri otevreni souboru
  EBIND, // chyba pri bindu
  ELISTEN, // chyba pri listen
  EACCEPT, // chyba pri accept
  EFORK, // chyba pri fork

  EUNDEFINED // unknow
} tErrorMessage;

/**
 * Tiskne hlaseni na standarni chybovy vystup.
 * @param errMsg Typ chyboveho hlaseni, pripadne retezec na vypis.
 */
void printErr(tErrorMessage errMsg);
void printErr(const std::string &errMsg);



#endif

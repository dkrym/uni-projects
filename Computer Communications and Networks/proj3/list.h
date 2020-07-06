/**
 * Projekt IPK3
 * Autor: David Krym
 * Datum: 22.4.2011
 * 
 * Obusmerne vazany seznam, prevzato z meho projektu do predmetu IAL. 
*/


/* Předmět: Algoritmy (IAL) - FIT VUT v Brně
 * Hlavičkový soubor pro c206.c (Dvousměrně vázaný lineární seznam)
 * Vytvořil: Martin Tuček, září 2005
 * Upravil: Bohuslav Křena, říjen 2008
 *  
 *  Tento soubor, prosíme, neupravujte!  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FALSE 0
#define TRUE 1
#define MAXCHARS 80

extern int errflg;
extern int solved;
 
typedef struct tDLElem {                /* prvek dvouosměrně vázaného seznamu */ 
        int num;
        char data[MAXCHARS+10];                               /* užitečná data */
        struct tDLElem *lptr;          /* ukazatel na předchozí prvek seznamu */
        struct tDLElem *rptr;        /* ukazatel na následující prvek seznamu */
} *tDLElemPtr;

typedef struct {                                  /* dvousměrně vázaný seznam */
    tDLElemPtr First;                      /* ukazatel na první prvek seznamu */
    tDLElemPtr Act;                     /* ukazatel na aktuální prvek seznamu */
    tDLElemPtr Last;                    /* ukazatel na posledni prvek seznamu */
} tDLList;

                                             /* prototypy jednotlivých funkcí */

void DLDisposeList (tDLList *);
void DLInitList (tDLList *);
void DLInsertLast(tDLList *, int, char *);
void DLCopy (tDLList *, int *, char **);
void DLSucc (tDLList *);
void DLPreDelete (tDLList *);
void DLFirst (tDLList *);
int DLActive (tDLList *);
void DLDeleteLast (tDLList *);


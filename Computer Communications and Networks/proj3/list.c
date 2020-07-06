/**
 * Projekt IPK3
 * Autor: David Krym
 * Datum: 22.4.2011
 * 
 * Obusmerne vazany seznam, prevzato z meho projektu do predmetu IAL. 
*/

  
/* c206.c **********************************************************}
{* Téma: Dvousměrně vázaný lineární seznam
**
**                   Návrh a referenční implementace: Bohuslav Křena, říjen 2001
**                            Přepracované do jazyka C: Martin Tuček, říjen 2004
**                                            Úpravy: Bohuslav Křena, říjen 2010
**
** Implementujte abstraktní datový typ dvousměrně vázaný lineární seznam.
** Užitečným obsahem prvku seznamu je hodnota typu int.
** Seznam bude jako datová abstrakce reprezentován proměnnou
** typu tDLList (DL znamená Double-Linked a slouží pro odlišení
** jmen konstant, typů a funkcí od jmen u jednosměrně vázaného lineárního
** seznamu). Definici konstant a typů naleznete v hlavičkovém souboru c206.h.
**
** Vaším úkolem je implementovat následující operace, které spolu
** s výše uvedenou datovou částí abstrakce tvoří abstraktní datový typ
** obousměrně vázaný lineární seznam:
**
**      DLInitList ...... inicializace seznamu před prvním použitím,
**      DLDisposeList ... zrušení všech prvků seznamu,
**      DLInsertFirst ... vložení prvku na začátek seznamu,
**      DLInsertLast .... vložení prvku na konec seznamu, 
**      DLFirst ......... nastavení aktivity na první prvek,
**      DLLast .......... nastavení aktivity na poslední prvek, 
**      DLCopyFirst ..... vrací hodnotu prvního prvku,
**      DLCopyLast ...... vrací hodnotu posledního prvku, 
**      DLDeleteFirst ... zruší první prvek seznamu,
**      DLDeleteLast .... zruší poslední prvek seznamu, 
**      DLPostDelete .... ruší prvek za aktivním prvkem,
**      DLPreDelete ..... ruší prvek před aktivním prvkem, 
**      DLPostInsert .... vloží nový prvek za aktivní prvek seznamu,
**      DLPreInsert ..... vloží nový prvek před aktivní prvek seznamu,
**      DLCopy .......... vrací hodnotu aktivního prvku,
**      DLActualize ..... přepíše obsah aktivního prvku novou hodnotou,
**      DLSucc .......... posune aktivitu na další prvek seznamu,
**      DLPred .......... posune aktivitu na předchozí prvek seznamu, 
**      DLActive ........ zjišťuje aktivitu seznamu.
**
** Při implementaci jednotlivých funkcí nevolejte žádnou z funkcí
** implementovaných v rámci tohoto příkladu, není-li u funkce
** explicitně uvedeno něco jiného.
**
** Nemusíte ošetřovat situaci, kdy místo legálního ukazatele na seznam 
** předá někdo jako parametr hodnotu NULL.
**
** Svou implementaci vhodně komentujte!
**
** Terminologická poznámka: Jazyk C nepoužívá pojem procedura.
** Proto zde používáme pojem funkce i pro operace, které by byly
** v algoritmickém jazyce Pascalovského typu implemenovány jako
** procedury (v jazyce C procedurám odpovídají funkce vracející typ void).
**/

#include "list.h"

int solved;
int errflg;

void DLError() {
/*
** Vytiskne upozornění na to, že došlo k chybě.
** Tato funkce bude volána z některých dále implementovaných operací.
**/	
    fprintf (stderr, "*ERROR* Chyba při práci se seznamem.\n");
    errflg = TRUE;             /* globální proměnná -- příznak ošetření chyby */
    return;
}

void DLInitList (tDLList *L)	{
/*
** Provede inicializaci seznamu L před jeho prvním použitím (tzn. žádná
** z následujících funkcí nebude volána nad neinicializovaným seznamem).
** Tato inicializace se nikdy nebude provádět nad již inicializovaným
** seznamem, a proto tuto možnost neošetřujte. Vždy předpokládejte,
** že neinicializované proměnné mají nedefinovanou hodnotu.
**/
    
  L->First = NULL;
  L->Act = NULL;
  L->Last = NULL;
}

void DLDisposeList (tDLList *L)	{
/*
** Zruší všechny prvky seznamu L a uvede seznam do stavu, v jakém
** se nacházel po inicializaci. Rušené prvky seznamu budou korektně
** uvolněny voláním operace free. 
**/
	
  tDLElemPtr pitem = NULL;
  while (L->First != NULL) { // procházení v jednom směru
    pitem = L->First->rptr; // následující prvek
    free(L->First);
    L->First = pitem;
  }
  L->Last = NULL;
  L->Act = NULL;
}


void DLInsertLast(tDLList *L, int num, char *data)	{
/*
** Vloží nový prvek na konec seznamu L (symetrická operace k DLInsertFirst).
** V případě, že není dostatek paměti pro nový prvek při operaci malloc,
** volá funkci DLError().
**/ 	
	
  tDLElemPtr pitem = NULL;
  pitem = malloc(sizeof(struct tDLElem));
  if (pitem == NULL) DLError();
  else {
    pitem->num = num;
    pitem->data[0] = '\0';
    strcat(pitem->data, data);
    pitem->lptr = L->Last; // vlevo na bývalý poslední
    pitem->rptr = NULL; // vpravo
    
    if (L->Last != NULL) L->Last->rptr = pitem; // navázání
    else L->First = pitem; // jediný prvek v seznamu
    
    L->Last = pitem;
  }
}

void DLFirst (tDLList *L)	{
/*
** Nastaví aktivitu na první prvek seznamu L.
** Funkci implementujte jako jediný příkaz (nepočítáme-li return),
** aniž byste testovali, zda je seznam L prázdný.
**/
	
  L->Act = L->First;
}


void DLPreDelete (tDLList *L)	{
/*
** Zruší prvek před aktivním prvkem seznamu L .
** Pokud je seznam L neaktivní nebo pokud je aktivní prvek
** prvním prvkem seznamu, nic se neděje.
**/
	
  if (L->Act != NULL && L->Act != L->First) {
    tDLElemPtr pitem = NULL;
    pitem = L->Act->lptr; // rušený prvek
    L->Act->lptr = pitem->lptr; // překlenutí
    
    if (pitem == L->First) L->First = L->Act; // nový první prvek
    else pitem->lptr->rptr = L->Act; // běžný rušený prvek, navázání
    
    free(pitem);
  }
}


void DLDeleteLast (tDLList *L)	{
/*
** Zruší poslední prvek seznamu L. Pokud byl poslední prvek aktivní,
** aktivita seznamu se ztrácí. Pokud byl seznam L prázdný, nic se neděje.
**/ 
	
  if (L->Last != NULL || L->First != NULL) { // neprázdný seznam
    if (L->Act == L->Last) L->Act = NULL; // ztracení aktivity
    
    tDLElemPtr pitem = NULL;
    pitem = L->Last;
    
    if (L->Last == L->First) { // jediný prvek, seznam bude prázdný
      L->First = NULL;
      L->Last = NULL;
    } 
    else { // alespoň 2 prvky
      L->Last = pitem->lptr; // předposlední prvek je posledním
      L->Last->rptr = NULL;
    }
    
    free(pitem);
  }
}


void DLCopy (tDLList *L, int *num, char **data)	{
/*
** Vrátí hodnotu aktivního prvku seznamu L.
** Pokud seznam L není aktivní, volá funkci DLError ().
**/
		
  if (L->Act == NULL) DLError();
  else {
    *num = L->Act->num;
    *data = L->Act->data;
  }
}


void DLSucc (tDLList *L)	{
/*
** Posune aktivitu na následující prvek seznamu L.
** Není-li seznam aktivní, nedělá nic.
** Všimněte si, že při aktivitě na posledním prvku se seznam stane neaktivním.
**/
	
  if (L->Act != NULL) {
    tDLElemPtr pitem = NULL;
    pitem = L->Act->rptr; // ukazatel na následující prvek
    L->Act = pitem; // aktivita na předchozí prvek
  }
}


int DLActive (tDLList *L) {		
/*
** Je-li seznam aktivní, vrací true. V opačném případě vrací false.
** Funkci implementujte jako jediný příkaz.
**/
	
  return (L->Act != NULL ? TRUE : FALSE);
}

/* Konec c206.c*/

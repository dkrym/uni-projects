/*
 * Soubor:  proj4.c
 * Datum:   2009/12/18
 * Autor:   David Krym
 * Projekt: �esk� �azen�, projekt �. 4 pro p�edm�t IZP
 * Popis:   Vzestupn� nebo sestupn� se�ad� slovn�kov� �daje ze vstupn�ho CSV souboru.
 *          O�ek�v� vstup ve tvaru: �esky;anglicky;druh
 *          Data zap�e do v�stupn�ho CSV souboru.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
// maxim�ln� d�lka �et�zce +1 (tzn zde 50 znak�)
#define WLENGTH 51 

// primary table
const unsigned char pTABLE[] =
{
  [' ']=1,  ['a']=2,  ['b']=3,  ['c']=4,  ['d']=6,  ['e']=7,
  ['f']=8,  ['g']=9,  ['h']=10, /*ch=11*/ ['i']=12, ['j']=13,
  ['k']=14, ['l']=15, ['m']=16, ['n']=17, ['o']=18, ['p']=19,
  ['q']=20, ['r']=21, ['s']=23, ['t']=25, ['u']=26, ['v']=27,
  ['w']=28, ['x']=29, ['y']=30, ['z']=31, ['.']=33, ['-']=34,
  
  [(unsigned char) '�']=2,  [(unsigned char) '�']=5,  [(unsigned char) '�']=6,
  [(unsigned char) '�']=7,  [(unsigned char) '�']=7,  [(unsigned char) '�']=12,
  [(unsigned char) '�']=17, [(unsigned char) '�']=18, [(unsigned char) '�']=22,
  [(unsigned char) '�']=24, [(unsigned char) '�']=25, [(unsigned char) '�']=26,
  [(unsigned char) '�']=26, [(unsigned char) '�']=30, [(unsigned char) '�']=32,
  
  [(unsigned char) '�']=2,  [(unsigned char) '�']=5,  [(unsigned char) '�']=6,
  [(unsigned char) '�']=7,  [(unsigned char) '�']=7,  [(unsigned char) '�']=12,
  [(unsigned char) '�']=17, [(unsigned char) '�']=18, [(unsigned char) '�']=22,
  [(unsigned char) '�']=24, [(unsigned char) '�']=25, [(unsigned char) '�']=26,
  [(unsigned char) '�']=26, [(unsigned char) '�']=30, [(unsigned char) '�']=32,
};

// secondary table
const unsigned char sTABLE[] =
{
  [(unsigned char) '�']=1,  [(unsigned char) '�']=1,  [(unsigned char) '�']=1,
  [(unsigned char) '�']=2,  [(unsigned char) '�']=1,  [(unsigned char) '�']=1,
  [(unsigned char) '�']=1,  [(unsigned char) '�']=1,  [(unsigned char) '�']=1,
  [(unsigned char) '�']=2,  [(unsigned char) '�']=1, 
  
  [(unsigned char) '�']=1,  [(unsigned char) '�']=1,  [(unsigned char) '�']=1,
  [(unsigned char) '�']=2,  [(unsigned char) '�']=1,  [(unsigned char) '�']=1,
  [(unsigned char) '�']=1,  [(unsigned char) '�']=1,  [(unsigned char) '�']=1,
  [(unsigned char) '�']=2,  [(unsigned char) '�']=1, 
};


/**
 * Struktura obsahuj�c� hodnoty otev�en�ho souboru.
 */
typedef struct tfile
{
  FILE *pfile;           /**< Ukazatel na otev�en� soubor. */
  int ecode;             /**< Chybov� k�d programu, odpov�d� v��tu tecodes. */
} TFile;


/**
 * Struktura obsahuj�c� analyzovan� ��dek souboru (slovn�ku)
 */
typedef struct tline
{
  char czech[WLENGTH];      /**< �esk� slovo. */
  char english[WLENGTH];    /**< Anglick� slovo. */
  int wclass;               /**< ��slo slovn�ho druhu. */
} TLine;


/**
 * Struktura obsahuj�c� slovn� druh a jeho po�adov� ��slo
 */
typedef struct twclass
{
  char *name;     /**< Jm�no slovn�ho druhu. */
  int number;     /**< ��slo slovn�ho druhu. */
} TWClass;


/**
 * Struktura obsahuj�c� hodnoty analyzovan�ho kl��e.
 */
typedef struct tkey
{
  char key[4];    /**< Slo�en� kl��, max 3 znaky. */
  char ord;       /**< Zda �adit vzestupn�/sestupn�. */
} TKey;


/**
 * Struktura obsahuj�c� hodnoty parametr� p��kazov� ��dky.
 */
typedef struct params
{
  TKey mykey;            /**< Analyzovan� slo�en� kl��. */
  char *fname1;          /**< Jm�no vstupn�ho souboru. */
  char *fname2;          /**< Jm�no v�stupn�ho souboru. */
  int ecode;             /**< Chybov� k�d programu, odpov�d� v��tu tecodes. */
  int state;             /**< Stavov� k�d programu, odpov�d� v��tu tstates. */
} TParams;


/**
 * Pole struktur TWClass, obsahuje slovn� druhy s jejich po�ad�m.
 */
const TWClass items[] =
{
  {"PODSTATN� JM�NO", 1},
  {"P��DAVN� JM�NO", 2},
  {"Z�JMENO", 3},
  {"��SLOVKA", 4},
  {"SLOVESO", 5},
  {"P��SLOVCE", 6},
  {"P�EDLO�KA", 7},
  {"SPOJKA", 8},
  {"��STICE", 9},
  {"CITOSLOVCE", 10},
  {"FR�ZE", 11},
};


/** K�dy chyb programu */
enum tecodes
{
  EOK = 0,     /**< Bez chyby. */
  ECLWRONG,    /**< Chybn� parametry. */
  EOPEN,       /**< Chyba p�i otev�r�n� souboru.*/
  EWRITE,      /**< Chyba p�i zapisov�n� souboru. */
  ECLOSE,      /**< Chyba p�i zav�r�n� souboru. */
  EBADFILE,    /**< Soubor neodpov�d� zad�n�. */
  EBADLINE,    /**< ��dka v souboru m� chybn� form�t. */
  EALLOC,      /**< Chyba p�i alokaci. */
  EBADKEY,     /**< �patn� zadan� kl��. */
  EWCLASS,     /**< Chybn� zadan� slovn� druh. */
  EBADCH,      /**< Chybn� znak v souboru. */
  EUNKNOWN,    /**< Nezn�m� chyba. */
};


/** Chybov� hl�en� odpov�daj�c� chybov�m k�d�m tencodes. */
const char *ECODEMSG[] =
{
  /* EOK */
  "Vse v poradku.\n",
  /* ECLWRONG */
  "Chybne parametry spusteni!\n",
  /* EOPEN */
  "Nepodarilo se otevrit soubor!\n",
  /* EWRITE */
  "Nepodarilo se zapsat do souboru!\n",
  /* ECLOSE */
  "Nepodarilo se zavrit soubor!\n",
  /* EBADFILE */
  "Chybny format souboru!\n",
  /* EBADLINE */
  "Nektera radka souboru neodpovida formatu: cesky;anglicky;druh !\n"
  "Pokud chcete nektery retezec vynechat, nahradte ho mezerou! (nelze cesky;;druh)\n",
  /* EALLOC */
  "Nepodarilo se alokovat pamet!\n",
  /* EBADKEY */
  "Byl zadan nespravny klic!\n",
  /* EWCLASS */
  "Chybn� slovni druh v souboru!\n",
  /* EBADCH */
  "Soubor obsahuje nedovolen� znaky!\n",
  
  "Nastala nepredvidatelna chyba!\n",
};


/** Stavov� k�dy programu */
enum tstates
{
  CHELP,       /**< N�pov�da */
  CSORT,       /**< Norm�ln� �azen�. */
};


///////// prototypy funkc� ////////////////////////////////////////
int load(char *infile, char *outfile, TKey *mykey);
int CSVtoFile(TLine *buffer, int row, int *order, char *outfile);
int tryClose(FILE *closefile);
int getWClass(char *wclass);
int cmpWords(TLine *first, TLine *second, TKey *mkey);
void isort(TLine *buffer, int rows, int *order, TKey *mykey);
int getKey(char *key, TKey *mykey);
int cmpCzech(char *first, char *second);
TParams getParams(int argc, char *argv[]);
void printECode(int ecode);
void printHelp(void);
///////////////////////////////////////////////////////////////////

/**
 * Hlavn� program.
 */
int main(int argc, char *argv[])
{
  TParams params = getParams(argc, argv);

  if (params.ecode != EOK)
  { // n�co nestandardn�ho
    printECode(params.ecode);
    return EXIT_FAILURE;
  }
  
  if (params.state == CHELP)
  { // vypsat n�pov�du, ukon�it program
    printHelp();
  }
  else
  { // jm�no prvn�ho souboru, druh�ho, kl��
    int ecode = load(params.fname1, params.fname2, &params.mykey);
    // vrac�m chybov� k�d
    if (ecode != EOK)
    { // n�co nestandardn�ho
      printECode(ecode);
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}


/**
 * Zpracuje argumenty p��kazov�ho ��dku a vr�t� je ve struktu�e TParams.
 * Pokud je form�t argument� chybn�, ukon�� program s chybov�m k�dem.
 * @param argc Po�et argument�.
 * @param argv Pole textov�ch �et�zc� s argumenty.
 * @return Vrac� analyzovan� argumenty p��kazov�ho ��dku.
 */
TParams getParams(int argc, char *argv[])
{
  TParams result =
  { // inicializace struktury
    .fname1 = NULL,
    .fname2 = NULL,
    .ecode = EOK,
    .state = CHELP,
  };

  if (argc == 2 && strcmp("-h", argv[1]) == 0)
  { // tisk n�pov�dy
    result.state = CHELP;
  }
  else if (argc == 4)
  { // �ty�i params, proj4 --key in.txt out.txt
    result.fname1 = argv[2];
    result.fname2 = argv[3];
    int ecode = getKey(argv[1], &result.mykey);
    if (ecode != EOK)
    { // chyba p�i spracov�n� kl��e
      result.ecode = ecode;
    }
    result.state = CSORT; // bude se �adit
  }
  else
  { // p��li� mnoho parametr�
    result.ecode = ECLWRONG;
  }

  return result;
} // konec funkce getParams


/**
 * Vytiskne hl�en� odpov�daj�c� chybov�mu k�du.
 * @param code K�d chyby programu
 */
void printECode(int ecode)
{
  if (ecode < EOK || ecode > EUNKNOWN)
  {
    ecode = EUNKNOWN;
  }
  fprintf(stderr, "%s", ECODEMSG[ecode]);
} // konec funkce printECode


 /**
 * Zkus� otev��t zadan� soubor v zadan�m m�du.
 * @param name Jm�no otev�ran�ho souboru.
 * @param mod M�d, ve kter�m chceme soubor otev��t.
 * @return Vrac� strukturu s ukazatelem na soubor a chybov�m k�dem.
 */
TFile tryOpen(char *name, char *mod)
{
  TFile openfile =
  {
    .pfile = NULL,
    .ecode = EOK,
  };
  // pokud o otev�en�
  FILE *f = fopen(name, mod);
  
  if (f == NULL)
  {
    openfile.ecode = EOPEN;
  }
  else
  {
    openfile.pfile = f;
  }
  
  return openfile;
} // konec funkce tryOpen


/**
* Hlavn� funkce, napln� pole souborem a vol� funkce na �azen� a ulo�rn� v�stupu.
* @param infile Jm�no vstupn�ho souboru.
* @param outfile Jm�no v�stupn�ho souboru.
* @param mykey Struktura se slo�en�m kl��em a typem �azen� (sest/vzest).
* @return Vrac� chybov� k�d.
*/
int load(char *infile, char *outfile, TKey *mykey)
{
  // pokus o otev�en� souboru
  TFile fr = tryOpen(infile,"r");
  if (fr.ecode != EOK)
  { // nepodarilo se otevrit
    return fr.ecode;
  }
///////////////////////// na��t�n�
  const int BLOCK_INCREMENT = 32;
  TLine *buf = NULL;
  int size = 0; // velikost bufferu, n�sobky BLOCK_INCREMENT
  int row = 0;  // po�et skute�n� na�ten�ch ��dk�
  int c = 0, cprev = 0; // na�ten� znak,p�edchoz� na�ten� znak
  int c_cnt = 0; // po�et znak� p�ed st�edn�kem
  int semicolon = 0; // po�et st�edn�k�
  char wclass[WLENGTH] = "\0"; // slovn� druh
  
  while ((c = fgetc(fr.pfile))!= EOF)
  { // �te soubor
    if (row == size)
    { // jsem za hranic�, nafouknu pole
      size += BLOCK_INCREMENT;
      TLine *rebuf = realloc(buf, size*sizeof(TLine));
      if (rebuf == NULL)
      { // nen� m�sto, uklid�m
        free(buf);
        buf = NULL;
        size = 0;
        tryClose(fr.pfile);
        return EALLOC;
      } // vpo��dku naalokov�no
      buf = rebuf;
    }
   
    // spracov�n� ��dk�
    if (c == ';')
    { // kolik�tej st�edn�k na ��dku
      if (cprev == c)
      { // pokud aktu�ln� i p�edchoz� znak je ;, chyba
        free(buf);
        tryClose(fr.pfile);
        return EBADLINE;
      }
      semicolon++;
      c_cnt = 0; // po��tadlo jede od znova
      cprev = c; // ulo�im si znak jako p�edchoz� znak
      continue;
    }
    else if (c == '\n')
    { // nov� ��dek
      if (c == cprev)
      { // pokud je v�c pr�zdn�ch ��dk� 
        continue;
      }
      wclass[c_cnt] = '\0';
      buf[row].wclass = getWClass(wclass); // p�evedeme slovn� druh na ��slo
      if (buf[row].wclass == 0)
      { // chybn� slovn� druh
        free(buf);
        tryClose(fr.pfile);
        return EWCLASS;
      }
      c_cnt = 0;
      semicolon = 0;
      row++;
      cprev = c; // ulo�im si znak jako p�edchoz� znak
      continue;
    }
    
    if (pTABLE[(unsigned char) tolower(c)] == 0)
    { // jestli obsahuje jen povolen� znaky
      free(buf);
      tryClose(fr.pfile);
      return EBADCH;
    }
    
    
    // podle po�tu ; na ��dku se na��t� dan� �et�zec
    if (semicolon == 0)
    { // �esk� slovo
      buf[row].czech[c_cnt] = c;
      buf[row].czech[c_cnt+1] = '\0';
      c_cnt++;
    }
    else if (semicolon == 1)
    { // anglick� slovo
      buf[row].english[c_cnt] = c;
      buf[row].english[c_cnt+1] = '\0';
      c_cnt++;
    }
    else if (semicolon == 2)
    { // slovn� druh
      wclass[c_cnt] = c;
      c_cnt++;
    }
    cprev = c; // ulo�im si znak jako p�edchoz� znak
    
    if (c_cnt > WLENGTH-1)
    { // vic jak 50 znak�, p�esko�it
      continue;
    }
  }
  tryClose(fr.pfile); // zav��t soubor
    
  if (cprev == '\n' && semicolon == 2)
  { // pokud neni soubor zakon�en nov�m ��dkem
    wclass[c_cnt] = '\0';
    buf[row].wclass = getWClass(wclass); // slovn� druh na ��slo
    if (buf[row].wclass == 0)
    { // �patn� slovn� druh, konec
      free(buf);
      buf = NULL;
      size = 0;
      return EWCLASS;
    }
    c_cnt = 0;
    row++;
  }
///////////////////////// konec na��t�n�

  // alokace index�, kter� budu �adit
  int *order = malloc(row * sizeof(int));
  if (order == NULL)
  { // buffer uvolnit
    free(buf);
    return EALLOC;
  }
  for (int i = 0; i < row; i++)
  { // napln�n� pole ��slama 0..row-1
    order[i] = i;
  }

  // �azen� index�
  isort(buf, row, order, mykey);
  // ulo�en� v�stupn�ho souboru
  int ecode = CSVtoFile(buf, row, order, outfile);
  free(buf);
  free(order);
  // chyba p�i z�pisu do souboru
  if (ecode != EOK)
  {
    return ecode;
  }
  else
  {
    return EOK;
  }
} // konec funkce load


/**
* Se�ad� indexy pole dle zadan�ho kl��e.
* @param buffer Ukazatel na za��tek pole struktur.
* @param row ��slo posledn�ho ��dku.
* @param order Pole s indexama po�ad�.
* @param mykey Struktura se slo�en�m kl��em a typem �azen� (sest/vzest).
* @return Vrac� chybov� k�d.
*/
void isort(TLine *buffer, int rows, int *order, TKey *mykey)
{
  if (mykey->ord == '+')
  { // vzestupne
    for (int i = 0; i < rows; i++)
    {
      int x = order[i]; // po�adov� ��slo aktu�ln�ho ��dku
      int j = i;  // pomocn�
                        // pokud prvni je mensi nez druhej
      while ((j > 0) && (cmpWords(&buffer[x], &buffer[order[j - 1]], mykey) < 0))
      { // posuv prvk� o jedni�ku doprava (nach�zen� pozice, kam vlo�it aktu�ln� index)
        order[j] = order[j - 1];  // p�edchoz� index na aktu�ln�
        j--;
      }
      order[j] = x;
    }
  }
  else
  { // sestupn�
    for (int i = 0; i < rows; i++)
    {
      int x = order[i]; // po�adov� ��slo aktu�ln�ho ��dku
      int j = i;  // pomocn�
                        // pokud prvni je mensi nez druhej
      while ((j > 0) && (cmpWords(&buffer[x], &buffer[order[j - 1]], mykey) > 0))
      { // posuv prvk� o jedni�ku doprava
        order[j] = order[j - 1];  // p�edchoz� na aktu�ln�
        j--;
      }
      order[j] = x;
    }
  }
} // konec funkce iSort


/**
* Porovn�v� slova dle zadan�ho kl��e.
* @param first Struktura s jedn�m ��dkem slovn�ku.
* @param second Struktura s druh�m ��dkem slovn�ku.
* @param mkey Slo�en� kl�� na �azen�.
* @return Vrac� -1 pokud je prvn� slovo men��,
                 1 pokud je druh� slovo men��,
                 0 pokud jsou slova stejn�.
*/
int cmpWords(TLine *first, TLine *second, TKey *mykey)
{ 
  int keylen = strlen(mykey->key); // d�lka kl��e
  int res = 0; // result
  
  for (int i = 0; i < keylen; i++)
  { // pro ka�d� ��slo z keye
    if (mykey->key[i] == '1')
    { // �azen� podle �esk�ho slova
      res = cmpCzech(first->czech, second->czech);
      if (res == 0)
      { // pokud se �et�zce rovnaj�
        continue;
      }
      // pokud se nerovnaj�, m�me v�sledek a vysko��me z cyklu
      break;
    }
    else if (mykey->key[i] == '2')
    { // �azen� podle anglick�ho slova
      res = strcmp(first->english, second->english);
      if (res == 0)
      { // pokud se �et�zce rovnaj�
        continue;
      }
      // pokud se nerovnaj�, m�me v�sledek a vysko��me z cyklu
      break;
    }
    else if (mykey->key[i] == '3')
    { // �azen� podle slovn�ho druhu
      if (first->wclass == second->wclass)
      {
        res = 0;
        continue;
      }
      // pokud prvn� druh men�� tak -1, jinak 1
      res = (first->wclass < second->wclass) ? -1 : 1;
      break;
    }
  }
  // vrac� -1,0,1
  return res;
} // konec funkce cmpWords


/**
* Vytiskne CSV ze zadan�ho pole.
* @param buffer Ukazatel na za��tek pole struktur.
* @param row ��slo posledn�ho ��dku.
* @param order Pole s indexama po�ad�.
* @param outfile Jm�no v�stupn�ho souboru.
* @return Vrac� chybov� k�d.
*/
int CSVtoFile(TLine *buffer, int row, int *order, char *outfile)
{
  TFile fw = tryOpen(outfile,"w"); // pro z�pis
  if (fw.ecode != EOK)
  { // nepodarilo se otevrit
    return fw.ecode;
  }
  
  char *wclass = '\0';
  for (int i = 0; i < row; i++)
  { // p�evod slovn�ho druhu z ��sla na �et�zec
    for (int y = 0; y < 11; y++)
    { // 11 slovn�ch druh�
      if (buffer[order[i]].wclass == items[y].number)
      {
        wclass = items[y].name;
        break;
      }
    }
    // z�pis do souboru v CSV form�tu
    fprintf(fw.pfile, "%s;%s;%s\n", buffer[order[i]].czech, buffer[order[i]].english, wclass);
  }
  tryClose(fw.pfile);
  
  return EOK;
} // konec funkce CSVtoFile


/**
* Porovn�v� lexikograficky dv� �esk� slova.
* @param first Prvn� �esk� slovo (�et�zec).
* @param second Druh� �esk� slovo.
* @return Vrac� -1 pokud je prvn� slovo men��,
                 1 pokud je druh� slovo men��,
                 0 pokud jsou slova stejn�.
*/
int cmpCzech(char *first, char *second)
{
  char str1[WLENGTH]; // na ulo�en� "hodnot" ��sel
  char str2[WLENGTH];
  
  unsigned char i, i2; // po��tadla
  unsigned char str1_len = strlen(first); // d�lka p�vodn�ho �et�zce
  for (i = i2 = 0; i < str1_len; i++, i2++)
  {
    if(tolower(first[i]) == 'c' && i < (str1_len-1) && tolower(first[i+1]) == 'h')
    { // pokud je CH
      str1[i2] = 11; // kod CH
      i++;
    }
    else
    { // ostatn� znaky se p�evedou podle pTABLE
      str1[i2] = pTABLE[(unsigned char) tolower(first[i])]; 
    }
  }
  str1[i2] = '\0';

  unsigned char y, y2; // po��tadla
  unsigned char str2_len = strlen(second); // d�lka p�vodn�ho �et�zce
  for (y = y2 = 0; y < str2_len; y++, y2++)
  { 
    if(tolower(second[y]) == 'c' && y < (str2_len-1) && tolower(second[y+1]) == 'h')
    { // pokud je CH
      str2[y2] = 11; // kod CH
      y++;
    }
    else
    { // ostatn� znaky se p�evedou podle pTABLE
      str2[y2] = pTABLE[(unsigned char) tolower(second[y])]; 
    }
  }
  str2[y2] = '\0'; // ukon�en� �et�zce
  
  // porovn�n� "hodnot" znak� �et�zce
  int code = strcmp(str1, str2);

  if (code == 0)
  {// pokud slova stejn� = stejn� dlouh�, zkus�m diakritiku
    unsigned char z;
    str1_len = strlen(str1); // nov� d�lka (p�i CH je p�vodn� d�lka-1)
    for (z = 0; z < str1_len; z++)
    { // jednotliv� znaky se p�evedou podle sTABLE
      str1[z] = sTABLE[(unsigned char) tolower(first[z])]+1; // pokud nenalezne p�smeno v tabulce, vr�t� 0
      str2[z] = sTABLE[(unsigned char) tolower(second[z])]+1;// ale pro 0 strcmp �patn� funguje, proto +1
    }
    str1[z] = '\0';
    str2[z] = '\0';
    // porovn�n� nov�ch hodnot dle sTABLE (secondary table)
    code = strcmp(str1, str2);
    // p��mo se vrac� k�d ze strcmp
    return code;
  }
  else
  { // pokud nebyly stejn�, vrac� se rovnou k�d
    return code;
  }
} // konec funkce cmpCzech


/**
* P�evede slovn� druh na ��slo.
* @param wclass �et�zec s n�zvem slovn�ho druhu.
* @return Vrac� ��slo slovn�ho druhu, nebo 0 kdy� nenajde.
*/
int getWClass(char *wclass)
{
  for (int i = 0; i < 11; i++)
  { // 11 slovn�ch druh�
    if (strcmp(items[i].name, wclass) == 0)
    {
      return items[i].number;
    }
  }
  // nena�lo to slovn� druh
  return 0;
} // konec funkce getWClass


/**
* Zpracuje zadan� parametr, ze kter�ho se pokus� extrahovat slo�en� kl��.
* @param key �et�zec s kl��em (typu t�eba --klic+=12).
* @param mykey Struktura s analyzovan�m kl��em.
* @return Vrac� chybov� k�d.
*/
int getKey(char *key, TKey *mykey)
{
  if (strlen(key) >= 9 && strlen(key) <= 11)
  { // pokud kl�� odpov�d� mo�n�m rozm�r�m
    char substr[7]; // pro prvnich 6 znaku klice + \0
    strncpy(substr, key, 6);
    substr[6] = '\0'; // substr by m�l b�t "--klic\0"
    
    if (strcmp("--klic", substr) != 0)
    { // test jestli je zacatek klice spravne
      return EBADKEY;
    }
    
    if (key[6] == '+' || key[6] == '-')
    {// zda vzestupne nebo sestupne
      mykey->ord = key[6];
    }
    else
    {
      return EBADKEY;
    }
    
    if (key[7] != '=')
    {
      return EBADKEY;
    }
    
    int len = strlen(key);  // d�lka kl��e
    int j = 0;
    char cnt1 = 0, cnt2 = 0, cnt3 = 0; // po�et 1,2,3 ve klici
    
    for (int i = 8; i < len; i++)
    {// od indexu 8 by melo zacinat cislo slozeneho klice
      if (key[i] > '0' && key[i] < '4')
      { // jen pro ��sla 1,2,3
        if (key[i] == '1')
        { // po�et 1
          cnt1++;
        }
        else if (key[i] == '2')
        { // po�et 2
          cnt2++;
        }
        else if (key[i] == '3')
        { // po�et 3
          cnt3++;
        }
        
        if (cnt1 > 1 || cnt2 > 1 || cnt3 > 1)
        { // pokud je v klici nejakej cislo vickrat
          return EBADKEY;
        }
        
        mykey->key[j] = key[i]; // v�sledn� kl��
        j++;
      }
      else
      {
        return EBADKEY;
      }
    }
    mykey->key[j] = '\0';
  }
  else
  { // kl�� nem� mo�n� rozm�ry
    return EBADKEY;
  }

  return EOK;
} // konec funkce getKey


/**
 * Vytiskne n�pov�du.
 */
void printHelp(void)
{
  printf(
      "\n"
      "* Program: �esk� �azen�\n"
      "* Autor: David Krym (c) 2009\n"
      "* \n"
      "* Program seradi CSV data ze vstupniho souboru dle zadaneho klice.\n"
      "* Serazena data se zapisou do vystupniho souboru.\n"
      "* Data na vstupu se ocekavaji ve formatu: cesky;anglicky;druh.\n"
      "\n"
      " Pouziti:\n"
      "\tproj2 -h\n"
      "\tproj2 --klic-=NNN in.txt out.txt\n"
      "\tproj2 --klic+=NNN in.txt out.txt\n"
      "\n"
      " Popis parametru:\n"
      "\t-h\t\t\t\t Vypise tuto obrazovku s napovedou.\n"
      "\t--klic-=NNN in.txt out.txt\t Seradi data sestupne dle klice.\n"
      "\t--klic+=NNN in.txt out.txt\t Seradi data vzestupne dle klice.\n"
      "\n"
  );

} // konec funkce printHelp


/**
 * Zkus� zav��t zadan� soubor.
 * @param closefile Soubor, kter� chceme zav��t.
 * @param mod M�d, ve kter�m chceme soubor otev��t.
 * @return Vrac� chybov� k�d.
 */
int tryClose(FILE *closefile)
{
  if(fclose(closefile) == EOF)
  {  
    return ECLOSE;
  }
  
  return EOK;
} // konec funkce tryClose
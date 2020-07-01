/*
 * Soubor:  proj4.c
 * Datum:   2009/12/18
 * Autor:   David Krym
 * Projekt: Èeské øazení, projekt è. 4 pro pøedmìt IZP
 * Popis:   Vzestupnì nebo sestupnì seøadí slovníkové údaje ze vstupního CSV souboru.
 *          Oèekává vstup ve tvaru: èesky;anglicky;druh
 *          Data zapí¹e do výstupního CSV souboru.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
// maximální délka øetìzce +1 (tzn zde 50 znakù)
#define WLENGTH 51 

// primary table
const unsigned char pTABLE[] =
{
  [' ']=1,  ['a']=2,  ['b']=3,  ['c']=4,  ['d']=6,  ['e']=7,
  ['f']=8,  ['g']=9,  ['h']=10, /*ch=11*/ ['i']=12, ['j']=13,
  ['k']=14, ['l']=15, ['m']=16, ['n']=17, ['o']=18, ['p']=19,
  ['q']=20, ['r']=21, ['s']=23, ['t']=25, ['u']=26, ['v']=27,
  ['w']=28, ['x']=29, ['y']=30, ['z']=31, ['.']=33, ['-']=34,
  
  [(unsigned char) 'á']=2,  [(unsigned char) 'è']=5,  [(unsigned char) 'ï']=6,
  [(unsigned char) 'é']=7,  [(unsigned char) 'ì']=7,  [(unsigned char) 'í']=12,
  [(unsigned char) 'ò']=17, [(unsigned char) 'ó']=18, [(unsigned char) 'ø']=22,
  [(unsigned char) '¹']=24, [(unsigned char) '»']=25, [(unsigned char) 'ú']=26,
  [(unsigned char) 'ù']=26, [(unsigned char) 'ý']=30, [(unsigned char) '¾']=32,
  
  [(unsigned char) 'Á']=2,  [(unsigned char) 'È']=5,  [(unsigned char) 'Ï']=6,
  [(unsigned char) 'É']=7,  [(unsigned char) 'Ì']=7,  [(unsigned char) 'Í']=12,
  [(unsigned char) 'Ò']=17, [(unsigned char) 'Ó']=18, [(unsigned char) 'Ø']=22,
  [(unsigned char) '©']=24, [(unsigned char) '«']=25, [(unsigned char) 'Ú']=26,
  [(unsigned char) 'Ù']=26, [(unsigned char) 'Ý']=30, [(unsigned char) '®']=32,
};

// secondary table
const unsigned char sTABLE[] =
{
  [(unsigned char) 'á']=1,  [(unsigned char) 'ï']=1,  [(unsigned char) 'é']=1,
  [(unsigned char) 'ì']=2,  [(unsigned char) 'í']=1,  [(unsigned char) 'ò']=1,
  [(unsigned char) 'ó']=1,  [(unsigned char) '»']=1,  [(unsigned char) 'ú']=1,
  [(unsigned char) 'ù']=2,  [(unsigned char) 'ý']=1, 
  
  [(unsigned char) 'Á']=1,  [(unsigned char) 'Ï']=1,  [(unsigned char) 'É']=1,
  [(unsigned char) 'Ì']=2,  [(unsigned char) 'Í']=1,  [(unsigned char) 'Ò']=1,
  [(unsigned char) 'Ó']=1,  [(unsigned char) '«']=1,  [(unsigned char) 'Ú']=1,
  [(unsigned char) 'Ù']=2,  [(unsigned char) 'Ý']=1, 
};


/**
 * Struktura obsahující hodnoty otevøeného souboru.
 */
typedef struct tfile
{
  FILE *pfile;           /**< Ukazatel na otevøený soubor. */
  int ecode;             /**< Chybový kód programu, odpovídá výètu tecodes. */
} TFile;


/**
 * Struktura obsahující analyzovaný øádek souboru (slovníku)
 */
typedef struct tline
{
  char czech[WLENGTH];      /**< Èeské slovo. */
  char english[WLENGTH];    /**< Anglické slovo. */
  int wclass;               /**< Èíslo slovního druhu. */
} TLine;


/**
 * Struktura obsahující slovní druh a jeho poøadové èíslo
 */
typedef struct twclass
{
  char *name;     /**< Jméno slovního druhu. */
  int number;     /**< Èíslo slovního druhu. */
} TWClass;


/**
 * Struktura obsahující hodnoty analyzovaného klíèe.
 */
typedef struct tkey
{
  char key[4];    /**< Slo¾ený klíè, max 3 znaky. */
  char ord;       /**< Zda øadit vzestupnì/sestupnì. */
} TKey;


/**
 * Struktura obsahující hodnoty parametrù pøíkazové øádky.
 */
typedef struct params
{
  TKey mykey;            /**< Analyzovaný slo¾ený klíè. */
  char *fname1;          /**< Jméno vstupního souboru. */
  char *fname2;          /**< Jméno výstupního souboru. */
  int ecode;             /**< Chybový kód programu, odpovídá výètu tecodes. */
  int state;             /**< Stavový kód programu, odpovídá výètu tstates. */
} TParams;


/**
 * Pole struktur TWClass, obsahuje slovní druhy s jejich poøadím.
 */
const TWClass items[] =
{
  {"PODSTATNÉ JMÉNO", 1},
  {"PØÍDAVNÉ JMÉNO", 2},
  {"ZÁJMENO", 3},
  {"ÈÍSLOVKA", 4},
  {"SLOVESO", 5},
  {"PØÍSLOVCE", 6},
  {"PØEDLO®KA", 7},
  {"SPOJKA", 8},
  {"ÈÁSTICE", 9},
  {"CITOSLOVCE", 10},
  {"FRÁZE", 11},
};


/** Kódy chyb programu */
enum tecodes
{
  EOK = 0,     /**< Bez chyby. */
  ECLWRONG,    /**< Chybné parametry. */
  EOPEN,       /**< Chyba pøi otevírání souboru.*/
  EWRITE,      /**< Chyba pøi zapisování souboru. */
  ECLOSE,      /**< Chyba pøi zavírání souboru. */
  EBADFILE,    /**< Soubor neodpovídá zadání. */
  EBADLINE,    /**< Øádka v souboru má chybný formát. */
  EALLOC,      /**< Chyba pøi alokaci. */
  EBADKEY,     /**< ©patnì zadaný klíè. */
  EWCLASS,     /**< Chybnì zadaný slovní druh. */
  EBADCH,      /**< Chybný znak v souboru. */
  EUNKNOWN,    /**< Neznámá chyba. */
};


/** Chybová hlá¹ení odpovídající chybovým kódùm tencodes. */
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
  "Chybný slovni druh v souboru!\n",
  /* EBADCH */
  "Soubor obsahuje nedovolené znaky!\n",
  
  "Nastala nepredvidatelna chyba!\n",
};


/** Stavové kódy programu */
enum tstates
{
  CHELP,       /**< Nápovìda */
  CSORT,       /**< Normální øazení. */
};


///////// prototypy funkcí ////////////////////////////////////////
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
 * Hlavní program.
 */
int main(int argc, char *argv[])
{
  TParams params = getParams(argc, argv);

  if (params.ecode != EOK)
  { // nìco nestandardního
    printECode(params.ecode);
    return EXIT_FAILURE;
  }
  
  if (params.state == CHELP)
  { // vypsat nápovìdu, ukonèit program
    printHelp();
  }
  else
  { // jméno prvního souboru, druhého, klíè
    int ecode = load(params.fname1, params.fname2, &params.mykey);
    // vracím chybová kód
    if (ecode != EOK)
    { // nìco nestandardního
      printECode(ecode);
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}


/**
 * Zpracuje argumenty pøíkazového øádku a vrátí je ve struktuøe TParams.
 * Pokud je formát argumentù chybný, ukonèí program s chybovým kódem.
 * @param argc Poèet argumentù.
 * @param argv Pole textových øetìzcù s argumenty.
 * @return Vrací analyzované argumenty pøíkazového øádku.
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
  { // tisk nápovìdy
    result.state = CHELP;
  }
  else if (argc == 4)
  { // ètyøi params, proj4 --key in.txt out.txt
    result.fname1 = argv[2];
    result.fname2 = argv[3];
    int ecode = getKey(argv[1], &result.mykey);
    if (ecode != EOK)
    { // chyba pøi spracování klíèe
      result.ecode = ecode;
    }
    result.state = CSORT; // bude se øadit
  }
  else
  { // pøíli¹ mnoho parametrù
    result.ecode = ECLWRONG;
  }

  return result;
} // konec funkce getParams


/**
 * Vytiskne hlá¹ení odpovídající chybovému kódu.
 * @param code Kód chyby programu
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
 * Zkusí otevøít zadaný soubor v zadaném módu.
 * @param name Jméno otevíraného souboru.
 * @param mod Mód, ve kterém chceme soubor otevøít.
 * @return Vrací strukturu s ukazatelem na soubor a chybovým kódem.
 */
TFile tryOpen(char *name, char *mod)
{
  TFile openfile =
  {
    .pfile = NULL,
    .ecode = EOK,
  };
  // pokud o otevøení
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
* Hlavní funkce, naplní pole souborem a volá funkce na øazení a ulo¾rní výstupu.
* @param infile Jméno vstupního souboru.
* @param outfile Jméno výstupního souboru.
* @param mykey Struktura se slo¾eným klíèem a typem øazení (sest/vzest).
* @return Vrací chybový kód.
*/
int load(char *infile, char *outfile, TKey *mykey)
{
  // pokus o otevøení souboru
  TFile fr = tryOpen(infile,"r");
  if (fr.ecode != EOK)
  { // nepodarilo se otevrit
    return fr.ecode;
  }
///////////////////////// naèítání
  const int BLOCK_INCREMENT = 32;
  TLine *buf = NULL;
  int size = 0; // velikost bufferu, násobky BLOCK_INCREMENT
  int row = 0;  // poèet skuteènì naètených øádkù
  int c = 0, cprev = 0; // naètený znak,pøedchozí naètený znak
  int c_cnt = 0; // poèet znakù pøed støedníkem
  int semicolon = 0; // poèet støedníkù
  char wclass[WLENGTH] = "\0"; // slovní druh
  
  while ((c = fgetc(fr.pfile))!= EOF)
  { // ète soubor
    if (row == size)
    { // jsem za hranicí, nafouknu pole
      size += BLOCK_INCREMENT;
      TLine *rebuf = realloc(buf, size*sizeof(TLine));
      if (rebuf == NULL)
      { // není místo, uklidím
        free(buf);
        buf = NULL;
        size = 0;
        tryClose(fr.pfile);
        return EALLOC;
      } // vpoøádku naalokováno
      buf = rebuf;
    }
   
    // spracování øádkù
    if (c == ';')
    { // kolikátej støedník na øádku
      if (cprev == c)
      { // pokud aktuální i pøedchozí znak je ;, chyba
        free(buf);
        tryClose(fr.pfile);
        return EBADLINE;
      }
      semicolon++;
      c_cnt = 0; // poèítadlo jede od znova
      cprev = c; // ulo¾im si znak jako pøedchozí znak
      continue;
    }
    else if (c == '\n')
    { // nový øádek
      if (c == cprev)
      { // pokud je víc prázdných øádkù 
        continue;
      }
      wclass[c_cnt] = '\0';
      buf[row].wclass = getWClass(wclass); // pøevedeme slovní druh na èíslo
      if (buf[row].wclass == 0)
      { // chybný slovní druh
        free(buf);
        tryClose(fr.pfile);
        return EWCLASS;
      }
      c_cnt = 0;
      semicolon = 0;
      row++;
      cprev = c; // ulo¾im si znak jako pøedchozí znak
      continue;
    }
    
    if (pTABLE[(unsigned char) tolower(c)] == 0)
    { // jestli obsahuje jen povolené znaky
      free(buf);
      tryClose(fr.pfile);
      return EBADCH;
    }
    
    
    // podle poètu ; na øádku se naèítá daný øetìzec
    if (semicolon == 0)
    { // èeské slovo
      buf[row].czech[c_cnt] = c;
      buf[row].czech[c_cnt+1] = '\0';
      c_cnt++;
    }
    else if (semicolon == 1)
    { // anglické slovo
      buf[row].english[c_cnt] = c;
      buf[row].english[c_cnt+1] = '\0';
      c_cnt++;
    }
    else if (semicolon == 2)
    { // slovní druh
      wclass[c_cnt] = c;
      c_cnt++;
    }
    cprev = c; // ulo¾im si znak jako pøedchozí znak
    
    if (c_cnt > WLENGTH-1)
    { // vic jak 50 znakù, pøeskoèit
      continue;
    }
  }
  tryClose(fr.pfile); // zavøít soubor
    
  if (cprev == '\n' && semicolon == 2)
  { // pokud neni soubor zakonèen novým øádkem
    wclass[c_cnt] = '\0';
    buf[row].wclass = getWClass(wclass); // slovní druh na èíslo
    if (buf[row].wclass == 0)
    { // ¹patný slovní druh, konec
      free(buf);
      buf = NULL;
      size = 0;
      return EWCLASS;
    }
    c_cnt = 0;
    row++;
  }
///////////////////////// konec naèítání

  // alokace indexù, které budu øadit
  int *order = malloc(row * sizeof(int));
  if (order == NULL)
  { // buffer uvolnit
    free(buf);
    return EALLOC;
  }
  for (int i = 0; i < row; i++)
  { // naplnìní pole èíslama 0..row-1
    order[i] = i;
  }

  // øazení indexù
  isort(buf, row, order, mykey);
  // ulo¾ení výstupního souboru
  int ecode = CSVtoFile(buf, row, order, outfile);
  free(buf);
  free(order);
  // chyba pøi zápisu do souboru
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
* Seøadí indexy pole dle zadaného klíèe.
* @param buffer Ukazatel na zaèátek pole struktur.
* @param row Èíslo posledního øádku.
* @param order Pole s indexama poøadí.
* @param mykey Struktura se slo¾eným klíèem a typem øazení (sest/vzest).
* @return Vrací chybový kód.
*/
void isort(TLine *buffer, int rows, int *order, TKey *mykey)
{
  if (mykey->ord == '+')
  { // vzestupne
    for (int i = 0; i < rows; i++)
    {
      int x = order[i]; // poøadové èíslo aktuálního øádku
      int j = i;  // pomocné
                        // pokud prvni je mensi nez druhej
      while ((j > 0) && (cmpWords(&buffer[x], &buffer[order[j - 1]], mykey) < 0))
      { // posuv prvkù o jednièku doprava (nacházení pozice, kam vlo¾it aktuální index)
        order[j] = order[j - 1];  // pøedchozí index na aktuální
        j--;
      }
      order[j] = x;
    }
  }
  else
  { // sestupnì
    for (int i = 0; i < rows; i++)
    {
      int x = order[i]; // poøadové èíslo aktuálního øádku
      int j = i;  // pomocné
                        // pokud prvni je mensi nez druhej
      while ((j > 0) && (cmpWords(&buffer[x], &buffer[order[j - 1]], mykey) > 0))
      { // posuv prvkù o jednièku doprava
        order[j] = order[j - 1];  // pøedchozí na aktuální
        j--;
      }
      order[j] = x;
    }
  }
} // konec funkce iSort


/**
* Porovnává slova dle zadaného klíèe.
* @param first Struktura s jedním øádkem slovníku.
* @param second Struktura s druhým øádkem slovníku.
* @param mkey Slo¾ený klíè na øazení.
* @return Vrací -1 pokud je první slovo men¹í,
                 1 pokud je druhé slovo men¹í,
                 0 pokud jsou slova stejná.
*/
int cmpWords(TLine *first, TLine *second, TKey *mykey)
{ 
  int keylen = strlen(mykey->key); // délka klíèe
  int res = 0; // result
  
  for (int i = 0; i < keylen; i++)
  { // pro ka¾dé èíslo z keye
    if (mykey->key[i] == '1')
    { // øazení podle èeského slova
      res = cmpCzech(first->czech, second->czech);
      if (res == 0)
      { // pokud se øetìzce rovnají
        continue;
      }
      // pokud se nerovnají, máme výsledek a vyskoèíme z cyklu
      break;
    }
    else if (mykey->key[i] == '2')
    { // øazení podle anglického slova
      res = strcmp(first->english, second->english);
      if (res == 0)
      { // pokud se øetìzce rovnají
        continue;
      }
      // pokud se nerovnají, máme výsledek a vyskoèíme z cyklu
      break;
    }
    else if (mykey->key[i] == '3')
    { // øazení podle slovního druhu
      if (first->wclass == second->wclass)
      {
        res = 0;
        continue;
      }
      // pokud první druh men¹í tak -1, jinak 1
      res = (first->wclass < second->wclass) ? -1 : 1;
      break;
    }
  }
  // vrací -1,0,1
  return res;
} // konec funkce cmpWords


/**
* Vytiskne CSV ze zadaného pole.
* @param buffer Ukazatel na zaèátek pole struktur.
* @param row Èíslo posledního øádku.
* @param order Pole s indexama poøadí.
* @param outfile Jméno výstupního souboru.
* @return Vrací chybový kód.
*/
int CSVtoFile(TLine *buffer, int row, int *order, char *outfile)
{
  TFile fw = tryOpen(outfile,"w"); // pro zápis
  if (fw.ecode != EOK)
  { // nepodarilo se otevrit
    return fw.ecode;
  }
  
  char *wclass = '\0';
  for (int i = 0; i < row; i++)
  { // pøevod slovního druhu z èísla na øetìzec
    for (int y = 0; y < 11; y++)
    { // 11 slovních druhù
      if (buffer[order[i]].wclass == items[y].number)
      {
        wclass = items[y].name;
        break;
      }
    }
    // zápis do souboru v CSV formátu
    fprintf(fw.pfile, "%s;%s;%s\n", buffer[order[i]].czech, buffer[order[i]].english, wclass);
  }
  tryClose(fw.pfile);
  
  return EOK;
} // konec funkce CSVtoFile


/**
* Porovnává lexikograficky dvì èeské slova.
* @param first První èeské slovo (øetìzec).
* @param second Druhé èeské slovo.
* @return Vrací -1 pokud je první slovo men¹í,
                 1 pokud je druhé slovo men¹í,
                 0 pokud jsou slova stejná.
*/
int cmpCzech(char *first, char *second)
{
  char str1[WLENGTH]; // na ulo¾ení "hodnot" èísel
  char str2[WLENGTH];
  
  unsigned char i, i2; // poèítadla
  unsigned char str1_len = strlen(first); // délka pùvodního øetìzce
  for (i = i2 = 0; i < str1_len; i++, i2++)
  {
    if(tolower(first[i]) == 'c' && i < (str1_len-1) && tolower(first[i+1]) == 'h')
    { // pokud je CH
      str1[i2] = 11; // kod CH
      i++;
    }
    else
    { // ostatní znaky se pøevedou podle pTABLE
      str1[i2] = pTABLE[(unsigned char) tolower(first[i])]; 
    }
  }
  str1[i2] = '\0';

  unsigned char y, y2; // poèítadla
  unsigned char str2_len = strlen(second); // délka pùvodního øetìzce
  for (y = y2 = 0; y < str2_len; y++, y2++)
  { 
    if(tolower(second[y]) == 'c' && y < (str2_len-1) && tolower(second[y+1]) == 'h')
    { // pokud je CH
      str2[y2] = 11; // kod CH
      y++;
    }
    else
    { // ostatní znaky se pøevedou podle pTABLE
      str2[y2] = pTABLE[(unsigned char) tolower(second[y])]; 
    }
  }
  str2[y2] = '\0'; // ukonèení øetìzce
  
  // porovnání "hodnot" znakù øetìzce
  int code = strcmp(str1, str2);

  if (code == 0)
  {// pokud slova stejné = stejnì dlouhé, zkusím diakritiku
    unsigned char z;
    str1_len = strlen(str1); // nová délka (pøi CH je pùvodní délka-1)
    for (z = 0; z < str1_len; z++)
    { // jednotlivé znaky se pøevedou podle sTABLE
      str1[z] = sTABLE[(unsigned char) tolower(first[z])]+1; // pokud nenalezne písmeno v tabulce, vrátí 0
      str2[z] = sTABLE[(unsigned char) tolower(second[z])]+1;// ale pro 0 strcmp ¹patnì funguje, proto +1
    }
    str1[z] = '\0';
    str2[z] = '\0';
    // porovnání nových hodnot dle sTABLE (secondary table)
    code = strcmp(str1, str2);
    // pøímo se vrací kód ze strcmp
    return code;
  }
  else
  { // pokud nebyly stejné, vrací se rovnou kód
    return code;
  }
} // konec funkce cmpCzech


/**
* Pøevede slovní druh na èíslo.
* @param wclass Øetìzec s názvem slovního druhu.
* @return Vrací èíslo slovního druhu, nebo 0 kdy¾ nenajde.
*/
int getWClass(char *wclass)
{
  for (int i = 0; i < 11; i++)
  { // 11 slovních druhù
    if (strcmp(items[i].name, wclass) == 0)
    {
      return items[i].number;
    }
  }
  // nena¹lo to slovní druh
  return 0;
} // konec funkce getWClass


/**
* Zpracuje zadaný parametr, ze kterého se pokusí extrahovat slo¾ený klíè.
* @param key Øetìzec s klíèem (typu tøeba --klic+=12).
* @param mykey Struktura s analyzovaným klíèem.
* @return Vrací chybový kód.
*/
int getKey(char *key, TKey *mykey)
{
  if (strlen(key) >= 9 && strlen(key) <= 11)
  { // pokud klíè odpovídá mo¾ným rozmìrùm
    char substr[7]; // pro prvnich 6 znaku klice + \0
    strncpy(substr, key, 6);
    substr[6] = '\0'; // substr by mìl být "--klic\0"
    
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
    
    int len = strlen(key);  // délka klíèe
    int j = 0;
    char cnt1 = 0, cnt2 = 0, cnt3 = 0; // poèet 1,2,3 ve klici
    
    for (int i = 8; i < len; i++)
    {// od indexu 8 by melo zacinat cislo slozeneho klice
      if (key[i] > '0' && key[i] < '4')
      { // jen pro èísla 1,2,3
        if (key[i] == '1')
        { // poèet 1
          cnt1++;
        }
        else if (key[i] == '2')
        { // poèet 2
          cnt2++;
        }
        else if (key[i] == '3')
        { // poèet 3
          cnt3++;
        }
        
        if (cnt1 > 1 || cnt2 > 1 || cnt3 > 1)
        { // pokud je v klici nejakej cislo vickrat
          return EBADKEY;
        }
        
        mykey->key[j] = key[i]; // výsledný klíè
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
  { // klíè nemá mo¾né rozmìry
    return EBADKEY;
  }

  return EOK;
} // konec funkce getKey


/**
 * Vytiskne nápovìdu.
 */
void printHelp(void)
{
  printf(
      "\n"
      "* Program: Èeské øazení\n"
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
 * Zkusí zavøít zadaný soubor.
 * @param closefile Soubor, který chceme zavøít.
 * @param mod Mód, ve kterém chceme soubor otevøít.
 * @return Vrací chybový kód.
 */
int tryClose(FILE *closefile)
{
  if(fclose(closefile) == EOF)
  {  
    return ECLOSE;
  }
  
  return EOK;
} // konec funkce tryClose
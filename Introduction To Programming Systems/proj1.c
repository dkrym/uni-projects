/*
 * Soubor:  proj1.c
 * Datum:   2009/09/18
 * Autor:   David Krym
 * Projekt: Zpracov�n� textu, projekt �. 1 pro p�edm�t IZP
 * Popis:   Program zpracov�v� text ze standardn�ho vstupu a nahrazuje ka�dou �adu b�l�ch znak� pr�v� jednou mezerou.
 *          S pou�it�m parametr� lze ur�it, kolik bude na v�stupu slov/v�t. V�stup se tiskne na standardn� v�stup.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <errno.h>


/** K�dy chyb programu */
enum tecodes
{
  EOK = 0,     /**< Bez chyby. */
  ECLWRONG,    /**< Chybn� p��kazov� ��dek. */
  ECLZERO,     /**< Parametr wpl/spl je nula. */
  ECLNOTNUM,   /**< Parametr wpl/spl neni ��slo. */
  ECLNOTN,     /**< Parametr wpl/spl neni zad�n. */
  ECLTOOBIG,   /**< Parametr wpl/spl je mimo rozsah. */
  EUNKNOWN,    /**< Nezn�m� chyba. */
};


/** Stavov� k�dy programu */
enum tstates
{
  CWHITE,      /**< Odstran�n� b�l�ch znak�. */
  CHELP,       /**< N�pov�da */
  CWPL,        /**< Slova na ��dku. */
  CSPL,        /**< V�ty na ��dku. */
};


/** Chybov� hl�en� odpov�daj�c� chybov�m k�d�m. */
const char *ECODEMSG[] =
{
  /* EOK */
  "Vse v poradku.\n",
  /* ECLWRONG */
  "Chybne parametry prikazoveho radku!\n",
  /* ECLZERO */
  "S parametrem wpl/spl byla zadana nula!\n",
  /* ECLNOTNUM */
  "S parametrem wpl/spl nebylo zadano cislo, nebo bylo zadano cislo zaporne!\n",
  /* ECLNOTN */
  "Nebyl zadan parametr N!\n",
  /* ECLTOOBIG */
  "S parametrem wpl/spl byla zadana hodnota mimo rozsah!\n",

  "Nastala nepredvidatelna chyba!\n",
};


/**
 * Struktura obsahuj�c� hodnoty parametr� p��kazov� ��dky.
 */
typedef struct params
{
  unsigned long int N;   /**< Hodnota N z p��kazov� ��dky. */
  int ecode;             /**< Chybov� k�d programu, odpov�d� v��tu tecodes. */
  int state;             /**< Stavov� k�d programu, odpov�d� v��tu tstates. */
} TParams;


/////////////////////////////////////////////////////////////////
///////// Prototypy funkc�
TParams testULong(char *numstr);
void printECode(int ecode);
bool wpl(unsigned long int N, bool wasSpace);
bool spl(int c, unsigned long int N, bool wasSpace);
void removeSpaces(int param, int N);
TParams getParams(int argc, char *argv[]);
void printHelp(void);


/////////////////////////////////////////////////////////////////
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
  else if (params.state == CWHITE)
  { // nahrad� b�l� znaky jednou mezerou
    removeSpaces(CWHITE,0);
  }
  else if (params.state == CWPL)
  { // nahrad� b�l� znaky jednou mezerou, N slov na ��dek
    removeSpaces(CWPL,params.N);
  }
  else if (params.state == CSPL)
  { // nahrad� b�l� znaky jednou mezerou, N v�t na ��dek
    removeSpaces(CSPL,params.N);
  }

  return EXIT_SUCCESS;
}
/////////////////////////////////////////////////////////////////
///////// Funkce


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
    .N = 0,
    .ecode = EOK,
    .state = CWHITE,
  };

  if (argc == 1)
  { //jen mezery
    result.state = CWHITE;
  }
  else if (argc == 2 && strcmp("-h", argv[1]) == 0)
  { // tisk n�pov�dy
    result.state = CHELP;
  }
  else if (argc == 3 && strcmp("--wpl", argv[1]) == 0)
  { // dva parametry, word per line
    TParams ULongNum = testULong(argv[2]);  // zda je zadan� ��slo v intervalu 1 - ULONG_MAX

    result.state = CWPL;
    result.ecode = ULongNum.ecode;
    result.N = ULongNum.N;
  }
  else if (argc == 3 && strcmp("--spl", argv[1]) == 0)
  { // dva parametry, sentence per line
    TParams ULongNum = testULong(argv[2]);  // zda je zadan� ��slo v intervalu 1 - ULONG_MAX

    result.state = CSPL;
    result.ecode = ULongNum.ecode;
    result.N = ULongNum.N;
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
 * Vytiskne n�pov�du.
 */
void printHelp(void)
{
  printf(
      "\n"
      "* Program: Zpracovani textu\n"
      "* Autor: David Krym (c) 2009\n"
      "* \n"
      "* Program zpracovava text ze standardniho vstupu. Nahradi kazdou posloupnost\n"
      "* bilych znaku prave jednou mezerou. Dale program umi vypsat urcity pocet\n"
      "* vet ci slov na radek.\n"
      "\n"
      " Pouziti:\n"
      "\tproj1\n"
      "\tproj1 -h\n"
      "\tproj1 --wpl N\n"
      "\tproj1 --spl N\n"
      "\n"
      " Popis parametru:\n"
      "\tbez parametru\t Nahradi posloupnost bilych znaku prave jednim.\n"
      "\t-h\t\t Vypise tuto obrazovku s napovedou.\n"
      "\t--wpl N\t\t Na kazdem radku na vystupu bude prave N slov.\n"
      "\t--spl N\t\t Na kazdem radku na vystupu bude prave N vet.\n"
      "\n"
      " Priklad pouziti (radek je v uvozovkach):\n"
      "\tproj1 --wpl 2\n"
      "\t vstup:\n"
      "\t\t\"  slovo.slovo   . slovo . \"\n"
      "\t vystup:\n"
      "\t\t\"slovo.slovo .\"\n"
      "\t\t\"slovo .\"\n"
      "\n"
      "\tproj1 --spl 2\n"
      "\t vstup:\n"
      "\t\t\"  .slovo.slovo   . slovo . \"\n"
      "\t vystup:\n"
      "\t\t\". slovo.\"\n"
      "\t\t\"slovo . slovo .\"\n"
      "\n"
  );

} // konec funkce printHelp


/**
 * Zkus� p�ev�st textov� �et�zec na ��slo pomoc� funkce strtoul.
 * @param numstr Ukazatel na �et�zec, kter� chceme p�ev�st na ulong.
 * @return Vrac� p�eveden� ��slo a chybov� k�d.
 */
TParams testULong(char *numstr)
{
  TParams result =
  { // inicializace struktury
    .N = 0,
    .ecode = EOK,
    .state = CWHITE,
  };

  if (!isdigit(numstr[0]))
  { // prvn� znak neni ��slo
    result.ecode = ECLNOTNUM;
    return result;
  }

  char *endptr;
  errno = 0; // aby n�s neru�ily p�edchoz� chyby
  // pokus o p�evod
  unsigned long int cislo = strtoul(numstr, &endptr, 10);

  int errcode = errno; // jen pro jistotu, kdybych �asem p�id�val dal�� p��kazy

  if ((errcode == ERANGE && cislo == ULONG_MAX) ||
      (errcode != 0 && cislo == 0))
  { // hodnota je mimo rozsah
    result.ecode = ECLTOOBIG;
  }
  else if (endptr == numstr)
  { // oba ukazatele ukazuj� na za��tek �et�zce, nic nebylo zad�no
    result.ecode = ECLNOTN;
  }
  else 
  {
    if (*endptr != '\0')  // za ��slem jsou dal�� znaky, n�kdy nemus� j�t o chybu
    { 
      result.ecode = ECLNOTNUM;
    }
  }


  if (result.ecode == EOK)
  {
    if (cislo == 0)
    { // o�et�en� nuly
      result.ecode = ECLZERO;
    }
    else
    {
      result.N = cislo;
    }
  }

  return result;
} // konec funkce testulong


/**
 * Zaji��uje zvolen� po�et slov na ��dku - words per line.
 * Na posledn�m ��dku m��e b�t slov m�n�, ov�em ��dek nem��e b�t pr�zdn�.
 * @param N Po�et slov na ��dek.
 * @param wasSpace Zda byla p�ed aktu�ln�m znakem mezera.
 * @return Vrac�, zda byl text zalomen na nov� ��dek.
 */
bool wpl(unsigned long int N, bool wasSpace)
{
  static unsigned long int words = 0; //slov na ��dku, prom�nn� si zachov�v� hodnotu

  if (wasSpace)
  { // p�edchoz� znak byla mezera -> ukon�eno slovo a za��n� nov�
    ++words;

    if (words == N)
    { // na ��dku u� je zvolen� po�et slov
      words = 0;
      putchar('\n');
      return true;
    }
  }

  return false;
} // konec funkce wpl


/**
 * Zaji��uje zvolen� po�et v�t na ��dku - sentences per line.
 * Na posledn�m ��dku m��e b�t v�t m�n�, ov�em ��dek nem��e b�t pr�zdn�.
 * @param c Aktu�ln� na�ten� neb�l� znak ze vstupu.
 * @param N Po�et v�t na ��dek.
 * @param wasSpace Zda byla p�ed aktu�ln�m znakem mezera
 * @return Vrac�, zda byl text zalomen na nov� ��dek.
 */
bool spl(int c, unsigned long int N, bool wasSpace)
{
  static unsigned long int sentences = 0;
  static bool wasDot = false;

  if (c == '.' && !wasDot)
  { // znak je te�ka, p�edchoz� znak te�ka nebyl -> p�i posloupnosti te�ek nepo��t� ka�dou
    wasDot = true;
  }
  else if ((c != '.' && wasDot) || (wasSpace && wasDot))
  { // znak neni te�ka, p�edchoz� znak te�ka byl, nebo byla mezera a p�ed n� te�ka (. . .)
    ++sentences;
    if (c == '.')
    { // aktu�ln� znak je te�ka
      wasDot = true;
    }
    else
    {
      wasDot = false;
    }

    if (sentences == N)
    { // na ��dku u� je zvolen� po�et v�t
      sentences = 0;
      putchar('\n');
      return true;
    }

    if (!wasSpace)
    { // pokud neni �a te�kou mezera, d� ji tam
    putchar(' ');
    return true;
    }
  }

  return false;
} // konec funkce spl


/**
 * P�i ka�d�m vol�n� za�ne ze standardn�ho vstupu na��tat znaky.
 * Posloupnost b�l�ch znak� nahrad� jednou mezerou, p��padn� zalom� ��dky podle po�tu slov/v�t.
 * Na posledn�m ��dku m��e b�t slov m�n�, ov�em ��dek nem��e b�t pr�zdn�.
 * @param param Zda zalomit ��dky, nebo jen odstranit b�l� znaky.
 * @param N Po�et slov/v�t na ��dek.
 */
void removeSpaces(int param, int N)
{
  int c;
  bool wasSpace = false, // prom�nn� na ur�en� mezery
       fsol = true; // first space on line | true - netiskni mezeru (za��tek ��dku) | false - vytiskni mezeru

  while ((c = getchar()) != EOF)
  {
    if (isspace(c))
    { // na�ten� znak je b�l� znak
      wasSpace = true;
    }
    else
    { // na�ten� znak neni b�l� znak
      if (param == CWPL)
      { // je ur�en� po�et slov na ��dek
        if (!fsol)
        { // ignorace mezer na za��tku
          fsol = wpl(N, wasSpace);  // true - nov� ��dek, netiskni mezeru | false - vytiskni pr�v� jednu mezeru
        }
      }
      else if (param == CSPL)
      { // je ur�en� po�et v�t na ��dek
        if (fsol)
        { // ignorace mezer na za��tku, pokud je prvn� nemezera te�ka, bere ji to jako v�tu
          wasSpace = false;
        }
        fsol = spl(c, N, wasSpace);  // true - nov� ��dek, netiskni mezeru | false - vytiskni pr�v� jednu mezeru
      }

      if (wasSpace && !fsol)
      { // pokud byla na vstupu mezera a nejedn� se o za��tek ��dku
        putchar(' ');
      }

      putchar(c);
      wasSpace = false;
      fsol = false;
    }
  }
} // konec funkce removeSpaces


/* konec project1.c */

/*
 * Soubor:  proj1.c
 * Datum:   2009/09/18
 * Autor:   David Krym
 * Projekt: Zpracování textu, projekt è. 1 pro pøedmìt IZP
 * Popis:   Program zpracovává text ze standardního vstupu a nahrazuje ka¾dou øadu bílých znakù právì jednou mezerou.
 *          S pou¾itím parametrù lze urèit, kolik bude na výstupu slov/vìt. Výstup se tiskne na standardní výstup.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <errno.h>


/** Kódy chyb programu */
enum tecodes
{
  EOK = 0,     /**< Bez chyby. */
  ECLWRONG,    /**< Chybný pøíkazový øádek. */
  ECLZERO,     /**< Parametr wpl/spl je nula. */
  ECLNOTNUM,   /**< Parametr wpl/spl neni èíslo. */
  ECLNOTN,     /**< Parametr wpl/spl neni zadán. */
  ECLTOOBIG,   /**< Parametr wpl/spl je mimo rozsah. */
  EUNKNOWN,    /**< Neznámá chyba. */
};


/** Stavové kódy programu */
enum tstates
{
  CWHITE,      /**< Odstranìní bílých znakù. */
  CHELP,       /**< Nápovìda */
  CWPL,        /**< Slova na øádku. */
  CSPL,        /**< Vìty na øádku. */
};


/** Chybová hlá¹ení odpovídající chybovým kódùm. */
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
 * Struktura obsahující hodnoty parametrù pøíkazové øádky.
 */
typedef struct params
{
  unsigned long int N;   /**< Hodnota N z pøíkazové øádky. */
  int ecode;             /**< Chybový kód programu, odpovídá výètu tecodes. */
  int state;             /**< Stavový kód programu, odpovídá výètu tstates. */
} TParams;


/////////////////////////////////////////////////////////////////
///////// Prototypy funkcí
TParams testULong(char *numstr);
void printECode(int ecode);
bool wpl(unsigned long int N, bool wasSpace);
bool spl(int c, unsigned long int N, bool wasSpace);
void removeSpaces(int param, int N);
TParams getParams(int argc, char *argv[]);
void printHelp(void);


/////////////////////////////////////////////////////////////////
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
  else if (params.state == CWHITE)
  { // nahradí bílé znaky jednou mezerou
    removeSpaces(CWHITE,0);
  }
  else if (params.state == CWPL)
  { // nahradí bílé znaky jednou mezerou, N slov na øádek
    removeSpaces(CWPL,params.N);
  }
  else if (params.state == CSPL)
  { // nahradí bílé znaky jednou mezerou, N vìt na øádek
    removeSpaces(CSPL,params.N);
  }

  return EXIT_SUCCESS;
}
/////////////////////////////////////////////////////////////////
///////// Funkce


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
    .N = 0,
    .ecode = EOK,
    .state = CWHITE,
  };

  if (argc == 1)
  { //jen mezery
    result.state = CWHITE;
  }
  else if (argc == 2 && strcmp("-h", argv[1]) == 0)
  { // tisk nápovìdy
    result.state = CHELP;
  }
  else if (argc == 3 && strcmp("--wpl", argv[1]) == 0)
  { // dva parametry, word per line
    TParams ULongNum = testULong(argv[2]);  // zda je zadané èíslo v intervalu 1 - ULONG_MAX

    result.state = CWPL;
    result.ecode = ULongNum.ecode;
    result.N = ULongNum.N;
  }
  else if (argc == 3 && strcmp("--spl", argv[1]) == 0)
  { // dva parametry, sentence per line
    TParams ULongNum = testULong(argv[2]);  // zda je zadané èíslo v intervalu 1 - ULONG_MAX

    result.state = CSPL;
    result.ecode = ULongNum.ecode;
    result.N = ULongNum.N;
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
 * Vytiskne nápovìdu.
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
 * Zkusí pøevést textový øetìzec na èíslo pomocí funkce strtoul.
 * @param numstr Ukazatel na øetìzec, který chceme pøevést na ulong.
 * @return Vrací pøevedené èíslo a chybový kód.
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
  { // první znak neni èíslo
    result.ecode = ECLNOTNUM;
    return result;
  }

  char *endptr;
  errno = 0; // aby nás neru¹ily pøedchozí chyby
  // pokus o pøevod
  unsigned long int cislo = strtoul(numstr, &endptr, 10);

  int errcode = errno; // jen pro jistotu, kdybych èasem pøidával dal¹í pøíkazy

  if ((errcode == ERANGE && cislo == ULONG_MAX) ||
      (errcode != 0 && cislo == 0))
  { // hodnota je mimo rozsah
    result.ecode = ECLTOOBIG;
  }
  else if (endptr == numstr)
  { // oba ukazatele ukazují na zaèátek øetìzce, nic nebylo zadáno
    result.ecode = ECLNOTN;
  }
  else 
  {
    if (*endptr != '\0')  // za èíslem jsou dal¹í znaky, nìkdy nemusí jít o chybu
    { 
      result.ecode = ECLNOTNUM;
    }
  }


  if (result.ecode == EOK)
  {
    if (cislo == 0)
    { // o¹etøení nuly
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
 * Zaji¹»uje zvolený poèet slov na øádku - words per line.
 * Na posledním øádku mù¾e být slov ménì, ov¹em øádek nemù¾e být prázdný.
 * @param N Poèet slov na øádek.
 * @param wasSpace Zda byla pøed aktuálním znakem mezera.
 * @return Vrací, zda byl text zalomen na nový øádek.
 */
bool wpl(unsigned long int N, bool wasSpace)
{
  static unsigned long int words = 0; //slov na øádku, promìnná si zachovává hodnotu

  if (wasSpace)
  { // pøedchozí znak byla mezera -> ukonèeno slovo a zaèíná nové
    ++words;

    if (words == N)
    { // na øádku u¾ je zvolený poèet slov
      words = 0;
      putchar('\n');
      return true;
    }
  }

  return false;
} // konec funkce wpl


/**
 * Zaji¹»uje zvolený poèet vìt na øádku - sentences per line.
 * Na posledním øádku mù¾e být vìt ménì, ov¹em øádek nemù¾e být prázdný.
 * @param c Aktuálnì naètený nebílý znak ze vstupu.
 * @param N Poèet vìt na øádek.
 * @param wasSpace Zda byla pøed aktuálním znakem mezera
 * @return Vrací, zda byl text zalomen na nový øádek.
 */
bool spl(int c, unsigned long int N, bool wasSpace)
{
  static unsigned long int sentences = 0;
  static bool wasDot = false;

  if (c == '.' && !wasDot)
  { // znak je teèka, pøedchozí znak teèka nebyl -> pøi posloupnosti teèek nepoèítá ka¾dou
    wasDot = true;
  }
  else if ((c != '.' && wasDot) || (wasSpace && wasDot))
  { // znak neni teèka, pøedchozí znak teèka byl, nebo byla mezera a pøed ní teèka (. . .)
    ++sentences;
    if (c == '.')
    { // aktuální znak je teèka
      wasDot = true;
    }
    else
    {
      wasDot = false;
    }

    if (sentences == N)
    { // na øádku u¾ je zvolený poèet vìt
      sentences = 0;
      putchar('\n');
      return true;
    }

    if (!wasSpace)
    { // pokud neni ¾a teèkou mezera, dá ji tam
    putchar(' ');
    return true;
    }
  }

  return false;
} // konec funkce spl


/**
 * Pøi ka¾dém volání zaène ze standardního vstupu naèítat znaky.
 * Posloupnost bílých znakù nahradí jednou mezerou, pøípadnì zalomý øádky podle poètu slov/vìt.
 * Na posledním øádku mù¾e být slov ménì, ov¹em øádek nemù¾e být prázdný.
 * @param param Zda zalomit øádky, nebo jen odstranit bílé znaky.
 * @param N Poèet slov/vìt na øádek.
 */
void removeSpaces(int param, int N)
{
  int c;
  bool wasSpace = false, // promìnná na urèení mezery
       fsol = true; // first space on line | true - netiskni mezeru (zaèátek øádku) | false - vytiskni mezeru

  while ((c = getchar()) != EOF)
  {
    if (isspace(c))
    { // naètený znak je bílý znak
      wasSpace = true;
    }
    else
    { // naètený znak neni bílý znak
      if (param == CWPL)
      { // je urèený poèet slov na øádek
        if (!fsol)
        { // ignorace mezer na zaèátku
          fsol = wpl(N, wasSpace);  // true - nový øádek, netiskni mezeru | false - vytiskni právì jednu mezeru
        }
      }
      else if (param == CSPL)
      { // je urèený poèet vìt na øádek
        if (fsol)
        { // ignorace mezer na zaèátku, pokud je první nemezera teèka, bere ji to jako vìtu
          wasSpace = false;
        }
        fsol = spl(c, N, wasSpace);  // true - nový øádek, netiskni mezeru | false - vytiskni právì jednu mezeru
      }

      if (wasSpace && !fsol)
      { // pokud byla na vstupu mezera a nejedná se o zaèátek øádku
        putchar(' ');
      }

      putchar(c);
      wasSpace = false;
      fsol = false;
    }
  }
} // konec funkce removeSpaces


/* konec project1.c */

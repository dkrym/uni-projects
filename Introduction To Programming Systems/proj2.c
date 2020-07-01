/*
 * Soubor:  proj2.c
 * Datum:   2009/11/20
 * Autor:   David Krym
 * Projekt: Iteraèní výpoèty, projekt è. 2 pro pøedmìt IZP
 * Popis:   Poèítá logaritmus, obecnou mocninu, harmonický prùmìr a standardní odchylku
 *
 * Zadání roz¹íøeno o výpoèet urèitého integrálu obdélníkovou metodou.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <errno.h>

/** Kódy chyb programu */
enum tecodes
{
  EOK = 0,     /**< Bez chyby. */
  ECLWRONG,    /**< Chybný pøíkazový øádek. */
  EAZERO,      /**< Parametr a je nula. */
  EANOTNUM,    /**< Parametr a neni èíslo. */
  EATOOBIG,    /**< Parametr a je mimo rozsah. */
  ECLNOTN,     /**< Parametr neni zadán. */
  ESIGNOTNUM,  /**< Parametr sigdigit neni cislo. */
  ESIGTOOBIG,  /**< Parametr sigdigit je mimo rozsah double. */
  ECLINTEGRAL, /**< Parametry pro výpoèet integrálu ¹patnì zadány. */
  EUNKNOWN,    /**< Neznámá chyba. */
};

/** Kód funkce pro výpoèet integrálu */
enum tintegr
{
  ILOGAX = 0, /**< Bude se poèítat integrál z logaritmu */
  IPOWAX,     /**< Bude se poèítat integrál z mocniny */
};

/** Stavové kódy programu */
enum tstates
{
  CHELP,       /**< Nápovìda */
  CPOWAX,      /**< Mocnina */
  CLOGAX,      /**< Logaritmus */
  CHM,         /**< Harmonický prùmìr */
  CSTDDEV,      /**< Standardní odchylka */
  CINTEGRAL,    /**< Integrál obdélníkovou metodou*/
};


/** Chybová hlá¹ení odpovídající chybovým kódùm. */
const char *ECODEMSG[] =
{
  /* EOK */
  "Vse v poradku.\n",
  /* ECLWRONG */
  "Chybne parametry prikazoveho radku!\n",
  /* EAZERO */
  "Zaklad \"a\" musi byt vetsi nez nula!\n",
  /* EANOTNUM */
  "Jako zaklad \"a\" nebylo zadano cislo!\n",
  /* EATOOBIG */
  "Hodnota zakladu \"a\" je mimo rozsah double!\n",
  /* ECLNOTN */
  "Nebyl zadan parametr a nebo sigdigit!\n",
  /* ESIGNOTNUM */
  "Parametr sigdigit neni cislo!\n",
  /* ESIGTOOBIG */
  "Parametr sigdigit musi byt cele cislo v intervalu <1,15>!\n",
  /* ECLINTEGRAL */
  "Parametry pro vypocet integralu byly spatne zadany!\n",
  
  "Nastala nepredvidatelna chyba!\n",
};


/**
 * Struktura obsahující hodnoty parametrù pøíkazové øádky.
 */
typedef struct params
{
  double a;              /**< Hodnota základu z pøíkazové øádky. */
  int sigdigit;          /**< Poèet platných cifer. */
  int ecode;             /**< Chybový kód programu, odpovídá výètu tecodes. */
  int state;             /**< Stavový kód programu, odpovídá výètu tstates. */
  int integr;            /**< Ze které funkce se bude poèítat integrál. */
} TParams;


/**
 * Struktura obsahující hodnoty pro harmonický prùmìr a standardní ochylku
 */
typedef struct values
{
  int N; // pocet clenu
  double Xi; // aktualni clen
  double Xall; // soucet Xi clenu
  double Xaux; // mezivýsledek pro Xka
  double auxResult; // mezivýsledek
} Tvalues;


/**
 * Struktura obsahující hodnoty pro výpoèet urèitého integrálu
 */
typedef struct integral
{
  double a; // mez 1
  double b; // mez 2
  double n; // poèet dílkù
  int sigdigit; // platné cifry pro fce logax a powax
  double base;  // základ
  int func;   // funkce pro integraci
} Tintegral;


/**
 * Eulerovo èíslo - konstanta
 */
const double IZP_E = 2.7182818284590452354;        // e


/////////////////////////////////////////////////////////////////
///////// Prototypy funkcí

void printECode(int ecode);
TParams getParams(int argc, char *argv[]);
void printHelp(void);
void loadLoop(double a, int sigdigit, int state);
void loadIntegral(Tintegral *item);
Tvalues myHm(Tvalues item);
Tvalues myStdDev(Tvalues item);
double myLnx(double x, double sigdigit);
double myLogax(double x, double a, double sigdigit);
double myPowax(double x, double a, double sigdigit);
int testSigdigit(char *numstr, int *sigdigit);
int testA(char *numstr, double *a);
double SIntegral(Tintegral *item, double (*MyFce)());

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
  else if (params.state == CINTEGRAL)
  {
    Tintegral integ =
    { // inicializace struktury
      .a = 0.0, // mez 1
      .b = 0.0, // mez 2
      .n = 0.0, // poèet obdélníkù
      .sigdigit = params.sigdigit, // platné cifry pro fce logax a powax
      .base = params.a,  // základ funkce
      .func = params.integr,  // funkce ze které se bude poèítat integral
     };
    loadIntegral(&integ);
  }
  else
  {
    loadLoop(params.a, params.sigdigit, params.state);
  }
  
  return EXIT_SUCCESS;
}
/////////////////////////////////////////////////////////////////
///////// Funkce

/**
 * Zkusí pøevést textový øetìzec na èíslo pomocí funkce strtol.
 * @param numstr Ukazatel na øetìzec, který chceme pøevést.
 * @param sigdigit Ukazatel na integer, kam pøípadnì ulo¾íme èíslo.
 * @return Vrací chybový kód.
 */
int testSigdigit(char *numstr, int *sigdigit)
{
  int ecode = EOK;
  
  if (!isdigit(numstr[0]))
  { // první znak neni èíslo
    ecode = ESIGNOTNUM;
    return ecode;
  }

  char *endptr;
  errno = 0; // aby nás neru¹ily pøedchozí chyby
  // pokus o pøevod
  int cislo = strtol(numstr, &endptr, 10);
  int errcode = errno; // jen pro jistotu, kdybych èasem pøidával dal¹í pøíkazy

  if ((errcode == ERANGE && cislo == INT_MAX) ||
      (errcode != 0 && cislo == 0) ||
      (cislo > 15))
  { // hodnota je mimo rozsah
    ecode = ESIGTOOBIG;
  }
  else if (endptr == numstr)
  { // oba ukazatele ukazují na zaèátek øetìzce, nic nebylo zadáno
    ecode = ECLNOTN;
  }
  else 
  {
    if (*endptr != '\0')  // za èíslem jsou dal¹í znaky, nìkdy nemusí jít o chybu
    { 
      ecode = ESIGNOTNUM;
    }
  }

  if (ecode == EOK)
  {
    if (cislo == 0)
    { // sigdigit musí být vìt¹í ne¾ 0
      ecode = ESIGTOOBIG;
    }
    else
    {
      *sigdigit = cislo;
    }
  }

  return ecode;
} // konec funkce testSigdigit


/**
 * Zkusí pøevést textový øetìzec na èíslo pomocí funkce strtod.
 * @param numstr Ukazatel na øetìzec, který chceme pøevést.
 * @param a Ukazatel na double, kam pøípadnì ulo¾íme èíslo.
 * @return Vrací chybový kód.
 */
int testA(char *numstr, double *a)
{
  int ecode = EOK;
  
  if (!isdigit(numstr[0]))
  { // první znak neni èíslo
    ecode = EANOTNUM;
    return ecode;
  }

  char *endptr;
  errno = 0; // aby nás neru¹ily pøedchozí chyby
  // pokus o pøevod
  double cislo = strtod(numstr, &endptr);
  int errcode = errno; // jen pro jistotu, kdybych èasem pøidával dal¹í pøíkazy

  if ((errcode == ERANGE && cislo == DBL_MAX) ||
      (errcode != 0 && cislo == 0))
  { // hodnota je mimo rozsah
    ecode = EATOOBIG;
  }
  else if (endptr == numstr)
  { // oba ukazatele ukazují na zaèátek øetìzce, nic nebylo zadáno
    ecode = ECLNOTN;
  }
  else 
  {
    if (*endptr != '\0')  // za èíslem jsou dal¹í znaky, nìkdy nemusí jít o chybu
    { 
      ecode = EANOTNUM;
    }
  }

  if (ecode == EOK)
  {
    if (cislo == 0)
    { // 'a' musí být vìt¹í ne¾ 0
      ecode = EAZERO;
    }
    else
    {
      *a = cislo;
    }
  }

  return ecode;
} // konec funkce testA


/**
 * Pomocná funkce pro výpoèet urèitého integrálu.
 * Funkce naèítá a vypisuje vstupy/výstupy pro funkci na výpoèet.
 * @param item Struktura s hodnotama z parametrù programu.
 **             item.a Mez 1
 **             item.b Mez 2
 **             item.n Poèet obdélníkù
 **             item.sigdigit Pøesnost poèítané funkce (powax, logax)
 **             item.base Základ funkce
 **             item.func Pro jakou funkci se bude poèítat
 * @return void
 */
void loadIntegral(Tintegral *item)
{
  int code; // návratový kód funkce scanf
  double a; // mez 1
  double b; // mez 2
  double n; // poèet obdélníkù
  double result = NAN;  // výsledek
  
  code = scanf("%lf %lf %lf", &n, &a, &b);

  if (code == 3)
  { // naèteny 3 èísla
    item->a = a;
    item->b = b;
    item->n = n;
    
      if (item->func == IPOWAX)
      { // výpoèet pro obecnou mocninu
        result = SIntegral(item, myPowax);
      }
      else if (item->func == ILOGAX)
      { // výpoèet pro obecný logaritmus
        result = SIntegral(item, myLogax);
      }
  }
  // výsledek na 10 desetinných míst
  printf("%.10e\n", result);
} // konec funkce loadIntegral


/**
 * Naèítá èísla ze standardního vstupu, výsledky tiskne na std. výstup.
 * Dle zadaného parametru volá funkce pro výpoèet.
 * @param a Základ pro logaritmus a mocninu.
 * @param sigdigit Poèet platných cifer výsledku.
 * @param state Zadaný parametr, urèuje matematickou operaci.
 * @return Void.
 */
void loadLoop(double a, int sigdigit, int state)
{
  double input; // èíslo ze vstupu
  int code; // návratový kód pro scanf
  double result;  // výsledek
  Tvalues hmsdResult =
  { // inicializace struktury
    .N = 0,
    .Xi = 0,
    .Xall = 0,
    .Xaux = 0,
    .auxResult = 0,
  };
  
  while ((code = scanf("%lf", &input)) != EOF)
  {
    if (code == 0)
    { // pokud neni zadáno èíslo, nastaví se NAN
      input = NAN;
      scanf("%*s");
    }
    
    if (state == CHM)
    { // harmonický prùmìr
      hmsdResult.Xi = input;
      hmsdResult = myHm(hmsdResult);
      result = hmsdResult.auxResult;
    }
    else if (state == CSTDDEV)
    { // standardní odchylka
      hmsdResult.Xi = input;
      hmsdResult = myStdDev(hmsdResult);  // pøedána struktura s pøedch. výsledky
      result = hmsdResult.auxResult;
    }
    else if (state == CLOGAX)
    { // logaritmus
      double resLog = myLogax(input, a, sigdigit);
      result = resLog;
    }
    else if (state == CPOWAX)
    { // mocnina
      double resPow = myPowax(input, a, sigdigit);
      result = resPow;
    }
    // výsledek na 10 desetinných míst
    printf("%.10e\n", result);
  }
} // konec funkce loadLoop


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
    .a = 0,
    .sigdigit = 0,
    .ecode = EOK,
    .state = CHELP,
    .integr = ILOGAX,
  };
  int sigdigit = 0;
  double a = 0.0;

  if (argc == 2 && strcmp("-h", argv[1]) == 0)
  { // tisk nápovìdy
    result.state = CHELP;
  }
  else if (argc == 2 && strcmp("--hm", argv[1]) == 0)
  { // dva parametry, harmonický prùmìr
    result.state = CHM;
  }
  else if (argc == 2 && strcmp("--stddev", argv[1]) == 0)
  { // dva parametry, standardní odchylka
    result.state = CSTDDEV;
  }
  else if (argc == 4)
  { // ètyøi parametry
    int esig = testSigdigit(argv[2], &sigdigit);  // pøevod na int, vrácen je chybový kód
    int ea = testA(argv[3], &a);  // pøevod na double
    if (esig != EOK)
    { // pokud se nepodaøilo pøevést parametr sigdigit
      result.ecode = esig;
    }
    else if (ea != EOK)
    { // pokud se nepodaøilo pøevést parametr 'a'
      result.ecode = ea;
    }
    else
    { // pøevedení probìhlo úspì¹nì
    result.sigdigit = sigdigit;
    result.a = a;
    }
    
    if (strcmp("--powax", argv[1]) == 0)
    { // bude se poèítat mocnina
      result.state = CPOWAX;
    }
    else if (strcmp("--logax", argv[1]) == 0)
    { // bude se poèítat logaritmus
      result.state = CLOGAX;
    }
    else
    { // jiný parametr = chyba
      result.ecode = ECLWRONG;
    }  
  }
  else if (argc == 5)
  { // --integral [logax|powax] sigdigit base
    int esig = testSigdigit(argv[3], &sigdigit);  // pøevod na int
    int ea = testA(argv[4], &a);  // pøevod na double
    if (esig != EOK)
    { // pokud se nepodaøilo pøevést parametr sigdigit
      result.ecode = esig;
    }
    else if (ea != EOK)
    { // pøevedení probìhlo úspì¹nì
      result.ecode = ea;
    }
    else
    { // pøevedení probìhlo úspì¹nì
    result.sigdigit = sigdigit;
    result.a = a;
    }
    
    if (strcmp("--integral", argv[1]) == 0)
    { // bude se poèítat integrál
      result.state = CINTEGRAL;
      if (strcmp("logax", argv[2]) == 0)
      { // z logaritmu
        result.integr = ILOGAX;
      }
      else if (strcmp("powax", argv[2]) == 0)
      { // z mocniny
        result.integr = IPOWAX;
      }
      else
      { // chyba
        result.ecode = ECLWRONG;
      }
    }
    else
    { // chybné parametry
      result.ecode = ECLWRONG;
    }
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
      "* Program: Iteracni vypocty\n"
      "* Autor: David Krym (c) 2009\n"
      "* \n"
      "* Program pocita logaritmus, mocninu, harmonicky prumer a standardni odchylku.\n"
      "* Cisla se nacitaji ze std. vstupu, vysledky prubezne tiskne na std. vystup.\n"
      "\n"
      " Pouziti:\n"
      "\tproj2 -h\n"
      "\tproj2 --powax sigdigit a\n"
      "\tproj2 --logax sigdigit a\n"
      "\tproj2 --hm\n"
      "\tproj2 --stddev\n"
      "\tproj2 --integral [powax|logax] sigdigit base\n"
      "\n"
      " Popis parametru:\n"
      "\t-h\t\t\t Vypise tuto obrazovku s napovedou.\n"
      "\t--powax sigdigit a\t Obecna mocnina o zakladu 'a' na pocet platnych cifer 'sigdigit'.\n"
      "\t--logax sigdigit a\t Obecny logaritmus o zakladu 'a' na pocet platnych cifer 'sigdigit'.\n"
      "\t--hm\t\t\t Vypocet harmonickeho prumeru.\n"
      "\t--stddev\t\t Vypocet vyberove smerodatne odchylky.\n\n"
      "\t--integral [powax|logax] sigdigit base\n"
      "\t\t\t\t Vypocet urciteho integralu logaritmu nebo mocniny,\n"
      "\t\t\t\t fce o zakladu 'base' a na pocet platnych cifer 'sigdigit'.\n"
      "\n"
      " Premiova funkce na vypocet urciteho integralu:\n"
      "\tFunkce ocekava na standardnim vstupu 3 cisla: 'n a b'\n"
      "\t n - Pocet obdelniku, na ktere se zadany interval rozdeli (obdelnikova metoda vypoctu)\n"
      "\t a - Prvni mez intervalu pro vypocet urciteho integralu\n"
      "\t b - Druha mez intervalu pro vypocet urciteho integralu\n"
      "\n"
  );

} // konec funkce printHelp


/**
 * Výpoèet harmonického prùmìru, mezivýsleky ukládá do struktury Tvalues.
 * @param item Struktura s mezivýsledkama z pøedchozího volání.
 **             item.Xi Aktuálnì naètené èíslo
 **             item.N Poèet ji¾ naètených èlenù
 **             item.Xall Souèet pøevrácených hodnot naètených èlenù
 **             item.auxResult Aktuální výsledek
 * @return Vrací mezivýsleky pro dal¹í volání.
 */
Tvalues myHm(Tvalues item)
{
  if (item.Xi <= 0)
  { // HM je jen pro èísla vìt¹í ne¾ nula
    item.Xi = NAN;
  }
  // HM = N / sum(1/Xi)
  item.N += 1;
  item.Xall += 1.0 / item.Xi; 
  item.auxResult = item.N / item.Xall;
  
  return item;
}


/**
 * Výpoèet standardní odchylky, mezivýsleky ukládá do struktury Tvalues.
 * @param item Struktura s mezivýsledkama z pøedchozího volání.
 **             item.Xi Aktuálnì naètené èíslo
 **             item.N Poèet ji¾ naètených èlenù
 **             item.Xall Souèet pøevrácených hodnot naètených èlenù
 **             item.auxResult Aktuální výsledek
 * @return Vrací mezivýsleky pro dal¹í volání.
 */
Tvalues myStdDev(Tvalues item)
{
  double am; // aritmetic mean
  
  item.Xall += item.Xi; // suma Xi
  item.N += 1;  // poèet èísel
  am = item.Xall / item.N; // aktualní aritmetický prùmìr
  // stddev= sqrt(1/N-1 * (sum(Xi^2) - N*aritm(x)^2))
  item.Xaux += item.Xi * item.Xi; // suma druhých mocnin Xi;
  item.auxResult = sqrt((item.Xaux - item.N * am * am) / (item.N - 1));

  return item;
}


/**
 * Výpoèet obecné mocniny.
 * @param x Exponent.
 * @param a Základ.
 * @param sigdigit Poèet platných cifer.
 * @return Umocnìné èíslo a^x na zadaný poèet platných cifer.
 */
double myPowax(double x, double a, double sigdigit)
{
  if (a < 0 || isinf(fabs(x)) || isnan(x))
  { // test definièního oboru
    return NAN;
  }
  
  double tj;  // aktualni clen rady
  double totalSum; // souèet øady
  double Y;   // pomocna promenna
  double j = 0;     // pomocnej citac
  double number;  // celá èást èísla
  double tenths;  // desetiny èísla
  double e = 1;
  double eps = 0.1;
  
  for (int i = 1; i < sigdigit; i++)
  { // pøepoèet sigdigit na eps jako 0.1^N
    eps *= 0.1;
  }
  
  if (x < 0)
  { // pro záporný exponent
    a = 1.0 / a;
    x = x * (-1.0);
  }
  // a^(c+d) = a^c * a^d
  number = trunc(x);  //celá èást èísla
  tenths = x - number;  //desetinná èást èísla
  
  for (int i = 0; i < number; i++)
  { // celá èást exponentu cyklem
    e *= a;
    if (isinf(e))
    { // pokud pøesáhnu rozsah double, vrátim INF
      return INFINITY;
    }
    else if (isinf(e) == -1 || e == 0.0)
    { // pokud pøesáhnu rozsah double, vrátim -INF
      return -INFINITY;
    }
  }
  // poèáteèní podmínky pro aprox. fci
  tj = 1;
  totalSum = tj;
  Y = tenths * myLnx(a, sigdigit);
  
  // desetinná èást exponentu pøes aprox. fci
  while (fabs(tj) >= (eps * fabs(totalSum)))
  { // výpoèet odvozeným rekurentnim vztahem
    j++;
    tj *= Y / j;
    totalSum += tj;
  }
  // výsledek = celá èást exp. * desetinná èást exp.
  return (totalSum * e);
}


/**
 * Výpoèet obecného logaritmu
 * @param x Èíslo.
 * @param a Základ.
 * @param sigdigit Poèet platných cifer.
 * @return Log èísla x o základu a na zadaný poèet platných cifer.
 */
double myLogax(double x, double a, double sigdigit)
{
  if (a == 1 || x<= 0)
  { //log neni pro 1 definován
    return NAN;
  }
  
  double logarithm;
  logarithm = myLnx(x, sigdigit) / myLnx(a, sigdigit);

  return logarithm;
}


/**
 * Výpoèet pøirozeného logaritmu
 * @param x Èíslo.
 * @param sigdigit Poèet platných cifer.
 * @return Pøirozený log èísla x na zadaný poèet platných cifer.
 */
double myLnx(double x, double sigdigit)
{ // o¹etøení výjimeèných stavù
  if (x < 0.0 || isnan(x))
  {
    return NAN;
  }
  else if (x == 0.0)
  {
    return -INFINITY;
  }
  else if (isinf(x))
  {
    return INFINITY;
  }
  
  double Yj;  //substituce
  double tj;  // aktualni clen rady
  double totalSum; // souèet øady
  double kj;     // pomocný èítaè
  double e = 0;  // pricteme nakonci -> ln e o zakladu e = 1
  double eps = 0.1;
  
  for (int i = 1; i < sigdigit; i++)
  { // pøepoèet sigdigit na eps jako 0.1^N
    eps *= 0.1;
  }
  
  while (x > IZP_E)
  { // zmen¹ení èísla dìlením Eulerovým èíslem
    x /= IZP_E;
    e++;
  }
  while (x < 1)
  { // malá èí¹la ¹patnì konvergují, proto úpravar
    x *= IZP_E;
    e--;
  }
  // poèáteèní hodnoty
  Yj = (x - 1.0) / (x + 1.0);  // Y0
  kj = 1.0;   // k0
  tj = Yj;  // t0 -> první èlen øady
  totalSum = tj;  // souèet øady je jen prvni clen
  
  while (fabs(tj) > eps * fabs(totalSum))
  { // výpoèet odvozeným rekurentním vztahem
    kj += 2.0;  // kj = kj-1 +2
    Yj *= ((x - 1) * (x - 1)) / ((x + 1) * (x + 1));
    tj = Yj * 1 / kj; // aktualni clen rady
    totalSum += tj; // soucet rady
  }
  
  return (2 * totalSum + e);
}

/**
 * Výpoèet urèitého integrálu obdélníkovou metodou
 * @param item Ukazatel na strukturu s hodnotama pro výpoèet.
 **             item.a První mez integrálu
 **             item.b Druhá mez integrálu
 **             item.n Poèet dílkù, na které bude interval rozdìlen
 **             item.sigdigit Pøesnost výsledku poèítané funkce
 **             item.base Základ pro poèítanou funkci
 * @param MyFce Ukazatel na funkci, ze které se poèítá urèitý integrál.
 * @return Urèitý integrál ze zadané funkce.
 */
double SIntegral(Tintegral *item, double (*MyFce) ())
{
  double result = 0;
  double dx = fabs(item->b - item->a) / item->n;  // delta x, ¹íøka jednoho obdélníku

  for (double i = item->a; i < item->b; i += dx)
  { // suma obsahù obdélníkù zadané funkce
    result += dx * MyFce(i, item->base, item->sigdigit);
  }

  return result;
}
/* konec proj1.c */
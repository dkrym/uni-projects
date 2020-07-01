/*
 * Soubor:  proj2.c
 * Datum:   2009/11/20
 * Autor:   David Krym
 * Projekt: Itera�n� v�po�ty, projekt �. 2 pro p�edm�t IZP
 * Popis:   Po��t� logaritmus, obecnou mocninu, harmonick� pr�m�r a standardn� odchylku
 *
 * Zad�n� roz���eno o v�po�et ur�it�ho integr�lu obd�ln�kovou metodou.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <errno.h>

/** K�dy chyb programu */
enum tecodes
{
  EOK = 0,     /**< Bez chyby. */
  ECLWRONG,    /**< Chybn� p��kazov� ��dek. */
  EAZERO,      /**< Parametr a je nula. */
  EANOTNUM,    /**< Parametr a neni ��slo. */
  EATOOBIG,    /**< Parametr a je mimo rozsah. */
  ECLNOTN,     /**< Parametr neni zad�n. */
  ESIGNOTNUM,  /**< Parametr sigdigit neni cislo. */
  ESIGTOOBIG,  /**< Parametr sigdigit je mimo rozsah double. */
  ECLINTEGRAL, /**< Parametry pro v�po�et integr�lu �patn� zad�ny. */
  EUNKNOWN,    /**< Nezn�m� chyba. */
};

/** K�d funkce pro v�po�et integr�lu */
enum tintegr
{
  ILOGAX = 0, /**< Bude se po��tat integr�l z logaritmu */
  IPOWAX,     /**< Bude se po��tat integr�l z mocniny */
};

/** Stavov� k�dy programu */
enum tstates
{
  CHELP,       /**< N�pov�da */
  CPOWAX,      /**< Mocnina */
  CLOGAX,      /**< Logaritmus */
  CHM,         /**< Harmonick� pr�m�r */
  CSTDDEV,      /**< Standardn� odchylka */
  CINTEGRAL,    /**< Integr�l obd�ln�kovou metodou*/
};


/** Chybov� hl�en� odpov�daj�c� chybov�m k�d�m. */
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
 * Struktura obsahuj�c� hodnoty parametr� p��kazov� ��dky.
 */
typedef struct params
{
  double a;              /**< Hodnota z�kladu z p��kazov� ��dky. */
  int sigdigit;          /**< Po�et platn�ch cifer. */
  int ecode;             /**< Chybov� k�d programu, odpov�d� v��tu tecodes. */
  int state;             /**< Stavov� k�d programu, odpov�d� v��tu tstates. */
  int integr;            /**< Ze kter� funkce se bude po��tat integr�l. */
} TParams;


/**
 * Struktura obsahuj�c� hodnoty pro harmonick� pr�m�r a standardn� ochylku
 */
typedef struct values
{
  int N; // pocet clenu
  double Xi; // aktualni clen
  double Xall; // soucet Xi clenu
  double Xaux; // meziv�sledek pro Xka
  double auxResult; // meziv�sledek
} Tvalues;


/**
 * Struktura obsahuj�c� hodnoty pro v�po�et ur�it�ho integr�lu
 */
typedef struct integral
{
  double a; // mez 1
  double b; // mez 2
  double n; // po�et d�lk�
  int sigdigit; // platn� cifry pro fce logax a powax
  double base;  // z�klad
  int func;   // funkce pro integraci
} Tintegral;


/**
 * Eulerovo ��slo - konstanta
 */
const double IZP_E = 2.7182818284590452354;        // e


/////////////////////////////////////////////////////////////////
///////// Prototypy funkc�

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
  else if (params.state == CINTEGRAL)
  {
    Tintegral integ =
    { // inicializace struktury
      .a = 0.0, // mez 1
      .b = 0.0, // mez 2
      .n = 0.0, // po�et obd�ln�k�
      .sigdigit = params.sigdigit, // platn� cifry pro fce logax a powax
      .base = params.a,  // z�klad funkce
      .func = params.integr,  // funkce ze kter� se bude po��tat integral
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
 * Zkus� p�ev�st textov� �et�zec na ��slo pomoc� funkce strtol.
 * @param numstr Ukazatel na �et�zec, kter� chceme p�ev�st.
 * @param sigdigit Ukazatel na integer, kam p��padn� ulo��me ��slo.
 * @return Vrac� chybov� k�d.
 */
int testSigdigit(char *numstr, int *sigdigit)
{
  int ecode = EOK;
  
  if (!isdigit(numstr[0]))
  { // prvn� znak neni ��slo
    ecode = ESIGNOTNUM;
    return ecode;
  }

  char *endptr;
  errno = 0; // aby n�s neru�ily p�edchoz� chyby
  // pokus o p�evod
  int cislo = strtol(numstr, &endptr, 10);
  int errcode = errno; // jen pro jistotu, kdybych �asem p�id�val dal�� p��kazy

  if ((errcode == ERANGE && cislo == INT_MAX) ||
      (errcode != 0 && cislo == 0) ||
      (cislo > 15))
  { // hodnota je mimo rozsah
    ecode = ESIGTOOBIG;
  }
  else if (endptr == numstr)
  { // oba ukazatele ukazuj� na za��tek �et�zce, nic nebylo zad�no
    ecode = ECLNOTN;
  }
  else 
  {
    if (*endptr != '\0')  // za ��slem jsou dal�� znaky, n�kdy nemus� j�t o chybu
    { 
      ecode = ESIGNOTNUM;
    }
  }

  if (ecode == EOK)
  {
    if (cislo == 0)
    { // sigdigit mus� b�t v�t�� ne� 0
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
 * Zkus� p�ev�st textov� �et�zec na ��slo pomoc� funkce strtod.
 * @param numstr Ukazatel na �et�zec, kter� chceme p�ev�st.
 * @param a Ukazatel na double, kam p��padn� ulo��me ��slo.
 * @return Vrac� chybov� k�d.
 */
int testA(char *numstr, double *a)
{
  int ecode = EOK;
  
  if (!isdigit(numstr[0]))
  { // prvn� znak neni ��slo
    ecode = EANOTNUM;
    return ecode;
  }

  char *endptr;
  errno = 0; // aby n�s neru�ily p�edchoz� chyby
  // pokus o p�evod
  double cislo = strtod(numstr, &endptr);
  int errcode = errno; // jen pro jistotu, kdybych �asem p�id�val dal�� p��kazy

  if ((errcode == ERANGE && cislo == DBL_MAX) ||
      (errcode != 0 && cislo == 0))
  { // hodnota je mimo rozsah
    ecode = EATOOBIG;
  }
  else if (endptr == numstr)
  { // oba ukazatele ukazuj� na za��tek �et�zce, nic nebylo zad�no
    ecode = ECLNOTN;
  }
  else 
  {
    if (*endptr != '\0')  // za ��slem jsou dal�� znaky, n�kdy nemus� j�t o chybu
    { 
      ecode = EANOTNUM;
    }
  }

  if (ecode == EOK)
  {
    if (cislo == 0)
    { // 'a' mus� b�t v�t�� ne� 0
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
 * Pomocn� funkce pro v�po�et ur�it�ho integr�lu.
 * Funkce na��t� a vypisuje vstupy/v�stupy pro funkci na v�po�et.
 * @param item Struktura s hodnotama z parametr� programu.
 **             item.a Mez 1
 **             item.b Mez 2
 **             item.n Po�et obd�ln�k�
 **             item.sigdigit P�esnost po��tan� funkce (powax, logax)
 **             item.base Z�klad funkce
 **             item.func Pro jakou funkci se bude po��tat
 * @return void
 */
void loadIntegral(Tintegral *item)
{
  int code; // n�vratov� k�d funkce scanf
  double a; // mez 1
  double b; // mez 2
  double n; // po�et obd�ln�k�
  double result = NAN;  // v�sledek
  
  code = scanf("%lf %lf %lf", &n, &a, &b);

  if (code == 3)
  { // na�teny 3 ��sla
    item->a = a;
    item->b = b;
    item->n = n;
    
      if (item->func == IPOWAX)
      { // v�po�et pro obecnou mocninu
        result = SIntegral(item, myPowax);
      }
      else if (item->func == ILOGAX)
      { // v�po�et pro obecn� logaritmus
        result = SIntegral(item, myLogax);
      }
  }
  // v�sledek na 10 desetinn�ch m�st
  printf("%.10e\n", result);
} // konec funkce loadIntegral


/**
 * Na��t� ��sla ze standardn�ho vstupu, v�sledky tiskne na std. v�stup.
 * Dle zadan�ho parametru vol� funkce pro v�po�et.
 * @param a Z�klad pro logaritmus a mocninu.
 * @param sigdigit Po�et platn�ch cifer v�sledku.
 * @param state Zadan� parametr, ur�uje matematickou operaci.
 * @return Void.
 */
void loadLoop(double a, int sigdigit, int state)
{
  double input; // ��slo ze vstupu
  int code; // n�vratov� k�d pro scanf
  double result;  // v�sledek
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
    { // pokud neni zad�no ��slo, nastav� se NAN
      input = NAN;
      scanf("%*s");
    }
    
    if (state == CHM)
    { // harmonick� pr�m�r
      hmsdResult.Xi = input;
      hmsdResult = myHm(hmsdResult);
      result = hmsdResult.auxResult;
    }
    else if (state == CSTDDEV)
    { // standardn� odchylka
      hmsdResult.Xi = input;
      hmsdResult = myStdDev(hmsdResult);  // p�ed�na struktura s p�edch. v�sledky
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
    // v�sledek na 10 desetinn�ch m�st
    printf("%.10e\n", result);
  }
} // konec funkce loadLoop


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
    .a = 0,
    .sigdigit = 0,
    .ecode = EOK,
    .state = CHELP,
    .integr = ILOGAX,
  };
  int sigdigit = 0;
  double a = 0.0;

  if (argc == 2 && strcmp("-h", argv[1]) == 0)
  { // tisk n�pov�dy
    result.state = CHELP;
  }
  else if (argc == 2 && strcmp("--hm", argv[1]) == 0)
  { // dva parametry, harmonick� pr�m�r
    result.state = CHM;
  }
  else if (argc == 2 && strcmp("--stddev", argv[1]) == 0)
  { // dva parametry, standardn� odchylka
    result.state = CSTDDEV;
  }
  else if (argc == 4)
  { // �ty�i parametry
    int esig = testSigdigit(argv[2], &sigdigit);  // p�evod na int, vr�cen je chybov� k�d
    int ea = testA(argv[3], &a);  // p�evod na double
    if (esig != EOK)
    { // pokud se nepoda�ilo p�ev�st parametr sigdigit
      result.ecode = esig;
    }
    else if (ea != EOK)
    { // pokud se nepoda�ilo p�ev�st parametr 'a'
      result.ecode = ea;
    }
    else
    { // p�eveden� prob�hlo �sp�n�
    result.sigdigit = sigdigit;
    result.a = a;
    }
    
    if (strcmp("--powax", argv[1]) == 0)
    { // bude se po��tat mocnina
      result.state = CPOWAX;
    }
    else if (strcmp("--logax", argv[1]) == 0)
    { // bude se po��tat logaritmus
      result.state = CLOGAX;
    }
    else
    { // jin� parametr = chyba
      result.ecode = ECLWRONG;
    }  
  }
  else if (argc == 5)
  { // --integral [logax|powax] sigdigit base
    int esig = testSigdigit(argv[3], &sigdigit);  // p�evod na int
    int ea = testA(argv[4], &a);  // p�evod na double
    if (esig != EOK)
    { // pokud se nepoda�ilo p�ev�st parametr sigdigit
      result.ecode = esig;
    }
    else if (ea != EOK)
    { // p�eveden� prob�hlo �sp�n�
      result.ecode = ea;
    }
    else
    { // p�eveden� prob�hlo �sp�n�
    result.sigdigit = sigdigit;
    result.a = a;
    }
    
    if (strcmp("--integral", argv[1]) == 0)
    { // bude se po��tat integr�l
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
    { // chybn� parametry
      result.ecode = ECLWRONG;
    }
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
 * V�po�et harmonick�ho pr�m�ru, meziv�sleky ukl�d� do struktury Tvalues.
 * @param item Struktura s meziv�sledkama z p�edchoz�ho vol�n�.
 **             item.Xi Aktu�ln� na�ten� ��slo
 **             item.N Po�et ji� na�ten�ch �len�
 **             item.Xall Sou�et p�evr�cen�ch hodnot na�ten�ch �len�
 **             item.auxResult Aktu�ln� v�sledek
 * @return Vrac� meziv�sleky pro dal�� vol�n�.
 */
Tvalues myHm(Tvalues item)
{
  if (item.Xi <= 0)
  { // HM je jen pro ��sla v�t�� ne� nula
    item.Xi = NAN;
  }
  // HM = N / sum(1/Xi)
  item.N += 1;
  item.Xall += 1.0 / item.Xi; 
  item.auxResult = item.N / item.Xall;
  
  return item;
}


/**
 * V�po�et standardn� odchylky, meziv�sleky ukl�d� do struktury Tvalues.
 * @param item Struktura s meziv�sledkama z p�edchoz�ho vol�n�.
 **             item.Xi Aktu�ln� na�ten� ��slo
 **             item.N Po�et ji� na�ten�ch �len�
 **             item.Xall Sou�et p�evr�cen�ch hodnot na�ten�ch �len�
 **             item.auxResult Aktu�ln� v�sledek
 * @return Vrac� meziv�sleky pro dal�� vol�n�.
 */
Tvalues myStdDev(Tvalues item)
{
  double am; // aritmetic mean
  
  item.Xall += item.Xi; // suma Xi
  item.N += 1;  // po�et ��sel
  am = item.Xall / item.N; // aktualn� aritmetick� pr�m�r
  // stddev= sqrt(1/N-1 * (sum(Xi^2) - N*aritm(x)^2))
  item.Xaux += item.Xi * item.Xi; // suma druh�ch mocnin Xi;
  item.auxResult = sqrt((item.Xaux - item.N * am * am) / (item.N - 1));

  return item;
}


/**
 * V�po�et obecn� mocniny.
 * @param x Exponent.
 * @param a Z�klad.
 * @param sigdigit Po�et platn�ch cifer.
 * @return Umocn�n� ��slo a^x na zadan� po�et platn�ch cifer.
 */
double myPowax(double x, double a, double sigdigit)
{
  if (a < 0 || isinf(fabs(x)) || isnan(x))
  { // test defini�n�ho oboru
    return NAN;
  }
  
  double tj;  // aktualni clen rady
  double totalSum; // sou�et �ady
  double Y;   // pomocna promenna
  double j = 0;     // pomocnej citac
  double number;  // cel� ��st ��sla
  double tenths;  // desetiny ��sla
  double e = 1;
  double eps = 0.1;
  
  for (int i = 1; i < sigdigit; i++)
  { // p�epo�et sigdigit na eps jako 0.1^N
    eps *= 0.1;
  }
  
  if (x < 0)
  { // pro z�porn� exponent
    a = 1.0 / a;
    x = x * (-1.0);
  }
  // a^(c+d) = a^c * a^d
  number = trunc(x);  //cel� ��st ��sla
  tenths = x - number;  //desetinn� ��st ��sla
  
  for (int i = 0; i < number; i++)
  { // cel� ��st exponentu cyklem
    e *= a;
    if (isinf(e))
    { // pokud p�es�hnu rozsah double, vr�tim INF
      return INFINITY;
    }
    else if (isinf(e) == -1 || e == 0.0)
    { // pokud p�es�hnu rozsah double, vr�tim -INF
      return -INFINITY;
    }
  }
  // po��te�n� podm�nky pro aprox. fci
  tj = 1;
  totalSum = tj;
  Y = tenths * myLnx(a, sigdigit);
  
  // desetinn� ��st exponentu p�es aprox. fci
  while (fabs(tj) >= (eps * fabs(totalSum)))
  { // v�po�et odvozen�m rekurentnim vztahem
    j++;
    tj *= Y / j;
    totalSum += tj;
  }
  // v�sledek = cel� ��st exp. * desetinn� ��st exp.
  return (totalSum * e);
}


/**
 * V�po�et obecn�ho logaritmu
 * @param x ��slo.
 * @param a Z�klad.
 * @param sigdigit Po�et platn�ch cifer.
 * @return Log ��sla x o z�kladu a na zadan� po�et platn�ch cifer.
 */
double myLogax(double x, double a, double sigdigit)
{
  if (a == 1 || x<= 0)
  { //log neni pro 1 definov�n
    return NAN;
  }
  
  double logarithm;
  logarithm = myLnx(x, sigdigit) / myLnx(a, sigdigit);

  return logarithm;
}


/**
 * V�po�et p�irozen�ho logaritmu
 * @param x ��slo.
 * @param sigdigit Po�et platn�ch cifer.
 * @return P�irozen� log ��sla x na zadan� po�et platn�ch cifer.
 */
double myLnx(double x, double sigdigit)
{ // o�et�en� v�jime�n�ch stav�
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
  double totalSum; // sou�et �ady
  double kj;     // pomocn� ��ta�
  double e = 0;  // pricteme nakonci -> ln e o zakladu e = 1
  double eps = 0.1;
  
  for (int i = 1; i < sigdigit; i++)
  { // p�epo�et sigdigit na eps jako 0.1^N
    eps *= 0.1;
  }
  
  while (x > IZP_E)
  { // zmen�en� ��sla d�len�m Eulerov�m ��slem
    x /= IZP_E;
    e++;
  }
  while (x < 1)
  { // mal� ��la �patn� konverguj�, proto �pravar
    x *= IZP_E;
    e--;
  }
  // po��te�n� hodnoty
  Yj = (x - 1.0) / (x + 1.0);  // Y0
  kj = 1.0;   // k0
  tj = Yj;  // t0 -> prvn� �len �ady
  totalSum = tj;  // sou�et �ady je jen prvni clen
  
  while (fabs(tj) > eps * fabs(totalSum))
  { // v�po�et odvozen�m rekurentn�m vztahem
    kj += 2.0;  // kj = kj-1 +2
    Yj *= ((x - 1) * (x - 1)) / ((x + 1) * (x + 1));
    tj = Yj * 1 / kj; // aktualni clen rady
    totalSum += tj; // soucet rady
  }
  
  return (2 * totalSum + e);
}

/**
 * V�po�et ur�it�ho integr�lu obd�ln�kovou metodou
 * @param item Ukazatel na strukturu s hodnotama pro v�po�et.
 **             item.a Prvn� mez integr�lu
 **             item.b Druh� mez integr�lu
 **             item.n Po�et d�lk�, na kter� bude interval rozd�len
 **             item.sigdigit P�esnost v�sledku po��tan� funkce
 **             item.base Z�klad pro po��tanou funkci
 * @param MyFce Ukazatel na funkci, ze kter� se po��t� ur�it� integr�l.
 * @return Ur�it� integr�l ze zadan� funkce.
 */
double SIntegral(Tintegral *item, double (*MyFce) ())
{
  double result = 0;
  double dx = fabs(item->b - item->a) / item->n;  // delta x, ���ka jednoho obd�ln�ku

  for (double i = item->a; i < item->b; i += dx)
  { // suma obsah� obd�ln�k� zadan� funkce
    result += dx * MyFce(i, item->base, item->sigdigit);
  }

  return result;
}
/* konec proj1.c */
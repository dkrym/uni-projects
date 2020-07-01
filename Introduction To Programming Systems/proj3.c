/*
 * Soubor:  proj3.c
 * Datum:   2009/11/29
 * Autor:   David Krym
 * Projekt: Maticové operace, projekt è. 3 pro pøedmìt IZP
 * Popis:   Poèítá souèet, souèin matic a výraz (A+A)*(B+B).
 *          Dále zji¹tuje souvislou matici a provádí LL prùchod.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define true 1
#define false 0

/** Kódy chyb programu */
enum tecodes
{
  EOK = 0,     /**< Bez chyby. */
  ECLWRONG,    /**< Chybné parametry. */
  EOPEN,       /**< Chyba pøi otevírání souboru.*/
  ECLOSE,      /**< Chyba pøi zavírání souboru. */
  EBADFILE,    /**< Soubor neodpovídá zadání. */  
  EALLOC,      /**< Chyba pøi alokaci. */
  ENORES,      /**< Operace s maticema nemá výsledek. */
  EUNKNOWN,    /**< Neznámá chyba. */
};


/** Chybová hlá¹ení odpovídající chybovým kódùm. */
const char *ECODEMSG[] =
{
  /* EOK */
  "Vse v poradku.\n",
  /* ECLWRONG */
  "Chybne parametry spusteni!\n",
  /* EOPEN */
  "Nepodarilo se otevrit soubor!\n",
  /* ECLOSE */
  "Nepodarilo se zavrit soubor!\n",
  /* EBADFILE */
  "Chybny format souboru!\n",
  /* EALLOC */
  "Nepodarilo se alokovat pamet!\n",
  /* ENORES */
  "Operace neni definovana!\n",

  "Nastala nepredvidatelna chyba!\n",
};


/** Stavové kódy programu */
enum tstates
{
  CHELP,       /**< Nápovìda */
  CADD,        /**< Souèet matic */
  CMULT,       /**< Násobení matic */
  CEXPR,       /**< Výraz (A+A) * (B+B) */
  CCONT,       /**< Souvislá matice */
  CLL,         /**< LL prùchod */
};


/**
 * Struktura obsahující hodnoty parametrù pøíkazové øádky.
 */
typedef struct params
{
  int state;             /**< Stavový kód programu, odpovídá výètu tstates. */
  char *fname1;          /**< Jméno souboru s první maticí. */
  char *fname2;          /**< Jméno souboru s druhou maticí. */
  int ecode;             /**< Chybový kód programu, odpovídá výètu tecodes. */
} TParams;


/**
 * Struktura obsahující hodnoty parametrù pøíkazové øádky.
 */
typedef struct tmatrix
{
  int rows;             /**< Poèet øádkù matice. */
  int cols;             /**< Poèet sloupcù matice. */
  int **matrix;          /**< Ukazatel na ukazatele, dynamické vícerozmìrné pole. */
} TMatrix;


/**
 * Struktura obsahující hodnoty otevøeného souboru.
 */
typedef struct tfile
{
  FILE *pfile;           /**< Ukazatel na otevøený soubor. */
  int ecode;             /**< Chybový kód programu, odpovídá výètu tecodes. */
} TFile;


/**
 * Struktura obsahující hodnoty delta pro pohyb v matici.
 */
typedef struct tincrement
{
  int dr, dc; // delta row, col
} TIncrement;


/**
 * Pole struktur obsahující smìry pohybu v matici.
 */
const TIncrement way[] =
{
  {0, -1}, // vlevo, 0
  {0, 1}, // vpravo
  {-1, 0}, // nahoru
  {1, 0}, // dolu
  {-1, -1}, // vlevo nahoru
  {-1, 1}, // vpravo nahoru
  {1, 1}, // vpravo dolu
  {1, -1}, // vlevo dolu, 7
};


/**
 * Kódy smìrù.
 */
enum tdirection
{
  LEFT = 0,
  RIGHT,
  UP,
  DOWN,
};


/////////////////////////////////////////////////////////////////
///////// Prototypy funkcí
TParams getParams(int argc, char *argv[]);
void printECode(int ecode);
void printHelp(void);
int loadInput(int state, char *fname1, char *fname2);
int loadMatrix(TMatrix *m, char *filename);
void printMatrix(const TMatrix *m);
int addMatrix(const TMatrix *m1, const TMatrix *m2, TMatrix *res);
int mulMatrix(const TMatrix *m1, const TMatrix *m2, TMatrix *res);
int exprMatrix(TMatrix *m1, TMatrix *m2, TMatrix *res);
int contMatrix(TMatrix *m);
int LLMatrix(TMatrix *m, TMatrix *res);
TFile tryOpen(char *name, char *mod);
int tryClose(FILE *closefile);
void freeMatrix(TMatrix *m);
TMatrix allocMatrix(int rows, int cols);
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
  else
  {
    int ecode = loadInput(params.state, params.fname1, params.fname2);
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
 * Dle zadaného parametru volá funkce pro výpoèet.
 * Pokud je formát argumentù chybný, ukonèí program s chybovým kódem.
 * @param state Zadaný parametr, co se bude poèítat.
 * @param fname1 Název prvního souboru s maticí.
 * @param fname1 Název druhého souboru s maticí.
 * @return Vrací chybový kód.
 */
int loadInput(int state, char *fname1, char *fname2)
{
   TMatrix m1;
   TMatrix m2;
   TMatrix m3;

  if (state == CCONT) // SOUVISLA MATICE
  { // pokus o naètení souboru do pole
    int ecode = loadMatrix(&m1, fname1);
    if (ecode != EOK)
    { // nìco nestandardního
      return ecode;
    }
    int result = contMatrix(&m1); // vrací true/false
    if (result == true)
    {
      printf("true\n");
    }
    else
    {
      printf("false\n");
    }
    freeMatrix(&m1);
  } // konec CCONT
  else if (state == CLL) // TERCOVA ROTACE
  { // pokus o naètení souboru do pole
    int ecode = loadMatrix(&m1, fname1);
    if (ecode != EOK)
    { // nìco nestandardního
      return ecode;
    }
    ecode = LLMatrix(&m1, &m2); // vrací chybový kód
    if (ecode != EOK)
    { // chyba pøi sèítání
      freeMatrix(&m1);
      freeMatrix(&m2);
      return ecode;
    }
    printMatrix(&m2); // vytiskne výsledek
    freeMatrix(&m1);
    freeMatrix(&m2);
  } // konec CDROT  
  else if (state == CADD) // SOUCET MATIC
  {
    int ecode = loadMatrix(&m1, fname1);
    if (ecode != EOK)
    { // nìco nestandardního
      return ecode;
    }
    ecode = loadMatrix(&m2, fname2);
    if (ecode != EOK)
    { // musíme uvolnit první matici
      freeMatrix(&m1);
      return ecode;
    }
    ecode = addMatrix(&m1, &m2, &m3);
    if (ecode != EOK)
    { // chyba pøi sèítání
      freeMatrix(&m1);
      freeMatrix(&m2);
      return ecode;
    }
    printMatrix(&m3); // vytiskne výsledek
    freeMatrix(&m1);
    freeMatrix(&m2);
    freeMatrix(&m3);
  } // konec CADD
  else if (state == CMULT)  // NASOBENI MATIC
  {
    int ecode = loadMatrix(&m1, fname1);
    if (ecode != EOK)
    { // nìco nestandardního
      return ecode;
    }
    ecode = loadMatrix(&m2, fname2);
    if (ecode != EOK)
    { // musíme uvolnit první matici
      freeMatrix(&m1);
      return ecode;
    }
    ecode = mulMatrix(&m1, &m2, &m3);
    if (ecode != EOK)
    { // chyba pøi sèítání
      freeMatrix(&m1);
      freeMatrix(&m2);
      return ecode;
    }
    printMatrix(&m3); // vytiskne výsledek
    freeMatrix(&m1);
    freeMatrix(&m2);
    freeMatrix(&m3);
  } // konec CMULT
  else if (state == CEXPR)  // (A+A)*(B+B)
  {
    int ecode = loadMatrix(&m1, fname1);
    if (ecode != EOK)
    { // nìco nestandardního
      return ecode;
    }
    ecode = loadMatrix(&m2, fname2);
    if (ecode != EOK)
    { // musíme uvolnit první matici
      freeMatrix(&m1);
      return ecode;
    }
    ecode = exprMatrix(&m1, &m2, &m3);
    if (ecode != EOK)
    { // chyba pøi sèítání
      freeMatrix(&m1);
      freeMatrix(&m2);
      return ecode;
    }
    printMatrix(&m3); // vytiskne výsledek
    freeMatrix(&m1);
    freeMatrix(&m2);
    freeMatrix(&m3);
  } // konec CEXPR

  return EOK;
} // konec funkce loadInput


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
  else if (argc == 3)
  { // tøi parametry
    if (strcmp("--cont", argv[1]) == 0)
    { // souvislá matice
      result.state = CCONT;
    }
    else if (strcmp("--LL", argv[1]) == 0)
    { // terèová rotace
      result.state = CLL;
    }
    else
    { // ¹patné parametry
      result.ecode = ECLWRONG;
    }
    
    result.fname1 = argv[2];
  }
  else if (argc == 4)
  { // ètyøi params
    if (strcmp("--mult", argv[1]) == 0)
    { // násobení matic
      result.state = CMULT;
    }
    else if (strcmp("--add", argv[1]) == 0)
    { // sèítání matic
      result.state = CADD;
    }
    else if (strcmp("--expr", argv[1]) == 0)
    { // zdaný výraz
      result.state = CEXPR;
    }
    else
    { // ¹patné parametry
      result.ecode = ECLWRONG;
    }
    
    result.fname1 = argv[2];
    result.fname2 = argv[3];
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

  if (ecode == ENORES)
  { // matematická operace neni definována
    printf("false\n");
  }
  else
  {
    fprintf(stderr, "%s", ECODEMSG[ecode]);
  }
} // konec funkce printECode


/**
 * Vytiskne nápovìdu.
 */
void printHelp(void)
{
  printf(
      "\n"
      "* Program: Maticové operace\n"
      "* Autor: David Krym (c) 2009\n"
      "* \n"
      "* Program poèítá souèet, souèin a výraz (A+A)*(B+B) s maticemi.\n"
      "* Dále zjis»uje, zda je matice souvislá. Provádí LL prùchod.\n"
      "* Matice se naèítají ze zadaných souborù.\n"
      "\n"
      " Pouziti:\n"
      "\tproj2 -h\n"
      "\tproj2 --add m1.txt m2.txt\n"
      "\tproj2 --mult m1.txt m2.txt\n"
      "\tproj2 --expr A.txt B.txt\n"
      "\tproj2 --cont m1.txt\n"
      "\tproj2 --LL m1.txt\n"
      "\n"
      " Popis parametru:\n"
      "\t-h\t\t\t Vypise tuto obrazovku s napovedou.\n"
      "\t--add m1.txt m2.txt\t Seète matice ze zadaných souborù.\n"
      "\t--mult m1.txt m2.txt\t Vynásobí matice ze zadaných souborù.\n"
      "\t--expr A.txt B.txt\t Vypoèítá výraz (A+A)*(B+B) s maticemi ze souborù.\n"
      "\t--cont m1.txt\t\t Zjistí, zda je matice souvislá.\n"
      "\t--LL m1.txt\t\t LL prùchod maticí.\n"
      "\n"
  );

} // konec funkce printHelp


/**
 * Naète matici ze souboru do pamìti.
 * @param m Ukazatel na strukturu, kam se ulo¾í výsledná matice.
 * @param filename Jméno souboru s maticí.
 * @return Vrací chybový kód.
 */
int loadMatrix(TMatrix *m, char *filename)
{
  if (filename == NULL)
  { // neznámá chyba
    return EUNKNOWN;
  }
    // pokud o otevøení souboru
  TFile f = tryOpen(filename,"r");
  
  if (f.ecode != EOK)
  { // nepodarilo se otevrit
    return f.ecode;
  }

  FILE * fr = f.pfile;  // pointer na naètený soubor
  int input;
  int code;
  int rows, cols;
  // poèet øádkù a sloupcù
  if ((code = fscanf(fr, "%d %d", &rows, &cols)) != 2)
  { // naète rozmìr matice
    tryClose(fr);
    return EBADFILE;
  }
  if (rows < 1 || cols < 1)
  { // pokud je rozmìr matice men¹í neý 1x1
    tryClose(fr);
    return EBADFILE;
  }
  
  int count = rows * cols; // poèet polo¾ek v matici
  // alokuji potøebnou pamì»
  TMatrix n = allocMatrix(rows, cols);
  
  if (n.matrix == NULL)
  { // chyba pøi alokaci
    tryClose(fr);
    return EALLOC;
  }
  
  int r = 0, c = 0; // radky a sloupce
  
  for (int i = 0; i < count; i++)
  { // naèítání hodnot do matice
    code = fscanf(fr,"%d",&input);
    if ((code == EOF && i != count - 1) || code == 0)
    {
      tryClose(fr);
      freeMatrix(&n);
      return EBADFILE;
    }
    
    n.matrix[r][c] = input;
    c++;  // inkrementace sloupcù
    if (c == n.cols)
    { // na dal¹í øádek
      c = 0;
      r++;
    }
  }
  
  if ((code = fscanf(fr,"%d",&input)) != 0 && code != EOF)
  { // pokud je v souboru víc hodnot ne¾ by mìlo
    tryClose(fr);
    freeMatrix(&n);
    return EBADFILE;
  }
  
  *m = n; // pøedám strukturu ven z funkce
  
  tryClose(fr);
  return EOK;
} // konec funkce loadMatrix


/**
 * Vytiskne matici na stdout.
 * @param m Ukazatel na strukturu s údajema o matici.
 * @return void.
 */
void printMatrix(const TMatrix *m)
{ // prvnì rozmìry
  printf("%d %d\n", m->rows, m->cols);
  
  for (int r = 0; r < m->rows; r++)
  { // potom samotná matice
    for (int s = 0; s < m->cols; s++)
    {
      printf("%d ", m->matrix[r][s]); 
    }
      printf("\n");
  }
} // konec funkce printMatrix


/**
 * Seète dvì matice, výsledek ulo¾í do res.
 * @param m1 První matice.
 * @param m2 Druhá matice.
 * @param res Výsledná matice.
 * @return Vrací chybový kód.
 */
int addMatrix(const TMatrix *m1, const TMatrix *m2, TMatrix *res)
{
  if (m1->rows != m2->rows || m1->cols != m2->cols)
  { // na výstup vypí¹e FALSE
    return ENORES;
  }
    //alokace výsledné matice
  TMatrix n = allocMatrix(m1->rows, m1->cols);
  if (n.matrix == NULL)
  { // chyba pøi alokaci
    return EALLOC;
  }
  
  for (int r = 0; r < m1->rows; r++)
  {
    for (int c = 0; c < m1->cols; c++)
    { // sèítání matic
      n.matrix[r][c] = m1->matrix[r][c] + m2->matrix[r][c];
    }
  }
  *res = n;
  
  return EOK;
} // konec funkce addMatrix


/**
 * Vynásobí dvì matice.
 * @param m1 Ukazatel na první matici.
 * @param m2 Ukazatel na druhou matici.
 * @param res Ukazatel na výsledek.
 * @return Vrací chybový kód.
 */
int mulMatrix(const TMatrix *m1, const TMatrix *m2, TMatrix *res)
{
  if (m1->cols != m2->rows)
  { // nelze násobit
    return ENORES;
  }
  // alokace výsledné matice
  TMatrix n = allocMatrix(m1->rows, m2->cols);
  if (n.matrix == NULL)
  { // chyba pøi alokaci
    return EALLOC;
  }
   // samotné násobení
  for (int i = 0; i < m1->rows; i++)
  {
    for (int j = 0; j < m2->cols; j++)
    {
      n.matrix[i][j] = 0;
      for (int k = 0; k < m1->cols; k++)
      { // násobení matic
        n.matrix[i][j] += m1->matrix[i][k] * m2->matrix[k][j];
      }
    }
  }
  *res = n;
  
  return EOK;
} // konec funkce mulMatrix


/**
 * Vypoèítá výraz (A+A)*(B+B).
 * @param m1 Ukazatel na první matici.
 * @param m2 Ukazatel na druhou matici.
 * @param res Ukazatel na výsledek.
 * @return Vrací chybový kód.
 */
int exprMatrix(TMatrix *m1, TMatrix *m2, TMatrix *res)
{
  TMatrix res1;
  
  int A = addMatrix(m1, m1, &res1);
  if (A != EOK)
  { // chyba, pøedej ji dál
    return A;
  }
  
  TMatrix res2;
  
  int B = addMatrix(m2, m2, &res2);
  if (B != EOK)
  { // chyba, pøedej ji dál
    freeMatrix(&res1);
    return B;
  }
  
  int C = mulMatrix(&res1, &res2, res);
  if (C != EOK)
  { // chyba, pøedej chybu dál
    freeMatrix(&res2);
    freeMatrix(&res1);
    return C;
  }
  
  freeMatrix(&res2);
  freeMatrix(&res1);
  return EOK;
} // konec funkce exprMatrix


/**
 * Zjistí, zda je matice souvislá.
 * @param m Ukazatel na matici.
 * @return Vrací chybový kód.
 */
int contMatrix(TMatrix *m)
{
  int number;
  TIncrement new =
  {
    .dr = 0,  // delta row
    .dc = 0,  // delta cols
  };
  
  for (int r = 0; r < m->rows - 1; r++)
  {
    for (int c = 0; c < m->cols - 1; c++)
    { 
      number = m->matrix[r][c]; // jádro osmiokolí
      new.dr = r;
      new.dc = c;
      
      for (int x = 0; x < 8; x++)
      { // osmiokolí kolem jádra
        new.dr += way[x].dr;  // nová souøadnice øádku
        new.dc += way[x].dc;  // nová souøadnice sloupce
        
        // zda se nacházíme uvitø pole
        if ( !((new.dr < 0 || new.dr > m->rows - 1) ||
               (new.dc < 0 || new.dc > m->cols - 1)))
        {// pokud nejsem mimo meze pole
          if (fabs(fabs(m->matrix[new.dr][new.dc]) - fabs(number)) > 1)
          {  // pokud to neni souvislá matice, nemá cenu dokonèovat cykly
            return false; // neni souvislá
          }
        }
        //pùvodná hodnoty jádra osmiokolí
        new.dr = r;
        new.dc = c;
      }
    }
  } 
  // je souvislá
  return true;
} // konec funkce  contMatrix


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


/**
 * Alokuje pamì» pro matici o zadaných rozmìrech.
 * @param rows Poèet øádkù matice.
 * @param cols Poèet sloupcù matice.
 * @return Vrací strukturu s informacema o matici.
 */
TMatrix allocMatrix(int rows, int cols)
{
  TMatrix m =
  {
    .rows = rows,
    .cols = cols,
    .matrix = NULL,  
  };
  // alokuje pamìt pro ukazatele
  m.matrix = malloc(rows * sizeof(int *));
  
  if (m.matrix == NULL)
  { //chyba
    return m;
  }
  
  for (int i = 0; i < rows; i++)
  { // alokuje pamìt pro sloupce matice
    m.matrix[i] = malloc(cols * sizeof(int));
  
    if (m.matrix[i] == NULL)
    { //pokud chyba, musim uvolnit ji¾ alokované
      for ( ; i >= 0; i--)
      {
        free(m.matrix[i]);
      }
      free(m.matrix);
      m.matrix = NULL;
      return m;
    }
  }

  return m;
} // konec funkce allocMatrix


/**
 * Uvolní alokovanou pamìt.
 * @param m Ukazatel na strukturu s informacemi o matici.
 * @return void.
 */
void freeMatrix(TMatrix *m)
{
  for (int i = 0; i < m->rows; i++)
  { // uvodlní sloupce
    free(m->matrix[i]);
  }
  // uvolní øádky
  free(m->matrix);
} // konec funkce freeMatrix


/**
 * Provede LL prùchod maticí.
 * @param m Ukazatel na zdrojovou matici.
 * @param res Ukazatel na výslednou matici.
 * @return Vrací chybový kód.
 */
int LLMatrix(TMatrix *m, TMatrix *res)
{
  TMatrix n = allocMatrix(m->rows, m->cols);
  
  if (n.matrix == NULL)
  {
    return EALLOC;
  }
  
  int r = 0, c = 0; // row, col
  int Cmax = m->cols - 1; // max index sloupce
  int Rmax = m->rows - 1; // max index øádku
  int dir = RIGHT;  // smìr
  int dirLast = RIGHT;  // poslední smìr
  int counter = 0;  // poèítadlo
  TIncrement new =
  {
    .dr=m->rows - 1,  // delta row
    .dc=0,  // delta cols
  };

  
  for (int i = 0; i < (m->rows * m->cols); i++)
  {
    n.matrix[r][c] = m->matrix[new.dr][new.dc]; // nová matice
    c++;  // inkrementace sloupcù
    if (c == n.cols)
    { // na dal¹í øádek
      c = 0;
      r++;
    }
    // jednotlivé smìry    
    if (dir == LEFT)
    { // smìr vlevo
      new.dr += way[LEFT].dr;  // nová souøadnice øádku
      new.dc += way[LEFT].dc;  // nová souøadnice sloupce
      if (dirLast == UP)
      { // pokud byl poslední smìr nahoru, zmìn¹ím rozmìry matice
        Cmax--;
        Rmax--;
        if (Rmax == 0)
        { // pokud nelze dolù, jdu doleva
          dir = LEFT;
          dirLast = LEFT;
        }
        else
        {
          dir = DOWN;
          dirLast = LEFT;
        }
      }
      else
      { // pokud byl poslední smìr cokoliv ne¾ nahoru
        counter++;
        if (counter == Cmax)
        { // pokud narazím na konec, zmìním smìr
          counter = 0;
          dir = UP;
          dirLast = LEFT;
        }
      }
    }
    else if (dir == RIGHT)
    { // smìr vpravo
      new.dr += way[RIGHT].dr;  // nová souøadnice øádku
      new.dc += way[RIGHT].dc;  // nová souøadnice sloupce
      counter++;
      if (counter == Cmax)
      {
        counter = 0;
        dir = UP;
        dirLast = RIGHT;
      }
    }
    else if (dir == UP)
    { // smìr nahoru
      new.dr += way[UP].dr;  // nová souøadnice øádku
      new.dc += way[UP].dc;  // nová souøadnice sloupce
      if (dirLast == LEFT)
      { // zase zmen¹ím matici
        dir = RIGHT;
        dirLast = UP;
        Cmax--;
        Rmax--;
      }
      else
      {
        counter++;
        if (counter == Rmax)
        {
          counter = 0;
          dir = LEFT;
          dirLast = UP;
        }
      }
    }
    else if (dir == DOWN)
    { // smìr dolu
      new.dr += way[DOWN].dr;  // nová souøadnice øádku
      new.dc += way[DOWN].dc;  // nová souøadnice sloupce
      counter++;
      if (counter == Rmax)
      {
        counter = 0;
        dir = LEFT;
        dirLast = DOWN;
      }
    }
  }
  
  *res = n;
  return EOK;
} // konec funkce LLMatrix


/* konec souboru proj3.c */

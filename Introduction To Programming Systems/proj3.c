/*
 * Soubor:  proj3.c
 * Datum:   2009/11/29
 * Autor:   David Krym
 * Projekt: Maticov� operace, projekt �. 3 pro p�edm�t IZP
 * Popis:   Po��t� sou�et, sou�in matic a v�raz (A+A)*(B+B).
 *          D�le zji�tuje souvislou matici a prov�d� LL pr�chod.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define true 1
#define false 0

/** K�dy chyb programu */
enum tecodes
{
  EOK = 0,     /**< Bez chyby. */
  ECLWRONG,    /**< Chybn� parametry. */
  EOPEN,       /**< Chyba p�i otev�r�n� souboru.*/
  ECLOSE,      /**< Chyba p�i zav�r�n� souboru. */
  EBADFILE,    /**< Soubor neodpov�d� zad�n�. */  
  EALLOC,      /**< Chyba p�i alokaci. */
  ENORES,      /**< Operace s maticema nem� v�sledek. */
  EUNKNOWN,    /**< Nezn�m� chyba. */
};


/** Chybov� hl�en� odpov�daj�c� chybov�m k�d�m. */
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


/** Stavov� k�dy programu */
enum tstates
{
  CHELP,       /**< N�pov�da */
  CADD,        /**< Sou�et matic */
  CMULT,       /**< N�soben� matic */
  CEXPR,       /**< V�raz (A+A) * (B+B) */
  CCONT,       /**< Souvisl� matice */
  CLL,         /**< LL pr�chod */
};


/**
 * Struktura obsahuj�c� hodnoty parametr� p��kazov� ��dky.
 */
typedef struct params
{
  int state;             /**< Stavov� k�d programu, odpov�d� v��tu tstates. */
  char *fname1;          /**< Jm�no souboru s prvn� matic�. */
  char *fname2;          /**< Jm�no souboru s druhou matic�. */
  int ecode;             /**< Chybov� k�d programu, odpov�d� v��tu tecodes. */
} TParams;


/**
 * Struktura obsahuj�c� hodnoty parametr� p��kazov� ��dky.
 */
typedef struct tmatrix
{
  int rows;             /**< Po�et ��dk� matice. */
  int cols;             /**< Po�et sloupc� matice. */
  int **matrix;          /**< Ukazatel na ukazatele, dynamick� v�cerozm�rn� pole. */
} TMatrix;


/**
 * Struktura obsahuj�c� hodnoty otev�en�ho souboru.
 */
typedef struct tfile
{
  FILE *pfile;           /**< Ukazatel na otev�en� soubor. */
  int ecode;             /**< Chybov� k�d programu, odpov�d� v��tu tecodes. */
} TFile;


/**
 * Struktura obsahuj�c� hodnoty delta pro pohyb v matici.
 */
typedef struct tincrement
{
  int dr, dc; // delta row, col
} TIncrement;


/**
 * Pole struktur obsahuj�c� sm�ry pohybu v matici.
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
 * K�dy sm�r�.
 */
enum tdirection
{
  LEFT = 0,
  RIGHT,
  UP,
  DOWN,
};


/////////////////////////////////////////////////////////////////
///////// Prototypy funkc�
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
  {
    int ecode = loadInput(params.state, params.fname1, params.fname2);
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
 * Dle zadan�ho parametru vol� funkce pro v�po�et.
 * Pokud je form�t argument� chybn�, ukon�� program s chybov�m k�dem.
 * @param state Zadan� parametr, co se bude po��tat.
 * @param fname1 N�zev prvn�ho souboru s matic�.
 * @param fname1 N�zev druh�ho souboru s matic�.
 * @return Vrac� chybov� k�d.
 */
int loadInput(int state, char *fname1, char *fname2)
{
   TMatrix m1;
   TMatrix m2;
   TMatrix m3;

  if (state == CCONT) // SOUVISLA MATICE
  { // pokus o na�ten� souboru do pole
    int ecode = loadMatrix(&m1, fname1);
    if (ecode != EOK)
    { // n�co nestandardn�ho
      return ecode;
    }
    int result = contMatrix(&m1); // vrac� true/false
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
  { // pokus o na�ten� souboru do pole
    int ecode = loadMatrix(&m1, fname1);
    if (ecode != EOK)
    { // n�co nestandardn�ho
      return ecode;
    }
    ecode = LLMatrix(&m1, &m2); // vrac� chybov� k�d
    if (ecode != EOK)
    { // chyba p�i s��t�n�
      freeMatrix(&m1);
      freeMatrix(&m2);
      return ecode;
    }
    printMatrix(&m2); // vytiskne v�sledek
    freeMatrix(&m1);
    freeMatrix(&m2);
  } // konec CDROT  
  else if (state == CADD) // SOUCET MATIC
  {
    int ecode = loadMatrix(&m1, fname1);
    if (ecode != EOK)
    { // n�co nestandardn�ho
      return ecode;
    }
    ecode = loadMatrix(&m2, fname2);
    if (ecode != EOK)
    { // mus�me uvolnit prvn� matici
      freeMatrix(&m1);
      return ecode;
    }
    ecode = addMatrix(&m1, &m2, &m3);
    if (ecode != EOK)
    { // chyba p�i s��t�n�
      freeMatrix(&m1);
      freeMatrix(&m2);
      return ecode;
    }
    printMatrix(&m3); // vytiskne v�sledek
    freeMatrix(&m1);
    freeMatrix(&m2);
    freeMatrix(&m3);
  } // konec CADD
  else if (state == CMULT)  // NASOBENI MATIC
  {
    int ecode = loadMatrix(&m1, fname1);
    if (ecode != EOK)
    { // n�co nestandardn�ho
      return ecode;
    }
    ecode = loadMatrix(&m2, fname2);
    if (ecode != EOK)
    { // mus�me uvolnit prvn� matici
      freeMatrix(&m1);
      return ecode;
    }
    ecode = mulMatrix(&m1, &m2, &m3);
    if (ecode != EOK)
    { // chyba p�i s��t�n�
      freeMatrix(&m1);
      freeMatrix(&m2);
      return ecode;
    }
    printMatrix(&m3); // vytiskne v�sledek
    freeMatrix(&m1);
    freeMatrix(&m2);
    freeMatrix(&m3);
  } // konec CMULT
  else if (state == CEXPR)  // (A+A)*(B+B)
  {
    int ecode = loadMatrix(&m1, fname1);
    if (ecode != EOK)
    { // n�co nestandardn�ho
      return ecode;
    }
    ecode = loadMatrix(&m2, fname2);
    if (ecode != EOK)
    { // mus�me uvolnit prvn� matici
      freeMatrix(&m1);
      return ecode;
    }
    ecode = exprMatrix(&m1, &m2, &m3);
    if (ecode != EOK)
    { // chyba p�i s��t�n�
      freeMatrix(&m1);
      freeMatrix(&m2);
      return ecode;
    }
    printMatrix(&m3); // vytiskne v�sledek
    freeMatrix(&m1);
    freeMatrix(&m2);
    freeMatrix(&m3);
  } // konec CEXPR

  return EOK;
} // konec funkce loadInput


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
  else if (argc == 3)
  { // t�i parametry
    if (strcmp("--cont", argv[1]) == 0)
    { // souvisl� matice
      result.state = CCONT;
    }
    else if (strcmp("--LL", argv[1]) == 0)
    { // ter�ov� rotace
      result.state = CLL;
    }
    else
    { // �patn� parametry
      result.ecode = ECLWRONG;
    }
    
    result.fname1 = argv[2];
  }
  else if (argc == 4)
  { // �ty�i params
    if (strcmp("--mult", argv[1]) == 0)
    { // n�soben� matic
      result.state = CMULT;
    }
    else if (strcmp("--add", argv[1]) == 0)
    { // s��t�n� matic
      result.state = CADD;
    }
    else if (strcmp("--expr", argv[1]) == 0)
    { // zdan� v�raz
      result.state = CEXPR;
    }
    else
    { // �patn� parametry
      result.ecode = ECLWRONG;
    }
    
    result.fname1 = argv[2];
    result.fname2 = argv[3];
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

  if (ecode == ENORES)
  { // matematick� operace neni definov�na
    printf("false\n");
  }
  else
  {
    fprintf(stderr, "%s", ECODEMSG[ecode]);
  }
} // konec funkce printECode


/**
 * Vytiskne n�pov�du.
 */
void printHelp(void)
{
  printf(
      "\n"
      "* Program: Maticov� operace\n"
      "* Autor: David Krym (c) 2009\n"
      "* \n"
      "* Program po��t� sou�et, sou�in a v�raz (A+A)*(B+B) s maticemi.\n"
      "* D�le zjis�uje, zda je matice souvisl�. Prov�d� LL pr�chod.\n"
      "* Matice se na��taj� ze zadan�ch soubor�.\n"
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
      "\t--add m1.txt m2.txt\t Se�te matice ze zadan�ch soubor�.\n"
      "\t--mult m1.txt m2.txt\t Vyn�sob� matice ze zadan�ch soubor�.\n"
      "\t--expr A.txt B.txt\t Vypo��t� v�raz (A+A)*(B+B) s maticemi ze soubor�.\n"
      "\t--cont m1.txt\t\t Zjist�, zda je matice souvisl�.\n"
      "\t--LL m1.txt\t\t LL pr�chod matic�.\n"
      "\n"
  );

} // konec funkce printHelp


/**
 * Na�te matici ze souboru do pam�ti.
 * @param m Ukazatel na strukturu, kam se ulo�� v�sledn� matice.
 * @param filename Jm�no souboru s matic�.
 * @return Vrac� chybov� k�d.
 */
int loadMatrix(TMatrix *m, char *filename)
{
  if (filename == NULL)
  { // nezn�m� chyba
    return EUNKNOWN;
  }
    // pokud o otev�en� souboru
  TFile f = tryOpen(filename,"r");
  
  if (f.ecode != EOK)
  { // nepodarilo se otevrit
    return f.ecode;
  }

  FILE * fr = f.pfile;  // pointer na na�ten� soubor
  int input;
  int code;
  int rows, cols;
  // po�et ��dk� a sloupc�
  if ((code = fscanf(fr, "%d %d", &rows, &cols)) != 2)
  { // na�te rozm�r matice
    tryClose(fr);
    return EBADFILE;
  }
  if (rows < 1 || cols < 1)
  { // pokud je rozm�r matice men�� ne� 1x1
    tryClose(fr);
    return EBADFILE;
  }
  
  int count = rows * cols; // po�et polo�ek v matici
  // alokuji pot�ebnou pam�
  TMatrix n = allocMatrix(rows, cols);
  
  if (n.matrix == NULL)
  { // chyba p�i alokaci
    tryClose(fr);
    return EALLOC;
  }
  
  int r = 0, c = 0; // radky a sloupce
  
  for (int i = 0; i < count; i++)
  { // na��t�n� hodnot do matice
    code = fscanf(fr,"%d",&input);
    if ((code == EOF && i != count - 1) || code == 0)
    {
      tryClose(fr);
      freeMatrix(&n);
      return EBADFILE;
    }
    
    n.matrix[r][c] = input;
    c++;  // inkrementace sloupc�
    if (c == n.cols)
    { // na dal�� ��dek
      c = 0;
      r++;
    }
  }
  
  if ((code = fscanf(fr,"%d",&input)) != 0 && code != EOF)
  { // pokud je v souboru v�c hodnot ne� by m�lo
    tryClose(fr);
    freeMatrix(&n);
    return EBADFILE;
  }
  
  *m = n; // p�ed�m strukturu ven z funkce
  
  tryClose(fr);
  return EOK;
} // konec funkce loadMatrix


/**
 * Vytiskne matici na stdout.
 * @param m Ukazatel na strukturu s �dajema o matici.
 * @return void.
 */
void printMatrix(const TMatrix *m)
{ // prvn� rozm�ry
  printf("%d %d\n", m->rows, m->cols);
  
  for (int r = 0; r < m->rows; r++)
  { // potom samotn� matice
    for (int s = 0; s < m->cols; s++)
    {
      printf("%d ", m->matrix[r][s]); 
    }
      printf("\n");
  }
} // konec funkce printMatrix


/**
 * Se�te dv� matice, v�sledek ulo�� do res.
 * @param m1 Prvn� matice.
 * @param m2 Druh� matice.
 * @param res V�sledn� matice.
 * @return Vrac� chybov� k�d.
 */
int addMatrix(const TMatrix *m1, const TMatrix *m2, TMatrix *res)
{
  if (m1->rows != m2->rows || m1->cols != m2->cols)
  { // na v�stup vyp�e FALSE
    return ENORES;
  }
    //alokace v�sledn� matice
  TMatrix n = allocMatrix(m1->rows, m1->cols);
  if (n.matrix == NULL)
  { // chyba p�i alokaci
    return EALLOC;
  }
  
  for (int r = 0; r < m1->rows; r++)
  {
    for (int c = 0; c < m1->cols; c++)
    { // s��t�n� matic
      n.matrix[r][c] = m1->matrix[r][c] + m2->matrix[r][c];
    }
  }
  *res = n;
  
  return EOK;
} // konec funkce addMatrix


/**
 * Vyn�sob� dv� matice.
 * @param m1 Ukazatel na prvn� matici.
 * @param m2 Ukazatel na druhou matici.
 * @param res Ukazatel na v�sledek.
 * @return Vrac� chybov� k�d.
 */
int mulMatrix(const TMatrix *m1, const TMatrix *m2, TMatrix *res)
{
  if (m1->cols != m2->rows)
  { // nelze n�sobit
    return ENORES;
  }
  // alokace v�sledn� matice
  TMatrix n = allocMatrix(m1->rows, m2->cols);
  if (n.matrix == NULL)
  { // chyba p�i alokaci
    return EALLOC;
  }
   // samotn� n�soben�
  for (int i = 0; i < m1->rows; i++)
  {
    for (int j = 0; j < m2->cols; j++)
    {
      n.matrix[i][j] = 0;
      for (int k = 0; k < m1->cols; k++)
      { // n�soben� matic
        n.matrix[i][j] += m1->matrix[i][k] * m2->matrix[k][j];
      }
    }
  }
  *res = n;
  
  return EOK;
} // konec funkce mulMatrix


/**
 * Vypo��t� v�raz (A+A)*(B+B).
 * @param m1 Ukazatel na prvn� matici.
 * @param m2 Ukazatel na druhou matici.
 * @param res Ukazatel na v�sledek.
 * @return Vrac� chybov� k�d.
 */
int exprMatrix(TMatrix *m1, TMatrix *m2, TMatrix *res)
{
  TMatrix res1;
  
  int A = addMatrix(m1, m1, &res1);
  if (A != EOK)
  { // chyba, p�edej ji d�l
    return A;
  }
  
  TMatrix res2;
  
  int B = addMatrix(m2, m2, &res2);
  if (B != EOK)
  { // chyba, p�edej ji d�l
    freeMatrix(&res1);
    return B;
  }
  
  int C = mulMatrix(&res1, &res2, res);
  if (C != EOK)
  { // chyba, p�edej chybu d�l
    freeMatrix(&res2);
    freeMatrix(&res1);
    return C;
  }
  
  freeMatrix(&res2);
  freeMatrix(&res1);
  return EOK;
} // konec funkce exprMatrix


/**
 * Zjist�, zda je matice souvisl�.
 * @param m Ukazatel na matici.
 * @return Vrac� chybov� k�d.
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
      number = m->matrix[r][c]; // j�dro osmiokol�
      new.dr = r;
      new.dc = c;
      
      for (int x = 0; x < 8; x++)
      { // osmiokol� kolem j�dra
        new.dr += way[x].dr;  // nov� sou�adnice ��dku
        new.dc += way[x].dc;  // nov� sou�adnice sloupce
        
        // zda se nach�z�me uvit� pole
        if ( !((new.dr < 0 || new.dr > m->rows - 1) ||
               (new.dc < 0 || new.dc > m->cols - 1)))
        {// pokud nejsem mimo meze pole
          if (fabs(fabs(m->matrix[new.dr][new.dc]) - fabs(number)) > 1)
          {  // pokud to neni souvisl� matice, nem� cenu dokon�ovat cykly
            return false; // neni souvisl�
          }
        }
        //p�vodn� hodnoty j�dra osmiokol�
        new.dr = r;
        new.dc = c;
      }
    }
  } 
  // je souvisl�
  return true;
} // konec funkce  contMatrix


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


/**
 * Alokuje pam� pro matici o zadan�ch rozm�rech.
 * @param rows Po�et ��dk� matice.
 * @param cols Po�et sloupc� matice.
 * @return Vrac� strukturu s informacema o matici.
 */
TMatrix allocMatrix(int rows, int cols)
{
  TMatrix m =
  {
    .rows = rows,
    .cols = cols,
    .matrix = NULL,  
  };
  // alokuje pam�t pro ukazatele
  m.matrix = malloc(rows * sizeof(int *));
  
  if (m.matrix == NULL)
  { //chyba
    return m;
  }
  
  for (int i = 0; i < rows; i++)
  { // alokuje pam�t pro sloupce matice
    m.matrix[i] = malloc(cols * sizeof(int));
  
    if (m.matrix[i] == NULL)
    { //pokud chyba, musim uvolnit ji� alokovan�
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
 * Uvoln� alokovanou pam�t.
 * @param m Ukazatel na strukturu s informacemi o matici.
 * @return void.
 */
void freeMatrix(TMatrix *m)
{
  for (int i = 0; i < m->rows; i++)
  { // uvodln� sloupce
    free(m->matrix[i]);
  }
  // uvoln� ��dky
  free(m->matrix);
} // konec funkce freeMatrix


/**
 * Provede LL pr�chod matic�.
 * @param m Ukazatel na zdrojovou matici.
 * @param res Ukazatel na v�slednou matici.
 * @return Vrac� chybov� k�d.
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
  int Rmax = m->rows - 1; // max index ��dku
  int dir = RIGHT;  // sm�r
  int dirLast = RIGHT;  // posledn� sm�r
  int counter = 0;  // po��tadlo
  TIncrement new =
  {
    .dr=m->rows - 1,  // delta row
    .dc=0,  // delta cols
  };

  
  for (int i = 0; i < (m->rows * m->cols); i++)
  {
    n.matrix[r][c] = m->matrix[new.dr][new.dc]; // nov� matice
    c++;  // inkrementace sloupc�
    if (c == n.cols)
    { // na dal�� ��dek
      c = 0;
      r++;
    }
    // jednotliv� sm�ry    
    if (dir == LEFT)
    { // sm�r vlevo
      new.dr += way[LEFT].dr;  // nov� sou�adnice ��dku
      new.dc += way[LEFT].dc;  // nov� sou�adnice sloupce
      if (dirLast == UP)
      { // pokud byl posledn� sm�r nahoru, zm�n��m rozm�ry matice
        Cmax--;
        Rmax--;
        if (Rmax == 0)
        { // pokud nelze dol�, jdu doleva
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
      { // pokud byl posledn� sm�r cokoliv ne� nahoru
        counter++;
        if (counter == Cmax)
        { // pokud naraz�m na konec, zm�n�m sm�r
          counter = 0;
          dir = UP;
          dirLast = LEFT;
        }
      }
    }
    else if (dir == RIGHT)
    { // sm�r vpravo
      new.dr += way[RIGHT].dr;  // nov� sou�adnice ��dku
      new.dc += way[RIGHT].dc;  // nov� sou�adnice sloupce
      counter++;
      if (counter == Cmax)
      {
        counter = 0;
        dir = UP;
        dirLast = RIGHT;
      }
    }
    else if (dir == UP)
    { // sm�r nahoru
      new.dr += way[UP].dr;  // nov� sou�adnice ��dku
      new.dc += way[UP].dc;  // nov� sou�adnice sloupce
      if (dirLast == LEFT)
      { // zase zmen��m matici
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
    { // sm�r dolu
      new.dr += way[DOWN].dr;  // nov� sou�adnice ��dku
      new.dc += way[DOWN].dc;  // nov� sou�adnice sloupce
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

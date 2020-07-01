/*
 * Soubor:  philosophers.c
 * Datum:   2010/4/20
 * Autor:   David Krym
 * Projekt: 2. projekt do IOS
 **
 * Zadání:  Po spu¹tìní vytváøí hlavní proces 5 podprocesù (ka¾dý reprezentuje akce jednoho filozofa).
 *          Hlavní proces èeká na ukonèení v¹ech svých potomkù a poté se ukonèí. Ka¾dý proces bude
 *          identifikovatelný podle èísla (1 a¾ 5), podobnì vidlièky (1 a¾ 5). Ka¾dý filozof pøemý¹lí,
 *          poté postupnì uchopí vidlièky, jí, postupnì vidlièky odlo¾í a opìt pøemý¹lí. Celý cyklus se
 *          opakuje. Ka¾dý filozof poskytuje informace o právì provádìné akci. Tyto informace zapisuje
 *          do souboru, jeho¾ název je philosophers.out. Øe¹eno frontou zpráv a sdilenou pamì»í.
 **
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>


// pocet filosofu
#define PHIL_NUM 5
// typ zpravy pro pracovani se souborem, vetsi nez 0
#define FMTYPE 2
// jmeno souboru
#define FNAME "philosophers.out"
// cas simulujici akci v milisekundachh, 0-MAXSLEEP
#define MAXSLEEP 100
// project id, pro fci ftok
#define PROJID1 12
#define PROJID2 21



// Prototypy fonkci
int printStr(int type, int msgqid, int *counter, FILE *f, int phil_num, int fork_num);
int childMain(int mynum, int msgqid, int *counter, int eatnum, FILE *f);
int child(int mynum, int msgqid, int shmid, int eatnum, FILE *f);
int getParam(int argc, char *argv[], int *num);
void kill_children(int count, pid_t *pidch);
int init_msgqueues(int *msgqid);
int init_shmemory(int *shmid);
int clean(int what, int id);
void printECode(int ecode);
void rand_sleep(int num);
int mainSec(int eatnum);


/** Kódy chyb programu */
enum tecodes
{
  EOK = 0,     /**< Bez chyby */
  EMSGQKEY,    /**< Chyba klice pro frontu zprav */
  EMSGQGET,    /**< Chyba pri vytvoreni fronty */
  EMSGSND,     /**< Chyba pri posilani zpravy */
  EMSGRCV,     /**< Chyba pri prijmu zpravy */
  EMSGQRM,     /**< Chyba pri ruseni fronty zprav */
  ESHMKEY,     /**< Chyba klice pro sdilenou pamet */
  ESHMGET,     /**< Chyba pri vytvoreni sdilene pameti */
  ESHMAT,      /**< Chyba pri pripojeni sdilene pameti */
  ESHMDT,      /**< Chyba pri odpojeni sdilene pameti */
  ESHMRM,      /**< Chyba pri ruseni sdilene pameti */
  EFILEO,      /**< Chyba pri vytvoreni/otevreni souboru */
  EFILEW,      /**< Chyba pri zapisu do souboru */
  EFILEC,      /**< Chyba pri zavreni souboru */
  EFORK,       /**< Chyba forku */
  EPARAM,      /**< Chyba parametru */
  ECHLD,       /**< Potomek zabit signalem */
  EWAIT,       /**< Chyba pri cekani na potomka */
  EUNKNOWN,    /**< Neznama chyba */
};


/** Chybová hlá¹ení odpovídající chybovým kódùm. */
const char *ECODEMSG[] =
{
  /*EOK*/
  "",
  /*EMSGQKEY*/
  "Chyba pri generovani klice pro frontu zprav\n",
  /*EMSGQGET*/
  "Chyba pri vytvareni fronty zprav\n",
  /*EMSGSND*/
  "Chyba pri posilani zpravy\n",
  /*EMSGRCV*/
  "Chyba pri prijimani zpravy\n",
  /*EMSGQRM*/
  "Chyba pri mazani fronty zprav\n",
  /*ESHMKEY*/
  "Chyba pri generovani klice pro sdilenou pamet\n",
  /*ESHMGET*/
  "Chyba pri vytvareni sdilene pameti\n",
  /*ESHMAT*/
  "Chyba pri pripojeni sdilene pameti\n",
  /*ESHMDT*/
  "Chyba pri odpojeni sdile pameti\n",
  /*ESHMRM*/
  "Chyba pri mazani sdilene pameti\n",
  /*EFILEO*/
  "Chyba pri otevreni/vytvoreni souboru\n",
  /*EFILEW*/
  "Chyba pri zapisu do souboru\n",
  /*EFILEC*/
  "Chyba pri zavirani souboru\n",
  /*EFORK*/
  "Chyba pri vytvareni filozofu\n",
  /*EPARAM*/
  "Chybny parametr, zadejte cele kladne cislo\n",
  /*ECHLD*/
  "Nektery potomek byl ukoncen signalem\n",
  /*EWAIT*/
  "Chyba pri cekani na potomka\n",
  /*EUNKNOWN*/
  "Neznama chyba\n",
};


/** Kódy hlá¹ek */
enum tstr
{
  PTHINK = 0,     /**< Filozof premysli */
  PEAT,           /**< Filozof ji */
  PPICKUP,        /**< Filozof zveda vidlicku */
  PRELEASE,       /**< Filozof poklada vidlicku */
};


/** Hlá¹ky odpovídající kódùm. */
const char *OUTMSG[] =
{
  "%d: philosopher %d: becomes thinking\n",
  "%d: philosopher %d: becomes eating\n",
  "%d: philosopher %d: picks up a fork %d\n",
  "%d: philosopher %d: releases a fork %d\n",
};


/** Kódy akce cisteni */
enum tclean
{
  CMSGQ = 0,     /**< Zruseni fronty zprav */
  CSHM,          /**< Zruseni sdilene pameti */
};


/**
 * Struktura s typem zpravy.
 */
typedef struct tmsgbuf
{
  long mtype;         /* message type, must be > 0 */
//  char mtext[1];    /* message data */
} Tmsgbuf;


/**
 * Hlavní program.
 */
int main(int argc, char *argv[])
{
  int ecode; // chybovy kod
  int eatnum; // zadany pocet jidel
  
  // spracovani parametru, ziskani poctu jidel
  if ((ecode = getParam(argc, argv, &eatnum)) != EOK)
  { // chybný parametr
    printECode(ecode); // vypis chyba
    return EXIT_FAILURE;
  }

  if (mainSec(eatnum) != EOK)
  {
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}



/**
 * Funkce potomka. Pripoji sdilenou pamet, zavola funkci na
 * provedeni akci a pote odpoji sdilenou pamet.
 * @param mynum Poradove cislo filozofa.
 * @param msgqid Identifikator vytvorene fronty zprav.
 * @param shmid Identifikator vytvorene sdilene pameti.
 * @param eatnum Pocet jidel kazdeho filozofa.
 * @param f Ukazatel na otevreny soubor pro zapis.
 * @return Vrací chybový kód.
 */
int child(int mynum, int msgqid, int shmid, int eatnum, FILE *f)
{
  setbuf(f,NULL); // nepouzivat buffer
  
  int ecode; // chybovy kod
  int *counter; // ukazatel na pocitadlo
  
  // pripojeni sdilene pameti
  if ((counter = shmat(shmid, NULL, 0)) == (void *) -1)
  { // chyba, rovnou konec
    printECode(ESHMAT);
    perror("shmat");
    exit(EXIT_FAILURE);
  }
  
  // hlavni funkce
  ecode = childMain(mynum, msgqid, counter, eatnum, f);

  // odpojeni sdilene pameti
  if ((shmdt(counter)) == -1)
  { // chyba
    printECode(ESHMDT);
    perror("shmdt");
    ecode = ESHMDT;
  }

  // zavreni souboru
  if ((fclose(f)) != 0)
  { // chyba
    ecode = EFILEC;
    printECode(ecode);
    perror("fclose");
  }
  
  if (ecode != EOK)
  { // jakakoliv chyba
    exit(EXIT_FAILURE);
  }
  
  // vse vporadku
  exit(EXIT_SUCCESS);
}


/**
 * Hlavni provadeci funkce pro potomka. Vypisuje akce do souboru.
 * Provadi mysleni->zvednuti vidlicek->jezeni->polozeni vidlicek.
 * @param mynum Poradove cislo filozofa.
 * @param msgqid Identifikator vytvorene fronty zprav.
 * @param counter Ukazatel na promennou ve sdilene pameti.
 * @param eatnum Pocet jidel kazdeho filozofa.
 * @param f Ukazatel na otevreny soubor pro zapis.
 * @return Vrací chybový kód.
 */
int childMain(int mynum, int msgqid, int *counter, int eatnum, FILE *f)
{
  Tmsgbuf forkbuf; // struktura pro zpravu (vidlicka)
  int mypid = getpid(); // pid daneho potomka
  int i; // pocitadlo iteraci
  int first_fork_num, second_fork_num; // cislo prvni a druhe vidlicky
  int ecode; // chybovy kod

  // prevence deadlocku
  if ((mynum % 2) == 1) 
  { // lichy filosof 1,3,5 bere prvni z prava
    first_fork_num = (mynum % PHIL_NUM) + 1; // prvni vidlicka
    second_fork_num = mynum; // druha vidlicka
  }
  else
  { // sudy filosof 2,4 bere prvni z leva
    first_fork_num = mynum; 
    second_fork_num = (mynum % PHIL_NUM) + 1;
  }

  // cyklus pro dany pocet jidel
  for (i = 0; i < eatnum; i++)
  {
  //// THINKING
    if ((ecode = printStr(PTHINK, msgqid, counter, f, mynum, 0)) != EOK)
    { // chyba pri pouziti souboru/sdilene pameti
      return ecode;
    }
    // premysleni
    rand_sleep(mypid);


  //// PICKING 1st FORK
    // vyzvedne zpravu ze bere vidlicku
    if (msgrcv(msgqid, &forkbuf, 0, first_fork_num, 0) == -1)
    { // chyba pri prijmu zpravy
      printECode(EMSGRCV);
      perror("msgrcv");
      return EMSGRCV;
    }
    // zapise do souboru ze sebral vidlicku
    if ((ecode = printStr(PPICKUP, msgqid, counter, f, mynum, first_fork_num)) != EOK)
    {
      return ecode;
    }


  //// PICKING 2nd FORK
    // vyzvedne zpravu ze bere vidlicku
    if (msgrcv(msgqid, &forkbuf, 0, second_fork_num, 0) == -1)
    { // chyba pri prijmu zpravy
      printECode(EMSGRCV);
      perror("msgrcv");
      return EMSGRCV;
    }
    // zapise do souboru ze sebral vidlicku
    if ((ecode = printStr(PPICKUP, msgqid, counter, f, mynum, second_fork_num)) != EOK)
    {
      return ecode;
    }


  //// EATING
    if ((ecode = printStr(PEAT, msgqid, counter, f, mynum, 0)) != EOK)
    {
      return ecode;
    }
    // jezeni
    rand_sleep(mypid);


  //// RELEASING 1st FORK
    // prvne vypisu, pak az polozim (aby ji nekdo nevzal pred vypisem)
    if ((ecode = printStr(PRELEASE, msgqid, counter, f, mynum, first_fork_num)) != EOK)
    {
      return ecode;
    }
    forkbuf.mtype = first_fork_num;  // prvni vidlicka
    // polozi prvni vidlicku
    if (msgsnd(msgqid, &forkbuf, 0, 0) == -1)
    { // nepodarilo se odeslat zpravu
      printECode(EMSGSND);
      perror("msgsnd");
      return EMSGSND;
    }


  //// RELEASING 2nd FORK
    if ((ecode = printStr(PRELEASE, msgqid, counter, f, mynum, second_fork_num)) != EOK)
    {
      return ecode;
    }
    forkbuf.mtype = second_fork_num;  // druha vidlicka
    // polozi druhou vidlicku
    if (msgsnd(msgqid, &forkbuf, 0, 0) == -1)
    { // nepodarilo se odeslat zpravu
      printECode(EMSGSND);
      perror("msgsnd");
      return EMSGSND;
    }
  }
  
  // vse vporadku
  return EOK;
} // konec funkce childMain


/**
 * Na zaklade zadaneho cisla generuje nahodne cisla 0-100(MAXSLEEP).
 * Ty jsou pouzity jako doba v ms, na kterou je proces uspan.
 * @param num Jakekoliv cislo, bude pouzito pro nove generovani.
 */
void rand_sleep(int num)
{
  int seed = (rand() + num);
  srand(seed); // nove generovani
  int myrand = (rand() % (MAXSLEEP + 1)); // nahodne cislo cislo 0-100(MAXSLEEP)
  usleep(myrand * 1000); // sleep 0-100(MAXSLEEP) milisekund
} // konec funkce rand_sleep


/**
 * Funkce vytvori frontu zprav a pote do ni posle 5 zprav reprezentujici
 * lezici vidlici a jednu zpravu reprezentujici moznost zapisovat do souboru.
 * @param msgqid Ukazatel na promennou, do ktere se ulozi ID fronty zprav.
 * @return Vrací chybový kód.
 */
int init_msgqueues(int *msgqid)
{
//// Generovani klice pro frontu zprav
  key_t msgkey; // klic pro vidlicky a soubor
  
  // vygeneruje klic
  if ((msgkey = ftok(getenv("HOME"), PROJID1)) == -1)
  {
    printECode(EMSGQKEY);
    perror("ftok");
    return EMSGQKEY;
  }

//// Vytvoreni fronty zprav
  if ((*msgqid = msgget(msgkey, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
  {
    printECode(EMSGQGET);
    perror("msgget");
    return EMSGQGET;
  }

//// Vytvoreni vsech vidlicek a povoleni zapisu do souboru
  Tmsgbuf mymsg; // odesilana zprava
  int i; //pocitadlo
  
  for (i = 1; i <= PHIL_NUM; i++)
  { // odesle zpravy znacici vidlicky
    mymsg.mtype = i;  // cislo vidlicky
    // posilam zpravu delky 0, pouzivam jen typ zprav 1-5
    if (msgsnd(*msgqid, &mymsg, 0, 0) == -1)
    { // nepodarilo se odeslat zpravu
      printECode(EMSGSND);
      perror("msgsnd");
      clean(CMSGQ,*msgqid); // zruseni fronty zprav
      return EMSGSND;
    }
  }
  
  // odeslani zpravy ze lze zapsat do souboru
  mymsg.mtype = (PHIL_NUM + FMTYPE); // typ zpravy pro soubor
  if (msgsnd(*msgqid, &mymsg, 0, 0) == -1)
  { // nepodarilo se odeslat zpravu
    printECode(EMSGSND);
    perror("msgsnd");
    clean(CMSGQ,*msgqid); // zruseni fronty zprav
    return EMSGSND;
  }
  
  // vse vporadku
  return EOK;
} // konec funkce init_msgqueues


/**
 * Funkce vytvori sdilenou pamet a vynuluje promennou ulozenou v ni.
 * @param shmid Ukazatel na promennou, do ktere se ulozi ID sdilene pameti.
 * @return Vrací chybový kód.
 */
int init_shmemory(int *shmid)
{
//// Generovani klice pro sdilenou pamet
  key_t shmkey; // klic pro sdilenou pamet
  
  // vygeneruje klic
  if ((shmkey = ftok(getenv("HOME"), PROJID2)) == -1)
  {
    printECode(ESHMKEY);
    perror("ftok");
    return ESHMKEY;
  }
  
//// vytvoreni sdilene pameti
  if ((*shmid = shmget(shmkey, sizeof(int), 0666 | IPC_CREAT | IPC_EXCL)) == -1)
  {
    printECode(ESHMGET);
    perror("shmget");
    return ESHMGET;
  }

//// Pripojeni sdilene pameti, vynulovani, odpojeni
  int *counter; // ukazatel na pocitadlo akci
  
  // pripojeni
  if ((counter = shmat(*shmid, NULL, 0)) == (void *) -1)
  {
    printECode(ESHMAT);
    perror("shmat");
    clean(CSHM,*shmid); // zruseni sdilene pameti
    return ESHMAT;
  }
  
  (*counter) = 0; // vynulování poèítadla
  
  //odpojeni
  if ((shmdt(counter)) == -1)
  {
    printECode(ESHMDT);
    perror("shmdt");
    clean(CSHM,*shmid); // zruseni sdilene pameti
    return ESHMDT;
  }

  // vse vporadku
  return EOK;
} // konec funkce init_shmemory


/**
 * Hlavni provadeci funkce pro rodice. Vola funkce na vytvoreni fronty
 * zprav a sdilene pameti. Otevira soubor pro zapis. Vytvari potomky.
 * @param eatnum Pocet jidel pro kazdeho filozofa.
 * @return Vrací chybový kód.
 */
int mainSec(int eatnum)
{
  int ecode; // chybovy kod

//// Fronta zprav
  int msgqid; // id fronty zprav
  // vytvori frontu zprav a odesle pocatecni zpravy
  if ((ecode = init_msgqueues(&msgqid)) != EOK)
  {
    return ecode;
  }

//// Sdilena pamet
  int shmid; // id sdilene pameti
  // vytvori sdilenou pamet a vynuluje promennou
  if ((ecode = init_shmemory(&shmid)) != EOK)
  {
    clean(CMSGQ, msgqid); // zruseni fronty zprav
    return ecode;
  }

//// Otevreni souboru
  FILE *f;
  if ((f = fopen(FNAME, "w")) == NULL)
  {
    printECode(EFILEO);
    perror("fopen");
    clean(CMSGQ, msgqid); // zruseni fronty zprav
    clean(CSHM, shmid); // zruseni sdilene pameti
    return EFILEO;
  }

//// Vytvareni potomku
  pid_t pid; // pid potomka
  pid_t pid_ch[PHIL_NUM]; // pole pid potomku
  int i; // pocitadlo iteraci
  
  for (i = 1; i <= PHIL_NUM; i++)
  {
      pid = fork(); // porod
      if (pid == 0)
      { // potomek
        child(i, msgqid, shmid, eatnum, f);
      }
      else if (pid < 0)
      { // chyba
        printECode(EFORK);
        perror("fork");
        kill_children(i, pid_ch); // zabije dosud vytvorene potomky
        clean(CMSGQ, msgqid); // zruseni fronty zprav
        clean(CSHM, shmid); // zruseni sdilene pameti
        return EFORK;
      }
      else
      { // rodic
        pid_ch[i-1] = pid; // ulozeni pid potomka
      }
  }
  
//// Terminace potomku
  int status; // navratovy stav potomka
  pid_t pidch; // pid potomka
  int term_cnt = 0; // pocet ukoncenych potomku

  while (term_cnt != PHIL_NUM) // pro vsechny potomky
  {
    // ziskani navratoveho kodu potomka
    if ((pidch = wait(&status)) == -1)
    {
      ecode = EWAIT;
      printECode(ecode);
      perror("wait");
      break;
    }
    
    // potomek skoncil normalne
    if (WIFEXITED(status))
    { 
      for (i = 0; i < PHIL_NUM; i++)
      {
        if (pid_ch[i] == pidch)
        { // jeho pid nahradime nulou
          pid_ch[i] = 0;
          ++term_cnt; // pocet ukoncenych potomku +1
          continue; // dal¹í iterace
        }
      }
    }
    
    // potomek ukoncen signalem
    if (WIFSIGNALED(status))
    { // ukonci ostatni potomky (u kterych neni pid=0)
      kill_children(PHIL_NUM, pid_ch);
    }
  }

  // zruseni fronty zprav
  if (clean(CMSGQ, msgqid) !=  EOK)
  {
    ecode = EMSGQRM;
    printECode(ecode);
  }
  
  // zruseni sdilene pameti
  if (clean(CSHM, shmid) != EOK)
  {
    ecode = ESHMRM;
    printECode(ecode);
  }
  
  // zavreni souboru
  if ((fclose(f)) != 0)
  { // chyba
    printECode(EFILEC);
    perror("fclose");
    ecode = EFILEC;
  }

  if (ecode != EOK)
  { // jakakoliv chyba
    return ecode;
  }
  
  // vse vporadku
  return EOK;
} // konec funkce mainSec


/**
 * Funkce zajistuje inkrementovani promenne ve sdilene pameti
 * a zapisuje do souboru. Funkce funguje jako kriticka sekce.
 * @param type Ktera hlaska se bude vypisovat.
 * @param msgqid Identifikator vytvorene fronty zprav.
 * @param counter Ukazatel na promennou ve sdilene pameti.
 * @param f Ukazatel na otevreny soubor pro zapis.
 * @param phil_num Poradove cislo filozofa.
 * @param fork_num Cislo vidlicky, pokud je potreba.
 * @return Vrací chybový kód.
 */
int printStr(int type, int msgqid, int *counter, FILE *f, int phil_num, int fork_num)
{ // kriticka sekce
  Tmsgbuf filebuf; // struktura pro zpravu
  int msgftype = (PHIL_NUM + FMTYPE); // typ zpravy pro soubor, aby nedoslo ke kolizi
  
//// "zamknu KS", chci vstoupit do KS
  if (msgrcv(msgqid, &filebuf, 0, msgftype, 0) == -1)
  { // neporadilo se prijmou zpravu
    printECode(EMSGRCV);
    perror("msgrcv");
    return EMSGRCV;
  }
  
  (*counter)++; // pocitadlo akci
  
  if ((type == PTHINK) || (type == PEAT))
  { // vypis 2 promennych
    if (fprintf(f, OUTMSG[type], *counter, phil_num) < 0)
    { // chyba pri zapisu do souboru
      printECode(EFILEW);
      perror("fprintf");
      return EFILEW;
    }
  }
  else if ((type == PPICKUP) || (type == PRELEASE))
  { // vypis 3 promennych
    if (fprintf(f, OUTMSG[type], *counter, phil_num, fork_num) < 0)
    { // chyba pri zapisu do souboru
      printECode(EFILEW);
      perror("fprintf");
      return EFILEW;
    }
  }
  else
  { // neznamy typ hlasky
    printECode(EUNKNOWN);
    return EUNKNOWN;
  }
  
//// "odemknu KS", ostatní mù¾ou vstoupit
  if (msgsnd(msgqid, &filebuf, 0, 0) == -1)
  { // nepodarilo se odeslat zpravu
    printECode(EMSGSND);
    perror("msgsnd");
    return EMSGSND;
  }

  // vse vporadku
  return EOK;
} // konec funkce printSrt


/**
 * Zrusi sdilenou pamet nebo frontu zprav dle zadaneho id.
 * @param what Co se bude rusit, zda fronta zprav nebo sdile pamet.
 * @param id Identifikator fronty/pameti.
 * @return Vrací chybový kód.
 */
int clean(int what, int id)
{
  int ecode = EOK; // vychozi chybovy kod
  
  if (what == CMSGQ)
  { // zruseni fronty zprav
    if (msgctl(id, IPC_RMID, NULL) == -1)
    {
      ecode = EMSGQRM;
      printECode(ecode);
      perror("msgctl");
    }
  }
  else if (what == CSHM)
  { // zruseni sdilene pameti
    if (shmctl(id, IPC_RMID, NULL) == -1)
    {
      ecode = ESHMRM;
      printECode(ecode);
      perror("shmctl");
    }
  }

  return ecode;
} // konec funkce clean


/**
 * Zpracuje argumenty prikazového radku.
 * Pokud je format argumentu chybny, vrati chybovy kod.
 * @param argc Poèet argumentù.
 * @param argv Pole textových øetìzcù s argumenty.
 * @param num Ukazatel na promennou pro ulozeni poctu jidel.
 * @return Vrací chybový kód.
 */
int getParam(int argc, char *argv[], int *num)
{
  if (argc == 2 && isdigit(argv[1][0]))
  { // jako parametr je zadáno èíslo
    (*num) = atoi(argv[1]);
    return EOK;
  }

  // chyba
  return EPARAM;
} // konec funkce getParam


/**
 * Zabije zadany pocet potomku od zacatku predaneho pole.
 * Pokud je pid 0, potomek uz byl ukoncen jinak.
 * @param count Pocet vytvorenych potomku
 * @param pidch Pole s pid potomku
*/
void kill_children(int count, pid_t *pidch)
{
  int i;
  
  for (i = 0; i < count; i++)
  {
    if (pidch[i] != 0)
    { // pokud pid neni 0 (pro zaverecnou terminaci jen nekterych potomku)
      kill(pidch[i],SIGTERM);
    }
  }
} // konec funkce kill_children


/**
 * Vytiskne hlá¹ení odpovídající chybovému kódu.
 * @param code Kód chyby programu
 */
void printECode(int ecode)
{
  if (ecode < EOK || ecode > EUNKNOWN)
  { // neznama chyba
    ecode = EUNKNOWN;
  }

  fprintf(stderr, "%s", ECODEMSG[ecode]);
} // konec funkce printECode

/* konec souboru */

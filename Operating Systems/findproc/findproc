#!/bin/ksh

###################################################################
# Soubor:  findproc                                               #
# Datum:   20.3.2010                                              #
# Autor:   David Krym                                             #
# Projekt: Výpis procesů                                          #
# Popis:   Skript findproc identifikuje procesy podle jména,      #
#          příkazové řádky a/nebo jejich aktuálního adresáře.     #
#          Skript tiskne na každý samostatný řádek PID každého    #
#          procesu splňujícího všechna zadaná kritéria.           #
###################################################################

printHelp()
{ # vypis napovedy
  printf "\
findproc prints PID of every process that conforms to specific criteria
usage: findproc criterion1 [criterion2 [...]]
criteria:
\t -n regexp \t the process name matches regexp
\t -a regexp \t an argument of a process matches regexp
\t -c regexp \t the current working directory matches regexp
\t -x file \t the process executable file is the same as file\n"
}


#########
## Test os, nastaveni promennych dle os 
#########
os=`uname`

if [ "$os" = "Linux" ]
then  # linux
  # param C, prikaz pro vypis cwd
  C_cmd="lsof -w -d cwd -Fn -a -p "'$pidn2'" | # vypis lsof po radcich
  awk 'BEGIN {
    while (getline line > 0)
    {
      type = substr(line,1,1) # prvni znak na radku
      sub(\"^.\",\"\",line) # smazeme ho
      if (\"p\" == type)
        {
          pid = line # ulozime si cislo PID procesu
        }
      else if (\"n\" == type)
        {
          cwd = line; # ulozime si cestu CWD
          printf \"%d:%s\n\",pid,cwd # sparujeme PID:CWD
        }
    }
  }' | sed 's/ ([^(]*)$//g'" # smazani pripojneho bodu ve vypisu
  # param X, pro jmeno linku s cestou
  X_file='exe'
elif [ "$os" = "FreeBSD" ]
then  # freebsd
  C_cmd="procstat -f "'$pidn1'" 2>/dev/null | grep \"^[0-9][0-9]* [^ ]* *cwd\" |
         sed 's/^ *\([0-9][0-9]*\) .* \(\/.*\)$/\1:\2/g'"
  X_file='file'
else  # jiny os
  printHelp
  exit 1
fi
########################################


#########
## Cteni a osetreni parametru
#########
N_param="" # pro ulozeni regexpu u parametru n
N_isset=false # zda byl prepinac pouzit
A_param=""
A_isset=false
C_param=""
C_isset=false
X_param=""
X_isset=false
any_params=false # zda byl vubec nejaky prepinac pouzit

while getopts "n:a:c:x:" param; do
  case $param in
    n) N_param="$OPTARG"; N_isset=true; any_params=true;;
    a) A_param="$OPTARG"; A_isset=true; any_params=true;;
    c) C_param="$OPTARG"; C_isset=true; any_params=true;;
    x) X_param="$OPTARG"; X_isset=true; any_params=true;;
    ?) printHelp; exit 1;;
  esac;
done
# shift parametru, pokud neni 0, bylo zadano vic veci nez by melo
shift "$((OPTIND - 1))"

# pokud nemame platny parametr, nebo nam neco zbylo
if [ "$any_params" = false -o "$#" -ne 0 ]
then
  printHelp
  exit 1
fi

# pokud je s prepinacem n nebo a predan prazdny retezec
if [ "$N_isset" = true -a -z "$N_param" -o "$A_isset" = true -a -z "$A_param" ]
then
  printHelp
  exit 1
fi

# pokud je s prepinacem c nebo x predan prazdny retezec
if [ "$C_isset" = true -a -z "$C_param" -o "$X_isset" = true -a -z "$X_param" ] 
then
  printHelp
  exit 1
fi
########################################


#########
## Sestaveni prikazu pro parametry a ziskani PID procesu
#########
N_cnt='strings -n 1 2>/dev/null < /proc/$process/cmdline |
       head -n 1 | grep -c -- "$N_param"'
A_cnt='strings -n 1 2>/dev/null < /proc/$process/cmdline |
       tail -n +2 | grep -c -- "$A_param"'
X_name='readlink 2>/dev/null /proc/$process/$X_file'
# ziskani PID aktualnich procesu
pidn1=`ls /proc/[0-9]*/cmdline 2>/dev/null |
       sed 's/\/proc\/\([0-9][0-9]*\)\/cmdline/\1/g'`
########################################


#########
## Pokud je zadan parametr C, ziska se pro PID procesu jejich CWD
#########
if [ "$C_isset" = true ]
then
  if [ "$os" = "Linux" ]
  then  # pro linux
    # upraveni vypisu PID, oddeli je carkou (pro pouziti lsof na Linuxu)
    pidn2=`echo $pidn1 | sed 's/ /,/g'`
  fi
  # do promenne ulozi dvojice PID:CWD
  # vypis se ziska jen pro PID z $pidn1, aby se nevypsal CWD pro proces,
  # ktery neprochazime v hlavnim cyklu pro ostatni parametry
  C_list=`eval "$C_cmd"`
fi
########################################


#########
## Hlavni cyklus, postupne podle PID prochazi procesy
#########
for process in $pidn1
do
  ## jmeno procesu
  if [ "$N_isset" = true ]
  then   
    if [ `eval "$N_cnt"` -eq 0 ]
    then # pro zadany PID testuje, zda jmeno odpovida regexpu
      continue # proces nevyhovuje, jdem na dalsi pruchod cyklu
    fi
  fi
  

  ## argumenty
  if [ "$A_isset" = true ]
  then
    if [ `eval "$A_cnt"` -eq 0 ]
    then
      continue # proces nevyhovuje, jdem na dalsi pruchod cyklu
    fi
  fi


  ## srovnani spustitelneho souboru
  if [ "$X_isset" = true ]
  then
    X_fullname=`eval "$X_name"` # jmeno souboru z /proc/PID/[file|exe]
    if [ "$X_fullname" != "$X_param" ]
    then # pokud se nerovnaji
      continue # proces nevyhovuje, jdem na dalsi pruchod cyklu
    fi
  fi


  ## pracovni adresar CWD
  if [ "$C_isset" = true ]
  then
    # test CWD pro zadany proces
    C_cwd=`printf "$C_list" | grep '^'"$process"':.*$' |
           sed 's/^[0-9][0-9]*://' | grep -c -- "$C_param"`
    if [ "$C_cwd" -eq 0 ]
    then
      continue # proces nevyhovuje, jdem na dalsi pruchod cyklu
    fi
  fi

  # PID vyhovuje, vypis
  printf "$process\n"

done


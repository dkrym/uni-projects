##! /usr/bin/python3
# -*- coding: utf-8 -*-
#CHA:xkrymd00

import getopt, sys, os,re

##
# Vstupni bod programu
##
def main():
    # zpracovani parametru
    param = dict(P_input="", P_output="", P_pretty=-1, P_noinline=0, P_par=-1)
    param = paramParse(param)
    
    # volba/uprava adresare pro vypis
    if (param['P_input'] == ""): dir_val = "./"
    elif (os.path.isfile(param['P_input'])): dir_val = ""
    elif (os.path.isdir(param['P_input'])): dir_val = param['P_input']
    else: printErr("XNONFILE", param['P_input'])
    
    # ziskani seznamu souboru na zpracovani
    all_files = getFileList(param['P_input'])

    # zda za elementama bude newline
    if (param['P_pretty'] == 0): newline = ""
    else: newline = "\n"
    
    # obalka, volani funkce pro vypis (odsazeneho) tela
    out = '<?xml version="1.0" encoding="iso-8859-2"?>' + newline
    out += '<functions dir="' +dir_val+ '">' + newline
    out += processFiles(param, dir_val, all_files)
    out += '</functions>'
    
    # samotny vypis na stdin/do souboru
    if (param['P_output'] == ""):
        print(out)
    else:
        try:
            with open(param['P_output'], 'w', encoding='iso-8859-2') as f:
                print(out,file=f)
        except:
            printErr("OUTFILE", param['P_output'])
#### END main()


##
# Funkce pro zpracovani prametru. Kontroluje vicenasobne zadani a nastavuje
# vychozi hodnoty nezadanym parametrum.
#   param - inicializovany slovnik moznych parametru
#   return - slovnik, obsahuje zadane parametry, pripadne defaultni hodnoty
##
def paramParse(param):
    # jen vypsat napovedu
    if ((len(sys.argv) == 2) and (sys.argv[1] == "--help")):
        usage()
        sys.exit(0)
    
    p = re.compile(r"([^=]+)=(.+)") # RE pro parametry s hodnotou
    
    for arg in sys.argv[1:]:
        if (arg == "--help"):
            printErr("HPARAM", arg)
        elif (arg == "--no-inline"):
            if (param['P_noinline'] == 0): param['P_noinline'] += 1
            else: printErr("MULTIPARAM", arg)
        elif (arg == "--pretty-xml"):
            if (param['P_pretty'] == -1): param['P_pretty'] = 4
            else: printErr("MULTIPARAM", arg)
        else: # parametry s hodnotou
            m = p.match(arg)  
            try:
                if (m):
                    if (m.group(1) == "--input"):
                        if (param['P_input'] == ""): param['P_input'] = m.group(2)
                        else: printErr("MULTIPARAM", m.group(1))
                    elif (m.group(1) == "--output"):
                        if (param['P_output'] == ""): param['P_output'] = m.group(2)
                        else: printErr("MULTIPARAM", m.group(1))
                    elif (m.group(1) == "--max-par"):
                        if (param['P_par'] == -1):
                            param['P_par'] = int(m.group(2))
                            if (param['P_par'] < 0): printErr("INTPARAM", "")
                        else: printErr("MULTIPARAM", m.group(1))
                    elif (m.group(1) == "--pretty-xml"):
                        if (param['P_pretty'] == -1):
                            param['P_pretty'] = int(m.group(2))
                            if (param['P_pretty'] < 0): printErr("INTPARAM", "")
                        else: printErr("MULTIPARAM", m.group(1))
                    else:
                        printErr("XPARAM",arg)
                else:
                    printErr("XPARAM",arg)
            except ValueError:
                printErr("INTPARAM", "")

    # vychozi hodnota pri nezadani --pretty-xml
    if (param['P_pretty'] == -1): param['P_pretty'] = 0
        
    return param
#### END paramParse()
    
    
##
# Vypise napovedu.
##
def usage():
    msg="""Skript C Header Analysis pro analyzu hlavickovych souboru jazyka C.
Vytvari databazi nalezenych funkci ve formatu XML.
Autor: David Krym, xkrymd00@stud.fit.vutbr.cz

Pouziti: cha.py [PREPINACE]
  --help                  Vypise tuto napovedu.
  --input=[soubor|slozka] Vstupni soubor/slozka. Vychozi nastaveni na stdin.
  --output=soubor         Vystupni soubor. Vychozi nastaveni na stdout.
  --pretty-xml[=n]        Odsazeni ve vystupu. Vychozi hodnota je 4.
  --no-inline             Skript preskoci funkce se specifikatorem inline.
  --max-par=n             Bere v uvahu funkce s maximalnim poctem parametru n."""
    
    print(msg)
#### END usage()


##
# Funkce pro vypis chybovych hlasek a ukonceni
# skriptu se spravnym navratovym kodem.
#   category - kategorie chyby, kvuli zvoleni navratoveho kodu skriptu
#   err - konkretni identifikace chyby
##
def printErr(category,err):
    retcode = 1
    
    if (category == "HPARAM"):
        print("Parametr nelze kombinovat s ostanimi:", err, file=sys.stderr)
    elif (category == "XPARAM"):
        print("Zadan neznamy parametr:", err, file=sys.stderr)
    elif (category == "MULTIPARAM"):
        print("Vicenasobny vyskyt parametru:", err, file=sys.stderr)
    elif (category == "INTPARAM"):
        print("S nekterym parametrem nebylo zadano cele kladne cislo.", file=sys.stderr)
    elif (category == "XNONFILE"):
        print("Neexistujici soubor/slozka:", err, file=sys.stderr)
        retcode = 2
    elif (category == "INFILE"):
        print("Chyba pri praci se vstupnim souborem:", err, file=sys.stderr)
        retcode = 2
    elif (category == "OUTFILE"):
        print("Chyba pri praci s vystupnim souborem:", err, file=sys.stderr)
        retcode = 3
    
    sys.exit(retcode)
#### END printErr()


##
# Funkce pro ziskani seznamu souboru. Pri zadani souboru se vrati stejna cesta.
# Pri zadani adresare se vrati seznam vsech souboru .h ze vsech podadresaru.
#   path - cesta souboru nebo adresare
#   return - seznam relativnich cest k .h souborum
##
def getFileList(path):
    all_paths = [] # vysledne pole vsech souboru
    
    if (path == ""): path = "./" # nezadana cesta == aktualni adresar
    
    if (os.path.isfile(path)): all_paths.append(path) # zadan primo soubor
    else: # zadana slozka
        for (dirpath, dirnames, filenames) in os.walk(path):
            # relativni cesty .h souboru vlozi do pole
            for filename in filenames:
                relpath = os.path.relpath(os.path.join(dirpath, filename), path)
                if (relpath.endswith(".h")): all_paths.append(relpath)
    
    return all_paths
#### END getFilesList()


##
# Funkce pro zpracovani souboru. Na kazdy soubor zavola pomocnou funkci pro
# parsovani funkci a vysledek vypise s pripadnym odsazenim.
#   param     - slovnik se zadanyma parametrama
#   dir_val   - zadana cesta ke slozce, soubory se berou relativne k ni
#   all_files - seznam vsech souboru ke zpracovani
##
def processFiles(param, dir_val, all_files):
    max_params = param['P_par']
    indent = param['P_pretty']
    noinline = param['P_noinline']
    out = ""
    if (indent == 0): newline = ""
    else: newline = "\n"
    
    for one_file in all_files: # provest pro vsechny soubory
        # ziska pole se slovnikama, ve slovniku jsou informace o funkci
        returned_funcs = getFunctions(os.path.join(dir_val, one_file), noinline)
        
        # vypis s pripadnym odsazenim
        for func in returned_funcs:
            if (max_params == -1) or (len(func['params']) <= max_params): # vypise funkce s urcenym poctem parametru a mensim
                out += indent*' '+'<function file="' +one_file+ '" name="' +func['name']+ '" varargs="' +func['varargs']+ '" rettype="' +func['rettype']+ '">' + newline
                if (func['params'] != []): # ma parametry
                    cnter = 1
                    for par in func['params']: # pro kazdy parametr
                        out += indent*'  ' + '<param number="' +str(cnter)+ '" type="' +par+ '" />' + newline
                        cnter += 1
                out += indent*' ' + '</function>' + newline

    return out
#### END processFiles()


##
# Funkce pro parsovani hlavicek funkci ze vstupu.
#   path     - konkretni cesta k souboru
#   noinline - zda byl zadan parametr --no-inline
##
def getFunctions(path, noinline):
    try:
        with open(path, encoding='iso-8859-2') as f:
            soubor = f.read() # cely soubor
    except:
        printErr("INFILE", path)

    soubor = re.sub(r'/\*.*?\*/', '', soubor, flags=re.DOTALL) # bez viceradkovych komentaru
    soubor = re.sub(r'//.*$', '', soubor, flags=re.MULTILINE) # bez jednoradkovych komentaru
    soubor = re.sub(r'^\s*#.*$', '', soubor, flags=re.MULTILINE) # bez maker
    soubor = re.sub(r'".*?(?<!\\)"', '""', soubor, flags=re.MULTILINE) # bez retezcu
    while (re.search(r'\{[^{}]*\}', soubor) != None): # odstrani tela funkci
        soubor = re.sub(r'\{[^{}]*\}',';', soubor,)
    soubor = re.sub(r';;', ';', soubor) # smaze zdvojene stredniky

    # najde jednotlive funkce a ulozi je cele do pole
    all_func = re.findall(r'(\w[^;{}]*\([^()]*\))\s*(?=;|\{)', soubor, flags=re.DOTALL)

    array_of_funcs = [] # vyslede pole slovniku
    funcs_names = [] # lokalne jmena funkci, kvuli zamezeni duplicite jmena
    pattern_func = re.compile(r'(.*?)(\w+)\s*\((.*)\)', flags=re.DOTALL) # pattern pro rozsekani funkce
    pattern_param = re.compile(r'(.*?)(?:\w+)\s*$', flags=re.DOTALL) # pattern pro rozsekani parametru

    for func in all_func:
        one_func = dict(name="", varargs="no", rettype="", params=[]) # prave jedna fce
        
        m = pattern_func.match(func) # rozsekani fce
        if (m == None): continue # chybna fce
        
        one_func['rettype'] = m.group(1).strip() # navratovy typ funkce
        if (noinline): # bez inline funkci, pri nalezeni inline preskocit
            if (one_func['rettype'].find("inline") != -1): continue
            
        one_func['name'] = m.group(2).strip() # jmeno funkce

        params = m.group(3); # vsechny parametry, nerozsekane
        
        if (params != ""):
            params = params.split(',') # pole, jednotlive parametry typ+jmeno        
            
            if (params[-1].strip() == "..."): # ma varargs
                one_func['varargs'] = "yes"
                del params[-1]
        
        for par_type in params: # typ parametru
            m = pattern_param.match(par_type) # rozsekani paramu
            if (m == None): continue # chybna fce
            if (m.group(1).strip() != ""): # jen pokud je uveden typ+jmeno
                one_func['params'].append(m.group(1).strip())

        if (one_func['name'] in funcs_names): # funkce se stejnym jmenem uz byla zpracovana
            same_func = array_of_funcs[funcs_names.index(one_func['name'])]
            if (same_func['params'] == one_func['params']) and (same_func['varargs'] == one_func['varargs']):
                pass # uplne stejnou funkci nepridavat
            else:
                array_of_funcs.append(one_func) # s jinyma parametrama pridam
        else:
            funcs_names.append(one_func['name']) # lokalne ulozit jmena funkci
            array_of_funcs.append(one_func) # vlozit slovnik s info o funkci

    return array_of_funcs
#### END getFunctions()



if __name__ == "__main__":
    main()

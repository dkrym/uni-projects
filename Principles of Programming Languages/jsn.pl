#!/usr/bin/perl -w

#JSN:xkrymd00

# pragma
use strict;
# moduly
use Switch;
use JSON;
use IO::File;
use XML::Writer;
use Encode;

# prototypy funkci
sub jsonParser($);
sub paramErr($);
sub paramParser(@);
sub printHelp;
sub printErr($);

# globalni promenne
my $param_in = "";
my $param_out = "";
my $param_r = "";
my $param_padding = 0;
my $param_start = -1;
my $param_help = 0;
my $param_n = 0;
my $param_s = 0;
my $param_i = 0;
my $param_l = 0;
my $param_e = 0;
my $param_a = 0;
my $param_t = 0;
my $out_str = "";

### MAIN

  # zpracovani parametru do globalnich promennych
  paramParser(@ARGV);

  # nacteni json souboru, zisk reference na data
  open(IN_FH, $param_in) or printErr("EIN");
  local $/;
  my $json_ref;
  eval { $json_ref = JSON->new->utf8->decode(<IN_FH>); };
  if ($@) { printErr("EINFORMAT"); } # chyba pri nacitani json
  close IN_FH;

  if ((ref $json_ref eq "ARRAY") and !$param_e)
    { printErr("EGLOBARR"); }

  # inicializace XML writeru, zapisuje do promenne
  my $writer = new XML::Writer(OUTPUT => \$out_str, UNSAFE => 1,
                                DATA_MODE => 1, DATA_INDENT => 2,
                                ENCODING => 'utf-8');
                                
  if (!$param_n) # generovat hlavicku
    { $writer->xmlDecl("UTF-8"); } 
  if ($param_r) # obalit do root elementu
    { $writer->startTag($param_r); } 
  jsonParser($json_ref);
  if ($param_r)
    { $writer->endTag($param_r); }

  # zapis vysledneho XML souboru
  open(OUT_FH, ">".$param_out) or printErr("EOUT");
  binmode OUT_FH, ":encoding(utf8)";
  print OUT_FH $out_str;
  close OUT_FH;

  exit(0);
  
### MAIN


# Funkce pro vypis chyb pri zpracovani parametru.
#   $param - predany parametr, ktereho se chyba tyka
sub paramErr($)
{
  my $param = shift;
  if ($param eq "--help")
    { warn "Parametr ".$param." nelze kombinovat s jinymi parametry!\n"; }
  elsif ($param eq "perr")
    { warn "Zadan neznamy parametr!\n"; }
  elsif ($param eq "enotr")
    { warn "Parametr -e lze zadat jen v kombinaci s parametrem -r!\n"; }
  elsif ($param eq "startnotnum")
    { warn "S parametrm --start nebylo zadano cele cislo!\n"; }
  else
    { warn "Parametr ".$param." byl zadan vicekrat!\n"; }
  exit(1);  
}


# Funkce pro vypis chyb az po zpracovani parametru.
# Ukoncuje program se zadanymi navratovymi kody.
#   $ecode - identifikator chyby
sub printErr($)
{
  my $ecode = shift;
  my $retcode = 0;
  if ($ecode eq "EIN")
    { warn "Chyba pri otevirani vstupniho souboru.\n"; $retcode = 2; }
  elsif ($ecode eq "EOUT")
    { warn "Chyba pri otevreni vystupniho souboru.\n"; $retcode = 3; }
  elsif ($ecode eq "EINFORMAT")
    { warn "Chybny format vstupniho souboru.\n"; $retcode = 4; }
  elsif ($ecode eq "EGLOBARR")
    { warn "Globalni pole neni obaleno korenovym objektem. Pouzijte -e!\n"; $retcode = 4; }
  
  exit($retcode); 
}


# Funkce pro zpracovani parametru z konzole.
#   @_ - pole vsech zadanych parametru
sub paramParser(@)
{
  if (@_ == 1 and $_[0] eq "--help")
    { printHelp; } # jen vypsat napovedu
  
  for (@_)
  { # parsovani parametru, hlidani vicenasobneho zadani
    switch ($_)
    { # parametry bez hodnoty
      case "--help" { paramErr($_); }
      case "-n" { $param_n++; if ($param_n > 1) { paramErr($_); } }
      case "-s" { $param_s++; if ($param_s > 1) { paramErr($_); } }
      case "-i" { $param_i++; if ($param_i > 1) { paramErr($_); } }
      case "-l" { $param_l++; if ($param_l > 1) { paramErr($_); } }
      case "--padding" { $param_padding++; if ($param_padding > 1) { paramErr($_); } }
      case ["-e", "--error-recovery"] { $param_e++; if ($param_e > 1) { paramErr($_); } }
      case ["-a", "--array-size"] { $param_a++; if ($param_a > 1) { paramErr($_); } }
      case ["-t", "--index-items"] { $param_t++; if ($param_t > 1) { paramErr($_); } }
    
      case /^(.*)=(.*)$/
      { # parametry s hodnotou
        $_ =~ /^([^=]*)=(.*)$/;
        switch ($1)
        { # o co se jedna
          case "--input" { if (!$param_in) {$param_in = glob($2);} else { paramErr($1) } }
          case "--output" { if (!$param_out) {$param_out = glob($2);} else { paramErr($1) } }
          case "--start" { if ($param_start == -1) {$param_start = $2;} else { paramErr($1) } }
          case "-r" { if (!$param_r) { $param_r = $2; utf8::decode($param_r); } else { paramErr($1) } }
          else { paramErr("perr"); }
        }      
      }
      # nevyhovuje zadnemu parametru
      else { paramErr("perr"); }
    }  
  }

  # test nekorektnich hodnot
  if (($param_start >= 0) and ($param_start !~ /^[0-9]+$/))
    {paramErr("startnotnum"); }
  if ($param_e and !$param_r)
    { paramErr("enotr"); }
  if ($param_t and ($param_start == -1))
    { $param_start = 1; } # od jednicky
  if (!$param_in)
    { $param_in = "-"; } # stdin
  if (!$param_out)
    { $param_out = "-"; } # stdout

  return;
}


# Funkce pro parsovani json dat.
#   $json - reference na json data
sub jsonParser($)
{
  my $json = shift; # reference na nacteny json
  
  if (ref $json eq "HASH")
  { # jedna se o hash
    foreach my $jkey (keys %$json)
    { # pro kazdy klic hashe   
      if ((ref $$json{$jkey} eq "HASH") or (ref $$json{$jkey} eq "ARRAY"))
      { # znovu reference -> rekurze
        $writer->startTag($jkey);
        jsonParser($$json{$jkey});
        $writer->endTag($jkey);
      }
      else
      { # normalni hodnota
        # dle zadanych parametru se ulozi xml do $out_str
        formatValue($jkey, $$json{$jkey}, -1)
      }
    }
  }
  elsif (ref $json eq "ARRAY")
  { # jedna se o pole
    my $arr_len = @$json; # pocet polozek
    my $formated_cnter; # spravne naformatovany index
    my $item_cnter = $param_start; # inicializace citace v itemu
    my $max_item_num_len = length($arr_len + $param_start -1); # max pocet cifer
    my $cnter_format = "%0".$max_item_num_len."d"; # format vypisu indexu

    if ($param_a) # s parametrem -a vypsat velikost pole
      { $writer->startTag("array", "size" => $arr_len); }
    else
      { $writer->startTag("array"); }
    
    foreach my $item (@$json)
    { # pro kazdy prvek pole 
      if ((ref $item eq "HASH") or (ref $item eq "ARRAY"))
      { # reference
        if ($param_t)
        { # cislovani jednotlivych itemu
          if ($param_padding) # na kolik mist zarovnavat nulama zleva
            { $formated_cnter = sprintf($cnter_format, $item_cnter++); }
          else
            { $formated_cnter = $item_cnter++; }
          
          $writer->startTag("item", "index" => $formated_cnter);
        }
        else
          { $writer->startTag("item"); }
        
        # reference -> rekurzivni volani parseru
        jsonParser($item);
        $writer->endTag("item");
      }
      else
      { # normalni hodnota
        # dle zadanych parametru se ulozi xml do $out_str
        if ($param_t)
        { # cislovani jednotlivych itemu
          if ($param_padding)
            { $formated_cnter = sprintf($cnter_format, $item_cnter++); }
          else
            { $formated_cnter = $item_cnter++; }
          
          formatValue("item", $item, $formated_cnter)
        }
        else
          { formatValue("item", $item, -1); }
      }
    }
    # konec pole
    $writer->endTag("array");
  }

  return;
}


# Funkce pro formatovani a vypis hodnot dle zadanych parametru.
#   $key - Jmeno elementu
#   $value - Hodnota/atribut alementu
#   $index - Index (jen pro pole)
sub formatValue($$$)
{
  my ($key, $value, $index) = @_;
  my $as_elem = 0;
  # prevod do JSON, retezce jsou v uvozovkach
  my $dump = JSON::XS->new->allow_nonref->encode($value);
  $dump = $dump =~ /"(.*)"/ ; # retezec?

  # upravy jednotlivych hodnot
  if (!defined($value))
  { # null
    if ($param_l) # jako element?
      { $as_elem = 1; }
      
    $value = "null";
  }
  elsif (JSON::is_bool($value))
  { # true/false  
    if ($param_l) # jako element?
      { $as_elem = 1; }
  
    if ($value eq JSON::true)
      { $value = "true"; }
    else
      { $value = "false"; }
  }
  elsif (($dump and $param_s) or (!$dump and $param_i)) 
    { $as_elem = 2; } # cislo/retezec jako textovy elementr
  
  # samotny vypis
  if ($as_elem == 1)
  { # null, true, false jako element
    if ($index < 0)
      { $writer->startTag($key); }
    else
      { $writer->startTag($key, "index" => $index); }  
    
    $writer->emptyTag($value);
    $writer->endTag($key);
  }
  elsif ($as_elem == 2)
  { # retezec, cislo jako textovy element
    if ($index < 0)
      { $writer->startTag($key); }
    else
      { $writer->startTag($key, "index" => $index); }
    
    $writer->characters($value);
    $writer->endTag($key);
  }
  else
  { # jako atribut
    if ($index < 0) { $writer->emptyTag($key, "value" => $value); }
    else { $writer->emptyTag($key, "index" => $index, "value" => $value); } # index u pole
  }
  
}


# Funkce pro vypis napovedy.
sub printHelp
{
  print <<END;
Program JSON2XML na prevod formatu JSON do XML.
Autor: David Krym

Pouziti: $0 [PREPINACE]

  --help\t\tVypise tuto napovedu.
  --input=filename\tZadany vstupni JSON soubor. Nezadan = STDIN.
  --output=filename\tTextovy vystupni XML soubor. Nezadan = STDOUT.
  -n\t\t\tNegenerovat XML hlavicku na vystup skriptu.
  -r=root-element\tKorenovy element obalujici vysledek.
  -s\t\t\tHodnoty typu string transformovat na elementy.
  -i\t\t\tHodnoty typu number transformovat na elementy.
  -l\t\t\tHodnoty (true, false, null) transformovat na elementy.
  -e, --error-recovery\tZotaveni z chybejiciho obaleni pole. Nutno s (-r).
  -a, --array-size\tU pole doplnit atribut size.
  -t, --index-items\tK prvkum pole doplnit atribut index.
  --start=n\t\tPocatecni hodnota citace pro pole (-t)
  --padding\t\tZarovnani citacu nejmensim poctem nul zleva.

END
  
  exit(0);
}


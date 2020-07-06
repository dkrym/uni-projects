1. O programu
Aplikace je tvořena ze dvou programů -- rdtclient a rdtserver.
Po spuštění server naslouchá a čeká na příjem jednotlivých řádků textu od
klienta. Klient po spuštění čeká na text ze standardního vstupu o maximální
délce 80 znaků. Každý celý řádek se okamžitě odesílá na server až do té doby,
než se na vstupu objeví konec souboru (Ctrl+D, EOF), čímž přenos končí.


2. Komunikace mezi programy

 2.1 Paket
 Každý řádek textu ukončený znakem "\n" slouží pro sestavení paketu. K textu se
 přidá identifikační číslo paketu a z těchto dat se vypočítá kontrolní součet
 (použit byl algoritmus Fletcher), který se znovu k textu připojí. Toto je
 výsledný paket, kde jsou jednotlivé položky odděleny oddělovačem "|".
 Formát paketu je tak následující: "checksum|packet_id|message"

 2.2 Komunikace
 Klient po spuštění začne s časovačem posílat HANDSHAKE do té doby, než server
 odpoví zprávou ACK. Nyní se klient přepne do režimu odesílání dat a server se
 přepne do režimu příjmu dat. Nyní začne probíhat samotná komunikace. Pokud jsou
 všechna data odeslána, klient pošle na server 5x zprávu ENDSHAKE. Poté se oba
 programy ukončí.


3. Implementační detaily
"Sliding window" je implementováno jako kruhové pole o deseti paketech. Pro
spolehlivý přenos používám upravený protokol Go-Back-N. Upravený tak, že při
vypršení časovače se odešlou všechny pakety aktuálně v okně. Zároveň, pokud od
serveru přijde žádost o paket, který je první v okénku, klient tento paket ihned
odesílá, aby se nemuselo čekat na vypršení časovače.
 Server má zároveň implementovaný lineární seznam, do kterého si ukládá příchozí
pakety, které bude potřebovat v budoucnu. Při příchodu správného paketu se jen
seznam prohledá na případné potřebné pakety a až potom se posílá potvrzení.

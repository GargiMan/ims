---
title: "Modelování a simulace - Model v energetice"
author: "Marek Gerger (xgerge01)"
date: "November 25, 2024"
abstract: "Pokrytie a predpoveď spotreby elektrickej energie v domácnosti pomocou fotovoltaických panelov za bežných (špecifikovaných) podmienok."
fontsize: 12pt
papersize: A4
documentclass: article
header-includes:
    - \usepackage{fancyhdr}
lang: sk
---

\thispagestyle{empty}
\newpage
\tableofcontents
\newpage

# Úvod

Táto simulačná štúdia sa zaoberá modelovaním a simuláciou domácnosti využívajúcej elektrickú energiu z fotovoltaických panelov. Cieľom je vytvoriť model, ktorý bude schopný predpovedať spotrebu elektrickej energie v domácnosti na základe historických dát a aktuálnych meteorologických podmienok. Model by mal odhaliť možné oblasti úspor a optimalizácie spotreby elektrickej energie, ale aj limity aktuálneho systému a jeho schopnosť pokryť spotrebu elektrickej energie v domácnosti v rôznych situáciách.

## Validita modelu

Modelačné dáta boli získané z informačného systému Victron Energy na správu elektrickej siete v domácnosti. Systém poskytuje informácie o spotrebe, výrobe a distribúcii elektrickej energie v reálnom čase a tiež históriu týchto údajov.  
Pre úplnosť boli doplnené informácie o funkčnosti a kapacitách tohto systému v komunikácii s firmou, ktorá tento systém inštalovala.

![Victron Energy IS](docs/VictronEnergyIS.png)

\newpage

# Rozbor témy a faktov

Fotovoltaických systémov existuje mnoho druhov, ktoré sa líšia svojimi vlastnosťami a schopnosťami. Pozorovaný systém v tejto štúdii je zložený z 35 fotovoltaických panelov s celkovým výkonom 8kW, 2 paralelne zapojených veľkokapacitných batérii s kapacitou 620Ah a nominálnym napätím 48V, a 3 meničmi napätia Victron Energy MultiPlus 48V 50A, ktoré slúžia na prevod napätia z batérií na napájanie domácnosti. Systém je taktiež napojený na verejnú elektrickú sieť, ktorá slúži ako záložný zdroj elektrickej energie v prípade, že fotovoltaický systém nedokáže pokryť celú spotrebu domácnosti. Kapacita verejnej elektrickej siete je limitovaná ochranou pred preťažením 16A na každej fáze. Systém je vhodne nadimenzovaný, a tak je možné počítať so súčtom výkonu všetkých fáz elektrického systému.

Priemerná mesačná spotreba elektrickej energie v domácnosti tohto systému je 350kWh. Počas letných mesiacov je systém schopný vyrobiť priemerne 700kWh elektrickej energie mesačne,ale v zimných mesiacoch tento výkon klesá na 300kWh mesačne. Počas zimných mesiacov je teda nutné doplniť spotrebu elektrickej energie z verejnej elektrickej siete.

Rozloženie spotreby elektrickej energie v domácnosti počas dňa je nepravidelné a závisí od aktuálnej situácie v domácnosti. Najbližšie sa k tejto spotrebe blíži lognormálna distribúcia, tá vykazuje, že najčastejšie je spotreba elektrickej energie stabilne nízka a len zriedkavo sa vyskytujú výkyvy spotreby elektrickej energie, ako napríklad pri používaní veľkých spotrebičov, ako sú práčky, sušičky, alebo rýchlovarné kanvice.

\newpage

# Koncepcia

Model, typu SHO (Systém hromadnej obsluhy), popisuje jednoduchý abstraktný model domácnosti, ktorý je schopný simulovať distribúciu elektrickej energie v domácnosti.  
Všetky jednotky systému sú prevedené na jednotky elektrickej energie v sekundových intervaloch, takže spotreba elektrickej energie je v modeli ukazovateľom aktuálnej spotreby v aktuálnom momente uvádzaná vo _W_ a nie celkovou spotrebou za určitý časový interval, ktorá je typicky úvádzana v _kWh_ pri spotrebe elektrickej energie.

## Popis modelu

Model obsahuje 2 zdroje energie so špecifikovanou kapacitou: batéria (Battery) a verejná sieť (Grid). Model tiež obsahuje zariadenie inverter (Inverter), ktoré nie je ako zdroj energie, ale obmedzuje maximálne využitie energie z batérie. Pri batérii je táto kapacita definovaná ako kapacita batérie vo Watt-och dostupenej energie. Pri inverteri je táto kapacita definovaná ako maximálny výkon, ktorý je inverter schopný dodávať do domácnosti. Rovnako je definovaná aj kapacita verejnej siete, ktorá je limitovaná ochranou pred preťažením.

Generovanie požiadavkov na výrobu energie je realizované pomocou generátoru (PowerChargeGenerator), ktorý generuje požiadavky výrobu energie v závislosti od aktuálneho času, ktoré je simulované pomocou sinusovej funkcie a závisí od aktuálneho počasia. Počasie je generované pomocou náhodného generátora a prechodovej matice. Generuje náhodné hodnoty v závislosti od konfigurovateľných pravdepodobností oblačnosti a daždivého počasia. Počasie ovplyvňuje efektivitu fotovoltaických panelov.

Generovanie požiadaov na spotrebu energie je realizované pomocou generátora (PowerRequiredGenerator), ktorý generuje požiadavky na spotrebu energie na základe lognormálnej distribúcie spotreby elektrickej energie v domácnosti.

Generátory sú konfigurovateľné a umožňujú nastavenie rôznych parametrov podľa systému alebo simulácie, ako napríklad priemerná spotreba elektrickej energie v domácnosti, priemerná produkcia elektrickej energie fotovoltaickými panelmi, alebo kapacita batérie.

## Sledovanie stavu modelu

Model sleduje parametre ako je aktuálny požadovaný výkon, aktuálny výkon zo solárnych panelov ale aj množstvo energie, ktoré nebolo možné do batérie uložiť. Histogram týchto hodnôt ukazuje výkon systému v priebehu simulácie, ktorý je zaznamenávaný v sekundových intervaloch.
Model taktiež sleduje dostupný výkon z batérie a využitý výkon invertorom a verejnej siete. Histogram týchto hodnôt zobrazuje v ktorých kapacitách sa systém najčastejšie pohybuje a je zaznamenávaný v konfigurovateľných intervaloch.
Z pozorovania týchto hodnôt je možné odhaliť limity v systéme.

\newpage

# Implementácia

Model je naimplementovaný pomocou C++ a knižnice SIMLIB.

Program je možné preložiť pomocou Makefile príkazu `make`. Experimenty modelu je možné spustiť pomocou príkazu `make run`.
Na modeli je taktiež možné spustiť vlastné experimenty s jednotlivými parametrami.

`./main <daysOfSimulation> <startOfSimulationHour> <batteryChargePercentage> <sunriseHour> <sunsetHour> <cloudyProbabilityPercentage> <rainyProbabilityPercentage>`

-   `daysOfSimulation` - počet dní simulácie
-   `startOfSimulationHour` - hodina, v ktorej sa má začať simulácia; vhodné ak chceme simuláciu začať v priebehu dňa
-   `batteryChargePercentage` - percentuálna úroveň nabitia batérií na začiatku simulácie
-   `sunriseHour` - hodina východu slnka
-   `sunsetHour` - hodina západu slnka
-   `cloudyProbabilityPercentage` - percentuálna pravdepodobnosť oblačnosti
-   `rainyProbabilityPercentage` - percentuálna pravdepodobnosť daždivého počasia

\newpage

# Experimenty

## Experiment 1

-   Počet dní simulácie: 1
-   Začiatok simulácie: 0:00
-   Počiatočný stav batérie: testovaný
-   Čas východu slnka: 6:45
-   Čas západu slnka: 16:30
-   Pravdepodobnosť oblačnosti: 80%
-   Pravdepodobnosť daždivého počasia: 10%

| Názov simulácie | Stav batérie na začiatku | Primerný stav batérie | Primerná nevyužitá energia |
| --------------- | ------------------------ | --------------------- | -------------------------- |
| exp1_1          | 90%                      | 92%                   | 2605W                      |
| exp1_2          | 70%                      | 82%                   | 1353W                      |
| exp1_3          | 50%                      | 63%                   | 0W                         |
| exp1_4          | 30%                      | 43%                   | 0W                         |
| exp1_5          | 10%                      | 23%                   | 0W                         |

Experiment testuje vplyv počiatočného stavu batérie na priemerný stav batérie počas dňa a množstvo energie, ktoré tento batéria nedokázala uložiť. Výsledky experimentu ukazujú, že počiatočný stav batérie má významný vplyv na výsledky simulácie. Taktiež tento experiment vykazuje informáciu o tom, že výkon solárnych panelov je dostatočný na pokrytie spotreby elektrickej energie v domácnosti počas dňa pri bežných podmienkach, kedy je väčšia pravdepodobnosť oblačnosti a tak je výkon solárnych panelov nižší.

\newpage

## Experiment 2

-   Počet dní simulácie: testované
-   Začiatok simulácie: 0:00
-   Počiatočný stav batérie: 20%
-   Čas východu slnka: 7:00
-   Čas západu slnka: 16:00
-   Pravdepodobnosť oblačnosti: 80%
-   Pravdepodobnosť daždivého počasia: 80%

| Názov simulácie | Počet dní simulácie | Primerný stav batérie | Primerná nevyužitá energia |
| --------------- | ------------------- | --------------------- | -------------------------- |
| exp2_1          | 2                   | 29%                   | 0W                         |
| exp2_2          | 4                   | 38%                   | 0W                         |
| exp2_3          | 6                   | 47%                   | 0W                         |
| exp2_4          | 8                   | 56%                   | 802W                       |
| exp2_5          | 10                  | 62%                   | 1885W                      |

Experiment testuje počet dní potrebných na dobitie batérie na 100% pri nepriaznivom počasí. Výsledky experimentu ukazujú, že pri nepriaznivých podmienkach je potrebné viac dní na dobitie batérie na 100%.

\newpage

# Záver

Pri experimentoch sa ukázalo, že systém solárnych panelov je schopný pokryť spotrebu elektrickej energie pri dostatočnom dobití batérie počas dňa aj v noci, keď nie je vyrábaná elektrická energia. Taktiež sa ukázalo, že systém je schopný pokryť spotrebu elektrickej energie aj počas oblačného počasia, ale pri daždivom počasí je nutné doplniť spotrebu elektrickej energie z verejnej elektrickej siete.

Model je schopný predpovedať spotrebu elektrickej energie v domácnosti na základe očakávaných a aktuálnych podmienok. Simulácie taktiež ukázali, že v aktuálnej konfigurácii je systém schopný pokryť spotrebu elektrickej energie v domácnosti aj počas nepriaznivých podmienok alebo dlhodobého výpadku verejnej siete, pri dostatočnom dobití batérie.

Výsledky experimentov simulácii boli konzultované s firmou, ktorá inštalovala fotovoltaický systém a boli schválené ako validné. Čiastočné porovnanie výsledok týchto experimentov prebehlo aj s reálnymi dátami z informačného systému Victron Energy a vykazovalo dostatočnú koreláciu.

\newpage

# Zdroje

[1] Peringer,P.: Modelovani a simulace. FIT VUT, [online] [https://www.fit.vutbr.cz/study/courses/IMS/public/prednasky/IMS.pdf](https://www.fit.vutbr.cz/study/courses/IMS/public/prednasky/IMS.pdf)  
[2] Fotovoltaika, [online] [https://en.wikipedia.org/wiki/Photovoltaics](https://en.wikipedia.org/wiki/Photovoltaics)  
[3] Priemerná spotreba energie domácností na Slovensku, [online] [https://www.vyhodnaenergia.sk/blog/126/elektrina/priemerna-spotreba-elektriny-v-domacnosti](https://www.vyhodnaenergia.sk/blog/126/elektrina/priemerna-spotreba-elektriny-v-domacnosti)  
[4] Elektro energetika solárnych panelov, [online] [https://www.vyhodnaenergia.sk/blog/18/elektrina/fotovoltaika-vyhodene-peniaze](https://www.vyhodnaenergia.sk/blog/18/elektrina/fotovoltaika-vyhodene-peniaze)  
[5] Štatististické informácie o elektro energetike na Slovensku, [online] [https://www.nationmaster.com/country-info/profiles/Slovakia/Energy](https://www.nationmaster.com/country-info/profiles/Slovakia/Energy)  
[6] Šetrenie elektrickej energie, [online] [https://www.geotherm.sk/priemerna-spotreba-elektrickej-energie-v-domacnosti-ako-usetrit-za-elektrinu/](https://www.geotherm.sk/priemerna-spotreba-elektrickej-energie-v-domacnosti-ako-usetrit-za-elektrinu/)  
[7] Čas východu/západu slnka 2024, [online] [https://www.suntoday.org/sunrise-sunset/2024.html](https://www.suntoday.org/sunrise-sunset/2024.html)  
[8] Počasie v Trnavskom okrese 2024, [online] [https://www.meteoblue.com/sk/po%C4%8Dasie/historyclimate/climatemodelled/trnava_slovensko_3057124](https://www.meteoblue.com/sk/po%C4%8Dasie/historyclimate/climatemodelled/trnava_slovensko_3057124)  


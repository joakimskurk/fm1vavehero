# Får alt plass på FM-1? Minne, CPU og grafikk

**Kort svar: ja.** Minnet har god plass til motoren, grafikken, 30 innebygde låter, 8 egne låter og alle poengene.
Grafikken koster lite. Lyden er den trange delen. Slik den er i referansen, ligger den på kanten, men med faste
grenser ender den rundt 30 % av brikken.

Tallene under er **estimater** fra Dinghy-dokumentasjonen og fra målingene Star Distroyer og SkurkyMello har gjort i
fm1-emu. Ingen av FM-1-appene er målt på et ekte FM-1 ennå. Derfor er første steg i porten å måle lydmotoren
(se [ROADMAP.md](../ROADMAP.md), fase 3).

## Hva FM-1 har (Dinghy 0.5)

| | |
|---|---|
| Kode og konstanter (flash, kjøres direkte) | 568 KB. `const`-tabeller koster ikke RAM. |
| RAM (`.data` + `.bss`) | 96 KB |
| Pool (store buffere) | 336 KB |
| Stack | 24 KB |
| Lagring | settings 4 KB, 8 blobs på 4 KB, 8 slots på 16 KB |
| Lyd | 256 frames hvert 5,8 ms. Regelen er å holde seg under halvparten (2,9 ms). |
| Kjerner | Én for appen. Dinghy gir ikke appen den andre, og noen enheter har rapportert problemer med den. |
| Regnekraft | I fm1-emu er brikken omtrent 240 millioner instruksjoner i sekundet: 8,7 M/s var 3,6 % i Star Distroyer. Det gir omtrent 5400 instruksjoner per lydframe. |

## Minne

| Hva | Hvor | Størrelse |
|---|---|---|
| Koden: spill, grafikk, lyd, MIDI, LINK | flash | omtrent 150 KB (Star Distroyer bruker rundt 111 KB) |
| Terminus-fontene (16 og 32 px) | flash | 33 KB |
| Bølgetabeller og kurver (tanh, bitcrush) | flash | omtrent 10 KB |
| Innebygde låter, 5 byte per tone | flash | FIRST LIGHT 2 KB, NEON CARRIER 5 KB, ENGINE TOUR 10 KB, CHROME HORIZON 7 KB. **30 låter er omtrent 180 KB.** |
| **Sum flash** | | **omtrent 375 KB av 568** |
| Spilltilstand, stemmer, MIDI MODE-buffer (en takt), LINK | RAM | omtrent 15 KB av 96 |
| Skjermlerret | pool | 59 KB (240 × 124, to striper) eller 113 KB (hele skjermen) |
| Tempo-delay, punktert åttedel ned til 80 BPM, mono | pool | omtrent 50 KB |
| Romklang (Schroeder/FDN, ikke konvolusjon) | pool | omtrent 32 KB |
| Gated reverb og chorus | pool | omtrent 24 KB |
| **Sum pool** | | **omtrent 165–220 KB av 336** |
| Poeng og medaljer: 5 byte per låt og nivå, pluss låtens hash | settings eller én blob | under 1 KB for 40 låter med 4 nivåer |
| Egne låter | slots | 8 × 16 KB. Det er omtrent 3000 toner per slot, nok til ENGINE TOUR (2023 toner). |

Spillsporet til én låt (MED, omtrent 200 toner) tar omtrent 2,5 KB RAM mens den spilles. Bandet leses rett fra flash.

## Lyd (CPU)

Det tyngste stedet er siste refreng i CHROME HORIZON eller finalen i ENGINE TOUR: alle delene samtidig, på de skitne
patchene. En oscillator koster 37–79 instruksjoner per frame i Star Distroyers kompilerte kode. Her regnes 40 for en
sagtann med PolyBLEP og 20 for et filter.

| Del | På det tyngste | Instruksjoner per frame |
|---|---|---|
| PAD | 3 toner × 4 sagtann, filter | omtrent 500 |
| BASS | 2 toner som overlapper, 3 oscillatorer, filter, forvrengning | omtrent 240 |
| ARP | 2 toner × 2 oscillatorer, filter | omtrent 180 |
| LEAD | 3 oscillatorer, filter, forvrengning | omtrent 140 |
| KEYS | 3 toner × 3 FM-operatorer | omtrent 350 |
| Spilleren | 3 toner × 3 oscillatorer, filter | omtrent 380 |
| Trommer | omtrent 6 lag samtidig (støy, tone, filter) | omtrent 270 |
| Effekter | chorus, delay, romklang, gated reverb, liming, kompressor, pumping | omtrent 300 |
| **Sum** | | **omtrent 2360, det vil si 43 % av brikken** |

Det er under halv-regelen, men for nær kanten når grafikken også skal ha sitt. Med disse grensene kommer det ned
mot 30 %, uten at det skal høres mye:

- **Stemmetak:** 24 oscillatorstemmer totalt, der den eldste stjeles når det er fullt, og 6 trommelag for seg.
- **PAD med 3 sagtann i stedet for 4.** Chorusen gir bredden.
- **KEYS som 2-operator FM.**
- **Kontrollrate:** konvolutter, LFO-er og filterkoeffisienter regnes hver 32. frame, slik Dinghys regler sier.
- **Romklang som liten Schroeder- eller FDN-reverb.** Konvolusjonen i referansen (2,8 s) går ikke på FM-1.
- **De varmeste løkkene i RAM-vinduet på 8 KB,** hvis cachemiss fra flash viser seg i målingene.

Grit (D7) er billig: forvrengning og bitcrush er et tabelloppslag, og pumping er én multiplikasjon per frame.

## Grafikk

- En hel skjerm er 240 × 240 × 2 byte = 115 KB over SPI. DMA sender 240 × 64 piksler på omtrent 3 ms, så hele
  skjermen tar omtrent 11 ms. I mellomtiden kan prosessoren gjøre annet (`LCD_IDLE`).
- Banen er rader med spenn (omtrent 150 rader × 17 spenn), pluss 20–40 kapsler og tekst. Det er omtrent 150 000–300 000
  instruksjoner per bilde. **Ved 30 bilder i sekundet er det 2–4 % av brikken.**
- Bare banen (rad 40–222) må tegnes hvert bilde. Toppstripa og knotteraden tegnes bare når de endrer seg. Det er
  omtrent 85 KB per bilde, så 30 bilder i sekundet bruker omtrent en tredjedel av SPI-tiden, men lite CPU.
- Mål: 30 bilder i sekundet, 60 om det går.

## Låter og poeng

- **Innebygde låter:** `tools/mid2keyfall` gjør MIDI-filene om til kompakte tabeller når firmwaren bygges. De ligger i
  flash og koster ikke RAM. Transponering, oktavskift og nivåer regnes ut der, ikke på FM-1.
- **Egne låter på FM-1:** SysEx er plattformens og brukes til oppdateringer, så en fil kan ikke sendes rett inn.
  To veier, begge med bare noter (D3):
  1. bygg låta inn i firmwaren, eller
  2. la **MIDI MODE ta opp** låta fra DAW-en til en slot mens DAW-en spiller den. Opptaket varer like lenge som låta.
- **Poeng:** lagres én gang etter hver låt, når lyden har stilnet. En sletting av flash stopper lyden i omtrent 50 ms.
  En sektor tåler omtrent 100 000 slettinger, så med 20 låter om dagen varer den i over 13 år.

## Det som må måles først

1. **Lydmotoren i fm1-emu,** med finalen i ENGINE TOUR som verste tilfelle, før noe av grafikken bygges.
2. **Ekte maskinvare.** fm1-emu teller instruksjoner, ikke sykluser, og cachemiss fra flash kan gjøre det tregere.
   Ingen av FM-1-appene er målt på et ekte FM-1 ennå.
3. **Én kjerne.** Alt planlegges for én kjerne. Den andre kjernen er en bonus hvis den viser seg å være stabil.

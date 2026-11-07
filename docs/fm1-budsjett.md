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
| Tittelskjermens bilde over horisonten (240 × 148, RGB565) | flash | 69 KB. Gulvet under horisonten tegnes levende. |
| Himmelen over spillbanen (240 × 20, RGB565, D10) | flash | 9,6 KB |
| Bølgetabeller og kurver (tanh, bitcrush) | flash | omtrent 10 KB |
| Innebygde låter, 5 byte per tone | flash | FIRST LIGHT 2 KB, NEON CARRIER 5 KB, ENGINE TOUR 10 KB, CHROME HORIZON 7 KB. **30 låter er omtrent 180 KB.** |
| **Sum flash** | | **omtrent 385 KB av 568** |
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

### Målt (firmware 0.1 under arbeid, 2026-11-07)

Lydmotoren finnes nå i fastkomma (`firmware/src/kf_engine.h`) og er målt i fm1-emu i CI (`tools/emu_cpu.sh`):

| Hva | Målt |
|---|---|
| Plattformen alene, stille (USB, tangenter, lydavbruddet uten lyd) | 2,8 % |
| Et syntetisk verste tilfelle, alle delene på skitne patcher (første DEMO) | 43,9 %, 2390 instruksjoner per frame |
| x86 mot brikken, samme kode | brikken teller omtrent 0,8 ganger x86 (callgrind) |

Sangene er tyngre enn budsjettet over antok, fordi pad-ene har lange utklingninger (SUPERSAW har τ = 1 s). I
ENGINE TOURs tyngste takter (41–52) var 24 stemmer i bruk, med omtrent 76 oscillatorer per frame. Derfor har
motoren et **oscillatorbudsjett** på 48 (D8): er det sprengt, toner de stilleste utklingningene ut på 25 ms. En
utklinging som står alene, for eksempel på slutten av en låt, får klinge helt ut.

| Hva | Målt i fm1-emu |
|---|---|
| ENGINE TOUR takt 41–44, lydmotoren alene (bandet og melodien) | **31,6 %**, 1717 instruksjoner per frame |
| Det samme som DEMO i spillet (spillerens stemme med comboens lysstyrke) | **34,0 %**, 1853 instruksjoner per frame; **30,8 %**, 1673, etter optimaliseringen under |
| Det samme, alle avbrudd | 36,9 %; 33,6 % etter |

x86-anslaget (callgrind × 0,8) sa 31,5 %, så anslaget kan brukes mellom CI-kjøringene.

**Blokk for blokk og bilde for bilde** (`tools/emu_timing.py`, i CI etter `emu_cpu.sh`). Et snitt viser ikke den ene
lydblokka som tar for lang tid og gir et klikk, eller det ene bildet som kommer for sent. Verktøyet setter
stoppunkter på kallet inn i lydavbruddet, på `draw()` og på `song_start`, og regner ut hver lydblokk og hvert bilde:

| Hva (fm1-emu, 2026-11-07) | DEMO-en, 60 bilder/s | MIDI MODE, flom fra en DAW |
|---|---|---|
| Lydblokkene (256 frames, 5,8 ms hver), median / verste | 32 % / 42 % av tida (før: 36 / 46) | 44 % / 48 % (før: 48 / 54) |
| `draw()` med avbruddene i seg, median / verste | 3,1 ms / 5,5 ms (før: 6,6 / 9,3) | 2,3 ms / 4,9 ms (30 bilder/s; før: 6,3 / 6,6) |
| Lengst mellom to bilder | 19,2 ms | 35,0 ms |

Flommen: hvert 100. ms slipper kanal 1–6 en akkord på 6 toner og spiller en ny, og kanal 10 slår fire trommer.
Oscillatorbudsjettet holder den tyngste blokka på 54 %. CI feiler hvis en blokk bruker over 75 % av tida, eller hvis
to bilder ligger lenger fra hverandre enn 25 ms i spillet eller 45 ms i MIDI MODE (en låts start er unntatt).

Første måling fant et hull på 30–40 ms hvert sekund. `usb_retry` ventet 25 ms i en løkke når ingen USB-vert svarte,
for eksempel med en powerbank eller en lader. Hvert sekund hoppet skjermen over tre bilder, og tangenter som ble
trykket i mellomtiden, fikk tid som var opptil 30 ms for sen. Nå venter `usb_retry` uten å blokkere (PLATFORM.md), og
tangentene får tid fra skanningen (`FM1_NOTE_EDGE`), ikke fra når hovedløkka leser dem.

**Hvor tida går, på brikkens egne instruksjoner.** fm1-emu teller hver instruksjonsadresse. En lokal kopi som skriver
ut hele histogrammet, sammen med en `-g`-bygging (`DINGHY_DEFS=-g ./build.sh` og `objdump -d -l`), gir en profil per
funksjon og per kildelinje. Kjøringen er den samme som i `emu_cpu.sh`: 9,6 s, med FX etter 1,6 s. Tallene er andeler
av hele brikken:

| Hva | Før | Etter |
|---|---|---|
| Lydavbruddet med motoren | 28,7 % | 25,9 % |
| Tekst (`ktext_n`) | 10,1 % | 4,8 % |
| Tømming av lerretet hvert bilde (`cv_begin`, nå `kfill_rows` for tangentradene) | 2,6 % | 0,5 % |
| Blandede rektangler, stråler og glimt (`krect_a`) | 3,0 % | 2,5 % |

Hovedløkka går i ring når den har ledig tid (`fm1_led_key` og `fm1_led_key_glow`, 39 %), og den tida koster ingenting.

Det som er gjort, med samme piksler og samme lyd (bit for bit, sjekket mot forrige commit):
- **Tekst:** en glyf som ligger helt på lerretet, tegnes uten sjekk per piksel. Fargen deles i kanaler én gang per
  glyf, ikke per piksel, og alfa kommer fra en tabell.
- **Lerretet:** spillskjermen tegner hver rad selv, så `draw()` tømmer det ikke først. Bare tangentradene fylles,
  to piksler per skriving.
- **Oscillatorene:** PolyBLEP-testen sjekket hvert sample om bølgen var nær hoppet sitt. Nå sjekkes en tick (32
  frames) én gang. Er ingen oscillator nær hoppet, går en løkke uten testen. En firkant uten hopp har samme nivå
  hele ticken.
- **Filteret og FM:** drive og crush er bestemt av patchen, ikke av samplet. Løkkene er delt i én per variant, og
  drive-tabellen slås opp én gang.

Motoren gikk fra 34,1 % til 30,8 % av brikken, og et bilde i DEMO-en fra 6,6 til 3,1 ms (median, med avbruddene).
Rendrene er byte for byte like (motorturen, verste tilfelle, 60 s av hver låt), og alle 23 skjermbildene fra
testskriptene er piksel for piksel like.

Lyden er sjekket mot referansen: alle 25 patcher og 4 trommesett ligger innenfor 1,5 dB og 25 % lysstyrke, de
fleste innenfor 0,3 dB. Hver fjerde takt av de første 30 sekundene av alle fire låtene ligger innenfor 0,8 dB
(`tests/run_vavehero.sh`).

## Grafikk

- En hel skjerm er 240 × 240 × 2 byte = 115 KB over SPI. DMA sender 240 × 64 piksler på omtrent 3 ms, så hele
  skjermen tar omtrent 11 ms. I mellomtiden kan prosessoren gjøre annet (`LCD_IDLE`).
- Banen er rader med spenn (omtrent 150 rader × 17 spenn), pluss 20–40 kapsler og tekst. Det er omtrent 150 000–300 000
  instruksjoner per bilde. **Ved 30 bilder i sekundet er det 2–4 % av brikken.**
- Bare banen (rad 40–222) må tegnes hvert bilde. Toppstripa og knotteraden tegnes bare når de endrer seg. Det er
  omtrent 85 KB per bilde, så 30 bilder i sekundet bruker omtrent en tredjedel av SPI-tiden, men lite CPU.
- Mål: 30 bilder i sekundet, 60 om det går.
- **Beregnet (firmware 0.1):** spillskjermen i ENGINE TOURs tyngste takter koster omtrent 0,69 millioner
  instruksjoner per bilde på brikken (callgrind × 0,8), det vil si omtrent 9 % av brikken ved 30 bilder i sekundet.
  Fargene på banen blandes én gang ved oppstart, og tekst blandes i RGB565. Hele skjermen er ett lerret på 113 KB.
  Med D10 (gulvet, rutenettet og himmelen) koster samme takter omtrent 7 % mindre, fordi tekst hopper over tomme
  glyfbyte.

## Låter og poeng

- **Innebygde låter:** `tools/mid2vavehero` gjør MIDI-filene om til kompakte tabeller når firmwaren bygges. De ligger i
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

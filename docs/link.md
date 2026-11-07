# LINK: to spillere på to FM-1-er

To spillere har hvert sitt FM-1 og spiller samme låt samtidig. Alt i spillet er bygget for dette fra start:
notekartet er deterministisk og hver enhet dømmer sine egne trykk mot sin egen klokke. Derfor er det bare start,
klokke og poeng som må over kabelen.

## Lagene: fra nettleser til USB-C

| Lag | Kobling | Status |
|---|---|---|
| **L0** | Protokollen: MIDI-kanalmeldinger på kanal 16, pluss klokke, start og stopp (under) | Versjon 1, i referansen |
| **L1** | To vinduer i samme nettleser (BroadcastChannel), og en bot som snakker samme protokoll | Ferdig i referanse 0.1 |
| **L2** | To FM-1-er via en USB-vert: en datamaskin eller et nettbrett med en relé-side (WebMIDI), eller en USB-MIDI-vertsboks | Mulig med Dinghy slik den er. Trenger firmware-porten. |
| **L3** | To FM-1-er med én USB-C-kabel direkte mellom seg | Krever USB-vertsmodus i firmware. Avklares i en forstudie. |

Meldingene er de samme på alle lag. Det som virker i L1, virker i L2 og L3 uten endringer.

### Hvorfor ikke bare én USB-C-kabel ennå

USB har alltid én vert og én enhet. FM-1 er en USB-enhet (class compliant USB-MIDI), og Dinghys `usb.c` kjører i
enhetsmodus. Kobles to FM-1-er sammen med en C-til-C-kabel, venter begge på en vert, og ingenting skjer. SLOOP-guiden
sier det samme om USB-keyboard rett i FM-1. TRS-kontakten er bare MIDI inn, så den kan heller ikke koble to FM-1-er
alene.

Forstudien for L3 må svare på dette:

1. **Vertsmodus.** Kan USB-kontrolleren i JieLi AC791N kjøre som vert (OTG)? Jeg har ikke funnet databladet for
   akkurat AC791N, så dette er ikke avklart.
2. **Strøm.** En vert må gi 5 V på VBUS. Kan FM-1 det fra batteriet, eller trengs en OTG-adapter med eget
   strøminntak?
3. **Vertsdriver.** Firmware trenger en liten USB-MIDI-vertsdriver (enumerering og bulk-endepunkter). Det er en
   betydelig jobb, men avgrenset: den trenger bare å kjenne én enhet, et annet FM-1.
4. **Roller.** Hvem er vert? Mest naturlig er at P1 er vert og P2 er enhet, valgt i menyen før kampen.

Svarer forstudien nei, blir L2 standardkoblingen. Den virker uansett. Noen FM-1-versjoner har også Bluetooth-MIDI,
som kan bli et senere lag (L4), men Dinghy har ikke Bluetooth.

## Protokoll, versjon 1

En Dinghy-app mottar bare kanalmeldinger, og av systemmeldingene bare klokke, start, fortsett og stopp (`0xF8`,
`0xFA`, `0xFB`, `0xFC`). SysEx tilhører plattformens oppdaterer, og Song Position Pointer (`0xF2`) kastes. Derfor
bruker protokollen kontrollmeldinger (CC) på kanal 16, status `0xBF`, og posisjonen er START pluss antall
klokkepulser.

| Melding | Bytes | Fra | Betydning |
|---|---|---|---|
| HELLO | `BF 64 v` (CC 100) | Begge, hvert sekund | Protokollversjon, nå 1 |
| ID | `BF 65 id` (CC 101) | Begge, hvert sekund | Tilfeldig 1–126. Lavest blir P1 (boten er 127). Ved likhet trekker mottakeren nytt. |
| SONG | `BF 66 n` (CC 102) | P1 | Låt |
| LEVEL | `BF 67 n` (CC 103) | P1 | 0 EASY, 1 MED, 2 HARD, 3 EXP |
| SPEED | `BF 68 %` (CC 104) | P1 | Tempo i prosent, 50–100 |
| START | `FA` | P1 | Begge starter låta nå. P2 bruker siste SONG, LEVEL og SPEED. |
| CLOCK | `F8` | P1 | 24 pulser per beat, fra START |
| STOP | `FC` | P1 | Kampen avbrytes, begge går til SONGS |
| SCORE | `BF 6E hi` `BF 6F mid` `BF 70 lo` (CC 110–112) | Begge | Poeng i tre 7-bits deler, opptil 2 097 151. Tas i bruk når `lo` kommer. |
| COMBO | `BF 71 hi` `BF 72 lo` (CC 113–114) | Begge | Combo. Tas i bruk når `lo` kommer. |
| JUDGE | `BF 73 j` (CC 115) | Begge | 1 PERFECT, 2 GREAT, 3 GOOD, 4 MISS |
| BOOST | `BF 74 0/1` (CC 116) | Begge | OP BOOST av eller på |
| OPS | `BF 75 n` (CC 117) | Begge | Operatorer tent, 0–6 |
| DONE | `BF 76 %` (CC 118) | Begge | Ferdig, med treffprosent |
| BYE | `BF 77 0` (CC 119) | Begge | Kobler fra |

Noter på kanal 16 (`9F` og `8F`) er satt av til senere, når hver spiller kan høre den andres stemme i band-modus.

**Regler**

- SCORE og COMBO sendes bare når de endrer seg, og høyst 15 ganger i sekundet.
- P2 starter på START og teller klokka. I firmware skal P2 justere tempoet litt når egen posisjon avviker fra
  pulstellingen med mer enn 10 ms. Referansen teller foreløpig bare pulsene. Hver spiller hører sin egen enhet, så
  bare startforskjellen merkes.
- Hver enhet dømmer sine egne trykk lokalt. Forsinkelse i kabelen påvirker aldri PERFECT eller GREAT, bare hvor fort
  den andres poeng vises.
- En linje som har vært stille i 3,5 sekunder regnes som frakoblet.

## Moduser

| Modus | Hva | Status |
|---|---|---|
| **VS** | Samme låt samtidig. Motstanderens poeng står i toppstripa, og vinneren vises på resultatskjermen. | Ferdig i referanse 0.1 (to vinduer og bot) |
| **Annenhver** | Spillerne tar hver sin frase. Lengst combo vinner runden. | Planlagt |
| **Band** | Samarbeid: P1 spiller melodien, P2 bass eller akkorder fra eget notekart. Notene på kanal 16 lar begge høre helheten. | Planlagt |
| **Battle** | OP BOOST sender en ulempe til motstanderen, for eksempel at notene først blir synlige nær trefflinja i to takter. | Idé |

En enklere variant med to spillere på ett FM-1, hver med sin ende av klaviaturet som i RoboSeqs battle, kan komme i
tillegg. Hovedsporet er to FM-1-er.

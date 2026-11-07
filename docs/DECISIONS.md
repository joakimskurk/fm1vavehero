# Beslutninger

En beslutning står her med dato og grunn, slik at koden og referansen kan sjekkes mot den. Den overstyrer konseptet
der de er uenige.

## D1 (2026-11-07): bare de hvite tangentene spilles

**Beslutning.** Notekartene har bare toner på hvite tangenter, og banene på skjermen er én per hvit tangent. De
svarte tangentene vises fortsatt på skjermen der de sitter, i den øvre raden av tangentstripa, men de har ingen
funksjon. Under en låt gir de ingen tone, ingen poeng, ingen straff og ingen lyd.

*Justert samme dag (REF 0.1.2).* En kort stund viste stripa bare de hvite tangentene. Den svarte raden er tilbake
fordi den gjør det lettere å se hvor man er på klaviaturet. Beslutningen gjaldt funksjonen, ikke visningen.

**Hvorfor.**

- Skjermen er bygget for det. Åtte baner, én per hvit tangent, er lette å lese på 240 piksler. Svarte toner på
  skillelinjene var den mest uryddige delen av banen.
- På FM-1 sitter de svarte tangentene i en egen øvre rad, ikke mellom de hvite som på et piano. Å treffe dem i fart
  er noe annet enn å spille piano.
- Med én funksjon per tangent, å spille, er det ingenting å huske. Stripa viser likevel de svarte, slik at
  skjermen ser ut som klaviaturet.

**Hva det koster.** Toner utenfor skalaen forsvinner. Det tas igjen med transponering per låt, slik at de hvite
tangentene følger låtas toneart og modus. Tre toner i NEON CARRIER ble skrevet om: D#4 ble D4, G#4 ble A4 og F#5 ble D5.

**Sjekkes av.** `buildChart` stopper med en feil hvis et notekart har en svart tangent. `tests/reference.cjs` sjekker
at ingen notekart har svarte tangenter, og at en svart tangent midt i en låt ikke endrer combo eller dom.

## D2 (2026-11-07): SPREAD, annenhver hvit tangent

**Beslutning.** En innstilling, SPREAD, legger de åtte banene på annenhver hvit tangent: F3 A3 C4 E4 G4 B4 D5 F5,
med én ubrukt hvit tangent mellom hver. Banene og musikken er de samme. Bare hvilken fysisk tangent som spiller hver
bane endres. Tangentene mellom har ingen funksjon, som de svarte (D1). SPREAD slås av og på med GLO (`Q` i
referansen), lagres, og kan ikke endres midt i en låt.

**Hvorfor.** Silikontangentene på FM-1 er små. Med store fingre er det lett å treffe nabotangenten. Med én tangent
mellomrom blir avstanden mellom banene omtrent dobbelt så stor. Det fyller nesten hele klaviaturet: 15 av 16 hvite
tangenter.

**Slik ser det ut.** Tangentstripa på skjermen viser de brukte tangentene under banene og en mørk, ubrukt tangent
mellom hver. LED-ene og bokstavhintene i panelet følger de brukte tangentene. SPREAD gjelder bare den som spiller,
så i en kamp kan den ene spilleren ha SPREAD på og den andre ikke.

**Sjekkes av.** `tests/reference.cjs` spiller NEON CARRIER med SPREAD på autospill (100 %). Den sjekker også at
LED-ene bare lyser på SPREAD-tangentene, at en tangent mellom to brukte ikke gjør noe, og at F3 spiller første bane.

## D3 (2026-11-07): låtene er MIDI-filer, ett instrument per kanal

**Beslutning.** En låt er en standard MIDI-fil (format 0 eller 1). Kanal 1 er spillsporet. Kanal 2 BASS, 3 PAD,
4 ARP, 5 LEAD, 6 KEYS og 10 DRUMS (General MIDI) er bandet. Kanal 16 er satt av til LINK. Note 11 (B-1) på kanal 1,
holdt over frasen, markerer OP-fraser (endret fra CC 85 samme dag, se D6). Importen transponerer spillsporet til hvite tangenter (D1), legger inn oktavskift og lager EASY. De
innebygde låtene er laget på samme måte (`tools/songs.py` skriver `songs/*.mid`) og går gjennom samme import. Hele
reglen står i [songs.md](songs.md).

**Hvorfor.** Låter skal kunne lages helt i en DAW, med verktøy som allerede finnes. Én kanal per instrument er
vanlig MIDI-praksis, og den samme kanalfordelingen virker både for filer og for live-MIDI. Når DAW-en spiller inn i
referansen, blir den en lydmodul der du hører låta som i spillet.

**Sjekkes av.** `tests/reference.cjs`: de innebygde filene gir de samme notekartene og oktavskiftene som før. En
testlåt i E-moll (format 0, running status) transponeres, får oktavskift, og spilles 100 % på autospill. Rapporten
nevner den kromatiske tonen, den fjerde akkordtonen, kanal 7 og kanal 16. Live-MIDI går til riktig del per kanal.

## D4 (2026-11-07): lydmotoren er synthwave

**Beslutning.** Lydmotoren er laget for synthwave og retrowave: sagtann-pads med chorus, pulserende bass,
arpeggio med tempo-delay (punktert åttedel), lead med vibrato, FM-piano, gated reverb på skarptromme og klapp,
syndrum-tammer og mørk hall. Spillerens stemme kan være SYN LEAD (standard), BELL EP, BRASS eller GLASS.

**Nivåer.** Målt i refrenget til NEON CARRIER, hver del alene: spillerens stemme −20 til −22 dB RMS, trommer −21,
bass −24,5, pad og arpeggio −30. Hele miksen har en toppverdi på 0,75. Spillerens stemme skal ligge øverst eller
likt med trommene. Den sjekken gjentas når lyden endres.

**Hva det koster.** Mange oscillatorer per tone (fire per pad-tone). På FM-1 må motoren lages i fastkomma og måles i
fm1-emu. Hvis den er for tung, blir pad-stemmene og hallen enklere først.

## D5 (2026-11-07): MIDI MODE, DAW-en rett i FM-1 mens låta lages

**Beslutning.** FM-1 har en egen modus for å lage låter: MIDI MODE, nederst i låtlista. DAW-en kobles til FM-1 med
USB-C, og hvert spor sendes på sin kanal som i D3. Modusen har to varianter, og PLAY bytter mellom dem:

- **LIVE:** FM-1 er en lydmodul. Hver kanal spilles straks av sitt instrument. Skjermen viser aktivitet per kanal,
  tempoet fra MIDI-klokka og takt og slag. Toner på kanal 1 lyser opp tangent-LED-ene, og toner på svarte tangenter
  varsles i rødt (D1).
- **PREVIEW:** alt forsinkes én takt, like langt som banen ser framover. Kanal 1 faller ned banen slik den blir i
  spillet, med oktavskift. Bandet spilles i takt med banen, og du kan spille med og bli dømt. Svarte toner vises som
  røde merker og telles ikke.

Tangentene på FM-1 sendes ut på kanal 1 i begge variantene, så DAW-en kan ta opp spillsporet. Uten MIDI-klokke
brukes tempoet fra KNOB 3 (BPM). MIDI MODE og LINK utelukker hverandre, fordi begge bruker MIDI-klokka og
START/STOP.

**Hvorfor.** Det er raskest å lage en spillbar låt når man hører den på FM-1 og ser spillsporet på banen mens man
skriver. PREVIEW svarer på spørsmålet «kan dette spilles?» uten å eksportere fila.

**Hva det koster.** Dinghy slipper ikke gjennom Song Position Pointer. Starter DAW-en midt i låta, teller FM-1 takt
fra der den startet, ikke fra låtas takt. PREVIEW henger én takt etter DAW-en, så DAW-ens egen lyd må være av, eller
bare spille MIDI.

**Sjekkes av.** `tests/reference.cjs`, med testene beskrevet i [reference/README.md](../reference/README.md).

## D6 (2026-11-07): flere patcher per instrument, valgt med nøkkelnoter

**Beslutning.** Hvert instrument har flere patcher: PLAY 4, BASS, PAD, ARP, LEAD, KEYS 3 hver, og trommene 3 sett.
Note 0–11 (C-1 til B-1) på instrumentets kanal velger patch, med note 0 som patch 1. Den høres aldri, og gjelder til
neste nøkkelnote. På kanal 1 velger den spillerens stemme når TONE står på SONG, som
er standard. Det virker likt i MIDI-filer, i live-MIDI og i MIDI MODE. Tabellen står i [songs.md](songs.md).

**Hvorfor.** Én lyd per instrument blir fort ensformig gjennom en hel låt. Nøkkelnoter er slik sample-bibliotek
bytter artikulasjon, så de er kjente fra DAW-en. Note 0–11 brukes ikke til musikk, så de kolliderer ikke med noe.
**Bare noter (justert samme dag).** Program Change og CC brukes ikke, verken til patcher eller til OP-fraser
(som nå er note 11 på kanal 1). Alt i en låt kan da tegnes i pianorullen, og det er det minst krevende å lage.

**Nivåer.** Hver patch er målt alene (fire toner, eller et trommemønster). Innenfor samme instrument ligger patchene
innenfor ±2 dB av hverandre, så et bytte ikke blir et volumhopp. Refrenget i NEON CARRIER, med FM BASS og
BELL-arpeggio, har samme balanse som i D4.

**Sjekkes av.** `tests/reference.cjs`, med testene beskrevet i [reference/README.md](../reference/README.md).

## D7 (2026-11-07): skitnere lyd, med GRIT

**Beslutning.** Lydmotoren skal kunne låte skitten og fransk (Kavinsky, Justice), ikke bare blank:

- **GRIT** (0–100, standard 70) styrer tre ting på hele miksen: pumping, der bandet dukker på hvert kick
  (sidekjede), metning og liming av hele miksen, og crunch på trommebussen. På 0 er lyden ren som før.
- **En skitten patch per instrument,** med forvrengning etter filteret, og noen med en knust kopi (bitcrush):
  FUZZ (spillerstemme), DIRT BASS, GRIT PAD, CRUSH (arpeggio), FUZZ LEAD og GRIT EP. Trommesettet heter DIRT. De
  ligger på neste ledige nøkkelnote, som er note 4 på kanal 1 og note 3 på de andre.
- **Tørrere miks:** hallen er senket fra 0,55 til 0,4.
- Spillerens stemme pumper ikke, så den alltid ligger fritt over bandet.

**Nivåer.** Hver skitne patch ligger innenfor ±2 dB av sine søsken. Unntaket er DIRT-settet, som ligger innenfor
spennet til de andre trommesettene. I det skitne siste refrenget i CHROME HORIZON ligger spilleren på −18,5 dB og
trommene på −18,7 dB, bassen 3,3 dB under, og keys, arpeggio og pad lenger bak. Med GRIT 70 blir miksen omtrent 2 dB
tettere, med lavere toppnivå, og ingen låt klipper på 0, 70 eller 100.

**For FM-1.** Forvrengning, bitcrush og pumping er billige i fastkomma, mye billigere enn stor romklang. Det skitne
er det enkleste å flytte over.

**Sjekkes av.** `tests/reference.cjs`, med testene beskrevet i [reference/README.md](../reference/README.md).

## D8 (2026-11-07): firmwarens lyd, der den avviker fra referansen

**Beslutning.** Lydmotoren i firmwaren (`firmware/src/kf_engine.h`) skal låte som referansen, og den sjekkes mot
referansens egne renders. Den avviker fra referansen bare på disse punktene, fordi FM-1 ikke har råd til resten:

- **Oscillatorbudsjett på 48.** Når stemmene trenger flere oscillatorer enn det, toner de stilleste utklingningene
  ut på 25 ms. En utklinging som står alene, for eksempel på slutten av en låt, får klinge helt ut. Utklingninger
  slutter ellers 40 dB under toppen sin, mens referansen lar dem gå i seks tidskonstanter.
- **Romklangen er en FDN** med fire linjer bak fire diffusorer, og ikke en konvolusjon på 2,8 s. Energien er
  kalibrert mot referansen. Den gatede romklangen er en FDN bak en port som skarptromma og klappen åpner.
- **FM-stemmer med hele forhold** (ORGAN, FM EP, TINE, FM BASS, BELL EP, GRIT EP, GLASS PAD) starter modulatorene på
  en fast fase som gir referansens gjennomsnittsnivå. I referansen følger fasen tonens startøyeblikk, og nivået varierer
  inntil 4 dB fra tone til tone.
- **Kompressoren** følger Chromiums kurve (målt med `tools/ref_tour.cjs --comp`), men uten 6 ms forsinkelse foran.

**Hvorfor.** Det tyngste stedet i låtene (takt 41–44 i ENGINE TOUR) kostet 76 oscillatorer per frame uten budsjett,
fordi pad-ene har lange utklingninger. Med budsjettet bruker lyden 31,6 % av brikken, og 34,0 % med spillet rundt
(målt i fm1-emu). En
konvolusjon på 2,8 s er langt over det brikken kan.

**Sjekkes av.** `tests/run_vavehero.sh`: alle 25 patcher og 4 trommesett innenfor 1,5 dB og 25 % lysstyrke av
referansen, de fleste innenfor 0,3 dB, og hver fjerde takt av de fire låtenes første 30 sekunder innenfor 0,8 dB.
`tools/emu_cpu.sh` i CI feiler hvis det tyngste stedet bruker mer enn 45 % av brikken.

## D9 (2026-11-07): spillet heter VaveHero

**Beslutning.** Spillet heter VaveHero, etter M-VAVE og «hero» fra sjangeren. Arbeidstittelen KEYFALL er borte fra
filnavn, skjermer, firmware og dokumenter. På skjermen står det VAVEHERO, med VAVE i hvitt og HERO i cyan, fordi
den store fonten bare har store bokstaver. Firmwaren heter `vavehero.fwsc`, appen `firmware/src/vavehero.c`,
referansen `reference/vavehero.html`, og pakkens identitet er fortsatt `FM-1_95N`.

**Hva det koster.** Innstillinger lagret i flash av en tidligere bygging starter på nytt, fordi lagringen har fått
eget merke (`STORE_MAGIC` "VHRO"). Det samme gjelder det referansen har lagret i nettleseren, også egne låter, fordi
nøklene nå heter `vavehero.*`. Interne navn i koden (`kf_`, `kg_`, `ks_` og `window.KF`) er beholdt, slik at koden ikke
endres bare for navnets skyld.

**Å sjekke før utgivelse.** «Hero» i Guitar Hero er Activisions varemerke, som konseptet allerede nevnte. VaveHero er
et eget navn, men det bør vurderes før spillet legges ut på Firmware Hub.


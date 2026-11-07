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
4 ARP, 5 LEAD, 6 KEYS og 10 DRUMS (General MIDI) er bandet. Kanal 16 er satt av til LINK. CC 85 på kanal 1 markerer
OP-fraser. Importen transponerer spillsporet til hvite tangenter (D1), legger inn oktavskift og lager EASY. De
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

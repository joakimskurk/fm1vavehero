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

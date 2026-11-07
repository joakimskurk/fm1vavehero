# Beslutninger

En beslutning står her med dato og grunn, slik at koden og referansen kan sjekkes mot den. Den overstyrer konseptet
der de er uenige.

## D1 (2026-11-07): bare de hvite tangentene spilles

**Beslutning.** Notekartene har bare toner på hvite tangenter. Banene på skjermen er én per hvit tangent, og
tangentstripa viser bare de hvite. Under en låt er de svarte tangentene utenfor spillet: ingen tone, ingen poeng,
ingen straff og ingen lyd.

**Hvorfor.**

- Skjermen er bygget for det. Åtte baner, én per hvit tangent, er lette å lese på 240 piksler. Svarte toner på
  skillelinjene var den mest uryddige delen av banen.
- På FM-1 sitter de svarte tangentene i en egen øvre rad, ikke mellom de hvite som på et piano. Å treffe dem i fart
  er noe annet enn å spille piano.
- De andre FM-1-appene bruker de svarte tangentene til funksjoner (styring i RoboSeq, RNDMZR og UNDO i SkurkyMello,
  OP1–OP6 i M-VAVEs egen firmware). Slik blir de ledige til det i Keyfall også.

**Hva det koster.** Toner utenfor skalaen forsvinner. Det tas igjen med transponering per låt, slik at de hvite
tangentene følger låtas toneart og modus. Tre toner i NEON CARRIER ble skrevet om: D#4 ble D4, G#4 ble A4 og F#5 ble D5.

**Sjekkes av.** `buildChart` stopper med en feil hvis et notekart har en svart tangent. `tests/reference.cjs` sjekker
at ingen notekart har svarte tangenter, og at en svart tangent midt i en låt ikke endrer combo eller dom.

# Referansen

`vavehero.html` er VaveHero i nettleseren: én fil, uten nettverk og uten byggesteg. Til Dinghy-porten finnes, er den
fasit for hvordan VaveHero føles, times og høres. Når C-koden og HTML-en er uenige, vinner HTML-en, slik som i
RoboSeq og SkurkyMello.

| Fil | Versjon | SHA-256 |
|---|---|---|
| `vavehero.html` | REF 0.1.10 (tittelskjermen som 80-tallsomslag, kromlogoen, DEMO på FX) | `7d0c1bca076368af10ccc2642e5bc38bb0efa3e51170d6aa0a133bd8579120e2` |

En ny HTML er en ny referanse og får en egen commit med ny SHA-256 her.

## Kjør

Åpne `vavehero.html` i en nettleser. Chrome og Edge har WebMIDI, de andre spiller med tastaturet og panelet. Trykk
Enter (PLAY) på tittelskjermen, eller mellomrom (FX) for DEMO-en, velg låt med pil opp og ned, og nivå med pil venstre og høyre.

To spillere på én maskin: åpne fila i to vinduer, velg **To vinduer** i begge, og gi hvert vindu sitt eget
MIDI-keyboard. P1 velger låt og starter.

## Test

```sh
node tests/reference.cjs          # krever Playwright med Chromium; skjermbilder i build/test/
```

Testen ser at tittelskjermen viser bildet sitt, og at FX starter DEMO-en (ENGINE TOUR fra takt 41) og PLAY
avslutter den. Den spiller begge låtene på autospill og krever 100 % uten bom, begge oktavskiftene og OP BOOST. Den sjekker
at ingen notekart har svarte tangenter, og at en svart tangent under spill blir ignorert (D1). Med SPREAD (D2)
sjekker den 100 % på autospill, at LED-ene bare lyser på SPREAD-tangentene, og at en tangent mellom to brukte ikke
gjør noe. Den sjekker
også at et trykk 80 ms for sent gir GOOD, at feil tangent bryter comboen, kampen mot boten, og to vinduer over
LINK (roller, start, klokke, poeng og stopp). De to vinduene starter med samme ID med vilje, og må trekke på nytt
for å koble opp.

Låtene (D3): de innebygde MIDI-filene gir samme notekart og oktavskift som før. En testlåt i E-moll (format 0,
running status) blir lastet inn via filvelgeren, transponert, får oktavskift og spilles 100 %. Rapporten nevner det
som er tatt ut eller ignorert. Live-MIDI går til riktig del per kanal, og malen lastes ned identisk med
`songs/neon-carrier.mid`. Lyden (D4): begge låtene rendres offline i 20 sekunder uten klipping, og WAV-filene havner
i `build/test/`.

MIDI MODE (D5): den åpnes fra siste rad i låtlista, og en MIDI-klokke på 120 BPM gir 120 BPM og riktig
pulstelling. I LIVE lyser kanal 1, 2 og 10, og E4 fra DAW-en tenner E4-LED-en. Tangenten C4 sendes ut på kanal 1. I
PREVIEW havner kanal 1 én takt foran, med oktavskift og holdtone. C#4 vises som varsel, og autospill treffer begge
tonene og skiftet. HOME går ut, og MIDI MODE holdes stengt mens LINK er på.

Lydprøvene: ENGINE TOUR og CHROME HORIZON importeres uten advarsler. ENGINE TOUR bruker alle 29 patcher og
trommesett, og CHROME HORIZON (D-moll) flyttes 5 halvtoner ned til hvite tangenter. Begge spilles 100 % på
autospill, i to vinduer samtidig, og rendres uten klipping.

GRIT (D7): ved GRIT 70 dukker bandet til omtrent halve nivået rett etter kicket, sammenlignet med GRIT 0.
CHROME HORIZON klipper verken på GRIT 0 eller 100, og innstillingen når motoren og sidepanelet.

Patcher (D6): de innebygde låtene bytter patch der nøkkelnotene sier det. Alle 22 patcher og trommesett høres alene,
og ingen klipper. Testlåtas nøkkelnoter havner i rapporten, og den uten patch flagges. Live flytter nøkkelnoter kanalens
patch, mens Program Change, OP-noten og en nøkkelnote uten patch ikke endrer noe. OP-fraser kommer fra note 11
på kanal 1, ikke CC.

## Låtene

`window.VAVEHERO_SONGS` i HTML-en er de innebygde MIDI-filene i base64. `python3 tools/songs.py` lager `songs/*.mid`
og oppdaterer blokka. HTML-en får da ny SHA-256.

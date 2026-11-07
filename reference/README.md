# Referansen

`keyfall.html` er Keyfall i nettleseren: én fil, uten nettverk og uten byggesteg. Til Dinghy-porten finnes, er den
fasit for hvordan Keyfall føles, times og høres. Når C-koden og HTML-en er uenige, vinner HTML-en, slik som i
RoboSeq og SkurkyMello.

| Fil | Versjon | SHA-256 |
|---|---|---|
| `keyfall.html` | REF 0.1.3 | `b8a414d6f99448ae4f053bcfd38879a6a1315e9c72742a249aba7380d050fa8a` |

En ny HTML er en ny referanse og får en egen commit med ny SHA-256 her.

## Kjør

Åpne `keyfall.html` i en nettleser. Chrome og Edge har WebMIDI, de andre spiller med tastaturet og panelet. Trykk
Enter (PLAY) på tittelskjermen, velg låt med pil opp og ned, og nivå med pil venstre og høyre.

To spillere på én maskin: åpne fila i to vinduer, velg **To vinduer** i begge, og gi hvert vindu sitt eget
MIDI-keyboard. P1 velger låt og starter.

## Test

```sh
node tests/reference.cjs          # krever Playwright med Chromium; skjermbilder i build/test/
```

Testen spiller begge låtene på autospill og krever 100 % uten bom, begge oktavskiftene og OP BOOST. Den sjekker
at ingen notekart har svarte tangenter, og at en svart tangent under spill blir ignorert (D1). Med SPREAD (D2)
sjekker den 100 % på autospill, at LED-ene bare lyser på SPREAD-tangentene, og at en tangent mellom to brukte ikke
gjør noe. Den sjekker
også at et trykk 80 ms for sent gir GOOD, at feil tangent bryter comboen, kampen mot boten, og to vinduer over
LINK (roller, start, klokke, poeng og stopp).

Låtene (D3): de innebygde MIDI-filene gir samme notekart og oktavskift som før. En testlåt i E-moll (format 0,
running status) blir lastet inn via filvelgeren, transponert, får oktavskift og spilles 100 %. Rapporten nevner det
som er tatt ut eller ignorert. Live-MIDI går til riktig del per kanal, og malen lastes ned identisk med
`songs/neon-carrier.mid`. Lyden (D4): begge låtene rendres offline i 20 sekunder uten klipping, og WAV-filene havner
i `build/test/`.

## Låtene

`window.KEYFALL_SONGS` i HTML-en er de innebygde MIDI-filene i base64. `python3 tools/songs.py` lager `songs/*.mid`
og oppdaterer blokka. HTML-en får da ny SHA-256.

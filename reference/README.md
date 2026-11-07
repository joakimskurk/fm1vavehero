# Referansen

`keyfall.html` er Keyfall i nettleseren: én fil, uten nettverk og uten byggesteg. Til Dinghy-porten finnes, er den
fasit for hvordan Keyfall føles, times og høres. Når C-koden og HTML-en er uenige, vinner HTML-en, slik som i
RoboSeq og SkurkyMello.

| Fil | Versjon | SHA-256 |
|---|---|---|
| `keyfall.html` | REF 0.1.1 | `4b480fe6907b88ca178ae922a78f850161f0fb0430c9ae94e64c8512f54695e8` |

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
at ingen notekart har svarte tangenter, og at en svart tangent under spill blir ignorert (D1). Den sjekker
også at et trykk 80 ms for sent gir GOOD, at feil tangent bryter comboen, kampen mot boten, og to vinduer over
LINK (roller, start, klokke, poeng og stopp).

# ROADMAP

Den gjeldende fasen er den første med et punkt som ikke er krysset av. Samme oppskrift som RoboSeq og Star Distroyer:
først en spillbar nettleserreferanse som er fasit for følelse, timing og lyd, deretter en Dinghy 0.4-app som testes
på verten og i fm1-emu.

**To spillere er bakt inn fra start.** Spillmotoren er deterministisk og har én protokoll for LINK
([docs/link.md](docs/link.md)), så alle faser under må holde VS-modus i live.

## Fase 0: idé og mockup

- [x] Konsept ([docs/konsept.md](docs/konsept.md))
- [x] Mockup av enheten og fire skjermer i 240 × 240 med Dinghys Terminus-fonter ([mockup/](mockup/))

## Fase 1: nettleserreferanse 0.1

- [x] `reference/keyfall.html`: én fil, spillbar med datamaskintastaturet, panelet (mus og berøring) og WebMIDI
- [x] Skjermen tegnet piksel for piksel i 240 × 240 med `FONT_S` og `FONT_L`
- [x] To låter (FIRST LIGHT, NEON CARRIER) med EASY og MED. Nivåene lages av samme notekart.
- [x] Notebane i perspektiv med én bane per hvit tangent, akkorder, hold-toner og oktavskift (OCT+/OCT−)
- [x] D1: bare de hvite tangentene spilles. De svarte vises, men har ingen funksjon (REF 0.1.2,
      [docs/DECISIONS.md](docs/DECISIONS.md)).
- [x] D2: SPREAD, annenhver hvit tangent for store fingre, med GLO
- [x] Poeng: PERFECT ±35 ms, GREAT ±70 ms, GOOD ±110 ms, combo og multiplikator, WRONG for feil tangent
- [x] OP-fraser og OP BOOST med FX
- [x] Tangent-LED-er (lyser og glød) og knappe-LED-er, slik FM-1 ville vist dem
- [x] FM-lyd: tre stemmer for spilleren (TONE) og et band med pad, bass og trommer. Lyden blir lysere med combo og
      matt ved feil.
- [x] Latensinnstilling, med kalibrering fra siste rundes snittavvik
- [x] LINK L0 og L1: protokoll versjon 1, to vinduer, og en bot som motstander
- [x] Test: `node tests/reference.cjs` spiller begge låtene på autospill, prøver feil og sene trykk, bot-kamp og to vinduer

## Fase 2: referanse 0.2, spillfølelse

- [ ] Spilltest med ekte hender: banebredde, notefart og tidsvinduer
- [ ] Sweep-partier med KNOB 1
- [ ] Spilltest av SPREAD med ekte hender: er F3–F5 riktig, eller G3–G5?
- [ ] Transponering per låt, så de hvite tangentene følger låtas toneart
- [ ] Øvemodus: loop av et parti, «vent på riktig tangent», og en hjelpemelodi
- [ ] Egen kalibreringsskjerm: trykk i takt med klikk
- [ ] HARD-nivå og 2–3 nye låter
- [ ] Notekartformat i JSON, og import fra MIDI
- [ ] Lydsjekk: renders av referansen som fasit for porten, som i RoboSeq

## Fase 3: Dinghy-port

- [ ] `keyfall.c` på Dinghy 0.4: `APP_NAME "KEYFALL"`, egen `FELUCCA_ID`, `PLATFORM.md` med avvik fra upstream
- [ ] Bane, toppstripe og knotterad i lerreter. Mål: minst 30 fps for banen.
- [ ] Tangent-LED-er med `fm1_led_key` og `fm1_led_key_glow`, og SPREAD (GLO) lagret i innstillingene
- [ ] Presis timing: tangentkanter leses i `LCD_IDLE` og stemples med `fm1_ticks()`
- [ ] FM-motor og band i lydavbruddet (fastkomma, begrenset arbeid), sjekket mot referansens renders
- [ ] Notekart i firmware, og beste resultat i flash
- [ ] Spillmotoren er klar for LINK fra første versjon: START, klokke og poeng går gjennom én modul
- [ ] Vertstester, fm1-emu i strict mode, og `.fwsc` med commit og SHA-256

## Fase 4: LINK på maskinvare, to FM-1-er koblet med USB-C

- [ ] **Forstudie for USB-C direkte (L3).** Kan AC791N være USB-vert (OTG)? Kan FM-1 gi 5 V på VBUS, eller trengs en
      OTG-adapter med strøm? Hvor stor blir en USB-MIDI-vertsdriver? Denne kan starte nå, parallelt med fase 2–3,
      fordi svaret avgjør maskinvaresporet.
- [ ] L2: relé-side i referansen (WebMIDI) som kobler FM-1 A og FM-1 B gjennom en datamaskin, og firmware som
      snakker protokollen
- [ ] L3: vertsmodus i firmware (P1 er vert) og én USB-C-kabel mellom to FM-1-er, hvis forstudien sier ja. Ellers
      blir L2 standard.
- [ ] Tempojustering hos P2 etter klokka (avvik over 10 ms)
- [ ] Moduser: VS på maskinvare, deretter annenhver frase og band (samarbeid)
- [ ] Test med to ekte FM-1-er: start, synk over en hel låt, frakobling midt i en kamp

## Fase 5: utgivelse

- [ ] Kort for Firmware Hub (`fmhub.json`), logo og bruksanvisning med leksjoner
- [ ] Installasjon og retur til M-VAVEs firmware beskrevet, som i de andre prosjektene

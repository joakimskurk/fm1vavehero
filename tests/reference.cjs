// Plays reference/keyfall.html in Chromium: both songs on autoplay, a match against the bot, and two windows over LINK.
// node tests/reference.cjs [outdir]     (Playwright with Chromium; the screens land in outdir, default build/test)
const { chromium } = require('playwright');
const path = require('path');
const fs = require('fs');

const URL = 'file://' + path.resolve(__dirname, '..', 'reference', 'keyfall.html');
const OUT = path.resolve(process.argv[2] || path.join(__dirname, '..', 'build', 'test'));
fs.mkdirSync(OUT, { recursive: true });
let failures = 0;
const check = (ok, what) => { console.log((ok ? 'ok   ' : 'FAIL ') + what); if (!ok) failures++; };
const sleep = ms => new Promise(r => setTimeout(r, ms));

async function open(ctx) {
  const page = await ctx.newPage();
  const errors = [];
  page.on('pageerror', e => errors.push(e.message));
  page.on('console', m => { if (m.type() === 'error') errors.push(m.text()); });
  await page.goto(URL);
  await page.waitForFunction(() => !!window.KF);
  return { page, errors };
}
async function screen(page, name) {   // the 240 x 240 screen, as drawn
  const url = await page.evaluate(() => document.getElementById('scr').toDataURL('image/png'));
  fs.writeFileSync(path.join(OUT, name + '.png'), Buffer.from(url.split(',')[1], 'base64'));
}
async function waitBeat(page, b, timeout = 90000) {
  await page.waitForFunction(b => window.KF.play && window.KF.play.beat >= b, b, { timeout, polling: 20 });
}
async function waitResults(page, timeout = 90000) {
  await page.waitForFunction(() => window.KF.G.scr === 'results', null, { timeout, polling: 100 });
}
async function choose(page, song, level) {
  await page.evaluate(([s, l]) => { window.KF.G.sel = s; window.KF.G.level = l; }, [song, level]);
}

(async () => {
  const browser = await chromium.launch({ args: ['--autoplay-policy=no-user-gesture-required'] });
  const ctx = await browser.newContext({ viewport: { width: 1280, height: 1000 }, deviceScaleFactor: 1 });

  /* 1. one player, autoplay: every note must be PERFECT, both OCT shifts hit, OP BOOST used */
  const a = await open(ctx);
  await screen(a.page, '01-splash');
  await a.page.keyboard.press('Enter');
  check(await a.page.evaluate(() => window.KF.G.scr) === 'songs', 'PLAY on the splash opens SONGS');
  await screen(a.page, '02-songs');
  const charts = await a.page.evaluate(() => window.KF.SONGS.map((s, i) => [0, 1].map(l => window.KF.buildChart(s, l).notes.length)));
  check(charts.every(c => c[0] > 10 && c[1] > c[0]), 'charts build, EASY is smaller than MED: ' + JSON.stringify(charts));
  const blacks = await a.page.evaluate(() => window.KF.SONGS.flatMap(s => [0, 1].flatMap(l => window.KF.buildChart(s, l).notes)).filter(n => [1, 3, 6, 8, 10].includes(((n.pitch % 12) + 12) % 12)).length);
  check(blacks === 0, 'D1: no black keys in any chart');

  await a.page.evaluate(() => window.KF.setAuto(true));
  await choose(a.page, 0, 1); await a.page.keyboard.press('Enter');
  await waitBeat(a.page, 2); await screen(a.page, '03-first-light-intro');
  await waitResults(a.page);
  let L = await a.page.evaluate(() => window.KF.G.last);
  check(L.acc === 100 && L.counts.M === 0 && L.grade === 'S', `FIRST LIGHT MED on autoplay: ${L.acc.toFixed(1)}% ${L.grade}, ${L.counts.M} misses`);
  check(Math.abs(L.meanDt) < 5, `mean timing ${L.meanDt.toFixed(2)} ms`);
  await screen(a.page, '04-first-light-results');

  await a.page.keyboard.press('Escape');
  await choose(a.page, 1, 1); await a.page.keyboard.press('Enter');
  let boosted = false;
  await waitBeat(a.page, 22); await screen(a.page, '05-neon-verse'); await a.page.screenshot({ path: path.join(OUT, '05b-page-play.png') });
  await waitBeat(a.page, 37.9); await screen(a.page, '06-neon-oct-band');
  await waitBeat(a.page, 44); await screen(a.page, '07-neon-upper-octave');
  for (let i = 0; i < 400 && !boosted; i++) { boosted = await a.page.evaluate(() => !!(window.KF.play && window.KF.play.boostTo != null)); if (!boosted) await sleep(100); }
  check(boosted, 'OP BOOST was activated');
  if (boosted) { await sleep(150); await screen(a.page, '08-neon-boost'); }
  await waitBeat(a.page, 84); await screen(a.page, '09-neon-chorus');
  await waitResults(a.page);
  L = await a.page.evaluate(() => window.KF.G.last);
  check(L.acc === 100 && L.counts.S === 2 && L.counts.M === 0, `NEON CARRIER MED on autoplay: ${L.acc.toFixed(1)}%, ${L.counts.S} shifts, ${L.counts.M} misses, full combo ${L.full}`);
  await screen(a.page, '10-neon-results');
  await a.page.screenshot({ path: path.join(OUT, '11-page.png') });
  check(a.errors.length === 0, 'no page errors' + (a.errors.length ? ': ' + a.errors.join(' | ') : ''));

  /* 2. a wrong key breaks the combo, a late press is GOOD, not PERFECT */
  await a.page.evaluate(() => window.KF.setAuto(false));
  await a.page.keyboard.press('Escape'); await choose(a.page, 0, 1); await a.page.keyboard.press('Enter');
  await waitBeat(a.page, 7.5);
  const probe = await a.page.evaluate(() => {
    const p = window.KF.play, n = p.notes[0];               /* C4 at beat 8 */
    return { t: n.t, pitch: n.pitch, state: n.state };
  });
  await waitBeat(a.page, 8.03);
  const after = await a.page.evaluate(() => {                    /* C4, stamped exactly 80 ms after it was due */
    const p = window.KF.play, bms = 60000 / p.bpm, ts = performance.now() - (p.beat - 8) * bms + 80 + window.KF.G.offset;
    window.KF.pressKey('test-late', 7, ts, 'test'); window.KF.releaseNote('test-late', ts + 50);
    return { state: p.notes[0].state, judge: p.judge && p.judge.text, combo: p.combo };
  });
  check(probe.pitch === 60 && after.state === 'hit' && after.judge === 'GOOD', `C4 80 ms late is GOOD: ${JSON.stringify(after)}`);
  await waitBeat(a.page, 8.95);
  await a.page.keyboard.down('KeyK'); await a.page.keyboard.up('KeyK');  /* C5 where E4 is due */
  const wrong = await a.page.evaluate(() => ({ judge: window.KF.play.judge && window.KF.play.judge.text, combo: window.KF.play.combo }));
  check(wrong.judge === 'WRONG' && wrong.combo === 0, `a wrong key breaks the combo: ${JSON.stringify(wrong)}`);
  await waitBeat(a.page, 11.93);                                 /* G4 is due at beat 12 (bar 3, beat 0)... */
  const black = await a.page.evaluate(() => {                    /* ...and a black key (C#4) next to it changes nothing */
    const p = window.KF.play, before = { combo: p.combo, judge: p.judge && p.judge.text, t0: p.judge && p.judge.t0 };
    window.KF.pressKey('test-black', 8, performance.now(), 'test'); window.KF.releaseNote('test-black', performance.now());
    return { before, combo: p.combo, judge: p.judge && p.judge.text, same: (p.judge && p.judge.t0) === before.t0 };
  });
  check(black.same && black.combo === black.before.combo, `D1: a black key in play is ignored: ${JSON.stringify(black)}`);
  await a.page.keyboard.press('Escape');

  /* 2b. SPREAD (D2): every other white key. Autoplay is still perfect, the LEDs light only the spread keys,
     a key between them does nothing, and the spread key of the lane hits. */
  await a.page.evaluate(() => { document.getElementById('spread').click(); window.KF.setAuto(true); });
  check(await a.page.evaluate(() => window.KF.G.spread) === true, 'SPREAD turns on');
  await choose(a.page, 1, 1); await a.page.keyboard.press('Enter');
  await waitBeat(a.page, 8);                                     /* after the intro, when the notes come */
  let ledsOk = true, seen = new Set();
  for (let i = 0; i < 40; i++) {
    const r = await a.page.evaluate(() => ({ L: window.KF.leds(), lay: window.KF.LAYOUTS.spread }));
    r.L.forEach((v, k) => { if (v) { seen.add(k); if (!r.lay.includes(k)) ledsOk = false; } });
    await sleep(120);
  }
  check(ledsOk && seen.size >= 4, `SPREAD: the LEDs light only F3 A3 C4 E4 G4 B4 D5 F5 (saw keys ${[...seen].sort((x, y) => x - y).join(' ')})`);
  await screen(a.page, '05c-spread'); await a.page.screenshot({ path: path.join(OUT, '05d-page-spread.png') });
  await waitResults(a.page);
  L = await a.page.evaluate(() => window.KF.G.last);
  check(L.acc === 100 && L.counts.M === 0, `NEON CARRIER MED with SPREAD on autoplay: ${L.acc.toFixed(1)}%`);
  await a.page.evaluate(() => window.KF.setAuto(false));
  await a.page.keyboard.press('Escape'); await choose(a.page, 0, 1); await a.page.keyboard.press('Enter');
  await waitBeat(a.page, 7.97);
  const sp = await a.page.evaluate(() => {                       /* C4 is due at beat 8: lane 0, the F3 key with SPREAD */
    const p = window.KF.play, bms = 60000 / p.bpm, at = performance.now() - (p.beat - 8) * bms + window.KF.G.offset;
    const t0 = p.judge && p.judge.t0;
    window.KF.pressKey('sp-gap', 9, at, 'test'); window.KF.releaseNote('sp-gap', at + 30);       /* D4: between two spread keys */
    const gapIgnored = (p.judge && p.judge.t0) === t0 && p.notes[0].state === 'pending';
    window.KF.pressKey('sp-hit', 0, at, 'test'); window.KF.releaseNote('sp-hit', at + 30);       /* F3: lane 0 */
    return { gapIgnored, state: p.notes[0].state, judge: p.judge && p.judge.text };
  });
  check(sp.gapIgnored && sp.state === 'hit' && sp.judge === 'PERFECT', `SPREAD: the key between does nothing, F3 plays lane 0: ${JSON.stringify(sp)}`);
  await a.page.keyboard.press('Escape');
  await a.page.evaluate(() => document.getElementById('spread').click());
  check(await a.page.evaluate(() => window.KF.G.spread) === false, 'SPREAD turns off');

  /* 3. against the bot: the protocol's receive path, and a winner */
  await a.page.evaluate(() => { document.querySelector('input[name=link][value=bot]').click(); window.KF.setAuto(true); });
  await choose(a.page, 0, 0); await a.page.keyboard.press('Enter');
  await waitBeat(a.page, 30); await screen(a.page, '12-vs-bot');
  await waitResults(a.page); await sleep(300);
  const vs = await a.page.evaluate(() => ({ me: window.KF.G.last.score, bot: window.KF.Link.rival.score, done: window.KF.Link.rival.done, vs: window.KF.G.last.vs }));
  check(vs.vs && vs.done && vs.bot > 0 && vs.me >= vs.bot, `VS bot: me ${vs.me}, bot ${vs.bot}`);
  await screen(a.page, '13-vs-bot-results');
  await a.page.evaluate(() => document.querySelector('input[name=link][value=off]').click());

  /* 4. two windows over LINK (BroadcastChannel): roles, P1 starts both, scores cross, STOP brings P2 home */
  const p1 = a, p2 = await open(ctx);
  for (const x of [p1, p2]) {
    await x.page.bringToFront();
    await x.page.evaluate(() => { document.querySelector('input[name=link][value=tab]').click(); });
    await x.page.keyboard.press('Escape'); await x.page.keyboard.press('Escape'); await x.page.keyboard.press('Enter');
  }
  await sleep(2600);
  const roles = await Promise.all([p1, p2].map(x => x.page.evaluate(() => ({ role: window.KF.Link.role(), linked: window.KF.Link.linked(), id: window.KF.Link.id }))));
  check(roles[0].linked && roles[1].linked && roles[0].role !== roles[1].role, 'two windows link up as P1 and P2: ' + JSON.stringify(roles));
  const lead = roles[0].role === 'P1' ? p1 : p2, follow = lead === p1 ? p2 : p1;
  await lead.page.evaluate(() => window.KF.setAuto(true));
  await follow.page.evaluate(() => window.KF.setAuto(true));
  await choose(lead.page, 1, 1); await sleep(1200);
  check(await follow.page.evaluate(() => window.KF.G.sel) === 1, 'P2 follows the song P1 picks');
  await lead.page.bringToFront(); await lead.page.keyboard.press('Enter');
  await waitBeat(follow.page, 1, 5000).catch(() => {});
  check(await follow.page.evaluate(() => window.KF.G.scr) === 'play', 'START from P1 starts P2');
  await waitBeat(lead.page, 24);
  const cross = await Promise.all([lead, follow].map(x => x.page.evaluate(() => ({ mine: window.KF.play.score, rival: window.KF.Link.rival.score, clocks: window.KF.Link.rival.clocks }))));
  check(cross[1].rival > 0 && cross[0].rival > 0, 'each window sees the other\'s score: ' + JSON.stringify(cross));
  check(cross[1].clocks > 24 * 16, `P2 gets P1's clock: ${cross[1].clocks} pulses`);
  await screen(lead.page, '14-link-p1'); await screen(follow.page, '15-link-p2');
  await lead.page.keyboard.press('Escape');
  await sleep(400);
  check(await follow.page.evaluate(() => window.KF.G.scr) === 'songs', 'STOP from P1 brings P2 back to SONGS');
  check(p1.errors.length === 0 && p2.errors.length === 0, 'no page errors in either window' + (p1.errors.concat(p2.errors).length ? ': ' + p1.errors.concat(p2.errors).join(' | ') : ''));

  await browser.close();
  console.log(failures ? `${failures} FAILED` : 'all passed');
  process.exitCode = failures ? 1 : 0;
})().catch(e => { console.error(e); process.exit(1); });

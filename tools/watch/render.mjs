// render.mjs — the watch's screens without a board or a browser: the round watch's engine (studio/web/build, built by
// studio/engine/build.sh — the firmware itself, compiled to WebAssembly) run in Node, driven by touches, PNGs out.
//
//   node tools/watch/render.mjs OUT_DIR "STEP; STEP; …"   [ENGINE=studio/web/build-x for another engine folder]
//
// Steps:  tap X,Y | drag X0,Y0,X1,Y1[,FRAMES] | wait FRAMES | shot NAME | cmd SERIAL COMMAND
// It starts on the face with two demo cities' weather (humidity, wind, UV, sunrise and sunset too). On the default
// firmware: a tap at 233,355 (the weather pill) opens Weather, a drag 233,300 → 233,40 the Apps list, a drag from the
// left edge goes back. Pixels outside the circle are stale (the panel's bands skip them): tools/watch/sheet.py masks
// them. Example:
//   node tools/watch/render.mjs /tmp/wx "tap 233,355; wait 60; shot hero; drag 233,400,233,200; shot hours; cmd upcheck"
// (cmd upcheck prints the paged apps' overlap audit into OUT_DIR/log.txt)
import fs from 'node:fs';
import zlib from 'node:zlib';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const B = path.resolve(ROOT, process.env.ENGINE || 'studio/web/build');
const [OUT, SCRIPT = 'shot face'] = process.argv.slice(2);
if (!OUT) {
  console.error('render.mjs OUT_DIR "tap X,Y; drag X0,Y0,X1,Y1; wait N; shot NAME; cmd LINE"');
  process.exit(1);
}
const factory = (await import(path.join(B, 'PopkaWatch.js'))).default;
const wasm = fs.readFileSync(path.join(B, 'PopkaWatch.wasm'));
const log = [];
const M = await factory({
  print: (s) => log.push(s), printErr: (s) => log.push(s),
  instantiateWasm: (imp, done) => { WebAssembly.instantiate(wasm, imp).then((r) => done(r.instance, r.module)); return {}; },
});
const W = 466, H = 466;
let t = 1000;
const call = (f, ...a) => M['_' + f](...a);
const frame = (n = 1) => { for (let i = 0; i < n; i++) t += 1000 / 60, call('web_frame', t, 1); };

const CRC = Array.from({ length: 256 }, (_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; return c >>> 0; });
const crc = (b) => { let r = 0xffffffff; for (const x of b) r = CRC[(r ^ x) & 255] ^ (r >>> 8); return (r ^ 0xffffffff) >>> 0; };
function png(file) {
  const rgba = new Uint8Array(M.HEAPU8.buffer, call('web_rgba'), W * H * 4), raw = Buffer.alloc((W * 4 + 1) * H);
  for (let y = 0; y < H; y++) Buffer.from(rgba.buffer, rgba.byteOffset + y * W * 4, W * 4).copy(raw, y * (W * 4 + 1) + 1);
  const chunk = (ty, d) => {
    const l = Buffer.alloc(4), c = Buffer.alloc(4), td = Buffer.concat([Buffer.from(ty), d]);
    l.writeUInt32BE(d.length), c.writeUInt32BE(crc(td));
    return Buffer.concat([l, td, c]);
  };
  const ih = Buffer.alloc(13);
  ih.writeUInt32BE(W, 0), ih.writeUInt32BE(H, 4), ih[8] = 8, ih[9] = 6;
  fs.writeFileSync(file, Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk('IHDR', ih),
    chunk('IDAT', zlib.deflateSync(raw)), chunk('IEND', Buffer.alloc(0))]));
}

// Two demo cities (studio_api.h: studio_weather's JSON)
const now = Math.floor(Date.now() / 1000), t0 = now - (now % 3600);
const city = (n, base, k) => ({
  n, now: base, feels: base - 2, hi: base + 4, lo: base - 5, k, t0, hum: 64, wind: 12, uv: 68, rise: now - 6 * 3600, set: now + 5 * 3600,
  hk: Array.from({ length: 48 }, (_, i) => [k, 1, 2, 3][(i >> 2) % 4]), ht: Array.from({ length: 48 }, (_, i) => Math.round(base + 5 * Math.sin((i - 4) / 4))),
  p: 1013, hp: Array(48).fill(1013), dk: [k, 1, 3, 0, 2, 1, 0],
  dhi: [4, 6, 2, 7, 5, 3, 8].map((d) => base + d), dlo: [-5, -3, -6, -2, -4, -5, -1].map((d) => base + d),
});
call('studio_weather', M.stringToNewUTF8(JSON.stringify({ c: [city('Bengaluru', 24, 1), city('Delhi', 31, 0)] })));
frame(30);
fs.mkdirSync(OUT, { recursive: true });

const touch = (down, x, y) => call('web_touch', down, x, y, t);
for (const step of SCRIPT.split(';').map((s) => s.trim()).filter(Boolean)) {
  const [op, ...rest] = step.split(' ');
  const a = rest.join(' ').split(',').map(Number);
  if (op === 'tap') touch(1, a[0], a[1]), frame(2), touch(0, a[0], a[1]), frame(90);
  else if (op === 'drag') {
    const n = a[4] || 12;
    touch(1, a[0], a[1]), frame(1);
    for (let i = 1; i <= n; i++) touch(1, a[0] + (a[2] - a[0]) * i / n, a[1] + (a[3] - a[1]) * i / n), frame(1);
    touch(0, a[2], a[3]), frame(90);
  } else if (op === 'wait') frame(a[0]);
  else if (op === 'shot') png(path.join(OUT, rest[0] + '.png')), console.log(path.join(OUT, rest[0] + '.png'));
  else if (op === 'cmd') call('web_command', M.stringToNewUTF8(rest.join(' '))), frame(30);
  else throw new Error(`render.mjs: unknown step "${step}"`);
}
fs.writeFileSync(path.join(OUT, 'log.txt'), log.join('\n'));

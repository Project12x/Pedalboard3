/* Scratch Mode — data model + helpers for Instant Scratch Capture.
   A take bundles raw DI + wet output + metadata (take.json), captured
   simultaneously so the raw can be reamped later. Mirrors ScratchTake.h /
   ScratchRecorderStatus from the Pedalboard 3 source. */

const SC_STATES = { READY: "ready", RECORDING: "recording", SAVING: "saving" };

/* current session context — baked into every take (from ScratchTakeContext) */
const SC_CONTEXT = {
  patch: "Lead Boost",
  patchIndex: 3,
  document: "default.pdl",
  device: "BEHRINGER USB AUDIO",
  sampleRate: 48000,
  rawCh: 2,
  wetCh: 2,
  inGainDb: 0.5,
  outGainDb: -15.7,
  root: "~/Pedalboard3/Scratch Ideas",
};

/* deterministic tiny PRNG so take thumbnails are stable across renders */
function scRng(seed) {
  let s = 0; for (let i = 0; i < seed.length; i++) s = (s * 31 + seed.charCodeAt(i)) >>> 0;
  return () => { s = (s * 1664525 + 1013904223) >>> 0; return s / 4294967296; };
}
/* a small set of bar heights for a take's frozen waveform thumbnail */
function scThumb(seed, n = 48) {
  const r = scRng(seed); const out = []; let env = 0.4;
  for (let i = 0; i < n; i++) {
    env += (r() - 0.5) * 0.5; env = Math.max(0.12, Math.min(1, env));
    const decay = i > n * 0.7 ? (1 - (i - n * 0.7) / (n * 0.3)) : 1;
    out.push(Math.max(0.06, env * (0.55 + r() * 0.45) * decay));
  }
  return out;
}

const pad2 = (n) => String(n).padStart(2, "0");
function scClock(totalSec) { const s = Math.max(0, Math.floor(totalSec)); return Math.floor(s / 60) + ":" + pad2(s % 60); }
function scTimer(ms) { const s = Math.floor(ms / 1000); const t = Math.floor((ms % 1000) / 100); return Math.floor(s / 60) + ":" + pad2(s % 60) + "." + t; }
function scTimeOfDay(d) { return pad2(d.getHours()) + ":" + pad2(d.getMinutes()) + ":" + pad2(d.getSeconds()); }
function scStamp(d) { return d.getFullYear() + "-" + pad2(d.getMonth() + 1) + "-" + pad2(d.getDate()) + "/" + pad2(d.getHours()) + pad2(d.getMinutes()) + pad2(d.getSeconds()); }

let _scN = 0;
function scMakeTake(ctx, durationSec, opts = {}) {
  const now = opts.time || new Date();
  const id = "tk_" + (++_scN) + "_" + Math.floor(now.getTime() / 1000);
  const patch = opts.patch || ctx.patch;
  const folder = scStamp(now) + "-" + (patch || "untitled").toLowerCase().replace(/[^a-z0-9]+/g, "-");
  const base = ctx.root + "/" + folder;
  return {
    id, time: now, durationSec,
    patch: patch || "<untitled>", patchIndex: opts.patchIndex != null ? opts.patchIndex : ctx.patchIndex,
    device: ctx.device, sampleRate: ctx.sampleRate, rawCh: ctx.rawCh, wetCh: ctx.wetCh,
    inGainDb: ctx.inGainDb, outGainDb: ctx.outGainDb,
    complete: opts.complete !== false,
    failureReason: opts.failureReason || "",
    rawFile: base + "/raw.wav", wetFile: base + "/wet.wav", metaFile: base + "/take.json",
    thumb: scThumb(id),
  };
}

/* seed history (most-recent first) — varied patches + one interrupted take */
function scInitialTakes() {
  const mk = (h, m, s, dur, patch, idx, opts = {}) => {
    const d = new Date(); d.setHours(h, m, s, 0);
    return scMakeTake(SC_CONTEXT, dur, { time: d, patch, patchIndex: idx, ...opts });
  };
  return [
    mk(14, 32, 8, 47, "Lead Boost", 3),
    mk(14, 18, 51, 23, "Rhythm Crunch", 5),
    mk(13, 55, 2, 12, "<untitled>", 0, { complete: false, failureReason: "Patch changed during scratch capture" }),
    mk(11, 40, 19, 88, "Clean Verb", 1),
  ];
}

const scFmtRate = (hz) => (hz % 1000 === 0 ? hz / 1000 + " kHz" : (hz / 1000).toFixed(1) + " kHz");
const scFmtRateShort = (hz) => (hz % 1000 === 0 ? hz / 1000 + "k" : (hz / 1000).toFixed(1) + "k");
const scFmtGain = (db) => (db >= 0 ? "+" : "") + db.toFixed(1) + " dB";

Object.assign(window, {
  SC_STATES, SC_CONTEXT, scThumb, scClock, scTimer, scTimeOfDay, scStamp,
  scMakeTake, scInitialTakes, scFmtRate, scFmtRateShort, scFmtGain,
});

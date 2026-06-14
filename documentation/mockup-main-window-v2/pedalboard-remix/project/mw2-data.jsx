/* Main Window v2 — data model. Graph nodes, cables, catalog, helpers. */

const M2 = {
  HEAD: 32,
  IO_TOP: 26,
  ROW: 22,
};

/* processor-category identity colours */
const M2_CAT = {
  source: { color: "#7dd3fc", label: "Sources" },
  dyn:    { color: "#4ade80", label: "Dynamics" },
  drive:  { color: "#fbbf24", label: "Drive" },
  amp:    { color: "#fb7185", label: "Amp / Cab" },
  cab:    { color: "#c084fc", label: "Cab / IR" },
  mod:    { color: "#f472d6", label: "Modulation" },
  delay:  { color: "#818cf8", label: "Delay" },
  reverb: { color: "#22d3ee", label: "Reverb" },
  fx:     { color: "#a78bfa", label: "Effects" },
  mix:    { color: "#6ee7b7", label: "Routing" },
  util:   { color: "#94a3b8", label: "Utilities" },
  meter:  { color: "#38bdf8", label: "Meters" },
  out:    { color: "#7dd3fc", label: "Output" },
};

/* Add-menu catalog */
const M2_CATALOG = [
  { cat: "dyn",    name: "Compressor",   cpu: 1.2 },
  { cat: "drive",  name: "TS808 Drive",  cpu: 0.8 },
  { cat: "amp",    name: "NAM Loader",   cpu: 4.6 },
  { cat: "cab",    name: "IR Loader",    cpu: 1.8 },
  { cat: "fx",     name: "Effect Rack",  cpu: 0.1 },
  { cat: "mod",    name: "LFO",          cpu: 0.2 },
  { cat: "delay",  name: "Deelay",       cpu: 2.1 },
  { cat: "reverb", name: "Hall Reverb",  cpu: 3.8 },
  { cat: "mix",    name: "DAW Mixer",    cpu: 0.8 },
  { cat: "mix",    name: "DAW Splitter", cpu: 0.3 },
  { cat: "meter",  name: "VU Meter",     cpu: 0.3 },
  { cat: "meter",  name: "Tuner",        cpu: 0.4 },
  { cat: "util",   name: "Note",         cpu: 0.0 },
];

let _m2Id = 100;
const m2Uid = (p) => (p || "n") + "_" + (++_m2Id);

function stereoPins(opts = {}) {
  const t = M2.IO_TOP, r = M2.ROW;
  const lab = opts.labels === false ? "" : null;
  const pins = [
    { id: "inL",  side: "l", dy: t,     kind: "audio", dir: "in",  label: lab == null ? "Left"  : lab },
    { id: "inR",  side: "l", dy: t + r, kind: "audio", dir: "in",  label: lab == null ? "Right" : lab },
    { id: "outL", side: "r", dy: t,     kind: "audio", dir: "out", label: lab == null ? "Left"  : lab },
    { id: "outR", side: "r", dy: t + r, kind: "audio", dir: "out", label: lab == null ? "Right" : lab },
  ];
  if (opts.param) pins.push({ id: "pin", side: "l", dy: t + 2 * r, kind: "param", dir: "in", label: lab == null ? "param" : lab });
  return pins;
}

/* ---- initial patch graph ---- */
function m2InitialGraph() {
  const N = [];
  const push = (n) => { N.push(n); return n; };

  /* sources */
  const ain = push({ id: "src_audio", type: "source", cat: "source", name: "Audio Input",
    sub: "BEHRINGER USB AUDIO", x: 36, y: 156, w: 188, h: 132,
    faders: [{ label: "1", db: "+0.5" }, { label: "2", db: "0.0" }],
    pins: [{ id: "o1", side: "r", dy: 46, kind: "audio", dir: "out", label: "" },
           { id: "o2", side: "r", dy: 84, kind: "audio", dir: "out", label: "" }] });

  push({ id: "src_vmidi", type: "io-mini", cat: "source", name: "Virtual MIDI Input",
    sub: "Virtual Keyboard", x: 36, y: 296, w: 188, h: 72,
    pins: [{ id: "po", side: "r", dy: 44, kind: "param", dir: "out", label: "" }] });

  push({ id: "src_midi", type: "io-mini", cat: "source", name: "MIDI Input",
    sub: "param", x: 36, y: 406, w: 188, h: 64,
    pins: [{ id: "po", side: "r", dy: 40, kind: "param", dir: "out", label: "" }] });

  push({ id: "src_osc", type: "io-mini", cat: "source", name: "OSC Input",
    sub: "param", x: 36, y: 500, w: 188, h: 64,
    pins: [{ id: "po", side: "r", dy: 40, kind: "param", dir: "out", label: "" }] });

  /* dynamics */
  const comp = push({ id: "fx_comp", type: "module", cat: "dyn", name: "Compressor",
    tone: "Opto Smooth", cpu: 1.2, x: 276, y: 214, w: 192,
    params: [["Thr", "-18", 0.42], ["Ratio", "4:1", 0.55], ["Gain", "+3", 0.62]],
    pins: stereoPins(), bypass: false, mute: false });

  /* NAM Loader — chassis node */
  const amp = push({ id: "fx_amp", type: "plugin", cat: "amp", name: "NAM Loader",
    model: "Friedman BE-100", cpu: 4.6, x: 520, y: 150, w: 304,
    ir1: "4×12 Greenback", ir2: null,
    lo: "80 Hz", hi: "12.0 kHz", blend: 0.0,
    gain: [["Input", "0.0", 0.5], ["Output", "0.0", 0.5], ["Gate", "-80", 0.08]],
    tone: [["Bass", 0.6], ["Mid", 0.5], ["Treble", 0.68]],
    pins: stereoPins({ param: true, labels: false }), bypass: false, mute: false });

  /* IR Loader — chassis node */
  const irLoader = push({ id: "fx_ir", type: "ir-loader", cat: "cab", name: "IR Loader",
    ir1: "4×12 Greenback", ir2: "2×12 Alnico Blue",
    blend: 0.65, loCut: 80, hiCut: 12000,
    cpu: 1.8, x: 880, y: 540, w: 280,
    pins: stereoPins({ labels: false }), bypass: false, mute: false });

  /* LFO */
  const lfo = push({ id: "mod_lfo", type: "mod", cat: "mod", name: "LFO",
    tone: "Sine · 1.2 Hz", cpu: 0.2, x: 884, y: 92, w: 184,
    params: [["Rate", "28", 0.28], ["Depth", "45", 0.45]],
    pins: [{ id: "pout", side: "r", dy: M2.IO_TOP, kind: "param", dir: "out", label: "" }],
    bypass: false, mute: false });

  /* Deelay */
  const delay = push({ id: "fx_delay", type: "module", cat: "delay", name: "Deelay",
    tone: "Vintage 60s", cpu: 2.1, x: 884, y: 300, w: 192,
    params: [["Time", "380", 0.5], ["Fbk", "42", 0.42], ["Mix", "35", 0.35]],
    pins: stereoPins({ param: true }), bypass: false, mute: false });

  /* Effect Rack — nested sub-graph */
  push({ id: "fx_rack", type: "effect-rack", cat: "fx", name: "Effect Rack",
    cpu: 5.9, x: 276, y: 510, w: 224,
    contains: [
      { short: "DRV", color: "#fbbf24", name: "TS808 Drive" },
      { short: "MOD", color: "#f472d6", name: "Tremolo" },
      { short: "VRB", color: "#22d3ee", name: "Spring Reverb" },
    ],
    pins: stereoPins({ labels: false }), bypass: false, mute: false });

  /* DAW Splitter */
  const splitter = push({ id: "fx_split", type: "splitter", cat: "mix", name: "DAW Splitter",
    cpu: 0.3, x: 36, y: 600, w: 200,
    outputs: [
      { db: "0.0",   level: 64 },
      { db: "−6.0",  level: 40 },
      { db: "−12.0", level: 22 },
    ],
    pins: [
      { id: "in",   side: "l", dy: M2.IO_TOP,           kind: "audio", dir: "in",  label: "" },
      { id: "out1", side: "r", dy: M2.IO_TOP,           kind: "audio", dir: "out", label: "" },
      { id: "out2", side: "r", dy: M2.IO_TOP + M2.ROW,  kind: "audio", dir: "out", label: "" },
      { id: "out3", side: "r", dy: M2.IO_TOP + M2.ROW * 2, kind: "audio", dir: "out", label: "" },
    ],
    bypass: false, mute: false });

  /* VU Meter */
  const vu = push({ id: "mtr_vu", type: "vu", cat: "meter", name: "VU Meter",
    cpu: 0.3, x: 1140, y: 176, w: 168, h: 196,
    pins: stereoPins({ labels: false }) });

  /* Hall Reverb */
  const hall = push({ id: "fx_hall", type: "module", cat: "reverb", name: "Hall Reverb",
    tone: "Cathedral 40%", cpu: 3.8, x: 1140, y: 432, w: 192,
    params: [["Decay", "58", 0.58], ["Size", "70", 0.7], ["Mix", "40", 0.4]],
    pins: stereoPins(), bypass: false, mute: false });

  /* DAW Mixer */
  push({ id: "fx_mix", type: "mixer", cat: "mix", name: "DAW Mixer",
    cpu: 0.8, x: 1360, y: 440, w: 220,
    strips: [
      { db: "0.0",  pan: 0,     mute: false, vol: 0.72 },
      { db: "−4.5", pan: 0.2,   mute: false, vol: 0.55 },
      { db: "−12",  pan: -0.15, mute: true,  vol: 0.28 },
    ],
    pins: [
      { id: "in1",  side: "l", dy: M2.IO_TOP,           kind: "audio", dir: "in",  label: "" },
      { id: "in2",  side: "l", dy: M2.IO_TOP + M2.ROW,  kind: "audio", dir: "in",  label: "" },
      { id: "in3",  side: "l", dy: M2.IO_TOP + M2.ROW * 2, kind: "audio", dir: "in", label: "" },
      { id: "outL", side: "r", dy: M2.IO_TOP,           kind: "audio", dir: "out", label: "" },
      { id: "outR", side: "r", dy: M2.IO_TOP + M2.ROW,  kind: "audio", dir: "out", label: "" },
    ],
    bypass: false, mute: false });

  /* Audio Output */
  const out = push({ id: "out_audio", type: "output", cat: "out", name: "Audio Output",
    sub: "BEHRINGER USB AUDIO", x: 1364, y: 176, w: 196, h: 156,
    meters: [{ label: "1", db: "-2.3" }, { label: "2", db: "-27.3" }],
    pins: [{ id: "i1", side: "l", dy: 50, kind: "audio", dir: "in", label: "" },
           { id: "i2", side: "l", dy: 96, kind: "audio", dir: "in", label: "" }] });

  /* Tuner */
  const tuner = push({ id: "mtr_tuner", type: "tuner", cat: "meter", name: "Tuner",
    cpu: 0.4, x: 552, y: 660, w: 332, h: 232,
    pins: [{ id: "inL", side: "l", dy: 40, kind: "audio", dir: "in", label: "" },
           { id: "pin", side: "l", dy: 200, kind: "param", dir: "in", label: "" }] });

  /* Note */
  push({ id: "util_note", type: "note", cat: "util", name: "Note",
    cpu: 0, x: 40, y: 810, w: 210,
    text: "Guitar → Comp → NAM → IR → Output\n\nGate threshold: −80 dB\nIR blend: 65% Greenback",
    pins: [] });

  const C = (a, ap, b, bp, kind) => ({ id: m2Uid("c"), from: { node: a, port: ap }, to: { node: b, port: bp }, kind: kind || "audio" });
  const conns = [
    C(ain.id, "o1", comp.id, "inL"),
    C(ain.id, "o2", comp.id, "inR"),
    C(comp.id, "outL", amp.id, "inL"),
    C(comp.id, "outR", amp.id, "inR"),
    C(amp.id, "outL", delay.id, "inL"),
    C(amp.id, "outR", delay.id, "inR"),
    C(amp.id, "outL", tuner.id, "inL"),
    C(amp.id, "outL", irLoader.id, "inL"),
    C(amp.id, "outR", irLoader.id, "inR"),
    C(delay.id, "outL", vu.id, "inL"),
    C(delay.id, "outR", vu.id, "inR"),
    C(vu.id, "outL", out.id, "i1"),
    C(vu.id, "outR", out.id, "i2"),
    C(delay.id, "outL", hall.id, "inL"),
    C(delay.id, "outR", hall.id, "inR"),
    C(lfo.id, "pout", delay.id, "pin", "param"),
  ];
  return { nodes: N, conns };
}

/* make a new node from Add-menu entry */
function m2MakeNode(entry, x, y) {
  const base = { id: m2Uid(entry.cat), cat: entry.cat, name: entry.name, cpu: entry.cpu, x, y, bypass: false, mute: false };

  if (entry.cat === "mod") return { ...base, type: "mod", tone: "Sine · 1.2 Hz", w: 184,
    params: [["Rate", "28", 0.28], ["Depth", "45", 0.45]],
    pins: [{ id: "pout", side: "r", dy: M2.IO_TOP, kind: "param", dir: "out", label: "" }] };

  if (entry.name === "Tuner") return { ...base, type: "tuner", w: 332, h: 232,
    pins: [{ id: "inL", side: "l", dy: 40, kind: "audio", dir: "in", label: "" }] };

  if (entry.name === "VU Meter") return { ...base, type: "vu", w: 168, h: 196, pins: stereoPins() };

  if (entry.cat === "amp" || entry.name === "NAM Loader") return { ...base, type: "plugin", w: 304,
    model: "No Model", ir1: null, ir2: null,
    lo: "80 Hz", hi: "12.0 kHz", blend: 0,
    gain: [["Input", "0.0", 0.5], ["Output", "0.0", 0.5], ["Gate", "-80", 0.08]],
    tone: [["Bass", 0.5], ["Mid", 0.5], ["Treble", 0.5]],
    pins: stereoPins({ param: true, labels: false }) };

  if (entry.cat === "cab" || entry.name === "IR Loader") return { ...base, type: "ir-loader", w: 280,
    ir1: null, ir2: null, blend: 0.5, loCut: 80, hiCut: 12000,
    pins: stereoPins({ labels: false }) };

  if (entry.name === "Effect Rack") return { ...base, type: "effect-rack", w: 224,
    contains: [{ short: "FX1", color: "#fbbf24", name: "Effect 1" }, { short: "FX2", color: "#22d3ee", name: "Effect 2" }],
    pins: stereoPins({ labels: false }) };

  if (entry.name === "DAW Mixer") return { ...base, type: "mixer", w: 200,
    strips: [{ db: "0.0", pan: 0, mute: false, vol: 0.72 },
             { db: "0.0", pan: 0, mute: false, vol: 0.72 }],
    pins: [{ id: "in1", side: "l", dy: M2.IO_TOP, kind: "audio", dir: "in", label: "" },
           { id: "in2", side: "l", dy: M2.IO_TOP + M2.ROW, kind: "audio", dir: "in", label: "" },
           { id: "outL", side: "r", dy: M2.IO_TOP, kind: "audio", dir: "out", label: "" },
           { id: "outR", side: "r", dy: M2.IO_TOP + M2.ROW, kind: "audio", dir: "out", label: "" }] };

  if (entry.name === "DAW Splitter") return { ...base, type: "splitter", w: 200,
    outputs: [{ db: "0.0", level: 60 }, { db: "0.0", level: 60 }],
    pins: [{ id: "in",   side: "l", dy: M2.IO_TOP, kind: "audio", dir: "in", label: "" },
           { id: "out1", side: "r", dy: M2.IO_TOP, kind: "audio", dir: "out", label: "" },
           { id: "out2", side: "r", dy: M2.IO_TOP + M2.ROW, kind: "audio", dir: "out", label: "" }] };

  if (entry.name === "Note") return { ...base, type: "note", w: 210,
    text: "Add your note here…", pins: [] };

  /* generic processor */
  return { ...base, type: "module", tone: "Default", w: 192,
    params: [["A", "50", 0.5], ["B", "50", 0.5], ["C", "50", 0.5]],
    pins: stereoPins({ param: entry.cat === "delay" }) };
}

/* helpers */
function m2Pin(node, portId) { return (node.pins || []).find((p) => p.id === portId); }
function m2PortPos(node, portId) {
  const p = m2Pin(node, portId);
  if (!p) return { x: node.x, y: node.y };
  return { x: p.side === "l" ? node.x : node.x + node.w, y: node.y + M2.HEAD + p.dy };
}
function m2Compatible(nodeA, pa, nodeB, pb) {
  const A = m2Pin(nodeA, pa), B = m2Pin(nodeB, pb);
  if (!A || !B) return false;
  return A.kind === B.kind && A.dir !== B.dir;
}
function m2CablePath(a, b) {
  const dx = Math.max(50, Math.min(220, Math.abs(b.x - a.x) * 0.55));
  return `M ${a.x},${a.y} C ${a.x + dx},${a.y} ${b.x - dx},${b.y} ${b.x},${b.y}`;
}

const M2_THEMES = [
  { id: "midnight",   name: "Midnight",   accent: "#00d9ff", bg: "#1a1a2e" },
  { id: "deep-ocean", name: "Deep Ocean", accent: "#00c8ff", bg: "#0a1628" },
  { id: "synthwave",  name: "Synthwave",  accent: "#ff2bff", bg: "#0d0221" },
  { id: "forest",     name: "Forest",     accent: "#66cc66", bg: "#1a2f1a" },
  { id: "daylight",   name: "Daylight",   accent: "#0077cc", bg: "#e8e8e8" },
];

Object.assign(window, {
  M2, M2_CAT, M2_CATALOG, M2_THEMES,
  m2Uid, m2InitialGraph, m2MakeNode, m2Pin, m2PortPos, m2Compatible, m2CablePath, stereoPins,
});

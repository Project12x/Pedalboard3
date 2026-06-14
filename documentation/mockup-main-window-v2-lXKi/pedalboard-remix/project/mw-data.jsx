/* Main Window — data model, catalog, geometry helpers.
   Exported to window for the other Babel scripts. */

const MW_HEADER = 36;
const MW_PORT_Y = 18;          // ports sit at header vertical-center
const MW_NODE_W = 188;
const MW_PUCK_W = 72;

/* fixed category identity colours (read on any theme) */
const MW_CAT = {
  io:    { color: "#7dd3fc", label: "I/O" },
  dyn:   { color: "#4ade80", label: "Dynamics" },
  drive: { color: "#fbbf24", label: "Drive" },
  amp:   { color: "#fb7185", label: "Amp" },
  mod:   { color: "#f472d6", label: "Modulation" },
  delay: { color: "#818cf8", label: "Delay" },
  reverb:{ color: "#22d3ee", label: "Reverb" },
  eq:    { color: "#2dd4bf", label: "EQ / Filter" },
};

/* processor catalog used by the Add menu */
const MW_CATALOG = [
  { cat: "dyn",    name: "Compressor",   tone: "Opto Smooth",   cpu: 1.2, params: [["Thr","-18"],["Ratio","4:1"],["Gain","+3"]] },
  { cat: "eq",     name: "Parametric EQ",tone: "Tilt Bright",   cpu: 1.6, params: [["Low","+2"],["Mid","-3"],["High","+4"]] },
  { cat: "drive",  name: "TS808",        tone: "Green Screamer", cpu: 0.8, params: [["Drive","62"],["Tone","48"],["Level","70"]] },
  { cat: "drive",  name: "Fuzz Face",    tone: "Germanium",     cpu: 0.9, params: [["Fuzz","80"],["Vol","65"]] },
  { cat: "amp",    name: "Amp · JCM",    tone: "Crunch Lead",   cpu: 4.6, params: [["Gain","72"],["Mid","58"],["Master","64"]] },
  { cat: "mod",    name: "LFO",          tone: "Sine · 1.2 Hz", cpu: 0.2, params: [["Rate","28"],["Depth","45"]], modOut: true },
  { cat: "mod",    name: "Chorus",       tone: "Lush Stereo",   cpu: 1.1, params: [["Rate","32"],["Depth","50"],["Mix","40"]] },
  { cat: "delay",  name: "Tape Delay",   tone: "Vintage 60s",   cpu: 2.1, params: [["Time","380"],["Fbk","42"],["Mix","35"]], modIn: "Time" },
  { cat: "reverb", name: "Hall",         tone: "Cathedral 40%", cpu: 3.8, params: [["Decay","58"],["Size","70"],["Mix","40"]] },
];

let _mwId = 100;
const mwUid = (p) => (p || "n") + "_" + (++_mwId);

/* build a node from a catalog entry */
function mwMakeNode(entry, x, y) {
  const ports = { in: true, out: true, pin: !!entry.modIn, pout: !!entry.modOut };
  // LFO has no audio path
  if (entry.modOut) { ports.in = false; ports.out = false; }
  return {
    id: mwUid(entry.cat),
    kind: "proc",
    cat: entry.cat,
    name: entry.name,
    tone: entry.tone,
    cpu: entry.cpu,
    x, y, w: MW_NODE_W,
    bypass: false, solo: false, mute: false,
    params: entry.params.map(([label, disp], i) => ({ label, disp, val: [0.62, 0.48, 0.7, 0.55][i % 4] })),
    ports,
    modIn: entry.modIn || null,
  };
}

/* ---- initial patch: Riptide · Drive Lead ---- */
function mwInitialGraph() {
  const N = (cat, name, tone, cpu, x, y, params, extra) => ({
    id: mwUid(cat), kind: "proc", cat, name, tone, cpu, x, y, w: MW_NODE_W,
    bypass: false, solo: false, mute: false,
    params: params.map(([label, disp], i) => ({ label, disp, val: [0.6, 0.46, 0.68, 0.52][i % 4] })),
    ports: { in: true, out: true, pin: false, pout: false, ...(extra && extra.ports) },
    modIn: (extra && extra.modIn) || null,
  });

  const input  = { id: "io_in",  kind: "io", io: "in",  cat: "io", name: "Audio In",  x: 24,   y: 300, w: MW_PUCK_W, ports: { in: false, out: true } };
  const comp   = N("dyn",   "Compressor", "Opto Smooth",  1.2, 132, 286, [["Thr","-18"],["Ratio","4:1"],["Gain","+3"]]);
  const ts     = N("drive", "TS808",      "Green Screamer",0.8, 342, 286, [["Drive","62"],["Tone","48"],["Level","70"]]);
  const amp    = N("amp",   "Amp · JCM",  "Crunch Lead",  4.6, 552, 286, [["Gain","72"],["Mid","58"],["Master","64"]]);
  const delay  = N("delay", "Tape Delay", "Vintage 60s",  2.1, 762, 286, [["Time","380"],["Fbk","42"],["Mix","35"]], { ports: { pin: true }, modIn: "Time" });
  const hall   = N("reverb","Hall",       "Cathedral 40%",3.8, 972, 286, [["Decay","58"],["Size","70"],["Mix","40"]]);
  const output = { id: "io_out", kind: "io", io: "out", cat: "io", name: "Audio Out", x: 1184, y: 300, w: MW_PUCK_W, ports: { in: true, out: false } };
  const lfo    = N("mod",   "LFO",        "Sine · 1.2 Hz",0.2, 762, 96,  [["Rate","28"],["Depth","45"]], { ports: { in: false, out: false, pout: true } });

  const nodes = [input, comp, ts, amp, delay, hall, output, lfo];

  const C = (from, fp, to, tp, kind) => ({ id: mwUid("c"), from: { node: from, port: fp }, to: { node: to, port: tp }, kind: kind || "audio" });
  const conns = [
    C(input.id, "out", comp.id, "in"),
    C(comp.id, "out", ts.id, "in"),
    C(ts.id, "out", amp.id, "in"),
    C(amp.id, "out", delay.id, "in"),
    C(delay.id, "out", hall.id, "in"),
    C(hall.id, "out", output.id, "in"),
    C(lfo.id, "pout", delay.id, "pin", "param"),
  ];
  return { nodes, conns };
}

/* ---- port metadata + world position ---- */
function mwPortMeta(portId) {
  switch (portId) {
    case "in":   return { dir: "in",  kind: "audio", side: "l" };
    case "out":  return { dir: "out", kind: "audio", side: "r" };
    case "pin":  return { dir: "in",  kind: "param", side: "t" };
    case "pout": return { dir: "out", kind: "param", side: "r" };
    default:     return { dir: "out", kind: "audio", side: "r" };
  }
}
function mwPortPos(node, portId) {
  const w = node.w || MW_NODE_W;
  const m = mwPortMeta(portId);
  if (m.side === "l") return { x: node.x, y: node.y + MW_PORT_Y };
  if (m.side === "t") return { x: node.x + w / 2, y: node.y };
  return { x: node.x + w, y: node.y + MW_PORT_Y }; // right
}
function mwCompatible(aId, bId) {
  const a = mwPortMeta(aId), b = mwPortMeta(bId);
  return a.kind === b.kind && a.dir !== b.dir;
}

/* ---- cable path generator ---- */
function mwCablePath(a, b, style, fromSide, toSide) {
  if (style === "angular") {
    const mx = (a.x + b.x) / 2;
    if (toSide === "t") {
      const my = b.y - 28;
      return `M ${a.x},${a.y} L ${a.x + 22},${a.y} L ${a.x + 22},${my} L ${b.x},${my} L ${b.x},${b.y}`;
    }
    return `M ${a.x},${a.y} L ${mx},${a.y} L ${mx},${b.y} L ${b.x},${b.y}`;
  }
  // curved (bezier)
  if (toSide === "t") {
    const dy = Math.max(40, Math.abs(b.y - a.y) * 0.6);
    return `M ${a.x},${a.y} C ${a.x + 60},${a.y} ${b.x},${b.y - dy} ${b.x},${b.y}`;
  }
  const dx = Math.min(160, Math.max(48, Math.abs(b.x - a.x) * 0.5));
  return `M ${a.x},${a.y} C ${a.x + dx},${a.y} ${b.x - dx},${b.y} ${b.x},${b.y}`;
}

const MW_THEMES = [
  { id: "midnight",   name: "Midnight",   accent: "#00d9ff", bg: "#1a1a2e" },
  { id: "synthwave",  name: "Synthwave",  accent: "#ff2bff", bg: "#0d0221" },
  { id: "deep-ocean", name: "Deep Ocean", accent: "#00c8ff", bg: "#0a1628" },
  { id: "forest",     name: "Forest",     accent: "#66cc66", bg: "#1a2f1a" },
  { id: "daylight",   name: "Daylight",   accent: "#0077cc", bg: "#e8e8e8" },
];

Object.assign(window, {
  MW_HEADER, MW_PORT_Y, MW_NODE_W, MW_PUCK_W, MW_CAT, MW_CATALOG, MW_THEMES,
  mwUid, mwMakeNode, mwInitialGraph, mwPortMeta, mwPortPos, mwCompatible, mwCablePath,
});

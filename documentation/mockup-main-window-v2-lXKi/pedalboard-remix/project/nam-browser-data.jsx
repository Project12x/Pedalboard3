/* NAM Browser — catalog data. Community-style NAM captures (amps, pedals,
   full rigs, outboard) and cabinet impulse responses. Shape mirrors what a
   ToneHunt-style browser exposes: author, gear type, tone tag, architecture,
   popularity, rating, file size. */

const NB_MODELS = [
  { id: "m1",  name: "'65 Twin Reverb",      maker: "Fender",  author: "tonejunkie",   type: "amp",  tone: "Clean",   arch: "WaveNet", dl: 18420, rating: 4.9, size: "12.1 MB", note: "Blackface clean, bright + normal channels jumpered." },
  { id: "m2",  name: "AC30 Top Boost",       maker: "Vox",     author: "chime_lab",    type: "amp",  tone: "Crunch",  arch: "WaveNet", dl: 14110, rating: 4.8, size: "11.7 MB", note: "Edge-of-breakup with the cut control at noon." },
  { id: "m3",  name: "JCM800 2203",          maker: "Marshall", author: "plexihead",   type: "amp",  tone: "Crunch",  arch: "LSTM",    dl: 26980, rating: 4.9, size: "9.8 MB",  note: "Master volume cranked, classic British crunch." },
  { id: "m4",  name: "Dual Rectifier",       maker: "Mesa",    author: "djentleman",   type: "amp",  tone: "Hi-Gain", arch: "WaveNet", dl: 31240, rating: 4.7, size: "13.4 MB", note: "Modern red channel, scooped and tight." },
  { id: "m5",  name: "SLO-100 Overdrive",    maker: "Soldano", author: "leadtone",     type: "amp",  tone: "Lead",    arch: "WaveNet", dl: 12750, rating: 4.9, size: "12.9 MB", note: "Singing sustain, depth switch engaged." },
  { id: "m6",  name: "BE-100 Friedman",      maker: "Friedman", author: "brownsound",  type: "amp",  tone: "Hi-Gain", arch: "WaveNet", dl: 21660, rating: 4.8, size: "12.6 MB", note: "BE channel, tight low end, sat 100%." },
  { id: "m7",  name: "Overdrive Special",    maker: "Dumble",  author: "smoothD",      type: "amp",  tone: "Lead",    arch: "WaveNet", dl: 9840,  rating: 5.0, size: "13.1 MB", note: "HRM-spec, rock mode, mid boost in." },
  { id: "m8",  name: "5150 Block Letter",    maker: "Peavey",  author: "metalcore_io", type: "amp",  tone: "Hi-Gain", arch: "LSTM",    dl: 28510, rating: 4.7, size: "10.2 MB", note: "Crunch channel pushed for rhythm chug." },
  { id: "m9",  name: "Tweed Deluxe 5E3",     maker: "Fender",  author: "vintagecap",   type: "amp",  tone: "Crunch",  arch: "WaveNet", dl: 8230,  rating: 4.8, size: "11.9 MB", note: "Volume at 7, that woolly tweed grind." },
  { id: "m10", name: "Klon Centaur",         maker: "Klon",    author: "goldhorse",    type: "pedal", tone: "Crunch", arch: "WaveNet", dl: 33890, rating: 4.9, size: "6.4 MB",  note: "Gain at 9 o'clock, transparent push." },
  { id: "m11", name: "TS808 Tube Screamer",  maker: "Ibanez",  author: "greenmachine", type: "pedal", tone: "Crunch", arch: "WaveNet", dl: 30120, rating: 4.8, size: "5.9 MB",  note: "Mid-hump boost stacked into a clean amp." },
  { id: "m12", name: "RAT Distortion",       maker: "ProCo",   author: "rodentking",   type: "pedal", tone: "Hi-Gain", arch: "LSTM",   dl: 17440, rating: 4.6, size: "5.2 MB",  note: "Filter rolled back, gritty lead voice." },
  { id: "m13", name: "Big Muff Pi",          maker: "EHX",     author: "fuzzlord",     type: "pedal", tone: "Hi-Gain", arch: "WaveNet", dl: 15980, rating: 4.7, size: "6.1 MB", note: "Triangle-era violin sustain fuzz." },
  { id: "m14", name: "Twin → 808 → Verb",    maker: "Full Rig", author: "studio_rat",  type: "rig",  tone: "Clean",   arch: "WaveNet", dl: 6720,  rating: 4.9, size: "18.7 MB", note: "Captured chain: screamer, amp, plate." },
  { id: "m15", name: "Recto Stack + OD",     maker: "Full Rig", author: "djentleman",  type: "rig",  tone: "Hi-Gain", arch: "WaveNet", dl: 9410,  rating: 4.8, size: "19.2 MB", note: "Tubescreamer into red channel, 4x12." },
  { id: "m16", name: "LA-2A Leveler",        maker: "Teletronix", author: "mixbus",    type: "out",  tone: "Clean",   arch: "WaveNet", dl: 5130,  rating: 4.9, size: "7.8 MB",  note: "Optical compression, gentle leveling." },
];

const NB_IRS = [
  { id: "i1", name: "4×12 Greenback",  maker: "Celestion", author: "ir_forge",   type: "ir", tone: "Crunch",  arch: "48kHz", dl: 22140, rating: 4.9, size: "512 KB", note: "G12M, SM57 cap-edge + R121 blend." },
  { id: "i2", name: "4×12 V30",        maker: "Celestion", author: "ir_forge",   type: "ir", tone: "Hi-Gain", arch: "48kHz", dl: 27600, rating: 4.8, size: "512 KB", note: "Vintage 30, tight modern metal cab." },
  { id: "i3", name: "2×12 Blue Alnico", maker: "Vox",      author: "chime_lab",  type: "ir", tone: "Clean",   arch: "48kHz", dl: 11280, rating: 4.9, size: "512 KB", note: "AlNiCo Blue, shimmer and chime." },
  { id: "i4", name: "1×12 Tweed",      maker: "Jensen",    author: "vintagecap",  type: "ir", tone: "Crunch",  arch: "48kHz", dl: 7640,  rating: 4.7, size: "512 KB", note: "P12N, woody single-speaker thump." },
  { id: "i5", name: "4×10 Bassman",    maker: "Fender",    author: "lowend_lab",  type: "ir", tone: "Clean",   arch: "48kHz", dl: 9120,  rating: 4.8, size: "512 KB", note: "P10R quad, classic bass + gtr cab." },
  { id: "i6", name: "1×12 Blackface",  maker: "Fender",    author: "studio_rat",  type: "ir", tone: "Clean",   arch: "48kHz", dl: 13050, rating: 4.9, size: "512 KB", note: "C12K, deluxe-style scooped clean." },
  { id: "i7", name: "4×12 Recto OS",   maker: "Mesa",      author: "djentleman",  type: "ir", tone: "Hi-Gain", arch: "48kHz", dl: 19870, rating: 4.8, size: "512 KB", note: "Oversized cab, V30s, dual-mic." },
  { id: "i8", name: "2×12 Matchless",  maker: "Matchless", author: "boutique_ir", type: "ir", tone: "Crunch",  arch: "48kHz", dl: 6310,  rating: 4.9, size: "512 KB", note: "DC30 cab, lush British midrange." },
];

const NB_TYPE_FILTERS = [
  { id: "all",   label: "All" },
  { id: "amp",   label: "Amps" },
  { id: "pedal", label: "Pedals" },
  { id: "rig",   label: "Full Rigs" },
  { id: "out",   label: "Outboard" },
];
const NB_IR_TYPE_FILTERS = [
  { id: "all", label: "All Cabs" },
  { id: "ir",  label: "Cabinets" },
];
const NB_TONES = ["Clean", "Crunch", "Hi-Gain", "Lead"];
const NB_SORTS = ["Popular", "Top Rated", "Newest", "Name A–Z"];

function nbFmtDl(n) { return n >= 1000 ? (n / 1000).toFixed(1).replace(/\.0$/, "") + "k" : String(n); }

function nbFilterSort(items, { type, tones, query, sort }) {
  let out = items.filter((m) => {
    if (type && type !== "all" && m.type !== type) return false;
    if (tones && tones.length && !tones.includes(m.tone)) return false;
    if (query) {
      const q = query.toLowerCase();
      if (!(m.name.toLowerCase().includes(q) || m.maker.toLowerCase().includes(q) || m.author.toLowerCase().includes(q))) return false;
    }
    return true;
  });
  const by = {
    "Popular": (a, b) => b.dl - a.dl,
    "Top Rated": (a, b) => b.rating - a.rating || b.dl - a.dl,
    "Newest": (a, b) => a.id < b.id ? 1 : -1,
    "Name A–Z": (a, b) => a.name.localeCompare(b.name),
  }[sort] || (() => 0);
  return out.slice().sort(by);
}

Object.assign(window, {
  NB_MODELS, NB_IRS, NB_TYPE_FILTERS, NB_IR_TYPE_FILTERS, NB_TONES, NB_SORTS,
  nbFmtDl, nbFilterSort,
});

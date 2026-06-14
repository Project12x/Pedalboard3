/* Shared stage data, icons, and primitives — exported to window.
   Unique names to avoid Babel global-scope collisions. */

/* One set of 8 patches drives every view. Each carries its own chain,
   key, tempo and tuner reading so switching patches feels live. */
const PB_SET = [
  { n: "01", song: "Opener",      tone: "Ambient Swell",  hue: "cyan",    key: "A Major", bpm: 72,  note: "A", cents: 2,  chain: ["Comp", "Shimmer", "Hall 70%"] },
  { n: "02", song: "Golden Hour", tone: "Clean Verse",    hue: "indigo",  key: "D Major", bpm: 96,  note: "D", cents: -1, chain: ["Comp", "Chorus", "Plate 25%"] },
  { n: "03", song: "Golden Hour", tone: "Big Chorus",     hue: "indigo",  key: "D Major", bpm: 96,  note: "D", cents: 0,  chain: ["Comp", "Drive", "Chorus", "Hall 40%"] },
  { n: "04", song: "Riptide",     tone: "Drive Lead",     hue: "amber",   key: "E Minor", bpm: 128, note: "E", cents: -4, chain: ["Comp", "TS808", "Amp · JCM", "Tape Delay", "Hall 40%"] },
  { n: "05", song: "Riptide",     tone: "High-Gain Solo", hue: "amber",   key: "E Minor", bpm: 128, note: "E", cents: 6,  chain: ["TS808", "Amp · JCM", "Octave", "Tape Delay"] },
  { n: "06", song: "Wildfire",    tone: "Crunch Rhythm",  hue: "cyan",    key: "G Major", bpm: 140, note: "G", cents: -2, chain: ["Comp", "Crunch", "Room 15%"] },
  { n: "07", song: "Closer",      tone: "Acoustic Sim",   hue: "green",   key: "C Major", bpm: 84,  note: "C", cents: 1,  chain: ["Acoustic IR", "Comp", "Plate 30%"] },
  { n: "08", song: "Encore",      tone: "Octave Fuzz",    hue: "magenta", key: "E Minor", bpm: 128, note: "E", cents: 12, chain: ["Fuzz", "Octave", "Delay", "Verb"] },
];

const PbIcon = {
  tuner: (p) => (
    <svg viewBox="0 0 24 24" fill="none" className="ic" {...p}>
      <path d="M12 3v6M12 3l3 2M12 3l-3 2" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" />
      <path d="M5 14a7 7 0 0 0 14 0" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" />
      <path d="M12 14v6" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" />
    </svg>
  ),
  exit: (p) => (
    <svg viewBox="0 0 24 24" fill="none" className="ic" {...p}>
      <path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" />
      <path d="M16 17l5-5-5-5M21 12H9" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  ),
  power: (p) => (
    <svg viewBox="0 0 24 24" fill="none" className="ic" {...p}>
      <path d="M12 3v9" stroke="currentColor" strokeWidth="2" strokeLinecap="round" />
      <path d="M7.2 6.5a8 8 0 1 0 9.6 0" stroke="currentColor" strokeWidth="2" strokeLinecap="round" />
    </svg>
  ),
  hero: (p) => (
    <svg viewBox="0 0 24 24" fill="none" className="ic" {...p}>
      <rect x="3" y="5" width="18" height="14" rx="2" stroke="currentColor" strokeWidth="1.7" />
      <path d="M7 12h10" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" />
    </svg>
  ),
  list: (p) => (
    <svg viewBox="0 0 24 24" fill="none" className="ic" {...p}>
      <path d="M8 6h13M8 12h13M8 18h13" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" />
      <circle cx="3.5" cy="6" r="1.4" fill="currentColor" />
      <circle cx="3.5" cy="12" r="1.4" fill="currentColor" />
      <circle cx="3.5" cy="18" r="1.4" fill="currentColor" />
    </svg>
  ),
  grid: (p) => (
    <svg viewBox="0 0 24 24" fill="none" className="ic" {...p}>
      <rect x="3" y="3" width="7.5" height="7.5" rx="1.5" stroke="currentColor" strokeWidth="1.7" />
      <rect x="13.5" y="3" width="7.5" height="7.5" rx="1.5" stroke="currentColor" strokeWidth="1.7" />
      <rect x="3" y="13.5" width="7.5" height="7.5" rx="1.5" stroke="currentColor" strokeWidth="1.7" />
      <rect x="13.5" y="13.5" width="7.5" height="7.5" rx="1.5" stroke="currentColor" strokeWidth="1.7" />
    </svg>
  ),
};

/* tuning colour helper (mirrors StageView::getTuningColour) */
function pbTuneCol(cents) {
  const a = Math.abs(cents);
  return a < 5 ? "var(--success)" : a < 15 ? "var(--warning)" : "var(--danger)";
}

/* ---- Stereo VU meter pair (animated idle wobble for the mock) ---- */
function PbVu({ label, base = [58, 52], width = 150 }) {
  const [lv, setLv] = React.useState(base);
  React.useEffect(() => {
    let raf, t = 0;
    const tick = () => {
      t += 0.05;
      setLv(base.map((b, i) => Math.max(4, Math.min(98, b + Math.sin(t * (1.3 + i * 0.4)) * 9 + Math.sin(t * 5.1) * 4))));
      raf = requestAnimationFrame(tick);
    };
    raf = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(raf);
  }, [base[0], base[1]]);
  return (
    <div className="vu">
      <span className="lab">{label}</span>
      <div className="stack">
        {lv.map((v, i) => (
          <div className="bar" key={i} style={{ width }}>
            <div className="fill" style={{ width: v + "%" }}></div>
            <div className="peak" style={{ left: Math.min(99, v + 8) + "%" }}></div>
          </div>
        ))}
      </div>
    </div>
  );
}

/* ---- Horizontal strobe tuner: size = 'sm' | 'md' | 'big' ---- */
function PbStrobe({ note = "E", cents = -4, size = "md" }) {
  const ticks = [-50, -40, -30, -20, -10, 0, 10, 20, 30, 40, 50];
  const col = pbTuneCol(cents);
  const pos = 50 + (cents / 50) * 50;
  return (
    <div className={"strobe " + size}>
      <div className="strobe-note" style={{ color: col }}>{note}</div>
      <div className="strobe-track">
        <div className="strobe-ticks">
          {ticks.map((t) => <span key={t} className={"tk" + (t === 0 ? " mid" : "")}></span>)}
        </div>
        <div className="strobe-needle" style={{ left: pos + "%", background: col, boxShadow: "0 0 14px " + col }}></div>
      </div>
      <div className="strobe-cents" style={{ color: col }}>
        {cents >= 0 ? "+" : ""}{cents}<span>cents</span>
      </div>
    </div>
  );
}

/* ---- shared signal-chain flow ---- */
function PbFlow({ chain }) {
  return (
    <div className="flow">
      {chain.map((node, i) => (
        <React.Fragment key={i}>
          <span className="node">{node}</span>
          {i < chain.length - 1 && <span className="arr">→</span>}
        </React.Fragment>
      ))}
    </div>
  );
}

Object.assign(window, { PB_SET, PbIcon, PbVu, PbStrobe, PbFlow, pbTuneCol });

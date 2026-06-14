/* Main Window v2 — visual primitives: icons, knob, faders, VU, tuner. */

const M2Icon = {
  mic:    (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><rect x="9" y="3" width="6" height="11" rx="3" stroke="currentColor" strokeWidth="1.7"/><path d="M5.5 11a6.5 6.5 0 0 0 13 0M12 17.5V21M8.5 21h7" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round"/></svg>),
  piano:  (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><rect x="3" y="5" width="18" height="14" rx="2" stroke="currentColor" strokeWidth="1.6"/><path d="M9 5v9M15 5v9M3 14h18" stroke="currentColor" strokeWidth="1.4"/></svg>),
  midi:   (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><circle cx="12" cy="12" r="9" stroke="currentColor" strokeWidth="1.6"/><circle cx="12" cy="7" r="1.3" fill="currentColor"/><circle cx="7.5" cy="10" r="1.3" fill="currentColor"/><circle cx="16.5" cy="10" r="1.3" fill="currentColor"/><circle cx="9" cy="15" r="1.3" fill="currentColor"/><circle cx="15" cy="15" r="1.3" fill="currentColor"/></svg>),
  osc:    (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M3 12c2 0 2-6 4.5-6S10 18 12 18s2-12 4.5-12S19 12 21 12" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round"/></svg>),
  speaker:(p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M4 9v6h4l5 4V5L8 9H4z" stroke="currentColor" strokeWidth="1.6" strokeLinejoin="round"/><path d="M16.5 8.5a5 5 0 0 1 0 7M19 6a8 8 0 0 1 0 12" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round"/></svg>),
  stage:  (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M4 18a8 8 0 0 1 16 0" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round"/><path d="M12 14V5M12 5l3 2M12 5 9 7" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round"/></svg>),
  play:   (p) => (<svg viewBox="0 0 24 24" {...p}><path d="M7 5l12 7-12 7z" fill="currentColor"/></svg>),
  rtz:    (p) => (<svg viewBox="0 0 24 24" {...p}><path d="M18 5l-9 7 9 7z" fill="currentColor"/><rect x="5" y="5" width="2.4" height="14" rx="1" fill="currentColor"/></svg>),
  rec:    (p) => (<svg viewBox="0 0 24 24" {...p}><circle cx="12" cy="12" r="6" fill="currentColor"/></svg>),
  chev:   (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M6 9l6 6 6-6" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"/></svg>),
  close:  (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M6 6l12 12M18 6 6 18" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round"/></svg>),
  min:    (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M6 12h12" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round"/></svg>),
  max:    (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><rect x="6" y="6" width="12" height="12" rx="1.5" stroke="currentColor" strokeWidth="1.6"/></svg>),
};

/* Rotary knob — 270° sweep, bezelled cap, glowing pointer tip */
function M2Knob({ label, disp, val = 0.5, size = 42, color }) {
  const v = Math.max(0, Math.min(1, val));
  const r = 15, c = 2 * Math.PI * r, sweep = 0.75;
  const arc = v * sweep * c;
  const ang = 135 + v * 270;
  const rad = (ang - 90) * Math.PI / 180;
  const col = color || "var(--cat)";
  const tipX = 21 + 7.4 * Math.cos(rad), tipY = 21 + 7.4 * Math.sin(rad);
  return (
    <div className="m2-knob" style={{ width: size }}>
      <svg width={size} height={size} viewBox="0 0 42 42">
        <g transform="rotate(135 21 21)">
          <circle className="kt" cx="21" cy="21" r={r} fill="none" strokeWidth="3" strokeLinecap="round" strokeDasharray={`${sweep * c} ${c}`} />
          <circle cx="21" cy="21" r={r} fill="none" strokeWidth="3" strokeLinecap="round" stroke={col} strokeDasharray={`${arc} ${c}`} style={{ filter: "drop-shadow(0 0 2.5px " + col + ")" }} />
        </g>
        <circle cx="21" cy="21" r="10.6" fill="var(--border)" opacity="0.55" />
        <circle cx="21" cy="21" r="9.6" fill="var(--knob-face)" stroke="var(--border)" strokeWidth="0.75" />
        <circle cx="21" cy="17.5" r="6" fill="color-mix(in srgb, var(--text) 7%, transparent)" />
        <line x1={21 + 3.2 * Math.cos(rad)} y1={21 + 3.2 * Math.sin(rad)} x2={tipX} y2={tipY} stroke={col} strokeWidth="2.1" strokeLinecap="round" />
        <circle cx={tipX} cy={tipY} r="1.3" fill={col} />
      </svg>
      {disp != null && <span className="kv">{disp}</span>}
      <span className="kl">{label}</span>
    </div>
  );
}

/* Horizontal level fader */
function M2Fader({ label, db, val = 0.62 }) {
  return (
    <div className="m2-fader">
      <span className="fl">{label}</span>
      <div className="ftrack"><i style={{ width: (val * 100) + "%" }}></i><span className="fthumb" style={{ left: (val * 100) + "%" }}></span></div>
      <span className="fv">{db} dB</span>
    </div>
  );
}

/* Segmented output channel meter */
function M2ChannelMeter({ label, db }) {
  const v = Math.max(0.04, Math.min(1, 1 + parseFloat(db) / 48));
  return (
    <div className="m2-chmeter">
      <span className="cl">{label}</span>
      <div className="ctrack"><i style={{ width: (v * 100) + "%" }}></i></div>
      <span className="cv">{db} dB</span>
    </div>
  );
}

/* Vertical VU bars */
function M2VU() {
  const ticks = ["0", "-6", "-12", "-24", "-48"];
  return (
    <div className="m2-vu">
      <div className="vu-scale">{ticks.map((t) => <span key={t}>{t}</span>)}</div>
      <div className="vu-bars">
        {[0.82, 0.64].map((h, i) => (
          <div className="vu-col" key={i}><i style={{ "--h": h, animationDelay: (i * 0.14) + "s" }}></i></div>
        ))}
      </div>
    </div>
  );
}

/* Perfect tuner — chromatic, strobe lock + coarse approach + IN TUNE
   The strobe band encodes pitch error as MOTION: it scrolls (flat ◄ / ► sharp)
   at a speed proportional to how far off you are, and freezes when in tune. */
const TUNER_NOTES = [
  { name: "E", acc: "",  oct: 2, freq: 82.4 },
  { name: "A", acc: "",  oct: 2, freq: 110.0 },
  { name: "C", acc: "♯", oct: 3, freq: 138.6 },
  { name: "F", acc: "♯", oct: 3, freq: 185.0 },
  { name: "B", acc: "♭", oct: 3, freq: 233.1 },
  { name: "A", acc: "♭", oct: 4, freq: 415.3 },
];
function TunerChromatic() {
  const [cents, setCents] = React.useState(-22);
  const [ni, setNi] = React.useState(1);
  const note = TUNER_NOTES[ni];

  const centsRef = React.useRef(cents); centsRef.current = cents;
  const phaseRef = React.useRef(0);
  const stripRef = React.useRef(null);

  // strobe: advance phase ∝ cents each frame → freezes near 0
  React.useEffect(() => {
    let raf, last = performance.now();
    const tick = (now) => {
      const dt = Math.min(48, now - last); last = now;
      phaseRef.current += (centsRef.current / 50) * dt * 0.62;
      if (stripRef.current) stripRef.current.style.backgroundPositionX = phaseRef.current.toFixed(1) + "px";
      raf = requestAnimationFrame(tick);
    };
    raf = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(raf);
  }, []);

  // pitch drifts toward centre
  React.useEffect(() => {
    const id = setInterval(() => {
      setCents((c) => Math.max(-50, Math.min(50, c * 0.9 + (Math.random() - 0.5) * 2.4)));
    }, 90);
    return () => clearInterval(id);
  }, []);

  // chromatic: re-acquire a new note every few seconds
  React.useEffect(() => {
    const id = setInterval(() => {
      setNi((i) => (i + 1) % TUNER_NOTES.length);
      setCents((Math.random() < 0.5 ? -1 : 1) * (16 + Math.random() * 26));
    }, 4200);
    return () => clearInterval(id);
  }, []);

  const inTune = Math.abs(cents) < 3;
  const close  = Math.abs(cents) < 14;
  const cls = inTune ? " in-tune" : close ? " close" : "";
  const sign = cents >= 0 ? "+" : "";

  return (
    <>
      <div className="tn-readout">
        <span className={"tn-flatdir" + (!inTune && cents < 0 ? " act" : "")}>♭</span>
        <span className={"tn-big-note" + cls}>{note.name}<span className="tn-acc">{note.acc}</span><span className="tn-oct">{note.oct}</span></span>
        <span className={"tn-sharpdir" + (!inTune && cents > 0 ? " act" : "")}>♯</span>
      </div>
      <div className="tn-freq2">{note.freq.toFixed(1)} Hz</div>

      {/* coarse approach */}
      <div className="tn-coarse">
        <div className="tn-coarse-track">
          <span className="tn-coarse-center"></span>
          <span className={"tn-coarse-dot" + cls} style={{ left: (50 + cents) + "%" }}></span>
        </div>
      </div>

      {/* strobe lock */}
      <div className={"tn-strobe" + (inTune ? " lock" : "")}>
        <div className="tn-strobe-band" ref={stripRef}></div>
        <span className="tn-strobe-center"></span>
      </div>

      <div className="tn-foot">
        <span className={"tn-status-badge" + cls}>
          <span className="tn-status-dot"></span>
          {inTune ? "In Tune" : sign + Math.round(cents) + "¢"}
        </span>
        <span className="tn-ref">A=440 · Strobe</span>
      </div>
    </>
  );
}

/* Needle mode — enhanced recreation of the app's analog TunerControl needle
   meter: color-zoned arc, smoothed needle with tip glow + gradient pivot,
   an 11-segment LED ladder, and a frequency / cents readout. */
function TunerNeedle() {
  const [cents, setCents] = React.useState(-14);
  const [ni, setNi] = React.useState(1);
  const note = TUNER_NOTES[ni];
  React.useEffect(() => {
    const id = setInterval(() => setCents((c) => Math.max(-50, Math.min(50, c * 0.9 + (Math.random() - 0.5) * 2.6))), 90);
    return () => clearInterval(id);
  }, []);
  React.useEffect(() => {
    const id = setInterval(() => { setNi((i) => (i + 1) % TUNER_NOTES.length); setCents((Math.random() < 0.5 ? -1 : 1) * (14 + Math.random() * 28)); }, 4200);
    return () => clearInterval(id);
  }, []);

  const inTune = Math.abs(cents) < 3;
  const close  = Math.abs(cents) < 15;
  const cls = inTune ? " in-tune" : close ? " close" : "";
  const col = inTune ? "var(--tuner)" : close ? "var(--warning)" : "var(--danger)";
  const sign = cents >= 0 ? "+" : "";

  const cx = 100, py = 90, R = 74;
  const aa = (deg) => deg * Math.PI / 180;
  const ticks = [];
  for (let i = -5; i <= 5; i++) {
    const a = aa(-90 + i * 10), isC = i === 0;
    const inner = isC ? R - 13 : R - 7, outer = R + 2;
    const tc = isC ? "var(--t85)" : Math.abs(i) <= 1 ? "var(--tuner)" : Math.abs(i) <= 2 ? "var(--warning)" : "var(--danger)";
    ticks.push(<line key={i} x1={cx + Math.cos(a) * inner} y1={py + Math.sin(a) * inner} x2={cx + Math.cos(a) * outer} y2={py + Math.sin(a) * outer}
      stroke={tc} strokeWidth={isC ? 2.6 : 1.8} strokeLinecap="round" opacity={Math.abs(i) <= 1 ? 0.95 : 0.6} />);
  }
  const arcR = R - 2, arcPts = [];
  for (let d = -140; d <= -40; d += 5) arcPts.push(`${(cx + Math.cos(aa(d)) * arcR).toFixed(1)},${(py + Math.sin(aa(d)) * arcR).toFixed(1)}`);
  const arcD = "M" + arcPts.join(" L");
  const na = aa(-90 + Math.max(-50, Math.min(50, cents))), L = R - 5;
  const tx = cx + Math.cos(na) * L, ty = py + Math.sin(na) * L;

  return (
    <div className="tn-needle-view">
      <div className="tn-readout">
        <span className={"tn-flatdir" + (!inTune && cents < 0 ? " act" : "")}>♭</span>
        <span className={"tn-big-note" + cls}>{note.name}<span className="tn-acc">{note.acc}</span><span className="tn-oct">{note.oct}</span></span>
        <span className={"tn-sharpdir" + (!inTune && cents > 0 ? " act" : "")}>♯</span>
      </div>
      <svg className="tn-ngauge" viewBox="0 0 200 102">
        <path d={arcD} fill="none" stroke="var(--surface-2)" strokeWidth="3" strokeLinecap="round" />
        {ticks}
        <text x={cx + Math.cos(aa(-142)) * (R - 19)} y={py + Math.sin(aa(-142)) * (R - 19) + 3} className="tn-ntk" textAnchor="middle">−50</text>
        <text x={cx + Math.cos(aa(-38)) * (R - 19)} y={py + Math.sin(aa(-38)) * (R - 19) + 3} className="tn-ntk" textAnchor="middle">+50</text>
        <line x1={cx} y1={py} x2={tx} y2={ty} stroke={col} strokeWidth="3.2" strokeLinecap="round"
          style={{ transition: "all 0.09s cubic-bezier(.3,.7,0,1.3)", filter: inTune ? "drop-shadow(0 0 5px var(--tuner))" : "none" }} />
        <circle cx={tx} cy={ty} r="5" fill={col} opacity="0.3" style={{ transition: "all 0.09s" }} />
        <circle cx={tx} cy={ty} r="3" fill={col} style={{ transition: "all 0.09s" }} />
        <circle cx={cx} cy={py} r="9" fill="var(--button-hi)" stroke="var(--border)" strokeWidth="1" />
        <circle cx={cx} cy={py} r="3.5" fill="var(--surface-2)" />
      </svg>
      <div className="tn-leds">
        {Array.from({ length: 11 }).map((_, i) => {
          const v = (i - 5) * 10, isC = i === 5;
          const lit = isC ? Math.abs(cents) < 5 : Math.abs(cents - v) < 10;
          const zone = isC ? "c" : Math.abs(i - 5) <= 1 ? "g" : Math.abs(i - 5) <= 2 ? "w" : "d";
          return <span key={i} className={"tn-led z-" + zone + (lit ? " lit" : "")}></span>;
        })}
      </div>
      <div className="tn-foot">
        <span className={"tn-status-badge" + cls}><span className="tn-status-dot"></span>{note.freq.toFixed(1)} Hz · {inTune ? "In Tune" : sign + Math.round(cents) + "¢"}</span>
        <span className="tn-ref">A=440</span>
      </div>
    </div>
  );
}

/* Polyphonic 6-string view — strum all strings, see every one at once
   (Polytune-style). Each string is a vertical mini-meter: dot rides up when
   sharp, down when flat, snaps green at centre when in tune. */
const POLY_STRINGS = [
  { name: "E", oct: 2 }, { name: "A", oct: 2 }, { name: "D", oct: 3 },
  { name: "G", oct: 3 }, { name: "B", oct: 3 }, { name: "E", oct: 4 },
];
function TunerPoly() {
  const [cents, setCents] = React.useState([-19, 7, -3, 24, -11, 2]);
  React.useEffect(() => {
    const id = setInterval(() => {
      setCents((arr) => arr.map((c) => Math.max(-50, Math.min(50, c * 0.9 + (Math.random() - 0.5) * 3.2))));
    }, 110);
    return () => clearInterval(id);
  }, []);
  const allOk = cents.every((c) => Math.abs(c) < 3.5);

  return (
    <div className="tn-poly">
      <div className="tn-poly-strings">
        {POLY_STRINGS.map((s, i) => {
          const c = cents[i];
          const it = Math.abs(c) < 3.5, close = Math.abs(c) < 14;
          const cls = it ? " in-tune" : close ? " close" : "";
          const top = 50 - (c / 50) * 40;     // +50¢ → top, −50¢ → bottom
          return (
            <div className={"tn-str" + cls} key={i}>
              <span className="tn-str-state">{it ? "✓" : c > 0 ? "♯" : "♭"}</span>
              <div className="tn-str-track">
                <span className="tn-str-zone"></span>
                <span className="tn-str-dot" style={{ top: top + "%" }}></span>
              </div>
              <span className="tn-str-name">{s.name}<i>{s.oct}</i></span>
            </div>
          );
        })}
      </div>
      <div className={"tn-poly-foot" + (allOk ? " ok" : "")}>
        <span className="tn-status-dot"></span>
        {allOk ? "All strings in tune" : "Strum all strings"}
      </div>
    </div>
  );
}

function M2Tuner() {
  const [mode, setMode] = React.useState("needle");
  const stop = (e) => e.stopPropagation();
  const modes = [["needle", "Needle"], ["strobe", "Strobe"], ["poly", "Poly"]];
  return (
    <div className="m2-tuner-v2">
      <div className="tn-modeseg">
        {modes.map(([k, lab]) => (
          <button key={k} className={"tn-modeseg-b" + (mode === k ? " on" : "")} onPointerDown={stop} onClick={() => setMode(k)}>{lab}</button>
        ))}
      </div>
      {mode === "needle" ? <TunerNeedle /> : mode === "strobe" ? <TunerChromatic /> : <TunerPoly />}
    </div>
  );
}

Object.assign(window, { M2Icon, M2Knob, M2Fader, M2ChannelMeter, M2VU, M2Tuner });

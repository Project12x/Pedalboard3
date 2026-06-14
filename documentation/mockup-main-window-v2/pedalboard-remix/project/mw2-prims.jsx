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

/* Polished tuner — semicircular needle gauge, glowing note + cents */
function M2Tuner() {
  const [cents, setCents] = React.useState(-8);
  React.useEffect(() => {
    const id = setInterval(() => {
      setCents((c) => Math.max(-50, Math.min(50, c * 0.93 + (Math.random() - 0.475) * 2.6)));
    }, 90);
    return () => clearInterval(id);
  }, []);

  const inTune = Math.abs(cents) < 3.5;
  const close  = Math.abs(cents) < 16;
  const cls = inTune ? " in-tune" : close ? " close" : "";
  const sign = cents >= 0 ? "+" : "";
  const needleCol = inTune ? "var(--tuner)" : close ? "var(--warning)" : "var(--danger)";

  const cx = 100, cy = 90, rOut = 80;
  const toRad = (cc) => (90 - cc * 1.8) * Math.PI / 180;   // −50¢→180°, 0¢→90°(up), +50¢→0°
  const na = toRad(cents);
  const nx = cx + 66 * Math.cos(na), ny = cy - 66 * Math.sin(na);

  const ticks = [];
  for (let cc = -50; cc <= 50; cc += 10) {
    const a = toRad(cc);
    const rIn = cc === 0 ? 64 : 70;
    const major = cc === 0;
    const col = Math.abs(cc) < 5 ? "var(--tuner)" : Math.abs(cc) <= 20 ? "var(--warning)" : "var(--danger)";
    ticks.push(<line key={cc} x1={cx + rOut * Math.cos(a)} y1={cy - rOut * Math.sin(a)}
      x2={cx + rIn * Math.cos(a)} y2={cy - rIn * Math.sin(a)}
      stroke={col} strokeWidth={major ? 3 : 2} strokeLinecap="round" opacity={Math.abs(cc) < 5 ? 0.95 : 0.6} />);
  }

  return (
    <div className="m2-tuner-v2">
      <div className="tn-readout">
        <span className={"tn-big-note" + cls}>E</span>
        <div className="tn-meta">
          <span className="tn-octave-badge">2</span>
          <span className="tn-freq-val">82.4 Hz</span>
        </div>
      </div>

      <svg className="tn-gauge" viewBox="0 0 200 104" width="100%">
        <path d="M20 90 A80 80 0 0 1 180 90" fill="none" stroke="var(--surface-2)" strokeWidth="4" strokeLinecap="round" />
        {ticks}
        <line x1={cx} y1={cy} x2={nx} y2={ny} stroke={needleCol} strokeWidth="2.6" strokeLinecap="round"
          style={{ filter: inTune ? "drop-shadow(0 0 5px var(--tuner))" : "none", transition: "all 0.09s cubic-bezier(.3,.7,0,1.3)" }} />
        <circle cx={cx} cy={cy} r="5.5" fill={needleCol} />
        <circle cx={cx} cy={cy} r="2.5" fill="var(--node-bg)" />
        <text x="16" y="100" className="tn-tk">♭</text>
        <text x="184" y="100" className="tn-tk" textAnchor="end">♯</text>
      </svg>

      <div className={"tn-status-badge" + cls}>
        <span className="tn-status-dot"></span>
        {inTune ? "In Tune" : sign + Math.round(cents) + " cents"}
      </div>
    </div>
  );
}

Object.assign(window, { M2Icon, M2Knob, M2Fader, M2ChannelMeter, M2VU, M2Tuner });

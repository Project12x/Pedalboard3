/* Main Window — small primitives: Knob, Meter, icons. */

const MwIcon = {
  plus: (p) => (<svg viewBox="0 0 24 24" fill="none" className="ic" {...p}><path d="M12 5v14M5 12h14" stroke="currentColor" strokeWidth="2" strokeLinecap="round"/></svg>),
  save: (p) => (<svg viewBox="0 0 24 24" fill="none" className="ic" {...p}><path d="M5 4h11l3 3v13H5z" stroke="currentColor" strokeWidth="1.7" strokeLinejoin="round"/><path d="M8 4v5h7M8 14h8v6H8z" stroke="currentColor" strokeWidth="1.7" strokeLinejoin="round"/></svg>),
  gear: (p) => (<svg viewBox="0 0 24 24" fill="none" className="ic" {...p}><circle cx="12" cy="12" r="3.2" stroke="currentColor" strokeWidth="1.7"/><path d="M12 2.5v3M12 18.5v3M21.5 12h-3M5.5 12h-3M18.7 5.3l-2.1 2.1M7.4 16.6l-2.1 2.1M18.7 18.7l-2.1-2.1M7.4 7.4 5.3 5.3" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round"/></svg>),
  stage: (p) => (<svg viewBox="0 0 24 24" fill="none" className="ic" {...p}><path d="M4 18a8 8 0 0 1 16 0" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round"/><path d="M12 14v-9M12 5l3 2M12 5l-3 2" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round"/></svg>),
  jackIn: (p) => (<svg viewBox="0 0 24 24" fill="none" className="ic" {...p}><path d="M3 12h12M15 12l-4-4M15 12l-4 4" stroke="currentColor" strokeWidth="1.9" strokeLinecap="round" strokeLinejoin="round"/><rect x="17" y="6" width="4" height="12" rx="1.4" stroke="currentColor" strokeWidth="1.7"/></svg>),
  jackOut: (p) => (<svg viewBox="0 0 24 24" fill="none" className="ic" {...p}><rect x="3" y="6" width="4" height="12" rx="1.4" stroke="currentColor" strokeWidth="1.7"/><path d="M9 12h12M21 12l-4-4M21 12l-4 4" stroke="currentColor" strokeWidth="1.9" strokeLinecap="round" strokeLinejoin="round"/></svg>),
};

/* rotary knob — 270deg sweep, value arc in category colour */
function MwKnob({ label, disp, val = 0.5 }) {
  const r = 15, c = 2 * Math.PI * r, sweep = 0.75; // 270deg
  const track = sweep * c;
  const arc = Math.max(0, Math.min(1, val)) * sweep * c;
  return (
    <div className="mw-knob">
      <svg width="38" height="38" viewBox="0 0 38 38">
        <g transform="rotate(135 19 19)">
          <circle className="ktrack" cx="19" cy="19" r={r} fill="none" strokeWidth="3.5"
            strokeLinecap="round" strokeDasharray={`${track} ${c}`} />
          <circle className="karc" cx="19" cy="19" r={r} fill="none" strokeWidth="3.5"
            strokeLinecap="round" strokeDasharray={`${arc} ${c}`} />
        </g>
        <circle cx="19" cy="19" r="2" fill="currentColor" opacity="0.35" />
      </svg>
      <span className="kv">{disp}</span>
      <span className="kl">{label}</span>
    </div>
  );
}

function MwMeter() {
  return (
    <div className="mw-meter">
      {Array.from({ length: 7 }).map((_, i) => <i key={i}></i>)}
    </div>
  );
}

Object.assign(window, { MwIcon, MwKnob, MwMeter });

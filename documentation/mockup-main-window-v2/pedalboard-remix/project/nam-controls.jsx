/* NAM Loader — interactive controls in the polished remix language
   (thin-arc knobs, clean fills, glowing pill toggles). No skeuomorphism. */

const { useState: nState, useRef: nRef, useCallback: nCB, useEffect: nEffect } = React;

/* amp-style rotary: warm metallic cap, amber value arc, cream pointer */
function NamKnob({ value, min = 0, max = 10, onChange, size = 60, color = "var(--amp-accent)" }) {
  const norm = (value - min) / (max - min);
  const r = 17, c = 2 * Math.PI * r, sweep = 0.75;
  const arc = Math.max(0, Math.min(1, norm)) * sweep * c;
  const ang = 135 + norm * 270;
  const rad = (ang - 90) * Math.PI / 180;
  const drag = nRef(null);
  const onDown = (e) => {
    e.preventDefault(); e.stopPropagation();
    drag.current = { y: e.clientY, v: value };
    const move = (ev) => {
      const dy = drag.current.y - ev.clientY;
      let nv = drag.current.v + (dy / 130) * (max - min);
      onChange(Math.max(min, Math.min(max, Math.round(nv * 10) / 10)));
    };
    const up = () => { window.removeEventListener("pointermove", move); window.removeEventListener("pointerup", up); };
    window.addEventListener("pointermove", move); window.addEventListener("pointerup", up);
  };
  return (
    <svg className="nk-dial" width={size} height={size} viewBox="0 0 44 44" onPointerDown={onDown}>
      <g transform="rotate(135 22 22)">
        <circle className="nk-arc-track" cx="22" cy="22" r={r} fill="none" strokeWidth="3" strokeLinecap="round" strokeDasharray={`${sweep * c} ${c}`} />
        <circle cx="22" cy="22" r={r} fill="none" strokeWidth="3" strokeLinecap="round" stroke={color} strokeDasharray={`${arc} ${c}`} style={{ filter: "drop-shadow(0 0 3px " + color + ")" }} />
      </g>
      <circle cx="22" cy="22" r="13" fill="var(--amp-edge-hi)" />
      <circle cx="22" cy="22" r="11.5" fill="var(--amp-knob-face)" stroke="var(--amp-edge)" strokeWidth="1" />
      <circle cx="22" cy="22" r="6" fill="none" stroke="var(--amp-edge)" strokeWidth="0.75" opacity="0.5" />
      <line x1={22 + 5 * Math.cos(rad)} y1={22 + 5 * Math.sin(rad)} x2={22 + 10.5 * Math.cos(rad)} y2={22 + 10.5 * Math.sin(rad)} stroke="var(--amp-text)" strokeWidth="2.4" strokeLinecap="round" />
      <circle cx={22 + 10.5 * Math.cos(rad)} cy={22 + 10.5 * Math.sin(rad)} r="1.6" fill={color} />
    </svg>
  );
}

/* clean horizontal slider with mono value chip */
function NamSlider({ value, min, max, step = 0.01, format, onChange, accent = "var(--amp-accent2)" }) {
  const trackRef = nRef(null);
  const norm = (value - min) / (max - min);
  const setFromX = (clientX) => {
    const el = trackRef.current; if (!el) return;
    const rr = el.getBoundingClientRect();
    let tt = Math.max(0, Math.min(1, (clientX - rr.left) / rr.width));
    let v = min + tt * (max - min);
    v = Math.round(v / step) * step;
    onChange(Math.max(min, Math.min(max, v)));
  };
  const onDown = (e) => {
    e.preventDefault(); e.stopPropagation();
    setFromX(e.clientX);
    const move = (ev) => setFromX(ev.clientX);
    const up = () => { window.removeEventListener("pointermove", move); window.removeEventListener("pointerup", up); };
    window.addEventListener("pointermove", move); window.addEventListener("pointerup", up);
  };
  return (
    <div className="nk-slider">
      <div className="nk-track" ref={trackRef} onPointerDown={onDown}>
        <div className="nk-fill" style={{ width: (norm * 100) + "%", background: "linear-gradient(90deg, color-mix(in srgb, " + accent + " 45%, transparent), " + accent + ")" }}></div>
        <div className="nk-thumb" style={{ left: (norm * 100) + "%" }}></div>
      </div>
      <div className="nk-val">{format(value)}</div>
    </div>
  );
}

/* glowing pill toggle */
function NamToggle({ on, label, onChange, accent }) {
  return (
    <button className={"nk-tog" + (on ? " on" : "")} style={accent ? { "--tg": accent } : null} onClick={() => onChange(!on)}>
      <span className="nk-tdot"></span><span>{label}</span>
    </button>
  );
}

function NamButton({ children, variant, onClick, title }) {
  return <button className={"nk-btn" + (variant ? " " + variant : "")} onClick={onClick} title={title}>{children}</button>;
}

Object.assign(window, { NamKnob, NamSlider, NamToggle, NamButton });

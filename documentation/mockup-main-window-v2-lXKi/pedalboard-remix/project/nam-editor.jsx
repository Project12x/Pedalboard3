/* NAM Loader — editor panel, restyled in the remix design language
   (cards, thin-arc knobs, glowing pills). Same controls as NAMControl.cpp,
   elevated to match Main Window v2 and Stage Mode. */

function NamCard({ title, accent, children, right }) {
  return (
    <div className="nk-card">
      <div className="nk-card-h">
        <span className="nk-dot" style={accent ? { background: accent, boxShadow: "0 0 8px " + accent } : null}></span>
        <span className="nk-card-t">{title}</span>
        {right}
      </div>
      {children}
    </div>
  );
}

function NAMEditor({ theme, collapsed, setCollapsed, finish, model, setModel, ir, setIr, ir2, setIr2, onBrowse }) {
  const [irOn, setIrOn] = nState(true);
  const [ir2On, setIr2On] = nState(true);
  const [blend, setBlend] = nState(0.35);
  const [loCut, setLoCut] = nState(80);
  const [hiCut, setHiCut] = nState(12000);
  const [fxLoop, setFxLoop] = nState(false);
  const [input, setInput] = nState(0.0);
  const [output, setOutput] = nState(0.0);
  const [gate, setGate] = nState(-80);
  const [eqOn, setEqOn] = nState(true);
  const [pre, setPre] = nState(false);
  const [norm, setNorm] = nState(false);
  const [bass, setBass] = nState(6.5);
  const [mid, setMid] = nState(5.0);
  const [treble, setTreble] = nState(7.0);

  const dB = (v) => (v >= 0 ? "+" : "") + v.toFixed(1);
  const Hz = (v) => (v >= 1000 ? (v / 1000).toFixed(v % 1000 === 0 ? 0 : 1) + "k" : Math.round(v));

  return (
    <div className={"nam theme-" + (theme || "midnight") + (collapsed ? " collapsed" : "") + (finish || "")} data-screen-label="NAM Loader editor">
      <span className="nk-screw tl"></span><span className="nk-screw tr"></span><span className="nk-screw bl"></span><span className="nk-screw br"></span>
      {/* ---------- header ---------- */}
      <header className="nk-head" onClick={() => setCollapsed(!collapsed)}>
        <div className="nk-head-l">
          <div className="nk-eyebrow">NAM Loader</div>
          <div className={"nk-title" + (model ? "" : " empty")}>{model || "No model loaded"}</div>
        </div>
        <div className="nk-head-r">
          <div className={"nk-status" + (model ? " on" : "")}>
            <i></i>{model ? "Active" : "Bypassed"}
          </div>
          <button className="nk-collapse" onClick={(e) => { e.stopPropagation(); setCollapsed(!collapsed); }} title="Collapse">
            <svg viewBox="0 0 24 24" fill="none"><path d="M6 9l6 6 6-6" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"/></svg>
          </button>
        </div>
      </header>

      <div className="nk-body">
        {/* ---------- SIGNAL CHAIN ---------- */}
        <NamCard title="Signal Chain" accent="var(--amp-accent)">
          <div className="nk-row model">
            <NamButton variant="primary" onClick={() => onBrowse && onBrowse("model")}>Load Model</NamButton>
            <NamButton onClick={() => onBrowse && onBrowse("model")} title="Browse NAM models online">Browse…</NamButton>
            <div className={"nk-field" + (model ? " loaded" : "")}>
              <span className="nk-field-t">{model || "No Model Loaded"}</span>
              {model && <span className="nk-tag">NAM</span>}
              {model && <button className="nk-clear" onClick={() => setModel(null)} title="Clear">✕</button>}
            </div>
          </div>

          <div className="nk-cab-grid">
            <div className="nk-cab">
              <div className="nk-cab-h"><span className="nk-cab-lab">Cabinet IR</span><NamToggle on={irOn} label="On" onChange={setIrOn} /></div>
              <div className="nk-field sm">
                <span className="nk-field-t">{ir || "No IR Loaded"}</span>
                <button className="nk-mini" onClick={() => ir ? setIr(null) : (onBrowse && onBrowse("ir"))}>{ir ? "✕" : "Load"}</button>
              </div>
            </div>
            <div className="nk-cab">
              <div className="nk-cab-h"><span className="nk-cab-lab">Cabinet IR 2</span><NamToggle on={ir2On} label="On" onChange={setIr2On} /></div>
              <div className="nk-field sm">
                <span className="nk-field-t">{ir2 || "No IR2 Loaded"}</span>
                <button className="nk-mini" onClick={() => ir2 ? setIr2(null) : (onBrowse && onBrowse("ir2"))}>{ir2 ? "✕" : "Load"}</button>
              </div>
            </div>
          </div>

          <div className="nk-ctl"><span className="nk-ctl-l">Blend</span><NamSlider value={blend} min={0} max={1} step={0.01} format={(v) => Math.round(v * 100) + "%"} onChange={setBlend} /></div>
          <div className="nk-ctl two">
            <div className="nk-sub"><span className="nk-ctl-l">Lo Cut</span><NamSlider value={loCut} min={20} max={500} step={1} format={(v) => Hz(v) + " Hz"} onChange={setLoCut} /></div>
            <div className="nk-sub"><span className="nk-ctl-l">Hi Cut</span><NamSlider value={hiCut} min={2000} max={20000} step={100} format={(v) => Hz(v) + "Hz"} onChange={setHiCut} /></div>
          </div>

          <div className="nk-fxrow">
            <NamToggle on={fxLoop} label="FX Loop" onChange={setFxLoop} />
            <NamButton onClick={() => {}} title="Edit effects loop">Edit FX Loop…</NamButton>
          </div>
        </NamCard>

        <div className="nk-two">
          {/* ---------- GAIN ---------- */}
          <NamCard title="Gain" accent="var(--amp-accent)">
            <div className="nk-ctl"><span className="nk-ctl-l wide">Input</span><NamSlider value={input} min={-20} max={20} step={0.1} format={dB} onChange={setInput} /><span className="nk-unit">dB</span></div>
            <div className="nk-ctl"><span className="nk-ctl-l wide">Output</span><NamSlider value={output} min={-40} max={40} step={0.1} format={dB} onChange={setOutput} /><span className="nk-unit">dB</span></div>
            <div className="nk-ctl"><span className="nk-ctl-l wide">Gate</span><NamSlider value={gate} min={-101} max={0} step={1} format={(v) => Math.round(v)} onChange={setGate} /><span className="nk-unit">dB</span></div>
          </NamCard>

          {/* ---------- TONE ---------- */}
          <NamCard title="Tone" accent="var(--amp-accent)" right={
            <div className="nk-tone-tabs">
              <NamToggle on={eqOn} label="EQ" onChange={setEqOn} />
              <button className={"nk-seg" + (pre ? " on" : "")} onClick={() => setPre(!pre)} title="EQ position">{pre ? "PRE" : "POST"}</button>
            </div>
          }>
            <div className={"nk-knobs" + (eqOn ? "" : " off")}>
              <div className="nk-knob"><NamKnob value={bass} onChange={setBass} /><span className="nk-kv">{bass.toFixed(1)}</span><span className="nk-kl">Bass</span></div>
              <div className="nk-knob"><NamKnob value={mid} onChange={setMid} /><span className="nk-kv">{mid.toFixed(1)}</span><span className="nk-kl">Mid</span></div>
              <div className="nk-knob"><NamKnob value={treble} onChange={setTreble} /><span className="nk-kv">{treble.toFixed(1)}</span><span className="nk-kl">Treble</span></div>
            </div>
            <div className="nk-norm"><NamToggle on={norm} label="Normalize output" onChange={setNorm} /></div>
          </NamCard>
        </div>
      </div>
    </div>
  );
}

window.NAMEditor = NAMEditor;

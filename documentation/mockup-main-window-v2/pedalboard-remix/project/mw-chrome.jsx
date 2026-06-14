/* Main Window — toolbar, add menu, status bar. */

function MwToolbar({ theme, setTheme, cpu, onAddClick, addOpen, nodeCount }) {
  return (
    <header className="mw-toolbar">
      <div className="mw-brand"><span className="dot"></span>Pedalboard</div>

      <div className="mw-patch">
        <div className="pn"><b>Riptide</b><span>Drive Lead · friday-set</span></div>
        <span className="car">▾</span>
      </div>

      <span className="mw-tdiv"></span>

      <div className="mw-acts">
        <button className={"mw-btn primary" + (addOpen ? " on" : "")} onClick={onAddClick}>{MwIcon.plus()}Add</button>
        <button className="mw-btn">{MwIcon.save()}Save</button>
      </div>

      <div className="mw-spacer"></div>

      <div className="mw-themes" role="group" aria-label="Theme">
        {MW_THEMES.map((t) => (
          <button key={t.id} className={"mw-theme-dot" + (theme === t.id ? " on" : "")}
            style={{ "--td-accent": t.accent, "--td-bg": t.bg }} title={t.name}
            onClick={() => setTheme(t.id)}></button>
        ))}
      </div>

      <span className="mw-tdiv"></span>

      <div className="mw-cpu">DSP <b>{cpu.toFixed(1)}%</b></div>
      <button className="mw-btn">{MwIcon.stage()}Stage</button>
      <button className="mw-btn icon" title="Settings">{MwIcon.gear()}</button>
    </header>
  );
}

function MwAddMenu({ onPick, onClose }) {
  const groups = {};
  MW_CATALOG.forEach((e) => { (groups[e.cat] = groups[e.cat] || []).push(e); });
  return (
    <>
      <div style={{ position: "fixed", inset: 0, zIndex: 39 }} onPointerDown={onClose}></div>
      <div className="mw-addmenu" style={{ left: 168, top: 62 }} onPointerDown={(e) => e.stopPropagation()}>
        {Object.keys(groups).map((cat) => (
          <div key={cat}>
            <div className="grp">{MW_CAT[cat].label}</div>
            {groups[cat].map((e) => (
              <button key={e.name} className="it" onClick={() => onPick(e)}>
                <span className="sw" style={{ background: MW_CAT[cat].color }}></span>
                {e.name}
                <span className="meta">{e.cpu.toFixed(1)}%</span>
              </button>
            ))}
          </div>
        ))}
      </div>
    </>
  );
}

function MwStatus({ nodeCount, cableCount, cpu }) {
  return (
    <footer className="mw-status">
      <div className="s"><b>48.0</b> kHz</div>
      <span className="sdiv"></span>
      <div className="s"><b>128</b> smp</div>
      <span className="sdiv"></span>
      <div className="s">latency <b>5.3 ms</b></div>
      <span className="sdiv"></span>
      <div className="s"><b>{nodeCount}</b> nodes · <b>{cableCount}</b> cables</div>
      <div className="right">
        <div className="midi"><span className="ml"></span>MIDI</div>
        <span className="sdiv"></span>
        <div className="s">DSP <b>{cpu.toFixed(1)}%</b></div>
      </div>
    </footer>
  );
}

Object.assign(window, { MwToolbar, MwAddMenu, MwStatus });

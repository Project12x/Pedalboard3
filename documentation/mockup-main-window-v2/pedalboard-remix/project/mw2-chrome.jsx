/* Main Window v2 — window chrome: OS title-bar, menu-bar (with theme dots
   + Stage), the bottom transport bar, and the virtual piano keyboard. */

function M2TitleBar() {
  return (
    <div className="m2-titlebar">
      <div className="tb-app"><span className="tb-glyph"></span>Pedalboard 3</div>
      <div className="tb-doc">default.pdl</div>
      <div className="tb-win">
        <button className="wb" title="Minimise">{M2Icon.min()}</button>
        <button className="wb" title="Maximise">{M2Icon.max()}</button>
        <button className="wb close" title="Close">{M2Icon.close()}</button>
      </div>
    </div>
  );
}

function M2MenuBar({ theme, setTheme, onStage, onManageSetlist }) {
  const [open, setOpen] = React.useState(null);
  const onRow = (it) => {
    if (it === "Manage Setlist…" && onManageSetlist) onManageSetlist();
  };
  const MENUS = {
    File: ["New", "Open…", "Save", "Save As…", "—", "Save As Default", "Exit"],
    Edit: ["Undo", "Redo", "—", "Delete Connection", "Manage Setlist…", "—", "Panic"],
    Options: ["Audio Settings…", "Plugin List…", "Preferences…", "—", "Snap to Grid", "Colour Schemes…"],
    Help: ["Documentation", "Log", "—", "About Pedalboard 3"],
  };
  return (
    <div className="m2-menubar" onPointerLeave={() => setOpen(null)}>
      {Object.keys(MENUS).map((m) => (
        <div className="mb-item-wrap" key={m}>
          <button className={"mb-item" + (open === m ? " on" : "")} onClick={() => setOpen((o) => (o === m ? null : m))} onPointerEnter={() => setOpen((o) => (o ? m : o))}>{m}</button>
          {open === m && (
            <div className="mb-menu" onClick={() => setOpen(null)}>
              {MENUS[m].map((it, i) => it === "—" ? <div key={i} className="mb-sep"></div> : <button key={i} className="mb-row" onClick={() => onRow(it)}>{it}</button>)}
            </div>
          )}
        </div>
      ))}
      <div className="mb-spacer"></div>
      <div className="mb-themes" title="Colour scheme">
        {M2_THEMES.map((t) => (
          <button key={t.id} className={"mb-dot" + (theme === t.id ? " on" : "")}
            style={{ "--td": t.accent, "--tb": t.bg }} title={t.name} onClick={() => setTheme(t.id)}></button>
        ))}
      </div>
      <button className="mb-stage" onClick={onStage}>{M2Icon.stage()}Stage</button>
    </div>
  );
}

/* ---- piano keyboard ---- */
function M2Keyboard() {
  const WHITE = ["C", "D", "E", "F", "G", "A", "B"];
  const BLACK = { 0: true, 1: true, 3: true, 4: true, 5: true }; // after C,D,F,G,A
  const octaves = [1, 2, 3, 4, 5];
  const whites = [];
  octaves.forEach((oct) => WHITE.forEach((n, i) => whites.push({ note: n, oct, isC: i === 0, idx: i })));
  whites.push({ note: "C", oct: 6, isC: true, idx: 0 });
  const W = 100 / whites.length;
  return (
    <div className="m2-keys">
      <div className="kb-whites">
        {whites.map((k, i) => (
          <div className="wkey" key={i} style={{ width: W + "%" }}>{k.isC && <span className="klabel">C{k.oct}</span>}</div>
        ))}
      </div>
      <div className="kb-blacks">
        {whites.map((k, i) => (k.note !== "C" || true) && BLACK[k.idx] && i < whites.length - 1
          ? <span className="bkey" key={i} style={{ left: ((i + 1) * W) + "%" }}></span>
          : null)}
      </div>
    </div>
  );
}

function M2Field({ label, value, w }) {
  return (
    <div className="tp-field" style={w ? { width: w } : null}>
      {label && <span className="tpf-l">{label}</span>}
      <span className="tpf-v">{value}</span>
    </div>
  );
}

function M2Transport({ cpu, tempo, onScratch }) {
  return (
    <div className="m2-transport">
      <button className="tp-btn rec" onClick={onScratch} title="Open Scratch Mode"><span className="rdot">{M2Icon.rec()}</span>REC</button>
      <button className="tp-btn" onClick={onScratch} title="Open Scratch Mode">Takes</button>
      <span className="tp-div"></span>

      <span className="tp-key">Patch</span>
      <button className="tp-step">−</button>
      <div className="tp-combo"><span>1 · &lt;untitled&gt;</span>{M2Icon.chev()}</div>
      <button className="tp-step">+</button>
      <span className="tp-div"></span>

      <span className="tp-key">Tempo</span>
      <M2Field value={tempo.toFixed(2)} w={72} />
      <button className="tp-ic">{M2Icon.play()}</button>
      <button className="tp-ic">{M2Icon.rtz()}</button>
      <span className="tp-div"></span>

      <M2Field label="IN" value="0.0 dB" />
      <button className="tp-btn sm">FX</button>
      <M2Field label="OUT" value="−15.7 dB" />
      <span className="tp-div"></span>

      <button className="tp-btn">Manage</button>
      <button className="tp-btn">Fit</button>
      <span className="tp-div"></span>

      <span className="tp-key">UI Scale</span>
      <div className="tp-combo sm"><span>100%</span>{M2Icon.chev()}</div>

      <div className="tp-cpu"><span>CPU</span><div className="cpu-bar"><i style={{ width: Math.min(100, cpu * 3.4) + "%" }}></i></div><b>{cpu.toFixed(0)}%</b></div>
    </div>
  );
}

function M2Footer(props) {
  return (
    <div className="m2-footer">
      <div className="m2-kbrow">
        <div className="kb-left">
          <button className="kb-step">−</button><span className="kb-lab">Oct 0</span><button className="kb-step">+</button>
          <span className="kb-lab vel">Vel</span><div className="kb-vel"><i style={{ width: "78%" }}></i><span className="kv-thumb" style={{ left: "78%" }}></span></div><span className="kb-lab num">100</span>
        </div>
        <M2Keyboard />
        <div className="kb-right"><button className="kb-sus">Sus</button></div>
      </div>
      <M2Transport cpu={props.cpu} tempo={props.tempo} onScratch={props.onScratch} />
    </div>
  );
}

Object.assign(window, { M2TitleBar, M2MenuBar, M2Keyboard, M2Transport, M2Footer });

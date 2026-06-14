/* Main Window v2 — app shell. Owns graph state, theme, tweaks; wires the
   real-layout chrome (title-bar, menu, transport, keyboard) around the canvas. */

const M2_TWEAKS = /*EDITMODE-BEGIN*/{
  "accent": "Cyan",
  "cableWeight": 4,
  "grid": "Lines",
  "showKeyboard": true,
  "snap": false
}/*EDITMODE-END*/;

function useStored2(key, init) {
  const [v, setV] = React.useState(() => {
    try { const s = localStorage.getItem(key); return s == null ? init : JSON.parse(s); } catch { return init; }
  });
  React.useEffect(() => { try { localStorage.setItem(key, JSON.stringify(v)); } catch {} }, [key, v]);
  return [v, setV];
}

function M2AddMenu({ onPick, onClose }) {
  const groups = {};
  M2_CATALOG.forEach((e) => { (groups[e.cat] = groups[e.cat] || []).push(e); });
  return (
    <>
      <div style={{ position: "fixed", inset: 0, zIndex: 49 }} onPointerDown={onClose}></div>
      <div className="m2-addmenu" onPointerDown={(e) => e.stopPropagation()}>
        <div className="am-h">Add Processor</div>
        {Object.keys(groups).map((cat) => (
          <div key={cat}>
            <div className="am-grp">{M2_CAT[cat] ? M2_CAT[cat].label : cat}</div>
            {groups[cat].map((e) => (
              <button key={e.name} className="am-it" onClick={() => onPick(e)}>
                <span className="am-sw" style={{ background: (M2_CAT[cat] || M2_CAT.source).color }}></span>{e.name}
                <span className="am-meta">{e.cpu.toFixed(1)}%</span>
              </button>
            ))}
          </div>
        ))}
      </div>
    </>
  );
}

function MainWindow2(props) {
  const { onOpenStage, onOpenScratch, onManageSetlist } = props || {};
  const [t, setTweak] = useTweaks(M2_TWEAKS);
  const [theme, setTheme] = useStored2("mw2.theme", "midnight");

  const initial = React.useMemo(() => m2InitialGraph(), []);
  const [nodes, setNodes] = React.useState(initial.nodes);
  const [conns, setConns] = React.useState(initial.conns);
  const [viewport, setViewport] = React.useState({ x: 0, y: 0, zoom: 1 });
  const [selected, setSelected] = React.useState(null);
  const [addOpen, setAddOpen] = React.useState(false);
  const [editNode, setEditNode] = React.useState(null);   // plugin node open in the NAM editor window

  const cpu = nodes.reduce((s, n) => s + (n.bypass ? 0 : (n.cpu || 0)), 0);

  const onToggle = React.useCallback((id, key) => {
    if (key === "__remove") {
      setNodes((ns) => ns.filter((n) => n.id !== id));
      setConns((cs) => cs.filter((c) => c.from.node !== id && c.to.node !== id));
      setSelected((s) => (s === id ? null : s));
      return;
    }
    if (key === "__edit") {
      setNodes((ns) => {
        const n = ns.find((x) => x.id === id);
        if (n && n.type === "plugin") setEditNode(n);
        return ns;
      });
      return;
    }
    setNodes((ns) => ns.map((n) => (n.id === id ? { ...n, [key]: !n[key] } : n)));
  }, []);

  const addNode = (entry) => {
    const cx = (700 - viewport.x) / viewport.zoom - 96;
    const cy = (300 - viewport.y) / viewport.zoom;
    setNodes((ns) => [...ns, m2MakeNode(entry, Math.round(cx), Math.round(cy))]);
    setAddOpen(false);
  };

  const accentMap = { Cyan: null, Amber: "#ffb020", Violet: "#a78bfa", Lime: "#a3e635" };
  const accentOverride = accentMap[t.accent];
  return (
    <div className={"mw2 theme-" + theme + " grid-" + (t.grid || "Dots").toLowerCase()}
      style={{ "--cable-w": (t.cableWeight || 4) + "px", ...(accentOverride ? { "--accent": accentOverride } : {}) }}>
      <M2TitleBar />
      <M2MenuBar theme={theme} setTheme={setTheme} onStage={onOpenStage} onManageSetlist={onManageSetlist || onOpenStage} />

      <div className="m2-stage">
        <MwCanvas2
          nodes={nodes} conns={conns} setNodes={setNodes} setConns={setConns}
          viewport={viewport} setViewport={setViewport}
          selected={selected} setSelected={setSelected}
          snap={!!t.snap} onToggle={onToggle} />

        <button className={"m2-add" + (addOpen ? " on" : "")} onClick={() => setAddOpen((o) => !o)} title="Add processor">
          <svg viewBox="0 0 24 24" fill="none" width="18" height="18"><path d="M12 5v14M5 12h14" stroke="currentColor" strokeWidth="2.2" strokeLinecap="round"/></svg>
          Add
        </button>
        {addOpen && <M2AddMenu onPick={addNode} onClose={() => setAddOpen(false)} />}

        <div className="m2-hint">drag header to move · drag a port to wire · hover a cable to delete · scroll to zoom · <b>e</b> on the NAM node opens its editor</div>
      </div>

      {editNode && (
        <NamEditorWindow
          theme={theme}
          finish={(t.brushed ? " fin-brushed" : "") + (t.bevel ? " fin-bevel" : "")}
          initialModel={editNode.model}
          onModelChange={(name) => setNodes((ns) => ns.map((n) => (n.id === editNode.id ? { ...n, model: name || "No Model" } : n)))}
          onClose={() => setEditNode(null)} />
      )}

      {t.showKeyboard !== false && <M2Footer cpu={cpu} tempo={120} onScratch={onOpenScratch} />}

      <TweaksPanel>
        <TweakSection label="Appearance" />
        <TweakRadio label="Accent" value={t.accent} options={["Cyan", "Amber", "Violet", "Lime"]} onChange={(v) => setTweak("accent", v)} />
        <TweakRadio label="Grid" value={t.grid} options={["Dots", "Lines", "Off"]} onChange={(v) => setTweak("grid", v)} />
        <TweakSection label="Routing" />
        <TweakSlider label="Cable weight" min={2} max={7} step={1} value={t.cableWeight} onChange={(v) => setTweak("cableWeight", v)} />
        <TweakToggle label="Snap to grid" value={t.snap} onChange={(v) => setTweak("snap", v)} />
        <TweakSection label="Layout" />
        <TweakToggle label="Keyboard + transport" value={t.showKeyboard} onChange={(v) => setTweak("showKeyboard", v)} />
      </TweaksPanel>
    </div>
  );
}

window.MainWindow2 = MainWindow2;

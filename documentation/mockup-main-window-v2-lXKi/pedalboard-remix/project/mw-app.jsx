/* Main Window — app shell. Owns graph state, theme, tweaks. */

const TWEAK_DEFAULTS = /*EDITMODE-BEGIN*/{
  "nodeStyle": "Module",
  "cableStyle": "Curved",
  "grid": "Dots",
  "snap": false
}/*EDITMODE-END*/;

function useStoredMW(key, init) {
  const [v, setV] = React.useState(() => {
    try { const s = localStorage.getItem(key); return s == null ? init : JSON.parse(s); } catch { return init; }
  });
  React.useEffect(() => { try { localStorage.setItem(key, JSON.stringify(v)); } catch {} }, [key, v]);
  return [v, setV];
}

function MainWindow() {
  const [t, setTweak] = useTweaks(TWEAK_DEFAULTS);
  const [theme, setTheme] = useStoredMW("mw.theme", "midnight");

  const initial = React.useMemo(() => mwInitialGraph(), []);
  const [nodes, setNodes] = React.useState(initial.nodes);
  const [conns, setConns] = React.useState(initial.conns);
  const [viewport, setViewport] = React.useState({ x: 0, y: 0, zoom: 1 });
  const [selected, setSelected] = React.useState(null);
  const [addOpen, setAddOpen] = React.useState(false);

  const nodeStyle = (t.nodeStyle || "Module").toLowerCase();
  const cableStyle = (t.cableStyle || "Curved").toLowerCase();
  const grid = (t.grid || "Dots").toLowerCase();

  const cpu = nodes.reduce((s, n) => s + (n.bypass ? 0 : (n.cpu || 0)), 0);

  const onToggle = React.useCallback((id, key) => {
    setNodes((ns) => ns.map((n) => (n.id === id ? { ...n, [key]: !n[key] } : n)));
  }, []);

  const addNode = (entry) => {
    const cx = (640 - viewport.x) / viewport.zoom - MW_NODE_W / 2;
    const cy = (353 - viewport.y) / viewport.zoom - 40;
    setNodes((ns) => [...ns, mwMakeNode(entry, Math.round(cx), Math.round(cy))]);
    setAddOpen(false);
  };

  return (
    <div className={"mw theme-" + theme}>
      <MwToolbar theme={theme} setTheme={setTheme} cpu={cpu} nodeCount={nodes.length}
        addOpen={addOpen} onAddClick={() => setAddOpen((o) => !o)} />
      {addOpen && <MwAddMenu onPick={addNode} onClose={() => setAddOpen(false)} />}

      <MwCanvas
        nodes={nodes} conns={conns} setNodes={setNodes} setConns={setConns}
        viewport={viewport} setViewport={setViewport}
        selected={selected} setSelected={setSelected}
        nodeStyle={nodeStyle} cableStyle={cableStyle} grid={grid} snap={!!t.snap}
        onToggle={onToggle} />

      <MwStatus nodeCount={nodes.length} cableCount={conns.length} cpu={cpu} />

      <TweaksPanel>
        <TweakSection label="Node style" />
        <TweakRadio label="Anatomy" value={t.nodeStyle} options={["Module", "Classic", "Signal"]}
          onChange={(v) => setTweak("nodeStyle", v)} />
        <TweakSection label="Routing" />
        <TweakRadio label="Cables" value={t.cableStyle} options={["Curved", "Angular", "Flowing"]}
          onChange={(v) => setTweak("cableStyle", v)} />
        <TweakRadio label="Grid" value={t.grid} options={["Dots", "Lines", "Off"]}
          onChange={(v) => setTweak("grid", v)} />
        <TweakToggle label="Snap to grid" value={t.snap} onChange={(v) => setTweak("snap", v)} />
      </TweaksPanel>
    </div>
  );
}

window.MainWindow = MainWindow;

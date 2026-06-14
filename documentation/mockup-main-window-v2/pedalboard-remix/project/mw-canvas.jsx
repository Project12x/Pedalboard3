/* Main Window — interactive canvas: pan / zoom, node drag,
   cable wiring (drag port→port), cable delete. */

const { useState, useRef, useEffect, useCallback } = React;

function MwCanvas(props) {
  const { nodes, conns, setNodes, setConns, viewport, setViewport,
          selected, setSelected, nodeStyle, cableStyle, grid, snap } = props;

  const canvasRef = useRef(null);
  const ia = useRef(null);                 // active interaction
  const [wire, setWire] = useState(null);  // {kind,dir,fromNode,fromPort,mouse:{x,y}}
  const [target, setTarget] = useState(null);
  const [hoverCable, setHoverCable] = useState(null);
  const [panning, setPanning] = useState(false);

  const nodeMap = {};
  nodes.forEach((n) => { nodeMap[n.id] = n; });

  /* screen(client) -> logical canvas px, accounting for outer fit-scale */
  const fitScale = () => {
    const el = canvasRef.current;
    if (!el) return 1;
    const r = el.getBoundingClientRect();
    return r.width / el.offsetWidth || 1;
  };
  const toLocal = (e) => {
    const el = canvasRef.current, r = el.getBoundingClientRect(), s = fitScale();
    return { x: (e.clientX - r.left) / s, y: (e.clientY - r.top) / s };
  };
  const toWorld = (e) => {
    const l = toLocal(e);
    return { x: (l.x - viewport.x) / viewport.zoom, y: (l.y - viewport.y) / viewport.zoom };
  };

  /* ---------- node drag ---------- */
  const onHeaderDown = useCallback((nodeId, e) => {
    const n = nodeMap[nodeId];
    if (!n) return;
    ia.current = { type: "node", nodeId, startWorld: toWorld(e), nodeStart: { x: n.x, y: n.y } };
    setSelected(nodeId);
  }, [nodes, viewport]);

  /* ---------- wiring ---------- */
  const onPortDown = useCallback((nodeId, portId, e) => {
    const m = mwPortMeta(portId);
    const start = mwPortPos(nodeMap[nodeId], portId);
    ia.current = { type: "wire", fromNode: nodeId, fromPort: portId, kind: m.kind, dir: m.dir };
    setWire({ kind: m.kind, dir: m.dir, fromNode: nodeId, fromPort: portId, start, mouse: toWorld(e) });
  }, [nodes, viewport]);

  const onPortEnter = useCallback((nodeId, portId) => {
    const w = ia.current;
    if (!w || w.type !== "wire") return;
    if (nodeId === w.fromNode) return;
    if (!mwCompatible(w.fromPort, portId)) return;
    setTarget({ node: nodeId, port: portId });
  }, []);
  const onPortLeave = useCallback((nodeId, portId) => {
    setTarget((t) => (t && t.node === nodeId && t.port === portId ? null : t));
  }, []);

  const connect = (aN, aP, bN, bP, kind) => {
    let fromN = aN, fromP = aP, toN = bN, toP = bP;
    if (mwPortMeta(aP).dir === "in") { fromN = bN; fromP = bP; toN = aN; toP = aP; }
    setConns((cs) => {
      if (cs.some((c) => c.from.node === fromN && c.from.port === fromP && c.to.node === toN && c.to.port === toP)) return cs;
      // single source into an input: replace existing into that input
      const filtered = cs.filter((c) => !(c.to.node === toN && c.to.port === toP));
      return [...filtered, { id: mwUid("c"), from: { node: fromN, port: fromP }, to: { node: toN, port: toP }, kind }];
    });
  };

  /* ---------- pan / background ---------- */
  const onCanvasDown = useCallback((e) => {
    if (e.target !== canvasRef.current && !e.target.classList.contains("mw-world") && !e.target.classList.contains("mw-grid")) {
      // still allow pan when clicking empty world area
    }
    setSelected(null);
    ia.current = { type: "pan", startScreen: { x: e.clientX, y: e.clientY }, vpStart: { ...viewport } };
    setPanning(true);
  }, [viewport]);

  /* ---------- global move / up ---------- */
  useEffect(() => {
    const move = (e) => {
      const it = ia.current;
      if (!it) return;
      if (it.type === "node") {
        const w = toWorld(e);
        let nx = it.nodeStart.x + (w.x - it.startWorld.x);
        let ny = it.nodeStart.y + (w.y - it.startWorld.y);
        if (snap) { nx = Math.round(nx / 16) * 16; ny = Math.round(ny / 16) * 16; }
        setNodes((ns) => ns.map((n) => (n.id === it.nodeId ? { ...n, x: nx, y: ny } : n)));
      } else if (it.type === "wire") {
        setWire((wv) => (wv ? { ...wv, mouse: toWorld(e) } : wv));
      } else if (it.type === "pan") {
        const s = fitScale();
        const dx = (e.clientX - it.startScreen.x) / s;
        const dy = (e.clientY - it.startScreen.y) / s;
        setViewport({ ...it.vpStart, x: it.vpStart.x + dx, y: it.vpStart.y + dy });
      }
    };
    const up = () => {
      const it = ia.current;
      if (it && it.type === "wire") {
        setTarget((t) => {
          if (t) connect(it.fromNode, it.fromPort, t.node, t.port, it.kind);
          return null;
        });
        setWire(null);
      }
      if (it && it.type === "pan") setPanning(false);
      ia.current = null;
    };
    window.addEventListener("pointermove", move);
    window.addEventListener("pointerup", up);
    return () => { window.removeEventListener("pointermove", move); window.removeEventListener("pointerup", up); };
  }, [viewport, snap]);

  /* ---------- wheel zoom ---------- */
  const onWheel = useCallback((e) => {
    e.preventDefault();
    const l = toLocal(e);
    const factor = e.deltaY < 0 ? 1.1 : 1 / 1.1;
    setViewport((vp) => {
      const z2 = Math.max(0.4, Math.min(2.2, vp.zoom * factor));
      const wx = (l.x - vp.x) / vp.zoom, wy = (l.y - vp.y) / vp.zoom;
      return { zoom: z2, x: l.x - wx * z2, y: l.y - wy * z2 };
    });
  }, []);

  const zoomBy = (factor) => setViewport((vp) => {
    const cx = 640, cy = 353;
    const z2 = Math.max(0.4, Math.min(2.2, vp.zoom * factor));
    const wx = (cx - vp.x) / vp.zoom, wy = (cy - vp.y) / vp.zoom;
    return { zoom: z2, x: cx - wx * z2, y: cy - wy * z2 };
  });

  /* ---------- delete selected with keyboard ---------- */
  useEffect(() => {
    const onKey = (e) => {
      if ((e.key === "Delete" || e.key === "Backspace") && selected) {
        setNodes((ns) => ns.filter((n) => n.id !== selected));
        setConns((cs) => cs.filter((c) => c.from.node !== selected && c.to.node !== selected));
        setSelected(null);
      }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [selected]);

  const portHandlers = { onDown: onPortDown, onEnter: onPortEnter, onLeave: onPortLeave };

  /* ---------- cables ---------- */
  const animated = cableStyle === "flowing" || nodeStyle === "signal";
  const cableEls = conns.map((c) => {
    const fn = nodeMap[c.from.node], tn = nodeMap[c.to.node];
    if (!fn || !tn) return null;
    const a = mwPortPos(fn, c.from.port), b = mwPortPos(tn, c.to.port);
    const toSide = mwPortMeta(c.to.port).side;
    const d = mwCablePath(a, b, cableStyle, mwPortMeta(c.from.port).side, toSide);
    const col = c.kind === "param" ? "var(--param)" : "var(--accent)";
    const mid = { x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 };
    return (
      <g key={c.id} className={animated ? "animated" : ""} onMouseEnter={() => setHoverCable(c.id)} onMouseLeave={() => setHoverCable((h) => (h === c.id ? null : h))}>
        <path className="hit" d={d} />
        <path className="wire" d={d} stroke={col} opacity={animated ? 0.4 : 0.92} />
        {animated && <path className="flow" d={d} stroke={col} />}
        {hoverCable === c.id && (
          <g className="cabdel" transform={`translate(${mid.x} ${mid.y})`} onClick={() => setConns((cs) => cs.filter((x) => x.id !== c.id))}>
            <circle r="9" />
            <path d="M -3 -3 L 3 3 M 3 -3 L -3 3" />
          </g>
        )}
      </g>
    );
  });

  /* ghost cable */
  let ghost = null;
  if (wire) {
    const out = wire.dir === "out";
    const a = out ? wire.start : wire.mouse;
    const b = out ? wire.mouse : wire.start;
    const col = wire.kind === "param" ? "var(--param)" : "var(--accent)";
    ghost = <path className="wire" d={mwCablePath(a, b, cableStyle, "r", "l")} stroke={col} opacity="0.85" strokeDasharray="2 7" />;
  }

  return (
    <div ref={canvasRef}
      className={"mw-canvas" + (panning ? " panning" : "") + (wire ? " wiring" : "")}
      onPointerDown={onCanvasDown} onWheel={onWheel}>
      <div className={"mw-grid " + grid} style={{ backgroundPosition: `${viewport.x}px ${viewport.y}px`, backgroundSize: `${22 * viewport.zoom}px ${22 * viewport.zoom}px` }}></div>

      <div className="mw-world" style={{ transform: `translate(${viewport.x}px, ${viewport.y}px) scale(${viewport.zoom})` }}>
        <svg className="mw-cables" width="3000" height="2000">
          {cableEls}
          {ghost}
        </svg>
        {nodes.map((n) => (
          <MwNode key={n.id} node={n} style={nodeStyle} selected={selected === n.id}
            wiring={wire} target={target} portHandlers={portHandlers}
            dragging={ia.current && ia.current.type === "node" && ia.current.nodeId === n.id}
            onHeaderDown={onHeaderDown} onSelect={setSelected} onToggle={props.onToggle} />
        ))}
      </div>

      <div className="mw-hint">drag header to move · drag a port to wire · hover a cable to delete · scroll to zoom</div>

      <div className="mw-zoom">
        <button onClick={() => zoomBy(1 / 1.15)} title="Zoom out">−</button>
        <span className="zval">{Math.round(viewport.zoom * 100)}%</span>
        <button onClick={() => zoomBy(1.15)} title="Zoom in">+</button>
        <button onClick={() => setViewport({ x: 0, y: 0, zoom: 1 })} title="Reset view" style={{ fontSize: 14 }}>⤢</button>
      </div>
    </div>
  );
}

window.MwCanvas = MwCanvas;

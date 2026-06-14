/* Main Window v2 — interactive canvas: pan / zoom, node drag,
   port→port wiring with the labelled-pin model, hover-to-delete cables. */

const { useState: u2State, useRef: u2Ref, useEffect: u2Effect, useCallback: u2CB } = React;

function MwCanvas2(props) {
  const { nodes, conns, setNodes, setConns, viewport, setViewport,
          selected, setSelected, snap } = props;

  const canvasRef = u2Ref(null);
  const ia = u2Ref(null);
  const [wire, setWire] = u2State(null);
  const [target, setTarget] = u2State(null);
  const [hoverCable, setHoverCable] = u2State(null);
  const [panning, setPanning] = u2State(false);

  const nodeMap = {};
  nodes.forEach((n) => { nodeMap[n.id] = n; });

  const fitScale = () => {
    const el = canvasRef.current; if (!el) return 1;
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

  const onHeaderDown = u2CB((nodeId, e) => {
    const n = nodeMap[nodeId]; if (!n) return;
    ia.current = { type: "node", nodeId, startWorld: toWorld(e), nodeStart: { x: n.x, y: n.y } };
    setSelected(nodeId);
  }, [nodes, viewport]);

  const onPortDown = u2CB((nodeId, portId, e) => {
    const n = nodeMap[nodeId], p = m2Pin(n, portId);
    const start = m2PortPos(n, portId);
    ia.current = { type: "wire", fromNode: nodeId, fromPort: portId, kind: p.kind, dir: p.dir };
    setWire({ kind: p.kind, dir: p.dir, start, mouse: toWorld(e) });
  }, [nodes, viewport]);

  const onPortEnter = u2CB((nodeId, portId) => {
    const w = ia.current;
    if (!w || w.type !== "wire" || nodeId === w.fromNode) return;
    if (!m2Compatible(nodeMap[w.fromNode], w.fromPort, nodeMap[nodeId], portId)) return;
    setTarget({ node: nodeId, port: portId });
  }, [nodes]);
  const onPortLeave = u2CB((nodeId, portId) => {
    setTarget((t) => (t && t.node === nodeId && t.port === portId ? null : t));
  }, []);

  const connect = (aN, aP, bN, bP, kind) => {
    let fromN = aN, fromP = aP, toN = bN, toP = bP;
    if (m2Pin(nodeMap[aN], aP).dir === "in") { fromN = bN; fromP = bP; toN = aN; toP = aP; }
    setConns((cs) => {
      if (cs.some((c) => c.from.node === fromN && c.from.port === fromP && c.to.node === toN && c.to.port === toP)) return cs;
      const filtered = cs.filter((c) => !(c.to.node === toN && c.to.port === toP));
      return [...filtered, { id: m2Uid("c"), from: { node: fromN, port: fromP }, to: { node: toN, port: toP }, kind }];
    });
  };

  const onCanvasDown = u2CB((e) => {
    setSelected(null);
    ia.current = { type: "pan", startScreen: { x: e.clientX, y: e.clientY }, vpStart: { ...viewport } };
    setPanning(true);
  }, [viewport]);

  u2Effect(() => {
    const move = (e) => {
      const it = ia.current; if (!it) return;
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
        setViewport({ ...it.vpStart, x: it.vpStart.x + (e.clientX - it.startScreen.x) / s, y: it.vpStart.y + (e.clientY - it.startScreen.y) / s });
      }
    };
    const up = () => {
      const it = ia.current;
      if (it && it.type === "wire") {
        setTarget((t) => { if (t) connect(it.fromNode, it.fromPort, t.node, t.port, it.kind); return null; });
        setWire(null);
      }
      if (it && it.type === "pan") setPanning(false);
      ia.current = null;
    };
    window.addEventListener("pointermove", move);
    window.addEventListener("pointerup", up);
    return () => { window.removeEventListener("pointermove", move); window.removeEventListener("pointerup", up); };
  }, [viewport, snap]);

  const onWheel = u2CB((e) => {
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
    const cx = 720, cy = 320;
    const z2 = Math.max(0.4, Math.min(2.2, vp.zoom * factor));
    const wx = (cx - vp.x) / vp.zoom, wy = (cy - vp.y) / vp.zoom;
    return { zoom: z2, x: cx - wx * z2, y: cy - wy * z2 };
  });

  u2Effect(() => {
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

  const handlers = { onDown: onPortDown, onEnter: onPortEnter, onLeave: onPortLeave };

  /* cables */
  const cableEls = conns.map((c) => {
    const fn = nodeMap[c.from.node], tn = nodeMap[c.to.node];
    if (!fn || !tn) return null;
    const a = m2PortPos(fn, c.from.port), b = m2PortPos(tn, c.to.port);
    const d = m2CablePath(a, b);
    const col = c.kind === "param" ? "var(--param)" : "var(--accent)";
    const mid = { x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 };
    return (
      <g key={c.id} onMouseEnter={() => setHoverCable(c.id)} onMouseLeave={() => setHoverCable((h) => (h === c.id ? null : h))}>
        <path className="hit" d={d} />
        <path className="wire-glow" d={d} stroke={col} />
        <path className="wire" d={d} stroke={col} />
        {hoverCable === c.id && (
          <g className="cabdel" transform={`translate(${mid.x} ${mid.y})`} onClick={() => setConns((cs) => cs.filter((x) => x.id !== c.id))}>
            <circle r="9" /><path d="M -3 -3 L 3 3 M 3 -3 L -3 3" />
          </g>
        )}
      </g>
    );
  });

  let ghost = null;
  if (wire) {
    const out = wire.dir === "out";
    const a = out ? wire.start : wire.mouse;
    const b = out ? wire.mouse : wire.start;
    const col = wire.kind === "param" ? "var(--param)" : "var(--accent)";
    ghost = <path className="wire" d={m2CablePath(a, b)} stroke={col} strokeDasharray="3 8" opacity="0.85" />;
  }

  return (
    <div ref={canvasRef}
      className={"m2-canvas" + (panning ? " panning" : "") + (wire ? " wiring" : "")}
      onPointerDown={onCanvasDown} onWheel={onWheel}>
      <div className="m2-grid" style={{ backgroundPosition: `${viewport.x}px ${viewport.y}px`, backgroundSize: `${24 * viewport.zoom}px ${24 * viewport.zoom}px` }}></div>

      <div className="m2-world" style={{ transform: `translate(${viewport.x}px, ${viewport.y}px) scale(${viewport.zoom})` }}>
        <svg className="m2-cables" width="4000" height="2600">{cableEls}{ghost}</svg>
        {nodes.map((n) => (
          <MwNode2 key={n.id} node={n} selected={selected === n.id}
            dragging={ia.current && ia.current.type === "node" && ia.current.nodeId === n.id}
            wiring={wire} target={target} handlers={handlers}
            onSelect={setSelected} onHeaderDown={onHeaderDown} onToggle={props.onToggle} />
        ))}
      </div>

      <div className="m2-zoom">
        <button onClick={() => zoomBy(1 / 1.15)} title="Zoom out">−</button>
        <span className="zv">{Math.round(viewport.zoom * 100)}%</span>
        <button onClick={() => zoomBy(1.15)} title="Zoom in">+</button>
        <button onClick={() => setViewport({ x: 0, y: 0, zoom: 1 })} title="Fit" style={{ fontSize: 13 }}>⤢</button>
      </div>
    </div>
  );
}

window.MwCanvas2 = MwCanvas2;

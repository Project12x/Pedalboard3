/* Main Window — a single processor / IO node.
   Renders differently per nodeStyle: classic | module | signal.
   Ports raise onPortDown / onPortEnter / onPortLeave for wiring. */

function MwPort({ nodeId, portId, kind, cls, live, target, style, onDown, onEnter, onLeave }) {
  return (
    <div
      className={"mw-port " + kind + " " + cls + (target ? " tgt" : "") + (live ? " live" : "")}
      style={style}
      onPointerDown={(e) => { e.stopPropagation(); onDown(nodeId, portId, e); }}
      onPointerEnter={() => onEnter(nodeId, portId)}
      onPointerLeave={() => onLeave(nodeId, portId)}
    ></div>
  );
}

/* build the port elements for a node with correct inline positioning */
function mwBuildPorts(node, wiring, target, portHandlers) {
  const w = node.w || MW_NODE_W;
  const els = [];
  const add = (portId, kind, cls, style) => {
    const m = mwPortMeta(portId);
    els.push(
      <MwPort key={portId} nodeId={node.id} portId={portId} kind={kind} cls={cls} style={style}
        live={!!wiring && wiring.kind === m.kind && wiring.dir !== m.dir}
        target={!!target && target.node === node.id && target.port === portId}
        {...portHandlers} />
    );
  };
  const sideStyle = { top: MW_PORT_Y - 7.5 };
  if (node.ports.in)   add("in",   "audio", "pin-l",  sideStyle);
  if (node.ports.out)  add("out",  "audio", "pout-r", sideStyle);
  if (node.ports.pout) add("pout", "param", "pout-r", sideStyle);
  if (node.ports.pin)  add("pin",  "param", "ppin-t", { left: w / 2 - 7.5 });
  return els;
}

function MwNode(props) {
  const { node, style, selected, density, wiring, target, portHandlers, onHeaderDown, onSelect, onToggle } = props;
  const cat = MW_CAT[node.cat] || MW_CAT.io;
  const positioned = mwBuildPorts(node, wiring, target, portHandlers);

  // ---- IO puck ----
  if (node.kind === "io") {
    return (
      <div className={"mw-node puck" + (selected ? " sel" : "")}
        style={{ left: node.x, top: node.y, "--cat": cat.color }}
        onPointerDown={(e) => { onSelect(node.id); onHeaderDown(node.id, e); }}>
        <div className="mw-puck">
          <span className="pic">{node.io === "in" ? MwIcon.jackIn() : MwIcon.jackOut()}</span>
          <span className="pl">{node.io === "in" ? "Input" : "Output"}</span>
        </div>
        {positioned}
      </div>
    );
  }

  const showKnobs = style === "module";
  const showMeter = style === "signal";
  const showTone = style !== "classic";

  return (
    <div className={"mw-node" + (selected ? " sel" : "") + (node.bypass ? " bypassed" : "") + (node.mute ? " muted" : "") + (props.dragging ? " dragging" : "")}
      style={{ left: node.x, top: node.y, "--cat": cat.color }}
      onPointerDown={(e) => { e.stopPropagation(); onSelect(node.id); }}>
      <div className="mw-nhead" onPointerDown={(e) => { e.stopPropagation(); onSelect(node.id); onHeaderDown(node.id, e); }}>
        <span className="cdot"></span>
        <b>{node.name}</b>
        <span className="cpu">{node.cpu.toFixed(1)}%</span>
      </div>
      <div className="mw-nbody">
        {showMeter && <MwMeter />}
        {showTone && <div className="mw-tone">{node.tone}</div>}
        {showKnobs && (
          <div className="mw-knobs">
            {node.params.slice(0, 3).map((p, i) => <MwKnob key={i} label={p.label} disp={p.disp} val={p.val} />)}
          </div>
        )}
        <div className="mw-bsm">
          <button className={"b" + (node.bypass ? " on b" : "")} onPointerDown={(e) => e.stopPropagation()} onClick={(e) => { e.stopPropagation(); onToggle(node.id, "bypass"); }}>BYP</button>
          <button className={"s" + (node.solo ? " on s" : "")} onPointerDown={(e) => e.stopPropagation()} onClick={(e) => { e.stopPropagation(); onToggle(node.id, "solo"); }}>SOLO</button>
          <button className={"m" + (node.mute ? " on m" : "")} onPointerDown={(e) => e.stopPropagation()} onClick={(e) => { e.stopPropagation(); onToggle(node.id, "mute"); }}>MUTE</button>
        </div>
      </div>
      {positioned}
    </div>
  );
}

window.MwNode = MwNode;

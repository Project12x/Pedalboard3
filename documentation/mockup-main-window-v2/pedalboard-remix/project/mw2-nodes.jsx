/* Main Window v2 — node renderers.
   Dispatches on node.type: source | io-mini | module | plugin | mod |
   ir-loader | effect-rack | mixer | splitter | vu | tuner | note | output. */

/* ---- port connectors + edge labels ---- */
function M2Pins({ node, wiring, target, handlers }) {
  return (node.pins || []).map((p) => {
    const top = M2.HEAD + p.dy;
    const isTgt = target && target.node === node.id && target.port === p.id;
    const live = wiring && wiring.kind === p.kind && wiring.dir !== p.dir;
    const conn = (
      <span key={"c-" + p.id}
        className={"m2-conn " + p.kind + " " + p.side + (isTgt ? " tgt" : "") + (live ? " live" : "")}
        style={{ top: top - 8, [p.side === "l" ? "left" : "right"]: -8 }}
        onPointerDown={(e) => { e.stopPropagation(); handlers.onDown(node.id, p.id, e); }}
        onPointerEnter={() => handlers.onEnter(node.id, p.id)}
        onPointerLeave={() => handlers.onLeave(node.id, p.id)}
      ></span>
    );
    if (!p.label) return conn;
    const lab = (
      <span key={"l-" + p.id} className={"m2-plabel " + p.side + " " + p.kind}
        style={{ top: top - 9, [p.side === "l" ? "left" : "right"]: 13 }}>{p.label}</span>
    );
    return [conn, lab];
  });
}

/* ---- standard node frame ---- */
function NodeFrame({ node, cat, selected, dragging, onSelect, onHeaderDown, onToggle, children, pins, bodyStyle, className, hideMB }) {
  return (
    <div className={"m2-node " + (className || "") + (selected ? " sel" : "") + (node.bypass ? " byp" : "") + (node.mute ? " mut" : "") + (dragging ? " drag" : "")}
      style={{ left: node.x, top: node.y, width: node.w, "--cat": cat.color }}
      onPointerDown={(e) => { e.stopPropagation(); onSelect(node.id); }}>
      <div className="m2-head" onPointerDown={(e) => { e.stopPropagation(); onSelect(node.id); onHeaderDown(node.id, e); }}>
        <span className="hdot"></span>
        <b>{node.name}</b>
        {node.cpu != null && <span className="hcpu">{node.cpu.toFixed(1)}%</span>}
        <button className="hx" title="Remove" onPointerDown={(e) => e.stopPropagation()} onClick={(e) => { e.stopPropagation(); onToggle(node.id, "__remove"); }}>{M2Icon.close()}</button>
      </div>
      <div className="m2-body" style={bodyStyle}>{children}</div>
      {!hideMB && (
        <div className="m2-foot">
          <button className={"fb" + (node.__edit ? " on" : "")} title="Edit" onPointerDown={(e) => e.stopPropagation()} onClick={(e) => { e.stopPropagation(); onToggle(node.id, "__edit"); }}>e</button>
          <button className={"fb" + (node.mute ? " on" : "")} title="Mute" onPointerDown={(e) => e.stopPropagation()} onClick={(e) => { e.stopPropagation(); onToggle(node.id, "mute"); }}>m</button>
          <span className="fspace"></span>
          <button className={"fb byp" + (node.bypass ? " on" : "")} title="Bypass" onPointerDown={(e) => e.stopPropagation()} onClick={(e) => { e.stopPropagation(); onToggle(node.id, "bypass"); }}>b</button>
        </div>
      )}
      {pins}
    </div>
  );
}

/* ---- warm amp-chassis frame (NAM Loader / IR Loader) ---- */
function ChassisFrame({ node, eyebrow, titleText, hasContent, selected, dragging, onSelect, onHeaderDown, onToggle, children, pins }) {
  return (
    <div className={"m2-node n-chassis" + (selected ? " sel" : "") + (node.bypass ? " byp" : "") + (node.mute ? " mut" : "") + (dragging ? " drag" : "")}
      style={{ left: node.x, top: node.y, width: node.w }}
      onPointerDown={(e) => { e.stopPropagation(); onSelect(node.id); }}>
      <span className="nc-screw tl"></span><span className="nc-screw tr"></span>
      <span className="nc-screw bl"></span><span className="nc-screw br"></span>

      <div className="nc-head" onPointerDown={(e) => { e.stopPropagation(); onSelect(node.id); onHeaderDown(node.id, e); }}>
        <div className="nc-head-l">
          <div className="nc-eyebrow">{eyebrow}</div>
          <div className={"nc-title" + (hasContent ? "" : " empty")}>{titleText}</div>
        </div>
        <div className="nc-head-r">
          <span className={"nc-status" + (hasContent ? " on" : "")}><i></i>{hasContent ? "Active" : "Bypass"}</span>
          {node.cpu != null && <span className="nc-cpu">{node.cpu.toFixed(1)}%</span>}
          <button className="nc-x" title="Remove" onPointerDown={(e) => e.stopPropagation()} onClick={(e) => { e.stopPropagation(); onToggle(node.id, "__remove"); }}>{M2Icon.close()}</button>
        </div>
      </div>

      <div className="nc-body">{children}</div>

      <div className="m2-foot">
        <button className={"fb" + (node.__edit ? " on" : "")} title="Edit" onPointerDown={(e) => e.stopPropagation()} onClick={(e) => { e.stopPropagation(); onToggle(node.id, "__edit"); }}>e</button>
        <button className={"fb" + (node.mute ? " on" : "")} title="Mute" onPointerDown={(e) => e.stopPropagation()} onClick={(e) => { e.stopPropagation(); onToggle(node.id, "mute"); }}>m</button>
        <span className="fspace"></span>
        <button className={"fb byp" + (node.bypass ? " on" : "")} title="Bypass" onPointerDown={(e) => e.stopPropagation()} onClick={(e) => { e.stopPropagation(); onToggle(node.id, "bypass"); }}>b</button>
      </div>
      {pins}
    </div>
  );
}

/* ---- mini chassis knob (SVG, 270° arc) ---- */
function NcKnob({ label, val = 0.5, size = 34 }) {
  const color = "var(--amp-accent)";
  const r = 11, c = 2 * Math.PI * r, sweep = 0.75;
  const arc = Math.max(0, Math.min(1, val)) * sweep * c;
  const ang = 135 + Math.max(0, Math.min(1, val)) * 270;
  const rad = (ang - 90) * Math.PI / 180;
  return (
    <div className="nc-knob">
      <svg width={size} height={size} viewBox="0 0 32 32">
        <g transform="rotate(135 16 16)">
          <circle cx="16" cy="16" r={r} fill="none" strokeWidth="2.5" strokeLinecap="round"
            stroke="color-mix(in srgb, var(--amp-text) 14%, transparent)"
            strokeDasharray={`${sweep * c} ${c}`} />
          <circle cx="16" cy="16" r={r} fill="none" strokeWidth="2.5" strokeLinecap="round"
            stroke={color} strokeDasharray={`${arc} ${c}`}
            style={{ filter: "drop-shadow(0 0 2.5px " + color + ")" }} />
        </g>
        <circle cx="16" cy="16" r="9.5" fill="var(--amp-edge-hi)" />
        <circle cx="16" cy="16" r="8.5" fill="var(--amp-knob-face)" stroke="var(--amp-edge)" strokeWidth="0.8" />
        <line x1={16 + 3.5 * Math.cos(rad)} y1={16 + 3.5 * Math.sin(rad)}
              x2={16 + 7.5 * Math.cos(rad)} y2={16 + 7.5 * Math.sin(rad)}
              stroke="var(--amp-text)" strokeWidth="2" strokeLinecap="round" />
      </svg>
      <span className="nc-kv">{(val * 10).toFixed(1)}</span>
      <span className="nc-kl">{label}</span>
    </div>
  );
}

/* ---- chassis gain slider row ---- */
function NcGainRow({ label, disp, val }) {
  return (
    <div className="nc-ctl">
      <span className="nc-ctl-l">{label}</span>
      <div className="nc-track">
        <div className="nc-fill" style={{ width: (val * 100) + "%" }}></div>
        <div className="nc-thumb" style={{ left: (val * 100) + "%" }}></div>
      </div>
      <span className="nc-val">{disp}</span>
    </div>
  );
}

/* ---- mini nested-graph preview for the Effect Rack (sub-graph) ---- */
function RackGraph({ contained }) {
  const yMid = 44;
  const n = Math.min(contained.length, 3);
  const items = contained.slice(0, n);
  const nodeW = 32, nodeH = 28;
  const startX = 64, endX = 138;
  const step = n > 1 ? (endX - startX) / (n - 1) : 0;
  const xs = []; for (let i = 0; i < n; i++) xs.push(n > 1 ? startX + i * step : 100);
  const inX = 16, outX = 184;
  const stops = [{ x: inX, edge: 4 }, ...xs.map((x) => ({ x, edge: nodeW / 2 })), { x: outX, edge: 4 }];
  const cables = [];
  for (let i = 0; i < stops.length - 1; i++) {
    const a = stops[i], b = stops[i + 1];
    const ax = a.x + a.edge, bx = b.x - b.edge;
    const dx = Math.max(10, (bx - ax) * 0.45);
    cables.push(<path key={i} d={`M${ax},${yMid} C${ax + dx},${yMid} ${bx - dx},${yMid} ${bx},${yMid}`}
      fill="none" stroke="var(--cat)" strokeWidth="1.7" opacity="0.55" />);
  }
  return (
    <svg className="rack-graph" viewBox="0 0 200 88">
      {cables}
      <circle cx={inX} cy={yMid} r="5" fill="var(--node-bg)" stroke="var(--cat)" strokeWidth="1.6" />
      <text x={inX} y={yMid + 17} className="rg-io" textAnchor="middle">IN</text>
      {items.map((it, i) => (
        <g key={i}>
          <rect x={xs[i] - nodeW / 2} y={yMid - nodeH / 2} width={nodeW} height={nodeH} rx="4.5" fill="var(--node-bg)" stroke="var(--border)" strokeWidth="1" />
          <path d={`M${xs[i] - nodeW / 2 + 1},${yMid - nodeH / 2 + 5} a4,4 0 0 1 4,-4 h${nodeW - 10} a4,4 0 0 1 4,4 v1 h-${nodeW - 2} z`} fill={it.color} opacity="0.92" />
          <text x={xs[i]} y={yMid + 5.5} className="rg-label" textAnchor="middle">{it.short}</text>
        </g>
      ))}
      <circle cx={outX} cy={yMid} r="5" fill="var(--node-bg)" stroke="var(--cat)" strokeWidth="1.6" />
      <text x={outX} y={yMid + 17} className="rg-io" textAnchor="middle">OUT</text>
    </svg>
  );
}

/* ======================================================================
   MAIN DISPATCHER
   ====================================================================== */
function MwNode2(props) {
  const { node, selected, dragging, wiring, target, handlers, onSelect, onHeaderDown, onToggle } = props;
  const cat = M2_CAT[node.cat] || M2_CAT.source;

  const labeledIO = node.type === "module" || node.type === "mod" || node.type === "plugin";
  const labeledDys = labeledIO ? (node.pins || []).filter((p) => p.label && (p.side === "l" || p.side === "r")).map((p) => p.dy) : [];
  const reserve = labeledDys.length ? Math.max(...labeledDys) + 14 : null;

  const pins = <M2Pins node={node} wiring={wiring} target={target} handlers={handlers} />;

  const frame = (cls, body, opts = {}) => (
    <NodeFrame node={node} cat={cat} selected={selected} dragging={dragging}
      onSelect={onSelect} onHeaderDown={onHeaderDown} onToggle={onToggle} className={cls} hideMB={opts.hideMB}
      bodyStyle={opts.reserve === false ? null : (reserve ? { paddingTop: reserve } : null)}
      pins={pins}>
      {body}
    </NodeFrame>
  );

  switch (node.type) {

    /* ---- Audio/MIDI/OSC source ---- */
    case "source":
      return frame("t-source", (
        <>
          <div className="m2-sub"><span className="dev-ic">{M2Icon.mic()}</span>{node.sub}</div>
          <div className="m2-faders">{node.faders.map((f, i) => <M2Fader key={i} label={f.label} db={f.db} val={0.55 + i * 0.04} />)}</div>
        </>
      ), { hideMB: true });

    case "io-mini": {
      const ic = node.name.includes("Virtual") ? M2Icon.piano() : node.name.includes("OSC") ? M2Icon.osc() : M2Icon.midi();
      return frame("t-mini", (
        <div className="m2-sub mini"><span className="dev-ic">{ic}</span>{node.sub}</div>
      ), { hideMB: true });
    }

    /* ---- Generic processor (polished: preset pill + recessed knob deck) ---- */
    case "module":
      return frame("t-module", (
        <>
          <div className="m2-preset">
            <span className="m2-preset-led"></span>
            <span className="m2-preset-name">{node.tone}</span>
            <span className="m2-preset-nav">{M2Icon.chev()}</span>
          </div>
          <div className="m2-knob-deck">{node.params.map((p, i) => <M2Knob key={i} label={p[0]} disp={p[1]} val={p[2]} />)}</div>
        </>
      ));

    case "mod":
      return frame("t-mod", (
        <>
          <div className="m2-preset mod">
            <span className="m2-preset-led"></span>
            <span className="m2-preset-name">{node.tone}</span>
            <span className="m2-preset-nav">{M2Icon.chev()}</span>
          </div>
          <div className="m2-knob-deck">{node.params.map((p, i) => <M2Knob key={i} label={p[0]} disp={p[1]} val={p[2]} color="var(--param)" />)}</div>
        </>
      ));

    /* ================================================================
       NAM LOADER — warm amp-chassis aesthetic
       ================================================================ */
    case "plugin": {
      const loaded = node.model && node.model !== "No Model";
      const openEdit = (e) => { e.stopPropagation(); onToggle(node.id, "__edit"); };
      return (
        <ChassisFrame node={node} eyebrow="NAM Loader" titleText={loaded ? node.model : "No model loaded"}
          hasContent={loaded} selected={selected} dragging={dragging}
          onSelect={onSelect} onHeaderDown={onHeaderDown} onToggle={onToggle} pins={pins}>

          {/* Signal Chain */}
          <div className="nc-sect">
            <div className="nc-sect-h"><span className="nc-dot"></span><span className="nc-sect-t">Signal Chain</span></div>
            <div className="nc-row">
              <button className="nc-btn primary" onPointerDown={(e) => e.stopPropagation()} onClick={openEdit}>Load Model</button>
              <button className="nc-btn" onPointerDown={(e) => e.stopPropagation()} onClick={openEdit}>Browse…</button>
            </div>
            <div className={"nc-field" + (loaded ? " loaded" : "")}>
              <span className="nc-field-t">{loaded ? node.model : "No Model Loaded"}</span>
              {loaded && <span className="nc-tag">NAM</span>}
            </div>
            <div className="nc-slots">
              {[{ key: "ir1", lab: "IR 1" }, { key: "ir2", lab: "IR 2" }].map(({ key, lab }) => (
                <div className="nc-slot" key={key}>
                  <div className="nc-slot-h">
                    <span className="nc-slot-lab">{lab}</span>
                    <span className={"nc-tog" + (node[key] ? " on" : "")}><span className="nc-tdot"></span>On</span>
                  </div>
                  <div className={"nc-field" + (node[key] ? " loaded" : "")}>
                    <span className="nc-field-t">{node[key] || "No IR"}</span>
                    {node[key] && <span className="nc-tag ir">IR</span>}
                  </div>
                </div>
              ))}
            </div>
          </div>

          {/* Gain */}
          <div className="nc-sect">
            <div className="nc-sect-h"><span className="nc-dot b"></span><span className="nc-sect-t">Gain</span></div>
            {node.gain.map((g, i) => <NcGainRow key={i} label={g[0]} disp={g[1]} val={g[2]} />)}
          </div>

          {/* Tone */}
          <div className="nc-sect">
            <div className="nc-sect-h"><span className="nc-dot"></span><span className="nc-sect-t">Tone</span></div>
            <div className="nc-knobs">
              {node.tone.map((t, i) => <NcKnob key={i} label={t[0]} val={t[1]} />)}
            </div>
          </div>
        </ChassisFrame>
      );
    }

    /* ================================================================
       IR LOADER — chassis aesthetic, dual IR slots
       ================================================================ */
    case "ir-loader": {
      const ir1Loaded = !!node.ir1;
      const ir2Loaded = !!node.ir2;
      const hasAny = ir1Loaded || ir2Loaded;
      return (
        <ChassisFrame node={node} eyebrow="IR Loader" titleText={ir1Loaded ? node.ir1 : "No IR loaded"}
          hasContent={hasAny} selected={selected} dragging={dragging}
          onSelect={onSelect} onHeaderDown={onHeaderDown} onToggle={onToggle} pins={pins}>

          <div className="nc-sect">
            <div className="nc-sect-h"><span className="nc-dot b"></span><span className="nc-sect-t">Impulse Responses</span></div>
            {[{ key: "ir1", lab: "Primary IR" }, { key: "ir2", lab: "Secondary IR" }].map(({ key, lab }) => (
              <div className="nc-slot" key={key} style={{ borderRadius: 8 }}>
                <div className="nc-slot-h">
                  <span className="nc-slot-lab">{lab}</span>
                  <span className={"nc-tog" + (node[key] ? " on" : "")}><span className="nc-tdot"></span>On</span>
                </div>
                <div className={"nc-field" + (node[key] ? " loaded" : "")}>
                  <span className="nc-field-t">{node[key] || "No file loaded"}</span>
                  {node[key] && <span className="nc-tag ir">IR</span>}
                </div>
              </div>
            ))}
          </div>

          <div className="nc-sect">
            <div className="nc-sect-h"><span className="nc-dot"></span><span className="nc-sect-t">Mix</span></div>
            <NcGainRow label="Blend" disp={Math.round(node.blend * 100) + "%"} val={node.blend} />
          </div>

          <div className="nc-sect">
            <div className="nc-sect-h"><span className="nc-dot b"></span><span className="nc-sect-t">Filter</span></div>
            <div className="nc-chips">
              <div className="nc-chip"><span className="nc-chip-l">Lo</span>{node.loCut >= 1000 ? (node.loCut / 1000).toFixed(1) + "k" : node.loCut} Hz</div>
              <div className="nc-chip"><span className="nc-chip-l">Hi</span>{node.hiCut >= 1000 ? (node.hiCut / 1000).toFixed(1) + "k" : node.hiCut} Hz</div>
            </div>
          </div>
        </ChassisFrame>
      );
    }

    /* ================================================================
       EFFECT RACK — nested sub-graph container (double-click to open)
       ================================================================ */
    case "effect-rack": {
      const contained = node.contains || [];
      const openRack = (e) => { e.stopPropagation(); onToggle(node.id, "__edit"); };
      return frame("t-rack", (
        <>
          <div className="rack-graph-wrap">
            <span className="rack-badge">SUB-GRAPH</span>
            <RackGraph contained={contained} />
          </div>
          <div className="rack-foot">
            <span className="rack-count"><b>{contained.length}</b>&nbsp;processor{contained.length === 1 ? "" : "s"} nested</span>
            <button className="rack-open" onPointerDown={(e) => e.stopPropagation()} onClick={openRack}>
              <svg viewBox="0 0 24 24" fill="none"><path d="M9 4h11v11M20 4 10 14M14 20H4V10" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round"/></svg>
              Open
            </button>
          </div>
        </>
      ), { reserve: false });
    }

    /* ================================================================
       DAW MIXER — generic numbered channel strips + master
       ================================================================ */
    case "mixer":
      return frame("t-mixer", (
        <div className="mix-container">
          {(node.strips || []).map((s, i) => (
            <div key={i} className="mix-strip">
              <span className="mix-num">{i + 1}</span>
              <div className="mix-pan"><span className="mix-pan-dot" style={{ left: (50 + s.pan * 46) + "%" }}></span></div>
              <div className="mix-fader-area">
                <div className="mix-vu" style={{ height: (s.mute ? 0 : Math.max(0.1, s.vol - 0.08) * 100) + "%" }}></div>
                <div className="mix-fader-fill" style={{ height: (s.vol * 100) + "%" }}></div>
                <div className="mix-fader-thumb" style={{ bottom: (s.vol * 100) + "%" }}></div>
              </div>
              <span className="mix-db">{s.db}</span>
              <span className={"mix-mute" + (s.mute ? " on" : "")}>M</span>
            </div>
          ))}
          <div className="mix-strip master">
            <span className="mix-num">M</span>
            <div className="mix-pan"><span className="mix-pan-dot" style={{ left: "50%" }}></span></div>
            <div className="mix-fader-area">
              <div className="mix-vu" style={{ height: "66%" }}></div>
              <div className="mix-fader-fill" style={{ height: "75%" }}></div>
              <div className="mix-fader-thumb" style={{ bottom: "75%" }}></div>
            </div>
            <span className="mix-db">0.0</span>
            <span className="mix-mute">M</span>
          </div>
        </div>
      ), { reserve: false });

    /* ================================================================
       DAW SPLITTER — 1 in → N numbered outputs
       ================================================================ */
    case "splitter":
      return frame("t-splitter", (
        <>
          <div className="spl-in-row">
            <span className="spl-io-badge">IN</span>
            <div className="spl-in-meter"><i style={{ width: "62%" }}></i></div>
          </div>
          <div className="spl-fan">
            <svg viewBox="0 0 180 22" preserveAspectRatio="none" width="100%" height="16">
              {(node.outputs || []).map((o, i, arr) => {
                const y2 = arr.length > 1 ? 4 + (i / (arr.length - 1)) * 14 : 11;
                return <path key={i} d={`M0,11 C60,11 100,${y2} 180,${y2}`} fill="none" stroke="var(--cat)" strokeWidth="1.5" opacity="0.5" />;
              })}
            </svg>
          </div>
          <div className="spl-outs">
            {(node.outputs || []).map((o, i) => (
              <div key={i} className="spl-out-row">
                <span className="spl-io-badge out">{i + 1}</span>
                <div className="spl-out-meter"><i style={{ width: o.level + "%" }}></i></div>
                <span className="spl-out-db">{o.db}</span>
              </div>
            ))}
          </div>
        </>
      ), { reserve: false });

    /* ================================================================
       VU METER
       ================================================================ */
    case "vu":
      return frame("t-vu", <M2VU />, { hideMB: true });

    /* ================================================================
       TUNER — polished note + cent deviation
       ================================================================ */
    case "tuner":
      return frame("t-tuner", <M2Tuner />, { hideMB: true });

    /* ================================================================
       NOTE — bespoke sticky annotation, no plugin controls
       ================================================================ */
    case "note":
      return frame("t-note", (
        <div className="note-body">{node.text}</div>
      ), { hideMB: true, reserve: false });

    /* ---- Audio output ---- */
    case "output":
      return frame("t-output", (
        <>
          <div className="m2-sub"><span className="dev-ic">{M2Icon.speaker()}</span>{node.sub}</div>
          <div className="m2-chmeters">{node.meters.map((m, i) => <M2ChannelMeter key={i} label={m.label} db={m.db} />)}</div>
        </>
      ), { hideMB: true });

    default:
      return frame("", <div className="m2-tone">{node.name}</div>);
  }
}

window.MwNode2 = MwNode2;

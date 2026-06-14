/* Scratch Mode — Instant Scratch Capture panel.
   Hero record control + live RAW/WET scope + elapsed timer, armed indicators,
   the session context baked into every take, a destination row, and a rich
   recent-takes history with reveal / play / reamp and a metadata detail.
   Full state machine: ready -> recording -> saving -> saved (toast). */

const { useState: scState, useRef: scuRef, useEffect: scuEffect, useCallback: scCB } = React;

const ScIcon = {
  folder: (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z" stroke="currentColor" strokeWidth="1.6"/></svg>),
  reveal: (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M7 17 17 7M9 7h8v8" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round"/></svg>),
  play: (p) => (<svg viewBox="0 0 24 24" {...p}><path d="M7 5l12 7-12 7z" fill="currentColor"/></svg>),
  reamp: (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M4 9a8 8 0 0 1 13-3l3 2M20 5v4h-4M20 15a8 8 0 0 1-13 3l-3-2M4 19v-4h4" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" strokeLinejoin="round"/></svg>),
  raw: (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M3 12h3l2-6 4 14 3-9 2 4h4" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round"/></svg>),
  wet: (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M12 3c4 5 6 8 6 11a6 6 0 0 1-12 0c0-3 2-6 6-11z" stroke="currentColor" strokeWidth="1.7" strokeLinejoin="round"/></svg>),
  warn: (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M12 4 2.5 20h19z" stroke="currentColor" strokeWidth="1.6" strokeLinejoin="round"/><path d="M12 10v4M12 17.5v.5" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round"/></svg>),
  chevR: (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M9 6l6 6-6 6" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"/></svg>),
  close: (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M6 6l12 12M18 6 6 18" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round"/></svg>),
};

function ArmRow({ ok, label, ch, icon }) {
  return (
    <div className={"sc-arm" + (ok ? " on" : "")}>
      <span className="sc-arm-led"></span>
      <span className="sc-arm-ic">{icon}</span>
      <span className="sc-arm-lab">{label}</span>
      <span className="sc-arm-ch">{ch} ch</span>
    </div>
  );
}

function CtxChip({ k, v, mono }) {
  return <div className="sc-ctx"><span className="sc-ctx-k">{k}</span><span className={"sc-ctx-v" + (mono ? " mono" : "")}>{v}</span></div>;
}

function TakeRow({ take, accent, selected, onSelect, onAction }) {
  const interrupted = !take.complete;
  return (
    <div className={"sc-take" + (selected ? " sel" : "") + (interrupted ? " bad" : "")} onClick={() => onSelect(take.id)}>
      <ScratchThumb data={take.thumb} color={interrupted ? "var(--t30)" : accent} />
      <div className="sc-take-main">
        <div className="sc-take-l1">
          <span className="sc-take-time">{scTimeOfDay(take.time)}</span>
          <span className="sc-take-patch">{take.patch}{take.patchIndex ? <em> · {take.patchIndex}</em> : null}</span>
        </div>
        <div className="sc-take-l2">
          <span className={"sc-pill ok"}>RAW</span>
          <span className={"sc-pill " + (interrupted ? "bad" : "ok")}>WET</span>
          <span className="sc-take-meta">{scClock(take.durationSec)} · {scFmtRateShort(take.sampleRate)} · {take.rawCh}/{take.wetCh}</span>
          {interrupted && <span className="sc-take-warn">{ScIcon.warn()} interrupted</span>}
        </div>
      </div>
      <div className="sc-take-act" onClick={(e) => e.stopPropagation()}>
        <button className="sc-iact" title="Play wet" onClick={() => onAction("play", take)}>{ScIcon.play()}</button>
        <button className="sc-iact" title="Reamp raw through current chain" onClick={() => onAction("reamp", take)}>{ScIcon.reamp()}</button>
        <button className="sc-iact" title="Reveal in folder" onClick={() => onAction("reveal", take)}>{ScIcon.reveal()}</button>
        <span className={"sc-take-go" + (selected ? " on" : "")}>{ScIcon.chevR()}</span>
      </div>
    </div>
  );
}

function TakeDetail({ take, onAction }) {
  const rows = [
    ["Patch", take.patch + (take.patchIndex ? " · " + take.patchIndex : "")],
    ["Device", take.device],
    ["Sample rate", scFmtRate(take.sampleRate)],
    ["Channels", "raw " + take.rawCh + " · wet " + take.wetCh],
    ["Master in / out", scFmtGain(take.inGainDb) + "  /  " + scFmtGain(take.outGainDb)],
    ["Duration", scClock(take.durationSec) + "  (" + Math.round(take.durationSec * take.sampleRate).toLocaleString() + " samples)"],
  ];
  return (
    <div className="sc-detail">
      {!take.complete && (
        <div className="sc-detail-warn">{ScIcon.warn()}<span><b>Capture interrupted.</b> {take.failureReason}. The wet print may be short — the raw DI is still usable.</span></div>
      )}
      <div className="sc-detail-grid">
        {rows.map(([k, v]) => <div className="sc-detail-row" key={k}><span>{k}</span><b>{v}</b></div>)}
      </div>
      <div className="sc-files">
        {[["raw.wav", take.rawFile, "Pre-chain DI"], ["wet.wav", take.wetFile, "What you heard"], ["take.json", take.metaFile, "Metadata"]].map(([n, path, sub]) => (
          <div className="sc-file" key={n}>
            <span className="sc-file-n">{n}</span><span className="sc-file-sub">{sub}</span>
            <span className="sc-file-path">{path}</span>
          </div>
        ))}
      </div>
      <div className="sc-detail-act">
        <button className="sc-btn ghost" onClick={() => onAction("reveal", take)}>{ScIcon.reveal()} Reveal folder</button>
        <button className="sc-btn ghost" onClick={() => onAction("play", take)}>{ScIcon.play()} Play wet</button>
        <button className="sc-btn primary" onClick={() => onAction("reamp", take)}>{ScIcon.reamp()} Reamp raw</button>
      </div>
    </div>
  );
}

function ScratchPanel({ theme, accentVar }) {
  const [phase, setPhase] = scState(SC_STATES.READY);
  const [elapsed, setElapsed] = scState(0);          // ms
  const [nonce, setNonce] = scState(0);
  const [takes, setTakes] = scState(() => scInitialTakes());
  const [sel, setSel] = scState(null);
  const startAt = scuRef(0);
  const tick = scuRef(0);

  const recording = phase === SC_STATES.RECORDING;
  const saving = phase === SC_STATES.SAVING;
  const ctx = SC_CONTEXT;

  const flash = (msg, kind) => {
    window.dispatchEvent(new CustomEvent("sc-toast", { detail: { msg, kind: kind || "ok" } }));
  };

  const start = scCB(() => {
    if (phase !== SC_STATES.READY) return;
    setNonce((n) => n + 1);
    startAt.current = performance.now();
    setElapsed(0);
    setPhase(SC_STATES.RECORDING);
  }, [phase]);

  const stop = scCB(() => {
    setPhase((p) => {
      if (p !== SC_STATES.RECORDING) return p;
      const dur = Math.max(1, (performance.now() - startAt.current) / 1000);
      setPhase(SC_STATES.SAVING);
      setTimeout(() => {
        const take = scMakeTake(ctx, dur, { time: new Date() });
        setTakes((ts) => [take, ...ts]);
        setPhase(SC_STATES.READY);
        setElapsed(0);
        flash("Scratch take saved · raw + wet printed", "ok");
      }, 650);
      return SC_STATES.SAVING;
    });
  }, [ctx]);

  const toggle = scCB(() => { (phase === SC_STATES.RECORDING) ? stop() : start(); }, [phase, start, stop]);

  // elapsed timer
  scuEffect(() => {
    if (!recording) return;
    tick.current = setInterval(() => setElapsed(performance.now() - startAt.current), 53);
    return () => clearInterval(tick.current);
  }, [recording]);

  // shortcut: Cmd/Ctrl+Shift+R, and Space when not focused in inputs
  scuEffect(() => {
    const onKey = (e) => {
      if ((e.metaKey || e.ctrlKey) && e.shiftKey && (e.key === "r" || e.key === "R")) { e.preventDefault(); toggle(); }
      else if (e.code === "Space" && e.target === document.body) { e.preventDefault(); toggle(); }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [toggle]);

  const onAction = (kind, take) => {
    if (kind === "reveal") flash("Revealing " + take.patch + " in Finder…", "ok");
    else if (kind === "play") flash("▶ Playing wet · " + take.patch, "ok");
    else if (kind === "reamp") flash("Reamping raw DI through current chain…", "accent");
  };

  const status = saving ? "Saving take…" : recording ? "Recording" : "Ready to capture";
  const todayCount = takes.filter((t) => { const n = new Date(); return t.time.getDate() === n.getDate(); }).length;
  const selTake = takes.find((t) => t.id === sel);

  return (
    <div className={"sc-panel theme-" + (theme || "midnight")} data-screen-label="Scratch Mode">
      {/* header */}
      <header className="sc-head">
        <div>
          <div className="sc-eyebrow">Pedalboard 3 · Instant Capture</div>
          <h1 className="sc-title">Scratch Mode</h1>
        </div>
        <div className="sc-shortcut" title="Toggle capture"><kbd>⌘</kbd><kbd>⇧</kbd><kbd>R</kbd></div>
      </header>

      {/* hero recorder */}
      <section className={"sc-hero " + phase}>
        <button className={"sc-rec " + phase} onClick={toggle} disabled={saving}
          title={recording ? "Stop capture" : "Start capture"}>
          <span className="sc-rec-ring"></span>
          <span className="sc-rec-glyph"></span>
        </button>

        <div className="sc-hero-main">
          <div className="sc-hero-top">
            <span className={"sc-dot " + phase}></span>
            <span className="sc-status">{status}</span>
            {recording && <span className="sc-rec-tag">● REC</span>}
          </div>
          <div className={"sc-timer" + (recording ? " live" : "")}>{scTimer(recording || saving ? elapsed : 0)}</div>
          <div className="sc-arms">
            <ArmRow ok label="Raw DI" ch={ctx.rawCh} icon={ScIcon.raw()} />
            <ArmRow ok label="Wet out" ch={ctx.wetCh} icon={ScIcon.wet()} />
          </div>
        </div>
      </section>

      {/* live scope */}
      <ScratchScope recording={recording} nonce={nonce} wetColor={accentVar} rawColor="rgba(168,180,200,0.85)" />

      {/* context baked into every take */}
      <div className="sc-ctxbar">
        <span className="sc-ctxbar-h">Captured with every take</span>
        <div className="sc-ctxrow">
          <CtxChip k="Patch" v={ctx.patch + " · " + ctx.patchIndex} />
          <CtxChip k="Device" v={ctx.device} />
          <CtxChip k="Rate" v={scFmtRate(ctx.sampleRate)} mono />
          <CtxChip k="In / Out" v={scFmtGain(ctx.inGainDb) + " / " + scFmtGain(ctx.outGainDb)} mono />
        </div>
      </div>

      {/* destination */}
      <div className="sc-dest">
        <span className="sc-dest-ic">{ScIcon.folder()}</span>
        <span className="sc-dest-path">{ctx.root}/<b>{scStamp(new Date()).split("/")[0]}</b>/</span>
        <button className="sc-btn ghost sm" onClick={() => flash("Revealing Scratch Ideas folder…", "ok")}>{ScIcon.reveal()} Reveal</button>
      </div>

      {/* recent takes */}
      <section className="sc-takes">
        <div className="sc-takes-h">
          <span className="sc-takes-t">Recent takes</span>
          <span className="sc-takes-n">{todayCount} today · {takes.length} total</span>
        </div>
        {takes.length === 0 ? (
          <div className="sc-empty">No scratch takes yet — hit record to capture your first idea.</div>
        ) : (
          <div className="sc-takelist">
            {takes.map((t) => (
              <React.Fragment key={t.id}>
                <TakeRow take={t} accent={accentVar} selected={sel === t.id}
                  onSelect={(id) => setSel((s) => (s === id ? null : id))} onAction={onAction} />
                {sel === t.id && <TakeDetail take={t} onAction={onAction} />}
              </React.Fragment>
            ))}
          </div>
        )}
      </section>
    </div>
  );
}

window.ScratchPanel = ScratchPanel;

/* Pedalboard 3 — integrated demo shell.
   Runs the Main Window as the hub. The menu-bar "Stage" button (and "Manage
   Setlist…") take over fullscreen with Stage Mode; the transport REC / Takes
   buttons float Scratch Mode as a draggable window; the NAM node editor +
   model browser already float from inside the Main Window. One running app. */

const DM = { useState: React.useState, useRef: React.useRef, useEffect: React.useEffect };

/* accent per theme id — mirrors SC_THEMES, used to tint the Scratch wet trace */
const SC_ACCENT = {
  midnight: "#00d9ff", "deep-ocean": "#00c8ff", synthwave: "#ff2bff",
  forest: "#66cc66", daylight: "#0077cc",
};

const DmIcon = {
  rec: () => (<svg viewBox="0 0 24 24" width="14" height="14"><circle cx="12" cy="12" r="6" fill="currentColor"/></svg>),
  close: () => (<svg viewBox="0 0 24 24" fill="none" width="15" height="15"><path d="M6 6l12 12M18 6 6 18" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round"/></svg>),
};

/* fit a fixed-size surface (w×h) into the viewport, letterboxed + centered */
function useFit(ref, w, h) {
  DM.useEffect(() => {
    const el = ref.current; if (!el) return;
    const fit = () => {
      const k = Math.min(window.innerWidth / w, window.innerHeight / h);
      if (k > 0 && isFinite(k)) el.style.transform = "scale(" + k + ")";
    };
    fit();
    const r = () => fit();
    window.addEventListener("resize", r);
    let ro = null;
    if (window.ResizeObserver) { ro = new ResizeObserver(fit); ro.observe(document.documentElement); }
    const raf = requestAnimationFrame(fit);
    return () => { window.removeEventListener("resize", r); if (ro) ro.disconnect(); cancelAnimationFrame(raf); };
  }, [w, h]);
}

function Scaled({ w, h, className, children }) {
  const ref = DM.useRef(null);
  useFit(ref, w, h);
  return (
    <div className="demo-fit">
      <div ref={ref} className={"demo-scale " + (className || "")} style={{ width: w, height: h }}>
        {children}
      </div>
    </div>
  );
}

/* a neutral, draggable OS-style window the secondary surfaces float in */
function FloatingWindow({ title, icon, width, onClose, children }) {
  const [pos, setPos] = DM.useState(() => ({
    x: Math.max(20, Math.min(window.innerWidth - width - 64, window.innerWidth - width - 64)),
    y: 84,
  }));
  const drag = DM.useRef(null);
  const onDown = (e) => {
    if (e.button !== 0) return;
    e.preventDefault();
    drag.current = { sx: e.clientX, sy: e.clientY, ox: pos.x, oy: pos.y };
    const move = (ev) => {
      const nx = drag.current.ox + (ev.clientX - drag.current.sx);
      const ny = drag.current.oy + (ev.clientY - drag.current.sy);
      setPos({
        x: Math.max(-width + 120, Math.min(window.innerWidth - 120, nx)),
        y: Math.max(0, Math.min(window.innerHeight - 56, ny)),
      });
    };
    const up = () => {
      window.removeEventListener("pointermove", move);
      window.removeEventListener("pointerup", up);
    };
    window.addEventListener("pointermove", move);
    window.addEventListener("pointerup", up);
  };
  return (
    <div className="demo-window" style={{ left: pos.x, top: pos.y, width }}>
      <div className="demo-win-bar" onPointerDown={onDown}>
        <span className="demo-win-ic">{icon}</span>
        <span className="demo-win-title">{title}</span>
        <button className="demo-win-x" onClick={onClose} title="Close">{DmIcon.close()}</button>
      </div>
      <div className="demo-win-body">{children}</div>
    </div>
  );
}

/* Scratch Mode body — the panel re-housed in a themed context that supplies
   the same CSS vars + button/box resets it gets from .sc-stage standalone,
   plus the stage-level "take saved" toast. */
function ScratchHost({ theme }) {
  const [toast, setToast] = DM.useState(null);
  const tt = DM.useRef(0);
  DM.useEffect(() => {
    const onToast = (e) => {
      setToast(e.detail);
      clearTimeout(tt.current);
      tt.current = setTimeout(() => setToast(null), 2600);
    };
    window.addEventListener("sc-toast", onToast);
    return () => { window.removeEventListener("sc-toast", onToast); clearTimeout(tt.current); };
  }, []);
  const accent = SC_ACCENT[theme] || SC_ACCENT.midnight;
  return (
    <div className={"sc-stage demo-sc-host theme-" + theme}>
      <ScratchPanel theme={theme} accentVar={accent} />
      {toast && (
        <div className={"sc-toast " + toast.kind}>
          <span className="sc-toast-led"></span>{toast.msg}
        </div>
      )}
    </div>
  );
}

function PedalboardDemo() {
  const [stage, setStage] = DM.useState(false);
  const [scratch, setScratch] = DM.useState(false);
  const [scratchTheme, setScratchTheme] = DM.useState("midnight");

  const openScratch = () => {
    let th = "midnight";
    try { th = JSON.parse(localStorage.getItem("mw2.theme")) || "midnight"; } catch {}
    // Scratch ships midnight/deep-ocean/synthwave/forest/daylight — main shares these ids
    setScratchTheme(SC_ACCENT[th] ? th : "midnight");
    setScratch(true);
  };

  return (
    <div className="demo-root">
      <Scaled w={1560} h={946} className="main">
        <MainWindow2
          onOpenStage={() => setStage(true)}
          onManageSetlist={() => setStage(true)}
          onOpenScratch={openScratch} />
      </Scaled>

      {scratch && (
        <FloatingWindow title="Scratch Mode · Instant Capture" icon={DmIcon.rec()} width={626}
          onClose={() => setScratch(false)}>
          <ScratchHost theme={scratchTheme} />
        </FloatingWindow>
      )}

      {stage && (
        <div className="demo-stage-overlay">
          <Scaled w={1280} h={800} className="stage">
            <StageMode onExit={() => setStage(false)} />
          </Scaled>
        </div>
      )}
    </div>
  );
}

window.PedalboardDemo = PedalboardDemo;

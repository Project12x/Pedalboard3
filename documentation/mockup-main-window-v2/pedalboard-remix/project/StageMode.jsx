/* StageMode — the single live performance surface.
   Shared chrome (top bar + view switcher + safety bar + tuner overlay)
   wraps three interchangeable views over one shared patch state. */

const { useState, useEffect, useCallback, useRef } = React;

const PB_VIEWS = [
  { id: "hero",    label: "Hero",    icon: PbIcon.hero },
  { id: "setlist", label: "Setlist", icon: PbIcon.list },
  { id: "grid",    label: "Grid",    icon: PbIcon.grid },
];

/* Five built-in themes (swatch = accent over background) */
const PB_THEMES = [
  { id: "midnight",   name: "Midnight",   accent: "#00d9ff", bg: "#1a1a2e" },
  { id: "synthwave",  name: "Synthwave",  accent: "#ff2bff", bg: "#0d0221" },
  { id: "deep-ocean", name: "Deep Ocean", accent: "#00c8ff", bg: "#0a1628" },
  { id: "forest",     name: "Forest",     accent: "#66cc66", bg: "#1a2f1a" },
  { id: "daylight",   name: "Daylight",   accent: "#0077cc", bg: "#e8e8e8" },
];

function useStored(key, init) {
  const [v, setV] = useState(() => {
    try { const s = localStorage.getItem(key); return s == null ? init : JSON.parse(s); } catch { return init; }
  });
  useEffect(() => { try { localStorage.setItem(key, JSON.stringify(v)); } catch {} }, [key, v]);
  return [v, setV];
}

function Clock() {
  const [t, setT] = useState(() => new Date());
  useEffect(() => { const id = setInterval(() => setT(new Date()), 10000); return () => clearInterval(id); }, []);
  const hh = String(t.getHours()).padStart(2, "0");
  const mm = String(t.getMinutes()).padStart(2, "0");
  return <span className="st-clock">{hh}:{mm}</span>;
}

function StageMode(props) {
  const onExit = props && props.onExit;
  const set = PB_SET;
  const [view, setView] = useStored("pb.stage.view", "setlist");
  const [theme, setTheme] = useStored("pb.stage.theme", "midnight");
  const [current, setCurrent] = useStored("pb.stage.current", 3);
  const [tuner, setTuner] = useState(false);
  const [panic, setPanic] = useState(false);
  const panicTimer = useRef(null);

  const now = set[Math.max(0, Math.min(current, set.length - 1))];

  const select = useCallback((i) => setCurrent(Math.max(0, Math.min(i, set.length - 1))), [set.length]);
  const prev = useCallback(() => setCurrent((c) => Math.max(0, c - 1)), []);
  const next = useCallback(() => setCurrent((c) => Math.min(set.length - 1, c + 1)), [set.length]);
  const firePanic = useCallback(() => {
    setPanic(true);
    clearTimeout(panicTimer.current);
    panicTimer.current = setTimeout(() => setPanic(false), 900);
  }, []);

  useEffect(() => {
    const onKey = (e) => {
      if (e.key === "ArrowLeft" || e.key === "ArrowUp") { prev(); e.preventDefault(); }
      else if (e.key === "ArrowRight" || e.key === "ArrowDown") { next(); e.preventDefault(); }
      else if (e.key === "1") setView("hero");
      else if (e.key === "2") setView("setlist");
      else if (e.key === "3") setView("grid");
      else if (e.key.toLowerCase() === "t") setTuner((t) => !t);
      else if (e.key.toLowerCase() === "p") firePanic();
      else if (e.key.toLowerCase() === "c") {
        const i = PB_THEMES.findIndex((x) => x.id === theme);
        setTheme(PB_THEMES[(i + 1) % PB_THEMES.length].id);
      }
      else if (e.key === "Escape" && onExit) { onExit(); e.preventDefault(); }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [prev, next, firePanic, theme, onExit]);

  return (
    <div className={"stage sm theme-" + theme + (panic ? " is-panic" : "")}>
      {/* ===== TOP BAR ===== */}
      <header className="sm-top">
        <div className="sm-left">
          <div className="st-brand"><span className="dot"></span>Stage Mode</div>
          <div className="sm-themes" role="group" aria-label="Theme">
            {PB_THEMES.map((t) => (
              <button
                key={t.id}
                className={"theme-dot" + (theme === t.id ? " on" : "")}
                style={{ "--td-accent": t.accent, "--td-bg": t.bg }}
                title={t.name}
                onClick={() => setTheme(t.id)}
              ></button>
            ))}
          </div>
        </div>

        <div className="sm-switch" role="tablist">
          {PB_VIEWS.map((v) => (
            <button
              key={v.id}
              className={"sm-seg" + (view === v.id ? " on" : "")}
              onClick={() => setView(v.id)}
            >
              {v.icon()}<span>{v.label}</span>
            </button>
          ))}
        </div>

        <div className="st-right">
          <Clock />
          <span className="st-sep"></span>
          <button className={"st-btn" + (tuner ? " on" : "")} onClick={() => setTuner((t) => !t)}>
            {PbIcon.tuner()}Tuner
          </button>
          <button className="st-btn" onClick={onExit}>{PbIcon.exit()}Exit</button>
        </div>
      </header>

      {/* ===== CONTENT ===== */}
      <main className="sm-content">
        {view === "hero" && <HeroView set={set} current={current} onSelect={select} onPrev={prev} onNext={next} />}
        {view === "setlist" && <SetlistView set={set} current={current} onSelect={select} />}
        {view === "grid" && <GridView set={set} current={current} onSelect={select} />}

        {/* tuner overlay — covers content, mutes output (like a stage tuner) */}
        {tuner && (
          <div className="sm-tuner-overlay">
            <div className="sm-tuner-card">
              <div className="sm-tuner-head">
                <span className="mute"><span className="d"></span>Output muted · tuning</span>
                <button className="st-btn" onClick={() => setTuner(false)}>{PbIcon.exit()}Close</button>
              </div>
              <PbStrobe note={now.note} cents={now.cents} size="big" />
              <div className="sm-tuner-ref">Reference A = 440 Hz · {now.key}</div>
            </div>
          </div>
        )}
      </main>

      {/* ===== SAFETY BAR ===== */}
      <footer className="sm-bottom">
        <div className="sm-b-tuner">
          <PbStrobe note={now.note} cents={now.cents} size="sm" />
        </div>
        <div className="sm-b-meters">
          <PbVu label="IN" base={[54, 49]} width={132} />
          <PbVu label="OUT" base={[70, 66]} width={132} />
        </div>
        <button className="panic sm-panic" onClick={firePanic}>{PbIcon.power()}Panic</button>
      </footer>

      {panic && <div className="sm-toast">Panic sent · All Notes Off</div>}
    </div>
  );
}

window.StageMode = StageMode;

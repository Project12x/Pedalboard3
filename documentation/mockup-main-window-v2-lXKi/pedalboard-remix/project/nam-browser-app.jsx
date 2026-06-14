/* NAM Browser — app shell. Backdrop + theme dots + finish tweaks, matching
   the NAM Loader. Amp tokens come from nam.css (.nam.theme-*). */

const NB_THEMES = [
  { id: "midnight",   accent: "#00d9ff", bg: "#1a1a2e" },
  { id: "deep-ocean", accent: "#00c8ff", bg: "#0a1628" },
  { id: "synthwave",  accent: "#ff2bff", bg: "#0d0221" },
  { id: "forest",     accent: "#66cc66", bg: "#1a2f1a" },
  { id: "daylight",   accent: "#0077cc", bg: "#e8e8e8" },
];

function NbApp() {
  const [theme, setTheme] = bState(() => { try { return JSON.parse(localStorage.getItem("nam.theme")) || "midnight"; } catch { return "midnight"; } });
  const [t, setTweak] = useTweaks({ backdrop: "Spotlight", brushed: false, bevel: false });
  React.useEffect(() => { try { localStorage.setItem("nam.theme", JSON.stringify(theme)); } catch {} }, [theme]);

  const panelRef = bRef(null);
  const fit = React.useCallback(() => {
    const el = panelRef.current; if (!el) return;
    el.style.transform = "scale(1)";
    const k = Math.min((window.innerWidth - 80) / el.offsetWidth, (window.innerHeight - 150) / el.offsetHeight, 1.5);
    el.style.transform = "scale(" + Math.max(0.4, k) + ")";
  }, []);
  React.useEffect(() => { fit(); const r = () => fit(); window.addEventListener("resize", r); return () => window.removeEventListener("resize", r); }, [fit]);
  React.useEffect(() => { const id = requestAnimationFrame(fit); return () => cancelAnimationFrame(id); }, [theme, fit]);

  const finish = (t.brushed ? " fin-brushed" : "") + (t.bevel ? " fin-bevel" : "");

  return (
    <div className={"nam-stage theme-" + theme + " bg-" + (t.backdrop || "Spotlight").toLowerCase()}>
      <div className="nam-topbar">
        <div className="nt-title"><span className="nt-glyph"></span>NAM Loader <span className="nt-sub">· Model Browser</span></div>
        <div className="nt-spacer"></div>
        <div className="nt-themes">
          {NB_THEMES.map((th) => (
            <button key={th.id} className={"nt-dot" + (theme === th.id ? " on" : "")}
              style={{ "--d": th.accent, "--b": th.bg }} title={th.id} onClick={() => setTheme(th.id)}></button>
          ))}
        </div>
      </div>

      <div className="nam-center">
        <div ref={panelRef} className="nam-scale">
          <NAMBrowser theme={theme} finish={finish} onClose={() => {}} />
        </div>
      </div>

      <div className="nam-hint">click a row to preview · double-click or Load to choose · switch Models / IRs · filter by type &amp; tone</div>

      <TweaksPanel>
        <TweakSection label="Finish" />
        <TweakToggle label="Brushed metal" value={t.brushed} onChange={(v) => setTweak("brushed", v)} />
        <TweakToggle label="Beveled edges" value={t.bevel} onChange={(v) => setTweak("bevel", v)} />
        <TweakSection label="Presentation" />
        <TweakRadio label="Backdrop" value={t.backdrop} options={["Spotlight", "Flat"]} onChange={(v) => setTweak("backdrop", v)} />
      </TweaksPanel>
    </div>
  );
}

window.NbApp = NbApp;

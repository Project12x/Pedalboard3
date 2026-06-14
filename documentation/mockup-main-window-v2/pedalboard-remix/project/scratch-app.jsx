/* Scratch Mode — app shell. Backdrop + theme dots + Tweaks. The panel reads
   the theme accent (for the WET waveform + highlights) from a CSS var resolved
   off the active theme. */

const { useState: scaState, useRef: scaRef, useEffect: scaEffect, useCallback: scaCB } = React;

const SC_THEMES = [
  { id: "midnight",   accent: "#00d9ff", bg: "#1a1a2e" },
  { id: "deep-ocean", accent: "#00c8ff", bg: "#0a1628" },
  { id: "synthwave",  accent: "#ff2bff", bg: "#0d0221" },
  { id: "forest",     accent: "#66cc66", bg: "#1a2f1a" },
  { id: "daylight",   accent: "#0077cc", bg: "#e8e8e8" },
];

function ScratchApp() {
  const [theme, setTheme] = scaState(() => { try { return JSON.parse(localStorage.getItem("scratch.theme")) || "midnight"; } catch { return "midnight"; } });
  const [t, setTweak] = useTweaks({ backdrop: "Spotlight" });
  const [toast, setToast] = scaState(null);
  const toastT = scaRef(0);
  scaEffect(() => { try { localStorage.setItem("scratch.theme", JSON.stringify(theme)); } catch {} }, [theme]);

  scaEffect(() => {
    const onToast = (e) => {
      setToast(e.detail);
      clearTimeout(toastT.current);
      toastT.current = setTimeout(() => setToast(null), 2600);
    };
    window.addEventListener("sc-toast", onToast);
    return () => window.removeEventListener("sc-toast", onToast);
  }, []);

  const accent = (SC_THEMES.find((x) => x.id === theme) || SC_THEMES[0]).accent;

  const panelRef = scaRef(null);
  const fit = scaCB(() => {
    const el = panelRef.current; if (!el) return;
    el.style.transform = "scale(1)";
    const k = Math.min((window.innerWidth - 80) / el.offsetWidth, (window.innerHeight - 150) / el.offsetHeight, 1.4);
    el.style.transform = "scale(" + Math.max(0.45, k) + ")";
  }, []);
  scaEffect(() => { fit(); const r = () => fit(); window.addEventListener("resize", r); return () => window.removeEventListener("resize", r); }, [fit]);
  scaEffect(() => { const id = requestAnimationFrame(fit); return () => cancelAnimationFrame(id); }, [theme, fit]);

  return (
    <div className={"sc-stage theme-" + theme + " bg-" + (t.backdrop || "Spotlight").toLowerCase()}>
      <div className="sc-topbar">
        <div className="sc-tb-title"><span className="sc-tb-glyph"></span>Scratch Mode <span className="sc-tb-sub">· Instant Capture</span></div>
        <div className="sc-tb-spacer"></div>
        <div className="sc-themes">
          {SC_THEMES.map((th) => (
            <button key={th.id} className={"sc-dot-th" + (theme === th.id ? " on" : "")}
              style={{ "--d": th.accent, "--b": th.bg }} title={th.id} onClick={() => setTheme(th.id)}></button>
          ))}
        </div>
      </div>

      <div className="sc-center">
        <div ref={panelRef} className="sc-scale">
          <ScratchPanel theme={theme} accentVar={accent} />
        </div>
      </div>

      <div className="sc-hint">press the big button (or ⌘⇧R / Space) to capture · click a take to expand · ▶ play wet · ⟳ reamp the raw DI</div>

      {toast && (
        <div className={"sc-toast " + toast.kind}>
          <span className="sc-toast-led"></span>{toast.msg}
        </div>
      )}

      <TweaksPanel>
        <TweakSection label="Presentation" />
        <TweakRadio label="Backdrop" value={t.backdrop} options={["Spotlight", "Flat"]} onChange={(v) => setTweak("backdrop", v)} />
      </TweaksPanel>
    </div>
  );
}

window.ScratchApp = ScratchApp;

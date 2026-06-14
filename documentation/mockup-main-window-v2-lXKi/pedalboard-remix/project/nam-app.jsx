/* NAM Loader — app shell. Backdrop + theme dots + Tweaks, in the remix
   language. The editor's "Browse…" / IR "Load" buttons open the NAM Browser
   as a modal over the panel; choosing a capture applies it to the loader. */

const NAM_THEMES = [
  { id: "midnight",   accent: "#00d9ff", bg: "#1a1a2e" },
  { id: "deep-ocean", accent: "#00c8ff", bg: "#0a1628" },
  { id: "synthwave",  accent: "#ff2bff", bg: "#0d0221" },
  { id: "forest",     accent: "#66cc66", bg: "#1a2f1a" },
  { id: "daylight",   accent: "#0077cc", bg: "#e8e8e8" },
];

function NamApp() {
  const [theme, setTheme] = nState(() => { try { return JSON.parse(localStorage.getItem("nam.theme")) || "midnight"; } catch { return "midnight"; } });
  const [collapsed, setCollapsed] = nState(false);
  const [t, setTweak] = useTweaks({ backdrop: "Spotlight", brushed: false, bevel: false });
  nEffect(() => { try { localStorage.setItem("nam.theme", JSON.stringify(theme)); } catch {} }, [theme]);

  // lifted load-target state so the browser modal can apply picks
  const [model, setModel] = nState(null);
  const [ir, setIr] = nState("4×12 Greenback");
  const [ir2, setIr2] = nState(null);
  const [browse, setBrowse] = nState(null); // null | { kind:'models'|'irs', target:'model'|'ir'|'ir2' }

  const finish = (t.brushed ? " fin-brushed" : "") + (t.bevel ? " fin-bevel" : "");

  const openBrowse = (target) => setBrowse({ kind: target === "model" ? "models" : "irs", target });
  const onChoose = (item) => {
    if (browse) {
      if (browse.target === "model") setModel(item.name);
      else if (browse.target === "ir") setIr(item.name);
      else if (browse.target === "ir2") setIr2(item.name);
    }
    setBrowse(null);
  };

  const panelRef = nRef(null);
  const fit = nCB(() => {
    const el = panelRef.current; if (!el) return;
    el.style.transform = "scale(1)";
    const k = Math.min((window.innerWidth - 90) / el.offsetWidth, (window.innerHeight - 150) / el.offsetHeight, 1.7);
    el.style.transform = "scale(" + Math.max(0.5, k) + ")";
  }, []);
  nEffect(() => { fit(); const r = () => fit(); window.addEventListener("resize", r); return () => window.removeEventListener("resize", r); }, [fit]);
  nEffect(() => { const id = requestAnimationFrame(fit); return () => cancelAnimationFrame(id); }, [collapsed, theme, fit]);

  // scale the modal browser to fit the viewport
  const modalRef = nRef(null);
  const fitModal = nCB(() => {
    const el = modalRef.current; if (!el) return;
    el.style.transform = "scale(1)";
    const k = Math.min((window.innerWidth - 80) / el.offsetWidth, (window.innerHeight - 120) / el.offsetHeight, 1.4);
    el.style.transform = "scale(" + Math.max(0.4, k) + ")";
  }, []);
  nEffect(() => {
    if (!browse) return;
    fitModal();
    const r = () => fitModal();
    window.addEventListener("resize", r);
    const onKey = (e) => { if (e.key === "Escape") setBrowse(null); };
    window.addEventListener("keydown", onKey);
    return () => { window.removeEventListener("resize", r); window.removeEventListener("keydown", onKey); };
  }, [browse, theme, fitModal]);

  return (
    <div className={"nam-stage theme-" + theme + " bg-" + (t.backdrop || "Spotlight").toLowerCase()}>
      <div className="nam-topbar">
        <div className="nt-title"><span className="nt-glyph"></span>NAM Loader <span className="nt-sub">· Plugin Editor</span></div>
        <div className="nt-spacer"></div>
        <div className="nt-themes">
          {NAM_THEMES.map((th) => (
            <button key={th.id} className={"nt-dot" + (theme === th.id ? " on" : "")}
              style={{ "--d": th.accent, "--b": th.bg }} title={th.id} onClick={() => setTheme(th.id)}></button>
          ))}
        </div>
      </div>

      <div className="nam-center">
        <div ref={panelRef} className="nam-scale">
          <NAMEditor theme={theme} collapsed={collapsed} setCollapsed={setCollapsed} finish={finish}
            model={model} setModel={setModel} ir={ir} setIr={setIr} ir2={ir2} setIr2={setIr2}
            onBrowse={openBrowse} />
        </div>
      </div>

      <div className="nam-hint">drag knobs &amp; sliders · click the pills · <b>Browse…</b> to open the model library · click the header to collapse</div>

      {browse && (
        <div className="nam-modal">
          <div className="nam-modal-scrim" onClick={() => setBrowse(null)}></div>
          <div className="nam-modal-scale" ref={modalRef}>
            <NAMBrowser theme={theme} finish={finish} initialTab={browse.kind}
              onChoose={onChoose} onClose={() => setBrowse(null)} />
          </div>
        </div>
      )}

      <TweaksPanel>
        <TweakSection label="Finish" />
        <TweakToggle label="Brushed metal" value={t.brushed} onChange={(v) => setTweak("brushed", v)} />
        <TweakToggle label="Beveled edges" value={t.bevel} onChange={(v) => setTweak("bevel", v)} />
        <TweakSection label="Presentation" />
        <TweakRadio label="Backdrop" value={t.backdrop} options={["Spotlight", "Flat"]} onChange={(v) => setTweak("backdrop", v)} />
        <TweakSection label="State" />
        <TweakToggle label="Collapsed" value={collapsed} onChange={setCollapsed} />
      </TweaksPanel>
    </div>
  );
}

window.NamApp = NamApp;

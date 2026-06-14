/* NamEditorWindow — reusable floating NAM editor for embedding in a host
   (e.g. the Main Window node graph). Owns model/IR state + the browser modal,
   fills its positioned parent, and reports model changes back to the host.
   Relies on NAMEditor (nam-editor.jsx) + NAMBrowser (nam-browser.jsx). */

function NamEditorWindow({ theme, finish, initialModel, initialIr, initialIr2, onModelChange, onClose }) {
  const [collapsed, setCollapsed] = nState(false);
  const [model, setModelRaw] = nState(initialModel || null);
  const [ir, setIr] = nState(initialIr === undefined ? "4×12 Greenback" : initialIr);
  const [ir2, setIr2] = nState(initialIr2 || null);
  const [browse, setBrowse] = nState(null); // null | { kind, target }

  const setModel = (v) => { setModelRaw(v); if (onModelChange) onModelChange(v); };

  const openBrowse = (target) => setBrowse({ kind: target === "model" ? "models" : "irs", target });
  const onChoose = (item) => {
    if (browse) {
      if (browse.target === "model") setModel(item.name);
      else if (browse.target === "ir") setIr(item.name);
      else if (browse.target === "ir2") setIr2(item.name);
    }
    setBrowse(null);
  };

  nEffect(() => {
    const onKey = (e) => { if (e.key === "Escape") { if (browse) setBrowse(null); else onClose && onClose(); } };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [browse, onClose]);

  const closeIcon = (
    <svg viewBox="0 0 24 24" fill="none"><path d="M6 6l12 12M18 6 6 18" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round"/></svg>
  );

  return (
    <div className="nam-winmodal">
      <div className="nam-winmodal-scrim" onClick={() => onClose && onClose()}></div>
      <div className="nam-win-editor">
        <button className="nam-win-close" onClick={() => onClose && onClose()} title="Close editor">{closeIcon}</button>
        <NAMEditor theme={theme} collapsed={collapsed} setCollapsed={setCollapsed} finish={finish}
          model={model} setModel={setModel} ir={ir} setIr={setIr} ir2={ir2} setIr2={setIr2}
          onBrowse={openBrowse} />
      </div>

      {browse && (
        <div className="nam-winmodal top">
          <div className="nam-winmodal-scrim" onClick={() => setBrowse(null)}></div>
          <div className="nam-win-editor">
            <NAMBrowser theme={theme} finish={finish} initialTab={browse.kind}
              onChoose={onChoose} onClose={() => setBrowse(null)} />
          </div>
        </div>
      )}
    </div>
  );
}

window.NamEditorWindow = NamEditorWindow;

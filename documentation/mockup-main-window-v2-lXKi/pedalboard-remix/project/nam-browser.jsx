/* NAM Browser — the window. Faceplate header, Models / Impulse Responses
   tabs, search + sort, a filter rail (type + tone jewels), a scrolling
   result list, a preview pane, and a load/cancel footer. Same amp-chassis
   language as the NAM Loader (warm chassis, amber jewels, cool meter text). */

const { useState: bState, useMemo: bMemo, useRef: bRef } = React;

const NbIcon = {
  amp:   (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><rect x="3" y="4" width="18" height="16" rx="2" stroke="currentColor" strokeWidth="1.6"/><circle cx="12" cy="13" r="4" stroke="currentColor" strokeWidth="1.6"/><circle cx="12" cy="13" r="1" fill="currentColor"/><path d="M6 7h2" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round"/></svg>),
  pedal: (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><rect x="5" y="3" width="14" height="18" rx="2.5" stroke="currentColor" strokeWidth="1.6"/><circle cx="12" cy="8" r="2.4" stroke="currentColor" strokeWidth="1.6"/><rect x="8.5" y="16" width="7" height="3" rx="1.5" fill="currentColor"/></svg>),
  rig:   (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><rect x="4" y="3" width="16" height="7" rx="1.5" stroke="currentColor" strokeWidth="1.6"/><rect x="4" y="12" width="16" height="9" rx="1.5" stroke="currentColor" strokeWidth="1.6"/><circle cx="8" cy="6.5" r="1" fill="currentColor"/><circle cx="14" cy="16.5" r="2.5" stroke="currentColor" strokeWidth="1.5"/></svg>),
  out:   (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><rect x="3" y="7" width="18" height="10" rx="1.5" stroke="currentColor" strokeWidth="1.6"/><circle cx="8" cy="12" r="2" stroke="currentColor" strokeWidth="1.5"/><path d="M14 10v4M17 10v4" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round"/></svg>),
  ir:    (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><rect x="3" y="4" width="18" height="16" rx="2" stroke="currentColor" strokeWidth="1.6"/><circle cx="8" cy="9" r="2.2" stroke="currentColor" strokeWidth="1.4"/><circle cx="16" cy="9" r="2.2" stroke="currentColor" strokeWidth="1.4"/><circle cx="8" cy="15.5" r="2.2" stroke="currentColor" strokeWidth="1.4"/><circle cx="16" cy="15.5" r="2.2" stroke="currentColor" strokeWidth="1.4"/></svg>),
  search:(p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><circle cx="11" cy="11" r="7" stroke="currentColor" strokeWidth="1.7"/><path d="M20 20l-3.5-3.5" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round"/></svg>),
  dl:    (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M12 4v10M12 14l4-4M12 14l-4-4M5 19h14" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" strokeLinejoin="round"/></svg>),
  star:  (p) => (<svg viewBox="0 0 24 24" {...p}><path d="M12 3l2.6 5.6 6.1.7-4.5 4.2 1.2 6L12 17l-5.4 2.7 1.2-6L3.3 9.3l6.1-.7z" fill="currentColor"/></svg>),
  close: (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M6 6l12 12M18 6 6 18" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round"/></svg>),
  chev:  (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M6 9l6 6 6-6" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"/></svg>),
  check: (p) => (<svg viewBox="0 0 24 24" fill="none" {...p}><path d="M5 12l4.5 4.5L19 7" stroke="currentColor" strokeWidth="2.2" strokeLinecap="round" strokeLinejoin="round"/></svg>),
};

function NbStars({ r }) {
  return (
    <span className="nb-stars" title={r.toFixed(1)}>
      <span className="nb-stars-bg">{NbIcon.star()}{NbIcon.star()}{NbIcon.star()}{NbIcon.star()}{NbIcon.star()}</span>
      <span className="nb-stars-fg" style={{ width: (r / 5 * 100) + "%" }}>{NbIcon.star()}{NbIcon.star()}{NbIcon.star()}{NbIcon.star()}{NbIcon.star()}</span>
    </span>
  );
}

function NbRow({ item, active, onPick, onLoad }) {
  const ic = NbIcon[item.type] || NbIcon.amp;
  return (
    <button className={"nb-row" + (active ? " on" : "")} onClick={() => onPick(item.id)} onDoubleClick={() => onLoad(item)}>
      <span className={"nb-ic t-" + item.type}>{ic()}</span>
      <span className="nb-main">
        <span className="nb-line1">
          <span className="nb-name">{item.name}</span>
          <span className={"nb-chip tone-" + item.tone.replace(/[^a-z]/gi, "")}>{item.tone}</span>
        </span>
        <span className="nb-line2">{item.maker} · <em>{item.author}</em> · {item.arch}</span>
      </span>
      <span className="nb-stat"><span className="nb-dl">{NbIcon.dl()}{nbFmtDl(item.dl)}</span><NbStars r={item.rating} /></span>
      <span className="nb-go">{active ? NbIcon.check() : NbIcon.chev({ style: { transform: "rotate(-90deg)" } })}</span>
    </button>
  );
}

function NAMBrowser({ theme, finish, initialTab, onChoose, onClose }) {
  const [tab, setTab] = bState(initialTab || "models");           // models | irs
  const [type, setType] = bState("all");
  const [tones, setTones] = bState([]);
  const [query, setQuery] = bState("");
  const [sort, setSort] = bState("Popular");
  const [sortOpen, setSortOpen] = bState(false);
  const [picked, setPicked] = bState("m3");
  const [loaded, setLoaded] = bState(null);

  const source = tab === "models" ? NB_MODELS : NB_IRS;
  const typeFilters = tab === "models" ? NB_TYPE_FILTERS : NB_IR_TYPE_FILTERS;
  const results = bMemo(() => nbFilterSort(source, { type, tones, query, sort }), [source, type, tones, query, sort]);
  const sel = results.find((r) => r.id === picked) || results[0];

  const switchTab = (tb) => { setTab(tb); setType("all"); setTones([]); setPicked(null); };
  const toggleTone = (t) => setTones((ts) => ts.includes(t) ? ts.filter((x) => x !== t) : [...ts, t]);
  const doLoad = (item) => { if (item) { setLoaded(item.id); setPicked(item.id); if (onChoose) onChoose(item); } };

  return (
    <div className={"nam nb theme-" + (theme || "midnight") + (finish || "")} data-screen-label="NAM Browser">
      <span className="nk-screw tl"></span><span className="nk-screw tr"></span><span className="nk-screw bl"></span><span className="nk-screw br"></span>

      {/* faceplate header */}
      <header className="nb-head">
        <div className="nb-head-l">
          <div className="nk-eyebrow">NAM Library</div>
          <div className="nk-title">ToneHunt Browser</div>
        </div>
        <div className="nb-tabs">
          <button className={"nb-tab" + (tab === "models" ? " on" : "")} onClick={() => switchTab("models")}>Models</button>
          <button className={"nb-tab" + (tab === "irs" ? " on" : "")} onClick={() => switchTab("irs")}>Impulse Responses</button>
        </div>
        <div className="nb-head-r">
          <span className="nb-online"><i></i>Online</span>
          <button className="nk-collapse" onClick={onClose} title="Close"><span className="nb-x">{NbIcon.close()}</span></button>
        </div>
      </header>

      {/* toolbar */}
      <div className="nb-toolbar">
        <label className="nb-search">
          <span className="nb-search-ic">{NbIcon.search()}</span>
          <input value={query} onChange={(e) => setQuery(e.target.value)} placeholder={"Search " + (tab === "models" ? "models, makers, authors" : "cabinets, makers") + "…"} />
          {query && <button className="nb-search-x" onClick={() => setQuery("")}>{NbIcon.close()}</button>}
        </label>
        <div className="nb-sort">
          <button className={"nb-sort-btn" + (sortOpen ? " on" : "")} onClick={() => setSortOpen((o) => !o)}>
            <span className="nb-sort-lab">Sort</span><span className="nb-sort-val">{sort}</span>{NbIcon.chev()}
          </button>
          {sortOpen && (
            <>
              <div className="nb-pop-scrim" onClick={() => setSortOpen(false)}></div>
              <div className="nb-sort-menu">
                {NB_SORTS.map((s) => (
                  <button key={s} className={"nb-sort-it" + (sort === s ? " on" : "")} onClick={() => { setSort(s); setSortOpen(false); }}>
                    {s}{sort === s && <span className="nb-sort-ck">{NbIcon.check()}</span>}
                  </button>
                ))}
              </div>
            </>
          )}
        </div>
      </div>

      {/* body: filter rail + list + preview */}
      <div className="nb-body">
        <aside className="nb-rail">
          <div className="nb-rail-grp">
            <div className="nb-rail-h">Type</div>
            {typeFilters.map((f) => (
              <button key={f.id} className={"nb-filt" + (type === f.id ? " on" : "")} onClick={() => setType(f.id)}>
                <span>{f.label}</span>
                <span className="nb-filt-n">{f.id === "all" ? source.length : source.filter((m) => m.type === f.id).length}</span>
              </button>
            ))}
          </div>
          <div className="nb-rail-grp">
            <div className="nb-rail-h">Tone</div>
            <div className="nb-tones">
              {NB_TONES.map((t) => (
                <button key={t} className={"nb-tone tone-" + t.replace(/[^a-z]/gi, "") + (tones.includes(t) ? " on" : "")} onClick={() => toggleTone(t)}>
                  <i></i>{t}
                </button>
              ))}
            </div>
          </div>
          <div className="nb-rail-foot">
            <div className="nb-rail-count">{results.length} result{results.length === 1 ? "" : "s"}</div>
          </div>
        </aside>

        <div className="nb-list">
          {results.length === 0 && <div className="nb-empty">No captures match your filters.</div>}
          {results.map((m) => <NbRow key={m.id} item={m} active={sel && sel.id === m.id} onPick={setPicked} onLoad={doLoad} />)}
        </div>

        <aside className="nb-preview">
          {sel ? (
            <>
              <div className={"nb-pv-ic t-" + sel.type}>{(NbIcon[sel.type] || NbIcon.amp)()}</div>
              <div className="nb-pv-name">{sel.name}</div>
              <div className="nb-pv-maker">{sel.maker}</div>
              <div className={"nb-chip lg tone-" + sel.tone.replace(/[^a-z]/gi, "")}>{sel.tone}</div>
              <div className="nb-pv-note">{sel.note}</div>
              <div className="nb-pv-meta">
                <div className="nb-pv-mrow"><span>Author</span><b>{sel.author}</b></div>
                <div className="nb-pv-mrow"><span>Architecture</span><b>{sel.arch}</b></div>
                <div className="nb-pv-mrow"><span>Downloads</span><b>{sel.dl.toLocaleString()}</b></div>
                <div className="nb-pv-mrow"><span>Rating</span><b className="nb-pv-rate"><NbStars r={sel.rating} />{sel.rating.toFixed(1)}</b></div>
                <div className="nb-pv-mrow"><span>Size</span><b>{sel.size}</b></div>
              </div>
              <button className="nb-audition">▶ Audition sample</button>
            </>
          ) : <div className="nb-empty">Select a capture to preview.</div>}
        </aside>
      </div>

      {/* footer */}
      <footer className="nb-foot">
        <div className="nb-foot-sel">
          {loaded
            ? <><span className="nb-foot-led on"></span>Loaded <b>{(source.find((m) => m.id === loaded) || {}).name}</b></>
            : sel ? <><span className="nb-foot-led"></span><b>{sel.name}</b> selected</> : <>Nothing selected</>}
        </div>
        <div className="nb-foot-act">
          <button className="nk-btn" onClick={onClose}>Cancel</button>
          <button className="nk-btn primary" disabled={!sel} onClick={() => doLoad(sel)}>{tab === "models" ? "Load Model" : "Load IR"}</button>        </div>
      </footer>
    </div>
  );
}

window.NAMBrowser = NAMBrowser;

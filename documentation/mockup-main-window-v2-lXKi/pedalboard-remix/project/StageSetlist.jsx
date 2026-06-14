/* Setlist view — the set as a live queue.
   Hero (current patch) + rail of all patches; tap any row to jump. */

function SetlistView({ set, current, onSelect }) {
  const now = set[current];
  return (
    <div className="view vsetlist">
      <section className="va-hero">
        <div className="va-eyebrow">
          <span className="np">Now Playing</span>
          <span className="pos"><b>{now.n}</b> / {String(set.length).padStart(2, "0")}</span>
          <span className="vbar"></span>
          <span className="tempo"><span className="beat"></span>{now.bpm} BPM</span>
          <span className="vbar"></span>
          <span className="kpill">{now.key}</span>
        </div>

        <div className="va-title">
          <h1>{now.song}</h1>
          <div className="va-tone">{now.tone}</div>
        </div>

        <div className="va-chain">
          <div className="va-chain-lab">Signal chain</div>
          <PbFlow chain={now.chain} />
        </div>
      </section>

      <aside className="va-rail">
        <div className="va-rail-head">
          <span className="t">Setlist</span>
          <span className="f">friday-set.pdl</span>
        </div>
        <div className="va-list">
          {set.map((p, i) => {
            const state = i < current ? "done" : i === current ? "active" : i === current + 1 ? "next" : "upcoming";
            return (
              <button className={"va-row " + state} key={p.n} onClick={() => onSelect(i)}>
                <span className="rn">{p.n}</span>
                <span className="rmeta">
                  <span className="rsong">{p.song}</span>
                  <span className="rtone">{p.tone}</span>
                </span>
                {state === "done" && <span className="rbadge done">✓</span>}
                {state === "active" && <span className="rbadge live"><i></i>LIVE</span>}
                {state === "next" && <span className="rbadge next">NEXT</span>}
              </button>
            );
          })}
        </div>
      </aside>
    </div>
  );
}

window.SetlistView = SetlistView;

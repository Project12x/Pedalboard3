/* Hero view — immersive single-focus.
   One patch fills the screen; prev/next peek from the edges (tap/swipe),
   set position shown as dots. */

function HeroView({ set, current, onSelect, onPrev, onNext }) {
  const now = set[current];
  const prev = current > 0 ? set[current - 1] : null;
  const next = current < set.length - 1 ? set[current + 1] : null;
  return (
    <div className="view vhero">
      {prev && (
        <button className="vc-peek vc-peek-l" onClick={onPrev}>
          <span className="chev">‹</span>
          <span className="pk-txt">
            <span className="pk-meta">Prev</span>
            <span className="pk-name">{prev.song}</span>
          </span>
        </button>
      )}
      {next && (
        <button className="vc-peek vc-peek-r" onClick={onNext}>
          <span className="pk-txt">
            <span className="pk-meta">Next</span>
            <span className="pk-name">{next.song} · {next.tone}</span>
          </span>
          <span className="chev">›</span>
        </button>
      )}

      <div className="vc-stage">
        <div className="vc-meta">
          <span className="m"><b>{now.n}</b> / {String(set.length).padStart(2, "0")}</span>
          <span className="vbar"></span>
          <span className="m">{now.key}</span>
          <span className="vbar"></span>
          <span className="tempo"><span className="beat"></span>{now.bpm} BPM</span>
        </div>

        <h1 className="vc-title">{now.song}</h1>
        <div className="vc-tone">{now.tone}</div>

        <div className="vc-chain"><PbFlow chain={now.chain} /></div>

        <div className="vc-dots">
          {set.map((p, i) => (
            <button
              key={p.n}
              className={"d " + (i < current ? "past" : i === current ? "cur" : "future")}
              onClick={() => onSelect(i)}
              aria-label={"Go to " + p.song}
            ></button>
          ))}
        </div>
      </div>
    </div>
  );
}

window.HeroView = HeroView;

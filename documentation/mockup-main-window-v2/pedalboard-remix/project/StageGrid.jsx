/* Grid view — hardware-style footswitch board.
   Direct access: tap any tile to switch. Banks group patches. */

function GridView({ set, current, onSelect }) {
  return (
    <div className="view vgrid">
      <div className="vb-banks">
        <button className="bk on">Bank A</button>
        <button className="bk">Bank B</button>
        <button className="bk">Bank C</button>
        <span className="vb-banks-hint">8 patches · tap to switch</span>
      </div>

      <div className="vb-grid">
        {set.map((t, i) => {
          const state = i === current ? "active" : i === current + 1 ? "next" : "";
          return (
            <button className={"vb-tile h-" + t.hue + (state ? " " + state : "")} key={t.n} onClick={() => onSelect(i)}>
              <div className="vb-tile-top">
                <span className="vb-n">{t.n}</span>
                {state === "active" && <span className="vb-led live"><i></i>Live</span>}
                {state === "next" && <span className="vb-led next">Next</span>}
              </div>
              <div className="vb-tile-name">
                <span className="vb-song">{t.song}</span>
                <span className="vb-tone">{t.tone}</span>
              </div>
              <div className="vb-stripe"></div>
            </button>
          );
        })}
      </div>
    </div>
  );
}

window.GridView = GridView;

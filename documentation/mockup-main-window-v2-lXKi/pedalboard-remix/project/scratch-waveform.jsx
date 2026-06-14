/* Scratch Mode — live capture scope. Two sample-synchronized lanes
   (RAW DI + WET OUT) drawn on canvas. One animation loop feeds both so the
   lanes stay locked together, reinforcing that raw and wet are captured from
   the same audio blocks. Freezes on stop; clears when `nonce` changes. */

const { useRef: scRef, useEffect: scEffect } = React;

function ScratchScope({ recording, nonce, rawColor, wetColor }) {
  const rawCv = scRef(null);
  const wetCv = scRef(null);
  const bars = scRef([]);          // [{raw, wet}]
  const env = scRef(0.0);
  const raf = scRef(0);
  const recRef = scRef(recording);
  recRef.current = recording;

  // reset buffer on a new capture
  scEffect(() => { bars.current = []; env.current = 0; drawAll(); /* eslint-disable-next-line */ }, [nonce]);

  const W = 600, H = 60, BAR = 3, GAP = 1.6, STEP = BAR + GAP;
  const MAX = Math.floor(W / STEP);

  function laneDraw(cv, key, color, glow) {
    if (!cv) return;
    const ctx = cv.getContext("2d");
    ctx.clearRect(0, 0, W, H);
    const mid = H / 2;
    // centre line
    ctx.strokeStyle = "rgba(255,255,255,0.06)"; ctx.lineWidth = 1;
    ctx.beginPath(); ctx.moveTo(0, mid + 0.5); ctx.lineTo(W, mid + 0.5); ctx.stroke();
    const arr = bars.current;
    const start = Math.max(0, arr.length - MAX);
    for (let i = start; i < arr.length; i++) {
      const a = arr[i][key];
      const h = Math.max(1.5, a * (H - 8));
      const x = (i - start) * STEP;
      const live = recRef.current && i >= arr.length - 2;
      ctx.fillStyle = color;
      ctx.globalAlpha = live ? 1 : 0.82;
      roundBar(ctx, x, mid - h / 2, BAR, h, BAR / 2);
    }
    ctx.globalAlpha = 1;
    // leading glow head while recording
    if (recRef.current && arr.length) {
      const x = (Math.min(arr.length, MAX) - 1) * STEP;
      ctx.fillStyle = glow; ctx.globalAlpha = 0.9;
      roundBar(ctx, x, 4, BAR, H - 8, BAR / 2);
      ctx.globalAlpha = 1;
    }
  }
  function roundBar(ctx, x, y, w, h, r) {
    r = Math.min(r, w / 2, h / 2);
    ctx.beginPath();
    ctx.moveTo(x + r, y); ctx.arcTo(x + w, y, x + w, y + h, r); ctx.arcTo(x + w, y + h, x, y + h, r);
    ctx.arcTo(x, y + h, x, y, r); ctx.arcTo(x, y, x + w, y, r); ctx.closePath(); ctx.fill();
  }
  function drawAll() {
    laneDraw(rawCv.current, "raw", rawColor || "rgba(180,190,210,0.9)", "#fff");
    laneDraw(wetCv.current, "wet", wetColor || "#00d9ff", wetColor || "#00d9ff");
  }

  scEffect(() => {
    let last = performance.now();
    const tick = (now) => {
      const dt = now - last;
      if (recRef.current && dt >= 32) {
        last = now;
        // wandering musical-ish envelope; wet is a touch hotter/compressed
        env.current += (Math.random() - 0.45) * 0.5;
        env.current = Math.max(0.08, Math.min(1, env.current));
        const pluck = Math.random() > 0.86 ? 0.5 : 0;
        const raw = Math.max(0.06, Math.min(1, env.current * (0.6 + Math.random() * 0.4) + pluck));
        const wet = Math.max(0.08, Math.min(1, 0.25 + raw * 0.85));
        bars.current.push({ raw, wet });
        if (bars.current.length > MAX + 4) bars.current.shift();
        drawAll();
      }
      raf.current = requestAnimationFrame(tick);
    };
    raf.current = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(raf.current);
  }, [rawColor, wetColor]);

  scEffect(() => { drawAll(); }, [rawColor, wetColor]);

  return (
    <div className={"sc-scope" + (recording ? " live" : "")}>
      <div className="sc-lane">
        <span className="sc-lane-tag raw">RAW<i>DI</i></span>
        <canvas ref={rawCv} width={W} height={H}></canvas>
      </div>
      <div className="sc-lane">
        <span className="sc-lane-tag wet">WET<i>OUT</i></span>
        <canvas ref={wetCv} width={W} height={H}></canvas>
      </div>
      <div className="sc-scope-sync" title="Raw and wet are captured from the same audio blocks">
        <svg viewBox="0 0 24 24" fill="none"><path d="M9 7H6a4 4 0 0 0 0 8h3M15 7h3a4 4 0 0 1 0 8h-3M8 11h8" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round"/></svg>
        sample-locked
      </div>
    </div>
  );
}

/* small static thumbnail of a saved take's waveform */
function ScratchThumb({ data, color }) {
  return (
    <div className="sc-thumb">
      {data.map((h, i) => <i key={i} style={{ height: (h * 100) + "%", background: color }}></i>)}
    </div>
  );
}

Object.assign(window, { ScratchScope, ScratchThumb });

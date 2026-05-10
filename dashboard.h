#pragma once

const char DASHBOARD[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>Soil Monitor</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4/dist/chart.umd.min.js"></script>
<style>
  :root {
    --green:#16a34a; 
    --green-l:#dcfce7; 
    --green-d:#15803d;
    --amber:#d97706; 
    --amber-l:#fef3c7;
    --red:#dc2626;   
    --red-l:#fee2e2;
    --blue:#2563eb;
    --yellow:FAE946;  
    --blue-l:#dbeafe;
    --gray:#6b7280;  
    --border:#e5e7eb;
    --bg:#f8fafc;    
    --card:#fff;
    --text:#111827;  
    --sub:#6b7280;
    --nav-h:56px;
  }
  *{box-sizing:border-box;margin:0;padding:0}
  html,body{height:100%}
  body{
    font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;
    background:var(--bg);color:var(--text);
    display:flex;flex-direction:column;min-height:100dvh;
  }
  /* ── Header ── */
  header{
    background:var(--card);border-bottom:1px solid var(--border);
    padding:10px 16px;display:flex;align-items:center;
    justify-content:space-between;gap:12px;position:sticky;top:0;z-index:10;
  }
  .header-title{display:flex;align-items:center;gap:10px;min-width:0}
  header h1{font-size:1rem;font-weight:700;white-space:nowrap}
  .header-meta{display:flex;align-items:center;gap:8px;min-width:0;flex-wrap:wrap;justify-content:flex-end}
  .conn-wrap{display:flex;align-items:center;gap:6px;font-size:.78rem;color:var(--sub);min-width:0}
  .dot{width:8px;height:8px;border-radius:50%;background:#ccc;transition:background .4s;flex-shrink:0}
  .dot.online{background:var(--green)}
  .dot.offline{background:var(--red)}
  .header-stats{display:flex;align-items:center;gap:8px;flex-wrap:wrap;justify-content:flex-end}
  .header-stat{
    display:flex;align-items:center;gap:6px;padding:5px 8px;
    border:1px solid var(--border);border-radius:999px;background:var(--bg);
    font-size:.72rem;color:var(--sub);white-space:nowrap;
  }
  .header-stat .val{font-size:.78rem;font-weight:700;color:var(--text)}
  /* ── Main ── */
  main{flex:1;padding:12px 12px calc(var(--nav-h) + 16px)}
  /* ── Tabs ── */
  .tab-panel{display:none}
  .tab-panel.active{display:block}
  /* ── Bottom nav ── */
  .bottom-nav{
    position:fixed;bottom:0;left:0;right:0;height:var(--nav-h);
    background:var(--card);border-top:1px solid var(--border);
    display:flex;align-items:stretch;z-index:10;
    padding-bottom:env(safe-area-inset-bottom,0);
  }
  .nav-btn{
    flex:1;display:flex;flex-direction:column;align-items:center;
    justify-content:center;gap:3px;border:none;background:none;
    cursor:pointer;font-size:.62rem;color:var(--sub);padding:4px 2px;
    transition:color .15s;position:relative;
  }
  .nav-btn svg{width:20px;height:20px}
  .nav-btn.active{color:var(--green)}
  .nav-btn.active::before{
    content:'';position:absolute;top:0;left:20%;right:20%;
    height:2px;background:var(--green);border-radius:0 0 2px 2px;
  }
  /* ── Cards ── */
  .card{
    background:var(--card);border:1px solid var(--border);
    border-radius:14px;padding:14px;margin-bottom:10px;
  }
  .card-title{
    font-size:.7rem;font-weight:700;text-transform:uppercase;
    letter-spacing:.07em;color:var(--sub);margin-bottom:10px;
  }
  /* ── Moisture hero ── */
  .moisture-hero{display:flex;align-items:center;gap:16px}
  .big{
    font-size:3.6rem;font-weight:800;line-height:1;
    transition:color .4s;font-variant-numeric:tabular-nums;
  }
  .big.wet{color:var(--green)}
  .big.mid{color:var(--amber)}
  .big.dry{color:var(--red)}
  .moisture-meta .label{font-size:.82rem;color:var(--sub)}
  .moisture-meta .ts{font-size:.72rem;color:#9ca3af;margin-top:4px}
  /* ── Chart ── */
  .chart-wrap{position:relative;height:180px}
  /* ── Pump ── */
  .pump-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:12px}
  .pump-indicator{display:flex;align-items:center;gap:10px}
  .pump-dot{
    width:13px;height:13px;border-radius:50%;background:#d1d5db;
    box-shadow:0 0 0 0 transparent;transition:box-shadow .4s,background .4s;flex-shrink:0;
  }
  .pump-dot.on{background:var(--green);box-shadow:0 0 0 4px rgba(22,163,74,.2)}
  .pump-label{font-size:.95rem;font-weight:700}
  .badge{
    font-size:.65rem;font-weight:700;padding:3px 8px;
    border-radius:99px;text-transform:uppercase;letter-spacing:.05em;white-space:nowrap;
  }
  .badge.auto{background:var(--blue-l);color:var(--blue)}
  .badge.manual{background:var(--amber-l);color:var(--amber)}
  .badge.cool{background:#f3f4f6;color:var(--gray)}
  .badge.active-blue{background:var(--blue-l);color:var(--blue)}
  /* ── Progress bars ── */
  .prog-wrap{display:none;margin:6px 0 2px}
  .prog-wrap.visible{display:block}
  .prog-track{background:#f3f4f6;border-radius:99px;height:5px;overflow:hidden}
  .prog-fill{height:100%;border-radius:99px;transition:width .5s}
  .prog-label{font-size:.72rem;margin-top:4px;display:block}
  /* ── Pump buttons ── */
  .pump-btns{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px;margin-top:12px}
  /* ── Settings ── */
  .setting-row{
    display:flex;align-items:center;gap:8px;
    padding:9px 0;border-bottom:1px solid var(--border);
  }
  .setting-row:last-child{border-bottom:none}
  .setting-row label{font-size:.8rem;color:var(--sub);flex:1;min-width:0}
  .setting-row input[type=range]{flex:1;min-width:0;accent-color:var(--green)}
  .setting-row input[type=number]{
    width:68px;padding:5px 7px;border-radius:8px;
    border:1px solid var(--border);font-size:.82rem;background:var(--bg);
  }
  .slider-val{font-size:.82rem;font-weight:700;min-width:32px;text-align:right}
  /* ── Buttons ── */
  button{
    padding:9px 10px;border-radius:10px;border:1px solid var(--border);
    background:var(--card);font-size:.8rem;font-weight:600;cursor:pointer;
    transition:background .15s,transform .1s;
    display:flex;align-items:center;justify-content:center;gap:5px;
  }
  button:active{transform:scale(.97)}
  button.primary{background:var(--green);color:#fff;border-color:var(--green-d)}
  button.danger{background:var(--red-l);color:var(--red);border-color:#fca5a5}
  button.warning{background:var(--amber-l);color:var(--amber);border-color:#fcd34d}
  button.active-blue{background:var(--blue-l);color:var(--blue);border-color:#93c5fd}
  button.neutral{color:var(--text)}
  .set-btn{padding:5px 12px;font-size:.75rem}
  /* ── Actions grid ── */
  .actions-grid{display:flex;flex-wrap:wrap;gap:8px;justify-content:space-between}
  .actions-grid button{flex:1 1 140px}
  /* ── Serial log ── */
  #serial-log{
    background:#1f2937;color:#d1fae5;font-family:monospace;
    font-size:.72rem;padding:10px;border-radius:10px;
    height:260px;overflow-y:auto;white-space:pre-wrap;word-break:break-all;
  }
  #serial-log::-webkit-scrollbar{width:4px}
  #serial-log::-webkit-scrollbar-track{background:#374151}
  #serial-log::-webkit-scrollbar-thumb{background:#4b5563;border-radius:4px}
  /* ── Toast ── */
  #toast{
    position:fixed;bottom:calc(var(--nav-h) + 10px);left:50%;
    transform:translateX(-50%) translateY(calc(100% + var(--nav-h) + 24px));
    background:#1f2937;color:#fff;padding:9px 18px;
    border-radius:99px;font-size:.82rem;
    transition:transform .3s, opacity .2s;
    opacity:0;visibility:hidden;
    pointer-events:none;white-space:nowrap;z-index:100;
  }
  #toast.show{transform:translateX(-50%) translateY(0);opacity:1;visibility:visible}
  /* ── Desktop layout ≥ 700px ── */
  @media(min-width:700px){
    main{padding:16px;padding-bottom:16px;max-width:1080px;margin:0 auto}
    .bottom-nav{display:none}
    .tab-panel{display:block!important}
    .header-meta{flex-wrap:nowrap}
    #desktop-grid{
      display:grid;
      grid-template-columns:1fr 1fr;
      grid-template-areas:
        "overview pump"
        "overview settings"
        "logs     logs";
      gap:10px;align-items:start;
    }
    #panel-overview{grid-area:overview}
    #panel-pump    {grid-area:pump}
    #panel-settings{grid-area:settings}
    #panel-logs    {grid-area:logs}
    #toast{bottom:12px}
  }
</style>
</head>
<body>
<header>
  <div class="header-title">
    <h1>&#127807; Soil Monitor</h1>
  </div>
  <div class="header-meta">
    <div class="header-stats">
      <div class="header-stat"><span class="lbl">Free RAM</span><span class="val" id="st-heap">--</span></div>
      <div class="header-stat"><span class="lbl">Storage</span><span class="val" id="st-spiffs">--</span></div>
    </div>
    <div class="conn-wrap">
      <span class="dot" id="wdot"></span>
      <span id="wlabel">connecting...</span>
    </div>
  </div>
</header>

<main>
<div id="desktop-grid">

  <!-- ══ OVERVIEW ══ -->
  <div id="panel-overview" class="tab-panel active">
    <div class="card">
      <div class="moisture-hero">
        <div class="big dry" id="now">--</div>
        <div class="moisture-meta">
          <div class="label">Soil moisture</div>
          <div class="ts" id="updated">no data yet</div>
        </div>
      </div>
    </div>
    <div class="card">
      <div class="card-title">History</div>
      <div class="chart-wrap"><canvas id="chart"></canvas></div>
    </div>
    <div class="card">
      <div class="card-title">Actions</div>
      <div class="actions-grid">
        <button class="primary" onclick="takeReading()">Take reading</button>
        <button class="neutral" onclick="refreshAll()">Refresh</button>
        <button class="neutral" onclick="window.location='/status'">Status JSON</button>
        <button class="neutral" onclick="window.location='/debug'">Raw history</button>
        <button class="danger" onclick="clearLog()">Clear log</button>
        <button class="warning" onclick="window.location='/update'">Firmware Update</button>
      </div>
    </div>
  </div>

  <!-- ══ PUMP ══ -->
  <div id="panel-pump" class="tab-panel">
    <div class="card">
      <div class="card-title">Pump control</div>
      <div class="pump-header">
        <div class="pump-indicator">
          <div class="pump-dot" id="pump-dot"></div>
          <span class="pump-label" id="pump-label">OFF</span>
        </div>
        <span class="badge auto" id="mode-badge">AUTO</span>
      </div>
      <div class="prog-wrap" id="run-wrap">
        <div class="prog-track"><div class="prog-fill" id="run-bar" style="width:0%;background:var(--green)"></div></div>
        <span class="prog-label" id="run-label" style="color:var(--green)"></span>
      </div>
      <div class="prog-wrap" id="soak-wrap">
        <div class="prog-track"><div class="prog-fill" id="soak-bar" style="width:0%;background:var(--blue)"></div></div>
        <span class="prog-label" id="soak-label" style="color:var(--blue)"></span>
      </div>
      <div class="prog-wrap" id="cooldown-wrap">
        <div class="prog-track"><div class="prog-fill" id="cooldown-bar" style="width:0%;background:var(--amber)"></div></div>
        <span class="prog-label" id="cooldown-label" style="color:var(--amber)"></span>
      </div>
      <span id="manual-run-label" style="font-size:.72rem;color:var(--green);display:none;margin-top:6px;display:none"></span>
      <div class="pump-btns">
        <button class="primary"     onclick="pumpManualOn()">ON</button>
        <button class="danger"      onclick="pumpManualOff()">OFF</button>
        <button class="active-blue" id="btn-auto" onclick="toggleAuto()">Auto: ON</button>
      </div>
      <div style="margin-top:10px;padding-top:10px;border-top:1px solid var(--border);font-size:.72rem;color:var(--sub)">
        Last run: <span id="last-pump-run" style="color:var(--text);font-weight:600">never</span>
      </div>
    </div>
  </div>

  <!-- ══ SETTINGS ══ -->
  <div id="panel-settings" class="tab-panel">
    <div class="card">
      <div class="card-title">Threshold</div>
      <div class="setting-row">
        <label>Low threshold</label>
        <input type="range" id="sl-low" min="0" max="99" oninput="onSlider()" onchange="sendThresholds()">
        <span class="slider-val" id="val-low">30%</span>
      </div>
    </div>
      <div class="card">
        <div class="card-title">Timings</div>
      <div class="setting-row">
        <label>Sample interval (s)</label>
        <input type="number" id="inp-sample" min="10" max="3600" step="10">
        <button class="neutral set-btn" onclick="sendSampleInterval()">Set</button>
      </div>
      <div class="setting-row">
        <label>Run duration (s)</label>
        <input type="number" id="inp-duration" min="5" max="3600" step="5">
        <button class="neutral set-btn" onclick="sendDuration()">Set</button>
      </div>
      <div class="setting-row">
        <label>Soak time (s)</label>
        <input type="number" id="inp-soak" min="30" max="7200" step="30">
        <button class="neutral set-btn" onclick="sendSoak()">Set</button>
      </div>
      <div class="setting-row">
        <label>Cooldown (s)</label>
        <input type="number" id="inp-cooldown" min="30" step="30">
        <button class="neutral set-btn" onclick="sendCooldown()">Set</button>
      </div>
    </div>
  </div>
  <div id="panel-logs" class="tab-panel">
    <div class="card">
      <div class="card-title" style="display:flex;justify-content:space-between;align-items:center">
        <span>Serial monitor</span>
        <div style="display:flex;gap:6px">
          <button class="neutral" style="padding:4px 10px;font-size:.72rem" onclick="clearLog2()">Clear</button>
          <button class="neutral" style="padding:4px 10px;font-size:.72rem" id="btn-scroll" onclick="toggleScroll()">Auto-scroll: ON</button>
        </div>
      </div>
      <div id="serial-log"></div>
    </div>
  </div>

</div><!-- /desktop-grid -->
</main>

<!-- Bottom nav (mobile) -->
<nav class="bottom-nav">
  <button class="nav-btn active" id="nav-overview" onclick="showTab('overview')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M3 9l9-7 9 7v11a2 2 0 01-2 2H5a2 2 0 01-2-2z"/><polyline points="9 22 9 12 15 12 15 22"/></svg>
    Overview
  </button>
  <button class="nav-btn" id="nav-pump" onclick="showTab('pump')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><ellipse cx="12" cy="5" rx="4" ry="3"/><path d="M8 5v6a4 4 0 008 0V5"/><line x1="12" y1="11" x2="12" y2="19"/><line x1="8" y1="19" x2="16" y2="19"/></svg>
    Pump
  </button>
  <button class="nav-btn" id="nav-settings" onclick="showTab('settings')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 00.33 1.82l.06.06a2 2 0 010 2.83 2 2 0 01-2.83 0l-.06-.06a1.65 1.65 0 00-1.82-.33 1.65 1.65 0 00-1 1.51V21a2 2 0 01-4 0v-.09A1.65 1.65 0 009 19.4a1.65 1.65 0 00-1.82.33l-.06.06a2 2 0 01-2.83-2.83l.06-.06A1.65 1.65 0 004.68 15a1.65 1.65 0 00-1.51-1H3a2 2 0 010-4h.09A1.65 1.65 0 004.6 9a1.65 1.65 0 00-.33-1.82l-.06-.06a2 2 0 012.83-2.83l.06.06A1.65 1.65 0 009 4.68a1.65 1.65 0 001-1.51V3a2 2 0 014 0v.09a1.65 1.65 0 001 1.51 1.65 1.65 0 001.82-.33l.06-.06a2 2 0 012.83 2.83l-.06.06A1.65 1.65 0 0019.4 9a1.65 1.65 0 001.51 1H21a2 2 0 010 4h-.09a1.65 1.65 0 00-1.51 1z"/></svg>
    Settings
  </button>
  <button class="nav-btn" id="nav-logs" onclick="showTab('logs')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 2H6a2 2 0 00-2 2v16a2 2 0 002 2h12a2 2 0 002-2V8z"/><polyline points="14 2 14 8 20 8"/><line x1="16" y1="13" x2="8" y2="13"/><line x1="16" y1="17" x2="8" y2="17"/></svg>
    Logs
  </button>
</nav>

<div id="toast"></div>

<script>
// ── Tab navigation ──
var currentTab = 'overview';
function showTab(name) {
  currentTab = name;
  ['overview','pump','settings','logs'].forEach(function(t) {
    document.getElementById('panel-' + t).classList.toggle('active', t === name);
    document.getElementById('nav-'   + t).classList.toggle('active', t === name);
  });
}

// ── Chart ──
var chart = new Chart(document.getElementById('chart').getContext('2d'), {
  type: 'line',
  data: { labels: [], datasets: [{
    label: 'Moisture %', data: [],
    borderColor: '#16a34a', backgroundColor: 'rgba(22,163,74,.08)',
    tension: 0.4, pointRadius: 2, fill: true
  }]},
  options: {
    responsive: true, maintainAspectRatio: false,
    scales: {
      x: { ticks: { maxTicksLimit: 6, maxRotation: 0, font: {size: 10} } },
      y: { min: 0, max: 100, ticks: { callback: function(v){ return v+'%'; }, font: {size: 10} } }
    },
    plugins: { legend: { display: false } },
    animation: false
  }
});

// Format a unix timestamp for chart x-axis: "May 7 14:30"
function fmtLabel(t) {
  var d = new Date(t * 1000);
  return d.toLocaleDateString([], {month:'short', day:'numeric'}) + ' ' +
         d.toLocaleTimeString([], {hour:'2-digit', minute:'2-digit'});
}

// Format a unix timestamp for the "Updated" line: "Wed, May 7 · 14:30"
function fmtTimestamp(t) {
  var d = new Date(t * 1000);
  return d.toLocaleDateString([], {weekday:'short', month:'short', day:'numeric'}) + ' \xb7 ' +
         d.toLocaleTimeString([], {hour:'2-digit', minute:'2-digit'});
}

var cooldownTotal = 300;
var inputsInitialised = false;
var toastTimer = null;

function toast(msg, dur) {
  var el = document.getElementById('toast');
  el.textContent = msg;
  el.classList.add('show');
  if (toastTimer) clearTimeout(toastTimer);
  toastTimer = setTimeout(function(){
    el.classList.remove('show');
    toastTimer = null;
  }, dur || 2500);
}

function setMoistureColor(val) {
  var el = document.getElementById('now');
  el.classList.remove('wet','mid','dry');
  el.classList.add(val >= 60 ? 'wet' : val >= 30 ? 'mid' : 'dry');
}

// ── Pump UI ──
function updatePumpUI(s) {
  var dot   = document.getElementById('pump-dot');
  var lbl   = document.getElementById('pump-label');
  var badge = document.getElementById('mode-badge');
  var btn   = document.getElementById('btn-auto');
  var rwrap = document.getElementById('run-wrap');
  var rbar  = document.getElementById('run-bar');
  var rlbl  = document.getElementById('run-label');
  var swrap = document.getElementById('soak-wrap');
  var sbar  = document.getElementById('soak-bar');
  var slbl  = document.getElementById('soak-label');
  var cwrap = document.getElementById('cooldown-wrap');
  var cbar  = document.getElementById('cooldown-bar');
  var clbl  = document.getElementById('cooldown-label');
  var mrlbl = document.getElementById('manual-run-label');

  dot.className   = 'pump-dot' + (s.pump_on ? ' on' : '');
  lbl.textContent = s.pump_on ? 'ON' : 'OFF';

  // Last pump run
  var lprEl = document.getElementById('last-pump-run');
  lprEl.textContent = (s.last_pump_run && s.last_pump_run > 0)
    ? fmtTimestamp(s.last_pump_run) : 'never';

  var duration  = s.pump_duration_s || 30;
  var soakTotal = s.soak_s || 300;
  document.getElementById('inp-duration').value = duration;
  document.getElementById('inp-soak').value     = soakTotal;

  if (!s.auto_mode) {
    rwrap.classList.remove('visible');
    swrap.classList.remove('visible');
    cwrap.classList.remove('visible');
    if (s.pump_on) {
      mrlbl.style.display = 'block';
      mrlbl.textContent   = 'Running for ' + (s.pump_elapsed_s || 0) + 's';
    } else {
      mrlbl.style.display = 'none';
    }
    badge.textContent = 'MANUAL'; badge.className = 'badge manual';
    btn.textContent   = 'Auto: OFF'; btn.className = 'neutral';
    return;
  }

  mrlbl.style.display = 'none';
  var cd       = s.cooldown_remaining_s || 0;
  var remaining = s.pump_remaining_s    || 0;
  var soakRem  = s.soak_remaining_s     || 0;
  cooldownTotal = s.cooldown_s || 300;

  if (s.in_soak && soakRem > 0) {
    swrap.classList.add('visible');
    sbar.style.width  = Math.round((soakRem / soakTotal) * 100) + '%';
    slbl.textContent  = 'Soaking \u2014 ' + soakRem + 's remaining';
    badge.textContent = 'SOAKING'; badge.className = 'badge active-blue';
  } else {
    swrap.classList.remove('visible');
    badge.textContent = cd > 0 ? 'COOLDOWN' : 'AUTO';
    badge.className   = 'badge ' + (cd > 0 ? 'cool' : 'auto');
  }

  if (s.pump_on && remaining > 0) {
    rwrap.classList.add('visible');
    rbar.style.width = Math.round((remaining / duration) * 100) + '%';
    rlbl.textContent = 'Running \u2014 ' + remaining + 's remaining';
  } else {
    rwrap.classList.remove('visible');
  }

  btn.textContent = 'Auto: ON'; btn.className = 'active-blue';

  if (cd > 0) {
    cwrap.classList.add('visible');
    cbar.style.width = Math.round((cd / cooldownTotal) * 100) + '%';
    clbl.textContent = 'Cooldown: ' + cd + 's remaining';
  } else {
    cwrap.classList.remove('visible');
  }
}

function initInputs(s) {
  if (inputsInitialised) return;
  document.getElementById('sl-low').value        = s.low_threshold     || 30;
  document.getElementById('val-low').textContent = (s.low_threshold    || 30) + '%';
  document.getElementById('inp-sample').value    = s.sample_interval_s || 60;
  document.getElementById('inp-duration').value  = s.pump_duration_s   || 30;
  document.getElementById('inp-cooldown').value  = s.cooldown_s        || 300;
  document.getElementById('inp-soak').value      = s.soak_s            || 300;
  inputsInitialised = true;
}

// ── Data / chart ──
function loadData() {
  fetch('/data', {credentials:'include'})
    .then(function(r){ return r.json(); })
    .then(function(d){
      if (!d || !d.length) return;
      var last = d[d.length-1];
      document.getElementById('now').textContent     = last.m + '%';
      setMoistureColor(last.m);
      document.getElementById('updated').textContent = fmtTimestamp(last.t);
      chart.data.labels              = d.map(function(e){ return fmtLabel(e.t); });
      chart.data.datasets[0].data   = d.map(function(e){ return e.m; });
      chart.update();
    })
    .catch(function(){ toast('Could not load data'); });
}

function refreshAll() { loadData(); toast('Refreshed'); }

function takeReading() {
  toast('Taking reading...');
  fetch('/read', {credentials:'include'})
    .then(function(r){ return r.json(); })
    .then(function(j){ toast(j.ok ? 'Logged '+j.moisture+'%' : 'Suspect reading \u2014 skipped'); })
    .catch(function(){ toast('Read failed'); });
}

function clearLog() {
  if (!confirm('Clear all logged data?')) return;
  fetch('/clear', {credentials:'include'})
    .then(function(r){ return r.json(); })
    .then(function(j){
      if (!j.ok) return;
      chart.data.labels = []; chart.data.datasets[0].data = [];
      chart.update();
      document.getElementById('now').textContent     = '--';
      document.getElementById('updated').textContent = 'no data yet';
      toast('Log cleared');
    });
}

// ── Settings ──
function onSlider() {
  document.getElementById('val-low').textContent = document.getElementById('sl-low').value + '%';
}
function sendThresholds() {
  fetch('/pump/config?low=' + document.getElementById('sl-low').value, {credentials:'include'})
    .then(function(r){ return r.json(); }).then(function(){ toast('Threshold updated'); });
}
function sendSampleInterval() {
  var v = document.getElementById('inp-sample').value;
  fetch('/pump/config?sample_interval=' + v, {credentials:'include'})
    .then(function(r){ return r.json(); }).then(function(){ toast('Sample interval \u2192 ' + v + 's'); });
}
function sendDuration() {
  var v = document.getElementById('inp-duration').value;
  fetch('/pump/config?duration=' + v, {credentials:'include'})
    .then(function(r){ return r.json(); }).then(function(){ toast('Run duration \u2192 ' + v + 's'); });
}
function sendSoak() {
  var v = document.getElementById('inp-soak').value;
  fetch('/pump/config?soak=' + v, {credentials:'include'})
    .then(function(r){ return r.json(); }).then(function(){ toast('Soak time \u2192 ' + v + 's'); });
}
function sendCooldown() {
  var v = document.getElementById('inp-cooldown').value;
  fetch('/pump/config?cooldown=' + v, {credentials:'include'})
    .then(function(r){ return r.json(); }).then(function(){ toast('Cooldown \u2192 ' + v + 's'); });
}

// ── Pump commands ──
function pumpManualOn() {
  fetch('/pump/on',   {credentials:'include'}).then(function(r){ return r.json(); })
    .then(function(){ toast('Pump ON'); });
}
function pumpManualOff() {
  fetch('/pump/off',  {credentials:'include'}).then(function(r){ return r.json(); })
    .then(function(){ toast('Pump OFF'); });
}
function toggleAuto() {
  fetch('/pump/auto', {credentials:'include'}).then(function(r){ return r.json(); })
    .then(function(j){ toast('Auto mode ' + (j.auto ? 'ON' : 'OFF')); });
}

// ── Serial log ──
var autoScroll = true;
function clearLog2() { document.getElementById('serial-log').innerHTML = ''; }
function toggleScroll() {
  autoScroll = !autoScroll;
  document.getElementById('btn-scroll').textContent = 'Auto-scroll: ' + (autoScroll ? 'ON' : 'OFF');
}

// ── WebSocket ──
var wsToken = null;
var wsConn;
var wsReconnectTimer = null;

function openWS() {
  if (wsConn && wsConn.readyState < 2) return;
  wsConn = new WebSocket('ws://' + location.hostname + ':81/');
  wsConn.onopen = function() {
    if (wsReconnectTimer) { clearTimeout(wsReconnectTimer); wsReconnectTimer = null; }
    wsConn.send(JSON.stringify({type:'auth', token: wsToken}));
  };
  wsConn.onmessage = function(evt) {
    var d; try { d = JSON.parse(evt.data); } catch(e) { return; }
    if (d.type === 'status') {
      var online = d.wifi_rssi < 0;
      document.getElementById('wdot').className    = online ? 'dot online' : 'dot offline';
      document.getElementById('wlabel').textContent = online
        ? d.wifi_ssid + ' (' + d.wifi_rssi + ' dBm)' : 'offline';
      document.getElementById('st-heap').textContent   = Math.round(d.free_heap / 1024) + 'K';
      document.getElementById('st-spiffs').textContent =
        Math.round(d.spiffs_used / 1024) + '/' + Math.round(d.spiffs_total / 1024) + 'K';
      initInputs(d);
      updatePumpUI(d);
    } else if (d.type === 'reading') {
      document.getElementById('now').textContent     = d.m + '%';
      setMoistureColor(d.m);
      document.getElementById('updated').textContent = fmtTimestamp(d.t);
      chart.data.labels.push(fmtLabel(d.t));
      chart.data.datasets[0].data.push(d.m);
      while (chart.data.labels.length > 1440) {
        chart.data.labels.shift(); chart.data.datasets[0].data.shift();
      }
      chart.update();
    } else if (d.type === 'log') {
      var box  = document.getElementById('serial-log');
      var line = document.createElement('div');
      line.textContent = '[' + (d.ts / 1000).toFixed(1) + 's] ' + d.m;
      box.appendChild(line);
      while (box.children.length > 200) box.removeChild(box.firstChild);
      if (autoScroll) box.scrollTop = box.scrollHeight;
    }
  };
  wsConn.onclose = function() {
    document.getElementById('wdot').className     = 'dot offline';
    document.getElementById('wlabel').textContent = 'reconnecting...';
    wsReconnectTimer = setTimeout(openWS, 3000);
  };
  wsConn.onerror = function() { wsConn.close(); };
}

function connectWS() {
  fetch('/wstoken', {credentials:'include'})
    .then(function(r){ return r.text(); })
    .then(function(t){ wsToken = t; openWS(); })
    .catch(function(){ wsReconnectTimer = setTimeout(connectWS, 3000); });
}

loadData();
connectWS();
</script>
</body></html>
)rawhtml";

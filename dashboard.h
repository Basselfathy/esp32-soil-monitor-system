#pragma once

const char DASHBOARD[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en" dir="ltr" id="html-root">
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
  /* ── Environment ── */
  .env-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;margin-top:2px}
  .env-tile{text-align:center;padding:8px 4px;background:var(--bg);border-radius:10px}
  .env-val{font-size:1.55rem;font-weight:700;font-variant-numeric:tabular-nums;color:var(--fg)}
  .env-val.na{color:#9ca3af;font-size:1.1rem}
  .env-label{font-size:.7rem;color:var(--sub);margin-top:3px}
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
  /* ── Calibration ── */
  .calib-table{width:100%;border-collapse:collapse;font-size:.82rem;margin-top:6px}
  .calib-table th{text-align:left;color:var(--sub);font-weight:600;font-size:.72rem;
    text-transform:uppercase;letter-spacing:.05em;padding:4px 8px 4px 0;border-bottom:1px solid var(--border)}
  .calib-table td{padding:7px 8px 7px 0;border-bottom:1px solid var(--border)}
  .calib-table tr:last-child td{border-bottom:none}
  .calib-raw{font-weight:700;font-variant-numeric:tabular-nums;color:var(--text);font-size:.9rem}
  .calib-pct{color:var(--sub)}
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
        "overview controls"
        "overview settings"
        "overview calib"
        "logs     logs";
      gap:10px;align-items:start;
    }
    #panel-overview {grid-area:overview}
    #panel-controls {grid-area:controls}
    #panel-settings {grid-area:settings}
    #panel-calib    {grid-area:calib}
    #panel-logs     {grid-area:logs}
    #toast{bottom:12px}
  }
  /* ── RTL ── */
  [dir=rtl] .slider-val{text-align:left}
  [dir=rtl] .calib-table th{text-align:right}
  [dir=rtl] .setting-row label{text-align:right}
</style>
</head>
<body>
<header>
  <div class="header-title">
    <h1>&#127807; <span data-i18n="app_title">Soil Monitor</span></h1>
  </div>
  <div class="header-meta">
    <div class="header-stats">
      <div class="header-stat"><span class="lbl" data-i18n="free_ram">Free RAM</span><span class="val" id="st-heap">--</span></div>
      <div class="header-stat"><span class="lbl" data-i18n="storage">Storage</span><span class="val" id="st-spiffs">--</span></div>
    </div>
    <div class="conn-wrap">
      <span class="dot" id="wdot"></span>
      <span id="wlabel">connecting...</span>
    </div>
    <button id="btn-lang" onclick="toggleLang()" style="padding:4px 10px;font-size:.75rem;font-weight:700;min-width:36px">AR</button>
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
          <div class="label" data-i18n="soil_moisture">Soil moisture</div>
          <div class="ts" id="updated">no data yet</div>
        </div>
      </div>
    </div>
    <div class="card">
      <div class="card-title" data-i18n="environment">Environment</div>
      <div class="env-grid">
        <div class="env-tile">
          <div class="env-val" id="soil-temp">--</div>
          <div class="env-label" data-i18n="soil_temp">Soil temp</div>
        </div>
        <div class="env-tile">
          <div class="env-val" id="air-temp">--</div>
          <div class="env-label" data-i18n="air_temp">Air temp</div>
        </div>
        <div class="env-tile">
          <div class="env-val" id="air-humid">--</div>
          <div class="env-label" data-i18n="humidity">Humidity</div>
        </div>
        <div class="env-tile">
          <div class="env-val" id="light-val">--</div>
          <div class="env-label" data-i18n="light">Light</div>
        </div>
      </div>
    </div>
    <div class="card">
      <div class="card-title" data-i18n="history">History</div>
      <div class="chart-wrap"><canvas id="chart"></canvas></div>
    </div>
    <div class="card">
      <div class="card-title" data-i18n="actions">Actions</div>
      <div class="actions-grid">
        <button class="primary" onclick="takeReading()" data-i18n="take_reading">Take reading</button>
        <button class="neutral" onclick="refreshAll()" data-i18n="refresh">Refresh</button>
        <button class="neutral" onclick="window.location='/status'" data-i18n="status_json">Status JSON</button>
        <button class="neutral" onclick="window.location='/debug'" data-i18n="raw_history">Raw history</button>
        <button class="danger" onclick="clearLog()" data-i18n="clear_log">Clear log</button>
        <button class="warning" onclick="window.location='/update'" data-i18n="firmware_update">Firmware Update</button>
      </div>
    </div>
    <div class="card">
      <div class="card-title" style="display:flex;justify-content:space-between;align-items:center">
        <span data-i18n="serial_monitor">Serial monitor</span>
        <div style="display:flex;gap:6px">
          <button class="neutral" style="padding:4px 10px;font-size:.72rem" onclick="clearLog2()" data-i18n="clear">Clear</button>
          <button class="neutral" style="padding:4px 10px;font-size:.72rem" id="btn-scroll" onclick="toggleScroll()" data-i18n="autoscroll_on">Auto-scroll: ON</button>
        </div>
      </div>
      <div id="serial-log"></div>
    </div>
  </div>

  <!-- ══ CONTROLS ══ -->
  <div id="panel-controls" class="tab-panel">

    <!-- Pump -->
    <div class="card">
      <div class="card-title" data-i18n="pump_control">Pump control</div>
      <div class="pump-header">
        <div class="pump-indicator">
          <div class="pump-dot" id="pump-dot"></div>
          <span class="pump-label" id="pump-label" data-i18n="pump_off">OFF</span>
        </div>
        <span class="badge auto" id="mode-badge" data-i18n="badge_auto">AUTO</span>
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
        <button class="primary"     onclick="pumpManualOn()" data-i18n="pump_on">ON</button>
        <button class="danger"      onclick="pumpManualOff()" data-i18n="pump_off">OFF</button>
        <button class="active-blue" id="btn-auto" onclick="toggleAuto()" data-i18n="auto_on">Auto: ON</button>
      </div>
      <div style="margin-top:10px;padding-top:10px;border-top:1px solid var(--border);font-size:.72rem;color:var(--sub)">
        <span data-i18n="last_run">Last run:</span> <span id="last-pump-run" style="color:var(--text);font-weight:600">never</span>
      </div>
    </div>

    <!-- LED -->
    <div class="card">
      <div class="card-title" style="display:flex;align-items:center;justify-content:space-between">
        <span data-i18n="led_title">LED</span>
        <span class="badge auto" id="led-mode-badge" data-i18n="badge_auto">AUTO</span>
      </div>
      <div style="display:flex;align-items:center;gap:10px;margin-bottom:10px">
        <div class="pump-dot" id="led-dot"></div>
        <span style="font-size:.95rem;font-weight:700" id="led-bright-label">0%</span>
      </div>
      <div class="setting-row" style="border:none;padding-bottom:0">
        <label data-i18n="brightness">Brightness</label>
        <input type="range" id="sl-led" min="0" max="100" oninput="onLedSlider()" onchange="sendLedManual()">
        <span class="slider-val" id="val-led">0%</span>
      </div>
      <div class="setting-row">
        <label data-i18n="auto_off_above">Auto off above</label>
        <input type="range" id="sl-led-thresh" min="0" max="100" oninput="onLedThreshSlider()" onchange="sendLedThreshold()">
        <span class="slider-val" id="val-led-thresh">50%</span>
      </div>
      <div class="pump-btns">
        <button class="primary"     onclick="ledSetBright(100)" data-i18n="full">Full</button>
        <button class="danger"      onclick="ledSetBright(0)" data-i18n="off">Off</button>
        <button class="active-blue" id="btn-led-auto" onclick="toggleLedAuto()" data-i18n="auto_on">Auto: ON</button>
      </div>
    </div>

  </div><!-- /panel-controls -->

  <!-- ══ SETTINGS ══ -->
  <div id="panel-settings" class="tab-panel">
    <div class="card">
      <div class="card-title" data-i18n="moisture_threshold">Moisture Threshold</div>
      <div class="setting-row">
        <label data-i18n="low_threshold">Low threshold</label>
        <input type="range" id="sl-low" min="0" max="99" oninput="onSlider()" onchange="sendThresholds()">
        <span class="slider-val" id="val-low">30%</span>
      </div>
    </div>
      <div class="card">
        <div class="card-title" data-i18n="timings">Timings</div>
      <div class="setting-row">
        <label data-i18n="sample_interval">Sample interval (s)</label>
        <input type="number" id="inp-sample" min="5" max="3600" step="5">
        <button class="neutral set-btn" onclick="sendSampleInterval()" data-i18n="set_btn">Set</button>
      </div>
      <div class="setting-row">
        <label data-i18n="run_duration">Run duration (s)</label>
        <input type="number" id="inp-duration" min="5" max="3600" step="5">
        <button class="neutral set-btn" onclick="sendDuration()" data-i18n="set_btn">Set</button>
      </div>
      <div class="setting-row">
        <label data-i18n="soak_time">Soak time (s)</label>
        <input type="number" id="inp-soak" min="5" max="7200" step="5">
        <button class="neutral set-btn" onclick="sendSoak()" data-i18n="set_btn">Set</button>
      </div>
      <div class="setting-row">
        <label data-i18n="cooldown">Cooldown (s)</label>
        <input type="number" id="inp-cooldown" min="5" step="5">
        <button class="neutral set-btn" onclick="sendCooldown()" data-i18n="set_btn">Set</button>
      </div>
    </div>
  </div>

  <!-- ══ CALIBRATION ══ -->
  <div id="panel-calib" class="tab-panel">
    <div class="card">
      <div class="card-title" style="display:flex;justify-content:space-between;align-items:center">
        <span data-i18n="live_readings">Live sensor readings</span>
        <button class="neutral" style="padding:4px 10px;font-size:.72rem" onclick="pollCalib()" data-i18n="sample_btn">&#8635; Sample</button>
      </div>
      <table class="calib-table">
        <thead><tr><th data-i18n="sensor_col">Sensor</th><th data-i18n="raw_adc">Raw ADC</th><th data-i18n="mapped">Mapped</th></tr></thead>
        <tbody>
          <tr>
            <td data-i18n="moisture_row">Moisture</td>
            <td><span class="calib-raw" id="cv-cap-raw">--</span></td>
            <td><span class="calib-pct" id="cv-cap-pct">--</span></td>
          </tr>
          <tr>
            <td data-i18n="light_ldr">Light (LDR)</td>
            <td><span class="calib-raw" id="cv-ldr-raw">--</span></td>
            <td><span class="calib-pct" id="cv-ldr-pct">--</span></td>
          </tr>
        </tbody>
      </table>
    </div>
    <div class="card">
      <div class="card-title" data-i18n="moisture_sensor">Moisture sensor</div>
      <div class="setting-row">
        <label data-i18n="dry_raw">Dry (air) raw</label>
        <input type="number" id="ci-cap-air" min="0" max="4095">
        <button class="neutral set-btn" onclick="captureRaw('ci-cap-air','cv-cap-raw')" data-i18n="use_btn">&#8593; Use</button>
      </div>
      <div class="setting-row">
        <label data-i18n="wet_raw">Wet (water) raw</label>
        <input type="number" id="ci-cap-water" min="0" max="4095">
        <button class="neutral set-btn" onclick="captureRaw('ci-cap-water','cv-cap-raw')" data-i18n="use_btn">&#8593; Use</button>
      </div>
    </div>
    <div class="card">
      <div class="card-title" data-i18n="light_sensor">Light sensor (LDR)</div>
      <div class="setting-row">
        <label data-i18n="dark_raw">Dark raw</label>
        <input type="number" id="ci-ldr-dark" min="0" max="4095">
        <button class="neutral set-btn" onclick="captureRaw('ci-ldr-dark','cv-ldr-raw')" data-i18n="use_btn">&#8593; Use</button>
      </div>
      <div class="setting-row">
        <label data-i18n="bright_raw">Bright raw</label>
        <input type="number" id="ci-ldr-bright" min="0" max="4095">
        <button class="neutral set-btn" onclick="captureRaw('ci-ldr-bright','cv-ldr-raw')" data-i18n="use_btn">&#8593; Use</button>
      </div>
    </div>
    <button class="primary" style="width:100%;margin-top:4px" onclick="saveCalib()" data-i18n="save_apply">Save &amp; apply</button>
  </div>

</div><!-- /desktop-grid -->
</main>

<!-- Bottom nav (mobile) -->
<nav class="bottom-nav">
  <button class="nav-btn active" id="nav-overview" onclick="showTab('overview')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M3 9l9-7 9 7v11a2 2 0 01-2 2H5a2 2 0 01-2-2z"/><polyline points="9 22 9 12 15 12 15 22"/></svg>
    <span data-i18n="overview">Overview</span>
  </button>
  <button class="nav-btn" id="nav-controls" onclick="showTab('controls')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="7" width="7" height="10" rx="1"/><rect x="15" y="7" width="7" height="10" rx="1"/><line x1="9" y1="10" x2="15" y2="10"/><line x1="9" y1="14" x2="15" y2="14"/></svg>
    <span data-i18n="controls">Controls</span>
  </button>
  <button class="nav-btn" id="nav-settings" onclick="showTab('settings')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 00.33 1.82l.06.06a2 2 0 010 2.83 2 2 0 01-2.83 0l-.06-.06a1.65 1.65 0 00-1.82-.33 1.65 1.65 0 00-1 1.51V21a2 2 0 01-4 0v-.09A1.65 1.65 0 009 19.4a1.65 1.65 0 00-1.82.33l-.06.06a2 2 0 01-2.83-2.83l.06-.06A1.65 1.65 0 004.68 15a1.65 1.65 0 00-1.51-1H3a2 2 0 010-4h.09A1.65 1.65 0 004.6 9a1.65 1.65 0 00-.33-1.82l-.06-.06a2 2 0 012.83-2.83l.06.06A1.65 1.65 0 009 4.68a1.65 1.65 0 001-1.51V3a2 2 0 014 0v.09a1.65 1.65 0 001 1.51 1.65 1.65 0 001.82-.33l.06-.06a2 2 0 012.83 2.83l-.06.06A1.65 1.65 0 0019.4 9a1.65 1.65 0 001.51 1H21a2 2 0 010 4h-.09a1.65 1.65 0 00-1.51 1z"/></svg>
    <span data-i18n="settings">Settings</span>
  </button>
  <button class="nav-btn" id="nav-calib" onclick="showTab('calib')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="4" y1="21" x2="4" y2="14"/><line x1="4" y1="10" x2="4" y2="3"/><line x1="12" y1="21" x2="12" y2="17"/><line x1="12" y1="13" x2="12" y2="3"/><line x1="20" y1="21" x2="20" y2="15"/><line x1="20" y1="11" x2="20" y2="3"/><circle cx="4" cy="12" r="2"/><circle cx="12" cy="15" r="2"/><circle cx="20" cy="13" r="2"/></svg>
    <span data-i18n="calib">Calib</span>
  </button>
</nav>

<div id="toast"></div>

<script>
// ── Tab navigation ──
var currentTab = 'overview';
function showTab(name) {
  currentTab = name;
  ['overview','controls','settings','logs','calib'].forEach(function(t) {
    document.getElementById('panel-' + t).classList.toggle('active', t === name);
    document.getElementById('nav-'   + t).classList.toggle('active', t === name);
  });
  if (name === 'calib') loadCalibData();
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

function fmtSensor(v, unit) {
  if (v == null || v === undefined || v !== v) return null; // null or NaN
  return v.toFixed(1) + unit;
}
function fmtLight(lx) {
  if (lx == null || lx < 0) return null;
  return lx + '%';
}
function updateEnv(d) {
  // Readings use d.st/at/ah/lx; status uses d.soil_temp/air_temp/air_humid/light
  var st = (d.st !== undefined) ? d.st : d.soil_temp;
  var at = (d.at !== undefined) ? d.at : d.air_temp;
  var ah = (d.ah !== undefined) ? d.ah : d.air_humid;
  var lx = (d.lx !== undefined) ? d.lx : d.light;
  var els = [
    {id:'soil-temp', v: fmtSensor(st, '\u00b0C')},
    {id:'air-temp',  v: fmtSensor(at, '\u00b0C')},
    {id:'air-humid', v: fmtSensor(ah, '%')},
    {id:'light-val', v: fmtLight(lx)}
  ];
  els.forEach(function(e) {
    var el = document.getElementById(e.id);
    if (e.v === null) { el.textContent = 'N/A'; el.className = 'env-val na'; }
    else              { el.textContent = e.v;   el.className = 'env-val'; }
  });
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
  lbl.textContent = s.pump_on ? t('pump_on') : t('pump_off');

  // Last pump run
  var lprEl = document.getElementById('last-pump-run');
  lprEl.textContent = (s.last_pump_run && s.last_pump_run > 0)
    ? fmtTimestamp(s.last_pump_run) : t('never');

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
      mrlbl.textContent   = T[lang].running_for(s.pump_elapsed_s || 0);
    } else {
      mrlbl.style.display = 'none';
    }
    badge.textContent = t('badge_manual'); badge.className = 'badge manual';
    btn.textContent   = t('auto_off'); btn.className = 'neutral';
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
    slbl.textContent  = T[lang].soaking_rem(soakRem);
    badge.textContent = t('badge_soaking'); badge.className = 'badge active-blue';
  } else {
    swrap.classList.remove('visible');
    badge.textContent = cd > 0 ? t('badge_cooldown') : t('badge_auto');
    badge.className   = 'badge ' + (cd > 0 ? 'cool' : 'auto');
  }

  if (s.pump_on && remaining > 0) {
    rwrap.classList.add('visible');
    rbar.style.width = Math.round((remaining / duration) * 100) + '%';
    rlbl.textContent = T[lang].running_rem(remaining);
  } else {
    rwrap.classList.remove('visible');
  }

  btn.textContent = t('auto_on'); btn.className = 'active-blue';

  if (cd > 0) {
    cwrap.classList.add('visible');
    cbar.style.width = Math.round((cd / cooldownTotal) * 100) + '%';
    clbl.textContent = T[lang].cooldown_rem(cd);
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
  // LED initial values from first status message
  document.getElementById('sl-led').value          = s.led_manual    || 50;
  document.getElementById('val-led').textContent   = (s.led_manual   || 50) + '%';
  document.getElementById('sl-led-thresh').value   = s.led_threshold || 50;
  document.getElementById('val-led-thresh').textContent = (s.led_threshold || 50) + '%';
  inputsInitialised = true;
}

// ── LED UI ──
function updateLedUI(s) {
  var bright = s.led_brightness || 0;
  var isAuto = s.led_auto !== undefined ? s.led_auto : true;
  var dot    = document.getElementById('led-dot');
  var badge  = document.getElementById('led-mode-badge');
  var btn    = document.getElementById('btn-led-auto');
  document.getElementById('led-bright-label').textContent = bright + '%';
  dot.className = 'pump-dot' + (bright > 0 ? ' on' : '');
  badge.textContent = isAuto ? t('badge_auto') : t('badge_manual');
  badge.className   = 'badge ' + (isAuto ? 'auto' : 'manual');
  btn.textContent   = isAuto ? t('auto_on') : t('auto_off');
  btn.className     = isAuto ? 'active-blue' : 'neutral';
  if (!isAuto) {
    document.getElementById('sl-led').value        = s.led_manual || 0;
    document.getElementById('val-led').textContent = (s.led_manual || 0) + '%';
  }
  document.getElementById('sl-led-thresh').value        = s.led_threshold || 50;
  document.getElementById('val-led-thresh').textContent = (s.led_threshold || 50) + '%';
}
function onLedSlider() {
  document.getElementById('val-led').textContent = document.getElementById('sl-led').value + '%';
}
function onLedThreshSlider() {
  document.getElementById('val-led-thresh').textContent = document.getElementById('sl-led-thresh').value + '%';
}
function sendLedManual() {
  var v = document.getElementById('sl-led').value;
  fetch('/led/set?brightness=' + v, {method:'POST', credentials:'include'})
    .then(function(r){ return r.json(); })
    .then(function(){ toast('LED \u2192 ' + v + '%'); });
}
function sendLedThreshold() {
  var v = document.getElementById('sl-led-thresh').value;
  fetch('/led/config?threshold=' + v, {method:'POST', credentials:'include'})
    .then(function(r){ return r.json(); })
    .then(function(){ toast('LED auto threshold \u2192 ' + v + '%'); });
}
function ledSetBright(v) {
  document.getElementById('sl-led').value        = v;
  document.getElementById('val-led').textContent = v + '%';
  fetch('/led/set?brightness=' + v, {method:'POST', credentials:'include'})
    .then(function(r){ return r.json(); })
    .then(function(){ toast(v === 0 ? t('toast_led_off') : t('toast_led_full')); });
}
function toggleLedAuto() {
  var cur = document.getElementById('btn-led-auto').textContent.indexOf('ON') >= 0;
  fetch('/led/config?auto=' + (cur ? '0' : '1'), {method:'POST', credentials:'include'})
    .then(function(r){ return r.json(); })
    .then(function(){ toast('LED auto ' + (cur ? t('pump_off') : t('pump_on'))); });
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
      updateEnv(last);
      chart.data.labels              = d.map(function(e){ return fmtLabel(e.t); });
      chart.data.datasets[0].data   = d.map(function(e){ return e.m; });
      chart.update();
    })
    .catch(function(){ toast('Could not load data'); });
}

function refreshAll() { loadData(); toast(t('toast_refresh')); }

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
      document.getElementById('updated').textContent = t('no_data');
      toast('Log cleared');
    });
}

// ── Settings ──
function onSlider() {
  document.getElementById('val-low').textContent = document.getElementById('sl-low').value + '%';
}
function sendThresholds() {
  fetch('/pump/config?low=' + document.getElementById('sl-low').value, {credentials:'include'})
    .then(function(r){ return r.json(); }).then(function(){ toast(t('toast_threshold')); });
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
    .then(function(){ toast(t('toast_pump_on')); });
}
function pumpManualOff() {
  fetch('/pump/off',  {credentials:'include'}).then(function(r){ return r.json(); })
    .then(function(){ toast(t('toast_pump_off')); });
}
function toggleAuto() {
  fetch('/pump/auto', {credentials:'include'}).then(function(r){ return r.json(); })
    .then(function(j){ toast('Auto mode ' + (j.auto ? 'ON' : 'OFF')); });
}

// ── Calibration ──
var calibInitialised = false;

function loadCalibData() {
  fetch('/calib', {credentials:'include'})
    .then(function(r){ return r.json(); })
    .then(function(d) {
      document.getElementById('cv-cap-raw').textContent = d.cap_raw;
      document.getElementById('cv-ldr-raw').textContent = d.ldr_raw;
      document.getElementById('cv-cap-pct').textContent = d.cap_pct >= 0 ? d.cap_pct + '%' : 'N/A';
      document.getElementById('cv-ldr-pct').textContent = d.ldr_pct >= 0 ? d.ldr_pct + '%' : 'N/A';
      if (!calibInitialised) {
        document.getElementById('ci-cap-air').value    = d.cap_air;
        document.getElementById('ci-cap-water').value  = d.cap_water;
        document.getElementById('ci-ldr-dark').value   = d.ldr_dark;
        document.getElementById('ci-ldr-bright').value = d.ldr_bright;
        calibInitialised = true;
      }
    })
    .catch(function(){ toast('Calibration fetch failed'); });
}

function pollCalib() { loadCalibData(); }

function captureRaw(inputId, rawId) {
  var raw = document.getElementById(rawId).textContent;
  if (raw === '--') { toast(t('err_capture')); return; }
  document.getElementById(inputId).value = raw;
  toast(t('val_captured'));
}

function saveCalib() {
  var capAir    = parseInt(document.getElementById('ci-cap-air').value,    10);
  var capWater  = parseInt(document.getElementById('ci-cap-water').value,  10);
  var ldrDark   = parseInt(document.getElementById('ci-ldr-dark').value,   10);
  var ldrBright = parseInt(document.getElementById('ci-ldr-bright').value, 10);
  if ([capAir, capWater, ldrDark, ldrBright].some(isNaN)) { toast(t('err_numbers')); return; }
  if (capAir === capWater)   { toast(t('err_zero_moist')); return; }
  if (ldrDark === ldrBright) { toast(t('err_zero_light')); return; }
  var body = 'cap_air='  + capAir  + '&cap_water='  + capWater +
             '&ldr_dark=' + ldrDark + '&ldr_bright=' + ldrBright;
  fetch('/calib', {
    method: 'POST', credentials: 'include',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: body
  })
    .then(function(r){ return r.json(); })
    .then(function(j){
      if (j.ok) { toast(t('toast_calib_saved')); calibInitialised = false; loadCalibData(); }
      else      toast(t('toast_calib_fail') + (j.error ? ': ' + j.error : ''));
    })
    .catch(function(){ toast('Save failed'); });
}

// ── Serial log ──
var autoScroll = true;
function clearLog2() { document.getElementById('serial-log').innerHTML = ''; }
function toggleScroll() {
  autoScroll = !autoScroll;
  document.getElementById('btn-scroll').textContent = autoScroll ? t('autoscroll_on') : t('autoscroll_off');
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
        ? d.wifi_ssid + ' (' + d.wifi_rssi + ' dBm)' : t('offline');
      document.getElementById('st-heap').textContent   = Math.round(d.free_heap / 1024) + 'K';
      document.getElementById('st-spiffs').textContent =
        Math.round(d.spiffs_used / 1024) + '/' + Math.round(d.spiffs_total / 1024) + 'K';
      initInputs(d);
      updatePumpUI(d);
      updateLedUI(d);
      updateEnv(d);
    } else if (d.type === 'reading') {
      document.getElementById('now').textContent     = d.m + '%';
      setMoistureColor(d.m);
      document.getElementById('updated').textContent = fmtTimestamp(d.t);
      updateEnv(d);
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
    document.getElementById('wlabel').textContent = t('reconnecting');
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

// ── i18n ──────────────────────────────────────────────
var T = {
  en: {
    app_title:'Soil Monitor', free_ram:'Free RAM', storage:'Storage',
    connecting:'connecting...', reconnecting:'reconnecting...', offline:'offline',
    lang_btn:'AR',
    overview:'Overview', controls:'Controls', settings:'Settings', calib:'Calib',
    soil_moisture:'Soil moisture', no_data:'no data yet',
    environment:'Environment', soil_temp:'Soil temp', air_temp:'Air temp',
    humidity:'Humidity', light:'Light', history:'History', actions:'Actions',
    take_reading:'Take reading', refresh:'Refresh', status_json:'Status JSON',
    raw_history:'Raw history', clear_log:'Clear log', firmware_update:'Firmware Update',
    serial_monitor:'Serial monitor', clear:'Clear',
    autoscroll_on:'Auto-scroll: ON', autoscroll_off:'Auto-scroll: OFF',
    pump_control:'Pump control', pump_on:'ON', pump_off:'OFF',
    auto_on:'Auto: ON', auto_off:'Auto: OFF',
    last_run:'Last run:', never:'never',
    badge_auto:'AUTO', badge_manual:'MANUAL', badge_soaking:'SOAKING', badge_cooldown:'COOLDOWN',
    led_title:'LED', brightness:'Brightness', auto_off_above:'Auto off above',
    full:'Full', off:'Off',
    moisture_threshold:'Moisture Threshold', timings:'Timings',
    low_threshold:'Low threshold', sample_interval:'Sample interval (s)',
    run_duration:'Run duration (s)', soak_time:'Soak time (s)', cooldown:'Cooldown (s)',
    set_btn:'Set',
    live_readings:'Live sensor readings', sample_btn:'\u27f3 Sample',
    sensor_col:'Sensor', raw_adc:'Raw ADC', mapped:'Mapped',
    moisture_row:'Moisture', light_ldr:'Light (LDR)',
    moisture_sensor:'Moisture sensor', dry_raw:'Dry (air) raw', wet_raw:'Wet (water) raw',
    light_sensor:'Light sensor (LDR)', dark_raw:'Dark raw', bright_raw:'Bright raw',
    save_apply:'Save & apply', use_btn:'\u2191 Use',
    chart_label:'Moisture %',
    running_for:function(s){return 'Running for '+s+'s';},
    soaking_rem:function(s){return 'Soaking \u2014 '+s+'s remaining';},
    running_rem:function(s){return 'Running \u2014 '+s+'s remaining';},
    cooldown_rem:function(s){return 'Cooldown: '+s+'s remaining';},
    toast_refresh:'Refreshed', toast_threshold:'Threshold updated',
    toast_led_off:'LED OFF', toast_led_full:'LED Full',
    toast_calib_saved:'Calibration saved \u2714', toast_calib_fail:'Error: save failed',
    toast_pump_on:'Pump ON', toast_pump_off:'Pump OFF',
    err_numbers:'All fields must be numbers',
    err_zero_moist:'Moisture range cannot be zero',
    err_zero_light:'Light range cannot be zero',
    err_capture:'Click Sample first to get a reading',
    val_captured:'Value captured \u2014 click Save to apply'
  },
  ar: {
    app_title:'\u0645\u0631\u0627\u0642\u0628 \u0627\u0644\u062a\u0631\u0628\u0629',
    free_ram:'\u0630\u0627\u0643\u0631\u0629 \u062d\u0631\u0629',
    storage:'\u0627\u0644\u062a\u062e\u0632\u064a\u0646',
    connecting:'\u062c\u0627\u0631\u064a \u0627\u0644\u0627\u062a\u0635\u0627\u0644...',
    reconnecting:'\u0625\u0639\u0627\u062f\u0629 \u0627\u0644\u0627\u062a\u0635\u0627\u0644...',
    offline:'\u063a\u064a\u0631 \u0645\u062a\u0635\u0644',
    lang_btn:'EN',
    overview:'\u0646\u0638\u0631\u0629 \u0639\u0627\u0645\u0629',
    controls:'\u0627\u0644\u062a\u062d\u0643\u0645',
    settings:'\u0627\u0644\u0625\u0639\u062f\u0627\u062f\u0627\u062a',
    calib:'\u0627\u0644\u0645\u0639\u0627\u064a\u0631\u0629',
    soil_moisture:'\u0631\u0637\u0648\u0628\u0629 \u0627\u0644\u062a\u0631\u0628\u0629',
    no_data:'\u0644\u0627 \u062a\u0648\u062c\u062f \u0628\u064a\u0627\u0646\u0627\u062a',
    environment:'\u0627\u0644\u0628\u064a\u0626\u0629',
    soil_temp:'\u062d\u0631\u0627\u0631\u0629 \u0627\u0644\u062a\u0631\u0628\u0629',
    air_temp:'\u062d\u0631\u0627\u0631\u0629 \u0627\u0644\u0647\u0648\u0627\u0621',
    humidity:'\u0627\u0644\u0631\u0637\u0648\u0628\u0629',
    light:'\u0627\u0644\u0625\u0636\u0627\u0621\u0629',
    history:'\u0627\u0644\u0633\u062c\u0644',
    actions:'\u0627\u0644\u0625\u062c\u0631\u0627\u0621\u0627\u062a',
    take_reading:'\u0642\u0631\u0627\u0621\u0629 \u0627\u0644\u0622\u0646',
    refresh:'\u062a\u062d\u062f\u064a\u062b',
    status_json:'\u062d\u0627\u0644\u0629 JSON',
    raw_history:'\u0627\u0644\u0628\u064a\u0627\u0646\u0627\u062a \u0627\u0644\u062e\u0627\u0645',
    clear_log:'\u0645\u0633\u062d \u0627\u0644\u0633\u062c\u0644',
    firmware_update:'\u062a\u062d\u062f\u064a\u062b \u0627\u0644\u0628\u0631\u0646\u0627\u0645\u062c',
    serial_monitor:'\u0633\u062c\u0644 \u0627\u0644\u0623\u062d\u062f\u0627\u062b',
    clear:'\u0645\u0633\u062d',
    autoscroll_on:'\u062a\u0645\u0631\u064a\u0631 \u062a\u0644\u0642\u0627\u0626\u064a: \u062a\u0634\u063a\u064a\u0644',
    autoscroll_off:'\u062a\u0645\u0631\u064a\u0631 \u062a\u0644\u0642\u0627\u0626\u064a: \u0625\u064a\u0642\u0627\u0641',
    pump_control:'\u0627\u0644\u062a\u062d\u0643\u0645 \u0628\u0627\u0644\u0645\u0636\u062e\u0629',
    pump_on:'\u062a\u0634\u063a\u064a\u0644', pump_off:'\u0625\u064a\u0642\u0627\u0641',
    auto_on:'\u062a\u0644\u0642\u0627\u0626\u064a: \u062a\u0634\u063a\u064a\u0644',
    auto_off:'\u062a\u0644\u0642\u0627\u0626\u064a: \u0625\u064a\u0642\u0627\u0641',
    last_run:'\u0622\u062e\u0631 \u062a\u0634\u063a\u064a\u0644:', never:'\u0623\u0628\u062f\u0627\u064b',
    badge_auto:'\u062a\u0644\u0642\u0627\u0626\u064a',
    badge_manual:'\u064a\u062f\u0648\u064a',
    badge_soaking:'\u0646\u0642\u0639',
    badge_cooldown:'\u0631\u0627\u062d\u0629',
    led_title:'\u0625\u0636\u0627\u0621\u0629 LED',
    brightness:'\u0627\u0644\u0633\u0637\u0648\u0639',
    auto_off_above:'\u0625\u064a\u0642\u0627\u0641 \u062a\u0644\u0642\u0627\u0626\u064a \u0641\u0648\u0642',
    full:'\u0643\u0627\u0645\u0644', off:'\u0625\u064a\u0642\u0627\u0641',
    moisture_threshold:'\u062d\u062f \u0627\u0644\u0631\u0637\u0648\u0628\u0629',
    timings:'\u0627\u0644\u062a\u0648\u0642\u064a\u062a\u0627\u062a',
    low_threshold:'\u0627\u0644\u062d\u062f \u0627\u0644\u0623\u062f\u0646\u0649',
    sample_interval:'\u0641\u062a\u0631\u0629 \u0627\u0644\u0642\u064a\u0627\u0633 (\u062b)',
    run_duration:'\u0645\u062f\u0629 \u0627\u0644\u062a\u0634\u063a\u064a\u0644 (\u062b)',
    soak_time:'\u0648\u0642\u062a \u0627\u0644\u0646\u0642\u0639 (\u062b)',
    cooldown:'\u0641\u062a\u0631\u0629 \u0627\u0644\u0631\u0627\u062d\u0629 (\u062b)',
    set_btn:'\u062d\u0641\u0638',
    live_readings:'\u0642\u0631\u0627\u0621\u0627\u062a \u0645\u0628\u0627\u0634\u0631\u0629',
    sample_btn:'\u27f3 \u0642\u064a\u0627\u0633',
    sensor_col:'\u0627\u0644\u0645\u0633\u062a\u0634\u0639\u0631',
    raw_adc:'ADC \u0627\u0644\u062e\u0627\u0645',
    mapped:'\u0627\u0644\u0645\u0639\u064a\u064e\u0651\u0631',
    moisture_row:'\u0627\u0644\u0631\u0637\u0648\u0628\u0629',
    light_ldr:'\u0627\u0644\u0636\u0648\u0621 (LDR)',
    moisture_sensor:'\u0645\u0633\u062a\u0634\u0639\u0631 \u0627\u0644\u0631\u0637\u0648\u0628\u0629',
    dry_raw:'\u062c\u0627\u0641 (\u0647\u0648\u0627\u0621)',
    wet_raw:'\u0645\u0628\u0644\u0644 (\u0645\u0627\u0621)',
    light_sensor:'\u0645\u0633\u062a\u0634\u0639\u0631 \u0627\u0644\u0636\u0648\u0621 (LDR)',
    dark_raw:'\u0645\u0638\u0644\u0645', bright_raw:'\u0645\u0634\u0631\u0642',
    save_apply:'\u062d\u0641\u0638 \u0648\u062a\u0637\u0628\u064a\u0642',
    use_btn:'\u2191 \u0627\u0633\u062a\u062e\u062f\u0645',
    chart_label:'\u0627\u0644\u0631\u0637\u0648\u0628\u0629 %',
    running_for:function(s){return '\u064a\u0639\u0645\u0644 \u0645\u0646\u0630 '+s+'\u062b';},
    soaking_rem:function(s){return '\u0646\u0642\u0639 \u2014 \u0645\u062a\u0628\u0642\u064a '+s+'\u062b';},
    running_rem:function(s){return '\u064a\u0639\u0645\u0644 \u2014 \u0645\u062a\u0628\u0642\u064a '+s+'\u062b';},
    cooldown_rem:function(s){return '\u0631\u0627\u062d\u0629: \u0645\u062a\u0628\u0642\u064a '+s+'\u062b';},
    toast_refresh:'\u062a\u0645 \u0627\u0644\u062a\u062d\u062f\u064a\u062b',
    toast_threshold:'\u062a\u0645 \u062a\u062d\u062f\u064a\u062b \u0627\u0644\u062d\u062f',
    toast_led_off:'LED \u0625\u064a\u0642\u0627\u0641',
    toast_led_full:'LED \u0643\u0627\u0645\u0644',
    toast_calib_saved:'\u062a\u0645 \u062d\u0641\u0638 \u0627\u0644\u0645\u0639\u0627\u064a\u0631\u0629 \u2714',
    toast_calib_fail:'\u062e\u0637\u0623: \u0641\u0634\u0644 \u0627\u0644\u062d\u0641\u0638',
    toast_pump_on:'\u062a\u0634\u063a\u064a\u0644 \u0627\u0644\u0645\u0636\u062e\u0629',
    toast_pump_off:'\u0625\u064a\u0642\u0627\u0641 \u0627\u0644\u0645\u0636\u062e\u0629',
    err_numbers:'\u062c\u0645\u064a\u0639 \u0627\u0644\u062d\u0642\u0648\u0644 \u064a\u062c\u0628 \u0623\u0646 \u062a\u0643\u0648\u0646 \u0623\u0631\u0642\u0627\u0645\u0627\u064b',
    err_zero_moist:'\u0646\u0637\u0627\u0642 \u0627\u0644\u0631\u0637\u0648\u0628\u0629 \u0644\u0627 \u064a\u0645\u0643\u0646 \u0623\u0646 \u064a\u0643\u0648\u0646 \u0635\u0641\u0631\u0627\u064b',
    err_zero_light:'\u0646\u0637\u0627\u0642 \u0627\u0644\u0636\u0648\u0621 \u0644\u0627 \u064a\u0645\u0643\u0646 \u0623\u0646 \u064a\u0643\u0648\u0646 \u0635\u0641\u0631\u0627\u064b',
    err_capture:'\u0627\u0636\u063a\u0637 \u0642\u064a\u0627\u0633 \u0623\u0648\u0644\u0627\u064b \u0644\u0644\u062d\u0635\u0648\u0644 \u0639\u0644\u0649 \u0642\u0631\u0627\u0621\u0629',
    val_captured:'\u062a\u0645 \u0627\u0644\u062a\u0642\u0627\u0637 \u0627\u0644\u0642\u064a\u0645\u0629 \u2014 \u0627\u0636\u063a\u0637 \u062d\u0641\u0638 \u0644\u0644\u062a\u0637\u0628\u064a\u0642'
  }
};
var lang = localStorage.getItem('lang') || 'en';
function t(key) { var v = T[lang][key]; return (v !== undefined) ? v : (T['en'][key] || key); }

function setLang(newLang) {
  lang = newLang;
  localStorage.setItem('lang', lang);
  var root = document.getElementById('html-root');
  root.setAttribute('dir',  lang === 'ar' ? 'rtl' : 'ltr');
  root.setAttribute('lang', lang);
  document.querySelectorAll('[data-i18n]').forEach(function(el) {
    var val = T[lang][el.dataset.i18n];
    if (typeof val === 'string') el.textContent = val;
  });
  document.getElementById('btn-lang').textContent = t('lang_btn');
  chart.data.datasets[0].label = t('chart_label');
  chart.update();
  // Update no-data placeholder if no reading received yet
  if (!inputsInitialised) {
    document.getElementById('updated').textContent = t('no_data');
  }
}
function toggleLang() { setLang(lang === 'en' ? 'ar' : 'en'); }

loadData();
connectWS();
// Apply saved language preference on load
setLang(lang);
</script>
</body></html>
)rawhtml";

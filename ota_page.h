#pragma once

const char UPDATE_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html><html><head>
<meta charset='utf-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Firmware Update</title>
<style>
  body{font-family:sans-serif;padding:24px;max-width:400px;margin:0 auto;background:#f4f6f8}
  h2{margin-bottom:16px;font-size:1.1rem}
  .card{background:#fff;border:1px solid #e5e7eb;border-radius:16px;padding:20px}
  input[type=file]{margin-bottom:12px;width:100%;font-size:.85rem}
  button{padding:12px 24px;background:#2a7d4f;color:#fff;border:none;
         border-radius:8px;font-size:1rem;cursor:pointer;width:100%}
  button:disabled{background:#9ca3af;cursor:not-allowed}
  #status{margin-top:16px;font-size:.85rem;color:#6b7280}
  progress{width:100%;margin-top:12px;display:none;accent-color:#2a7d4f}
  .back{display:inline-block;margin-bottom:16px;font-size:.85rem;
        color:#2a7d4f;text-decoration:none}
</style></head><body>
<a class='back' href='/'>← Back to dashboard</a>
<div class='card'>
  <h2>Firmware Update</h2>
  <input type='file' id='bin' accept='.bin'>
  <button id='btn' onclick='upload()'>Upload & Update</button>
  <progress id='bar' value='0' max='100'></progress>
  <div id='status'>Select a .bin file to begin</div>
</div>
<script>
function upload() {
  var file = document.getElementById('bin').files[0];
  if (!file) { alert('Select a .bin file first'); return; }
  // Only accept the sketch-only .bin — not the merged binary
  if (file.name.indexOf('merged') !== -1) {
    alert('Wrong file — use the sketch .bin, not the .merged.bin');
    return;
  }
  var btn    = document.getElementById('btn');
  var bar    = document.getElementById('bar');
  var status = document.getElementById('status');
  btn.disabled      = true;
  bar.style.display = 'block';
  var fd = new FormData();
  fd.append('firmware', file, file.name);
  var xhr = new XMLHttpRequest();
  xhr.upload.onprogress = function(e) {
    if (e.lengthComputable) {
      var pct = Math.round(e.loaded / e.total * 100);
      bar.value = pct;
      status.textContent = 'Uploading... ' + pct + '%';
    }
  };
  xhr.onload = function() {
    if (xhr.status == 200) {
      status.textContent = 'Update complete — device rebooting...';
      setTimeout(function(){ window.location='/'; }, 10000);
    } else {
      status.textContent = 'Update failed: ' + xhr.responseText;
      btn.disabled = false;
      bar.style.display = 'none';
    }
  };
  xhr.onerror = function() {
    status.textContent = 'Upload error — check connection';
    btn.disabled = false;
    bar.style.display = 'none';
  };
  xhr.open('POST', '/update');
  xhr.send(fd);
}
</script></body></html>
)rawhtml";
(function () {
  'use strict';

  var CLIMATE_NAMES = ['temperate', 'coastal', 'desert', 'mountain'];

  var state = {
    port: null,
    reader: null,
    connected: false,
    connectTime: 0,
    pSamples: [],
    peakRms: 0,
    wakeCount: 0,
    wifiTimer: null,
    db: null,
    persistCount: 0
  };

  var dbReady = new Promise(function (resolve) {
    if (!('indexedDB' in window)) return resolve(null);
    var req = indexedDB.open('DeskPalTelemetry', 1);
    req.onupgradeneeded = function () {
      var db = req.result;
      if (!db.objectStoreNames.contains('samples')) db.createObjectStore('samples', { keyPath: 'id', autoIncrement: true });
      if (!db.objectStoreNames.contains('events')) db.createObjectStore('events', { keyPath: 'id', autoIncrement: true });
    };
    req.onsuccess = function () { state.db = req.result; resolve(req.result); };
    req.onerror = function () { resolve(null); };
  });

  function persist(store, value) {
    dbReady.then(function (db) {
      if (!db) return;
      var tx = db.transaction(store, 'readwrite');
      var objectStore = tx.objectStore(store);
      objectStore.add({ time: Date.now(), value: value });
      state.persistCount++;
      var limit = store === 'samples' ? 20000 : 2000;
      var cadence = store === 'samples' ? 30 : 20;
      if (state.persistCount % cadence !== 0) return;
      var count = objectStore.count();
      count.onsuccess = function () {
        var excess = count.result - limit;
        if (excess <= 0) return;
        var cursor = objectStore.openCursor();
        cursor.onsuccess = function () {
          var row = cursor.result;
          if (row && excess-- > 0) { row.delete(); row.continue(); }
        };
      };
    });
  }

  async function exportHistory() {
    var db = await dbReady;
    if (!db) { el('connStatus').textContent = 'IndexedDB is unavailable in this browser'; return; }
    var tx = db.transaction(['samples', 'events'], 'readonly');
    var result = await Promise.all(['samples', 'events'].map(function (name) {
      return new Promise(function (resolve, reject) {
        var req = tx.objectStore(name).getAll();
        req.onsuccess = function () { resolve(req.result); };
        req.onerror = function () { reject(req.error); };
      });
    }));
    var blob = new Blob([JSON.stringify({ exportedAt: new Date().toISOString(), samples: result[0], events: result[1] }, null, 2)], { type: 'application/json' });
    var link = document.createElement('a'); link.href = URL.createObjectURL(blob); link.download = 'deskpal-history.json'; link.click();
    setTimeout(function () { URL.revokeObjectURL(link.href); }, 1000);
  }

  async function clearHistory() {
    if (!window.confirm('Clear all DeskPal history stored in this browser?')) return;
    var db = await dbReady;
    if (!db) return;
    var tx = db.transaction(['samples', 'events'], 'readwrite');
    tx.objectStore('samples').clear(); tx.objectStore('events').clear();
    el('events').tBodies[0].replaceChildren();
    state.pSamples = []; drawSpark(null);
    logRow(timeStr(), 'System', 'local history cleared');
  }

  function el(id) { return document.getElementById(id); }
  function set(id, text) { el(id).textContent = text; }

  function timeStr() {
    var d = new Date();
    function p(n) { return (n < 10 ? '0' : '') + n; }
    return p(d.getHours()) + ':' + p(d.getMinutes()) + ':' + p(d.getSeconds());
  }

  function catOf(m) {
    if (m.indexOf('MiniAI') === 0) return 'Mini-AI';
    if (m.indexOf('Wake word') === 0) return 'KWS';
    if (m.indexOf('Anomaly') === 0) return 'Anomaly';
    if (m.indexOf('Picked up') === 0) return 'Lift';
    if (m.indexOf('Mic') === 0) return 'Mic';
    if (m.indexOf('Over-petted') === 0 || m.indexOf('Long cuddle') === 0) return 'Touch';
    return 'System';
  }

  function logRow(time, source, msg) {
    var tbody = el('events').tBodies[0];
    var tr = tbody.insertRow(0);
    tr.insertCell(0).textContent = time;
    tr.insertCell(1).textContent = source;
    tr.insertCell(2).textContent = msg;
    while (tbody.rows.length > 200) tbody.deleteRow(tbody.rows.length - 1);
    persist('events', { time: time, source: source, message: msg });
  }

  function drawSpark(arr) {
    var c = el('spark');
    var ctx = c.getContext('2d');
    var W = c.width, H = c.height;
    ctx.clearRect(0, 0, W, H);
    ctx.fillStyle = '#f5e4ea'; 
    ctx.fillRect(0, 0, W, H);
    if (!arr || arr.length < 2) {
      ctx.fillStyle = '#865a6e'; 
      ctx.font = '13px "JetBrains Mono", Consolas, monospace';
      ctx.fillText('waiting for device temperature buffer', 16, H / 2);
      return;
    }
    var mn = Math.min.apply(null, arr);
    var mx = Math.max.apply(null, arr);
    if (mx - mn < 0.5) { var mid = (mx + mn) / 2; mn = mid - 0.25; mx = mid + 0.25; }
    var padL = 52, padR = 16, padT = 16, padB = 16;
    
    // Draw guide lines
    ctx.strokeStyle = '#ecd4de';
    ctx.lineWidth = 1;
    ctx.beginPath(); ctx.moveTo(padL, padT); ctx.lineTo(W - padR, padT); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(padL, H - padB); ctx.lineTo(W - padR, H - padB); ctx.stroke();
    
    // Draw labels
    ctx.fillStyle = '#865a6e'; 
    ctx.font = '11px "JetBrains Mono", Consolas, monospace';
    ctx.fillText(mx.toFixed(1) + ' C', 4, padT + 4);
    ctx.fillText(mn.toFixed(1) + ' C', 4, H - padB + 4);
    
    // Draw line with pink glow
    ctx.strokeStyle = '#ec4899';
    ctx.lineWidth = 2.5;
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';
    ctx.beginPath();
    for (var i = 0; i < arr.length; i++) {
      var x = padL + i * (W - padL - padR) / (arr.length - 1);
      var y = padT + (mx - arr[i]) * (H - padT - padB) / (mx - mn);
      if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    }
    ctx.stroke();
    
    // Draw end point indicator
    var lx = padL + (arr.length - 1) * (W - padL - padR) / (arr.length - 1);
    var ly = padT + (mx - arr[arr.length - 1]) * (H - padT - padB) / (mx - mn);
    ctx.fillStyle = '#db2777';
    ctx.beginPath();
    ctx.arc(lx, ly, 5, 0, 2 * Math.PI);
    ctx.fill();
    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 1.5;
    ctx.stroke();
  }

  function updatePressure(p) {
    var now = Date.now();
    state.pSamples.push({ ts: now, p: p });
    if (state.pSamples.length > 7200) state.pSamples.shift();
    var first = state.pSamples[0];
    var delta = p - first.p;
    set('pNow', p.toFixed(1) + ' hPa');
    set('pStart', first.p.toFixed(1) + ' hPa');
    set('pDelta', (delta >= 0 ? '+' : '') + delta.toFixed(2) + ' hPa');
    var cls = 'stable';
    if (delta <= -1.5) cls = 'falling';
    else if (delta >= 1.5) cls = 'rising';
    set('pClass', cls);
    var mins = Math.max(1, Math.round((now - state.connectTime) / 60000));
    set('pWindow', mins + ' min');
  }

  function applyTelemetry(t) {
    t.t = t.t === null ? null : Number(t.t); t.h = t.h === null ? null : Number(t.h); t.p = t.p === null ? null : Number(t.p);
    t.infMs = Number(t.infMs); t.batt = t.batt === null ? null : Number(t.batt); t.micRms = Number(t.micRms);
    set('vTemp', t.t === null ? 'unavailable' : t.t.toFixed(1) + ' C');
    set('vHum', t.h === null ? 'unavailable' : t.h + ' %');
    set('vPress', t.p === null ? 'unavailable' : t.p + ' hPa');
    set('vMood', t.mood);
    set('vComfort', t.comfort);
    set('vAnom', t.anomaly ? 'active' : 'clear');
    el('tileAnom').className = 'tile' + (t.anomaly ? ' bad' : '');
    set('vInf', Number.isFinite(t.infMs) ? t.infMs.toFixed(2) + ' ms' : 'unavailable');
    set('vClimate', CLIMATE_NAMES[t.climate] || ('zone ' + t.climate));
    set('vBatt', t.batt === null ? 'not installed' : t.batt + ' %');
    set('vMute', t.micReady === false ? 'not detected' : (t.micMute ? 'muted' : 'open'));
    set('vSensorState', t.sensorsOk === false ? 'offline' : (Number(t.sensorAgeMs) > 10000 ? 'stale' : 'ready'));
    set('vModelState', t.modelReady ? 'ready' : 'not installed');

    set('vRms', Math.round(t.micRms));
    if (t.micRms > state.peakRms) state.peakRms = t.micRms;
    set('vPeak', Math.round(state.peakRms));
    var pct = state.peakRms > 0 ? Math.min(100, (t.micRms / state.peakRms) * 100) : 0;
    el('barFill').style.width = pct.toFixed(0) + '%';

    if (Array.isArray(t.histT) && t.histT.length) drawSpark(t.histT);
    if (Number.isFinite(t.p)) updatePressure(t.p);
    persist('samples', t);
  }

  function handleLine(line) {
    var obj;
    try { obj = JSON.parse(line); } catch (e) { return; }
    if (!obj || typeof obj !== 'object') return;
    if (obj.type === 'telemetry') {
      applyTelemetry(obj);
    } else if (obj.type === 'decision' && typeof obj.msg === 'string') {
      if (obj.msg.indexOf('Wake word') === 0) {
        state.wakeCount++;
        set('vWake', String(state.wakeCount));
      }
      logRow(timeStr(), catOf(obj.msg), obj.msg);
    }
  }

  function updateConnUI() {
    var s = el('connStatus');
    if (state.connected) {
      s.textContent = state.wifiTimer ? 'connected over Wi-Fi' : 'connected over USB serial';
      s.className = 'status on';
      el('btnConnect').disabled = true;
      el('btnDisconnect').disabled = false;
    } else {
      s.textContent = 'not connected';
      s.className = 'status off';
      el('btnConnect').disabled = false;
      el('btnDisconnect').disabled = true;
    }
  }

  function resetTiles() {
    var ids = ['vTemp','vHum','vPress','vMood','vComfort','vAnom','vInf','vClimate','vBatt','vMute','vSensorState','vModelState',
               'pNow','pStart','pDelta','pClass','pWindow','vRms','vPeak'];
    for (var i = 0; i < ids.length; i++) set(ids[i], 'no link');
    el('tileAnom').className = 'tile';
    el('barFill').style.width = '0%';
    drawSpark(null);
  }

  async function readLoop() {
    var decoder = new TextDecoder();
    var buf = '';
    while (state.port && state.port.readable) {
      var reader = state.port.readable.getReader();
      state.reader = reader;
      try {
        for (;;) {
          var res = await reader.read();
          if (res.done) break;
          buf += decoder.decode(res.value, { stream: true });
          var idx;
          while ((idx = buf.indexOf('\n')) >= 0) {
            var line = buf.slice(0, idx).trim();
            buf = buf.slice(idx + 1);
            if (line.charAt(0) === '{') handleLine(line);
          }
        }
      } catch (e) {
        break;
      } finally {
        reader.releaseLock();
        state.reader = null;
      }
      break;
    }
    state.connected = false;
    updateConnUI();
    resetTiles();
  }

  async function connect() {
    if (!('serial' in navigator)) {
      el('connStatus').textContent = 'Web Serial unsupported, use Chrome or Edge';
      el('connStatus').className = 'status off';
      return;
    }
    try {
      var port = await navigator.serial.requestPort();
      await port.open({ baudRate: 115200 });
      state.port = port;
      state.connected = true;
      state.connectTime = Date.now();
      state.pSamples = [];
      state.peakRms = 0;
      state.wakeCount = 0;
      set('vWake', '0');
      updateConnUI();
      logRow(timeStr(), 'System', 'dashboard connected');
      readLoop();
    } catch (e) {
      el('connStatus').textContent = 'connect failed: ' + e.message;
      el('connStatus').className = 'status off';
    }
  }

  async function disconnect() {
    if (state.wifiTimer) { clearInterval(state.wifiTimer); state.wifiTimer = null; }
    try { if (state.reader) await state.reader.cancel(); } catch (e) {}
    try { if (state.port) await state.port.close(); } catch (e) {}
    state.port = null;
    state.reader = null;
    state.connected = false;
    updateConnUI();
    resetTiles();
    logRow(timeStr(), 'System', 'dashboard disconnected');
  }

  async function connectWifi() {
    var base = el('deviceUrl').value.trim().replace(/\/$/, '');
    var token = el('deviceToken').value.trim();
    if (!base) return;
    if (!token) { el('connStatus').textContent = 'Enter the device token printed in Serial Monitor'; el('connStatus').className = 'status off'; return; }
    try {
      var response = await fetch(base + '/api/telemetry', { cache: 'no-store', headers: { 'X-DeskPal-Token': token } });
      if (!response.ok) throw new Error('HTTP ' + response.status);
      var data = await response.json();
      if (!data || data.type !== 'telemetry') throw new Error('invalid device response');
      await disconnect();
      state.connected = true; state.connectTime = Date.now(); state.pSamples = []; state.peakRms = 0;
      handleLine(JSON.stringify(data));
      state.wifiTimer = setInterval(async function () {
        try {
          var res = await fetch(base + '/api/telemetry', { cache: 'no-store', headers: { 'X-DeskPal-Token': token } });
          if (!res.ok) throw new Error('HTTP ' + res.status);
          handleLine(JSON.stringify(await res.json()));
        } catch (e) {
          clearInterval(state.wifiTimer); state.wifiTimer = null; state.connected = false;
          el('connStatus').textContent = 'Wi-Fi disconnected: ' + e.message; el('connStatus').className = 'status off';
          updateConnUI();
        }
      }, 1000);
      updateConnUI();
    } catch (e) {
      el('connStatus').textContent = 'Wi-Fi connect failed: ' + e.message; el('connStatus').className = 'status off';
    }
  }

  // DeskPal 2D Pixel Eyes Animation
  function initPixelEyes() {
    var c = el('deskpalEyes');
    if (!c) return;
    var ctx = c.getContext('2d');

    var P = 5; // Pixel unit size (matches the 2D grid)
    var eyeW = 7;
    var eyeH = 9;
    var eyeGap = 3;
    var totalW = eyeW * 2 + eyeGap;
    var startX = Math.round((c.width - totalW * P) / 2);
    var startY = Math.round((c.height - eyeH * P) / 2);

    // 0 = background (#ffffff), 1 = black (#0f0e14), 2 = white highlight/sclera (#ffffff)
    var EYE_OPEN = [
      [0,0,1,1,1,0,0],
      [0,1,1,2,2,1,0],
      [1,1,1,1,2,2,1],
      [1,1,2,1,2,2,1], // Specular highlight at [3,2]
      [1,1,1,1,2,2,1],
      [1,1,1,1,2,2,1],
      [1,1,1,1,2,2,1],
      [0,1,1,2,2,1,0],
      [0,0,1,1,1,0,0]
    ];

    var EYE_HALF = [
      [0,0,1,1,1,0,0],
      [0,1,1,2,2,1,0],
      [1,1,2,1,2,2,1],
      [0,1,1,2,2,1,0],
      [0,0,1,1,1,0,0]
    ];

    var EYE_CLOSED = [
      [0,1,1,1,1,1,0],
      [1,1,1,1,1,1,1]
    ];

    var blinkStage = 0; // 0=open, 1=half, 2=closed
    var isBlinking = false;

    function drawEye(matrix, originX, originY) {
      for (var r = 0; r < matrix.length; r++) {
        for (var col = 0; col < matrix[r].length; col++) {
          var val = matrix[r][col];
          if (val === 1) {
            ctx.fillStyle = '#0f0e14';
            ctx.fillRect(originX + col * P, originY + r * P, P, P);
          } else if (val === 2) {
            ctx.fillStyle = '#ffffff';
            ctx.fillRect(originX + col * P, originY + r * P, P, P);
          }
        }
      }
    }

    function render() {
      ctx.fillStyle = '#f5e4ea';
      ctx.fillRect(0, 0, c.width, c.height);

      var leftX = startX;
      var rightX = startX + (eyeW + eyeGap) * P;

      if (blinkStage === 0) {
        drawEye(EYE_OPEN, leftX, startY);
        drawEye(EYE_OPEN, rightX, startY);
      } else if (blinkStage === 1) {
        var halfY = startY + 2 * P;
        drawEye(EYE_HALF, leftX, halfY);
        drawEye(EYE_HALF, rightX, halfY);
      } else if (blinkStage === 2) {
        var closedY = startY + 4 * P;
        drawEye(EYE_CLOSED, leftX, closedY);
        drawEye(EYE_CLOSED, rightX, closedY);
      }
    }

    function doBlink(onComplete) {
      if (isBlinking && !onComplete) return;
      isBlinking = true;
      blinkStage = 1;
      render();
      setTimeout(function () {
        blinkStage = 2;
        render();
        setTimeout(function () {
          blinkStage = 1;
          render();
          setTimeout(function () {
            blinkStage = 0;
            render();
            isBlinking = false;
            if (onComplete) onComplete();
          }, 50);
        }, 80);
      }, 50);
    }

    function scheduleBlink() {
      var delay = 2400 + Math.random() * 3000;
      setTimeout(function () {
        doBlink(function () {
          // Occasional cute double-blink
          if (Math.random() < 0.28) {
            setTimeout(function () {
              doBlink(scheduleBlink);
            }, 120);
          } else {
            scheduleBlink();
          }
        });
      }, delay);
    }

    var faceBox = el('deskpalFaceBox');
    if (faceBox) {
      faceBox.addEventListener('click', function () {
        doBlink();
      });
    }

    render();
    scheduleBlink();
  }

  el('btnConnect').addEventListener('click', connect);
  el('btnWifi').addEventListener('click', connectWifi);
  el('btnExport').addEventListener('click', exportHistory);
  el('btnClearHistory').addEventListener('click', clearHistory);
  el('btnDisconnect').addEventListener('click', disconnect);

  updateConnUI();
  resetTiles();
  initPixelEyes();
})();

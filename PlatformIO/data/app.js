document.addEventListener('DOMContentLoaded', initApp);

let settingsLoaded = false;

// Tab navigation
function initApp() {
  initNavigation();
  initControls();
  fetchSettings();  // Load settings once on page load
  fetchStatus();    // Initial status fetch
  setInterval(fetchStatus, 1000);  // Continue fetching live data only
}

function initNavigation() {
  const tabs = document.querySelectorAll(".nav-tab");
  const pages = document.querySelectorAll(".page");

  tabs.forEach((tab) => {
    tab.addEventListener("click", () => {
      const page = tab.dataset.page;

      tabs.forEach((t) => t.classList.remove("active"));
      tab.classList.add("active");

      pages.forEach((p) => p.classList.remove("active"));
      document.getElementById(`${page}-page`).classList.add("active");
    });
  });
}

function initControls() {
  const gpsRateSelect = document.getElementById('gpsRateSelect');
  const setGpsRateBtn = document.getElementById('setGpsRateBtn');
  const gpsRateResponse = document.getElementById('gpsRateResponse');
  if (gpsRateSelect) {
    const validRates = [1, 5, 10, 16];
    gpsRateSelect.innerHTML = '';
    validRates.forEach(rate => {
      const opt = document.createElement('option');
      opt.value = rate;
      opt.textContent = rate + ' Hz';
      gpsRateSelect.appendChild(opt);
    });
  }
  if (setGpsRateBtn && gpsRateSelect && gpsRateResponse) {
    setGpsRateBtn.addEventListener('click', async () => {
      const rate = parseInt(gpsRateSelect.value, 10);
      gpsRateResponse.textContent = 'Sending...';
      window.gpsRateUserMessageUntil = Date.now() + 4000;
      try {
        const resp = await fetch('/api/gpsRate', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ rate })
        });
        const data = await resp.json();
        gpsRateResponse.textContent = data.message || (data.success ? 'Success' : 'Failed');
        gpsRateResponse.style.color = data.success ? '#007a3d' : '#b00020';
        window.gpsRateUserMessageUntil = Date.now() + 4000;
      } catch (e) {
        gpsRateResponse.textContent = 'Error sending command.';
        gpsRateResponse.style.color = '#b00020';
        window.gpsRateUserMessageUntil = Date.now() + 4000;
      }
    });
  }

  // Dashboard controls
  const testBtn = document.getElementById('testNeedleSweep');
  if (testBtn) {
    testBtn.addEventListener('click', () => pushAction('needleSweep'));
  }

  // Test Shift Light button
  const testShiftBtn = document.getElementById('testShiftLight');
  if (testShiftBtn) {
    testShiftBtn.addEventListener('click', () => pushAction('testShiftLight'));
  }

  const otaUploadBtn = document.getElementById('otaUploadBtn');
  if (otaUploadBtn) {
    otaUploadBtn.addEventListener('click', uploadFirmware);
  }

  const otaFsUploadBtn = document.getElementById('otaFsUploadBtn');
  if (otaFsUploadBtn) {
    otaFsUploadBtn.addEventListener('click', uploadFilesystem);
  }

  // Configuration controls
  const configInputs = ['hasNeedleSweep', 'sweepSpeed', 'stepRPM', 'stepSpeed', 'shiftLight', 'shiftLimit', 'shiftFlashes', 'coilType', 'useMPH', 'dsgParkMode'];
  configInputs.forEach(id => {
    const el = document.getElementById(id);
    if (el) {
      el.addEventListener('change', () => {
        const value = el.type === 'checkbox' ? el.checked : el.value;
        pushControl(id, value);
      });
    }
  });

  // Advanced test controls
  const advancedInputs = [
    'testRPM', 'tempRPM', 'testSpeedo', 'tempSpeed',
    'broadcastSpeedEnabled', 'broadcastSpeedID', 'broadcastSpeedDLC',
    'broadcastSpeedLowByte', 'broadcastSpeedHighByte', 'broadcastSpeedLittleEndian',
    'broadcastSpeedScale', 'broadcastSpeedOffset',
    'broadcastSpeedData0', 'broadcastSpeedData1', 'broadcastSpeedData2', 'broadcastSpeedData3',
    'broadcastSpeedData4', 'broadcastSpeedData5', 'broadcastSpeedData6', 'broadcastSpeedData7',
    'aftermarketSpeedID', 'aftermarketSpeedLowByte', 'aftermarketSpeedHighByte',
    'aftermarketSpeedLittleEndian', 'aftermarketSpeedScale', 'aftermarketSpeedOffset',
    'testReverse', 'testEML', 'testEPC', 'diagTest'
  ];
  advancedInputs.forEach(id => {
    const el = document.getElementById(id);
    if (el) {
      el.addEventListener('change', () => {
        let value;
        if (el.type === 'checkbox') {
          value = el.checked;
        } else if (el.type === 'number' || el.type === 'range') {
          value = Number(el.value);
        } else if (id === 'broadcastSpeedLittleEndian' || id === 'aftermarketSpeedLittleEndian') {
          value = el.value === 'true';
        } else if (id === 'broadcastSpeedID' || id === 'aftermarketSpeedID') {
          value = el.value.trim();
        } else {
          value = el.value;
        }

        pushControl(id, value);
        
        // Update highlighting when test states change
        if (id === 'testRPM') {
          const displayEl = document.getElementById('tempRPM-display');
          if (displayEl) {
              displayEl.style.color = value ? 'orange' : '';
          }
        }
        if (id === 'testSpeedo') {
          const displayEl = document.getElementById('tempSpeed-display');
          if (displayEl) {
              displayEl.style.color = value ? 'orange' : '';
          }
        }
      });
      // For sliders, also update live display
      if (el.type === 'range') {
        el.addEventListener('input', () => {
          const displayId = id + '-display';
          const displayEl = document.getElementById(displayId);
          if (displayEl) {
            displayEl.textContent = el.value;
          }
        });
      }
    }
  });

  // Speed source dropdown
  const speedSourceEl = document.getElementById('speedSource');
  function updateCustomCANVisibility() {
    const card = document.getElementById('customCANInputCard');
    if (card && speedSourceEl) {
      card.style.display = speedSourceEl.value === 'Custom CAN' ? '' : 'none';
    }
  }
  if (speedSourceEl) {
    speedSourceEl.addEventListener('change', () => {
      pushControl('speedType', speedSourceEl.value);
      updateCustomCANVisibility();
    });
  }

  // RPM source dropdown
  const rpmSourceEl = document.getElementById('rpmSource');
  if (rpmSourceEl) {
    rpmSourceEl.addEventListener('change', () => {
      pushControl('rpmType', rpmSourceEl.value);
    });
  }

  // Cluster mapping limit sliders
  const clusterFrequencyLimitEl = document.getElementById('clusterFrequencyLimit');
  if (clusterFrequencyLimitEl) {
    clusterFrequencyLimitEl.addEventListener('change', () => {
      pushControl('clusterFrequencyLimit', Number(clusterFrequencyLimitEl.value));
    });
    clusterFrequencyLimitEl.addEventListener('input', () => {
      const displayEl = document.getElementById('clusterFrequencyLimit-display');
      if (displayEl) displayEl.textContent = clusterFrequencyLimitEl.value;
    });
  }

  const clusterRPMLimitEl = document.getElementById('clusterRPMLimit');
  if (clusterRPMLimitEl) {
    clusterRPMLimitEl.addEventListener('change', () => {
      pushControl('clusterRPMLimit', Number(clusterRPMLimitEl.value));
    });
    clusterRPMLimitEl.addEventListener('input', () => {
      const displayEl = document.getElementById('clusterRPMLimit-display');
      if (displayEl) displayEl.textContent = clusterRPMLimitEl.value;
    });
  }

  const resetClusterFrequencyLimitBtn = document.getElementById('resetClusterFrequencyLimit');
  if (resetClusterFrequencyLimitBtn && clusterFrequencyLimitEl) {
    resetClusterFrequencyLimitBtn.addEventListener('click', () => {
      clusterFrequencyLimitEl.value = 230;
      const displayEl = document.getElementById('clusterFrequencyLimit-display');
      if (displayEl) displayEl.textContent = '230';
      pushControl('clusterFrequencyLimit', 230);
    });
  }

  const resetClusterRPMLimitBtn = document.getElementById('resetClusterRPMLimit');
  if (resetClusterRPMLimitBtn && clusterRPMLimitEl) {
    resetClusterRPMLimitBtn.addEventListener('click', () => {
      clusterRPMLimitEl.value = 7000;
      const displayEl = document.getElementById('clusterRPMLimit-display');
      if (displayEl) displayEl.textContent = '7000';
      pushControl('clusterRPMLimit', 7000);
    });
  }

  const maxFreqHallEl = document.getElementById('maxFreqHall');
  if (maxFreqHallEl) {
    maxFreqHallEl.addEventListener('change', () => {
      pushControl('maxFreqHall', Number(maxFreqHallEl.value));
    });
    maxFreqHallEl.addEventListener('input', () => {
      const displayEl = document.getElementById('maxFreqHall-display');
      if (displayEl) displayEl.textContent = maxFreqHallEl.value;
    });
  }

  const resetMaxFreqHallBtn = document.getElementById('resetMaxFreqHall');
  if (resetMaxFreqHallBtn && maxFreqHallEl) {
    resetMaxFreqHallBtn.addEventListener('click', () => {
      maxFreqHallEl.value = 200;
      const displayEl = document.getElementById('maxFreqHall-display');
      if (displayEl) displayEl.textContent = '200';
      pushControl('maxFreqHall', 200);
    });
  }

  // CAN Analyzer - SavvyCAN: WiFi and Serial are mutually exclusive
  const analyzerModeEl = document.getElementById('analyzerMode');
  const analyzerSerialEl = document.getElementById('analyzerSerial');
  if (analyzerModeEl && analyzerSerialEl) {
    analyzerModeEl.addEventListener('change', () => {
      if (analyzerModeEl.checked) {
        analyzerSerialEl.checked = false;
        pushControl('analyzerSerial', false);
      }
      pushControl('analyzerMode', analyzerModeEl.checked);
    });
    analyzerSerialEl.addEventListener('change', () => {
      if (analyzerSerialEl.checked) {
        analyzerModeEl.checked = false;
        pushControl('analyzerMode', false);
      }
      pushControl('analyzerSerial', analyzerSerialEl.checked);
    });
  }
}

async function fetchSettings() {
  try {
    const response = await fetch('/api/settings');
    const data = await response.json();

    // Load all settings from API once
    document.getElementById('hasNeedleSweep').checked = data.hasNeedleSweep || false;
    document.getElementById('sweepSpeed').value = data.sweepSpeed || 0;
    document.getElementById('stepRPM').value = data.stepRPM || 100;
    document.getElementById('stepSpeed').value = data.stepSpeed || 100;
    document.getElementById('shiftLight').value = data.shiftLight || 'None';
    document.getElementById('shiftLimit').value = data.shiftLimit || 0;
    document.getElementById('shiftFlashes').value = data.shiftFlashes || 0;
    document.getElementById('coilType').checked = data.coilType || false;
    const useMPHEl = document.getElementById('useMPH');
    if (useMPHEl) useMPHEl.checked = data.useMPH || false;
    applySpeedUnitLabels(data.useMPH);
    document.getElementById('dsgParkMode').value = data.dsgParkMode || 'None';

    // Advanced controls
    document.getElementById('testRPM').checked = data.testRPM || false;
    document.getElementById('tempRPM').value = data.tempRPM || 0;
    const tempRPMDisplay = document.getElementById('tempRPM-display');
    tempRPMDisplay.textContent = data.tempRPM || 0;
    tempRPMDisplay.style.color = data.testRPM ? 'orange' : '';

    document.getElementById('testSpeedo').checked = data.testSpeedo || false;
    document.getElementById('tempSpeed').value = data.tempSpeed || 0;
    const tempSpeedDisplay = document.getElementById('tempSpeed-display');
    tempSpeedDisplay.textContent = data.tempSpeed || 0;
    tempSpeedDisplay.style.color = data.testSpeedo ? 'orange' : '';

    document.getElementById('broadcastSpeedEnabled').checked = data.broadcastSpeedEnabled || false;
    document.getElementById('broadcastSpeedID').value = (data.broadcastSpeedID || 0).toString(16).toUpperCase();
    document.getElementById('broadcastSpeedDLC').value = data.broadcastSpeedDLC ?? 8;
    document.getElementById('broadcastSpeedLowByte').value = data.broadcastSpeedLowByte ?? 3;
    document.getElementById('broadcastSpeedHighByte').value = data.broadcastSpeedHighByte ?? 2;
    document.getElementById('broadcastSpeedLittleEndian').value = (data.broadcastSpeedLittleEndian ? 'true' : 'false');
    document.getElementById('broadcastSpeedScale').value = (data.broadcastSpeedScale ?? 1.0).toFixed(3);
    document.getElementById('broadcastSpeedOffset').value = data.broadcastSpeedOffset ?? 0;
    for (let i = 0; i < 8; i += 1) {
      const dataEl = document.getElementById(`broadcastSpeedData${i}`);
      if (dataEl) {
        dataEl.value = data[`broadcastSpeedData${i}`] ?? 0;
      }
    }

    // Aftermarket / Custom CAN input settings
    document.getElementById('aftermarketSpeedID').value = (data.aftermarketSpeedID || 0).toString(16).toUpperCase();
    document.getElementById('aftermarketSpeedLowByte').value = data.aftermarketSpeedLowByte ?? 0;
    document.getElementById('aftermarketSpeedHighByte').value = data.aftermarketSpeedHighByte ?? 1;
    document.getElementById('aftermarketSpeedLittleEndian').value = (data.aftermarketSpeedLittleEndian ? 'true' : 'false');
    document.getElementById('aftermarketSpeedScale').value = (data.aftermarketSpeedScale ?? 1.0).toFixed(3);
    document.getElementById('aftermarketSpeedOffset').value = data.aftermarketSpeedOffset ?? 0;

    // Test outputs
    document.getElementById('testReverse').checked = data.testReverse || false;
    document.getElementById('testEML').checked = data.testEML || false;
    document.getElementById('testEPC').checked = data.testEPC || false;
    const diagTestEl = document.getElementById('diagTest');
    if (diagTestEl) diagTestEl.checked = data.diagTest || false;
    const analyzerModeEl = document.getElementById('analyzerMode');
    const analyzerSerialEl = document.getElementById('analyzerSerial');
    // Enforce mutual exclusion: WiFi takes priority if somehow both are true
    const wifiOn = !!(data.analyzerMode);
    const serialOn = !!(data.analyzerSerial) && !wifiOn;
    if (analyzerModeEl) analyzerModeEl.checked = wifiOn;
    if (analyzerSerialEl) analyzerSerialEl.checked = serialOn;

    // Speed type dropdown - map speedType to dropdown options
    let speedTypeValue = 'Hall';  // default
    if (data.speedType === 'ECU') speedTypeValue = 'ECU';
    else if (data.speedType === 'ABS') speedTypeValue = 'ABS';
    else if (data.speedType === 'DSG') speedTypeValue = 'DSG';
    else if (data.speedType === 'TP2.0-DSG' || data.speedType === 'TP/UDS DSG') speedTypeValue = 'TP2.0-DSG';
    else if (data.speedType === 'GPS') speedTypeValue = 'GPS';
    else if (data.speedType === 'Custom CAN') speedTypeValue = 'Custom CAN';
    document.getElementById('speedSource').value = speedTypeValue;
    const customCANCard = document.getElementById('customCANInputCard');
    if (customCANCard) customCANCard.style.display = speedTypeValue === 'Custom CAN' ? '' : 'none';

    const rpmTypeValue = data.rpmType === 'Hall' ? 'Hall' : 'CAN';
    document.getElementById('rpmSource').value = rpmTypeValue;

    const clusterFrequencyLimitValue = data.clusterFrequencyLimit || 230;
    document.getElementById('clusterFrequencyLimit').value = clusterFrequencyLimitValue;
    document.getElementById('clusterFrequencyLimit-display').textContent = clusterFrequencyLimitValue;

    const clusterRPMLimitValue = data.clusterRPMLimit || 7000;
    document.getElementById('clusterRPMLimit').value = clusterRPMLimitValue;
    document.getElementById('clusterRPMLimit-display').textContent = clusterRPMLimitValue;

    const maxFreqHallValue = data.maxFreqHall || 200;
    const maxFreqHallEl2 = document.getElementById('maxFreqHall');
    if (maxFreqHallEl2) {
      maxFreqHallEl2.value = maxFreqHallValue;
      document.getElementById('maxFreqHall-display').textContent = maxFreqHallValue;
    }

    if (document.getElementById('gpsRateSelect')) {
      const savedGpsRate = String(data.gpsUpdateRateHz ?? 1);
      document.getElementById('gpsRateSelect').value = savedGpsRate;
    }

    // Update FW version
    const fwResponse = await fetch('/api/settings');
    const fwData = await fwResponse.json();
    const fwStr = 'FW: ' + (fwData.FW_VERSION || '--');
    document.getElementById('fwVersion').textContent = fwStr;
    if (document.getElementById('fwVersionOta')) {
      document.getElementById('fwVersionOta').textContent = fwStr;
    }

    settingsLoaded = true;
  } catch (error) {
    console.log('Error fetching settings:', error);
  }
}

async function fetchStatus() {
  try {
    const response = await fetch('/api/status');
    const data = await response.json();

    const testRPMActive = data.testRPM || false;
    const testSpeedActive = data.testSpeedo || false;
    const testEMLActive = data.testEML || false;
    const testEPCActive = data.testEPC || false;
    const testReverseActive = data.testReverse || false;

    // Update dashboard live data with highlighting for tested values
    applySpeedUnitLabels(data.useMPH);
    const speedEl = document.getElementById('speed');
    speedEl.textContent = data.vehicleSpeed || '--';
    speedEl.style.color = testSpeedActive ? 'orange' : '';
    speedEl.title = testSpeedActive ? 'Test Mode: ' + (data.tempSpeed || 0) + (data.useMPH ? ' MPH' : ' KMH') : '';
    const rpmEl = document.getElementById('rpm');
    rpmEl.textContent = data.vehicleRPM || '--';
    rpmEl.style.color = testRPMActive ? 'orange' : '';
    rpmEl.title = testRPMActive ? 'Test Mode: ' + (data.tempRPM || 0) + ' RPM' : '';

    // Update advanced live data - all speed sources
    if (document.getElementById('liveRPM')) {
      document.getElementById('liveRPM').textContent = data.vehicleRPM || '--';
    }
    if (document.getElementById('liveHallRPM')) {
      document.getElementById('liveHallRPM').textContent = data.hallRPM || '--';
    }
    if (document.getElementById('liveCANRPM')) {
      document.getElementById('liveCANRPM').textContent = data.canRPM || '--';
    }
    if (document.getElementById('liveHallSpeed')) {
      document.getElementById('liveHallSpeed').textContent = data.hallSpeed || '--';
    }
    if (document.getElementById('liveECUSpeed')) {
      document.getElementById('liveECUSpeed').textContent = data.ecuSpeed || '--';
    }
    if (document.getElementById('liveABSSpeed')) {
      document.getElementById('liveABSSpeed').textContent = data.absSpeed || '--';
    }
    if (document.getElementById('liveDSGSpeed')) {
      document.getElementById('liveDSGSpeed').textContent = data.dsgSpeed || '--';
    }
    if (document.getElementById('liveUDSSpeed')) {
      document.getElementById('liveUDSSpeed').textContent = data.udsSpeed || '--';
    }
    if (document.getElementById('liveGPSSpeed')) {
      document.getElementById('liveGPSSpeed').textContent = data.gpsSpeed || '--';
    }
    if (document.getElementById('liveAftermarketSpeed')) {
      document.getElementById('liveAftermarketSpeed').textContent = data.aftermarketSpeed !== undefined ? Number(data.aftermarketSpeed).toFixed(1) : '--';
    }
    if (document.getElementById('liveAftermarketSpeedCard')) {
      document.getElementById('liveAftermarketSpeedCard').textContent = data.aftermarketSpeed !== undefined ? Number(data.aftermarketSpeed).toFixed(1) : '--';
    }
    if (document.getElementById('liveGPSStatus')) {
      if (data.hasGPS) {
        document.getElementById('liveGPSStatus').textContent = `Connected, ${data.gpsSatellites} satellites`;
      } else if (data.gpsUnavailable) {
        document.getElementById('liveGPSStatus').textContent = 'Unavailable';
      } else {
        document.getElementById('liveGPSStatus').textContent = 'Not Connected';
      }
    }
    if (document.getElementById('liveGPSFrequency')) {
      const rawFreq = data.gpsFrequency;
      const freq = (typeof rawFreq === 'number') ? rawFreq : Number(rawFreq);
      document.getElementById('liveGPSFrequency').textContent = Number.isFinite(freq) ? freq.toFixed(2) : '--';
    }

    // GPS auto rate countdown - show in the same green response area used by
    // the Set button, but only when the user isn't actively reading their
    // own click feedback.
    const gpsRateResponseEl = document.getElementById('gpsRateResponse');
    if (gpsRateResponseEl && (!window.gpsRateUserMessageUntil || Date.now() > window.gpsRateUserMessageUntil)) {
      const secs = data.gpsAutoApplySecs;
      if (typeof secs === 'number' && secs >= 0) {
        gpsRateResponseEl.style.color = '#007a3d';
        if (secs === 0) {
          gpsRateResponseEl.textContent = 'Auto-applying saved rate...';
        } else {
          gpsRateResponseEl.textContent = `Auto-apply in ${secs}s (waiting for satellite lock + 20s).`;
        }
      } else if (gpsRateResponseEl.textContent.startsWith('Auto-apply') || gpsRateResponseEl.textContent.startsWith('Auto-applying')) {
        gpsRateResponseEl.textContent = '';
      }
    }

    if (document.getElementById('liveBroadcastSpeedValue')) {
      const suffix = data.broadcastSpeedEnabled ? '' : ' (disabled)';
      document.getElementById('liveBroadcastSpeedValue').textContent = `${data.broadcastSpeedValue || 0}${suffix}`;
    }

    // System status (read-only, not settings)
    document.getElementById('canStatus').textContent = data.hasCAN ? 'CAN: Healthy' : 'CAN: Not Healthy';
    document.getElementById('canPresent').textContent = data.hasCAN ? 'Healthy' : 'Not Healthy';
    if (document.getElementById('gpsPresent')) {
      if (data.hasGPS) {
        document.getElementById('gpsPresent').textContent = `Connected (${data.gpsSatellites} sat)`;
      } else if (data.gpsUnavailable) {
        document.getElementById('gpsPresent').textContent = 'Unavailable';
      } else {
        document.getElementById('gpsPresent').textContent = 'Not Connected';
      }
    }
    
    // Output status with highlighting for tests
    const emlEl = document.getElementById('emlStatus');
    const emlEffectiveActive = !!data.vehicleEML || testEMLActive;
    emlEl.textContent = emlEffectiveActive ? 'Active' : 'Inactive';
    emlEl.style.color = testEMLActive ? 'orange' : '';
    emlEl.title = testEMLActive ? 'Test Mode: Forced ON' : '';

    const epcEl = document.getElementById('epcStatus');
    const epcEffectiveActive = !!data.vehicleEPC || testEPCActive;
    epcEl.textContent = epcEffectiveActive ? 'Active' : 'Inactive';
    epcEl.style.color = testEPCActive ? 'orange' : '';
    epcEl.title = testEPCActive ? 'Test Mode: Forced ON' : '';

    const reverseEl = document.getElementById('reverseStatus');
    const reverseEffectiveActive = !!data.vehicleReverse || testReverseActive;
    reverseEl.textContent = reverseEffectiveActive ? 'Active' : 'Inactive';
    reverseEl.style.color = testReverseActive ? 'orange' : '';
    reverseEl.title = testReverseActive ? 'Test Mode: Forced ON' : '';

    document.getElementById('parkStatus').textContent = data.vehiclePark ? 'Active' : 'Inactive';
    
    // Paddle feedback
    document.getElementById('paddleUpStatus').textContent = data.paddleUp ? 'Active' : 'Inactive';
    document.getElementById('paddleDownStatus').textContent = data.paddleDown ? 'Active' : 'Inactive';

  } catch (error) {
    console.log('Error fetching status:', error);
  }
}

function applySpeedUnitLabels(useMPH) {
  const label = useMPH ? 'MPH' : 'KMH';
  const speedUnitEl = document.getElementById('speedUnit');
  if (speedUnitEl) speedUnitEl.textContent = label;
  const tempSpeedUnitEl = document.getElementById('tempSpeed-unit');
  if (tempSpeedUnitEl) tempSpeedUnitEl.textContent = label;
}

function pushControl(key, value) {
  fetch('/api/control', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ key, value })
  }).catch(e => console.log('Control error:', e));
}

function pushAction(action) {
  fetch('/api/action', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ action })
  }).catch(e => console.log('Action error:', e));
}

function hex2bin(hex) {
  return ("00000000" + parseInt(hex, 16).toString(2)).substr(-8);
}

function showNotification(message, type = "success") {
  const notification = document.createElement("div");
  notification.textContent = message;
  notification.style.cssText = `
        position: fixed;
        top: 20px;
        left: 50%;
        transform: translateX(-50%);
        padding: 1rem 2rem;
        background: ${type === "error" ? "var(--danger)" : "var(--success)"};
        color: white;
        border-radius: 8px;
        z-index: 10000;
        font-weight: 600;
        box-shadow: 0 4px 12px rgba(0,0,0,0.3);
    `;

  document.body.appendChild(notification);

  setTimeout(() => {
    notification.style.transition = "opacity 0.3s";
    notification.style.opacity = "0";
    setTimeout(() => notification.remove(), 300);
  }, 3000);
}

async function uploadFirmware() {
  const fileInput = document.getElementById('otaBinFile');
  const statusEl = document.getElementById('otaStatus');
  const progressEl = document.getElementById('otaProgress');
  const uploadBtn = document.getElementById('otaUploadBtn');

  if (!fileInput || !fileInput.files || fileInput.files.length === 0) {
    if (statusEl) statusEl.textContent = 'Please select a .bin file first';
    return;
  }

  const file = fileInput.files[0];
  const formData = new FormData();
  formData.append('firmware', file, file.name);

  return new Promise((resolve) => {
    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/ota');

    if (uploadBtn) uploadBtn.disabled = true;
    if (progressEl) { progressEl.style.display = 'block'; progressEl.value = 0; }

    xhr.upload.addEventListener('progress', (e) => {
      if (e.lengthComputable) {
        const pct = Math.round((e.loaded / e.total) * 100);
        if (statusEl) statusEl.textContent = `Uploading... ${pct}%`;
        if (progressEl) progressEl.value = pct;
      }
    });

    xhr.addEventListener('load', () => {
      if (xhr.status === 200) {
        if (statusEl) statusEl.textContent = 'Upload complete. Device rebooting...';
        if (progressEl) progressEl.value = 100;
      } else {
        if (statusEl) statusEl.textContent = 'Upload failed. Please try again.';
        if (progressEl) progressEl.style.display = 'none';
        if (uploadBtn) uploadBtn.disabled = false;
      }
      resolve();
    });

    xhr.addEventListener('error', () => {
      if (statusEl) statusEl.textContent = 'Upload failed. Check connection and retry.';
      if (progressEl) progressEl.style.display = 'none';
      if (uploadBtn) uploadBtn.disabled = false;
      resolve();
    });

    xhr.send(formData);
  });
}

async function uploadFilesystem() {
  const fileInput = document.getElementById('otaFsBinFile');
  const statusEl = document.getElementById('otaFsStatus');
  const progressEl = document.getElementById('otaFsProgress');
  const uploadBtn = document.getElementById('otaFsUploadBtn');

  if (!fileInput || !fileInput.files || fileInput.files.length === 0) {
    if (statusEl) statusEl.textContent = 'Please select a .bin file first';
    return;
  }

  const file = fileInput.files[0];
  const formData = new FormData();
  formData.append('filesystem', file, file.name);

  return new Promise((resolve) => {
    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/ota/fs');

    if (uploadBtn) uploadBtn.disabled = true;
    if (progressEl) { progressEl.style.display = 'block'; progressEl.value = 0; }

    xhr.upload.addEventListener('progress', (e) => {
      if (e.lengthComputable) {
        const pct = Math.round((e.loaded / e.total) * 100);
        if (statusEl) statusEl.textContent = `Uploading... ${pct}%`;
        if (progressEl) progressEl.value = pct;
      }
    });

    xhr.addEventListener('load', () => {
      if (xhr.status === 200) {
        if (statusEl) statusEl.textContent = 'Upload complete. Device rebooting...';
        if (progressEl) progressEl.value = 100;
      } else {
        if (statusEl) statusEl.textContent = 'Upload failed. Please try again.';
        if (progressEl) progressEl.style.display = 'none';
        if (uploadBtn) uploadBtn.disabled = false;
      }
      resolve();
    });

    xhr.addEventListener('error', () => {
      if (statusEl) statusEl.textContent = 'Upload failed. Check connection and retry.';
      if (progressEl) progressEl.style.display = 'none';
      if (uploadBtn) uploadBtn.disabled = false;
      resolve();
    });

    xhr.send(formData);
  });
}

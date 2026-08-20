document.addEventListener('DOMContentLoaded', initApp);

let settingsLoaded = false;

// Tab navigation
function initApp() {
  initNavigation();
  initControls();
  initCollapsibleCards();
  initCoolant();
  initOutputConflicts();
  initOta();
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

      if (page === "advanced") {
        drawCoolantCurve();
      }
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

  const otaFsUploadBtn = document.getElementById('otaFsUploadBtn');

  // Configuration controls
  const configInputs = ['hasNeedleSweep', 'sweepSpeed', 'stepRPM', 'stepSpeed', 'shiftLimit', 'shiftFlashes', 'coilType', 'useMPH'];
  configInputs.forEach(id => {
    const el = document.getElementById(id);
    if (el) {
      el.addEventListener('change', () => {
        const value = el.type === 'checkbox' ? el.checked : el.value;
        pushControl(id, value);
      });
      if (el.type === 'range') {
        el.addEventListener('input', () => {
          const displayEl = document.getElementById(id + '-display');
          if (displayEl) displayEl.textContent = el.value;
        });
      }
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
    'dsgRatio1', 'dsgRatio2', 'dsgRatio3', 'dsgRatio4', 'dsgRatio5', 'dsgRatio6',
    'dsgFinal14', 'dsgFinal56', 'dsgTireCirc',
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
    const dsgCard = document.getElementById('dsgCalcCard');
    if (dsgCard && speedSourceEl) {
      dsgCard.style.display = speedSourceEl.value === 'DSG' ? '' : 'none';
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

  const maxFreqVREl = document.getElementById('maxFreqVR');
  if (maxFreqVREl) {
    maxFreqVREl.addEventListener('change', () => {
      pushControl('maxFreqVR', Number(maxFreqVREl.value));
    });
    maxFreqVREl.addEventListener('input', () => {
      const displayEl = document.getElementById('maxFreqVR-display');
      if (displayEl) displayEl.textContent = maxFreqVREl.value;
    });
  }

  const resetMaxFreqVRBtn = document.getElementById('resetMaxFreqVR');
  if (resetMaxFreqVRBtn && maxFreqVREl) {
    resetMaxFreqVRBtn.addEventListener('click', () => {
      maxFreqVREl.value = 200;
      const displayEl = document.getElementById('maxFreqVR-display');
      if (displayEl) displayEl.textContent = '200';
      pushControl('maxFreqVR', 200);
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
    setText('sweepSpeed-display', data.sweepSpeed || 0);
    document.getElementById('stepRPM').value = data.stepRPM || 100;
    setText('stepRPM-display', data.stepRPM || 100);
    document.getElementById('stepSpeed').value = data.stepSpeed || 100;
    setText('stepSpeed-display', data.stepSpeed || 100);
    document.getElementById('shiftLight').value = data.shiftLight || 'None';
    document.getElementById('shiftLimit').value = data.shiftLimit || 0;
    document.getElementById('shiftFlashes').value = data.shiftFlashes || 0;
    document.getElementById('coilType').checked = data.coilType || false;
    const useMPHEl = document.getElementById('useMPH');
    if (useMPHEl) useMPHEl.checked = data.useMPH || false;
    applySpeedUnitLabels(data.useMPH);
    document.getElementById('dsgParkMode').value = data.dsgParkMode || 'None';

    // Coolant gauge output
    const coolantOutputEl = document.getElementById('coolantOutput');
    if (coolantOutputEl) coolantOutputEl.value = data.coolantOutput || 'Off';
    const coolantWarnEl = document.getElementById('coolantWarnTemp');
    if (coolantWarnEl) {
      const wt = (data.coolantWarnTemp !== undefined) ? data.coolantWarnTemp : 120;
      coolantWarnEl.value = wt;
      const coolantWarnDisp = document.getElementById('coolantWarnTemp-display');
      if (coolantWarnDisp) coolantWarnDisp.textContent = wt;
    }

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

    // DSG speed calculation settings
    const dsgDefaults = { dsgRatio1: 3.462, dsgRatio2: 2.050, dsgRatio3: 1.300, dsgRatio4: 0.902, dsgRatio5: 0.914, dsgRatio6: 0.756, dsgFinal14: 4.118, dsgFinal56: 3.043, dsgTireCirc: 1.885 };
    Object.keys(dsgDefaults).forEach(id => {
      const el = document.getElementById(id);
      if (el) el.value = Number(data[id] ?? dsgDefaults[id]).toFixed(3);
    });

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
    if (data.speedType === 'VR') speedTypeValue = 'VR';
    else if (data.speedType === 'ECU') speedTypeValue = 'ECU';
    else if (data.speedType === 'ABS') speedTypeValue = 'ABS';
    else if (data.speedType === 'DSG') speedTypeValue = 'DSG';
    else if (data.speedType === 'TP2.0-DSG' || data.speedType === 'TP/UDS DSG') speedTypeValue = 'TP2.0-DSG';
    else if (data.speedType === 'GPS') speedTypeValue = 'GPS';
    else if (data.speedType === 'Custom CAN') speedTypeValue = 'Custom CAN';
    document.getElementById('speedSource').value = speedTypeValue;
    const customCANCard = document.getElementById('customCANInputCard');
    if (customCANCard) customCANCard.style.display = speedTypeValue === 'Custom CAN' ? '' : 'none';
    const dsgCalcCard = document.getElementById('dsgCalcCard');
    if (dsgCalcCard) dsgCalcCard.style.display = speedTypeValue === 'DSG' ? '' : 'none';

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

    const maxFreqVRValue = data.maxFreqVR || 200;
    const maxFreqVREl2 = document.getElementById('maxFreqVR');
    if (maxFreqVREl2) {
      maxFreqVREl2.value = maxFreqVRValue;
      document.getElementById('maxFreqVR-display').textContent = maxFreqVRValue;
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
    syncLastOutputValues();
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
    if (document.getElementById('liveVRSpeed')) {
      document.getElementById('liveVRSpeed').textContent = data.vrSpeed || '--';
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
    if (document.getElementById('liveDSGSpeedCard')) {
      document.getElementById('liveDSGSpeedCard').textContent = data.dsgSpeed !== undefined ? Number(data.dsgSpeed).toFixed(1) : '--';
    }
    if (document.getElementById('liveTP20Speed')) {
      document.getElementById('liveTP20Speed').textContent = data.tp20Speed || '--';
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
        document.getElementById('liveGPSStatus').textContent = `Connected (${data.gpsSatellites} sats)`;
      } else if (data.gpsUnavailable) {
        document.getElementById('liveGPSStatus').textContent = 'Not Available';
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
        document.getElementById('gpsPresent').textContent = `Connected (${data.gpsSatellites} sats)`;
      } else if (data.gpsUnavailable) {
        document.getElementById('gpsPresent').textContent = 'Not Available';
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

    // Coolant gauge live data
    if (document.getElementById('liveCoolantTemp')) {
      document.getElementById('liveCoolantTemp').textContent =
        (data.vehicleCoolantTemp !== undefined) ? data.vehicleCoolantTemp + ' \u00B0C' : '--';
    }
    if (document.getElementById('liveCoolantDuty')) {
      document.getElementById('liveCoolantDuty').textContent =
        (data.coolantDuty !== undefined) ? data.coolantDuty : '--';
    }
    if (data.vehicleCoolantTemp !== undefined) coolantState.temp = data.vehicleCoolantTemp;
    if (data.coolantDuty !== undefined) coolantState.appliedDuty = data.coolantDuty;
    if (typeof data.coolantCalMode === 'boolean') coolantState.calMode = data.coolantCalMode;
    const coolantPageEl = document.getElementById('advanced-page');
    if (coolantPageEl && coolantPageEl.classList.contains('active')) {
      drawCoolantCurve();
    }

    // Board & I2C diagnostics
    if (data.i2c) {
      const i2c = data.i2c;
      const setTxt = (id, val) => { const el = document.getElementById(id); if (el) el.textContent = val; };
      setTxt('liveBoard', i2c.newBoard ? 'New (I2C)' : 'Old (GPIO)');
      if (i2c.newBoard) {
        setTxt('liveMcp4725', i2c.mcp4725 ? 'Detected' : 'Missing');
        setTxt('liveTca9554', i2c.tca9554 ? 'Detected' : 'Missing');
        setTxt('liveCoolantDac', i2c.coolantDac);
        setTxt('liveTcaInputs', '0x' + (i2c.tcaInputs || 0).toString(16).padStart(2, '0').toUpperCase());
        setTxt('liveTcaInt', i2c.tcaIntCount);
      } else {
        setTxt('liveMcp4725', 'n/a');
        setTxt('liveTca9554', 'n/a');
        setTxt('liveCoolantDac', 'n/a');
        setTxt('liveTcaInputs', 'n/a');
        setTxt('liveTcaInt', 'n/a');
      }
    }

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
  return fetch('/api/control', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ key, value })
  }).catch(e => console.log('Control error:', e));
}

// ---- Collapsible cards (SpeedPulser-style) ----
function initCollapsibleCards() {
  ['configuration-page', 'advanced-page', 'diag-page'].forEach(pageId => {
    const page = document.getElementById(pageId);
    if (!page) return;
    page.querySelectorAll('.card').forEach(card => {
      if (card.classList.contains('no-collapse')) return;
      card.classList.add('collapsible', 'collapsed');
      const h2 = card.querySelector('h2');
      if (h2) {
        h2.addEventListener('click', () => card.classList.toggle('collapsed'));
      }
    });
  });
}

// ---- Coolant gauge calibration builder ----
let coolantState = { duty: 0, maxDuty: 1023, calMode: false, points: [], temp: 0, appliedDuty: 0 };
let coolantSelectedTemp = 90;

function initCoolant() {
  const warnEl = document.getElementById('coolantWarnTemp');
  if (warnEl) {
    warnEl.addEventListener('input', () => {
      const d = document.getElementById('coolantWarnTemp-display');
      if (d) d.textContent = warnEl.value;
    });
    warnEl.addEventListener('change', () => pushControl('coolantWarnTemp', Number(warnEl.value)));
  }

  const calModeEl = document.getElementById('coolantCalMode');
  if (calModeEl) {
    calModeEl.addEventListener('change', () => calPost({ op: calModeEl.checked ? 'enter' : 'exit' }));
  }

  // Jog steppers with press-and-hold repeat + acceleration
  document.querySelectorAll('.stepper-btn[data-jog]').forEach(btn => {
    const delta = Number(btn.dataset.jog);
    attachHold(btn, () => calPost({ op: 'jog', delta }));
  });

  const tempEl = document.getElementById('coolantTargetTemp');
  if (tempEl) {
    tempEl.addEventListener('input', () => setCoolantTarget(Number(tempEl.value) || 0, false));
  }

  const capBtn = document.getElementById('coolantCaptureBtn');
  if (capBtn) {
    capBtn.addEventListener('click', () => {
      calPost({ op: 'addPoint', temp: coolantSelectedTemp }).then(() => {
        showNotification('Captured ' + coolantSelectedTemp + ' \u00B0C @ duty ' + (coolantState.duty || 0));
      });
    });
  }

  const clrBtn = document.getElementById('coolantClearBtn');
  if (clrBtn) {
    clrBtn.addEventListener('click', () => {
      if (confirm('Clear all calibration points?')) calPost({ op: 'clearPoints' });
    });
  }

  const chipWrap = document.getElementById('coolantTargetChips');
  if (chipWrap) {
    [40, 60, 80, 90, 100, 110].forEach(v => {
      const chip = document.createElement('button');
      chip.className = 'chip';
      chip.dataset.temp = v;
      chip.textContent = v + ' \u00B0C';
      chip.addEventListener('click', () => setCoolantTarget(v, true));
      chipWrap.appendChild(chip);
    });
  }
  setCoolantTarget(coolantSelectedTemp, false);

  // Initial calibration state
  fetch('/api/coolantcal').then(r => r.json()).then(applyCoolantState).catch(() => {});
}

function calPost(payload) {
  return fetch('/api/coolantcal', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  }).then(r => r.json()).then(applyCoolantState).catch(e => console.log('coolantcal error:', e));
}

function applyCoolantState(s) {
  if (!s) return;
  coolantState = Object.assign(coolantState, s);
  const maxDuty = coolantState.maxDuty || 1023;
  const duty = s.duty || 0;
  const dutyNow = document.getElementById('coolantDutyNow');
  if (dutyNow) dutyNow.textContent = duty;
  const dutyMax = document.getElementById('coolantDutyMax');
  if (dutyMax) dutyMax.textContent = maxDuty;
  const dutyPct = document.getElementById('coolantDutyPct');
  if (dutyPct) dutyPct.textContent = (duty / maxDuty * 100).toFixed(1);
  const capDuty = document.getElementById('coolantCaptureDuty');
  if (capDuty) capDuty.textContent = duty;
  const calModeEl = document.getElementById('coolantCalMode');
  if (calModeEl) calModeEl.checked = !!s.calMode;
  renderCoolantPoints();
  drawCoolantCurve();
}

function renderCoolantPoints() {
  const list = document.getElementById('coolantPointsList');
  if (!list) return;
  const pts = coolantState.points || [];
  if (!pts.length) {
    list.innerHTML = '<p class="hint">No points captured yet.</p>';
    return;
  }
  let html = '<div class="cal-point-head"><span>Temp</span><span>Duty</span><span></span></div>';
  pts.forEach((p, i) => {
    html += '<div class="cal-point-row">' +
              '<span>' + p.temp + ' \u00B0C</span>' +
              '<span>' + p.duty + '</span>' +
              '<button class="cal-del" data-index="' + i + '" title="Remove point">\u2715</button>' +
            '</div>';
  });
  list.innerHTML = html;
  list.querySelectorAll('.cal-del').forEach(btn => {
    btn.addEventListener('click', () => {
      calPost({ op: 'deletePoint', index: parseInt(btn.dataset.index, 10) });
    });
  });
}

// Press-and-hold helper: fires immediately, then repeats faster while held.
function attachHold(el, fn) {
  let timer = null;
  let delay = 320;
  const stop = () => { if (timer) { clearTimeout(timer); timer = null; } };
  const start = (e) => {
    e.preventDefault();
    fn();
    delay = 320;
    const tick = () => { fn(); delay = Math.max(60, delay * 0.8); timer = setTimeout(tick, delay); };
    timer = setTimeout(tick, delay);
  };
  el.addEventListener('pointerdown', start);
  el.addEventListener('pointerup', stop);
  el.addEventListener('pointerleave', stop);
  el.addEventListener('pointercancel', stop);
}

function setCoolantTarget(temp, fromChip) {
  coolantSelectedTemp = temp;
  const input = document.getElementById('coolantTargetTemp');
  if (input && fromChip) input.value = temp;
  const capTemp = document.getElementById('coolantCaptureTemp');
  if (capTemp) capTemp.textContent = temp;
  highlightCoolantChip();
}

function highlightCoolantChip() {
  document.querySelectorAll('#coolantTargetChips .chip').forEach(chip => {
    chip.classList.toggle('active', Number(chip.dataset.temp) === coolantSelectedTemp);
  });
}

function drawCoolantCurve() {
  const canvas = document.getElementById('coolantCurveCanvas');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');
  const W = canvas.width, H = canvas.height;
  const styles = getComputedStyle(document.documentElement);
  const cPrimary = (styles.getPropertyValue('--primary') || '#00D9FF').trim();
  const cSecondary = (styles.getPropertyValue('--secondary') || '#FF6B35').trim();
  const cSuccess = (styles.getPropertyValue('--success') || '#2EA043').trim();
  const cDim = (styles.getPropertyValue('--text-dim') || '#8B949E').trim();
  const cGrid = (styles.getPropertyValue('--border') || '#30363D').trim();

  ctx.clearRect(0, 0, W, H);
  const padL = 46, padR = 16, padT = 16, padB = 34;
  const plotW = W - padL - padR, plotH = H - padT - padB;

  const pts = (coolantState.points || []).slice().sort((a, b) => a.temp - b.temp);

  let tMin = 0, tMax = 120;
  if (pts.length) {
    tMin = Math.min(tMin, pts[0].temp);
    tMax = Math.max(tMax, pts[pts.length - 1].temp);
  }
  if (typeof coolantState.temp === 'number') {
    tMin = Math.min(tMin, coolantState.temp);
    tMax = Math.max(tMax, coolantState.temp);
  }
  if (tMax - tMin < 10) tMax = tMin + 10;
  const dMax = coolantState.maxDuty || 1023;

  const xOf = t => padL + (t - tMin) / (tMax - tMin) * plotW;
  const yOf = d => padT + plotH - (d / dMax) * plotH;

  ctx.strokeStyle = cGrid;
  ctx.lineWidth = 1;
  ctx.fillStyle = cDim;
  ctx.font = '11px sans-serif';
  ctx.textAlign = 'right';
  ctx.textBaseline = 'middle';
  for (let i = 0; i <= 4; i++) {
    const d = dMax * i / 4;
    const y = yOf(d);
    ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(W - padR, y); ctx.stroke();
    ctx.fillText(Math.round(d), padL - 6, y);
  }
  ctx.textAlign = 'center';
  ctx.textBaseline = 'top';
  for (let i = 0; i <= 4; i++) {
    const t = tMin + (tMax - tMin) * i / 4;
    ctx.fillText(Math.round(t) + '\u00B0', xOf(t), H - padB + 6);
  }

  if (pts.length >= 1) {
    ctx.strokeStyle = cPrimary;
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(xOf(tMin), yOf(pts[0].duty));
    pts.forEach(p => ctx.lineTo(xOf(p.temp), yOf(p.duty)));
    ctx.lineTo(xOf(tMax), yOf(pts[pts.length - 1].duty));
    ctx.stroke();
    ctx.fillStyle = cSecondary;
    pts.forEach(p => {
      ctx.beginPath();
      ctx.arc(xOf(p.temp), yOf(p.duty), 4, 0, Math.PI * 2);
      ctx.fill();
    });
  }

  if (typeof coolantState.temp === 'number') {
    const lt = coolantState.temp;
    const ld = coolantState.appliedDuty || 0;
    ctx.strokeStyle = cSuccess;
    ctx.setLineDash([4, 4]);
    ctx.beginPath();
    ctx.moveTo(xOf(lt), padT);
    ctx.lineTo(xOf(lt), padT + plotH);
    ctx.stroke();
    ctx.setLineDash([]);
    ctx.fillStyle = cSuccess;
    ctx.beginPath();
    ctx.arc(xOf(lt), yOf(ld), 5, 0, Math.PI * 2);
    ctx.fill();
  }
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

// ===== Small DOM helper =====
function setText(id, val) {
  const el = document.getElementById(id);
  if (el) el.textContent = val;
}

// ============================================================================
// Output pin conflict handling (EML / EPC shared by Shift Light, DSG Park and
// the Coolant gauge). Applying a new owner for a pin turns off whatever else
// was using it, with a confirmation prompt + toast so nothing is silently lost.
// ============================================================================
const OUTPUT_IDS = ['shiftLight', 'dsgParkMode', 'coolantOutput'];
const FEATURE_LABELS = {
  shiftLight: 'Shift Light',
  dsgParkMode: 'DSG Park Indicator',
  coolantOutput: 'Coolant Gauge'
};
let lastOutputValues = {};

function pinsForFeature(featureId, value) {
  if (featureId === 'shiftLight') {
    if (value === 'Both') return ['EML', 'EPC'];
    if (value === 'EML') return ['EML'];
    if (value === 'EPC') return ['EPC'];
    return [];
  }
  // dsgParkMode and coolantOutput each own a single pin (or none)
  if (value === 'EML') return ['EML'];
  if (value === 'EPC') return ['EPC'];
  return [];
}

function clearedValueForPin(featureId, currentValue, pin) {
  if (featureId === 'shiftLight') {
    // "Both" releasing one pin keeps the other; otherwise turn the light off
    if (currentValue === 'Both') return pin === 'EML' ? 'EPC' : 'EML';
    return 'None';
  }
  if (featureId === 'coolantOutput') return 'Off';
  return 'None'; // dsgParkMode
}

function syncLastOutputValues() {
  OUTPUT_IDS.forEach(id => {
    const el = document.getElementById(id);
    if (el) lastOutputValues[id] = el.value;
  });
}

function initOutputConflicts() {
  OUTPUT_IDS.forEach(id => {
    const el = document.getElementById(id);
    if (el) el.addEventListener('change', () => handleOutputChange(id, el));
  });
  syncLastOutputValues();
}

function handleOutputChange(changingId, el) {
  const newValue = el.value;
  const prevValue = lastOutputValues[changingId];
  const desiredPins = pinsForFeature(changingId, newValue);

  // Find other features currently claiming any of the pins we now want
  const seen = new Set();
  const conflicts = [];
  desiredPins.forEach(pin => {
    OUTPUT_IDS.forEach(otherId => {
      if (otherId === changingId || seen.has(otherId)) return;
      const otherEl = document.getElementById(otherId);
      if (!otherEl) return;
      if (pinsForFeature(otherId, otherEl.value).includes(pin)) {
        seen.add(otherId);
        conflicts.push({
          id: otherId,
          el: otherEl,
          cleared: clearedValueForPin(otherId, otherEl.value, pin)
        });
      }
    });
  });

  if (conflicts.length) {
    const msg =
      `${FEATURE_LABELS[changingId]} will use the ${desiredPins.join(' & ')} ` +
      `output${desiredPins.length > 1 ? 's' : ''}.\n\nThis will turn off:\n` +
      conflicts.map(c => `  \u2022 ${FEATURE_LABELS[c.id]} (currently ${c.el.value})`).join('\n') +
      `\n\nApply this change?`;
    if (!confirm(msg)) {
      el.value = prevValue; // user cancelled: restore previous selection
      return;
    }
  }

  (async () => {
    // Release conflicting owners first so the server sees pins free
    for (const c of conflicts) {
      c.el.value = c.cleared;
      lastOutputValues[c.id] = c.cleared;
      await pushControl(c.id, c.cleared);
    }
    await pushControl(changingId, newValue);
    lastOutputValues[changingId] = newValue;

    if (conflicts.length) {
      showNotification(
        `${FEATURE_LABELS[changingId]} now uses ${desiredPins.join(' & ')} \u2014 ` +
        conflicts.map(c => FEATURE_LABELS[c.id]).join(' & ') + ' turned off'
      );
    }
  })();
}

// ============================================================================
// OTA update manager
// ============================================================================
let otaSelectedFile = null;

function initOta() {
  const dropZone = document.getElementById('otaDropZone');
  const fileInput = document.getElementById('otaFile');
  const chooseBtn = document.getElementById('otaChooseBtn');
  const uploadBtn = document.getElementById('otaUploadBtn');
  if (!dropZone || !fileInput || !uploadBtn) return;

  // Populate firmware info
  fetch('/api/ota/info')
    .then(r => r.json())
    .then(info => {
      setText('otaFwVersion', info.version || '--');
      setText('otaHardware', info.hardware || '--');
      setText('otaBoard', info.board || '--');
    })
    .catch(() => { /* offline: leave placeholders */ });

  const pick = () => fileInput.click();
  if (chooseBtn) chooseBtn.addEventListener('click', (e) => { e.stopPropagation(); pick(); });
  dropZone.addEventListener('click', pick);

  fileInput.addEventListener('change', () => {
    if (fileInput.files && fileInput.files.length) selectOtaFile(fileInput.files[0]);
  });

  ['dragenter', 'dragover'].forEach(ev =>
    dropZone.addEventListener(ev, (e) => {
      e.preventDefault();
      dropZone.classList.add('drag-over');
    })
  );
  ['dragleave', 'drop'].forEach(ev =>
    dropZone.addEventListener(ev, (e) => {
      e.preventDefault();
      dropZone.classList.remove('drag-over');
    })
  );
  dropZone.addEventListener('drop', (e) => {
    if (e.dataTransfer.files && e.dataTransfer.files.length) selectOtaFile(e.dataTransfer.files[0]);
  });

  uploadBtn.addEventListener('click', startOtaUpload);
}

function selectOtaFile(file) {
  if (!file.name.toLowerCase().endsWith('.bin')) {
    setOtaStatus('Please choose a .bin file', 'error');
    return;
  }
  otaSelectedFile = file;
  const dropZone = document.getElementById('otaDropZone');
  if (dropZone) dropZone.classList.add('file-selected');
  setText('otaFileName', file.name);
  setOtaStatus('', '');
}

function setOtaStatus(msg, type) {
  const el = document.getElementById('otaStatus');
  if (!el) return;
  el.textContent = msg;
  el.className = 'ota-status' + (type ? ' ' + type : '');
}

function startOtaUpload() {
  if (!otaSelectedFile) {
    setOtaStatus('Select a .bin file first', 'error');
    return;
  }
  const type = (document.getElementById('otaType') || {}).value || 'firmware';
  const url = type === 'filesystem' ? '/api/ota/fs' : '/api/ota';
  const uploadBtn = document.getElementById('otaUploadBtn');
  const progressWrap = document.getElementById('otaProgressWrap');
  const progressBar = document.getElementById('otaProgressBar');
  const progressLabel = document.getElementById('otaProgressLabel');

  const formData = new FormData();
  formData.append('update', otaSelectedFile, otaSelectedFile.name);

  const xhr = new XMLHttpRequest();
  xhr.open('POST', url);

  if (uploadBtn) uploadBtn.disabled = true;
  if (progressWrap) progressWrap.style.display = 'block';
  if (progressBar) progressBar.style.width = '0%';
  if (progressLabel) progressLabel.textContent = '0%';
  setOtaStatus('Uploading...', '');

  xhr.upload.addEventListener('progress', (e) => {
    if (!e.lengthComputable) return;
    const pct = Math.round((e.loaded / e.total) * 100);
    if (progressBar) progressBar.style.width = pct + '%';
    if (progressLabel) progressLabel.textContent = pct + '%';
  });

  xhr.addEventListener('load', () => {
    let ok = false;
    try { ok = JSON.parse(xhr.responseText).success === true; } catch (e) { ok = xhr.status === 200; }
    if (ok) {
      if (progressBar) progressBar.style.width = '100%';
      if (progressLabel) progressLabel.textContent = '100%';
      setOtaStatus('Update complete. Device rebooting...', 'success');
    } else {
      setOtaStatus('Update failed. Please try again.', 'error');
      if (progressWrap) progressWrap.style.display = 'none';
      if (uploadBtn) uploadBtn.disabled = false;
    }
  });

  xhr.addEventListener('error', () => {
    setOtaStatus('Upload failed. Check connection and retry.', 'error');
    if (progressWrap) progressWrap.style.display = 'none';
    if (uploadBtn) uploadBtn.disabled = false;
  });

  xhr.send(formData);
}

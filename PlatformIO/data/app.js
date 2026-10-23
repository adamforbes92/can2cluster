document.addEventListener('DOMContentLoaded', initApp);

let settingsLoaded = false;

// Tab navigation
function initApp() {
  initNavigation();
  initControls();
  initCollapsibleCards();
  initCoolant();
  initOutputConflicts();
  initGaugeUI();
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

  // Test Paddle +/- buttons: a short pull sent on the selected paddle frame
  const paddleTestUpBtn = document.getElementById('paddleTestUp');
  if (paddleTestUpBtn) {
    paddleTestUpBtn.addEventListener('click', () => pushAction('paddleTestUp'));
  }
  const paddleTestDownBtn = document.getElementById('paddleTestDown');
  if (paddleTestDownBtn) {
    paddleTestDownBtn.addEventListener('click', () => pushAction('paddleTestDown'));
  }

  const otaFsUploadBtn = document.getElementById('otaFsUploadBtn');

  // Configuration controls
  const configInputs = ['hasNeedleSweep', 'sweepSpeed', 'stepRPM', 'stepSpeed', 'maxSpeed', 'shiftLimit', 'shiftFlashes', 'coilType', 'useMPH', 'gatewayFrames'];
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
    'testReverse', 'testEML', 'testEPC', 'diagTest',
    'paddleOutput'
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

  // Input Mode (pins 10/11/12): paddles or MK4 cruise stalk. Ask first - in
  // paddle mode a cruise stalk still wired to pins 10/11 would shift the gearbox.
  const inputModeEl = document.getElementById('inputMode');
  if (inputModeEl) {
    inputModeEl.addEventListener('change', () => {
      const value = inputModeEl.value;
      let msg = 'Switch pins 10/11/12 to the MK4 cruise stalk?\n\nPaddle shifting through Can2Cluster stops, hall speed stops, and the stalk is sent to the ECU on 0x38A.';
      if (value === 'Paddles') {
        msg = 'Switch pins 10/11/12 to DSG paddles?\n\nOnly do this if the cruise stalk is NOT wired to them - in paddle mode, grounding pin 10 or 11 shifts the gearbox.';
      }
      if (!confirm(msg)) {
        inputModeEl.value = inputModeEl.dataset.last || 'Paddles'; // put the old choice back
        return;
      }
      inputModeEl.dataset.last = value;
      pushControl('inputMode', value);
    });
  }

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

    // Show only the options this board has (before the values are filled in,
    // so the coolant drop-down already holds the right choices)
    applyBoard(data.board || 'esp32');

    // Load all settings from API once
    document.getElementById('hasNeedleSweep').checked = data.hasNeedleSweep || false;
    document.getElementById('sweepSpeed').value = data.sweepSpeed || 0;
    setText('sweepSpeed-display', data.sweepSpeed || 0);
    document.getElementById('stepRPM').value = data.stepRPM || 12;
    setText('stepRPM-display', data.stepRPM || 12);
    document.getElementById('stepSpeed').value = data.stepSpeed || 10;
    setText('stepSpeed-display', data.stepSpeed || 10);
    const dialTop = data.maxSpeed || 200;
    document.getElementById('maxSpeed').value = dialTop;
    setText('maxSpeed-display', dialTop);
    // the Speed Output Test slider (Diagnostics) goes up to the dial top too
    const tempSpeedEl = document.getElementById('tempSpeed');
    if (tempSpeedEl) tempSpeedEl.max = dialTop;
    document.getElementById('shiftLight').value = data.shiftLight || 'None';
    document.getElementById('shiftLimit').value = data.shiftLimit || 0;
    document.getElementById('shiftFlashes').value = data.shiftFlashes || 0;
    document.getElementById('coilType').checked = data.coilType || false;
    const useMPHEl = document.getElementById('useMPH');
    if (useMPHEl) useMPHEl.checked = data.useMPH || false;
    applySpeedUnitLabels(data.useMPH);
    document.getElementById('dsgParkMode').value = data.dsgParkMode || 'None';
    const paddleOutputEl = document.getElementById('paddleOutput');
    if (paddleOutputEl) paddleOutputEl.value = data.paddleOutput || 'Off';
    const inputModeEl = document.getElementById('inputMode');
    if (inputModeEl) {
      inputModeEl.value = data.inputMode || 'Paddles';
      inputModeEl.dataset.last = inputModeEl.value; // what to put back if a change is cancelled
    }
    const gatewayFramesEl = document.getElementById('gatewayFrames');
    if (gatewayFramesEl) gatewayFramesEl.checked = data.gatewayFrames || false;

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
    updateTileGauges();

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

    // System status (read-only, not settings) — rendered as status pills
    const canOk = !!data.hasCAN;
    const canStatusEl = document.getElementById('canStatus');
    canStatusEl.textContent = canOk ? 'CAN: Healthy' : 'CAN: Not Healthy';
    canStatusEl.className = 'status-badge ' + (canOk ? 'ok' : 'error');
    setValuePill('canPresent', canOk ? 'ok' : 'bad', canOk ? 'Healthy' : 'Not Healthy');
    if (document.getElementById('gpsPresent')) {
      const sats = data.gpsSatellites || 0;
      if (data.gpsUnavailable) setValuePill('gpsPresent', 'bad', 'Not Available');
      else if (data.hasGPS && sats > 0) setValuePill('gpsPresent', 'ok', `Connected (${sats} sats)`);
      else if (data.hasGPS) setValuePill('gpsPresent', 'warn', 'Connected (no sats)');
      else setValuePill('gpsPresent', 'bad', 'Not Connected');
    }

    // Output status: active=green, inactive=neutral, test-forced=orange
    const setOutputPill = (id, active, testActive) => {
      setValuePill(id, testActive ? 'warn' : (active ? 'ok' : ''), active ? 'Active' : 'Inactive');
      const el = document.getElementById(id);
      if (el) el.title = testActive ? 'Test Mode: Forced ON' : '';
    };
    setOutputPill('emlStatus', !!data.vehicleEML || testEMLActive, testEMLActive);
    setOutputPill('epcStatus', !!data.vehicleEPC || testEPCActive, testEPCActive);
    setOutputPill('reverseStatus', !!data.vehicleReverse || testReverseActive, testReverseActive);
    setValuePill('parkStatus', data.vehiclePark ? 'ok' : '', data.vehiclePark ? 'Active' : 'Inactive');

    // Paddle feedback - in cruise stalk mode the same inputs are the cruise stalk
    const isCruiseMode = !!data.cruiseMode;
    if (isCruiseMode) {
      setText('paddleUpLabel', 'Cruise Main');
      setText('paddleDownLabel', 'Cruise SET');
      setValuePill('paddleUpStatus', data.cruiseMain ? 'ok' : '', data.cruiseMain ? 'On' : 'Off');
      setValuePill('paddleDownStatus', data.cruiseSet ? 'ok' : '', data.cruiseSet ? 'Pressed' : 'Released');
      setValuePill('cruiseResumeStatus', data.cruiseResume ? 'ok' : '', data.cruiseResume ? 'Pressed' : 'Released');
    } else {
      setText('paddleUpLabel', 'Paddle Up');
      setText('paddleDownLabel', 'Paddle Down');
      setValuePill('paddleUpStatus', data.paddleUp ? 'ok' : '', data.paddleUp ? 'Active' : 'Inactive');
      setValuePill('paddleDownStatus', data.paddleDown ? 'ok' : '', data.paddleDown ? 'Active' : 'Inactive');
    }
    showElement('cruiseResumeItem', isCruiseMode);
    showElement('cruiseGrid', isCruiseMode);
    showElement('cruiseHint', isCruiseMode);
    showElement('paddleStalkNote', isCruiseMode);
    ['paddleOutput', 'paddleTestUp', 'paddleTestDown'].forEach((id) => {
      const el = document.getElementById(id);
      if (el) el.disabled = isCruiseMode;
    });

    // MK4 cruise stalk live readout (Input Mode card)
    if (isCruiseMode) {
      setValuePill('cruiseMain', data.cruiseMain ? 'ok' : '', data.cruiseMain ? 'On' : 'Off');
      setValuePill('cruiseSet', data.cruiseSet ? 'ok' : '', data.cruiseSet ? 'Pressed' : 'Released');
      setValuePill('cruiseResume', data.cruiseResume ? 'ok' : '', data.cruiseResume ? 'Pressed' : 'Released');
      if (data.paddlePqGatewayPresent) {
        setValuePill('cruiseSending', 'warn', 'Another module sends 0x38A');
      } else {
        const buttons = data.cruiseButtons || 0;      // GRA byte 1: bit 0 main, bit 1 cancel
        const setResume = data.cruiseSetResume || 0;  // GRA byte 2: bit 0 SET, bit 1 RESUME
        const parts = [];
        if (buttons & 0x01) parts.push('Main on');
        if (buttons & 0x02) parts.push('Cancel');
        if (setResume & 0x01) parts.push('SET');
        if (setResume & 0x02) parts.push('RESUME');
        setValuePill('cruiseSending', (buttons & 0x01) ? 'ok' : '', parts.length ? parts.join(' + ') : 'Main off');
      }
    }

    // PQ gateway frames live readout: 0 off, 1 sending, 2 another module sends it
    const gwNames = { 0: 'Off', 1: 'Sending', 2: 'Other module sends' };
    const gwStates = { 0: '', 1: 'ok', 2: 'warn' };
    [['gwBsgLast', data.gwBsgLast], ['gwGateKomf1', data.gwGateKomf1], ['gwDiagnose1', data.gwDiagnose1]].forEach(([id, state]) => {
      setValuePill(id, gwStates[state] || '', gwNames[state] || '--');
    });
    if (data.clusterKmValid) setValuePill('gwKm', 'ok', Number(data.clusterKm).toLocaleString() + ' km');
    else setValuePill('gwKm', data.gatewayFrames ? 'warn' : '', 'Not seen (0 sent)');

    // DSG paddle live readout (DSG Paddle Output card). 255 = that frame hasn't been seen.
    if (document.getElementById('paddleSending')) {
      const paddleMode = (document.getElementById('paddleOutput') || {}).value || 'Off';
      const sendNames = { 0: 'Nothing pulled', 1: 'Paddle -', 2: 'Paddle +', 3: 'Both' };
      // Frames a real gateway is already sending, so Can2Cluster stays quiet on them
      const ownedIds = [];
      if (data.paddleGatewayPresent && paddleMode !== 'PQ') ownedIds.push('0x3DD');
      if (data.paddlePqGatewayPresent && paddleMode !== 'MQB') ownedIds.push('0x38A');
      if (isCruiseMode) {
        setValuePill('paddleSending', '', 'Not used (cruise stalk)');
      } else if (ownedIds.length) {
        setValuePill('paddleSending', 'warn', 'Gateway owns ' + ownedIds.join(' + '));
      } else if (paddleMode === 'Off') {
        setValuePill('paddleSending', '', 'Off');
      } else {
        const sending = data.paddleSending || 0;
        setValuePill('paddleSending', sending ? 'ok' : '', sendNames[sending] || '--');
      }

      const leverNames = { 0: 'Init', 4: 'Not pressed', 5: 'D', 6: 'N', 7: 'R', 8: 'P', 10: 'Tip +', 11: 'Tip -', 12: 'S', 13: 'L', 14: 'M gate', 15: 'Error' };
      if (data.mqbLever === undefined || data.mqbLever === 255) setValuePill('mqbLever', '', 'Not seen');
      else setValuePill('mqbLever', data.mqbLever === 15 ? 'bad' : 'ok', leverNames[data.mqbLever] || ('Pos ' + data.mqbLever));

      const tipNames = { 0: 'None (D/S)', 1: 'M gate', 2: 'Paddle (temporary)', 3: 'Unknown' };
      if (data.mqbTipState === undefined || data.mqbTipState === 255) setValuePill('mqbTipMode', '', 'Not seen');
      else setValuePill('mqbTipMode', data.mqbTipState === 2 ? 'ok' : '', tipNames[data.mqbTipState]);

      setValuePill('dsgGear', '', data.dsgGear ? String(data.dsgGear) : '-');

      const warnNames = { 0: 'None', 1: 'Level I', 2: 'Level II', 3: 'Restricted', 4: 'D only', 5: 'Roll-away', 6: 'Paddle limp + warn', 7: 'Paddle limp', 8: 'Speed limit' };
      if (data.mqbGearboxWarning === undefined || data.mqbGearboxWarning === 255) setValuePill('mqbGearboxWarning', '', 'Not seen');
      else setValuePill('mqbGearboxWarning', data.mqbGearboxWarning ? 'warn' : 'ok', warnNames[data.mqbGearboxWarning] || ('Code ' + data.mqbGearboxWarning));
    }

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

    // Board diagnostics (S3-only items are hidden on the DevKit by applyBoard)
    if (data.boardInfo) {
      const info = data.boardInfo;
      setText('liveBoard', info.name || info.board);
      if (info.board === 'esp32s3') {
        setText('liveMcp4725', info.mcp4725 ? 'Detected' : 'Missing');
        setText('liveCoolantDac', info.coolantDac);
        setText('liveBoardId', info.boardIdMv + ' mV');
        setText('liveCanTerm', info.canTermination ? 'On (120R)' : 'Off');
        setText('liveOutputPullUp', info.outputPullUp ? 'On (12V)' : 'Off');
        let biasText = 'None';
        if (info.speedBias === 1) biasText = 'Pull-up (12V)';
        if (info.speedBias === 2) biasText = 'Pull-down';
        setText('liveSpeedBias', biasText);
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
  const maxSpeedUnitEl = document.getElementById('maxSpeed-unit');
  if (maxSpeedUnitEl) maxSpeedUnitEl.textContent = label;
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

/* =======================================================================
   Per-tile dial gauges (ported from the OpenHaldex theme). Any dashboard
   .gauge tile listed below can render as a 270° dial instead of a number,
   toggled per-tile in Display Options and saved in this browser.
   ======================================================================= */
const GAUGE_TILES = [
  { id: "rpm",   label: "RPM",   min: 0, max: 8000, unit: "rpm" },
  { id: "speed", label: "Speed", min: 0, max: 300,  unit: "km/h" },
];
const TG_R = 40;
const TG_CIRC = 2 * Math.PI * TG_R;
const TG_ARC = TG_CIRC * 0.75;
const TG_GAP = TG_CIRC - TG_ARC;
const GAUGE_PREFS_KEY = "c2cGaugePrefs";
const GAUGE_DEFAULTS = { tiles: ["rpm", "speed"] };
let gaugePrefs = loadGaugePrefs();

function loadGaugePrefs() {
  try {
    const raw = localStorage.getItem(GAUGE_PREFS_KEY);
    if (raw) {
      const p = JSON.parse(raw);
      return { tiles: Array.isArray(p.tiles) ? p.tiles : GAUGE_DEFAULTS.tiles.slice() };
    }
  } catch (e) { /* fall through to defaults */ }
  return { tiles: GAUGE_DEFAULTS.tiles.slice() };
}
function saveGaugePrefs() {
  try { localStorage.setItem(GAUGE_PREFS_KEY, JSON.stringify(gaugePrefs)); } catch (e) {}
}

// Inject the dial SVG (once) into a tile; colours come from CSS via var().
function ensureTileGauge(tile) {
  if (tile.querySelector(".tile-gauge")) return;
  const wrap = document.createElement("div");
  wrap.className = "tile-gauge";
  wrap.innerHTML =
    `<svg viewBox="0 0 100 100" aria-hidden="true">` +
    `<circle class="tg-track" cx="50" cy="50" r="${TG_R}" transform="rotate(135 50 50)" ` +
    `stroke-dasharray="${TG_ARC.toFixed(2)} ${TG_GAP.toFixed(2)}"/>` +
    `<circle class="tg-fill" cx="50" cy="50" r="${TG_R}" transform="rotate(135 50 50)" ` +
    `stroke-dasharray="0 ${TG_CIRC.toFixed(2)}"/>` +
    `<text class="tg-val" x="50" y="52" text-anchor="middle">--</text>` +
    `<text class="tg-unit" x="50" y="66" text-anchor="middle"></text>` +
    `<text class="tg-min" x="24" y="92" text-anchor="middle">0</text>` +
    `<text class="tg-max" x="76" y="92" text-anchor="middle">0</text>` +
    `</svg>`;
  tile.appendChild(wrap);
}

// Reflect prefs: toggle .as-gauge per tile and set the gauge unit label.
function applyGaugePrefs() {
  GAUGE_TILES.forEach((t) => {
    const el = document.getElementById(t.id);
    if (!el) return;
    const tile = el.closest(".gauge");
    if (!tile) return;
    ensureTileGauge(tile);
    const on = gaugePrefs.tiles.includes(t.id);
    tile.classList.toggle("as-gauge", on);
    if (on) {
      const unitEl = tile.querySelector(".tg-unit");
      const srcUnit = tile.querySelector(".gauge-unit");
      if (unitEl) unitEl.textContent = srcUnit ? srcUnit.textContent.trim() : t.unit;
      const minEl = tile.querySelector(".tg-min");
      const maxEl = tile.querySelector(".tg-max");
      if (minEl) minEl.textContent = t.min;
      if (maxEl) maxEl.textContent = t.max;
    }
  });
}

// Redraw every tile in gauge mode from the value the poll just wrote.
function updateTileGauges() {
  GAUGE_TILES.forEach((t) => {
    const el = document.getElementById(t.id);
    if (!el) return;
    const tile = el.closest(".gauge");
    if (!tile || !tile.classList.contains("as-gauge")) return;
    const raw = parseFloat(el.textContent);
    const valEl = tile.querySelector(".tg-val");
    const fillEl = tile.querySelector(".tg-fill");
    if (!valEl || !fillEl) return;
    // Mirror the numeric test-mode highlight (poll paints the number orange).
    const gaugeWrap = tile.querySelector(".tile-gauge");
    if (gaugeWrap) gaugeWrap.classList.toggle("warn", el.style.color === "orange");
    if (Number.isNaN(raw)) {
      valEl.textContent = "--";
      fillEl.style.strokeDasharray = `0 ${TG_CIRC.toFixed(2)}`;
      return;
    }
    valEl.textContent = el.textContent;
    // Grow a single arc from the start up to the value (do NOT animate offset,
    // which would just rotate the whole 3/4 ring instead of filling it).
    const frac = Math.max(0, Math.min(1, (raw - t.min) / (t.max - t.min || 1)));
    fillEl.style.strokeDasharray = `${(TG_ARC * frac).toFixed(2)} ${TG_CIRC.toFixed(2)}`;
  });
}

// Build the Display Options per-tile customizer and apply saved prefs.
function initGaugeUI() {
  const host = document.getElementById("gaugeCustomizer");
  if (host) {
    host.innerHTML = "";
    GAUGE_TILES.forEach((t) => {
      const label = document.createElement("label");
      label.className = "tile-opt";
      const cb = document.createElement("input");
      cb.type = "checkbox";
      cb.checked = gaugePrefs.tiles.includes(t.id);
      cb.addEventListener("change", () => {
        const set = new Set(gaugePrefs.tiles);
        if (cb.checked) set.add(t.id); else set.delete(t.id);
        gaugePrefs.tiles = [...set];
        saveGaugePrefs();
        applyGaugePrefs();
      });
      const span = document.createElement("span");
      span.textContent = t.label;
      label.appendChild(cb);
      label.appendChild(span);
      host.appendChild(label);
    });
  }
  applyGaugePrefs();
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

// Show or hide one element by id (inline style, like applyBoard below)
function showElement(id, show) {
  const el = document.getElementById(id);
  if (el) el.style.display = show ? '' : 'none';
}

// One web page serves both boards. Anything marked data-board="esp32" (DevKit)
// or data-board="esp32s3" (S3) is only shown on that board; <option>s are
// also disabled so they can't be picked.
function applyBoard(board) {
  document.querySelectorAll('[data-board]').forEach(function (el) {
    const isThisBoard = (el.dataset.board === board);
    // inline style beats any display: rule in style.css (the hidden attribute doesn't)
    if (isThisBoard) {
      el.style.display = '';
    } else {
      el.style.display = 'none';
    }
    if (el.tagName === 'OPTION') {
      el.disabled = !isThisBoard;
    }
  });
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

// Set a status tile value as a coloured pill (state = ok|bad|warn|'').
function setValuePill(id, state, text) {
  const el = document.getElementById(id);
  if (!el) return;
  el.textContent = text;
  el.className = 'status-value pill' + (state ? ' ' + state : '');
}

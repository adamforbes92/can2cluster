document.addEventListener('DOMContentLoaded', initApp);

let settingsLoaded = false;

// Tab navigation
function initApp() {
  initNavigation();
  initControls();
  initOTA();
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

  // Configuration controls
  const configInputs = ['hasNeedleSweep', 'sweepSpeed', 'stepRPM', 'stepSpeed', 'shiftLight', 'shiftLimit', 'shiftFlashes', 'coilType', 'dsgParkMode'];
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
  const advancedInputs = ['testRPM', 'tempRPM', 'testSpeedo', 'tempSpeed', 'testReverse', 'testEML', 'testEPC'];
  advancedInputs.forEach(id => {
    const el = document.getElementById(id);
    if (el) {
      el.addEventListener('change', () => {
        const value = el.type === 'checkbox' ? el.checked : el.value;
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
  if (speedSourceEl) {
    speedSourceEl.addEventListener('change', () => {
      pushControl('speedType', speedSourceEl.value);
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

    // Test outputs
    document.getElementById('testReverse').checked = data.testReverse || false;
    document.getElementById('testEML').checked = data.testEML || false;
    document.getElementById('testEPC').checked = data.testEPC || false;

    // Speed type dropdown - map speedType to dropdown options
    let speedTypeValue = 'Hall';  // default
    if (data.speedType === 'ECU') speedTypeValue = 'ECU';
    else if (data.speedType === 'ABS') speedTypeValue = 'ABS';
    else if (data.speedType === 'DSG') speedTypeValue = 'DSG';
    else if (data.speedType === 'TP2.0-DSG' || data.speedType === 'TP/UDS DSG') speedTypeValue = 'TP2.0-DSG';
    else if (data.speedType === 'GPS') speedTypeValue = 'GPS';
    document.getElementById('speedSource').value = speedTypeValue;

    const rpmTypeValue = data.rpmType === 'Hall' ? 'Hall' : 'CAN';
    document.getElementById('rpmSource').value = rpmTypeValue;

    const clusterFrequencyLimitValue = data.clusterFrequencyLimit || 230;
    document.getElementById('clusterFrequencyLimit').value = clusterFrequencyLimitValue;
    document.getElementById('clusterFrequencyLimit-display').textContent = clusterFrequencyLimitValue;

    const clusterRPMLimitValue = data.clusterRPMLimit || 7000;
    document.getElementById('clusterRPMLimit').value = clusterRPMLimitValue;
    document.getElementById('clusterRPMLimit-display').textContent = clusterRPMLimitValue;

    // Update FW version
    const fwResponse = await fetch('/api/settings');
    const fwData = await fwResponse.json();
    document.getElementById('fwVersion').textContent = 'FW: ' + (fwData.FW_VERSION || '--');

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
    const speedEl = document.getElementById('speed');
    speedEl.textContent = data.vehicleSpeed || '--';
    speedEl.style.color = testSpeedActive ? 'orange' : '';
    speedEl.title = testSpeedActive ? 'Test Mode: ' + (data.tempSpeed || 0) + ' km/h' : '';

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
    if (document.getElementById('liveGPSStatus')) {
      if (data.hasGPS) {
        document.getElementById('liveGPSStatus').textContent = `Connected, ${data.gpsSatellites} satellites`;
      } else if (data.gpsTaskSuspended) {
        document.getElementById('liveGPSStatus').textContent = 'Unavailable';
      } else {
        document.getElementById('liveGPSStatus').textContent = 'Not Connected';
      }
    }

    // System status (read-only, not settings)
    document.getElementById('canStatus').textContent = data.hasCAN ? 'CAN: Healthy' : 'CAN: Not Healthy';
    document.getElementById('canPresent').textContent = data.hasCAN ? 'Healthy' : 'Not Healthy';
    document.getElementById('gpsPresent').textContent = data.hasGPS ? 'Yes' : 'No';
    
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

// OTA Update functionality
function initOTA() {
  fetchOTAInfo();
  
  const fileInput = document.getElementById('otaFileInput');
  const uploadBtn = document.getElementById('otaUploadBtn');
  
  if (fileInput) {
    fileInput.addEventListener('change', (e) => {
      const file = e.target.files[0];
      if (file) {
        if (!file.name.endsWith('.bin')) {
          showNotification('Please select a .bin file', 'error');
          fileInput.value = '';
          document.getElementById('otaFileName').textContent = 'No file selected';
          uploadBtn.disabled = true;
          return;
        }
        document.getElementById('otaFileName').textContent = file.name + ` (${(file.size / 1024 / 1024).toFixed(2)} MB)`;
        uploadBtn.disabled = false;
      }
    });
  }
  
  if (uploadBtn) {
    uploadBtn.addEventListener('click', startOTAUpdate);
  }
}

async function fetchOTAInfo() {
  try {
    const response = await fetch('/api/ota/info');
    const data = await response.json();
    
    document.getElementById('otaBoard').textContent = data.board || 'Unknown';
    document.getElementById('otaHardware').textContent = data.hardware || 'Unknown';
    document.getElementById('otaCurrentVersion').textContent = data.version || 'Unknown';
  } catch (error) {
    console.log('Error fetching OTA info:', error);
    showNotification('Failed to fetch device information', 'error');
  }
}

async function startOTAUpdate() {
  const fileInput = document.getElementById('otaFileInput');
  const file = fileInput.files[0];
  
  if (!file) {
    showNotification('Please select a file', 'error');
    return;
  }
  
  const uploadBtn = document.getElementById('otaUploadBtn');
  const progressContainer = document.getElementById('otaProgressContainer');
  const progressFill = document.getElementById('otaProgressFill');
  const progressPercent = document.getElementById('otaProgressPercent');
  const statusMessage = document.getElementById('otaStatusMessage');
  
  uploadBtn.disabled = true;
  fileInput.disabled = true;
  progressContainer.style.display = 'block';
  statusMessage.style.display = 'none';
  
  const formData = new FormData();
  formData.append('file', file);
  
  try {
    const xhr = new XMLHttpRequest();
    
    xhr.upload.addEventListener('progress', (e) => {
      if (e.lengthComputable) {
        const percent = Math.round((e.loaded / e.total) * 100);
        progressFill.style.width = percent + '%';
        progressPercent.textContent = percent + '%';
        document.getElementById('otaProgressLabel').textContent = 
          `Uploading... ${(e.loaded / 1024 / 1024).toFixed(2)} MB of ${(e.total / 1024 / 1024).toFixed(2)} MB`;
      }
    });
    
    xhr.addEventListener('load', () => {
      if (xhr.status === 200) {
        const response = JSON.parse(xhr.responseText);
        progressContainer.style.display = 'none';
        statusMessage.style.display = 'block';
        statusMessage.className = 'status-message success';
        statusMessage.textContent = response.message || 'Update completed successfully! Device will reboot...';
        showNotification('Firmware update started! Device will reboot.', 'success');
        
        // Reset form after delay
        setTimeout(() => {
          fileInput.value = '';
          document.getElementById('otaFileName').textContent = 'No file selected';
          uploadBtn.disabled = true;
          fileInput.disabled = false;
          fetchOTAInfo();
        }, 3000);
      } else {
        const response = JSON.parse(xhr.responseText);
        progressContainer.style.display = 'none';
        statusMessage.style.display = 'block';
        statusMessage.className = 'status-message error';
        statusMessage.textContent = response.message || 'Update failed. Please try again.';
        showNotification('Update failed: ' + (response.message || 'Unknown error'), 'error');
        uploadBtn.disabled = false;
        fileInput.disabled = false;
      }
    });
    
    xhr.addEventListener('error', () => {
      progressContainer.style.display = 'none';
      statusMessage.style.display = 'block';
      statusMessage.className = 'status-message error';
      statusMessage.textContent = 'Network error during upload. Please try again.';
      showNotification('Network error during upload', 'error');
      uploadBtn.disabled = false;
      fileInput.disabled = false;
    });
    
    xhr.open('POST', '/api/ota/upload');
    xhr.send(formData);
    
  } catch (error) {
    console.log('Error starting OTA update:', error);
    progressContainer.style.display = 'none';
    statusMessage.style.display = 'block';
    statusMessage.className = 'status-message error';
    statusMessage.textContent = 'Error: ' + error.message;
    uploadBtn.disabled = false;
    fileInput.disabled = false;
  }
}

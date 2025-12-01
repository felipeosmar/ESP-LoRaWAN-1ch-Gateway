// ESP32 LoRaWAN Gateway Web Interface

let ws = null;
let statusInterval = null;

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
    initTabs();
    initForms();
    connectWebSocket();
    loadStatus();
    loadConfigs();

    // Start periodic status updates
    statusInterval = setInterval(loadStatus, 5000);
});

// Tab navigation
function initTabs() {
    document.querySelectorAll('.tab').forEach(tab => {
        tab.addEventListener('click', () => {
            // Update active tab
            document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
            tab.classList.add('active');

            // Show corresponding content
            document.querySelectorAll('.tab-content').forEach(content => {
                content.classList.remove('active');
            });
            document.getElementById(tab.dataset.tab).classList.add('active');

            // Load files when Files tab is activated
            if (tab.dataset.tab === 'files') {
                refreshFiles();
            }
        });
    });
}

// Form handlers
function initForms() {
    // LoRa form
    document.getElementById('lora-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        await saveLoRaConfig();
    });

    // Server form
    document.getElementById('server-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        await saveServerConfig();
    });

    // WiFi form
    document.getElementById('wifi-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        await saveWiFiConfig();
    });

    // OTA form
    document.getElementById('ota-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        await uploadFirmware();
    });

    // NTP form
    document.getElementById('ntp-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        await saveNTPConfig();
    });

    // LCD form
    document.getElementById('lcd-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        await saveLCDConfig();
    });
}

// WebSocket connection
function connectWebSocket() {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    ws = new WebSocket(`${protocol}//${window.location.host}/ws`);

    ws.onopen = () => {
        console.log('WebSocket connected');
        addLog('WebSocket connected');
    };

    ws.onclose = () => {
        console.log('WebSocket disconnected');
        setTimeout(connectWebSocket, 3000);
    };

    ws.onerror = (error) => {
        console.error('WebSocket error:', error);
    };

    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            if (data.type === 'log') {
                addLog(data.message);
            }
        } catch (e) {
            console.error('Failed to parse WebSocket message:', e);
        }
    };
}

// Load status
async function loadStatus() {
    try {
        const response = await fetch('/api/status');
        const data = await response.json();

        // Update status indicators
        updateStatusIndicator('wifi-status',
            data.wifi.connected ? 'WiFi: Connected' : 'WiFi: AP Mode',
            data.wifi.connected);
        updateStatusIndicator('server-status',
            data.gateway.server_connected ? 'Server: Connected' : 'Server: Disconnected',
            data.gateway.server_connected);
        updateStatusIndicator('lora-status',
            data.lora.receiving ? 'LoRa: Active' : 'LoRa: Inactive',
            data.lora.available);

        // Update dashboard
        document.getElementById('gateway-eui').textContent = data.gateway.eui;
        document.getElementById('ip-address').textContent = data.wifi.ip;
        document.getElementById('uptime').textContent = formatUptime(data.system.uptime);
        document.getElementById('free-memory').textContent = formatBytes(data.system.heap_free);

        // Update stats
        document.getElementById('rx-packets').textContent = data.lora.rx_packets || 0;
        document.getElementById('last-rssi').textContent = `${data.lora.last_rssi || '--'} dBm`;
        document.getElementById('last-snr').textContent = `${data.lora.last_snr || '--'} dB`;

        // Update system info
        document.getElementById('chip-model').textContent = data.system.chip_model;
        document.getElementById('cpu-freq').textContent = `${data.system.cpu_freq} MHz`;
        document.getElementById('mac-address').textContent = data.wifi.mac;
        document.getElementById('heap-total').textContent = formatBytes(data.system.heap_total);

        // Load full stats
        loadStats();
    } catch (error) {
        console.error('Failed to load status:', error);
    }
}

// Load statistics
async function loadStats() {
    try {
        const response = await fetch('/api/stats');
        const data = await response.json();

        document.getElementById('rx-packets').textContent = data.lora.rx_received || 0;
        document.getElementById('tx-packets').textContent = data.lora.tx_sent || 0;
        document.getElementById('rx-forwarded').textContent = data.lora.rx_forwarded || 0;
        document.getElementById('crc-errors').textContent = data.lora.rx_crc_error || 0;
    } catch (error) {
        console.error('Failed to load stats:', error);
    }
}

// Load configurations
async function loadConfigs() {
    await loadLoRaConfig();
    await loadServerConfig();
    await loadWiFiConfig();
    await loadNTPConfig();
    await loadLCDConfig();
}

async function loadLoRaConfig() {
    try {
        const response = await fetch('/api/lora/config');
        const data = await response.json();

        document.getElementById('lora-enabled').checked = data.enabled;
        document.getElementById('lora-frequency').value = data.frequency;
        document.getElementById('lora-sf').value = data.spreading_factor;
        document.getElementById('lora-bw').value = data.bandwidth;
        document.getElementById('lora-cr').value = data.coding_rate;
        document.getElementById('lora-power').value = data.tx_power;
        document.getElementById('lora-syncword').value = data.sync_word || 52;
    } catch (error) {
        console.error('Failed to load LoRa config:', error);
    }
}

async function loadServerConfig() {
    try {
        const response = await fetch('/api/server/config');
        const data = await response.json();

        document.getElementById('server-enabled').checked = data.enabled;
        document.getElementById('server-host').value = data.host;
        document.getElementById('server-port-up').value = data.port_up;
        document.getElementById('server-port-down').value = data.port_down;
        document.getElementById('server-eui').value = data.gateway_eui;
        document.getElementById('server-description').value = data.description || '';
        document.getElementById('server-region').value = data.region || 'US915';
        document.getElementById('server-lat').value = data.latitude || '';
        document.getElementById('server-lon').value = data.longitude || '';
        document.getElementById('server-alt').value = data.altitude || '';
    } catch (error) {
        console.error('Failed to load server config:', error);
    }
}

async function loadWiFiConfig() {
    try {
        const response = await fetch('/api/wifi/config');
        const data = await response.json();

        document.getElementById('wifi-hostname').value = data.hostname || '';
        document.getElementById('wifi-mode').value = data.ap_mode.toString();
        document.getElementById('wifi-ssid').value = data.current_ssid || '';
    } catch (error) {
        console.error('Failed to load WiFi config:', error);
    }
}

async function loadNTPConfig() {
    try {
        const response = await fetch('/api/ntp/config');
        const data = await response.json();

        document.getElementById('ntp-enabled').checked = data.enabled;
        document.getElementById('ntp-server1').value = data.server1 || '';
        document.getElementById('ntp-server2').value = data.server2 || '';
        document.getElementById('ntp-timezone').value = data.timezone_offset || 0;
        document.getElementById('ntp-daylight').value = data.daylight_offset || 0;
        document.getElementById('ntp-interval').value = Math.round((data.sync_interval || 3600000) / 60000);

        // Update status
        document.getElementById('ntp-status').textContent = data.synced ? 'Synchronized' : 'Not Synced';
        document.getElementById('ntp-status').className = data.synced ? 'status-ok' : 'status-warn';
        document.getElementById('ntp-current-time').textContent = data.current_time || '--';
        document.getElementById('ntp-sync-count').textContent = data.status?.sync_count || 0;

        if (data.status?.last_sync_ago !== undefined) {
            document.getElementById('ntp-last-sync').textContent = formatUptime(data.status.last_sync_ago) + ' ago';
        } else {
            document.getElementById('ntp-last-sync').textContent = 'Never';
        }
    } catch (error) {
        console.error('Failed to load NTP config:', error);
    }
}

async function loadLCDConfig() {
    try {
        const response = await fetch('/api/lcd/config');
        const data = await response.json();

        // Update status display
        document.getElementById('lcd-status').textContent = data.available ? 'Active' : (data.enabled ? 'Enabled (Disconnected)' : 'Disabled');
        document.getElementById('lcd-status').className = data.available ? 'status-ok' : 'status-warn';
        document.getElementById('lcd-address-display').textContent = data.address_hex || ('0x' + data.address.toString(16).toUpperCase());
        document.getElementById('lcd-dimensions').textContent = data.cols + 'x' + data.rows;
        document.getElementById('lcd-pins').textContent = 'SDA=' + data.sda + ', SCL=' + data.scl;

        // Update form fields
        document.getElementById('lcd-enabled').checked = data.enabled;
        document.getElementById('lcd-address').value = data.address;
        document.getElementById('lcd-cols').value = data.cols;
        document.getElementById('lcd-rows').value = data.rows;
        document.getElementById('lcd-sda').value = data.sda;
        document.getElementById('lcd-scl').value = data.scl;
        document.getElementById('lcd-backlight').checked = data.backlight;
        document.getElementById('lcd-rotation').value = data.rotation_interval || 5;
    } catch (error) {
        console.error('Failed to load LCD config:', error);
    }
}

// Save configurations
async function saveLoRaConfig() {
    const config = {
        enabled: document.getElementById('lora-enabled').checked,
        frequency: parseInt(document.getElementById('lora-frequency').value),
        spreading_factor: parseInt(document.getElementById('lora-sf').value),
        bandwidth: parseFloat(document.getElementById('lora-bw').value),
        coding_rate: parseInt(document.getElementById('lora-cr').value),
        tx_power: parseInt(document.getElementById('lora-power').value),
        sync_word: parseInt(document.getElementById('lora-syncword').value)
    };

    try {
        const response = await fetch('/api/lora/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
        const result = await response.json();
        alert(result.message || 'Configuration saved');
    } catch (error) {
        alert('Failed to save configuration');
        console.error(error);
    }
}

async function saveServerConfig() {
    const config = {
        enabled: document.getElementById('server-enabled').checked,
        host: document.getElementById('server-host').value,
        port_up: parseInt(document.getElementById('server-port-up').value),
        port_down: parseInt(document.getElementById('server-port-down').value),
        description: document.getElementById('server-description').value,
        region: document.getElementById('server-region').value,
        latitude: parseFloat(document.getElementById('server-lat').value) || 0,
        longitude: parseFloat(document.getElementById('server-lon').value) || 0,
        altitude: parseInt(document.getElementById('server-alt').value) || 0
    };

    try {
        const response = await fetch('/api/server/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
        const result = await response.json();
        alert(result.message || 'Configuration saved');
    } catch (error) {
        alert('Failed to save configuration');
        console.error(error);
    }
}

async function saveWiFiConfig() {
    const config = {
        hostname: document.getElementById('wifi-hostname').value,
        ap_mode: document.getElementById('wifi-mode').value === 'true',
        ssid: document.getElementById('wifi-ssid').value,
        password: document.getElementById('wifi-password').value
    };

    try {
        const response = await fetch('/api/wifi/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
        const result = await response.json();
        alert(result.message || 'Configuration saved');
    } catch (error) {
        alert('Failed to save configuration');
        console.error(error);
    }
}

async function saveNTPConfig() {
    const config = {
        enabled: document.getElementById('ntp-enabled').checked,
        server1: document.getElementById('ntp-server1').value,
        server2: document.getElementById('ntp-server2').value,
        timezone_offset: parseInt(document.getElementById('ntp-timezone').value),
        daylight_offset: parseInt(document.getElementById('ntp-daylight').value),
        sync_interval: parseInt(document.getElementById('ntp-interval').value) * 60000
    };

    try {
        const response = await fetch('/api/ntp/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
        const result = await response.json();
        alert(result.message || 'Configuration saved');
    } catch (error) {
        alert('Failed to save configuration');
        console.error(error);
    }
}

async function saveLCDConfig() {
    const config = {
        enabled: document.getElementById('lcd-enabled').checked,
        address: parseInt(document.getElementById('lcd-address').value),
        cols: parseInt(document.getElementById('lcd-cols').value),
        rows: parseInt(document.getElementById('lcd-rows').value),
        sda: parseInt(document.getElementById('lcd-sda').value),
        scl: parseInt(document.getElementById('lcd-scl').value),
        backlight: document.getElementById('lcd-backlight').checked,
        rotation_interval: parseInt(document.getElementById('lcd-rotation').value)
    };

    try {
        const response = await fetch('/api/lcd/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
        const result = await response.json();
        alert(result.message || 'Configuration saved');
        loadLCDConfig();
    } catch (error) {
        alert('Failed to save configuration');
        console.error(error);
    }
}

async function toggleLCDBacklight() {
    const currentState = document.getElementById('lcd-backlight').checked;
    const newState = !currentState;

    const config = {
        backlight: newState
    };

    try {
        const response = await fetch('/api/lcd/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
        const result = await response.json();

        if (response.ok) {
            document.getElementById('lcd-backlight').checked = newState;
            addLog('LCD backlight ' + (newState ? 'ON' : 'OFF'));
        }
    } catch (error) {
        console.error('Failed to toggle backlight:', error);
    }
}

// NTP sync
async function syncNTP() {
    try {
        const response = await fetch('/api/ntp/sync', { method: 'POST' });
        const data = await response.json();

        if (data.success) {
            alert('Time synchronized: ' + data.current_time);
            loadNTPConfig();
            addLog('NTP synchronized');
        } else {
            alert('Sync failed: ' + (data.error || 'Unknown error'));
        }
    } catch (error) {
        alert('Sync failed');
        console.error(error);
    }
}

// WiFi scan
async function scanWiFi() {
    const resultsDiv = document.getElementById('wifi-scan-results');
    resultsDiv.innerHTML = '<p>Scanning...</p>';

    try {
        let response = await fetch('/api/wifi/scan');
        let data = await response.json();

        // Wait for scan to complete
        while (data.status === 'scanning') {
            await new Promise(resolve => setTimeout(resolve, 2000));
            response = await fetch('/api/wifi/scan');
            data = await response.json();
        }

        if (data.networks && data.networks.length > 0) {
            resultsDiv.innerHTML = data.networks.map(network => `
                <div class="wifi-network" onclick="selectNetwork('${network.ssid}')">
                    <span>${network.ssid} ${network.encryption ? '🔒' : ''}</span>
                    <span>${network.rssi} dBm</span>
                </div>
            `).join('');
        } else {
            resultsDiv.innerHTML = '<p>No networks found</p>';
        }
    } catch (error) {
        resultsDiv.innerHTML = '<p>Scan failed</p>';
        console.error(error);
    }
}

function selectNetwork(ssid) {
    document.getElementById('wifi-ssid').value = ssid;
    document.getElementById('wifi-mode').value = 'false';  // Switch to Station mode when selecting a network
    document.getElementById('wifi-scan-results').innerHTML = '';
}

// Reset statistics
async function resetStats() {
    try {
        await fetch('/api/stats/reset', { method: 'POST' });
        loadStats();
        addLog('Statistics reset');
    } catch (error) {
        console.error('Failed to reset stats:', error);
    }
}

// Restart device
async function restartDevice() {
    if (!confirm('Are you sure you want to restart the device?')) return;

    try {
        await fetch('/api/restart', { method: 'POST' });
        alert('Device is restarting...');
    } catch (error) {
        console.error('Failed to restart:', error);
    }
}

// Firmware upload
async function uploadFirmware() {
    const fileInput = document.getElementById('ota-file');
    const file = fileInput.files[0];

    if (!file) {
        alert('Please select a firmware file');
        return;
    }

    if (!file.name.endsWith('.bin')) {
        alert('Please select a .bin file');
        return;
    }

    const progressBar = document.getElementById('ota-progress');
    const progressFill = progressBar.querySelector('.progress-fill');
    progressBar.style.display = 'block';

    const formData = new FormData();
    formData.append('firmware', file);

    try {
        const xhr = new XMLHttpRequest();

        xhr.upload.onprogress = (e) => {
            if (e.lengthComputable) {
                const percent = (e.loaded / e.total) * 100;
                progressFill.style.width = percent + '%';
            }
        };

        xhr.onload = () => {
            if (xhr.status === 200) {
                alert('Firmware updated! Device will restart...');
            } else {
                const result = JSON.parse(xhr.responseText);
                alert('Upload failed: ' + (result.error || 'Unknown error'));
            }
            progressBar.style.display = 'none';
            progressFill.style.width = '0%';
        };

        xhr.onerror = () => {
            alert('Upload failed');
            progressBar.style.display = 'none';
            progressFill.style.width = '0%';
        };

        xhr.open('POST', '/api/ota');
        xhr.send(formData);
    } catch (error) {
        alert('Upload failed');
        console.error(error);
        progressBar.style.display = 'none';
    }
}

// Helper functions
function updateStatusIndicator(id, text, connected) {
    const element = document.getElementById(id);
    element.textContent = text;
    element.className = 'status-indicator ' + (connected ? 'connected' : 'disconnected');
}

function addLog(message) {
    const container = document.getElementById('log-container');
    const entry = document.createElement('div');
    entry.className = 'log-entry';

    const time = new Date().toLocaleTimeString();
    entry.innerHTML = `<span class="log-time">${time}</span>${message}`;

    container.insertBefore(entry, container.firstChild);

    // Keep only last 100 entries
    while (container.children.length > 100) {
        container.removeChild(container.lastChild);
    }
}

function formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;

    if (days > 0) {
        return `${days}d ${hours}h ${minutes}m`;
    } else if (hours > 0) {
        return `${hours}h ${minutes}m ${secs}s`;
    } else if (minutes > 0) {
        return `${minutes}m ${secs}s`;
    } else {
        return `${secs}s`;
    }
}

function formatBytes(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
}

// ============================================================
// File Manager Functions
// ============================================================

let currentFilePath = '/';
let currentFileList = [];
let currentEditFile = '';

// Refresh file list
async function refreshFiles() {
    try {
        const response = await fetch('/api/files/list?dir=' + encodeURIComponent(currentFilePath));
        const data = await response.json();

        currentFileList = data.files || [];
        const fileListEl = document.getElementById('fileList');
        fileListEl.innerHTML = '';

        // Add parent directory link if not at root
        if (currentFilePath !== '/') {
            const upItem = createFileItem('..', 0, true, true);
            fileListEl.appendChild(upItem);
        }

        // Add files and directories
        currentFileList.forEach(file => {
            const item = createFileItem(file.name, file.size, file.isDir);
            fileListEl.appendChild(item);
        });

        document.getElementById('currentPath').textContent = currentFilePath;
    } catch (error) {
        console.error('Failed to load files:', error);
        document.getElementById('fileList').innerHTML = '<p style="color: #f44336; padding: 20px;">Error loading files</p>';
    }
}

// Create file item element
function createFileItem(name, size, isDir, isUp = false) {
    const item = document.createElement('div');
    item.className = 'file-item' + (isDir ? ' directory' : '');

    const icon = isDir ? '&#128193;' : '&#128196;';
    const displayName = isUp ? '..' : name;
    const sizeStr = isDir ? '' : formatBytes(size);

    let actionsHtml = '';
    if (!isUp) {
        if (!isDir) {
            actionsHtml = `
                <button class="btn-sm btn-primary" onclick="editFile('${name}', event)">Edit</button>
                <button class="btn-sm btn-secondary" onclick="downloadFile('${name}', event)">Download</button>
                <button class="btn-sm btn-danger" onclick="deleteFile('${name}', false, event)">Delete</button>
            `;
        } else {
            actionsHtml = `
                <button class="btn-sm btn-danger" onclick="deleteFile('${name}', true, event)">Delete</button>
            `;
        }
    }

    item.innerHTML = `
        <div class="file-icon">${icon}</div>
        <div class="file-info">
            <span class="file-name">${displayName}</span>
            <span class="file-size">${sizeStr}</span>
        </div>
        <div class="file-actions">${actionsHtml}</div>
    `;

    if (isDir || isUp) {
        item.onclick = () => navigateTo(isUp ? '..' : name);
        item.style.cursor = 'pointer';
    }

    return item;
}

// Navigate to directory
function navigateTo(name) {
    if (name === '..') {
        const parts = currentFilePath.split('/').filter(p => p);
        parts.pop();
        currentFilePath = parts.length > 0 ? '/' + parts.join('/') : '/';
    } else {
        if (currentFilePath === '/') {
            currentFilePath = '/' + name;
        } else {
            currentFilePath = currentFilePath + '/' + name;
        }
    }
    refreshFiles();
}

// Download file
function downloadFile(name, event) {
    event.stopPropagation();
    const filepath = (currentFilePath + '/' + name).replace('//', '/');
    window.open('/api/files/download?file=' + encodeURIComponent(filepath), '_blank');
}

// Delete file or directory
async function deleteFile(name, isDir, event) {
    event.stopPropagation();
    const type = isDir ? 'directory' : 'file';
    if (!confirm(`Delete this ${type}?\n${name}`)) return;

    const filepath = (currentFilePath + '/' + name).replace('//', '/');
    const formData = new FormData();
    formData.append('file', filepath);

    try {
        const response = await fetch('/api/files/delete', {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            addLog('Deleted: ' + name);
            refreshFiles();
        } else {
            alert('Failed to delete');
        }
    } catch (error) {
        alert('Error: ' + error.message);
    }
}

// Create new folder
async function createFolder() {
    const name = prompt('Folder name:');
    if (!name) return;

    let dirpath = currentFilePath === '/' ? '/' + name : currentFilePath + '/' + name;

    const formData = new FormData();
    formData.append('dir', dirpath);

    try {
        const response = await fetch('/api/files/mkdir', {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            addLog('Created folder: ' + name);
            refreshFiles();
        } else {
            alert('Failed to create folder');
        }
    } catch (error) {
        alert('Error: ' + error.message);
    }
}

// Handle file upload
async function handleFileUpload(files) {
    for (const file of files) {
        await uploadSingleFile(file);
    }
}

// Upload single file
async function uploadSingleFile(file) {
    const fileExists = currentFileList.some(f => f.name === file.name && !f.isDir);

    if (fileExists) {
        if (!confirm(`File "${file.name}" already exists. Overwrite?`)) {
            return;
        }
    }

    const encodedPath = encodeURIComponent(currentFilePath);
    const uploadUrl = `/api/files/upload?dir=${encodedPath}`;

    const formData = new FormData();
    formData.append('file', file);

    const progressBar = document.getElementById('uploadProgress');
    const progressFill = document.getElementById('uploadProgressFill');
    progressBar.style.display = 'block';
    progressFill.style.width = '0%';

    try {
        const xhr = new XMLHttpRequest();

        xhr.upload.onprogress = (e) => {
            if (e.lengthComputable) {
                const percent = Math.round((e.loaded / e.total) * 100);
                progressFill.style.width = percent + '%';
            }
        };

        xhr.onload = () => {
            if (xhr.status === 200) {
                addLog('Uploaded: ' + file.name);
                setTimeout(() => {
                    progressBar.style.display = 'none';
                    refreshFiles();
                }, 500);
            } else {
                alert('Upload failed');
                progressBar.style.display = 'none';
            }
        };

        xhr.onerror = () => {
            alert('Upload error');
            progressBar.style.display = 'none';
        };

        xhr.open('POST', uploadUrl);
        xhr.send(formData);
    } catch (error) {
        alert('Error: ' + error.message);
        progressBar.style.display = 'none';
    }
}

// Edit file
async function editFile(name, event) {
    event.stopPropagation();
    const filepath = (currentFilePath + '/' + name).replace('//', '/');
    currentEditFile = filepath;

    try {
        const response = await fetch('/api/files/read?file=' + encodeURIComponent(filepath));
        const data = await response.json();

        if (data.error) {
            alert('Error: ' + data.error);
            return;
        }

        if (data.size > 51200) {
            alert('File too large to edit (max 50KB)');
            return;
        }

        document.getElementById('editorTitle').textContent = 'Edit: ' + name;
        document.getElementById('fileEditor').value = data.content;
        document.getElementById('editorModal').style.display = 'flex';
    } catch (error) {
        alert('Error loading file: ' + error.message);
    }
}

// Save file
async function saveFile() {
    const content = document.getElementById('fileEditor').value;
    const formData = new FormData();
    formData.append('file', currentEditFile);
    formData.append('content', content);

    try {
        const response = await fetch('/api/files/write', {
            method: 'POST',
            body: formData
        });

        const data = await response.json();

        if (data.error) {
            alert('Error: ' + data.error);
            return;
        }

        addLog('Saved: ' + currentEditFile);
        closeEditor();
        refreshFiles();
    } catch (error) {
        alert('Error saving file: ' + error.message);
    }
}

// Close editor modal
function closeEditor() {
    document.getElementById('editorModal').style.display = 'none';
    document.getElementById('fileEditor').value = '';
    currentEditFile = '';
}

// Close modal on outside click
window.addEventListener('click', (event) => {
    const modal = document.getElementById('editorModal');
    if (event.target === modal) {
        closeEditor();
    }
});

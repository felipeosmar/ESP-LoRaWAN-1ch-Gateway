// ESP32 LoRaWAN Gateway Web Interface

let ws = null;
let statusInterval = null;
let healthInterval = null;

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
    initTabs();
    initForms();
    connectWebSocket();
    loadStatus();
    loadConfigs();

    // Start periodic status updates
    statusInterval = setInterval(loadStatus, 5000);

    // Start health status updates when on network tab
    healthInterval = setInterval(() => {
        const networkTab = document.getElementById('network');
        if (networkTab && networkTab.classList.contains('active')) {
            loadNetworkHealth();
            loadNetworkStatus();
        }
    }, 3000);
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

            // Load network data when Network tab is activated
            if (tab.dataset.tab === 'network') {
                loadNetworkConfig();
                loadNetworkStatus();
                loadNetworkHealth();
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

    // Buzzer form
    document.getElementById('buzzer-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        await saveBuzzerConfig();
    });

    // GPS form
    document.getElementById('gps-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        await saveGPSConfig();
    });

    // RTC form
    document.getElementById('rtc-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        await saveRTCConfig();
    });

    // RTC set time form
    document.getElementById('rtc-settime-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        await setRTCManualTime();
    });

    // Network form
    document.getElementById('network-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        await saveNetworkConfig();
    });

    // Failover form (if separate)
    const failoverForm = document.getElementById('failover-form');
    if (failoverForm) {
        failoverForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            await saveFailoverConfig();
        });
    }

    // WiFi DHCP toggle to show/hide static IP fields
    const wifiDhcpCheckbox = document.getElementById('net-wifi-dhcp');
    if (wifiDhcpCheckbox) {
        wifiDhcpCheckbox.addEventListener('change', toggleWifiStaticFields);
        toggleWifiStaticFields();
    }

    // Ethernet DHCP toggle
    const ethDhcpCheckbox = document.getElementById('net-eth-dhcp');
    if (ethDhcpCheckbox) {
        ethDhcpCheckbox.addEventListener('change', toggleEthStaticFields);
        toggleEthStaticFields();
    }
}

// Toggle WiFi static IP fields based on DHCP setting
function toggleWifiStaticFields() {
    const useDhcp = document.getElementById('net-wifi-dhcp').checked;
    const staticFields = ['net-wifi-static-ip', 'net-wifi-gateway', 'net-wifi-subnet', 'net-wifi-dns'];

    staticFields.forEach(id => {
        const field = document.getElementById(id);
        if (field) {
            field.disabled = useDhcp;
            field.style.opacity = useDhcp ? '0.5' : '1';
        }
    });
}

// Toggle Ethernet static IP fields based on DHCP setting
function toggleEthStaticFields() {
    const useDhcp = document.getElementById('net-eth-dhcp').checked;
    const staticFields = ['net-eth-static-ip', 'net-eth-gateway', 'net-eth-subnet', 'net-eth-dns'];

    staticFields.forEach(id => {
        const field = document.getElementById(id);
        if (field) {
            field.disabled = useDhcp;
            field.style.opacity = useDhcp ? '0.5' : '1';
        }
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
    await loadNetworkConfig();
    await loadNTPConfig();
    await loadLCDConfig();
    await loadBuzzerConfig();
    await loadGPSConfig();
    await loadRTCConfig();
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

// ================== Buzzer Functions ==================

async function loadBuzzerConfig() {
    try {
        const response = await fetch('/api/buzzer/config');
        const data = await response.json();

        // Update status display
        document.getElementById('buzzer-status').textContent = data.available ? 'Active' : (data.enabled ? 'Enabled' : 'Disabled');
        document.getElementById('buzzer-status').className = data.available ? 'status-ok' : 'status-warn';
        document.getElementById('buzzer-pin').textContent = 'GPIO ' + data.pin;

        // Update form fields
        document.getElementById('buzzer-enabled').checked = data.enabled;
        document.getElementById('buzzer-startup-sound').checked = data.startup_sound;
        document.getElementById('buzzer-packet-rx').checked = data.packet_rx_sound;
        document.getElementById('buzzer-packet-tx').checked = data.packet_tx_sound;
        document.getElementById('buzzer-volume').value = data.volume || 75;
    } catch (error) {
        console.error('Failed to load Buzzer config:', error);
    }
}

async function saveBuzzerConfig() {
    const config = {
        enabled: document.getElementById('buzzer-enabled').checked,
        startup_sound: document.getElementById('buzzer-startup-sound').checked,
        packet_rx_sound: document.getElementById('buzzer-packet-rx').checked,
        packet_tx_sound: document.getElementById('buzzer-packet-tx').checked,
        volume: parseInt(document.getElementById('buzzer-volume').value)
    };

    try {
        const response = await fetch('/api/buzzer/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
        const result = await response.json();
        alert(result.message || 'Configuration saved');
        loadBuzzerConfig();
    } catch (error) {
        alert('Failed to save configuration');
        console.error(error);
    }
}

async function testBuzzerTone() {
    const frequency = parseInt(document.getElementById('buzzer-test-freq').value);
    const duration = parseInt(document.getElementById('buzzer-test-duration').value);

    try {
        const response = await fetch('/api/buzzer/test', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ action: 'tone', frequency, duration })
        });
        const result = await response.json();
        if (!result.success) {
            alert(result.error || 'Failed to play tone');
        }
    } catch (error) {
        console.error('Failed to test buzzer:', error);
    }
}

async function testBuzzerStartup() {
    try {
        const response = await fetch('/api/buzzer/test', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ action: 'startup' })
        });
        const result = await response.json();
        if (!result.success) {
            alert(result.error || 'Failed to play sound');
        }
    } catch (error) {
        console.error('Failed to test buzzer:', error);
    }
}

async function testBuzzerSuccess() {
    try {
        const response = await fetch('/api/buzzer/test', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ action: 'success' })
        });
        const result = await response.json();
        if (!result.success) {
            alert(result.error || 'Failed to play sound');
        }
    } catch (error) {
        console.error('Failed to test buzzer:', error);
    }
}

async function testBuzzerError() {
    try {
        const response = await fetch('/api/buzzer/test', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ action: 'error' })
        });
        const result = await response.json();
        if (!result.success) {
            alert(result.error || 'Failed to play sound');
        }
    } catch (error) {
        console.error('Failed to test buzzer:', error);
    }
}

async function stopBuzzer() {
    try {
        const response = await fetch('/api/buzzer/test', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ action: 'stop' })
        });
    } catch (error) {
        console.error('Failed to stop buzzer:', error);
    }
}

// ================== GPS Functions ==================

async function loadGPSConfig() {
    try {
        const response = await fetch('/api/gps/config');
        const data = await response.json();

        // Update status display
        if (data.enabled) {
            document.getElementById('gps-status').textContent = data.has_fix ? 'Active' : 'No Fix';
            document.getElementById('gps-status').className = data.has_fix ? 'status-ok' : 'status-warn';
        } else {
            document.getElementById('gps-status').textContent = 'Disabled';
            document.getElementById('gps-status').className = 'status-error';
        }

        document.getElementById('gps-fix').textContent = data.has_fix ? (data.use_fixed ? 'Fixed Location' : 'GPS Fix') : 'No Fix';
        document.getElementById('gps-satellites').textContent = data.satellites || 0;
        document.getElementById('gps-hdop').textContent = data.hdop ? data.hdop.toFixed(2) : '--';

        // Update position display
        document.getElementById('gps-latitude').textContent = data.latitude ? data.latitude.toFixed(6) + '\u00B0' : '--';
        document.getElementById('gps-longitude').textContent = data.longitude ? data.longitude.toFixed(6) + '\u00B0' : '--';
        document.getElementById('gps-altitude').textContent = data.altitude ? data.altitude + ' m' : '--';
        document.getElementById('gps-speed').textContent = data.speed ? data.speed.toFixed(1) + ' km/h' : '--';

        // Update form fields
        document.getElementById('gps-enabled').checked = data.enabled;
        document.getElementById('gps-use-fixed').checked = data.use_fixed;
        document.getElementById('gps-rx-pin').value = data.rx_pin || 16;
        document.getElementById('gps-tx-pin').value = data.tx_pin || 17;
        document.getElementById('gps-baud').value = data.baud_rate || 9600;
        document.getElementById('gps-fixed-lat').value = data.latitude || 0;
        document.getElementById('gps-fixed-lon').value = data.longitude || 0;
        document.getElementById('gps-fixed-alt').value = data.altitude || 0;
    } catch (error) {
        console.error('Failed to load GPS config:', error);
    }
}

async function saveGPSConfig() {
    const config = {
        enabled: document.getElementById('gps-enabled').checked,
        use_fixed: document.getElementById('gps-use-fixed').checked,
        rx_pin: parseInt(document.getElementById('gps-rx-pin').value),
        tx_pin: parseInt(document.getElementById('gps-tx-pin').value),
        baud_rate: parseInt(document.getElementById('gps-baud').value),
        latitude: parseFloat(document.getElementById('gps-fixed-lat').value),
        longitude: parseFloat(document.getElementById('gps-fixed-lon').value),
        altitude: parseInt(document.getElementById('gps-fixed-alt').value)
    };

    try {
        const response = await fetch('/api/gps/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
        const result = await response.json();
        alert(result.message || 'Configuration saved');
        loadGPSConfig();
    } catch (error) {
        alert('Failed to save configuration');
        console.error(error);
    }
}

// ================== RTC Functions ==================

async function loadRTCConfig() {
    try {
        const response = await fetch('/api/rtc/config');
        const data = await response.json();

        // Update form fields
        document.getElementById('rtc-enabled').checked = data.enabled;
        document.getElementById('rtc-address').value = data.i2cAddress || 104;
        document.getElementById('rtc-sda').value = data.sdaPin || 21;
        document.getElementById('rtc-scl').value = data.sclPin || 22;
        document.getElementById('rtc-sync-ntp').checked = data.syncWithNTP;
        document.getElementById('rtc-sync-interval').value = data.syncInterval || 3600;
        document.getElementById('rtc-sqw').value = data.squareWaveMode || 0;
        document.getElementById('rtc-timezone').value = data.timezoneOffset || -3;

        // Load status
        await loadRTCStatus();
    } catch (error) {
        console.error('Failed to load RTC config:', error);
    }
}

async function loadRTCStatus() {
    try {
        const response = await fetch('/api/rtc/status');
        const data = await response.json();

        // Update status display
        if (data.available) {
            document.getElementById('rtc-status').textContent = 'Active';
            document.getElementById('rtc-status').className = 'status-ok';
        } else {
            document.getElementById('rtc-status').textContent = data.enabled === false ? 'Disabled' : 'Not Found';
            document.getElementById('rtc-status').className = 'status-error';
        }

        document.getElementById('rtc-oscillator').textContent = data.oscillatorRunning ? 'Running' : 'Stopped';
        document.getElementById('rtc-oscillator').className = data.oscillatorRunning ? 'status-ok' : 'status-warn';

        document.getElementById('rtc-date').textContent = data.formattedDate || '--';
        document.getElementById('rtc-time').textContent = data.formattedTime || '--';

        if (data.currentTime && data.currentTime.dayName) {
            document.getElementById('rtc-day-of-week').textContent = data.currentTime.dayName;
        } else {
            document.getElementById('rtc-day-of-week').textContent = '--';
        }

        document.getElementById('rtc-read-count').textContent = data.readCount || 0;
        document.getElementById('rtc-write-count').textContent = data.writeCount || 0;
        document.getElementById('rtc-error-count').textContent = data.errorCount || 0;
        document.getElementById('rtc-error-count').className = data.errorCount > 0 ? 'status-warn' : '';

    } catch (error) {
        console.error('Failed to load RTC status:', error);
    }
}

async function saveRTCConfig() {
    const config = {
        enabled: document.getElementById('rtc-enabled').checked,
        i2cAddress: parseInt(document.getElementById('rtc-address').value),
        sdaPin: parseInt(document.getElementById('rtc-sda').value),
        sclPin: parseInt(document.getElementById('rtc-scl').value),
        syncWithNTP: document.getElementById('rtc-sync-ntp').checked,
        syncInterval: parseInt(document.getElementById('rtc-sync-interval').value),
        squareWaveMode: parseInt(document.getElementById('rtc-sqw').value),
        timezoneOffset: parseInt(document.getElementById('rtc-timezone').value)
    };

    try {
        const response = await fetch('/api/rtc/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
        const result = await response.json();
        alert(result.message || 'Configuration saved');
        loadRTCConfig();
    } catch (error) {
        alert('Failed to save configuration');
        console.error(error);
    }
}

async function syncRTCWithNTP() {
    try {
        const response = await fetch('/api/rtc/sync', { method: 'POST' });
        const data = await response.json();

        if (data.success) {
            alert('RTC synchronized with NTP:\n' + data.formattedDateTime);
            addLog('RTC synchronized with NTP');
            loadRTCStatus();
        } else {
            alert('Sync failed: ' + (data.error || 'Unknown error'));
        }
    } catch (error) {
        alert('Sync failed: ' + error.message);
        console.error(error);
    }
}

function setRTCFromBrowser() {
    const now = new Date();

    // Format date as YYYY-MM-DD for input type="date"
    const dateStr = now.getFullYear() + '-' +
        String(now.getMonth() + 1).padStart(2, '0') + '-' +
        String(now.getDate()).padStart(2, '0');

    // Format time as HH:MM:SS for input type="time"
    const timeStr = String(now.getHours()).padStart(2, '0') + ':' +
        String(now.getMinutes()).padStart(2, '0') + ':' +
        String(now.getSeconds()).padStart(2, '0');

    document.getElementById('rtc-set-date').value = dateStr;
    document.getElementById('rtc-set-time').value = timeStr;
}

async function setRTCManualTime() {
    const dateVal = document.getElementById('rtc-set-date').value;
    const timeVal = document.getElementById('rtc-set-time').value;

    if (!dateVal || !timeVal) {
        alert('Please enter both date and time');
        return;
    }

    const dateParts = dateVal.split('-');
    const timeParts = timeVal.split(':');

    const config = {
        year: parseInt(dateParts[0]),
        month: parseInt(dateParts[1]),
        day: parseInt(dateParts[2]),
        hours: parseInt(timeParts[0]),
        minutes: parseInt(timeParts[1]),
        seconds: parseInt(timeParts[2]) || 0
    };

    try {
        const response = await fetch('/api/rtc/settime', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
        const result = await response.json();

        if (result.success) {
            alert('Time set successfully:\n' + result.formattedDateTime);
            addLog('RTC time set manually');
            loadRTCStatus();
        } else {
            alert('Failed to set time: ' + (result.error || 'Unknown error'));
        }
    } catch (error) {
        alert('Failed to set time: ' + error.message);
        console.error(error);
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
                    <span>${network.ssid} ${network.encryption ? '\uD83D\uDD12' : ''}</span>
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

// ============================================================
// Network Manager Functions
// ============================================================

async function loadNetworkConfig() {
    try {
        const response = await fetch('/api/network/config');
        const data = await response.json();

        if (data.error) {
            console.log('Network Manager not available');
            return;
        }

        // Update form fields - Interface enable/disable
        document.getElementById('net-wifi-enabled').checked = data.wifi_enabled;
        document.getElementById('net-eth-enabled').checked = data.ethernet_enabled;

        // Failover configuration
        document.getElementById('net-primary').value = data.primary || 'wifi';
        document.getElementById('net-failover-enabled').checked = data.failover_enabled;
        document.getElementById('net-failover-timeout').value = data.failover_timeout || 30000;
        document.getElementById('net-reconnect-interval').value = data.reconnect_interval || 10000;

        // Stability period (new field)
        const stabilityField = document.getElementById('net-stability-period');
        if (stabilityField) {
            stabilityField.value = data.stability_period || 60000;
        }

        // WiFi config (including new static IP fields)
        if (data.wifi) {
            const wifiDhcp = document.getElementById('net-wifi-dhcp');
            if (wifiDhcp) {
                wifiDhcp.checked = data.wifi.dhcp !== false;
            }

            const wifiStaticIp = document.getElementById('net-wifi-static-ip');
            if (wifiStaticIp) {
                wifiStaticIp.value = data.wifi.static_ip || '';
            }

            const wifiGateway = document.getElementById('net-wifi-gateway');
            if (wifiGateway) {
                wifiGateway.value = data.wifi.gateway || '';
            }

            const wifiSubnet = document.getElementById('net-wifi-subnet');
            if (wifiSubnet) {
                wifiSubnet.value = data.wifi.subnet || '';
            }

            const wifiDns = document.getElementById('net-wifi-dns');
            if (wifiDns) {
                wifiDns.value = data.wifi.dns || '';
            }

            toggleWifiStaticFields();
        }

        // Ethernet config
        if (data.ethernet) {
            document.getElementById('net-eth-dhcp').checked = data.ethernet.dhcp !== false;
            document.getElementById('net-eth-static-ip').value = data.ethernet.static_ip || '';
            document.getElementById('net-eth-gateway').value = data.ethernet.gateway || '';
            document.getElementById('net-eth-subnet').value = data.ethernet.subnet || '';
            document.getElementById('net-eth-dns').value = data.ethernet.dns || '';
            document.getElementById('net-eth-dhcp-timeout').value = data.ethernet.dhcp_timeout || 10000;

            toggleEthStaticFields();
        }

        // Load status and health
        await loadNetworkStatus();
        await loadNetworkHealth();
    } catch (error) {
        console.error('Failed to load Network config:', error);
    }
}

async function loadNetworkStatus() {
    try {
        const response = await fetch('/api/network/status');
        const data = await response.json();

        if (data.error) {
            document.getElementById('net-active-interface').textContent = 'Not Available';
            return;
        }

        // Active interface info
        const activeInterface = data.activeInterface || 'None';
        document.getElementById('net-active-interface').textContent = activeInterface;
        document.getElementById('net-active-interface').className = data.connected ? 'status-ok' : 'status-warn';

        // Mode display
        let modeText = 'Auto';
        if (data.manualMode) {
            modeText = 'Manual (' + (data.forcedInterface || 'Unknown') + ')';
        }
        document.getElementById('net-mode').textContent = modeText;
        document.getElementById('net-ip').textContent = data.ip || '--';
        document.getElementById('net-gateway').textContent = data.gateway || '--';

        // Update override mode display
        updateOverrideModeDisplay(data.manualMode ? data.forcedInterface : 'auto');

        // Update interface indicators
        updateInterfaceIndicator('wifi', activeInterface === 'WiFi', data.wifi?.connected);
        updateInterfaceIndicator('eth', activeInterface === 'Ethernet', data.ethernet?.connected);

        // WiFi status
        if (data.wifi) {
            document.getElementById('net-wifi-status').textContent = data.wifi.connected ? 'Connected' : 'Disconnected';
            document.getElementById('net-wifi-status').className = data.wifi.connected ? 'status-ok' : 'status-warn';
            document.getElementById('net-wifi-ssid').textContent = data.wifi.ssid || '--';
            document.getElementById('net-wifi-rssi').textContent = data.wifi.rssi ? data.wifi.rssi + ' dBm' : '--';
            document.getElementById('net-wifi-ip').textContent = data.wifi.ip || '--';
        }

        // Ethernet status
        if (data.ethernet) {
            let ethStatus = 'Disabled';
            if (data.ethernet.connected) ethStatus = 'Connected';
            else if (data.ethernet.linkUp) ethStatus = 'Link Up';
            else if (data.ethernet.enabled) ethStatus = 'No Cable';

            document.getElementById('net-eth-status').textContent = ethStatus;
            document.getElementById('net-eth-status').className = data.ethernet.connected ? 'status-ok' :
                                                                   (data.ethernet.linkUp ? 'status-warn' : 'status-error');
            document.getElementById('net-eth-link').textContent = data.ethernet.linkUp ? 'Up' : 'Down';
            document.getElementById('net-eth-link').className = data.ethernet.linkUp ? 'status-ok' : 'status-error';
            document.getElementById('net-eth-ip').textContent = data.ethernet.ip || '--';
            document.getElementById('net-eth-mac').textContent = data.ethernet.mac || '--';
        }

        // Statistics
        if (data.stats) {
            document.getElementById('net-wifi-connects').textContent = data.stats.wifiConnections || 0;
            document.getElementById('net-eth-connects').textContent = data.stats.ethernetConnections || 0;
            document.getElementById('net-failovers').textContent = data.stats.failoverCount || 0;

            // Calculate active uptime
            let uptime = 0;
            if (activeInterface === 'WiFi') {
                uptime = data.stats.totalUptimeWifi || 0;
            } else if (activeInterface === 'Ethernet') {
                uptime = data.stats.totalUptimeEthernet || 0;
            }
            document.getElementById('net-uptime').textContent = formatUptime(Math.floor(uptime / 1000));
        }
    } catch (error) {
        console.error('Failed to load Network status:', error);
    }
}

/**
 * Load and display health check status
 */
async function loadNetworkHealth() {
    try {
        const response = await fetch('/api/network/health');
        const data = await response.json();

        if (data.error) {
            // Health endpoint may not exist in older firmware
            document.getElementById('net-health-status').textContent = 'N/A';
            return;
        }

        // Health status
        const healthStatusEl = document.getElementById('net-health-status');
        if (data.healthy) {
            healthStatusEl.textContent = 'Healthy';
            healthStatusEl.className = 'healthy';
        } else {
            healthStatusEl.textContent = 'Unhealthy';
            healthStatusEl.className = 'unhealthy';
        }

        // Last ACK time
        const lastAckEl = document.getElementById('net-last-ack');
        if (data.lastAckTime !== undefined) {
            if (data.lastAckTime === 0) {
                lastAckEl.textContent = 'Never';
            } else {
                const agoMs = Date.now() - data.lastAckTime;
                lastAckEl.textContent = formatUptime(Math.floor(agoMs / 1000)) + ' ago';
            }
        } else if (data.lastAckAgo !== undefined) {
            lastAckEl.textContent = formatUptime(Math.floor(data.lastAckAgo / 1000)) + ' ago';
        } else {
            lastAckEl.textContent = '--';
        }

        // Failover state
        const failoverStateEl = document.getElementById('net-failover-state');
        if (data.failoverActive) {
            failoverStateEl.textContent = 'Active (on backup)';
            failoverStateEl.className = 'status-warn';
        } else {
            failoverStateEl.textContent = 'Inactive';
            failoverStateEl.className = 'status-ok';
        }

        // Stability timer countdown
        const stabilityTimerEl = document.getElementById('net-stability-timer');
        if (data.failoverActive && data.primaryStableFor !== undefined) {
            const stabilityPeriod = data.stabilityPeriod || 60000;
            const remaining = Math.max(0, stabilityPeriod - data.primaryStableFor);
            if (remaining > 0) {
                stabilityTimerEl.textContent = formatUptime(Math.ceil(remaining / 1000)) + ' remaining';
                stabilityTimerEl.className = 'status-warn';
            } else {
                stabilityTimerEl.textContent = 'Ready to switch back';
                stabilityTimerEl.className = 'status-ok';
            }
        } else {
            stabilityTimerEl.textContent = '--';
            stabilityTimerEl.className = '';
        }

    } catch (error) {
        console.error('Failed to load Network health:', error);
        // Set defaults for missing endpoint
        document.getElementById('net-health-status').textContent = 'Unknown';
        document.getElementById('net-last-ack').textContent = '--';
        document.getElementById('net-failover-state').textContent = '--';
        document.getElementById('net-stability-timer').textContent = '--';
    }
}

/**
 * Update interface indicator visual state
 */
function updateInterfaceIndicator(type, isActive, isConnected) {
    const indicatorId = type === 'wifi' ? 'wifi-indicator' : 'eth-indicator';
    const labelId = type === 'wifi' ? 'wifi-active-label' : 'eth-active-label';

    const indicator = document.getElementById(indicatorId);
    const label = document.getElementById(labelId);

    if (!indicator || !label) return;

    // Remove all state classes
    indicator.classList.remove('active', 'standby', 'error');

    if (isActive) {
        indicator.classList.add('active');
        label.textContent = 'Active';
    } else if (isConnected) {
        indicator.classList.add('standby');
        label.textContent = 'Standby';
    } else {
        indicator.classList.add('error');
        label.textContent = 'Offline';
    }
}

/**
 * Update the override mode button states
 */
function updateOverrideModeDisplay(currentMode) {
    const modeSpan = document.getElementById('net-override-mode');
    const btnAuto = document.getElementById('btn-auto');
    const btnWifi = document.getElementById('btn-wifi');
    const btnEthernet = document.getElementById('btn-ethernet');

    // Update mode display text
    if (currentMode === 'auto') {
        modeSpan.textContent = 'Auto';
    } else if (currentMode === 'wifi') {
        modeSpan.textContent = 'Forced WiFi';
    } else if (currentMode === 'ethernet') {
        modeSpan.textContent = 'Forced Ethernet';
    } else {
        modeSpan.textContent = currentMode || 'Auto';
    }

    // Update button active states
    [btnAuto, btnWifi, btnEthernet].forEach(btn => {
        if (btn) btn.classList.remove('btn-active');
    });

    if (currentMode === 'wifi' && btnWifi) {
        btnWifi.classList.add('btn-active');
    } else if (currentMode === 'ethernet' && btnEthernet) {
        btnEthernet.classList.add('btn-active');
    } else if (btnAuto) {
        btnAuto.classList.add('btn-active');
    }
}

async function saveNetworkConfig() {
    const config = {
        wifi_enabled: document.getElementById('net-wifi-enabled').checked,
        ethernet_enabled: document.getElementById('net-eth-enabled').checked,
        primary: document.getElementById('net-primary').value,
        failover_enabled: document.getElementById('net-failover-enabled').checked,
        failover_timeout: parseInt(document.getElementById('net-failover-timeout').value),
        reconnect_interval: parseInt(document.getElementById('net-reconnect-interval').value),
        stability_period: parseInt(document.getElementById('net-stability-period')?.value || 60000),
        wifi: {
            dhcp: document.getElementById('net-wifi-dhcp')?.checked !== false,
            static_ip: document.getElementById('net-wifi-static-ip')?.value || '',
            gateway: document.getElementById('net-wifi-gateway')?.value || '',
            subnet: document.getElementById('net-wifi-subnet')?.value || '',
            dns: document.getElementById('net-wifi-dns')?.value || ''
        },
        ethernet: {
            dhcp: document.getElementById('net-eth-dhcp').checked,
            static_ip: document.getElementById('net-eth-static-ip').value,
            gateway: document.getElementById('net-eth-gateway').value,
            subnet: document.getElementById('net-eth-subnet').value,
            dns: document.getElementById('net-eth-dns').value,
            dhcp_timeout: parseInt(document.getElementById('net-eth-dhcp-timeout').value)
        }
    };

    try {
        const response = await fetch('/api/network/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
        const result = await response.json();

        if (result.status === 'ok') {
            alert('Network configuration saved');
            addLog('Network config saved');
            loadNetworkConfig();
        } else {
            alert('Failed to save: ' + (result.error || 'Unknown error'));
        }
    } catch (error) {
        alert('Failed to save configuration');
        console.error(error);
    }
}

/**
 * Save failover-specific configuration
 */
async function saveFailoverConfig() {
    const config = {
        primary: document.getElementById('net-primary').value,
        failover_enabled: document.getElementById('net-failover-enabled').checked,
        failover_timeout: parseInt(document.getElementById('net-failover-timeout').value),
        stability_period: parseInt(document.getElementById('net-stability-period')?.value || 60000),
        reconnect_interval: parseInt(document.getElementById('net-reconnect-interval').value)
    };

    try {
        const response = await fetch('/api/network/failover', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
        const result = await response.json();

        if (result.status === 'ok') {
            alert('Failover configuration saved');
            addLog('Failover config saved');
            loadNetworkConfig();
        } else {
            alert('Failed to save: ' + (result.error || 'Unknown error'));
        }
    } catch (error) {
        alert('Failed to save failover configuration');
        console.error(error);
    }
}

/**
 * Force interface selection (auto/wifi/ethernet)
 */
async function forceInterface(iface) {
    try {
        const response = await fetch('/api/network/force', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ interface: iface })
        });
        const result = await response.json();

        if (result.status === 'ok') {
            if (iface === 'auto') {
                addLog('Network: Switched to auto mode');
            } else {
                addLog('Network: Forced to ' + iface);
            }

            // Update button states immediately
            updateOverrideModeDisplay(iface);

            // Reload status after a short delay
            setTimeout(loadNetworkStatus, 1000);
        } else {
            alert('Failed: ' + (result.error || 'Unknown error'));
        }
    } catch (error) {
        alert('Failed to switch interface');
        console.error(error);
    }
}

async function networkReconnect() {
    try {
        const response = await fetch('/api/network/reconnect', { method: 'POST' });
        const result = await response.json();

        if (result.status === 'ok') {
            addLog('Network: Reconnecting...');
            setTimeout(loadNetworkStatus, 3000);
        } else {
            alert('Failed: ' + (result.error || 'Unknown error'));
        }
    } catch (error) {
        alert('Failed to reconnect');
        console.error(error);
    }
}

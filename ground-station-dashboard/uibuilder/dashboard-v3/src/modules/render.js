function el(id) {
  return document.getElementById(id)
}

function escapeHtml(value) {
  return String(value)
    .replaceAll('&', '&amp;')
    .replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;')
    .replaceAll('"', '&quot;')
}

function tlmEntry(state, target, parameter) {
  const tlm = state.payload.tlm || {}
  if (!tlm[target] || !tlm[target][parameter]) return { value: '', time: '' }
  return tlm[target][parameter]
}

function field(state, target, parameter, label) {
  const entry = tlmEntry(state, target, parameter)
  return { key: `${target}-${parameter}`, parameter, label, value: entry.value || '', time: entry.time || '' }
}

function unitSuffix(parameter) {
  if (parameter.endsWith('_MS2')) return 'm/s²'
  if (parameter.endsWith('_DPS')) return 'dps'
  if (parameter.endsWith('_UT')) return 'uT'
  if (parameter.endsWith('_KPH')) return 'kph'
  if (parameter.endsWith('_DEG')) return '°'
  if (parameter.endsWith('_V')) return 'V'
  if (parameter.endsWith('_A')) return 'A'
  if (parameter.endsWith('_P')) return '%'
  if (parameter.endsWith('_M')) return 'm'
  return ''
}

function receivedMs(item) {
  if (!item || !item.time) return null
  const parsed = Date.parse(item.time)
  return Number.isNaN(parsed) ? null : parsed
}

function isFresh(state, item) {
  if (!item || item.value === '' || item.value === null || typeof item.value === 'undefined') return false
  const parsed = receivedMs(item)
  if (parsed === null) return false
  return (state.nowMs - parsed) <= state.freshnessThresholdMs
}

function freshnessClass(state, item) {
  if (!item || item.value === '' || item.value === null || typeof item.value === 'undefined') return 'freshness-empty'
  return isFresh(state, item) ? 'freshness-fresh' : 'freshness-stale'
}

function displayValue(item) {
  if (!item || item.value === '' || item.value === null || typeof item.value === 'undefined') return '--'
  const unit = item.parameter ? unitSuffix(item.parameter) : ''
  return `${item.value}${unit}`
}

function formatSummary(entry, type) {
  if (!entry) return '--'
  if (type === 'ack') {
    if (!entry.target) return '--'
    return `${entry.time || ''} ${entry.target || ''} ${entry.value || ''}`.trim()
  }
  if (!entry.error) return '--'
  const detail = entry.contextDecoded || entry.context || ''
  return `${entry.time || ''} ${entry.error}${detail ? ` ${detail}` : ''}`.trim()
}

function lastTlmEntry(state) {
  const rows = state.payload.packetLog || []
  const row = rows.find((item) => item && item.packetType === 'TLM' && item.time)
  if (!row) return null
  return { value: row.time, time: row.time }
}

function gpsEntries(state) {
  return {
    latitude: tlmEntry(state, 'GPS', 'LATITUDE_D'),
    longitude: tlmEntry(state, 'GPS', 'LONGITUDE_D'),
    altitude: tlmEntry(state, 'GPS', 'ALTITUDE_M'),
    satellites: tlmEntry(state, 'GPS', 'SATELLITES_N'),
    available: tlmEntry(state, 'GPS', 'AVAILABLE'),
  }
}

function setVisualMetric(state, id, item) {
  const node = el(id)
  if (!node) return
  node.textContent = displayValue(item)
  node.className = `metric-value ${freshnessClass(state, item)}`
}

function gpsFix(state) {
  const entries = gpsEntries(state)
  if (entries.available.value !== 'TRUE') return null

  const lat = Number(entries.latitude.value)
  const lon = Number(entries.longitude.value)
  if (!Number.isFinite(lat) || !Number.isFinite(lon)) return null

  return {
    lat,
    lon,
    altitude: entries.altitude.value || '--',
    satellites: entries.satellites.value || '--',
  }
}

function buildMapBounds(fix, deltaLat = 0.02, deltaLon = 0.04) {
  const left = (fix.lon - deltaLon).toFixed(5)
  const right = (fix.lon + deltaLon).toFixed(5)
  const top = (fix.lat + deltaLat).toFixed(5)
  const bottom = (fix.lat - deltaLat).toFixed(5)
  return `${left}%2C${bottom}%2C${right}%2C${top}`
}

function updateGpsMap(state) {
  const status = el('gps-map-status')
  const frame = el('gps-map')
  if (!status || !frame) return

  const fix = gpsFix(state)
  if (!fix) {
    status.textContent = 'Awaiting fix'
    frame.removeAttribute('src')
    return
  }

  const src = `https://www.openstreetmap.org/export/embed.html?bbox=${buildMapBounds(fix)}&layer=mapnik&marker=${fix.lat.toFixed(5)}%2C${fix.lon.toFixed(5)}`
  if (frame.dataset.src !== src) {
    frame.src = src
    frame.dataset.src = src
  }

  status.textContent = `${fix.satellites} sats`
}

function optionMarkup(options, selected) {
  return options.map((option) => {
    const isSelected = option === selected ? ' selected' : ''
    return `<option value="${option}"${isSelected}>${option}</option>`
  }).join('')
}

function renderLogs(containerId, rows, emptyText) {
  const container = el(containerId)
  if (!rows || rows.length === 0) {
    container.innerHTML = `<div class="log-line freshness-empty">${emptyText}</div>`
    return
  }
  container.innerHTML = rows.map((row) => `<div class="log-line">${escapeHtml(row.raw || '')}</div>`).join('')
}

function renderSystems(state, systemConfigs) {
  const body = el('systems-body')
  body.innerHTML = systemConfigs.map((system) => {
    const fields = system.fields.map(([parameter, label]) => field(state, system.target, parameter, label))
    const fieldMarkup = fields.map((item) => {
      return `<div class="field-chip"><span class="field-label">${item.label}</span><span class="field-value ${freshnessClass(state, item)}" data-field-key="${item.key}">${escapeHtml(displayValue(item))}</span></div>`
    }).join('')

    const getSelect = `
      <div class="select-shell get-shell">
        <select class="mini-select get-select" data-role="get-select" data-target="${system.target}">
          ${optionMarkup(system.getOptions, state.getSelections[system.target])}
        </select>
        <span class="select-arrow">▼</span>
      </div>
    `

    const enableControls = system.target === 'RTC'
      ? '<div class="control-gap"></div><div class="control-gap"></div>'
      : `<button class="mini-btn cmd-green" data-role="enable" data-target="${system.target}" data-value="TRUE">EN</button>
         <button class="mini-btn cmd-red" data-role="enable" data-target="${system.target}" data-value="FALSE">DIS</button>`

    let modeControls = '<div class="control-gap"></div><div class="control-gap"></div>'
    if (system.target === 'LED') {
      modeControls = `<button class="mini-btn cmd-green" data-command="SET,LED,STATE,ON">ON</button>
        <button class="mini-btn cmd-red" data-command="SET,LED,STATE,OFF">OFF</button>`
    } else if (system.target === 'RTC') {
      modeControls = `<button class="mini-btn cmd-blue" data-role="rtc-now">NOW</button>
        <button class="mini-btn cmd-blue" data-command="SET,RTC,SYNC,GPS">GPS</button>`
    }

    let auxControls = '<div class="control-gap"></div><div class="control-gap"></div>'
    if (system.target === 'TELEMETRY') {
      auxControls = `<input class="mini-input inline-field" id="telemetry-interval" type="number" min="1" max="3600" value="${escapeHtml(state.telemetryInterval)}">
        <button class="mini-btn cmd-neutral" data-role="telemetry-interval">INT</button>`
    } else if (system.target === 'LED') {
      auxControls = `<div class="select-shell compact">
          <select class="mini-select compact" id="led-color-select">
            ${optionMarkup(['RED', 'GREEN', 'BLUE'], state.ledColor)}
          </select>
          <span class="select-arrow">▼</span>
        </div>
        <button class="mini-btn cmd-neutral" data-role="led-color">CLR</button>`
    }

    return `
      <div class="systems-row">
        <div class="col-system system-name">${system.title}</div>
        <div class="col-values values-wrap">${fieldMarkup}</div>
        <div class="col-actions">
          <div class="controls-grid">
            <button class="mini-btn cmd-purple" data-command="GET,${system.target},NONE,NONE">POL</button>
            ${enableControls}
            <button class="mini-btn cmd-green-soft" data-role="telemetry" data-target="${system.target}" data-value="ENABLE">T+</button>
            <button class="mini-btn cmd-red-soft" data-role="telemetry" data-target="${system.target}" data-value="DISABLE">T-</button>
            ${modeControls}
            ${auxControls}
            ${getSelect}
          </div>
        </div>
      </div>
    `
  }).join('')
}

function refreshSystemValues(state, systemConfigs) {
  systemConfigs.forEach((system) => {
    system.fields.forEach(([parameter, label]) => {
      const item = field(state, system.target, parameter, label)
      const node = document.querySelector(`[data-field-key="${item.key}"]`)
      if (!node) return
      node.textContent = displayValue(item)
      node.className = `field-value ${freshnessClass(state, item)}`
    })
  })
}

export function refreshDashboardStatus(state, systemConfigs) {
  const heartbeat = field(state, 'STATUS', 'HEARTBEAT_N', 'N')
  const lastTlm = lastTlmEntry(state)
  el('heartbeat-value').textContent = displayValue(heartbeat)
  el('heartbeat-value').className = `stat-value ${freshnessClass(state, heartbeat)}`

  el('last-tlm').textContent = displayValue(lastTlm)
  el('last-tlm').className = `stat-value ${freshnessClass(state, lastTlm)}`

  el('last-ack').textContent = formatSummary(state.payload.lastAck, 'ack')
  el('last-ack').className = `stat-value stat-wrap ${state.payload.lastAck ? 'freshness-fresh' : 'freshness-empty'}`

  el('last-err').textContent = formatSummary(state.payload.lastErr, 'err')
  el('last-err').className = `stat-value stat-wrap ${state.payload.lastErr ? 'freshness-stale' : 'freshness-empty'}`

  setVisualMetric(state, 'visual-altitude', field(state, 'GPS', 'ALTITUDE_M', 'ALT'))
  setVisualMetric(state, 'visual-speed', field(state, 'GPS', 'SPEED_KPH', 'SPD'))
  setVisualMetric(state, 'visual-roll', field(state, 'ADCS', 'ROLL_DEG', 'ROL'))
  setVisualMetric(state, 'visual-pitch', field(state, 'ADCS', 'PITCH_DEG', 'PIT'))

  refreshSystemValues(state, systemConfigs)
  updateGpsMap(state)
}

export function renderDashboard(state, systemConfigs) {
  if (!el('systems-body').children.length) {
    renderSystems(state, systemConfigs)
  }

  renderLogs('ack-log', state.payload.ackLog, 'No acknowledgements yet')
  renderLogs('err-log', state.payload.errorLog, 'No errors yet')
  renderLogs('packet-log', state.payload.packetLog, 'No packets yet')
  refreshDashboardStatus(state, systemConfigs)
}

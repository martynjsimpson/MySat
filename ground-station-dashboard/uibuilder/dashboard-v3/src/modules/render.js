function el(id) {
  return document.getElementById(id)
}

let lastValidGpsFix = null

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
  return {
    key: `${target}-${parameter}`,
    parameter,
    label,
    value: entry.value || '',
    time: entry.time || '',
    receivedAtMs: entry.receivedAtMs,
  }
}

function unitSuffix(parameter) {
  if (parameter.endsWith('_MS2')) return 'm/s²'
  if (parameter.endsWith('_DPS')) return 'dps'
  if (parameter.endsWith('_UT')) return 'uT'
  if (parameter.endsWith('_KPH')) return 'kph'
  if (parameter.endsWith('_C')) return '°C'
  if (parameter.endsWith('_DEG')) return '°'
  if (parameter.endsWith('_V')) return 'V'
  if (parameter.endsWith('_A')) return 'A'
  if (parameter.endsWith('_P')) return '%'
  if (parameter.endsWith('_M')) return 'm'
  return ''
}

function receivedMs(item) {
  if (!item) return null
  if (typeof item.receivedAtMs === 'number' && Number.isFinite(item.receivedAtMs)) {
    return item.receivedAtMs
  }
  if (!item.time) return null
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

function isGroundErrorRow(row) {
  const raw = String(row && row.raw ? row.raw : '')
  return raw.includes(',ERR,TIMEOUT') ||
    raw.includes(',ERR,LINK_DOWN') ||
    raw.includes(',ERR,RETRY_SEND_FAILED') ||
    raw.includes(',ERR,BAD_PARAMETER,GROUND') ||
    raw.includes('%2CGROUND%2C') ||
    raw.includes(',GROUND,')
}

function packetOrigin(row) {
  if (!row) return ''
  if (row.packetType === 'TLM' || row.packetType === 'ACK') {
    return row.target === 'GROUND' ? 'ground' : 'satellite'
  }
  if (row.packetType === 'ERR') {
    return isGroundErrorRow(row) ? 'ground' : 'satellite'
  }
  return ''
}

function lastPacketRow(state, predicate) {
  const rows = state.payload.packetLog || []
  return rows.find((item) => item && predicate(item)) || null
}

function lastTlmEntry(state, origin) {
  const row = lastPacketRow(state, (item) => item.packetType === 'TLM' && packetOrigin(item) === origin && item.time)
  if (!row) return null
  return { value: row.time, time: row.time, receivedAtMs: row.receivedAtMs }
}

function ackSummary(state, origin) {
  const row = lastPacketRow(state, (item) => item.packetType === 'ACK' && packetOrigin(item) === origin)
  if (!row) return '--'
  return `${row.time || ''} ${row.target || ''} ${row.value || ''}`.trim()
}

function errSummary(state, origin) {
  const row = lastPacketRow(state, (item) => item.packetType === 'ERR' && packetOrigin(item) === origin)
  if (!row) return '--'
  return `${row.time || ''} ${row.value || ''}`.trim()
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

function setVisualStatus(state, id, item, text) {
  const node = el(id)
  if (!node) return
  node.textContent = text
  node.className = `metric-value ${freshnessClass(state, item)}`
}

function setVisualFixedMetric(id, item) {
  const node = el(id)
  if (!node) return
  node.textContent = displayValue(item)
  node.className = 'metric-value'
}

function setLinkMetric(state, id, item) {
  const node = el(id)
  if (!node) return
  node.textContent = displayValue(item)
  node.className = `field-value ${freshnessClass(state, item)}`
}

function setLinkDot(id, healthy) {
  const node = el(id)
  if (!node) return
  node.classList.toggle('is-healthy', healthy)
  node.classList.toggle('is-unhealthy', !healthy)
}

function setAttitudeReadout(state, id, item) {
  const node = el(id)
  if (!node) return
  node.textContent = displayValue(item)
  node.className = `attitude-readout-value ${freshnessClass(state, item)}`
}

function updateAttitudeIndicator(state) {
  const horizon = el('attitude-horizon')
  const status = el('attitude-status')

  const roll = field(state, 'ADCS', 'ROLL_DEG', 'ROL')
  const pitch = field(state, 'ADCS', 'PITCH_DEG', 'PIT')
  const heading = field(state, 'ADCS', 'HEADING_DEG', 'HDG')
  const yaw = field(state, 'ADCS', 'YAW_RATE_DPS', 'YAW')

  setAttitudeReadout(state, 'attitude-heading', heading)
  setAttitudeReadout(state, 'attitude-roll', roll)
  setAttitudeReadout(state, 'attitude-pitch', pitch)
  setAttitudeReadout(state, 'attitude-yaw', yaw)

  const rollValue = Number(roll.value)
  const pitchValue = Number(pitch.value)
  const attitudeFresh = isFresh(state, roll) || isFresh(state, pitch) || isFresh(state, heading)

  if (status) {
    status.textContent = attitudeFresh ? 'Live' : 'Stale'
  }

  if (!horizon || !Number.isFinite(rollValue) || !Number.isFinite(pitchValue)) {
    return
  }

  const limitedPitch = Math.max(-45, Math.min(45, pitchValue))
  const pitchOffset = (limitedPitch / 45) * 52
  horizon.setAttribute('transform', `translate(0 ${pitchOffset.toFixed(2)}) rotate(${(-rollValue).toFixed(2)} 110 110)`)
}

function gpsFix(state) {
  const entries = gpsEntries(state)
  if (entries.available.value !== 'TRUE') return null

  const lat = Number(entries.latitude.value)
  const lon = Number(entries.longitude.value)
  if (!Number.isFinite(lat) || !Number.isFinite(lon)) return null
  if (lat === 0 && lon === 0) return null

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

  const liveFix = gpsFix(state)
  if (liveFix) {
    lastValidGpsFix = liveFix
  }

  const fix = liveFix || lastValidGpsFix
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

  status.textContent = liveFix ? `${fix.satellites} sats` : 'Last fix'
}

function updateLinkPanel(state) {
  const groundRadio = field(state, 'GROUND', 'RADIO', 'RAD')
  const groundPending = field(state, 'GROUND', 'PENDING', 'PND')
  const groundTx = field(state, 'GROUND', 'TX_PACKETS_N', 'TX')
  const groundRx = field(state, 'GROUND', 'RX_PACKETS_N', 'RX')
  const groundDrop = field(state, 'GROUND', 'DROP_PACKETS_N', 'DRP')
  const groundRetry = field(state, 'GROUND', 'LAST_RETRY_N', 'RTY')
  const groundDropReason = field(state, 'GROUND', 'LAST_DROP_REASON', 'RSN')

  setLinkMetric(state, 'link-ground-radio', groundRadio)
  setLinkMetric(state, 'link-ground-pending', groundPending)
  setLinkMetric(state, 'link-ground-tx', groundTx)
  setLinkMetric(state, 'link-ground-rx', groundRx)
  setLinkMetric(state, 'link-ground-drop', groundDrop)
  setLinkMetric(state, 'link-ground-retry', groundRetry)
  setLinkMetric(state, 'link-ground-drop-reason', groundDropReason)
  setLinkDot('link-ground-dot', groundRadio.value === 'READY')

  const satTelemetryEnable = field(state, 'TELEMETRY', 'ENABLE', 'EN')
  const satInterval = field(state, 'TELEMETRY', 'INTERVAL_S', 'INT')
  const satSkipped = field(state, 'TELEMETRY', 'SKIPPED_N', 'SKP')
  const satSkipReason = field(state, 'TELEMETRY', 'LAST_SKIP_REASON', 'RSN')

  setLinkMetric(state, 'link-sat-telemetry-enable', satTelemetryEnable)
  setLinkMetric(state, 'link-sat-interval', satInterval)
  setLinkMetric(state, 'link-sat-skipped', satSkipped)
  setLinkMetric(state, 'link-sat-skip-reason', satSkipReason)
  setLinkDot('link-sat-dot', satTelemetryEnable.value === 'TRUE')
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
    const scope = system.target === 'GROUND' ? 'ground' : 'satellite'
    const fields = system.fields.map(([parameter, label]) => field(state, system.target, parameter, label))
    const fieldMarkup = fields.map((item) => {
      return `<div class="field-chip"><span class="field-label">${item.label}</span><span class="field-value ${freshnessClass(state, item)}" data-field-key="${item.key}">${escapeHtml(displayValue(item))}</span></div>`
    }).join('')

    const getSelect = `
      <div class="select-shell get-shell">
        <select class="mini-select get-select" data-role="get-select" data-target="${system.target}" data-command-scope="${scope}">
          ${optionMarkup(system.getOptions, state.getSelections[system.target])}
        </select>
        <span class="select-arrow">▼</span>
      </div>
    `

    const enableControls = system.target === 'RTC' || system.target === 'GROUND' || system.target === 'MODE'
      ? '<div class="control-gap"></div><div class="control-gap"></div>'
      : `<button class="mini-btn cmd-green" data-role="enable" data-target="${system.target}" data-value="TRUE" data-command-scope="${scope}">EN</button>
         <button class="mini-btn cmd-red" data-role="enable" data-target="${system.target}" data-value="FALSE" data-command-scope="${scope}">DIS</button>`

    let modeControls = '<div class="control-gap"></div><div class="control-gap"></div>'
    if (system.target === 'GROUND') {
      modeControls = `<button class="mini-btn cmd-blue" data-role="ground-now" data-command-scope="${scope}">NOW</button>
        <div class="control-gap"></div>`
    } else if (system.target === 'RTC') {
      modeControls = `<button class="mini-btn cmd-blue" data-role="rtc-now" data-command-scope="${scope}">NOW</button>
        <button class="mini-btn cmd-blue" data-command="SET,RTC,SYNC,GPS" data-command-scope="${scope}">GPS</button>`
    }

    let auxControls = '<div class="control-gap"></div><div class="control-gap"></div>'
    if (system.target === 'TELEMETRY') {
      auxControls = `<input class="mini-input inline-field" id="telemetry-interval" type="number" min="1" max="3600" value="${escapeHtml(state.telemetryInterval)}" data-command-scope="${scope}">
        <button class="mini-btn cmd-neutral" data-role="telemetry-interval" data-command-scope="${scope}">INT</button>`
    }

    const telemetryControls = `<button class="mini-btn cmd-green-soft" data-role="telemetry" data-target="${system.target}" data-value="ENABLE" data-command-scope="${scope}">T+</button>
         <button class="mini-btn cmd-red-soft" data-role="telemetry" data-target="${system.target}" data-value="DISABLE" data-command-scope="${scope}">T-</button>`

    return `
      <div class="systems-row">
        <div class="col-system system-name">${system.title}</div>
        <div class="col-values values-wrap">${fieldMarkup}</div>
        <div class="col-actions">
          <div class="controls-grid">
            <button class="mini-btn cmd-purple" data-command="GET,${system.target},NONE,NONE" data-command-scope="${scope}">POL</button>
            ${enableControls}
            ${telemetryControls}
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
  const groundHeartbeat = field(state, 'GROUND', 'HEARTBEAT_N', 'N')
  const satHeartbeat = field(state, 'STATUS', 'HEARTBEAT_N', 'N')
  const groundLastTlm = lastTlmEntry(state, 'ground')
  const satLastTlm = lastTlmEntry(state, 'satellite')
  const groundAck = ackSummary(state, 'ground')
  const satAck = ackSummary(state, 'satellite')
  const groundErr = errSummary(state, 'ground')
  const satErr = errSummary(state, 'satellite')

  el('ground-heartbeat-value').textContent = displayValue(groundHeartbeat)
  el('ground-heartbeat-value').className = `stat-value ${freshnessClass(state, groundHeartbeat)}`

  el('ground-last-tlm').textContent = displayValue(groundLastTlm)
  el('ground-last-tlm').className = `stat-value ${freshnessClass(state, groundLastTlm)}`

  el('ground-last-ack').textContent = groundAck
  el('ground-last-ack').className = `stat-value stat-wrap ${groundAck !== '--' ? 'freshness-fresh' : 'freshness-empty'}`

  el('ground-last-err').textContent = groundErr
  el('ground-last-err').className = `stat-value stat-wrap ${groundErr !== '--' ? 'freshness-stale' : 'freshness-empty'}`

  el('sat-heartbeat-value').textContent = displayValue(satHeartbeat)
  el('sat-heartbeat-value').className = `stat-value ${freshnessClass(state, satHeartbeat)}`

  el('sat-last-tlm').textContent = displayValue(satLastTlm)
  el('sat-last-tlm').className = `stat-value ${freshnessClass(state, satLastTlm)}`

  el('sat-last-ack').textContent = satAck
  el('sat-last-ack').className = `stat-value stat-wrap ${satAck !== '--' ? 'freshness-fresh' : 'freshness-empty'}`

  el('sat-last-err').textContent = satErr
  el('sat-last-err').className = `stat-value stat-wrap ${satErr !== '--' ? 'freshness-stale' : 'freshness-empty'}`

  const gpsAvailable = field(state, 'GPS', 'AVAILABLE', 'AVL')
  setVisualFixedMetric('visual-altitude', field(state, 'GPS', 'ALTITUDE_M', 'ALT'))
  setVisualFixedMetric('visual-speed', field(state, 'GPS', 'SPEED_KPH', 'SPD'))
  setVisualStatus(state, 'visual-gps-status', gpsAvailable, gpsAvailable.value === 'TRUE' ? 'Fix' : 'Lost')
  const gpsStatusNode = el('visual-gps-status')
  if (gpsStatusNode) {
    gpsStatusNode.className = `metric-value${gpsAvailable.value === 'TRUE' ? '' : ' freshness-stale'}`
  }
  setVisualFixedMetric('visual-temperature', field(state, 'THERMAL', 'TEMPERATURE_C', 'TMP'))

  refreshSystemValues(state, systemConfigs)
  updateLinkPanel(state)
  updateGpsMap(state)
  updateAttitudeIndicator(state)
}

export function applyBusyControlState(state) {
  document.querySelectorAll('[data-command-scope]').forEach((node) => {
    const scope = node.dataset.commandScope
    const pending = state.commandUi && state.commandUi[scope]
    const disabled = Boolean(pending && pending.active)
    node.disabled = disabled
    node.classList.toggle('is-disabled', disabled)
  })
}

export function renderDashboard(state, systemConfigs) {
  if (!el('systems-body').children.length) {
    renderSystems(state, systemConfigs)
  }

  renderLogs('ack-log', state.payload.ackLog, 'No acknowledgements yet')
  renderLogs('err-log', state.payload.errorLog, 'No errors yet')
  renderLogs('packet-log', state.payload.packetLog, 'No packets yet')
  refreshDashboardStatus(state, systemConfigs)
  applyBusyControlState(state)
}

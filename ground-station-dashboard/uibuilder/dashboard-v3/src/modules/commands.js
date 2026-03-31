function el(id) {
  return document.getElementById(id)
}

let nextCommandControlId = 1

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

function expectedResponseTypes(command) {
  const [type = '', target = ''] = String(command || '').trim().split(',', 4)
  const scope = target === 'GROUND' ? 'ground' : 'satellite'

  if (type === 'GET') {
    return { scope, resolveOn: ['TLM', 'ERR'] }
  }

  if (type === 'SET' || type === 'PING' || type === 'RESET') {
    return { scope, resolveOn: ['ACK', 'ERR'] }
  }

  return { scope, resolveOn: ['ACK', 'ERR'] }
}

function ensureControlId(node) {
  if (!node) return ''
  if (!node.dataset.commandControlId) {
    node.dataset.commandControlId = `cmd-${nextCommandControlId++}`
  }
  return node.dataset.commandControlId
}

function setPendingCommand(state, command, sourceNode) {
  if (!state || !state.commandUi) return
  const payload = String(command || '').trim()
  if (!payload) return

  const { scope, resolveOn } = expectedResponseTypes(payload)
  state.commandUi[scope] = {
    active: true,
    sentAtMs: Date.now(),
    resolveOn,
    controlId: ensureControlId(sourceNode),
  }
}

export function reconcileCommandBusyState(state) {
  if (!state || !state.commandUi) return

  const packetLog = state.payload && Array.isArray(state.payload.packetLog)
    ? state.payload.packetLog
    : []

  ;['ground', 'satellite'].forEach((scope) => {
    const pending = state.commandUi[scope]
    if (!pending || !pending.active) return

    if ((state.nowMs - pending.sentAtMs) > state.commandTimeoutMs) {
      pending.active = false
      return
    }

    const resolved = packetLog.some((row) => {
      if (!row || packetOrigin(row) !== scope) return false
      if (typeof row.receivedAtMs !== 'number' || row.receivedAtMs < pending.sentAtMs) return false
      return pending.resolveOn.includes(row.packetType)
    })

    if (resolved) {
      pending.active = false
    }
  })
}

export function updateCommandBusyState(state) {
  if (!state || !state.commandUi) return

  document.querySelectorAll('[data-command-scope]').forEach((node) => {
    const scope = node.dataset.commandScope
    const pending = state.commandUi[scope]
    const controlId = ensureControlId(node)
    const disabled = Boolean(pending && pending.active && pending.controlId === controlId)
    node.disabled = disabled
    node.classList.toggle('is-disabled', disabled)
  })
}

export function sendCommand(command, state, sourceNode = null) {
  const payload = String(command || '').trim()
  if (!payload) return
  setPendingCommand(state, payload, sourceNode)
  updateCommandBusyState(state)
  window.uibuilder.send({ payload: `${payload}\n` })
}

export function requestStateSync() {
  window.uibuilder.sendCtrl({ topic: 'state-sync-request' })
}

export function updateConnection(connected) {
  const dot = el('conn-dot')
  const label = el('conn-label')
  dot.classList.remove('connected', 'stale', 'idle')
  dot.classList.add(connected ? 'connected' : 'stale')
  label.textContent = connected ? 'Node-RED Connected' : 'Node-RED Disconnected'
}

export function bindPanelToggles() {
  document.body.addEventListener('click', (event) => {
    const toggle = event.target.closest('[data-panel-toggle]')
    if (!toggle) return

    const panel = toggle.closest('[data-panel]')
    if (!panel) return

    const collapsed = panel.classList.toggle('is-collapsed')
    toggle.setAttribute('aria-expanded', String(!collapsed))
    toggle.textContent = collapsed ? 'Expand' : 'Collapse'
  })
}

export function updateSatelliteStatus(mode) {
  const dot = el('sat-dot')
  const label = el('sat-label')
  dot.classList.remove('connected', 'stale', 'idle')

  if (mode === 'live') {
    dot.classList.add('connected')
    label.textContent = 'Satellite Live'
    return
  }

  if (mode === 'stale') {
    dot.classList.add('stale')
    label.textContent = 'Satellite Stale'
    return
  }

  dot.classList.add('idle')
  label.textContent = 'Satellite Unknown'
}

export function bindEvents(state) {
  document.body.addEventListener('click', (event) => {
    const button = event.target.closest('button')
    if (!button) return

    if (button.dataset.command) {
      sendCommand(button.dataset.command, state, button)
      return
    }

    if (button.dataset.role === 'custom-send') {
      const input = el(button.dataset.input)
      if (!input) return
      sendCommand(input.value, state, button)
      input.value = ''
      return
    }

    if (button.dataset.role === 'enable') {
      sendCommand(`SET,${button.dataset.target},ENABLE,${button.dataset.value}`, state, button)
      return
    }

    if (button.dataset.role === 'telemetry') {
      sendCommand(`SET,${button.dataset.target},TELEMETRY,${button.dataset.value}`, state, button)
      return
    }

    if (button.dataset.role === 'telemetry-interval') {
      const input = el('telemetry-interval')
      state.telemetryInterval = String(input.value || '').trim() || state.telemetryInterval
      sendCommand(`SET,TELEMETRY,INTERVAL_S,${state.telemetryInterval}`, state, button)
      return
    }

    if (button.dataset.role === 'ground-now') {
      const iso = new Date().toISOString().replace(/\.\d{3}Z$/, 'Z')
      sendCommand(`SET,GROUND,CURRENT_TIME,${iso}`, state, button)
      return
    }

    if (button.dataset.role === 'ground-reset') {
      sendCommand('RESET,GROUND,NONE,NONE', state, button)
      return
    }

    if (button.dataset.role === 'rtc-now') {
      const iso = new Date().toISOString().replace(/\.\d{3}Z$/, 'Z')
      sendCommand(`SET,RTC,CURRENT_TIME,${iso}`, state, button)
    }
  })

  document.body.addEventListener('change', (event) => {
    const target = event.target
    if (target.matches('[data-role="get-select"]')) {
      state.getSelections[target.dataset.target] = target.value
      if (target.dataset.target === 'MODE') {
        sendCommand(`SET,MODE,STATE,${target.value}`, state, target)
        return
      }
      sendCommand(`GET,${target.dataset.target},${target.value},NONE`, state, target)
      return
    }

    if (target.id === 'telemetry-interval') {
      state.telemetryInterval = target.value
    }
  })

  document.querySelectorAll('#ground-custom-command, #sat-custom-command').forEach((input) => {
    input.addEventListener('keydown', (event) => {
      if (event.key !== 'Enter') return
      sendCommand(event.currentTarget.value, state, event.currentTarget)
      event.currentTarget.value = ''
    })
  })
}

function el(id) {
  return document.getElementById(id)
}

export function sendCommand(command) {
  const payload = String(command || '').trim()
  if (!payload) return
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
      sendCommand(button.dataset.command)
      return
    }

    if (button.dataset.role === 'custom-send') {
      const input = el(button.dataset.input)
      if (!input) return
      sendCommand(input.value)
      input.value = ''
      return
    }

    if (button.dataset.role === 'enable') {
      sendCommand(`SET,${button.dataset.target},ENABLE,${button.dataset.value}`)
      return
    }

    if (button.dataset.role === 'telemetry') {
      sendCommand(`SET,${button.dataset.target},TELEMETRY,${button.dataset.value}`)
      return
    }

    if (button.dataset.role === 'telemetry-interval') {
      const input = el('telemetry-interval')
      state.telemetryInterval = String(input.value || '').trim() || state.telemetryInterval
      sendCommand(`SET,TELEMETRY,INTERVAL_S,${state.telemetryInterval}`)
      return
    }

    if (button.dataset.role === 'ground-now') {
      const iso = new Date().toISOString().replace(/\.\d{3}Z$/, 'Z')
      sendCommand(`SET,GROUND,CURRENT_TIME,${iso}`)
      return
    }

    if (button.dataset.role === 'ground-reset') {
      sendCommand('RESET,GROUND,NONE,NONE')
      return
    }

    if (button.dataset.role === 'rtc-now') {
      const iso = new Date().toISOString().replace(/\.\d{3}Z$/, 'Z')
      sendCommand(`SET,RTC,CURRENT_TIME,${iso}`)
    }
  })

  document.body.addEventListener('change', (event) => {
    const target = event.target
    if (target.matches('[data-role="get-select"]')) {
      state.getSelections[target.dataset.target] = target.value
      sendCommand(`GET,${target.dataset.target},${target.value},NONE`)
      return
    }

    if (target.id === 'telemetry-interval') {
      state.telemetryInterval = target.value
    }
  })

  document.querySelectorAll('#ground-custom-command, #sat-custom-command').forEach((input) => {
    input.addEventListener('keydown', (event) => {
      if (event.key !== 'Enter') return
      sendCommand(event.currentTarget.value)
      event.currentTarget.value = ''
    })
  })
}

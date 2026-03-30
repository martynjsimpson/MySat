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
  dot.classList.toggle('connected', connected)
  label.textContent = connected ? 'Connected' : 'Disconnected'
}

export function bindEvents(state) {
  document.body.addEventListener('click', (event) => {
    const button = event.target.closest('button')
    if (!button) return

    if (button.dataset.command) {
      sendCommand(button.dataset.command)
      return
    }

    if (button.id === 'custom-send') {
      const input = el('custom-command')
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

    if (button.dataset.role === 'led-color') {
      const select = el('led-color-select')
      state.ledColor = select.value
      sendCommand(`SET,LED,COLOR,${state.ledColor}`)
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

    if (target.id === 'led-color-select') {
      state.ledColor = target.value
      return
    }

    if (target.id === 'telemetry-interval') {
      state.telemetryInterval = target.value
    }
  })

  el('custom-command').addEventListener('keydown', (event) => {
    if (event.key !== 'Enter') return
    sendCommand(event.currentTarget.value)
    event.currentTarget.value = ''
  })
}

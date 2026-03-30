import { systemConfigs } from './modules/config.js'
import { createState, stateActions } from './modules/state.js'
import { bindEvents, bindPanelToggles, requestStateSync, updateConnection, updateSatelliteStatus } from './modules/commands.js'
import { refreshDashboardStatus, renderDashboard } from './modules/render.js'

const state = createState()

function satelliteMode() {
  const tlm = state.payload.tlm || {}
  const heartbeat = tlm.STATUS && tlm.STATUS.HEARTBEAT_N ? tlm.STATUS.HEARTBEAT_N : null
  if (!heartbeat || !heartbeat.time) return 'idle'

  const parsed = Date.parse(heartbeat.time)
  if (Number.isNaN(parsed)) return 'idle'
  return (state.nowMs - parsed) <= state.freshnessThresholdMs ? 'live' : 'stale'
}

function applyPayload(payload) {
  if (!payload || typeof payload !== 'object') return
  stateActions.setPayload(state, payload)
  renderDashboard(state, systemConfigs)
  updateSatelliteStatus(satelliteMode())
}

function init() {
  bindEvents(state)
  bindPanelToggles()
  const startDashboard = () => {
    renderDashboard(state, systemConfigs)
    updateSatelliteStatus(satelliteMode())

    window.uibuilder.start()

    window.uibuilder.onChange('ioConnected', (connected) => {
      const isConnected = Boolean(connected)
      updateConnection(isConnected)
      if (isConnected) requestStateSync()
    })

    window.uibuilder.onChange('msg', (msg) => {
      if (!msg || !msg.payload || typeof msg.payload !== 'object') return
      applyPayload(msg.payload)
    })

    window.setInterval(() => {
      state.nowMs = Date.now()
      refreshDashboardStatus(state, systemConfigs)
      updateSatelliteStatus(satelliteMode())
    }, 1000)
  }

  if (document.readyState === 'complete') {
    startDashboard()
  } else {
    window.addEventListener('load', startDashboard, { once: true })
  }
}

window.addEventListener('DOMContentLoaded', init)

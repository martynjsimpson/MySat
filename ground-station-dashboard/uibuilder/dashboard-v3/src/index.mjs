import { systemConfigs } from './modules/config.js'
import { createState, stateActions } from './modules/state.js'
import { bindEvents, requestStateSync, updateConnection } from './modules/commands.js'
import { renderDashboard } from './modules/render.js'

const state = createState()

function applyPayload(payload) {
  if (!payload || typeof payload !== 'object') return
  stateActions.setPayload(state, payload)
  renderDashboard(state, systemConfigs)
}

function init() {
  bindEvents(state)
  renderDashboard(state, systemConfigs)

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
    renderDashboard(state, systemConfigs)
  }, 1000)
}

window.addEventListener('DOMContentLoaded', init)

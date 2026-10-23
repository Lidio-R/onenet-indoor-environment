const db = require('../../utils/database.js')

/**
 * Promise wrapper for wx.request
 */
function wxRequest(options) {
  return new Promise((resolve, reject) => {
    wx.request({
      ...options,
      success: (res) => resolve(res),
      fail: (err) => reject(err)
    })
  })
}

Page({
  data: {
    light: '--',
    pir: '--',
    buzzer: '--',
    lastUpdateTime: '--',
    isLoading: false,
    errorMsg: '',
    deviceStatus: 'offline',
    lightDevice: 'addp_01',
    humanDevice: 'addp_02',
    buzzerDevice: 'addp_03',
    productId: '',
    commandInput: '',
    isSending: false,
    lightStatus: 'off',
    ledMode: 'auto',
    buzzerEnabled: false,
    isTurningOn: false,
    isTurningOff: false,
    isBuzzerOn: false,
    isBuzzerOff: false,
    sensors: [
      { key: 'light', label: '光照强度', unit: 'lux', icon: '☀️', status: 'normal', bgGradient: 'linear-gradient(135deg, #fa709a 0%, #fee140 100%)', device: '' },
      { key: 'pir', label: '人体感应', unit: '', icon: '👤', status: 'normal', bgGradient: 'linear-gradient(135deg, #667eea 0%, #764ba2 100%)', device: '' },
      { key: 'buzzer', label: '蜂鸣器', unit: '', icon: '🔊', status: 'normal', bgGradient: 'linear-gradient(135deg, #f093fb 0%, #f5576c 100%)', device: '' }
    ],
    historyData: [],
    maxHistoryLength: 50,
    chartData: {
      lightPoints: [],
      pirPoints: [],
      labels: []
    },
    activeChart: 'light',
    chartStartIndex: 0,
    visiblePointsCount: 12,
    touchStartX: 0,
    touchStartIndex: 0,
    isTouching: false,
    showDetailModal: false,
    currentSensor: {},
    currentSensorValue: '--'
  },

  onLoad() {
    this.loadConfig()
    this.fetchData()
    this.startRefreshTimer()
  },

  onUnload() {
    if (this.timer) {
      clearInterval(this.timer)
    }
  },

  loadConfig() {
    const config = wx.getStorageSync('deviceConfig')
    if (config) {
      this.setData({
        lightDevice: config.lightDevice || 'addp_01',
        humanDevice: config.humanDevice || 'addp_02',
        buzzerDevice: config.buzzerDevice || 'addp_03',
        productId: config.productId || ''
      })
    }
  },

  startRefreshTimer() {
    this.timer = setInterval(() => {
      if (!this.isFetching) {
        this.fetchData()
      }
    }, 10000)
  },

  /**
   * Query device properties from OneNET IoT platform.
   * Uses GET /thingmodel/query-device-property with authorization header.
   */
  async queryDevice(deviceName, accessKey, productId) {
    console.log('Querying device:', deviceName)
    try {
      const res = await wxRequest({
        url: 'https://iot-api.heclouds.com/thingmodel/query-device-property',
        method: 'GET',
        header: {
          'Content-Type': 'application/json',
          'authorization': accessKey
        },
        data: {
          product_id: productId,
          device_name: deviceName
        },
        timeout: 8000
      })
      return { deviceName, res }
    } catch (err) {
      console.error('Query failed for', deviceName, ':', err)
      return { deviceName, res: null, error: err }
    }
  },

  async fetchData() {
    if (this.isFetching) {
      console.log('Previous request still pending, skipping...')
      return
    }

    this.isFetching = true
    this.setData({ isLoading: true })

    const config = wx.getStorageSync('deviceConfig')
    if (!config) {
      this.setData({
        errorMsg: '请先配置设备',
        deviceStatus: 'offline',
        isLoading: false
      })
      this.isFetching = false
      return
    }

    const { productId, accessKey, lightDevice, humanDevice, buzzerDevice } = config

    if (!productId || !accessKey) {
      this.setData({
        errorMsg: '配置不完整，请检查产品ID和AccessKey',
        deviceStatus: 'offline',
        isLoading: false
      })
      this.isFetching = false
      return
    }

    console.log('=== Fetch Data Start ===')
    console.log('Product:', productId)
    console.log('Devices:', lightDevice, humanDevice, buzzerDevice)

    // Query all three devices in parallel
    const queries = []
    if (lightDevice) queries.push(this.queryDevice(lightDevice, accessKey, productId))
    if (humanDevice) queries.push(this.queryDevice(humanDevice, accessKey, productId))
    if (buzzerDevice) queries.push(this.queryDevice(buzzerDevice, accessKey, productId))

    try {
      const results = await Promise.all(queries)
      console.log('All queries completed:', results.length)

      let onlineCount = 0
      let allErrors = []

      results.forEach(({ deviceName, res, error }) => {
        if (error || !res) {
          allErrors.push(deviceName + ': 网络错误')
          return
        }
        if (res.statusCode === 200 && res.data && res.data.code === 0) {
          onlineCount++
        } else {
          const msg = res.data ? (res.data.msg || res.data.message || 'HTTP ' + res.statusCode) : 'HTTP ' + res.statusCode
          allErrors.push(deviceName + ': ' + msg)
        }
      })

      this.setData({
        deviceStatus: onlineCount > 0 ? 'online' : 'offline',
        errorMsg: allErrors.length > 0 ? allErrors.join('; ') : ''
      })

      // Parse data from all devices
      this.parseAllDeviceData(results, { lightDevice, humanDevice, buzzerDevice })

    } catch (err) {
      console.error('Fetch all failed:', err)
      this.setData({
        errorMsg: '数据获取异常',
        deviceStatus: 'offline'
      })
    }

    this.isFetching = false
    this.setData({ isLoading: false })
    console.log('=== Fetch Data Complete ===')
  },

  parseAllDeviceData(results, deviceNames) {
    let light = '--'
    let pir = '--'
    let buzzer = '--'
    let buzzerEnabled = false

    results.forEach(({ deviceName, res, error }) => {
      if (error || !res || res.statusCode !== 200 || !res.data || res.data.code !== 0) {
        return
      }

      const data = res.data.data
      if (!data) return

      // data can be array or object depending on API version
      const properties = Array.isArray(data) ? data : [data]

      if (deviceName === deviceNames.lightDevice) {
        properties.forEach(item => {
          if (item.identifier === 'light' || (typeof item === 'object' && 'light' in item)) {
            const val = item.value !== undefined ? item.value : item.light
            light = this.formatValue(val)
          }
        })
      } else if (deviceName === deviceNames.humanDevice) {
        properties.forEach(item => {
          if (item.identifier === 'Pir' || (typeof item === 'object' && 'Pir' in item)) {
            const val = item.value !== undefined ? item.value : item.Pir
            pir = (val === true || val === 'true' || val === 1) ? '有人' : '无人'
          }
        })
      } else if (deviceName === deviceNames.buzzerDevice) {
        properties.forEach(item => {
          if (item.identifier === 'buzzer_switch' || (typeof item === 'object' && 'buzzer_switch' in item)) {
            const val = item.value !== undefined ? item.value : item.buzzer_switch
            buzzerEnabled = (val === true || val === 'true' || val === 1)
            buzzer = buzzerEnabled ? '已触发' : '未触发'
          }
        })
      }
    })

    // Auto buzzer status based on light threshold (scene linkage)
    const lightNum = parseFloat(light)
    if (!isNaN(lightNum) && lightNum >= 3000) {
      buzzer = buzzer === '--' ? '已触发(联动)' : buzzer
    }

    const now = new Date()
    const timeStr = `${now.getFullYear()}-${(now.getMonth() + 1).toString().padStart(2, '0')}-${now.getDate().toString().padStart(2, '0')} ${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}`
    const timeLabel = `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}`

    this.updateHistoryData(light, pir, timeLabel)

    this.setData({
      light,
      pir,
      buzzer,
      buzzerEnabled,
      lastUpdateTime: timeStr
    })
  },

  updateHistoryData(light, pir, timeLabel) {
    const { historyData, maxHistoryLength } = this.data

    const newRecord = {
      light: parseFloat(light) || null,
      pir: pir === '有人' ? 1 : (pir === '无人' ? 0 : null),
      time: timeLabel
    }

    const updatedHistory = [...historyData, newRecord]
    if (updatedHistory.length > maxHistoryLength) {
      updatedHistory.shift()
    }

    const lightPoints = updatedHistory.map(item => item.light)
    const pirPoints = updatedHistory.map(item => item.pir)
    const labels = updatedHistory.map(item => item.time)

    this.setData({
      historyData: updatedHistory,
      chartData: {
        lightPoints,
        pirPoints,
        labels
      }
    })

    this.updateChartYLabels()
    this.drawChart()

    this.saveToDatabase(light, pir)
  },

  async saveToDatabase(light, pir) {
    const { lightDevice } = this.data
    try {
      await db.addSensorData({
        deviceId: lightDevice || 'unknown',
        light: parseFloat(light) || null,
        pir: pir,
        timestamp: Date.now()
      })
    } catch (error) {
      console.error('Failed to save data:', error)
    }
  },

  formatValue(value) {
    if (typeof value === 'number') {
      return value.toFixed(1)
    }
    return String(value)
  },

  onRefresh() {
    if (!this.isFetching) {
      this.fetchData()
    }
  },

  /* ──────────── 光照灯控制 ──────────── */

  turnLightOn() {
    if (this.data.isTurningOn) return
    this.setData({ isTurningOn: true })
    this.sendLightCommand(true)
    setTimeout(() => {
      this.setData({ isTurningOn: false })
    }, 1000)
  },

  turnLightOff() {
    if (this.data.isTurningOff) return
    this.setData({ isTurningOff: true })
    this.sendLightCommand(false)
    setTimeout(() => {
      this.setData({ isTurningOff: false })
    }, 1000)
  },

  setAutoMode() {
    this.setData({ ledMode: 'auto', lightStatus: 'off' })
    wx.showToast({ title: '已切换到自动模式', icon: 'success' })
    // Also send led=false to switch STM32 to auto mode
    this.sendLightCommand(false)
  },

  sendLightCommand(ledOn) {
    const config = wx.getStorageSync('deviceConfig')
    if (!config) {
      wx.showToast({ title: '请先配置设备', icon: 'none' })
      return
    }

    const { productId, accessKey, lightDevice } = config
    const deviceName = lightDevice || 'addp_01'

    console.log('Sending LED command to', deviceName, ':', ledOn)

    wx.request({
      url: 'https://iot-api.heclouds.com/thingmodel/set-device-property',
      method: 'POST',
      header: {
        'Content-Type': 'application/json',
        'authorization': accessKey
      },
      data: {
        product_id: productId,
        device_name: deviceName,
        params: {
          led: ledOn
        }
      },
      timeout: 8000,
      success: (res) => {
        console.log('LED command response:', res.data)
        if (res.statusCode === 200 && res.data && res.data.code === 0) {
          this.setData({
            lightStatus: ledOn ? 'on' : 'off',
            ledMode: 'manual'
          })
          wx.showToast({
            title: ledOn ? '灯已打开' : '灯已关闭',
            icon: 'success'
          })
        } else {
          wx.showToast({
            title: '控制失败: ' + (res.data ? (res.data.msg || res.data.message || '') : ''),
            icon: 'none'
          })
        }
      },
      fail: (err) => {
        console.error('LED command failed:', err)
        wx.showToast({ title: '网络请求失败', icon: 'none' })
      }
    })
  },

  /* ──────────── 蜂鸣器控制 ──────────── */

  turnBuzzerOn() {
    if (this.data.isBuzzerOn) return
    this.setData({ isBuzzerOn: true })
    this.sendBuzzerCommand(true)
    setTimeout(() => {
      this.setData({ isBuzzerOn: false })
    }, 1000)
  },

  turnBuzzerOff() {
    if (this.data.isBuzzerOff) return
    this.setData({ isBuzzerOff: true })
    this.sendBuzzerCommand(false)
    setTimeout(() => {
      this.setData({ isBuzzerOff: false })
    }, 1000)
  },

  sendBuzzerCommand(buzzerOn) {
    const config = wx.getStorageSync('deviceConfig')
    if (!config) {
      wx.showToast({ title: '请先配置设备', icon: 'none' })
      return
    }

    const { productId, accessKey, buzzerDevice } = config
    const deviceName = buzzerDevice || 'addp_03'

    console.log('Sending Buzzer command to', deviceName, ':', buzzerOn)

    wx.request({
      url: 'https://iot-api.heclouds.com/thingmodel/set-device-property',
      method: 'POST',
      header: {
        'Content-Type': 'application/json',
        'authorization': accessKey
      },
      data: {
        product_id: productId,
        device_name: deviceName,
        params: {
          buzzer_switch: buzzerOn
        }
      },
      timeout: 8000,
      success: (res) => {
        console.log('Buzzer command response:', res.data)
        if (res.statusCode === 200 && res.data && res.data.code === 0) {
          this.setData({
            buzzerEnabled: buzzerOn,
            buzzer: buzzerOn ? '已触发' : '未触发'
          })
          wx.showToast({
            title: buzzerOn ? '蜂鸣器已开启' : '蜂鸣器已关闭',
            icon: 'success'
          })
        } else {
          wx.showToast({
            title: '控制失败: ' + (res.data ? (res.data.msg || res.data.message || '') : ''),
            icon: 'none'
          })
        }
      },
      fail: (err) => {
        console.error('Buzzer command failed:', err)
        wx.showToast({ title: '网络请求失败', icon: 'none' })
      }
    })
  },

  /* ──────────── 命令下发 ──────────── */

  onCommandInput(e) {
    this.setData({ commandInput: e.detail.value })
  },

  sendCommand() {
    const { commandInput, productId, lightDevice } = this.data
    if (!commandInput.trim()) {
      wx.showToast({ title: '请输入命令', icon: 'none' })
      return
    }

    const config = wx.getStorageSync('deviceConfig')
    if (!config) {
      wx.showToast({ title: '请先配置设备', icon: 'none' })
      return
    }

    this.setData({ isSending: true })

    wx.request({
      url: 'https://iot-api.heclouds.com/thingmodel/set-device-property',
      method: 'POST',
      header: {
        'Content-Type': 'application/json',
        'authorization': config.accessKey
      },
      data: {
        product_id: config.productId,
        device_name: config.lightDevice || 'addp_01',
        params: {
          cmd: commandInput.trim()
        }
      },
      timeout: 8000,
      success: (res) => {
        if (res.statusCode === 200 && res.data && res.data.code === 0) {
          wx.showToast({ title: '命令发送成功', icon: 'success' })
          this.setData({ commandInput: '' })
        } else {
          wx.showToast({
            title: '发送失败: ' + (res.data ? (res.data.msg || res.data.message || '') : ''),
            icon: 'none'
          })
        }
      },
      fail: () => {
        wx.showToast({ title: '网络请求失败', icon: 'none' })
      },
      complete: () => {
        this.setData({ isSending: false })
      }
    })
  },

  /* ──────────── 图表绘制 ──────────── */

  updateChartYLabels() {
    const { chartData, activeChart } = this.data
    let allValues = []

    if (activeChart === 'light') {
      allValues = [...chartData.lightPoints]
    } else if (activeChart === 'all') {
      allValues = [...chartData.lightPoints, ...chartData.pirPoints]
    }

    const validValues = allValues.filter(v => v !== null && !isNaN(v))
    if (validValues.length === 0) {
      this.setData({ chartYLabels: ['0', '1000', '2000', '3000', '4000'] })
      return
    }

    const min = Math.min(...validValues)
    const max = Math.max(...validValues)
    const range = max - min || 1
    const padding = range * 0.1

    const yMin = Math.floor((min - padding) / 10) * 10
    const yMax = Math.ceil((max + padding) / 10) * 10
    const yStep = Math.ceil((yMax - yMin) / 4 / 10) * 10 || 10

    const yLabels = []
    for (let i = 0; i <= 4; i++) {
      yLabels.push(String(Math.round(yMin + yStep * i)))
    }

    this.setData({ chartYLabels: yLabels })
  },

  drawChart() {
    const { chartData, chartStartIndex, visiblePointsCount, activeChart } = this.data
    const canvas = wx.createCanvasContext('chartCanvas')

    canvas.clearRect(0, 0, 375, 200)

    const padding = { top: 20, right: 20, bottom: 30, left: 45 }
    const chartWidth = 375 - padding.left - padding.right
    const chartHeight = 200 - padding.top - padding.bottom

    const getVisibleData = (data) => {
      const endIndex = Math.min(chartStartIndex + visiblePointsCount, data.length)
      return data.slice(chartStartIndex, endIndex)
    }

    let pointsToDraw = []
    let allValues = []

    if (activeChart === 'light') {
      pointsToDraw = [
        { data: getVisibleData(chartData.lightPoints), color: '#ffd93d', name: '光照' }
      ]
      allValues = [...getVisibleData(chartData.lightPoints)]
    } else if (activeChart === 'all') {
      pointsToDraw = [
        { data: getVisibleData(chartData.lightPoints), color: '#ffd93d', name: '光照' },
        { data: getVisibleData(chartData.pirPoints), color: '#667eea', name: '人体感应' }
      ]
      allValues = [...getVisibleData(chartData.lightPoints), ...getVisibleData(chartData.pirPoints)]
    }

    const validValues = allValues.filter(v => v !== null && !isNaN(v))
    if (validValues.length === 0) {
      canvas.draw()
      return
    }

    const minVal = Math.min(...validValues)
    const maxVal = Math.max(...validValues)
    const valueRange = maxVal - minVal || 1

    canvas.setStrokeStyle('rgba(255, 255, 255, 0.1)')
    canvas.setLineWidth(1)
    for (let i = 0; i <= 4; i++) {
      const y = padding.top + (chartHeight / 4) * i
      canvas.beginPath()
      canvas.moveTo(padding.left, y)
      canvas.lineTo(375 - padding.right, y)
      canvas.stroke()
    }

    pointsToDraw.forEach(({ data, color }) => {
      if (data.length < 2) return

      canvas.setStrokeStyle(color)
      canvas.setLineWidth(2)
      canvas.beginPath()

      data.forEach((value, index) => {
        if (value === null || isNaN(value)) return

        const x = padding.left + (chartWidth / (data.length - 1)) * index
        const normalizedValue = (value - minVal) / valueRange
        const y = padding.top + chartHeight * (1 - normalizedValue)

        if (index === 0) {
          canvas.moveTo(x, y)
        } else {
          canvas.lineTo(x, y)
        }
      })
      canvas.stroke()
    })

    const visibleLabels = chartData.labels.slice(chartStartIndex, chartStartIndex + visiblePointsCount)
    canvas.setFillStyle('rgba(255, 255, 255, 0.5)')
    canvas.setFontSize(10)
    visibleLabels.forEach((label, index) => {
      if (index % 2 === 0) {
        const x = padding.left + (chartWidth / (visibleLabels.length - 1)) * index - 15
        canvas.fillText(label, x, 200 - 10)
      }
    })

    canvas.draw()
  },

  onChartTouchStart(e) {
    this.setData({
      isTouching: true,
      touchStartX: e.touches[0].x,
      touchStartIndex: this.data.chartStartIndex
    })
  },

  onChartTouchMove(e) {
    if (!this.data.isTouching) return

    const { touchStartX, touchStartIndex, chartData, visiblePointsCount } = this.data
    const deltaX = e.touches[0].x - touchStartX
    const sensitivity = 3
    const indexChange = Math.round(deltaX / sensitivity)

    const maxIndex = Math.max(0, chartData.labels.length - visiblePointsCount)
    let newIndex = touchStartIndex - indexChange
    newIndex = Math.max(0, Math.min(maxIndex, newIndex))

    if (newIndex !== this.data.chartStartIndex) {
      this.setData({ chartStartIndex: newIndex })
      this.drawChart()
    }
  },

  onChartTouchEnd() {
    this.setData({ isTouching: false })
  },

  switchChart(e) {
    const type = e.currentTarget.dataset.type
    this.setData({ activeChart: type })
    this.updateChartYLabels()
    this.drawChart()
  },

  viewSensorDetail(e) {
    const sensorKey = e.currentTarget.dataset.sensor
    const { sensors, light, pir, buzzer, chartData } = this.data

    const sensor = sensors.find(s => s.key === sensorKey)
    let value = '--'

    if (sensorKey === 'light') value = light
    else if (sensorKey === 'pir') value = pir
    else if (sensorKey === 'buzzer') value = buzzer

    this.setData({
      showDetailModal: true,
      currentSensor: sensor || {},
      currentSensorValue: value
    })
  },

  closeDetailModal() {
    this.setData({ showDetailModal: false })
  },

  /* ──────────── 导航 ──────────── */

  goToConfig() {
    wx.navigateTo({ url: '/pages/config/config' })
  },

  goToHistory() {
    wx.navigateTo({ url: '/pages/logs/logs' })
  }
})

const db = require('../../utils/database.js')

Page({
  data: {
    productId: '',
    accessKey: '',
    lightDevice: 'addp_01',
    humanDevice: 'addp_02',
    buzzerDevice: 'addp_03',
    savedConfig: null,
    isTesting: false,
    testResult: '',
    configId: null,
    configs: []
  },

  onLoad() {
    this.loadSavedConfig()
    this.loadAllConfigs()
  },

  async loadSavedConfig() {
    const saved = wx.getStorageSync('deviceConfig')
    if (saved) {
      this.setData({ savedConfig: saved })
    }
    
    try {
      const configs = await db.getAllConfigs()
      if (configs.length > 0) {
        const latest = configs[configs.length - 1]
        this.setData({
          savedConfig: latest,
          configId: latest.id,
          productId: latest.productId || '',
          accessKey: latest.accessKey || latest.token || '',
          lightDevice: latest.lightDevice || 'addp_01',
          humanDevice: latest.humanDevice || 'addp_02',
          buzzerDevice: latest.buzzerDevice || 'addp_03'
        })
      }
    } catch (error) {
      console.error('Failed to load configs:', error)
    }
  },

  async loadAllConfigs() {
    try {
      const configs = await db.getAllConfigs()
      this.setData({ configs })
    } catch (error) {
      console.error('Failed to load configs:', error)
    }
  },

  onProductIdInput(e) {
    this.setData({ productId: e.detail.value })
  },

  onAccessKeyInput(e) {
    this.setData({ accessKey: e.detail.value })
  },

  onLightDeviceInput(e) {
    this.setData({ lightDevice: e.detail.value })
  },

  onHumanDeviceInput(e) {
    this.setData({ humanDevice: e.detail.value })
  },

  onBuzzerDeviceInput(e) {
    this.setData({ buzzerDevice: e.detail.value })
  },

  async saveConfig() {
    const { productId, accessKey, lightDevice, humanDevice, buzzerDevice, configId } = this.data
    if (!productId || !accessKey) {
      wx.showToast({ title: '请填写产品ID和AccessKey', icon: 'none' })
      return
    }
    if (!lightDevice && !humanDevice && !buzzerDevice) {
      wx.showToast({ title: '请至少填写一个设备名称', icon: 'none' })
      return
    }

    const config = {
      productId,
      accessKey,
      lightDevice,
      humanDevice,
      buzzerDevice,
      savedAt: new Date().toLocaleString()
    }

    wx.setStorageSync('deviceConfig', config)

    try {
      if (configId) {
        await db.updateConfig(configId, { productId, accessKey, lightDevice, humanDevice, buzzerDevice })
      } else {
        const id = await db.addConfig({ productId, accessKey, lightDevice, humanDevice, buzzerDevice })
        this.setData({ configId: id })
      }
      await this.loadAllConfigs()
      wx.showToast({ title: '配置保存成功', icon: 'success' })
    } catch (error) {
      console.error('Failed to save config:', error)
      wx.showToast({ title: '保存到数据库失败', icon: 'none' })
    }

    this.setData({ savedConfig: config })
  },

  clearConfig() {
    wx.showModal({
      title: '确认清除',
      content: '确定要清除已保存的配置吗？',
      success: async (res) => {
        if (res.confirm) {
          wx.removeStorageSync('deviceConfig')
          
          try {
            if (this.data.configId) {
              await db.deleteConfig(this.data.configId)
            }
            await this.loadAllConfigs()
          } catch (error) {
            console.error('Failed to delete config:', error)
          }
          
          this.setData({
            savedConfig: null,
            productId: '',
            accessKey: '',
            lightDevice: 'addp_01',
            humanDevice: 'addp_02',
            buzzerDevice: 'addp_03',
            configId: null
          })
          wx.showToast({ title: '已清除', icon: 'success' })
        }
      }
    })
  },

  async selectConfig(config) {
    this.setData({
      productId: config.productId || '',
      accessKey: config.accessKey || config.token || '',
      lightDevice: config.lightDevice || 'addp_01',
      humanDevice: config.humanDevice || 'addp_02',
      buzzerDevice: config.buzzerDevice || 'addp_03',
      configId: config.id,
      savedConfig: config
    })
    
    const storageConfig = {
      productId: config.productId,
      accessKey: config.accessKey || config.token,
      lightDevice: config.lightDevice || 'addp_01',
      humanDevice: config.humanDevice || 'addp_02',
      buzzerDevice: config.buzzerDevice || 'addp_03',
      savedAt: new Date().toLocaleString()
    }
    wx.setStorageSync('deviceConfig', storageConfig)
    
    wx.showToast({ title: '已选择配置', icon: 'success' })
  },

  testConnection() {
    if (!this.data.savedConfig) {
      wx.showToast({ title: '请先保存配置', icon: 'none' })
      return
    }

    this.setData({ isTesting: true, testResult: '' })

    const { productId, lightDevice, accessKey } = this.data.savedConfig
    const deviceName = lightDevice || this.data.savedConfig.humanDevice || this.data.savedConfig.buzzerDevice

    wx.request({
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
      timeout: 10000,
      success: (res) => {
        if (res.statusCode === 200 && res.data.code === 0) {
          this.setData({ testResult: 'success' })
          wx.showToast({ title: '连接成功', icon: 'success' })
        } else {
          this.setData({ testResult: 'failed' })
          wx.showToast({
            title: '连接失败: ' + (res.data.msg || 'HTTP ' + res.statusCode),
            icon: 'none',
            duration: 2000
          })
        }
      },
      fail: (err) => {
        this.setData({ testResult: 'failed' })
        console.error('Test connection failed:', err)
        wx.showToast({ title: '网络连接失败', icon: 'none' })
      },
      complete: () => {
        this.setData({ isTesting: false })
      }
    })
  },

  goToMonitor() {
    if (!this.data.savedConfig) {
      wx.showToast({ title: '请先保存配置', icon: 'none' })
      return
    }
    wx.navigateTo({ url: '/pages/index/index' })
  }
})

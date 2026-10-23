class Database {
  constructor() {
    this.useIndexedDB = false
    this.init()
  }

  init() {
    try {
      if (typeof indexedDB !== 'undefined') {
        this.useIndexedDB = true
        this.dbName = 'IoTMonitorDB'
        this.dbVersion = 1
        this.db = null
        this.ready = false
        return this.initIndexedDB()
      }
    } catch (e) {
      console.log('IndexedDB not available, falling back to storage API')
    }
    return Promise.resolve()
  }

  initIndexedDB() {
    const that = this
    return new Promise((resolve, reject) => {
      if (this.ready && this.db) {
        resolve(this.db)
        return
      }

      const request = indexedDB.open(this.dbName, this.dbVersion)

      request.onerror = function(event) {
        console.error('IndexedDB error:', event.target.error)
        that.useIndexedDB = false
        reject(event.target.error)
      }

      request.onsuccess = function(event) {
        that.db = event.target.result
        that.ready = true
        resolve(that.db)
      }

      request.onupgradeneeded = function(event) {
        const db = event.target.result
        
        if (!db.objectStoreNames.contains('configs')) {
          const configStore = db.createObjectStore('configs', { keyPath: 'id', autoIncrement: true })
          configStore.createIndex('deviceId', 'deviceId', { unique: true })
        }

        if (!db.objectStoreNames.contains('sensorData')) {
          const dataStore = db.createObjectStore('sensorData', { keyPath: 'id', autoIncrement: true })
          dataStore.createIndex('deviceId', 'deviceId', { unique: false })
          dataStore.createIndex('timestamp', 'timestamp', { unique: false })
        }
      }
    })
  }

  async addConfig(config) {
    if (this.useIndexedDB) {
      try {
        await this.init()
        return new Promise((resolve, reject) => {
          const transaction = this.db.transaction(['configs'], 'readwrite')
          const store = transaction.objectStore('configs')
          
          const configData = {
            deviceId: config.deviceId,
            productId: config.productId,
            token: config.token,
            name: config.name || config.deviceId,
            createdAt: new Date().toISOString(),
            updatedAt: new Date().toISOString()
          }

          const request = store.add(configData)
          request.onsuccess = () => resolve(request.result)
          request.onerror = () => {
            console.log('IndexedDB add failed, trying storage API')
            this.useIndexedDB = false
            this.addConfigStorage(config).then(resolve).catch(reject)
          }
        })
      } catch (e) {
        this.useIndexedDB = false
        return this.addConfigStorage(config)
      }
    } else {
      return this.addConfigStorage(config)
    }
  }

  async addConfigStorage(config) {
    try {
      const configs = this.getConfigsStorage()
      const newConfig = {
        id: Date.now(),
        deviceId: config.deviceId,
        productId: config.productId,
        token: config.token,
        name: config.name || config.deviceId,
        createdAt: new Date().toISOString(),
        updatedAt: new Date().toISOString()
      }
      configs.push(newConfig)
      wx.setStorageSync('db_configs', JSON.stringify(configs))
      return newConfig.id
    } catch (e) {
      console.error('Storage API add config failed:', e)
      throw e
    }
  }

  async updateConfig(id, config) {
    if (this.useIndexedDB) {
      try {
        await this.init()
        return new Promise((resolve, reject) => {
          const transaction = this.db.transaction(['configs'], 'readwrite')
          const store = transaction.objectStore('configs')
          
          const request = store.get(id)
          request.onsuccess = function() {
            const existing = request.result
            if (existing) {
              Object.assign(existing, config, { updatedAt: new Date().toISOString() })
              const updateRequest = store.put(existing)
              updateRequest.onsuccess = () => resolve(updateRequest.result)
              updateRequest.onerror = () => reject(updateRequest.error)
            } else {
              reject(new Error('Config not found'))
            }
          }
          request.onerror = () => {
            this.useIndexedDB = false
            this.updateConfigStorage(id, config).then(resolve).catch(reject)
          }
        })
      } catch (e) {
        this.useIndexedDB = false
        return this.updateConfigStorage(id, config)
      }
    } else {
      return this.updateConfigStorage(id, config)
    }
  }

  async updateConfigStorage(id, config) {
    try {
      const configs = this.getConfigsStorage()
      const index = configs.findIndex(c => c.id === id)
      if (index !== -1) {
        configs[index] = { ...configs[index], ...config, updatedAt: new Date().toISOString() }
        wx.setStorageSync('db_configs', JSON.stringify(configs))
        return id
      }
      throw new Error('Config not found')
    } catch (e) {
      console.error('Storage API update config failed:', e)
      throw e
    }
  }

  async getAllConfigs() {
    if (this.useIndexedDB) {
      try {
        await this.init()
        return new Promise((resolve, reject) => {
          const transaction = this.db.transaction(['configs'], 'readonly')
          const store = transaction.objectStore('configs')
          const request = store.getAll()
          request.onsuccess = () => resolve(request.result || [])
          request.onerror = () => {
            this.useIndexedDB = false
            resolve(this.getConfigsStorage())
          }
        })
      } catch (e) {
        this.useIndexedDB = false
        return this.getConfigsStorage()
      }
    } else {
      return this.getConfigsStorage()
    }
  }

  getConfigsStorage() {
    try {
      const data = wx.getStorageSync('db_configs')
      return data ? JSON.parse(data) : []
    } catch (e) {
      console.error('Storage API get configs failed:', e)
      return []
    }
  }

  async deleteConfig(id) {
    if (this.useIndexedDB) {
      try {
        await this.init()
        return new Promise((resolve, reject) => {
          const transaction = this.db.transaction(['configs'], 'readwrite')
          const store = transaction.objectStore('configs')
          const request = store.delete(id)
          request.onsuccess = () => resolve()
          request.onerror = () => {
            this.useIndexedDB = false
            this.deleteConfigStorage(id).then(resolve).catch(reject)
          }
        })
      } catch (e) {
        this.useIndexedDB = false
        return this.deleteConfigStorage(id)
      }
    } else {
      return this.deleteConfigStorage(id)
    }
  }

  async deleteConfigStorage(id) {
    try {
      const configs = this.getConfigsStorage()
      const filtered = configs.filter(c => c.id !== id)
      wx.setStorageSync('db_configs', JSON.stringify(filtered))
    } catch (e) {
      console.error('Storage API delete config failed:', e)
      throw e
    }
  }

  async addSensorData(data) {
    if (this.useIndexedDB) {
      try {
        await this.init()
        return new Promise((resolve, reject) => {
          const transaction = this.db.transaction(['sensorData'], 'readwrite')
          const store = transaction.objectStore('sensorData')
          
          const sensorData = {
            deviceId: data.deviceId,
            temperature: data.temperature,
            humidity: data.humidity,
            light: data.light,
            pressure: data.pressure,
            timestamp: data.timestamp || Date.now(),
            createTime: new Date().toISOString()
          }

          const request = store.add(sensorData)
          request.onsuccess = () => resolve(request.result)
          request.onerror = () => {
            this.useIndexedDB = false
            this.addSensorDataStorage(data).then(resolve).catch(reject)
          }
        })
      } catch (e) {
        this.useIndexedDB = false
        return this.addSensorDataStorage(data)
      }
    } else {
      return this.addSensorDataStorage(data)
    }
  }

  async addSensorDataStorage(data) {
    try {
      const sensorData = this.getSensorDataStorage(data.deviceId)
      const newData = {
        id: Date.now() + Math.random(),
        deviceId: data.deviceId,
        temperature: data.temperature,
        humidity: data.humidity,
        light: data.light,
        pressure: data.pressure,
        timestamp: data.timestamp || Date.now(),
        createTime: new Date().toISOString()
      }
      sensorData.push(newData)
      
      if (sensorData.length > 1000) {
        sensorData.splice(0, sensorData.length - 1000)
      }
      
      wx.setStorageSync(`db_sensor_${data.deviceId}`, JSON.stringify(sensorData))
      return newData.id
    } catch (e) {
      console.error('Storage API add sensor data failed:', e)
      throw e
    }
  }

  async getSensorDataByDeviceId(deviceId, limit = 100) {
    if (this.useIndexedDB) {
      try {
        await this.init()
        return new Promise((resolve, reject) => {
          const transaction = this.db.transaction(['sensorData'], 'readonly')
          const store = transaction.objectStore('sensorData')
          const index = store.index('deviceId')
          const request = index.getAll(deviceId)
          
          request.onsuccess = function() {
            let results = request.result || []
            results = results.sort((a, b) => b.timestamp - a.timestamp).slice(0, limit)
            resolve(results)
          }
          request.onerror = () => {
            this.useIndexedDB = false
            resolve(this.getSensorDataStorage(deviceId, limit))
          }
        })
      } catch (e) {
        this.useIndexedDB = false
        return this.getSensorDataStorage(deviceId, limit)
      }
    } else {
      return this.getSensorDataStorage(deviceId, limit)
    }
  }

  getSensorDataStorage(deviceId, limit = 100) {
    try {
      const data = wx.getStorageSync(`db_sensor_${deviceId}`)
      let results = data ? JSON.parse(data) : []
      results = results.sort((a, b) => b.timestamp - a.timestamp).slice(0, limit)
      return results
    } catch (e) {
      console.error('Storage API get sensor data failed:', e)
      return []
    }
  }

  async deleteOldSensorData(deviceId, daysToKeep = 7) {
    if (this.useIndexedDB) {
      try {
        await this.init()
        return new Promise((resolve, reject) => {
          const cutoffTime = Date.now() - (daysToKeep * 24 * 60 * 60 * 1000)
          const transaction = this.db.transaction(['sensorData'], 'readwrite')
          const store = transaction.objectStore('sensorData')
          const request = store.openCursor()
          let deletedCount = 0

          request.onsuccess = function(event) {
            const cursor = event.target.result
            if (cursor) {
              if (cursor.value.deviceId === deviceId && cursor.value.timestamp < cutoffTime) {
                cursor.delete()
                deletedCount++
              }
              cursor.continue()
            } else {
              resolve(deletedCount)
            }
          }
          request.onerror = () => {
            this.useIndexedDB = false
            this.deleteOldSensorDataStorage(deviceId, daysToKeep).then(resolve).catch(reject)
          }
        })
      } catch (e) {
        this.useIndexedDB = false
        return this.deleteOldSensorDataStorage(deviceId, daysToKeep)
      }
    } else {
      return this.deleteOldSensorDataStorage(deviceId, daysToKeep)
    }
  }

  async deleteOldSensorDataStorage(deviceId, daysToKeep = 7) {
    try {
      const cutoffTime = Date.now() - (daysToKeep * 24 * 60 * 60 * 1000)
      let sensorData = this.getSensorDataStorage(deviceId, Infinity)
      const originalLength = sensorData.length
      sensorData = sensorData.filter(item => item.timestamp >= cutoffTime)
      wx.setStorageSync(`db_sensor_${deviceId}`, JSON.stringify(sensorData))
      return originalLength - sensorData.length
    } catch (e) {
      console.error('Storage API delete old data failed:', e)
      throw e
    }
  }

  async clearAllSensorData() {
    if (this.useIndexedDB) {
      try {
        await this.init()
        return new Promise((resolve, reject) => {
          const transaction = this.db.transaction(['sensorData'], 'readwrite')
          const store = transaction.objectStore('sensorData')
          const request = store.clear()
          request.onsuccess = () => resolve()
          request.onerror = () => {
            this.useIndexedDB = false
            this.clearAllSensorDataStorage().then(resolve).catch(reject)
          }
        })
      } catch (e) {
        this.useIndexedDB = false
        return this.clearAllSensorDataStorage()
      }
    } else {
      return this.clearAllSensorDataStorage()
    }
  }

  async clearAllSensorDataStorage() {
    try {
      const keys = wx.getStorageInfoSync().keys
      keys.forEach(key => {
        if (key.startsWith('db_sensor_')) {
          wx.removeStorageSync(key)
        }
      })
    } catch (e) {
      console.error('Storage API clear all data failed:', e)
      throw e
    }
  }
}

const db = new Database()

module.exports = db
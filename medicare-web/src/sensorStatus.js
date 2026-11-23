// ECG snapshots are published when a hardware recording finishes. Treat the
// live snapshot as current for a short window; old Firebase values are history.
export const LIVE_SENSOR_WINDOW_MS = 2 * 60 * 1000;
export const DEVICE_HEARTBEAT_WINDOW_MS = 18 * 1000;

export const isRecentSensorReading = (reading, now = Date.now()) => {
  const timestamp = Number(reading?.timestamp);
  return Number.isFinite(timestamp) && timestamp > 0 && now - timestamp <= LIVE_SENSOR_WINDOW_MS && timestamp <= now + 30_000;
};

export const getEcgConnectionState = (reading, now = Date.now()) => {
  if (!reading || !isRecentSensorReading(reading, now)) return 'disconnected';
  if (reading.leadsConnected !== true) return 'disconnected';
  if (!Array.isArray(reading.samples) || reading.samples.length < 6) return 'disconnected';
  return 'connected';
};

export const isDeviceConnected = (presence, now = Date.now()) => {
  const timestamp = Number(presence?.lastSeen);
  return presence?.connected === true &&
    Number.isFinite(timestamp) &&
    timestamp > 0 &&
    now - timestamp <= DEVICE_HEARTBEAT_WINDOW_MS &&
    timestamp <= now + 30_000;
};

export const isSpO2SensorConnected = (presence, now = Date.now()) =>
  isDeviceConnected(presence, now) && presence.spo2Connected === true;

export const isEcgModuleConnected = (presence, now = Date.now()) =>
  isDeviceConnected(presence, now) && presence.ecgConnected === true;

export const areEcgLeadsConnected = (presence, now = Date.now()) =>
  isEcgModuleConnected(presence, now) && presence.ecgLeadsConnected === true;

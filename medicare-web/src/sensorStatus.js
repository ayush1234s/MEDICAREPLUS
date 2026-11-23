// ECG snapshots are published when a hardware recording finishes. Treat the
// live snapshot as current for a short window; old Firebase values are history.
export const LIVE_SENSOR_WINDOW_MS = 2 * 60 * 1000;

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

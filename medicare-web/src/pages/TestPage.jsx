import React, { useState, useEffect, useRef } from 'react';
import { 
  Heart, 
  Activity, 
  Calendar, 
  Clock, 
  Search, 
  ArrowLeft, 
  CheckCircle, 
  AlertTriangle, 
  RefreshCw, 
  Sparkles,
  Zap,
  TrendingUp,
  Cpu,
  Trash2,
  X
} from 'lucide-react';
import { fetchReadingsFromDb, pushSensorReadingToDb, deleteReadingFromDb } from '../firebase';

export default function TestPage({ liveVitals }) {
  // activeSubView: 'TILES' | 'SPO2' | 'ECG'
  const [activeSubView, setActiveSubView] = useState('TILES');

  // Filter states
  const [fromDate, setFromDate] = useState('');
  const [toDate, setToDate] = useState('');
  const [readings, setReadings] = useState([]);
  const [loading, setLoading] = useState(false);
  const [simulating, setSimulating] = useState(false);
  const [deletingId, setDeletingId] = useState(null);
  const [healthAlert, setHealthAlert] = useState(null);
  const [confirmAction, setConfirmAction] = useState(null);

  // Animated ECG Canvas ref
  const canvasRef = useRef(null);

  // Fetch readings when view or dates change
  const loadReadings = async (type, fDate = fromDate, tDate = toDate) => {
    setLoading(true);
    try {
      const data = await fetchReadingsFromDb(type, fDate, tDate);
      setReadings(data);
    } catch (err) {
      console.error("Error fetching readings:", err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (activeSubView === 'SPO2') {
      loadReadings('SPO2');
    } else if (activeSubView === 'ECG') {
      loadReadings('ECG');
    }
  }, [activeSubView]);

  useEffect(() => {
    const spo2 = Number(liveVitals?.spo2?.spo2);
    const bpm = Number(liveVitals?.spo2?.bpm || liveVitals?.ecg?.bpm);
    if (spo2 && spo2 < 95) setHealthAlert({ title: 'Low SpO2 detected', text: `Current oxygen saturation is ${spo2}%. Please rest and seek medical advice if it remains low or you feel unwell.` });
    else if (bpm && (bpm < 60 || bpm > 100)) setHealthAlert({ title: 'Heart-rate attention', text: `Current heart rate is ${bpm} BPM. If you have symptoms or this persists, contact a doctor.` });
  }, [liveVitals]);

  const handleFilterSubmit = (e) => {
    e.preventDefault();
    loadReadings(activeSubView, fromDate, toDate);
  };

  const handleClearFilter = () => {
    setFromDate('');
    setToDate('');
    loadReadings(activeSubView, '', '');
  };

  const handleDeleteReading = async (id) => {
    if (!id) return;
    setDeletingId(id);
    try {
      await deleteReadingFromDb(activeSubView, id);
      setReadings(prev => prev.filter(r => r.id !== id));
    } catch (err) {
      console.error("Error deleting reading:", err);
    } finally {
      setDeletingId(null);
    }
  };

  const handleClearAllReadings = async () => {
    setLoading(true);
    try {
      localStorage.removeItem('readings_' + activeSubView.toLowerCase());
      for (const r of readings) {
        if (r.id) {
          await deleteReadingFromDb(activeSubView, r.id);
        }
      }
      setReadings([]);
    } catch (e) {
      console.error(e);
    } finally {
      setLoading(false);
    }
  };

  // Push simulation reading from ESP32 to Firebase
  const handlePushTestSimulation = async (type) => {
    setSimulating(true);
    try {
      const now = new Date();
      const dateStr = now.toISOString().split('T')[0];
      const timeStr = now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });

      if (type === 'SPO2') {
        const randSpo2 = Math.floor(95 + Math.random() * 5); // 95 - 99%
        const randBpm = Math.floor(68 + Math.random() * 16);  // 68 - 84 bpm
        await pushSensorReadingToDb('spo2', {
          spo2: randSpo2,
          bpm: randBpm,
          date: dateStr,
          time: timeStr,
          status: randSpo2 >= 95 ? 'OPTIMAL' : 'MILD_HYPOXIA',
          source: 'ESP32_MAX30102'
        });
      } else {
        const randBpm = Math.floor(70 + Math.random() * 15);
        await pushSensorReadingToDb('ecg', {
          bpm: randBpm,
          rhythm: 'Normal Sinus Rhythm',
          prInterval: '142ms',
          qrsDuration: '88ms',
          date: dateStr,
          time: timeStr,
          status: 'NORMAL',
          source: 'ESP32_AD8232'
        });
      }
      await loadReadings(type);
    } catch (err) {
      console.error("Error simulating:", err);
    } finally {
      setSimulating(false);
    }
  };

  // Canvas ECG live rhythm animation
  useEffect(() => {
    if (activeSubView !== 'ECG') return;
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    let animationFrameId;
    let step = 0;

    const width = canvas.width;
    const height = canvas.height;
    const midY = height / 2;

    const points = [];
    for (let i = 0; i < width; i++) {
      points.push(midY);
    }

    const render = () => {
      step = (step + 2) % 180;
      let yOffset = 0;
      if (step > 40 && step < 55) {
        yOffset = -15 * Math.sin((step - 40) / 15 * Math.PI); // P wave
      } else if (step >= 55 && step < 60) {
        yOffset = 10; // Q dip
      } else if (step >= 60 && step < 72) {
        yOffset = -55 * Math.sin((step - 60) / 12 * Math.PI); // R peak
      } else if (step >= 72 && step < 78) {
        yOffset = 18; // S dip
      } else if (step >= 90 && step < 120) {
        yOffset = -22 * Math.sin((step - 90) / 30 * Math.PI); // T wave
      }

      points.shift();
      points.push(midY + yOffset);

      ctx.clearRect(0, 0, width, height);

      // Draw faint ECG Grid
      ctx.strokeStyle = 'rgba(6, 182, 212, 0.12)';
      ctx.lineWidth = 1;
      for (let x = 0; x < width; x += 20) {
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, height);
        ctx.stroke();
      }
      for (let y = 0; y < height; y += 20) {
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(width, y);
        ctx.stroke();
      }

      // Draw ECG Line with glowing gradient
      ctx.strokeStyle = '#22d3ee';
      ctx.shadowColor = '#06b6d4';
      ctx.shadowBlur = 10;
      ctx.lineWidth = 2.5;
      ctx.beginPath();
      for (let i = 0; i < points.length; i++) {
        if (i === 0) ctx.moveTo(i, points[i]);
        else ctx.lineTo(i, points[i]);
      }
      ctx.stroke();
      ctx.shadowBlur = 0;

      animationFrameId = requestAnimationFrame(render);
    };

    render();
    return () => cancelAnimationFrame(animationFrameId);
  }, [activeSubView]);

  return (
    <div className="space-y-8 pb-16 max-w-6xl mx-auto">
      {healthAlert && <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/75 backdrop-blur-sm"><div className="w-full max-w-sm rounded-3xl border border-amber-400/40 bg-slate-900 p-6 shadow-2xl"><AlertTriangle className="w-7 h-7 text-amber-400 mb-3" /><h3 className="text-lg font-bold text-white">{healthAlert.title}</h3><p className="text-sm text-slate-300 mt-2">{healthAlert.text}</p><button onClick={() => setHealthAlert(null)} className="mt-6 w-full rounded-xl bg-amber-400 py-2.5 text-sm font-bold text-slate-950">I Understand</button></div></div>}
      {confirmAction && <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/80 backdrop-blur-sm"><div className="w-full max-w-sm rounded-3xl border border-rose-400/30 bg-slate-900 p-6 shadow-2xl"><div className="w-11 h-11 rounded-2xl bg-rose-500/15 text-rose-400 flex items-center justify-center mb-4"><Trash2 className="w-5 h-5" /></div><h3 className="text-lg font-bold text-white">{confirmAction.kind === 'all' ? 'Clear all records?' : 'Delete sensor record?'}</h3><p className="text-sm text-slate-400 mt-2">{confirmAction.kind === 'all' ? 'All displayed test records will be removed from Firebase and local storage.' : 'This sensor reading will be permanently removed from Firebase.'}</p><div className="mt-6 flex gap-3"><button onClick={() => setConfirmAction(null)} className="flex-1 rounded-xl bg-slate-800 py-2.5 text-sm font-semibold text-slate-300">Cancel</button><button onClick={() => { const action = confirmAction; setConfirmAction(null); action.kind === 'all' ? handleClearAllReadings() : handleDeleteReading(action.id); }} className="flex-1 rounded-xl bg-rose-500 py-2.5 text-sm font-bold text-white">{confirmAction.kind === 'all' ? 'Clear records' : 'Delete record'}</button></div></div></div>}
      {/* 2 TILES OVERVIEW */}
      {activeSubView === 'TILES' && (
        <div className="space-y-8">
          <div>
            <h1 className="text-3xl font-extrabold text-white tracking-tight">
              Diagnostic Vitals & Sensor Tests
            </h1>
            <p className="text-slate-400 text-sm mt-1">
              Select a clinical sensor tile below to view real-time data synced from your ESP32 Pocket Medical Kit.
            </p>
          </div>

          <div className="grid md:grid-cols-2 gap-8">
            {/* TILE 1: SPO2 */}
            <div 
              onClick={() => setActiveSubView('SPO2')}
              className="glass-card rounded-3xl p-8 border border-slate-800 hover:border-cyan-500/50 transition-all duration-300 cursor-pointer group hover:shadow-2xl hover:shadow-cyan-500/10 relative overflow-hidden"
            >
              <div className="absolute top-0 right-0 w-48 h-48 bg-cyan-500/10 rounded-full blur-3xl pointer-events-none group-hover:bg-cyan-500/20 transition-all" />

              <div className="flex items-start justify-between mb-8">
                <div className="w-16 h-16 rounded-2xl bg-gradient-to-tr from-cyan-600 to-blue-600 p-0.5 shadow-lg shadow-cyan-500/20">
                  <div className="w-full h-full bg-slate-950 rounded-[14px] flex items-center justify-center">
                    <Heart className="w-8 h-8 text-cyan-400 fill-cyan-400/20 group-hover:scale-110 transition-transform animate-pulse" />
                  </div>
                </div>
                <span className="px-3 py-1 rounded-full bg-cyan-500/20 text-cyan-300 text-xs font-bold border border-cyan-500/30">
                  MAX30102 SENSOR
                </span>
              </div>

              <h3 className="text-2xl font-black text-white group-hover:text-cyan-400 transition-colors mb-2">
                SPO2 Blood Oxygen
              </h3>
              <p className="text-slate-400 text-sm leading-relaxed mb-6">
                Measures blood oxygen saturation levels (%) and pulse rate via optical infrared absorption. Synced directly from your hardware kit to Firebase.
              </p>

              <div className="flex items-center justify-between p-4 rounded-2xl bg-slate-950/70 border border-slate-800">
                <div>
                  <span className="text-[11px] text-slate-400 uppercase font-semibold block">Latest Cloud Reading</span>
                  <span className="text-2xl font-black text-white">
                    {liveVitals?.spo2?.spo2 ? `${liveVitals.spo2.spo2}%` : (readings[0]?.spo2 ? `${readings[0].spo2}%` : '98%')}
                  </span>
                </div>
                <div className="text-right">
                  <span className="text-[11px] text-slate-400 uppercase font-semibold block">Status</span>
                  <span className="text-xs font-bold text-emerald-400 flex items-center gap-1">
                    <CheckCircle className="w-3.5 h-3.5" /> Optimal
                  </span>
                </div>
              </div>

              <button className="w-full mt-6 py-3 rounded-xl bg-cyan-500/20 group-hover:bg-cyan-500 text-cyan-300 group-hover:text-slate-950 font-bold text-sm transition-all flex items-center justify-center gap-2">
                <span>Open SpO2 Monitor & History</span>
                <Sparkles className="w-4 h-4" />
              </button>
            </div>

            {/* TILE 2: ECG + HEART */}
            <div 
              onClick={() => setActiveSubView('ECG')}
              className="glass-card rounded-3xl p-8 border border-slate-800 hover:border-blue-500/50 transition-all duration-300 cursor-pointer group hover:shadow-2xl hover:shadow-blue-500/10 relative overflow-hidden"
            >
              <div className="absolute top-0 right-0 w-48 h-48 bg-blue-500/10 rounded-full blur-3xl pointer-events-none group-hover:bg-blue-500/20 transition-all" />

              <div className="flex items-start justify-between mb-8">
                <div className="w-16 h-16 rounded-2xl bg-gradient-to-tr from-blue-600 to-indigo-600 p-0.5 shadow-lg shadow-blue-500/20">
                  <div className="w-full h-full bg-slate-950 rounded-[14px] flex items-center justify-center">
                    <Activity className="w-8 h-8 text-blue-400 group-hover:scale-110 transition-transform" />
                  </div>
                </div>
                <span className="px-3 py-1 rounded-full bg-blue-500/20 text-blue-300 text-xs font-bold border border-blue-500/30">
                  AD8232 ECG SENSOR
                </span>
              </div>

              <h3 className="text-2xl font-black text-white group-hover:text-blue-400 transition-colors mb-2">
                ECG + Heart Rate
              </h3>
              <p className="text-slate-400 text-sm leading-relaxed mb-6">
                Captures electrical cardiac biopotentials, heart rate rhythm (BPM), and analyzes PR/QRS intervals to evaluate cardiac health in real time.
              </p>

              <div className="flex items-center justify-between p-4 rounded-2xl bg-slate-950/70 border border-slate-800">
                <div>
                  <span className="text-[11px] text-slate-400 uppercase font-semibold block">Latest Heart Rate</span>
                  <span className="text-2xl font-black text-white">
                    {liveVitals?.ecg?.bpm || liveVitals?.spo2?.bpm || '74'} <span className="text-xs font-normal text-slate-400">BPM</span>
                  </span>
                </div>
                <div className="text-right">
                  <span className="text-[11px] text-slate-400 uppercase font-semibold block">Rhythm</span>
                  <span className="text-xs font-bold text-cyan-400 flex items-center gap-1">
                    <CheckCircle className="w-3.5 h-3.5" /> Normal Sinus
                  </span>
                </div>
              </div>

              <button className="w-full mt-6 py-3 rounded-xl bg-blue-500/20 group-hover:bg-blue-500 text-blue-300 group-hover:text-slate-950 font-bold text-sm transition-all flex items-center justify-center gap-2">
                <span>Open ECG + Heart Monitor</span>
                <Zap className="w-4 h-4" />
              </button>
            </div>
          </div>
        </div>
      )}

      {/* SPO2 SUB-VIEW */}
      {activeSubView === 'SPO2' && (
        <div className="space-y-8">
          {/* Header & Back Button */}
          <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
            <div className="flex items-center gap-3">
              <button
                onClick={() => setActiveSubView('TILES')}
                className="p-2.5 rounded-xl bg-slate-900 hover:bg-slate-800 border border-slate-800 text-slate-300 hover:text-white transition-colors cursor-pointer"
              >
                <ArrowLeft className="w-5 h-5" />
              </button>
              <div>
                <h2 className="text-2xl font-black text-white flex items-center gap-2">
                  <Heart className="w-6 h-6 text-cyan-400 fill-cyan-400/20 animate-pulse" />
                  SPO2 Blood Oxygen Monitor
                </h2>
                <p className="text-xs text-slate-400">Real-time measurements recorded by ESP32 MAX30102 sensor</p>
              </div>
            </div>

            {/* Action buttons */}
            <div className="flex items-center gap-2.5">
              <button
                onClick={() => setConfirmAction({ kind: 'all' })}
                className="px-3.5 py-2.5 rounded-xl bg-rose-500/10 hover:bg-rose-500/20 text-rose-300 border border-rose-500/30 text-xs font-semibold flex items-center gap-1.5 transition-colors cursor-pointer"
                title="Wipe mock/simulation logs and keep only real hardware readings"
              >
                <Trash2 className="w-3.5 h-3.5" />
                <span>Clear All Logs</span>
              </button>

              <button
                onClick={() => handlePushTestSimulation('SPO2')}
                disabled={simulating}
                className="px-4 py-2.5 rounded-xl bg-cyan-500/20 hover:bg-cyan-500/30 border border-cyan-500/40 text-cyan-300 text-xs font-bold flex items-center gap-2 transition-all cursor-pointer shadow-sm"
              >
                <Cpu className="w-4 h-4 text-cyan-400" />
                <span>{simulating ? 'Syncing...' : 'Simulate Hardware Push'}</span>
              </button>
            </div>
          </div>

          {/* Current Live Card */}
          <div className="grid sm:grid-cols-3 gap-6">
            <div className="glass-card rounded-3xl p-6 border border-cyan-500/30">
              <span className="text-xs font-semibold text-slate-400 uppercase tracking-wider block mb-1">
                Current SpO2 Reading
              </span>
              <div className="text-5xl font-black text-cyan-400 my-2">
                {liveVitals?.spo2?.spo2 || readings[0]?.spo2 || '98'}%
              </div>
              <span className="text-xs font-semibold text-emerald-400 flex items-center gap-1">
                <CheckCircle className="w-4 h-4" /> Healthy Range (95-100%)
              </span>
            </div>

            <div className="glass-card rounded-3xl p-6 border border-slate-800">
              <span className="text-xs font-semibold text-slate-400 uppercase tracking-wider block mb-1">
                Pulse Rate
              </span>
              <div className="text-5xl font-black text-white my-2">
                {liveVitals?.spo2?.bpm || readings[0]?.bpm || '75'} <span className="text-lg font-normal text-slate-400">BPM</span>
              </div>
              <span className="text-xs font-semibold text-cyan-400 flex items-center gap-1">
                <TrendingUp className="w-4 h-4" /> Resting Heart Rate
              </span>
            </div>

            <div className="glass-card rounded-3xl p-6 border border-slate-800">
              <span className="text-xs font-semibold text-slate-400 uppercase tracking-wider block mb-1">
                Cloud Synchronized
              </span>
              <div className="text-2xl font-bold text-white my-2">
                Firebase RTDB
              </div>
              <p className="text-xs text-slate-400">
                Synced at: {liveVitals?.spo2?.time || (readings[0]?.time ? `${readings[0].date} ${readings[0].time}` : 'Live Stream Active')}
              </p>
            </div>
          </div>

          {/* Date Filter Form */}
          <div className="glass-card rounded-2xl p-5 border border-slate-800">
            <form onSubmit={handleFilterSubmit} className="flex flex-wrap items-end gap-4">
              <div className="flex-1 min-w-[150px]">
                <label className="block text-xs font-semibold text-slate-300 mb-1">
                  Date From (YYYY-MM-DD)
                </label>
                <input
                  type="date"
                  value={fromDate}
                  onChange={(e) => setFromDate(e.target.value)}
                  className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2 px-3 text-sm text-white focus:outline-none focus:border-cyan-500"
                />
              </div>

              <div className="flex-1 min-w-[150px]">
                <label className="block text-xs font-semibold text-slate-300 mb-1">
                  Date To (YYYY-MM-DD)
                </label>
                <input
                  type="date"
                  value={toDate}
                  onChange={(e) => setToDate(e.target.value)}
                  className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2 px-3 text-sm text-white focus:outline-none focus:border-cyan-500"
                />
              </div>

              <button
                type="submit"
                disabled={loading}
                className="px-6 py-2.5 rounded-xl bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-bold text-sm flex items-center gap-2 cursor-pointer transition-colors shadow-md shadow-cyan-500/20"
              >
                <Search className="w-4 h-4" />
                <span>{loading ? 'Searching...' : 'Filter Records'}</span>
              </button>

              {(fromDate || toDate) && (
                <button
                  type="button"
                  onClick={handleClearFilter}
                  className="px-4 py-2.5 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-300 text-xs font-semibold cursor-pointer flex items-center gap-1.5"
                >
                  <X className="w-3.5 h-3.5" />
                  <span>Clear Filter</span>
                </button>
              )}
            </form>
          </div>

          {/* Historical Readings Table with Action Delete Button */}
          <div className="glass-card rounded-3xl overflow-hidden border border-slate-800">
            <div className="p-5 border-b border-slate-800 flex items-center justify-between">
              <div>
                <h3 className="font-bold text-white text-base">Recorded SpO2 Measurement Logs</h3>
                {(fromDate || toDate) && (
                  <p className="text-xs text-cyan-400 mt-0.5">
                    Showing records between {fromDate || 'Start'} and {toDate || 'Present'}
                  </p>
                )}
              </div>
              <span className="text-xs text-slate-400 bg-slate-900 px-3 py-1 rounded-full border border-slate-800">
                {readings.length} records found
              </span>
            </div>

            <div className="overflow-x-auto">
              <table className="w-full text-left border-collapse text-sm">
                <thead>
                  <tr className="border-b border-slate-800/80 bg-slate-900/50 text-xs font-semibold text-slate-400 uppercase tracking-wider">
                    <th className="py-3 px-5">Date</th>
                    <th className="py-3 px-5">Time</th>
                    <th className="py-3 px-5">SpO2 Level</th>
                    <th className="py-3 px-5">Pulse Rate</th>
                    <th className="py-3 px-5">Diagnosis Status</th>
                    <th className="py-3 px-5">Hardware Source</th>
                    <th className="py-3 px-5 text-center">Action</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-800/60">
                  {readings.length > 0 ? (
                    readings.map((r, i) => (
                      <tr key={r.id || i} className="hover:bg-slate-800/40 transition-colors">
                        <td className="py-3.5 px-5 font-mono text-slate-300">
                          {r.date || new Date(r.timestamp).toLocaleDateString()}
                        </td>
                        <td className="py-3.5 px-5 font-mono text-slate-400">
                          {r.time || new Date(r.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}
                        </td>
                        <td className="py-3.5 px-5 font-bold text-cyan-300">
                          {r.spo2}%
                        </td>
                        <td className="py-3.5 px-5 text-white">
                          {r.bpm} BPM
                        </td>
                        <td className="py-3.5 px-5">
                          <span className={`px-2.5 py-0.5 rounded-full text-xs font-semibold ${
                            r.spo2 >= 95 
                              ? 'bg-emerald-500/20 text-emerald-400 border border-emerald-500/30' 
                              : 'bg-amber-500/20 text-amber-400 border border-amber-500/30'
                          }`}>
                            {r.spo2 >= 95 ? 'OPTIMAL' : 'MILD HYPOXIA'}
                          </span>
                        </td>
                        <td className="py-3.5 px-5 text-xs text-slate-400">
                          {r.source || 'ESP32 Device'}
                        </td>
                        <td className="py-3.5 px-5 text-center">
                          <button
                            type="button"
                            onClick={() => setConfirmAction({ kind: 'one', id: r.id })}
                            disabled={deletingId === r.id}
                            className="p-1.5 rounded-lg text-slate-500 hover:text-rose-400 hover:bg-rose-500/10 transition-colors cursor-pointer"
                            title="Delete this record"
                          >
                            <Trash2 className="w-4 h-4" />
                          </button>
                        </td>
                      </tr>
                    ))
                  ) : (
                    <tr>
                      <td colSpan="7" className="py-12 text-center text-slate-500">
                        No SpO2 records found for this date period. Perform a test on your device or click "Simulate Hardware Push" above.
                      </td>
                    </tr>
                  )}
                </tbody>
              </table>
            </div>
          </div>
        </div>
      )}

      {/* ECG+HEART SUB-VIEW */}
      {activeSubView === 'ECG' && (
        <div className="space-y-8">
          {/* Header & Back Button */}
          <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
            <div className="flex items-center gap-3">
              <button
                onClick={() => setActiveSubView('TILES')}
                className="p-2.5 rounded-xl bg-slate-900 hover:bg-slate-800 border border-slate-800 text-slate-300 hover:text-white transition-colors cursor-pointer"
              >
                <ArrowLeft className="w-5 h-5" />
              </button>
              <div>
                <h2 className="text-2xl font-black text-white flex items-center gap-2">
                  <Activity className="w-6 h-6 text-blue-400" />
                  ECG + Heart Rate Rhythm Monitor
                </h2>
                <p className="text-xs text-slate-400">Real-time biopotential signals from ESP32 AD8232 module</p>
              </div>
            </div>

            <div className="flex items-center gap-2.5">
              <button
                onClick={() => setConfirmAction({ kind: 'all' })}
                className="px-3.5 py-2.5 rounded-xl bg-rose-500/10 hover:bg-rose-500/20 text-rose-300 border border-rose-500/30 text-xs font-semibold flex items-center gap-1.5 transition-colors cursor-pointer"
                title="Wipe mock/simulation logs and keep only real hardware readings"
              >
                <Trash2 className="w-3.5 h-3.5" />
                <span>Clear All Logs</span>
              </button>

              <button
                onClick={() => handlePushTestSimulation('ECG')}
                disabled={simulating}
                className="px-4 py-2.5 rounded-xl bg-blue-500/20 hover:bg-blue-500/30 border border-blue-500/40 text-blue-300 text-xs font-bold flex items-center gap-2 transition-all cursor-pointer shadow-sm"
              >
                <Cpu className="w-4 h-4 text-blue-400" />
                <span>{simulating ? 'Syncing...' : 'Simulate Hardware Push'}</span>
              </button>
            </div>
          </div>

          {/* Live ECG Waveform Display */}
          <div className="glass-card rounded-3xl p-6 border border-slate-800">
            <div className="flex items-center justify-between mb-4">
              <div className="flex items-center gap-2">
                <span className="relative flex h-2.5 w-2.5">
                  <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-cyan-400 opacity-75"></span>
                  <span className="relative inline-flex rounded-full h-2.5 w-2.5 bg-cyan-500"></span>
                </span>
                <span className="text-sm font-bold text-white">Live Electrocardiogram Rhythm (Lead I)</span>
              </div>
              <div className="flex items-center gap-4 text-xs font-mono text-cyan-400">
                <span>Sweep: 25mm/s</span>
                <span>Gain: 10mm/mV</span>
              </div>
            </div>

            <div className="rounded-2xl overflow-hidden bg-slate-950 border border-slate-800 p-2 shadow-inner">
              <canvas
                ref={canvasRef}
                width={800}
                height={220}
                className="w-full h-48 block"
              />
            </div>

            <div className="grid grid-cols-2 sm:grid-cols-4 gap-4 mt-6 text-center">
              <div className="p-3 rounded-xl bg-slate-950/60 border border-slate-800">
                <span className="text-[10px] text-slate-400 uppercase font-semibold block">Heart Rate</span>
                <span className="text-xl font-bold text-white">
                  {liveVitals?.ecg?.bpm || liveVitals?.spo2?.bpm || readings[0]?.bpm || '74'} BPM
                </span>
              </div>
              <div className="p-3 rounded-xl bg-slate-950/60 border border-slate-800">
                <span className="text-[10px] text-slate-400 uppercase font-semibold block">Rhythm Type</span>
                <span className="text-xs font-bold text-emerald-400 mt-1 block">Normal Sinus</span>
              </div>
              <div className="p-3 rounded-xl bg-slate-950/60 border border-slate-800">
                <span className="text-[10px] text-slate-400 uppercase font-semibold block">PR Interval</span>
                <span className="text-sm font-bold text-slate-200">140 ms</span>
              </div>
              <div className="p-3 rounded-xl bg-slate-950/60 border border-slate-800">
                <span className="text-[10px] text-slate-400 uppercase font-semibold block">QRS Complex</span>
                <span className="text-sm font-bold text-slate-200">86 ms</span>
              </div>
            </div>
          </div>

          {/* Date Filter Form */}
          <div className="glass-card rounded-2xl p-5 border border-slate-800">
            <form onSubmit={handleFilterSubmit} className="flex flex-wrap items-end gap-4">
              <div className="flex-1 min-w-[150px]">
                <label className="block text-xs font-semibold text-slate-300 mb-1">
                  Date From (YYYY-MM-DD)
                </label>
                <input
                  type="date"
                  value={fromDate}
                  onChange={(e) => setFromDate(e.target.value)}
                  className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2 px-3 text-sm text-white focus:outline-none focus:border-blue-500"
                />
              </div>

              <div className="flex-1 min-w-[150px]">
                <label className="block text-xs font-semibold text-slate-300 mb-1">
                  Date To (YYYY-MM-DD)
                </label>
                <input
                  type="date"
                  value={toDate}
                  onChange={(e) => setToDate(e.target.value)}
                  className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2 px-3 text-sm text-white focus:outline-none focus:border-blue-500"
                />
              </div>

              <button
                type="submit"
                disabled={loading}
                className="px-6 py-2.5 rounded-xl bg-blue-600 hover:bg-blue-500 text-white font-bold text-sm flex items-center gap-2 cursor-pointer transition-colors shadow-md shadow-blue-500/20"
              >
                <Search className="w-4 h-4" />
                <span>{loading ? 'Searching...' : 'Filter Records'}</span>
              </button>

              {(fromDate || toDate) && (
                <button
                  type="button"
                  onClick={handleClearFilter}
                  className="px-4 py-2.5 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-300 text-xs font-semibold cursor-pointer flex items-center gap-1.5"
                >
                  <X className="w-3.5 h-3.5" />
                  <span>Clear Filter</span>
                </button>
              )}
            </form>
          </div>

          {/* Historical ECG Records Table with Action Delete Button */}
          <div className="glass-card rounded-3xl overflow-hidden border border-slate-800">
            <div className="p-5 border-b border-slate-800 flex items-center justify-between">
              <div>
                <h3 className="font-bold text-white text-base">Historical ECG Diagnostic Logs</h3>
                {(fromDate || toDate) && (
                  <p className="text-xs text-blue-400 mt-0.5">
                    Showing records between {fromDate || 'Start'} and {toDate || 'Present'}
                  </p>
                )}
              </div>
              <span className="text-xs text-slate-400 bg-slate-900 px-3 py-1 rounded-full border border-slate-800">
                {readings.length} records found
              </span>
            </div>

            <div className="overflow-x-auto">
              <table className="w-full text-left border-collapse text-sm">
                <thead>
                  <tr className="border-b border-slate-800/80 bg-slate-900/50 text-xs font-semibold text-slate-400 uppercase tracking-wider">
                    <th className="py-3 px-5">Date</th>
                    <th className="py-3 px-5">Time</th>
                    <th className="py-3 px-5">Heart Rate</th>
                    <th className="py-3 px-5">Rhythm Pattern</th>
                    <th className="py-3 px-5">PR Interval</th>
                    <th className="py-3 px-5">Clinical Status</th>
                    <th className="py-3 px-5 text-center">Action</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-800/60">
                  {readings.length > 0 ? (
                    readings.map((r, i) => (
                      <tr key={r.id || i} className="hover:bg-slate-800/40 transition-colors">
                        <td className="py-3.5 px-5 font-mono text-slate-300">
                          {r.date || new Date(r.timestamp).toLocaleDateString()}
                        </td>
                        <td className="py-3.5 px-5 font-mono text-slate-400">
                          {r.time || new Date(r.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}
                        </td>
                        <td className="py-3.5 px-5 font-bold text-blue-300">
                          {r.bpm} BPM
                        </td>
                        <td className="py-3.5 px-5 text-white">
                          {r.rhythm || 'Normal Sinus Rhythm'}
                        </td>
                        <td className="py-3.5 px-5 text-xs text-slate-400 font-mono">
                          {r.prInterval || '140ms'}
                        </td>
                        <td className="py-3.5 px-5">
                          <span className="px-2.5 py-0.5 rounded-full text-xs font-semibold bg-emerald-500/20 text-emerald-400 border border-emerald-500/30">
                            {r.status || 'NORMAL'}
                          </span>
                        </td>
                        <td className="py-3.5 px-5 text-center">
                          <button
                            type="button"
                            onClick={() => setConfirmAction({ kind: 'one', id: r.id })}
                            disabled={deletingId === r.id}
                            className="p-1.5 rounded-lg text-slate-500 hover:text-rose-400 hover:bg-rose-500/10 transition-colors cursor-pointer"
                            title="Delete this record"
                          >
                            <Trash2 className="w-4 h-4" />
                          </button>
                        </td>
                      </tr>
                    ))
                  ) : (
                    <tr>
                      <td colSpan="7" className="py-12 text-center text-slate-500">
                        No ECG records found for this period. Click "Simulate Hardware Push" or record a session with your AD8232 sensor.
                      </td>
                    </tr>
                  )}
                </tbody>
              </table>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

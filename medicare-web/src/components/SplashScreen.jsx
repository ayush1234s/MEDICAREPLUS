import React, { useState, useEffect, useRef } from 'react';
import { Activity, Heart, Shield, Cpu, Sparkles, Volume2, VolumeX } from 'lucide-react';

export default function SplashScreen({ onComplete }) {
  const [progress, setProgress] = useState(0);
  const [statusText, setStatusText] = useState('Booting MEDICARE+ Core...');
  const [soundEnabled, setSoundEnabled] = useState(false); // Default false to avoid browser autoplay policy warning
  const audioCtxRef = useRef(null);

  // Play audio telemetry beep only if user explicitly enabled it
  const playHeartBeep = () => {
    if (!soundEnabled) return;
    try {
      if (!audioCtxRef.current) {
        audioCtxRef.current = new (window.AudioContext || window.webkitAudioContext)();
      }
      const ctx = audioCtxRef.current;
      if (ctx.state === 'suspended') {
        ctx.resume().catch(() => {});
      }
      if (ctx.state === 'running') {
        const osc = ctx.createOscillator();
        const gain = ctx.createGain();

        osc.type = 'sine';
        osc.frequency.setValueAtTime(880, ctx.currentTime);
        osc.frequency.exponentialRampToValueAtTime(600, ctx.currentTime + 0.08);

        gain.gain.setValueAtTime(0.04, ctx.currentTime);
        gain.gain.exponentialRampToValueAtTime(0.0001, ctx.currentTime + 0.09);

        osc.connect(gain);
        gain.connect(ctx.destination);

        osc.start();
        osc.stop(ctx.currentTime + 0.1);
      }
    } catch (e) {
      // AudioContext safely ignored if blocked
    }
  };

  const toggleSound = (e) => {
    e.stopPropagation();
    const nextState = !soundEnabled;
    setSoundEnabled(nextState);
    if (nextState) {
      playHeartBeep();
    }
  };

  useEffect(() => {
    const totalDuration = 5000;
    const intervalTime = 50;
    const increment = 100 / (totalDuration / intervalTime);

    const beepTimer = setInterval(() => {
      if (soundEnabled) {
        playHeartBeep();
      }
    }, 1250);

    const timer = setInterval(() => {
      setProgress((prev) => {
        const next = prev + increment;
        if (next >= 100) {
          clearInterval(timer);
          clearInterval(beepTimer);
          setTimeout(() => {
            onComplete();
          }, 350);
          return 100;
        }

        if (next < 25) {
          setStatusText('Booting MEDICARE+ Telemetry Engine...');
        } else if (next < 50) {
          setStatusText('Linking ESP32 MAX30102 & AD8232 Cloud Gateway...');
        } else if (next < 75) {
          setStatusText('Calibrating Medication Alarms & Cloud Stream...');
        } else {
          setStatusText('Security Protocols Verified. Launching Portal...');
        }

        return next;
      });
    }, intervalTime);

    return () => {
      clearInterval(timer);
      clearInterval(beepTimer);
      if (audioCtxRef.current && audioCtxRef.current.state !== 'closed') {
        try {
          audioCtxRef.current.close().catch(() => {});
        } catch (e) {}
      }
    };
  }, [onComplete, soundEnabled]);

  return (
    <div className="fixed inset-0 z-50 flex flex-col items-center justify-center bg-slate-950 text-white overflow-hidden select-none">
      {/* Background radial glows & animated gradients */}
      <div className="absolute w-[600px] h-[600px] bg-cyan-500/15 rounded-full blur-[120px] pointer-events-none animate-pulse-glow" />
      <div className="absolute w-[500px] h-[500px] bg-blue-600/15 rounded-full blur-[140px] pointer-events-none -bottom-20 -right-20" />
      <div className="absolute w-[400px] h-[400px] bg-rose-500/10 rounded-full blur-[100px] pointer-events-none -top-20 -left-20" />
      
      {/* ECG Background Grid */}
      <div className="absolute inset-0 bg-ecg-grid opacity-35 pointer-events-none" />

      {/* Sound toggle in top corner */}
      <button
        onClick={toggleSound}
        className="absolute top-6 right-6 p-2 rounded-xl bg-slate-900/80 border border-slate-800 text-slate-400 hover:text-white transition-colors cursor-pointer text-xs flex items-center gap-1.5 backdrop-blur-md z-20"
      >
        {soundEnabled ? <Volume2 className="w-4 h-4 text-cyan-400" /> : <VolumeX className="w-4 h-4 text-slate-500" />}
        <span className="hidden sm:inline">{soundEnabled ? 'Telemetry Audio ON' : 'Click for Audio'}</span>
      </button>

      {/* Center Medical Kit Emblem */}
      <div className="relative z-10 flex flex-col items-center max-w-lg w-full px-6 text-center">
        {/* Pulsing Ripple Rings */}
        <div className="relative mb-8 flex items-center justify-center">
          <div className="absolute w-36 h-36 rounded-full border border-cyan-500/30 animate-ping opacity-30 pointer-events-none" />
          <div className="absolute w-48 h-48 rounded-full border border-rose-500/20 animate-pulse pointer-events-none" />

          {/* Central Holographic Emblem Box */}
          <div className="w-32 h-32 rounded-3xl bg-gradient-to-tr from-cyan-500 via-blue-600 to-rose-500 p-1 shadow-2xl shadow-cyan-500/30 relative">
            <div className="w-full h-full bg-slate-950 rounded-[22px] flex flex-col items-center justify-center relative overflow-hidden">
              <div className="absolute inset-0 bg-gradient-to-b from-cyan-500/20 via-transparent to-rose-500/10 pointer-events-none" />
              
              {/* Cardiac Pulse + Beeping Heart Icon */}
              <div className="relative flex items-center justify-center">
                <Activity className="w-14 h-14 text-cyan-400 animate-pulse drop-shadow-[0_0_12px_rgba(34,211,238,0.8)]" />
                <Heart className="w-6 h-6 text-rose-500 fill-rose-500 absolute -top-1.5 -right-2.5 animate-heartbeat drop-shadow-[0_0_14px_rgba(244,63,94,0.9)]" />
              </div>
            </div>
          </div>
        </div>

        {/* Brand Name */}
        <h1 className="text-4xl sm:text-5xl font-black tracking-tight mb-2 bg-gradient-to-r from-white via-cyan-200 to-cyan-400 bg-clip-text text-transparent drop-shadow-sm">
          MEDICARE<span className="text-cyan-400 drop-shadow-[0_0_15px_rgba(34,211,238,0.8)]">+</span>
        </h1>
        <div className="inline-flex items-center gap-2 px-3.5 py-1 rounded-full bg-cyan-950/70 border border-cyan-500/30 text-cyan-300 text-[11px] uppercase tracking-widest font-bold mb-8 shadow-inner">
          <span className="w-1.5 h-1.5 rounded-full bg-cyan-400 animate-ping" />
          Your Smart Medical Kit
        </div>

        {/* 5-Second Futuristic Progress Bar */}
        <div className="w-full bg-slate-900/90 border border-slate-800 rounded-2xl p-1.5 shadow-2xl shadow-black/80 mb-4 backdrop-blur-xl relative overflow-hidden">
          <div 
            className="h-3 rounded-xl bg-gradient-to-r from-blue-600 via-cyan-400 to-emerald-400 transition-all duration-75 ease-out shadow-lg shadow-cyan-500/50 relative overflow-hidden"
            style={{ width: `${Math.min(100, Math.floor(progress))}%` }}
          >
            <div className="absolute inset-0 bg-white/20 animate-pulse" />
          </div>
        </div>

        {/* Progress Percent & Live Status */}
        <div className="flex items-center justify-between w-full text-xs font-mono text-slate-400 px-2">
          <span className="flex items-center gap-2 text-cyan-300 font-semibold truncate max-w-[280px]">
            <span className="w-2 h-2 rounded-full bg-cyan-400 animate-ping shrink-0" />
            {statusText}
          </span>
          <span className="font-bold text-white bg-slate-900 px-2 py-0.5 rounded-md border border-slate-800 shrink-0">
            {Math.min(100, Math.floor(progress))}%
          </span>
        </div>

        {/* Security badges at bottom */}
        <div className="mt-12 flex items-center justify-center gap-6 text-[11px] text-slate-500 font-medium">
          <span className="flex items-center gap-1.5">
            <Shield className="w-3.5 h-3.5 text-cyan-400" /> Clinical 256-Bit SSL
          </span>
          <span className="flex items-center gap-1.5">
            <Cpu className="w-3.5 h-3.5 text-emerald-400" /> ESP32 Realtime Linked
          </span>
        </div>
      </div>
    </div>
  );
}

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
    <div className="fixed inset-0 z-50 flex flex-col items-center justify-center bg-[#f7fbfa] text-slate-900 overflow-hidden select-none">
      {/* Background radial glows & animated gradients */}
      <div className="absolute w-[600px] h-[600px] bg-emerald-400/10 rounded-full blur-[120px] pointer-events-none animate-pulse-glow" />
      
      {/* ECG Background Grid */}
      <div className="absolute inset-0 bg-ecg-grid opacity-[0.08] pointer-events-none" />

      {/* Sound toggle in top corner */}
      <button
        onClick={toggleSound}
        className="absolute top-6 right-6 p-2 rounded-xl bg-white/80 border border-emerald-900/10 text-slate-500 hover:text-emerald-700 transition-colors cursor-pointer text-xs flex items-center gap-1.5 backdrop-blur-md z-20"
      >
        {soundEnabled ? <Volume2 className="w-4 h-4 text-cyan-400" /> : <VolumeX className="w-4 h-4 text-slate-500" />}
        <span className="hidden sm:inline">{soundEnabled ? 'Telemetry Audio ON' : 'Click for Audio'}</span>
      </button>

      {/* Center Medical Kit Emblem */}
      <div className="relative z-10 flex flex-col items-center max-w-lg w-full px-6 text-center">
        {/* Pulsing Ripple Rings */}
        <div className="relative mb-8 flex items-center justify-center">
          <div className="absolute w-36 h-36 rounded-full border border-emerald-500/20 animate-ping opacity-30 pointer-events-none" />

          {/* Central Holographic Emblem Box */}
          <div className="w-24 h-24 rounded-[28px] bg-[#0f8075] shadow-xl shadow-emerald-900/15 flex items-center justify-center relative">
            <span className="text-6xl leading-none font-light text-white mb-1">+</span>
          </div>
        </div>

        {/* Brand Name */}
        <h1 className="text-4xl sm:text-5xl font-black tracking-[0.22em] ml-[0.22em] mb-2 text-[#103d4b]">
          MEDICARE<span className="text-[#13b8ad]">+</span>
        </h1>
        <p className="text-sm text-slate-500 tracking-wide mb-8">Your health, thoughtfully connected</p>

        {/* 5-Second Futuristic Progress Bar */}
        <div className="w-44 bg-emerald-950/10 rounded-full p-1 mb-4 relative overflow-hidden">
          <div 
            className="h-1 rounded-full bg-[#0f8075] transition-all duration-75 ease-out relative overflow-hidden"
            style={{ width: `${Math.min(100, Math.floor(progress))}%` }}
          >
            <div className="absolute inset-0 bg-white/20 animate-pulse" />
          </div>
        </div>

        {/* Progress Percent & Live Status */}
        <div className="flex items-center justify-center w-full text-xs font-mono text-slate-500 px-2">
          <span className="flex items-center gap-2 font-semibold truncate max-w-[280px]">
            {statusText}
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

import React, { useEffect, useRef, useState } from 'react';
import { Heart, ShieldCheck, Cpu, Volume2, VolumeX } from 'lucide-react';

export default function SplashScreen({ onComplete, theme = 'light' }) {
  const [progress, setProgress] = useState(0);
  const [statusText, setStatusText] = useState('Preparing your health workspace');
  const [soundEnabled, setSoundEnabled] = useState(false);
  const audioCtxRef = useRef(null);

  const playHeartBeep = () => {
    if (!soundEnabled) return;
    try {
      if (!audioCtxRef.current) audioCtxRef.current = new (window.AudioContext || window.webkitAudioContext)();
      const ctx = audioCtxRef.current;
      if (ctx.state === 'suspended') ctx.resume();
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.frequency.setValueAtTime(880, ctx.currentTime);
      gain.gain.setValueAtTime(0.035, ctx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.0001, ctx.currentTime + 0.09);
      osc.connect(gain); gain.connect(ctx.destination);
      osc.start(); osc.stop(ctx.currentTime + 0.1);
    } catch {}
  };

  useEffect(() => {
    const timer = setInterval(() => {
      setProgress((prev) => {
        const next = prev + 2;
        if (next < 25) setStatusText('Starting MEDICARE+');
        else if (next < 50) setStatusText('Connecting the health sensors');
        else if (next < 75) setStatusText('Checking medication services');
        else setStatusText('Opening your patient portal');
        if (next >= 100) {
          clearInterval(timer);
          setTimeout(onComplete, 300);
          return 100;
        }
        return next;
      });
    }, 100);
    const beep = setInterval(playHeartBeep, 1250);
    return () => { clearInterval(timer); clearInterval(beep); if (audioCtxRef.current) audioCtxRef.current.close?.(); };
  }, [onComplete, soundEnabled]);

  return (
    <div className={`splash-page ${theme === 'dark' ? 'splash-dark' : ''}`}>
      <button className="splash-sound" onClick={() => setSoundEnabled(!soundEnabled)}>
        {soundEnabled ? <Volume2 /> : <VolumeX />}<span>{soundEnabled ? 'Sound on' : 'Sound off'}</span>
      </button>
      <div className="splash-card">
        <div className="splash-mark"><Heart /></div>
        <div className="splash-brand">MEDICARE<span>+</span></div>
        <p>Smart health, simply managed</p>
        <div className="splash-progress"><span style={{ width: `${progress}%` }} /></div>
        <div className="splash-status"><strong>{statusText}</strong><span>{progress}%</span></div>
        <div className="splash-trust"><span><ShieldCheck /> Secure</span><span><Cpu /> ESP32 link</span></div>
      </div>
    </div>
  );
}

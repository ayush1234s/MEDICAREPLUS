import React from 'react';
import { 
  Activity, 
  Heart, 
  Pill, 
  FileText, 
  Cpu, 
  Wifi, 
  ShieldCheck, 
  Sparkles, 
  ArrowRight, 
  CheckCircle, 
  Clock, 
  Smartphone, 
  Sliders, 
  HelpCircle,
  ExternalLink,
  AlertTriangle,
  ShieldAlert
} from 'lucide-react';

export default function HomePage({ onNavigate, liveVitals, medicines = [], user, userProfile }) {
  const pendingMedicinesCount = medicines.filter(m => m.status === 'Pending').length;
  const hour = new Date().getHours();
  const greeting = hour < 12 ? 'Good Morning' : hour < 18 ? 'Good Afternoon' : 'Good Night';
  const patientName = userProfile?.name || user?.displayName || user?.email?.split('@')[0] || 'Patient';

  return (
    <div className="space-y-10 pb-16">
      {/* HERO SECTION */}
      <section className="relative rounded-[2rem] overflow-hidden border border-teal-400/15 bg-gradient-to-br from-[#14263e] via-[#102238] to-[#091426] p-6 sm:p-10 md:p-12 shadow-[0_26px_70px_-38px_rgba(0,0,0,.95)]">
        <div className="absolute top-0 right-1/4 w-96 h-96 bg-cyan-500/10 rounded-full blur-3xl pointer-events-none" />
        <div className="absolute bottom-0 left-10 w-80 h-80 bg-blue-600/10 rounded-full blur-3xl pointer-events-none" />
        <div className="absolute inset-0 bg-ecg-grid opacity-25 pointer-events-none" />

        <div className="relative z-10 max-w-4xl">
          <p className="text-sm sm:text-base font-semibold text-cyan-300 mb-3">
            {greeting}, <span className="text-white">{patientName}</span>
          </p>
          {/* Live Device Pill */}
          <div className="inline-flex items-center gap-2.5 px-4 py-1.5 rounded-full bg-cyan-950/80 border border-cyan-500/30 text-cyan-300 text-xs font-semibold mb-6 shadow-sm">
            <span className="relative flex h-2 w-2">
              <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"></span>
              <span className="relative inline-flex rounded-full h-2 w-2 bg-emerald-500"></span>
            </span>
            <span>ESP32 Hardware Connected: Cloud Sync Active</span>
          </div>

          <h1 className="text-4xl sm:text-5xl lg:text-6xl font-black text-white tracking-[-0.045em] leading-[.98] mb-5">
            MEDICARE<span className="text-cyan-400">+</span>
            <span className="block text-2xl sm:text-3xl lg:text-4xl font-extrabold bg-gradient-to-r from-cyan-400 via-teal-300 to-blue-400 bg-clip-text text-transparent mt-1">
              Your Smart Pocket Medical Kit
            </span>
          </h1>

          <p className="text-slate-300 text-base md:text-lg leading-relaxed max-w-2xl mb-8">
            Experience clinical-grade wellness monitoring. Synchronize real-time blood oxygen (SpO2), electrocardiogram (ECG), heart rate, and hardware-interactive medicine schedules seamlessly between your pocket device and web dashboard.
          </p>

          {/* Quick CTA Buttons */}
          <div className="flex flex-wrap items-center gap-4 mb-10">
            <button
              onClick={() => onNavigate('TEST')}
              className="px-6 py-3.5 rounded-2xl bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-slate-950 font-bold text-sm flex items-center gap-2.5 transition-all shadow-lg shadow-cyan-500/25 cursor-pointer"
            >
              <Activity className="w-4 h-4" />
              <span>Launch Live Vitals Test</span>
              <ArrowRight className="w-4 h-4" />
            </button>

            <button
              onClick={() => onNavigate('MEDICINE')}
              className="px-6 py-3.5 rounded-2xl bg-slate-900 hover:bg-slate-800 border border-slate-700 text-white font-semibold text-sm flex items-center gap-2.5 transition-all cursor-pointer"
            >
              <Pill className="w-4 h-4 text-emerald-400" />
              <span>Manage Medicines ({pendingMedicinesCount} Due)</span>
            </button>

            <button
              onClick={() => onNavigate('REPORT')}
              className="px-5 py-3.5 rounded-2xl bg-slate-900/60 hover:bg-slate-800 border border-slate-800 text-slate-300 hover:text-white font-medium text-sm flex items-center gap-2 transition-all cursor-pointer"
            >
              <FileText className="w-4 h-4 text-purple-400" />
              <span>Download PDF Report</span>
            </button>
          </div>

          {/* Real-time Quick Metric Cards */}
          <div className="grid grid-cols-2 sm:grid-cols-4 gap-3">
            <div className="glass-card rounded-2xl p-4 border border-white/10">
              <div className="flex items-center justify-between text-slate-400 text-xs mb-2 font-medium">
                <span>Blood Oxygen</span>
                <Heart className="w-4 h-4 text-cyan-400 fill-cyan-400/20" />
              </div>
              <div className="text-2xl font-black text-white">
                {liveVitals?.spo2?.spo2 ? `${liveVitals.spo2.spo2}%` : '98%'}
              </div>
              <span className="text-[11px] text-emerald-400 font-semibold flex items-center gap-1 mt-1">
                <CheckCircle className="w-3 h-3" /> Normal SpO2
              </span>
            </div>

            <div className="glass-card rounded-2xl p-4 border border-white/10">
              <div className="flex items-center justify-between text-slate-400 text-xs mb-2 font-medium">
                <span>Heart Rate</span>
                <Activity className="w-4 h-4 text-rose-400" />
              </div>
              <div className="text-2xl font-black text-white">
                {liveVitals?.spo2?.bpm || liveVitals?.ecg?.bpm || '74'} <span className="text-sm font-normal text-slate-400">BPM</span>
              </div>
              <span className="text-[11px] text-cyan-400 font-semibold flex items-center gap-1 mt-1">
                Sinus Rhythm
              </span>
            </div>

            <div className="glass-card rounded-2xl p-4 border border-white/10">
              <div className="flex items-center justify-between text-slate-400 text-xs mb-2 font-medium">
                <span>Medications</span>
                <Pill className="w-4 h-4 text-emerald-400" />
              </div>
              <div className="text-2xl font-black text-white">
                {medicines.length} <span className="text-sm font-normal text-slate-400">Total</span>
              </div>
              <span className="text-[11px] text-amber-400 font-semibold flex items-center gap-1 mt-1">
                {pendingMedicinesCount} Pending Alert
              </span>
            </div>

            <div className="glass-card rounded-2xl p-4 border border-white/10">
              <div className="flex items-center justify-between text-slate-400 text-xs mb-2 font-medium">
                <span>Kit Link Status</span>
                <Wifi className="w-4 h-4 text-cyan-400" />
              </div>
              <div className="text-xl font-bold text-emerald-400">
                Online
              </div>
              <span className="text-[11px] text-slate-400 font-medium flex items-center gap-1 mt-1">
                Firebase RTDB
              </span>
            </div>
          </div>
        </div>
      </section>

      {/* ABOUT MEDICARE+ - YOUR SMART MEDICAL KIT */}
      <section className="space-y-8">
        <div className="text-center max-w-2xl mx-auto">
          <div className="inline-flex items-center gap-2 px-3.5 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/40 text-cyan-300 text-xs font-semibold uppercase tracking-wider mb-3">
            Hardware & Cloud Ecosystem
          </div>
          <h2 className="text-3xl md:text-4xl font-extrabold text-white tracking-tight">
            ABOUT MEDICARE+
          </h2>
          <p className="text-slate-400 text-sm mt-2">
            YOUR SMART MEDICAL KIT is an integrated IoT healthcare hardware device powered by ESP32, paired with cloud analytics to deliver clinical-grade vital stats anywhere.
          </p>
        </div>

        <div className="grid md:grid-cols-3 gap-6">
          <div className="glass-card rounded-3xl p-6 border border-slate-800 hover:border-cyan-500/30 transition-all">
            <div className="w-12 h-12 rounded-2xl bg-cyan-500/10 border border-cyan-500/20 flex items-center justify-center text-cyan-400 mb-5">
              <Heart className="w-6 h-6 fill-cyan-400/20" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">MAX30102 Pulse Oximeter</h3>
            <p className="text-slate-400 text-xs leading-relaxed mb-4">
              Equipped with dual Red and Infrared optical photoplethysmogram (PPG) sensors measuring blood oxygen saturation (SpO2) and pulse with clinical precision.
            </p>
            <ul className="text-xs text-slate-300 space-y-2 border-t border-slate-800 pt-3">
              <li className="flex items-center gap-2">
                <CheckCircle className="w-3.5 h-3.5 text-cyan-400" /> Real-time waveform rendering
              </li>
              <li className="flex items-center gap-2">
                <CheckCircle className="w-3.5 h-3.5 text-cyan-400" /> Instant cloud upload to Firebase
              </li>
            </ul>
          </div>

          <div className="glass-card rounded-3xl p-6 border border-slate-800 hover:border-cyan-500/30 transition-all">
            <div className="w-12 h-12 rounded-2xl bg-blue-500/10 border border-blue-500/20 flex items-center justify-center text-blue-400 mb-5">
              <Activity className="w-6 h-6" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">AD8232 ECG Bio-Amplifier</h3>
            <p className="text-slate-400 text-xs leading-relaxed mb-4">
              Integrated heart monitor module that extracts, amplifies, and filters tiny biopotential signals in noisy environments like limb motion or electrode placement.
            </p>
            <ul className="text-xs text-slate-300 space-y-2 border-t border-slate-800 pt-3">
              <li className="flex items-center gap-2">
                <CheckCircle className="w-3.5 h-3.5 text-blue-400" /> Dynamic Sinus rhythm detection
              </li>
              <li className="flex items-center gap-2">
                <CheckCircle className="w-3.5 h-3.5 text-blue-400" /> PDF clinical trace export
              </li>
            </ul>
          </div>

          <div className="glass-card rounded-3xl p-6 border border-slate-800 hover:border-cyan-500/30 transition-all">
            <div className="w-12 h-12 rounded-2xl bg-emerald-500/10 border border-emerald-500/20 flex items-center justify-center text-emerald-400 mb-5">
              <Pill className="w-6 h-6" />
            </div>
            <h3 className="text-lg font-bold text-white mb-2">Two-Way Medication Link</h3>
            <p className="text-slate-400 text-xs leading-relaxed mb-4">
              Schedule medications from the web portal. The hardware kit sounds buzzer alerts on the exact time and presents physical <strong className="text-emerald-400">TAKE</strong> or <strong className="text-rose-400">SKIP</strong> buttons!
            </p>
            <ul className="text-xs text-slate-300 space-y-2 border-t border-slate-800 pt-3">
              <li className="flex items-center gap-2">
                <CheckCircle className="w-3.5 h-3.5 text-emerald-400" /> Device buttons update web table live
              </li>
              <li className="flex items-center gap-2">
                <CheckCircle className="w-3.5 h-3.5 text-emerald-400" /> Missed dose alarms & adherence logs
              </li>
            </ul>
          </div>
        </div>
      </section>

      {/* HOW TO USE MEDICARE+ */}
      <section className="rounded-3xl p-6 sm:p-10 md:p-12 border border-slate-800 bg-slate-900/60 space-y-10">
        <div className="max-w-xl">
          <span className="text-xs font-bold text-cyan-400 uppercase tracking-widest">
            Simple 4-Step Workflow
          </span>
          <h2 className="text-3xl font-extrabold text-white tracking-tight mt-1">
            How To Use Your Smart Kit
          </h2>
          <p className="text-slate-400 text-sm mt-2">
            Seamlessly monitor your health and coordinate care from any computer or mobile browser.
          </p>
        </div>

        <div className="grid sm:grid-cols-2 lg:grid-cols-4 gap-6">
          <div className="p-5 rounded-2xl bg-slate-950/60 border border-slate-800 relative">
            <div className="w-8 h-8 rounded-full bg-cyan-500/20 text-cyan-400 font-mono font-bold flex items-center justify-center text-sm mb-4">
              01
            </div>
            <h4 className="font-bold text-white text-sm mb-1.5">Power On Device</h4>
            <p className="text-slate-400 text-xs leading-relaxed">
              Turn on your ESP32 Pocket Kit. It auto-connects to your saved Wi-Fi and connects to the Firebase Cloud gateway.
            </p>
          </div>

          <div className="p-5 rounded-2xl bg-slate-950/60 border border-slate-800 relative">
            <div className="w-8 h-8 rounded-full bg-cyan-500/20 text-cyan-400 font-mono font-bold flex items-center justify-center text-sm mb-4">
              02
            </div>
            <h4 className="font-bold text-white text-sm mb-1.5">Place Finger On Sensor</h4>
            <p className="text-slate-400 text-xs leading-relaxed">
              Navigate to SpO2 or ECG on the device TFT menu and place your finger gently on the sensor for 10-15 seconds.
            </p>
          </div>

          <div className="p-5 rounded-2xl bg-slate-950/60 border border-slate-800 relative">
            <div className="w-8 h-8 rounded-full bg-cyan-500/20 text-cyan-400 font-mono font-bold flex items-center justify-center text-sm mb-4">
              03
            </div>
            <h4 className="font-bold text-white text-sm mb-1.5">View Live Portal Sync</h4>
            <p className="text-slate-400 text-xs leading-relaxed">
              Readings immediately show on the web portal under TEST page. Filter by date to analyze trends and health history.
            </p>
          </div>

          <div className="p-5 rounded-2xl bg-slate-950/60 border border-slate-800 relative">
            <div className="w-8 h-8 rounded-full bg-cyan-500/20 text-cyan-400 font-mono font-bold flex items-center justify-center text-sm mb-4">
              04
            </div>
            <h4 className="font-bold text-white text-sm mb-1.5">Medicine Alarms</h4>
            <p className="text-slate-400 text-xs leading-relaxed">
              Add medications on the website. Device beeps at scheduled time. Press device buttons to confirm Take or Skip!
            </p>
          </div>
        </div>
      </section>

      {/* MANDATORY CLINICAL ADVISORY & MEDICAL DISCLAIMER NOTE */}
      <section className="rounded-3xl p-6 sm:p-8 border border-rose-500/40 bg-gradient-to-r from-rose-950/30 via-slate-900 to-rose-950/30 shadow-xl relative overflow-hidden">
        <div className="absolute top-0 right-0 w-64 h-64 bg-rose-500/10 rounded-full blur-3xl pointer-events-none" />

        <div className="flex flex-col sm:flex-row items-start gap-4 relative z-10">
          <div className="w-12 h-12 rounded-2xl bg-rose-500/20 border border-rose-500/40 flex items-center justify-center text-rose-400 shrink-0 shadow-lg shadow-rose-500/20">
            <ShieldAlert className="w-6 h-6" />
          </div>

          <div className="space-y-2">
            <div className="flex items-center gap-2">
              <span className="text-xs font-bold text-rose-400 uppercase tracking-widest px-2 py-0.5 rounded bg-rose-500/20 border border-rose-500/30">
                CRITICAL HEALTH ADVISORY
              </span>
              <span className="text-xs text-slate-400 font-semibold">Important Medical Notice</span>
            </div>

            <h3 className="text-lg font-black text-white">
              Medical Disclaimer & Emergency Care Warning
            </h3>

            <p className="text-xs sm:text-sm text-slate-300 leading-relaxed">
              <strong className="text-rose-300">In Any Critical Condition Please Go To Doctors, Do Not Use This Device.</strong> This device is designed for <em>daily general wellness purposes</em> to check your Oxygen Saturation (SpO2), Resting Heart Rate, and preliminary ECG wave patterns. <strong>MEDICARE+ does not suggest relying solely on this record in acute or critical medical emergencies.</strong> Certified doctors and emergency hospitals have advanced, clinical-grade medical instruments. If you experience shortness of breath, chest pain, or severe symptoms, please seek immediate emergency medical attention.
            </p>
          </div>
        </div>
      </section>

      {/* FOOTER WITH CREATOR ATTRIBUTION & LINK */}
      <footer className="pt-8 border-t border-slate-800/80 text-xs text-slate-400">
        <div className="flex flex-col md:flex-row items-center justify-between gap-4">
          <div className="flex items-center gap-3">
            <div className="w-7 h-7 rounded-lg bg-cyan-500/20 flex items-center justify-center text-cyan-400">
              <Activity className="w-4 h-4" />
            </div>
            <span className="font-bold text-white">MEDICARE+</span>
            <span className="text-slate-600">|</span>
            <span>Your Smart Medical Kit &copy; 2026</span>
          </div>

          {/* MADE WITH HEART BY AYUSH SRIVASTAVA */}
          <div className="flex items-center gap-1.5 text-xs text-slate-300">
            <span>Made with</span>
            <Heart className="w-4 h-4 text-rose-500 fill-rose-500 inline animate-heartbeat drop-shadow-[0_0_8px_rgba(244,63,94,0.8)]" />
            <span>by</span>
            <a 
              href="https://aayushsrivastava.vercel.app/" 
              target="_blank" 
              rel="noopener noreferrer" 
              className="text-cyan-400 hover:text-cyan-300 font-extrabold underline decoration-cyan-400/50 hover:decoration-cyan-300 transition-all ml-0.5"
            >
              AYUSH SRIVASTAVA
            </a>
          </div>

          <div className="flex items-center gap-6">
            <button onClick={() => onNavigate('HELP')} className="hover:text-cyan-400 transition-colors cursor-pointer">
              Help Center
            </button>
            <button onClick={() => onNavigate('REPORT')} className="hover:text-cyan-400 transition-colors cursor-pointer">
              Clinical Reports
            </button>
            <button onClick={() => onNavigate('PROFILE')} className="hover:text-cyan-400 transition-colors cursor-pointer">
              Profile Settings
            </button>
          </div>
        </div>
      </footer>
    </div>
  );
}

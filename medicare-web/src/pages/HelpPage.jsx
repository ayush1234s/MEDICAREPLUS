import React, { useState } from 'react';
import { 
  HelpCircle, 
  ChevronDown, 
  GripVertical, 
  Cpu, 
  Activity, 
  Pill, 
  Wifi, 
  CheckCircle2, 
  MessageSquare, 
  FileQuestion,
  Sparkles,
  Send,
  Layers
} from 'lucide-react';

export default function HelpPage() {
  const [faqs, setFaqs] = useState([
    {
      id: 'faq-1',
      question: 'How do I link my ESP32 Pocket Kit to this website?',
      answer: 'Your ESP32 firmware is configured with your Wi-Fi credentials and Firebase Realtime Database URL. When turned on, the ESP32 automatically connects to Wi-Fi, syncs with NTP time (UTC+5:30), and connects directly to https://medicare-plus-74a1a-default-rtdb.firebaseio.com/. Sensor readings stream into the website automatically.',
      category: 'Hardware Setup',
      isOpen: true
    },
    {
      id: 'faq-2',
      question: 'How does the two-way Medicine Reminder and Hardware button sync work?',
      answer: 'When you add a medication in the MEDICINE section of the website, it writes to Firebase RTDB (/medicines and /active_reminder). Your ESP32 periodically polls the active reminder. At the scheduled time, the device sounds an alarm and displays the reminder on the TFT screen. Pressing ENTER on the device marks it "Taken", and pressing NEXT marks it "Skip". The web table updates immediately!',
      category: 'Medicine Reminders',
      isOpen: false
    },
    {
      id: 'faq-3',
      question: 'What is the correct way to measure SpO2 on the MAX30102 sensor?',
      answer: 'Navigate to the SPO2 menu on the kit screen, press ENTER, and place your index finger flat over the red optical sensor with gentle, consistent pressure. Avoid pressing too hard as it restricts capillary blood flow. The device counts down for 10-15 seconds and automatically posts the reading to Firebase.',
      category: 'Sensors & SpO2',
      isOpen: false
    },
    {
      id: 'faq-4',
      question: 'How do the 3 physical hardware buttons function on the device?',
      answer: 'The kit features 3 navigation buttons connected to GPIOs (active HIGH via 3.3V): BACK (Pin 1) returns to the main menu or cancels tests; NEXT (Pin 12) cycles forward through tiles or skips a medicine alarm; ENTER (Pin 13) confirms a tile selection, starts measurement, or marks medicine as "Taken".',
      category: 'Hardware Buttons',
      isOpen: false
    },
    {
      id: 'faq-5',
      question: 'How can I download my vitals as a PDF medical report?',
      answer: 'Go to the REPORT page in the sidebar. Choose your desired test (SPO2, ECG, or BOTH), select your date range, and click "Generate Clinical Report". Review the live preview and click "Download Report as PDF" to save the official diagnostic document to your device.',
      category: 'Reporting & PDF',
      isOpen: false
    },
    {
      id: 'faq-6',
      question: 'What happens when I schedule a 7-day temporary account deletion?',
      answer: 'Your account is entered into a 7-day grace period. During these 7 days, your data is preserved, and you can cancel the deletion at any time from your Profile page. If 7 days elapse without cancellation, your profile and logs will be permanently deleted.',
      category: 'Account & Security',
      isOpen: false
    }
  ]);

  // Drag and drop state
  const [draggedItemIndex, setDraggedItemIndex] = useState(null);
  const [feedbackSent, setFeedbackSent] = useState(false);
  const [feedbackText, setFeedbackText] = useState('');

  const toggleFaq = (id) => {
    setFaqs((prev) =>
      prev.map((faq) => (faq.id === id ? { ...faq, isOpen: !faq.isOpen } : faq))
    );
  };

  const handleDragStart = (e, index) => {
    setDraggedItemIndex(index);
    e.dataTransfer.effectAllowed = 'move';
  };

  const handleDragOver = (e, index) => {
    e.preventDefault();
    if (draggedItemIndex === null || draggedItemIndex === index) return;

    const updated = [...faqs];
    const draggedItem = updated[draggedItemIndex];
    updated.splice(draggedItemIndex, 1);
    updated.splice(index, 0, draggedItem);
    setDraggedItemIndex(index);
    setFaqs(updated);
  };

  const handleDragEnd = () => {
    setDraggedItemIndex(null);
  };

  const handleSendFeedback = (e) => {
    e.preventDefault();
    if (!feedbackText.trim()) return;
    setFeedbackSent(true);
    setFeedbackText('');
    setTimeout(() => setFeedbackSent(false), 4000);
  };

  return (
    <div className="space-y-12 max-w-5xl mx-auto pb-16">
      {/* Header */}
      <div>
        <div className="inline-flex items-center gap-2 px-3.5 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/40 text-cyan-300 text-xs font-semibold uppercase tracking-wider mb-3">
          Knowledge Base & User Manual
        </div>
        <h1 className="text-3xl font-extrabold text-white tracking-tight">
          Help Center & Interactive Guide
        </h1>
        <p className="text-slate-400 text-sm mt-1">
          Everything you need to know about your MEDICARE+ Smart Pocket Kit, sensors, and cloud portal.
        </p>
      </div>

      <div className="rounded-2xl border border-cyan-500/25 bg-cyan-500/10 p-4 text-sm text-slate-300">
        <p className="font-bold text-cyan-300">सरल हिंदी सहायता</p>
        <p className="mt-1 text-xs leading-relaxed">ESP32 को Wi-Fi से जोड़ें, फिर वेबसाइट पर डेटा अपने-आप आएगा। दवा का समय सेट करने पर TFT पर अलार्म आएगा। SpO2 या दिल की धड़कन असामान्य लगे या कोई गंभीर लक्षण हों, तो तुरंत डॉक्टर से सलाह लें।</p>
      </div>

      {/* HARDWARE BUTTONS & ARCHITECTURE INFOGRAPHIC */}
      <div className="glass-card rounded-3xl p-6 sm:p-8 border border-slate-800 space-y-6">
        <h3 className="text-lg font-bold text-white flex items-center gap-2">
          <Cpu className="w-5 h-5 text-cyan-400" />
          Hardware Kit Physical Control Reference
        </h3>

        <div className="grid sm:grid-cols-3 gap-4">
          <div className="p-4 rounded-2xl bg-slate-950 border border-slate-800">
            <div className="flex items-center justify-between mb-2">
              <span className="text-xs font-mono font-bold text-cyan-400">GPIO 1</span>
              <span className="text-[10px] px-2 py-0.5 rounded bg-slate-800 text-slate-300 font-bold">PIN_BTN_BACK</span>
            </div>
            <h4 className="font-bold text-white text-sm mb-1">BACK Button</h4>
            <p className="text-xs text-slate-400 leading-relaxed">
              Press to cancel tests, dismiss alarm screen, or return to the main dashboard menu.
            </p>
          </div>

          <div className="p-4 rounded-2xl bg-slate-950 border border-slate-800">
            <div className="flex items-center justify-between mb-2">
              <span className="text-xs font-mono font-bold text-cyan-400">GPIO 12</span>
              <span className="text-[10px] px-2 py-0.5 rounded bg-slate-800 text-slate-300 font-bold">PIN_BTN_NEXT</span>
            </div>
            <h4 className="font-bold text-white text-sm mb-1">NEXT Button</h4>
            <p className="text-xs text-slate-400 leading-relaxed">
              Press to cycle through dashboard tiles, or press during a medicine reminder to mark <strong className="text-rose-400">"SKIP"</strong>.
            </p>
          </div>

          <div className="p-4 rounded-2xl bg-slate-950 border border-slate-800">
            <div className="flex items-center justify-between mb-2">
              <span className="text-xs font-mono font-bold text-cyan-400">GPIO 13</span>
              <span className="text-[10px] px-2 py-0.5 rounded bg-slate-800 text-slate-300 font-bold">PIN_BTN_ENTER</span>
            </div>
            <h4 className="font-bold text-white text-sm mb-1">ENTER Button</h4>
            <p className="text-xs text-slate-400 leading-relaxed">
              Press to launch a sensor test, start SpO2 reading, or press during medicine reminder to mark <strong className="text-emerald-400">"TAKEN"</strong>.
            </p>
          </div>
        </div>
      </div>

      {/* DRAG & DROP QUESTIONS AND ANSWERS */}
      <div className="space-y-4">
        <div className="flex items-center justify-between">
          <div>
            <h3 className="text-xl font-bold text-white flex items-center gap-2">
              <Layers className="w-5 h-5 text-cyan-400" />
              Frequently Asked Questions (Drag to Reorder)
            </h3>
            <p className="text-xs text-slate-400 mt-0.5">
              Grab the grip handle on the left to drag & organize questions in your preferred order. Click any card to expand answers.
            </p>
          </div>
          <span className="text-xs font-mono text-cyan-400 bg-cyan-950/60 px-2.5 py-1 rounded-lg border border-cyan-800/40">
            {faqs.length} Answers
          </span>
        </div>

        <div className="space-y-3">
          {faqs.map((faq, index) => (
            <div
              key={faq.id}
              draggable
              onDragStart={(e) => handleDragStart(e, index)}
              onDragOver={(e) => handleDragOver(e, index)}
              onDragEnd={handleDragEnd}
              className={`glass-card rounded-2xl border transition-all ${
                draggedItemIndex === index 
                  ? 'opacity-40 border-cyan-400 scale-[0.99]' 
                  : 'border-slate-800 hover:border-slate-700'
              }`}
            >
              <div
                onClick={() => toggleFaq(faq.id)}
                className="p-5 flex items-center justify-between gap-4 cursor-pointer select-none"
              >
                <div className="flex items-center gap-3.5 flex-1 min-w-0">
                  <div 
                    className="cursor-grab active:cursor-grabbing text-slate-500 hover:text-cyan-400 p-1"
                    title="Drag to reorder"
                    onClick={(e) => e.stopPropagation()}
                  >
                    <GripVertical className="w-4 h-4" />
                  </div>
                  <div>
                    <span className="text-[10px] font-bold text-cyan-400 uppercase tracking-wider block mb-0.5">
                      {faq.category}
                    </span>
                    <h4 className="text-sm font-bold text-white leading-snug">{faq.question}</h4>
                  </div>
                </div>

                <div className={`p-1.5 rounded-lg bg-slate-900 border border-slate-800 text-slate-400 transition-transform ${faq.isOpen ? 'rotate-180 text-cyan-400' : ''}`}>
                  <ChevronDown className="w-4 h-4" />
                </div>
              </div>

              {faq.isOpen && (
                <div className="px-5 pb-5 pt-1 text-xs text-slate-300 leading-relaxed border-t border-slate-800/80 bg-slate-950/30">
                  {faq.answer}
                </div>
              )}
            </div>
          ))}
        </div>
      </div>

      {/* Quick Help & Feedback Box */}
      <div className="glass-card rounded-3xl p-6 sm:p-8 border border-slate-800">
        <h3 className="text-lg font-bold text-white mb-1 flex items-center gap-2">
          <MessageSquare className="w-5 h-5 text-cyan-400" />
          Need Additional Assistance?
        </h3>
        <p className="text-xs text-slate-400 mb-4">
          Send a query to the MEDICARE+ engineering team for custom hardware calibration or questions.
        </p>

        <form onSubmit={handleSendFeedback} className="flex gap-3">
          <input
            type="text"
            value={feedbackText}
            onChange={(e) => setFeedbackText(e.target.value)}
            placeholder="Type your question or hardware issue..."
            className="flex-1 bg-slate-900 border border-slate-800 rounded-xl py-2.5 px-4 text-sm text-white focus:outline-none focus:border-cyan-500 placeholder-slate-500"
          />
          <button
            type="submit"
            className="px-5 py-2.5 rounded-xl bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-bold text-sm flex items-center gap-2 cursor-pointer transition-colors shadow-md shadow-cyan-500/20"
          >
            <Send className="w-4 h-4" />
            <span>Send Query</span>
          </button>
        </form>

        {feedbackSent && (
          <p className="text-xs text-emerald-400 font-semibold mt-3 flex items-center gap-1.5 animate-pulse">
            <CheckCircle2 className="w-4 h-4" />
            Thank you! Your inquiry has been dispatched to MEDICARE+ support.
          </p>
        )}
      </div>
    </div>
  );
}

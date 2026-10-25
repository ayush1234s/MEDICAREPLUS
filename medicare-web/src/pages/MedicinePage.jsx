import React, { useState } from 'react';
import { 
  Pill, 
  Clock, 
  Calendar, 
  Plus, 
  CheckCircle2, 
  XCircle, 
  AlertCircle, 
  Trash2, 
  Cpu, 
  Smartphone, 
  Sparkles,
  User,
  Radio,
  Check
} from 'lucide-react';
import confetti from 'canvas-confetti';
import { addMedicineToDb, updateMedicineStatusInDb, deleteMedicineFromDb } from '../firebase';

export default function MedicinePage({ user, userProfile, medicines = [] }) {
  const [medicineName, setMedicineName] = useState('');
  const [dosage, setDosage] = useState('');
  const [date, setDate] = useState(() => new Date().toISOString().split('T')[0]);
  const [time, setTime] = useState(() => {
    const d = new Date();
    d.setMinutes(d.getMinutes() + 5);
    return d.toTimeString().slice(0, 5);
  });
  const [days, setDays] = useState('5');
  const [fullName, setFullName] = useState(userProfile?.name || user?.displayName || 'Patient');
  const [gender, setGender] = useState(userProfile?.gender || 'Male');
  
  const [loading, setLoading] = useState(false);
  const [actionLoading, setActionLoading] = useState(null);
  const [medicineToDelete, setMedicineToDelete] = useState(null);

  const handleAddMedicine = async (e) => {
    e.preventDefault();
    if (!medicineName.trim()) {
      alert("Please enter a medicine name");
      return;
    }
    setLoading(true);

    try {
      const payload = {
        medicine: medicineName.trim(),
        dose: dosage.trim() || '1 Dose',
        date,
        time,
        days: Number(days) || 1,
        fullName: fullName || 'Patient',
        gender: gender || 'Male',
        status: 'Pending'
      };

      await addMedicineToDb(payload);

      // Trigger celebratory particle effect
      try {
        confetti({
          particleCount: 40,
          spread: 60,
          origin: { y: 0.6 }
        });
      } catch (err) {}

      // Reset form
      setMedicineName('');
      setDosage('');
    } catch (err) {
      console.error("Error adding medicine:", err);
      alert("Failed to add medication schedule. Check Firebase connection.");
    } finally {
      setLoading(false);
    }
  };

  const handleStatusChange = async (medId, newStatus) => {
    setActionLoading(medId);
    try {
      await updateMedicineStatusInDb(medId, newStatus);
      if (newStatus === 'Taken') {
        try {
          confetti({
            particleCount: 30,
            spread: 50,
            origin: { y: 0.7 }
          });
        } catch (e) {}
      }
    } catch (err) {
      console.error("Error updating medicine status:", err);
    } finally {
      setActionLoading(null);
    }
  };

  const handleDelete = async () => {
    if (!medicineToDelete) return;
    try {
      await deleteMedicineFromDb(medicineToDelete.id);
      setMedicineToDelete(null);
    } catch (err) {
      console.error("Error deleting medicine:", err);
    }
  };

  // Find the most recent active / pending medicine for device preview simulation
  const activeReminderMed = medicines.find(m => m.status === 'Pending') || medicines[0];

  return (
    <div className="space-y-10 max-w-6xl mx-auto pb-16">
      {/* Header */}
      <div>
        <h1 className="text-3xl font-extrabold text-white tracking-tight">
          Medication Management & Device Sync
        </h1>
        <p className="text-slate-400 text-sm mt-1">
          Schedule prescriptions and sync them with your physical ESP32 Smart Pocket Kit. Device beeps on schedule and hardware button presses update status in real time.
        </p>
      </div>

      <div className="grid lg:grid-cols-3 gap-8">
        {/* LEFT COLUMN: Add Medicine Schedule Form */}
        <div className="lg:col-span-2 space-y-6">
          <div className="glass-card rounded-3xl p-6 sm:p-8 border border-slate-800">
            <h3 className="text-lg font-bold text-white mb-1 flex items-center gap-2">
              <Pill className="w-5 h-5 text-emerald-400" />
              Schedule New Medication
            </h3>
            <p className="text-xs text-slate-400 mb-6">
              The ESP32 checks Firebase every few seconds and shows the TFT alarm at the selected date and time.
            </p>

            <form onSubmit={handleAddMedicine} className="space-y-5">
              <div className="grid sm:grid-cols-2 gap-4">
                <div>
                  <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                    Medicine Name *
                  </label>
                  <input
                    type="text"
                    required
                    value={medicineName}
                    onChange={(e) => setMedicineName(e.target.value)}
                    placeholder="e.g. Paracetamol, Amoxicillin"
                    className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2.5 px-3.5 text-sm text-white placeholder-slate-500 focus:outline-none focus:border-cyan-500"
                  />
                </div>

                <div>
                  <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                    Medicine Dose / Strength *
                  </label>
                  <input
                    type="text"
                    required
                    value={dosage}
                    onChange={(e) => setDosage(e.target.value)}
                    placeholder="e.g. 500mg, 1 Capsule, 10ml"
                    className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2.5 px-3.5 text-sm text-white placeholder-slate-500 focus:outline-none focus:border-cyan-500"
                  />
                </div>
              </div>

              <div className="grid sm:grid-cols-3 gap-4">
                <div>
                  <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                    Select Start Date *
                  </label>
                  <input
                    type="date"
                    required
                    value={date}
                    onChange={(e) => setDate(e.target.value)}
                    className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2.5 px-3.5 text-sm text-white focus:outline-none focus:border-cyan-500"
                  />
                </div>

                <div>
                  <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                    Select Alarm Time *
                  </label>
                  <input
                    type="time"
                    required
                    value={time}
                    onChange={(e) => setTime(e.target.value)}
                    className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2.5 px-3.5 text-sm text-white focus:outline-none focus:border-cyan-500"
                  />
                </div>

                <div>
                  <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                    How Many Days To Take It? *
                  </label>
                  <input
                    type="number"
                    min="1"
                    max="365"
                    required
                    value={days}
                    onChange={(e) => setDays(e.target.value)}
                    placeholder="e.g. 5"
                    className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2.5 px-3.5 text-sm text-white focus:outline-none focus:border-cyan-500"
                  />
                </div>
              </div>

              {/* Patient Identity confirmation */}
              <div className="grid sm:grid-cols-2 gap-4 pt-2">
                <div>
                  <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                    Patient Full Name
                  </label>
                  <input
                    type="text"
                    value={fullName}
                    onChange={(e) => setFullName(e.target.value)}
                    placeholder="Ayush Sharma"
                    className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2.5 px-3.5 text-sm text-white focus:outline-none focus:border-cyan-500"
                  />
                </div>

                <div>
                  <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                    Gender
                  </label>
                  <select
                    value={gender}
                    onChange={(e) => setGender(e.target.value)}
                    className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2.5 px-3.5 text-sm text-white focus:outline-none focus:border-cyan-500"
                  >
                    <option value="Male">Male</option>
                    <option value="Female">Female</option>
                    <option value="Other">Other</option>
                  </select>
                </div>
              </div>

              <button
                type="submit"
                disabled={loading}
                className="w-full sm:w-auto px-8 py-3 rounded-xl bg-gradient-to-r from-emerald-500 to-cyan-500 hover:from-emerald-400 hover:to-cyan-400 text-slate-950 font-bold text-sm flex items-center justify-center gap-2 transition-all shadow-md shadow-emerald-500/20 cursor-pointer disabled:opacity-50"
              >
                <Plus className="w-4 h-4" />
                <span>{loading ? 'Transmitting to ESP32...' : 'Add Medicine & Sync with Device'}</span>
              </button>
            </form>
          </div>
        </div>

        {/* RIGHT COLUMN: Interactive Hardware Simulation Hub */}
        <div className="space-y-6">
          <div className="glass-card rounded-3xl p-6 border border-slate-800 relative overflow-hidden">
            <div className="flex items-center justify-between mb-4">
              <span className="text-[10px] font-bold text-cyan-400 uppercase tracking-wider flex items-center gap-1.5">
                <Cpu className="w-3.5 h-3.5" /> MEDICARE+ DEVICE VIEW
              </span>
              <span className="text-[10px] px-2 py-0.5 rounded-full bg-emerald-500/20 text-emerald-400 font-bold border border-emerald-500/30">
                LIVE CLOCK LINK
              </span>
            </div>

            {/* Simulated 320x240 ILI9341 Screen Mockup */}
            <div className="rounded-2xl bg-black border-2 border-slate-700 p-4 font-mono text-center shadow-2xl relative">
              <div className="bg-blue-950 text-white text-[11px] font-bold py-1 border-b-2 border-cyan-400 mb-2">
                MEDICARE+ MEDICINE REMINDER
              </div>

              <p className="text-amber-300 text-xs font-bold mt-2">
                HELLO, {activeReminderMed?.fullName || 'PATIENT'}!
              </p>
              <p className="text-slate-300 text-[10px] mb-3">
                IT IS TIME TO TAKE YOUR MEDICINE.
              </p>

              <div className="border border-cyan-400 bg-slate-900/90 rounded-lg p-3 my-2 text-left space-y-1">
                <p className="text-white text-xs font-black truncate">
                  MEDICINE: <span className="text-cyan-300">{activeReminderMed?.medicine || 'Paracetamol'}</span>
                </p>
                <p className="text-slate-300 text-[10px]">
                  DOSAGE: {activeReminderMed?.dose || '500mg'}
                </p>
                <p className="text-cyan-400 text-[10px]">
                  TIME: {activeReminderMed?.time || '12:00'}
                </p>
                <p className="text-emerald-400 text-[10px] font-bold">
                  STATUS: {activeReminderMed?.status || 'PENDING'}
                </p>
              </div>

              <div className="bg-blue-950 text-slate-300 text-[9px] py-1 mt-3 border-t border-cyan-400 flex justify-around">
                <span>[ENTER: TAKE]</span>
                <span>[NEXT: SKIP]</span>
              </div>
            </div>

            {/* Hardware Physical Buttons Simulation */}
            <div className="mt-4 pt-4 border-t border-slate-800 space-y-3">
              <p className="text-xs text-slate-400 font-semibold">
                Test Device Button Presses:
              </p>
              <div className="grid grid-cols-2 gap-3">
                <button
                  type="button"
                  disabled={!activeReminderMed || actionLoading}
                  onClick={() => activeReminderMed && handleStatusChange(activeReminderMed.id, 'Taken')}
                  className="py-2.5 px-3 rounded-xl bg-emerald-500/20 hover:bg-emerald-500 text-emerald-300 hover:text-slate-950 text-xs font-bold border border-emerald-500/40 transition-all flex items-center justify-center gap-1.5 cursor-pointer disabled:opacity-40"
                >
                  <Check className="w-3.5 h-3.5" />
                  <span>Press ENTER (Take It)</span>
                </button>

                <button
                  type="button"
                  disabled={!activeReminderMed || actionLoading}
                  onClick={() => activeReminderMed && handleStatusChange(activeReminderMed.id, 'Skip')}
                  className="py-2.5 px-3 rounded-xl bg-rose-500/20 hover:bg-rose-500 text-rose-300 hover:text-white text-xs font-bold border border-rose-500/40 transition-all flex items-center justify-center gap-1.5 cursor-pointer disabled:opacity-40"
                >
                  <XCircle className="w-3.5 h-3.5" />
                  <span>Press NEXT (Skip)</span>
                </button>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* MEDICATION SCHEDULE TABLE */}
      <div className="glass-card rounded-3xl overflow-hidden border border-slate-800">
        <div className="p-6 border-b border-slate-800 flex items-center justify-between">
          <div>
            <h3 className="font-bold text-white text-base">Current Medication Roster & Adherence</h3>
            <p className="text-xs text-slate-400 mt-0.5">
              Live updates when hardware buttons are pressed on the device.
            </p>
          </div>
          <span className="text-xs font-mono text-cyan-400 bg-cyan-950/60 px-3 py-1 rounded-full border border-cyan-800/40">
            {medicines.length} Prescriptions Scheduled
          </span>
        </div>

        <div className="overflow-x-auto">
          <table className="w-full text-left border-collapse text-sm">
            <thead>
              <tr className="border-b border-slate-800/80 bg-slate-900/50 text-xs font-semibold text-slate-400 uppercase tracking-wider">
                <th className="py-3 px-4">SrNo.</th>
                <th className="py-3 px-4">Full Name</th>
                <th className="py-3 px-4">Gender</th>
                <th className="py-3 px-4">Medicine</th>
                <th className="py-3 px-4">Dose</th>
                <th className="py-3 px-4">Date</th>
                <th className="py-3 px-4">Time</th>
                <th className="py-3 px-4">Days</th>
                <th className="py-3 px-4">Action Status</th>
                <th className="py-3 px-4 text-center">Manage</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60">
              {medicines.length > 0 ? (
                medicines.map((item, idx) => (
                  <tr key={item.id} className="hover:bg-slate-800/30 transition-colors">
                    <td className="py-3.5 px-4 font-mono text-slate-400 text-xs">
                      #{idx + 1}
                    </td>
                    <td className="py-3.5 px-4 font-semibold text-white">
                      {item.fullName || 'Patient'}
                    </td>
                    <td className="py-3.5 px-4 text-xs text-slate-300">
                      {item.gender || 'Male'}
                    </td>
                    <td className="py-3.5 px-4 font-bold text-cyan-300">
                      {item.medicine}
                    </td>
                    <td className="py-3.5 px-4 text-slate-300 font-mono text-xs">
                      {item.dose}
                    </td>
                    <td className="py-3.5 px-4 font-mono text-xs text-slate-300">
                      {item.date}
                    </td>
                    <td className="py-3.5 px-4 font-mono text-xs text-slate-300">
                      {item.time}
                    </td>
                    <td className="py-3.5 px-4 text-slate-400 text-xs">
                      {item.days} days
                    </td>
                    <td className="py-3.5 px-4">
                      {/* Action Status Badges with quick interactive toggle */}
                      <div className="flex items-center gap-1.5">
                        <button
                          type="button"
                          onClick={() => handleStatusChange(item.id, 'Taken')}
                          title="Mark as Taken (or via Device ENTER)"
                          className={`px-2.5 py-1 rounded-lg text-xs font-bold transition-all cursor-pointer ${
                            item.status === 'Taken'
                              ? 'bg-emerald-500 text-slate-950 shadow-md shadow-emerald-500/20'
                              : 'bg-slate-800 text-slate-400 hover:text-emerald-400'
                          }`}
                        >
                          Taken
                        </button>
                        <button
                          type="button"
                          onClick={() => handleStatusChange(item.id, 'Pending')}
                          title="Mark as Pending"
                          className={`px-2.5 py-1 rounded-lg text-xs font-bold transition-all cursor-pointer ${
                            item.status === 'Pending'
                              ? 'bg-amber-500 text-slate-950 shadow-md shadow-amber-500/20'
                              : 'bg-slate-800 text-slate-400 hover:text-amber-400'
                          }`}
                        >
                          Pending
                        </button>
                        <button
                          type="button"
                          onClick={() => handleStatusChange(item.id, 'Skip')}
                          title="Mark as Skipped (or via Device NEXT)"
                          className={`px-2.5 py-1 rounded-lg text-xs font-bold transition-all cursor-pointer ${
                            item.status === 'Skip'
                              ? 'bg-rose-500 text-white shadow-md shadow-rose-500/20'
                              : 'bg-slate-800 text-slate-400 hover:text-rose-400'
                          }`}
                        >
                          Skip
                        </button>
                      </div>
                    </td>
                    <td className="py-3.5 px-4 text-center">
                      <button
                        type="button"
                        onClick={() => setMedicineToDelete(item)}
                        className="p-1.5 rounded-lg text-slate-500 hover:text-rose-400 hover:bg-slate-800 transition-colors cursor-pointer"
                        title="Delete Medication"
                      >
                        <Trash2 className="w-4 h-4" />
                      </button>
                    </td>
                  </tr>
                ))
              ) : (
                <tr>
                  <td colSpan="10" className="py-12 text-center text-slate-500">
                    No medications scheduled yet. Fill out the form above to add your first medicine and sync with your device.
                  </td>
                </tr>
              )}
            </tbody>
          </table>
        </div>
      </div>

      {medicineToDelete && (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/75 backdrop-blur-sm">
          <div className="w-full max-w-sm rounded-3xl border border-rose-400/35 bg-slate-900 p-6 shadow-2xl">
            <div className="w-11 h-11 rounded-2xl bg-rose-500/15 text-rose-400 flex items-center justify-center mb-4">
              <AlertCircle className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-bold text-white">Remove medicine?</h3>
            <p className="text-sm text-slate-400 mt-2">
              <span className="text-white font-semibold">{medicineToDelete.medicine}</span> will be removed from your schedule and Firebase.
            </p>
            <div className="flex justify-end gap-3 mt-6">
              <button onClick={() => setMedicineToDelete(null)} className="px-4 py-2 rounded-xl bg-slate-800 text-slate-300 text-sm font-semibold">Cancel</button>
              <button onClick={handleDelete} className="px-4 py-2 rounded-xl bg-rose-500 text-white text-sm font-bold">Delete Medicine</button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

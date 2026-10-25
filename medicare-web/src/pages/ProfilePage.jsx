import React, { useState, useEffect } from 'react';
import { 
  User, 
  Mail, 
  Calendar, 
  CheckCircle, 
  Shield, 
  Trash2, 
  Moon, 
  Sun, 
  Sparkles, 
  QrCode, 
  AlertTriangle,
  RotateCcw,
  Check,
  Save,
  Clock,
  IdCard
} from 'lucide-react';
import confetti from 'canvas-confetti';
import QRCode from 'qrcode';
import { saveUserProfileToDb, generateMedicareId } from '../firebase';

export default function ProfilePage({ user, userProfile, onProfileUpdated, theme, setTheme, onLogout, liveVitals }) {
  const [name, setName] = useState(userProfile?.name || user?.displayName || '');
  const [email] = useState(user?.email || '');
  const [dob, setDob] = useState(userProfile?.dob || '');
  const [gender, setGender] = useState(userProfile?.gender || 'Male');
  const [bloodGroup, setBloodGroup] = useState(userProfile?.bloodGroup || 'O+');
  const [phone, setPhone] = useState(userProfile?.phone || '');
  
  const [medicareId, setMedicareId] = useState(userProfile?.medicareId || '');
  const [savedSuccess, setSavedSuccess] = useState(false);
  const [saving, setSaving] = useState(false);

  // 7-day deletion state
  const [scheduledDeletion, setScheduledDeletion] = useState(userProfile?.deletionScheduledAt || null);
  const [showDeleteConfirm, setShowDeleteConfirm] = useState(false);
  const [showQr, setShowQr] = useState(false);
  const [qrImage, setQrImage] = useState('');
  const [section, setSection] = useState('profile');
  const [photoFailed, setPhotoFailed] = useState(false);

  useEffect(() => {
    if (userProfile) {
      if (userProfile.name) setName(userProfile.name);
      if (userProfile.dob) setDob(userProfile.dob);
      if (userProfile.gender) setGender(userProfile.gender);
      if (userProfile.bloodGroup) setBloodGroup(userProfile.bloodGroup);
      if (userProfile.phone) setPhone(userProfile.phone);
      if (userProfile.medicareId) setMedicareId(userProfile.medicareId);
      if (userProfile.deletionScheduledAt) setScheduledDeletion(userProfile.deletionScheduledAt);
    }
  }, [userProfile]);

  const handleSubmitProfile = async (e) => {
    e.preventDefault();
    if (!user) return;
    setSaving(true);
    setSavedSuccess(false);

    try {
      // Auto-generate Medicare ID if not existing
      let assignedId = medicareId;
      if (!assignedId) {
        assignedId = generateMedicareId(user.uid);
        setMedicareId(assignedId);
      }

      const updatedData = {
        name,
        email,
        dob,
        gender,
        bloodGroup,
        phone,
        medicareId: assignedId,
        photoURL: userProfile?.photoURL || user?.photoURL || null
      };

      await saveUserProfileToDb(user.uid, updatedData);
      if (onProfileUpdated) {
        onProfileUpdated(updatedData);
      }

      setSavedSuccess(true);
      try {
        confetti({
          particleCount: 50,
          spread: 60,
          origin: { y: 0.7 }
        });
      } catch (err) {}

      setTimeout(() => setSavedSuccess(false), 4000);
    } catch (err) {
      console.error("Error saving profile:", err);
      alert("Failed to save profile. Please check your connection.");
    } finally {
      setSaving(false);
    }
  };

  const handleScheduleDelete = async () => {
    if (!user) return;
    const deletionDate = new Date();
    deletionDate.setDate(deletionDate.getDate() + 7); // 7 days grace period

    const isoDate = deletionDate.toISOString();
    await saveUserProfileToDb(user.uid, {
      deletionScheduledAt: isoDate,
      accountStatus: 'DELETION_SCHEDULED'
    });

    setScheduledDeletion(isoDate);
    setShowDeleteConfirm(false);
    alert("Account scheduled for temporary deletion. You have 7 days to restore your account before permanent deletion.");
  };

  const handleCancelDelete = async () => {
    if (!user) return;
    await saveUserProfileToDb(user.uid, {
      deletionScheduledAt: null,
      accountStatus: 'ACTIVE'
    });
    setScheduledDeletion(null);
    alert("Account deletion cancelled. Your MEDICARE+ profile remains fully active.");
  };

  const userPhoto = userProfile?.photoURL || user?.photoURL;
  const profileComplete = Boolean(medicareId && name && dob && phone);
  const qrDetails = `MEDICARE+ PATIENT DETAILS\nName: ${name}\nMedicare ID: ${medicareId}\nBlood Group: ${bloodGroup}\nSpO2: ${liveVitals?.spo2?.spo2 ?? 'No reading'}${liveVitals?.spo2?.spo2 ? '%' : ''}\nHeart Rate: ${liveVitals?.spo2?.bpm || liveVitals?.ecg?.bpm || 'No reading'}${liveVitals?.spo2?.bpm || liveVitals?.ecg?.bpm ? ' BPM' : ''}\nView portal: https://medicarepluss.vercel.app/`;

  useEffect(() => {
    if (profileComplete) {
      QRCode.toDataURL(qrDetails, { width: 360, margin: 2, color: { dark: '#0f172a', light: '#ffffff' } }).then(setQrImage).catch(() => setQrImage(''));
    }
  }, [profileComplete, qrDetails]);

  useEffect(() => setPhotoFailed(false), [userPhoto]);

  return (
    <div className="max-w-5xl mx-auto space-y-10 pb-16">
      {/* Header */}
      <div>
        <h1 className="text-3xl font-extrabold text-white tracking-tight">
          User Profile & Settings
        </h1>
        <p className="text-slate-400 text-sm mt-1">
          Manage your personal patient credentials, system Medicare ID, and app preferences.
        </p>
      </div>

      <div className="inline-flex rounded-xl border border-slate-800 bg-slate-900 p-1">
        <button onClick={() => setSection('profile')} className={`px-4 py-2 rounded-lg text-sm font-semibold ${section === 'profile' ? 'bg-cyan-500 text-slate-950' : 'text-slate-400'}`}>Profile</button>
        <button onClick={() => setSection('settings')} className={`px-4 py-2 rounded-lg text-sm font-semibold ${section === 'settings' ? 'bg-cyan-500 text-slate-950' : 'text-slate-400'}`}>Settings</button>
      </div>

      {/* 7-Day Temporary Deletion Warning Banner if Active */}
      {scheduledDeletion && (
        <div className="p-4 rounded-2xl bg-rose-500/15 border border-rose-500/40 text-rose-200 text-sm flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4">
          <div className="flex items-center gap-3">
            <AlertTriangle className="w-5 h-5 text-rose-400 shrink-0" />
            <div>
              <p className="font-bold">Temporary Account Deletion Scheduled</p>
              <p className="text-xs text-rose-300">
                Your account will be permanently erased on <strong className="underline">{new Date(scheduledDeletion).toLocaleDateString()}</strong> (7 days grace period).
              </p>
            </div>
          </div>
          <button
            onClick={handleCancelDelete}
            className="px-4 py-2 rounded-xl bg-slate-900 hover:bg-slate-800 text-white border border-rose-500/40 text-xs font-semibold flex items-center gap-2 cursor-pointer transition-colors"
          >
            <RotateCcw className="w-3.5 h-3.5 text-cyan-400" />
            Cancel Deletion & Restore
          </button>
        </div>
      )}

      <div className="grid lg:grid-cols-3 gap-8">
        {/* Left Column: Official Digital Medicare ID Card */}
        <div className="space-y-6">
          <div className="glass-card rounded-3xl p-6 border border-cyan-500/30 relative overflow-hidden shadow-2xl">
            {/* Holographic background badge */}
            <div className="absolute top-0 right-0 w-36 h-36 bg-gradient-to-br from-cyan-500/20 to-transparent rounded-full blur-2xl pointer-events-none" />

            <div className="flex items-center justify-between border-b border-slate-800 pb-4 mb-5">
              <div className="flex items-center gap-2">
                <div className="w-8 h-8 rounded-lg bg-cyan-500/20 flex items-center justify-center text-cyan-400 font-black text-sm">
                  M+
                </div>
                <div>
                  <h3 className="text-sm font-bold text-white leading-tight">MEDICARE+</h3>
                  <p className="text-[9px] uppercase tracking-wider text-cyan-400 font-semibold">SMART MEDICAL PASS</p>
                </div>
              </div>
              <span className="px-2 py-0.5 rounded-full bg-emerald-500/20 text-emerald-400 text-[10px] font-bold border border-emerald-500/30">
                VERIFIED
              </span>
            </div>

            {/* Profile Photo & Name */}
            <div className="flex items-center gap-4 mb-6">
              {userPhoto && !photoFailed ? (
                <img
                  src={userPhoto}
                  alt={name}
                  referrerPolicy="no-referrer"
                  onError={() => setPhotoFailed(true)}
                  className="w-18 h-18 rounded-2xl object-cover ring-2 ring-cyan-400/50 shadow-md"
                />
              ) : (
                <div className="w-18 h-18 rounded-2xl bg-gradient-to-tr from-cyan-600 to-blue-600 flex items-center justify-center text-white text-2xl font-black ring-2 ring-cyan-400/50 shadow-md">
                  {name ? name.charAt(0).toUpperCase() : 'M'}
                </div>
              )}

              <div className="min-w-0">
                <h4 className="text-base font-bold text-white truncate">{name || 'Unregistered Patient'}</h4>
                <p className="text-xs text-slate-400 truncate">{email}</p>
                <div className="flex items-center gap-2 mt-2">
                  <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-slate-800 text-slate-300">
                    {gender || 'Not specified'}
                  </span>
                  <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-rose-500/20 text-rose-300">
                    {bloodGroup}
                  </span>
                </div>
              </div>
            </div>

            {/* Unique Medicare ID Box */}
            <div className="p-3.5 rounded-2xl bg-slate-950/80 border border-slate-800 mb-4">
              <span className="text-[10px] uppercase font-semibold text-slate-400 block mb-1">
                SYSTEM MEDICARE ID (UNIQUE)
              </span>
              <div className="flex items-center justify-between">
                <span className="text-base font-mono font-black text-cyan-300 tracking-wider">
                  {medicareId || 'CLICK SUBMIT TO GENERATE'}
                </span>
                <IdCard className="w-5 h-5 text-cyan-400" />
              </div>
            </div>

            {/* QR Code & Kit sync chip info */}
            <div className="flex items-center justify-between text-xs text-slate-400 pt-2 border-t border-slate-800/80">
              <div className="space-y-0.5">
                <p className="text-[10px] text-slate-500">DOB: {dob || 'Pending'}</p>
                <p className="text-[10px] text-slate-500">Kit Link: ESP32-RTDB</p>
              </div>
              <button disabled={!profileComplete} onClick={() => setShowQr(true)} className="p-1 rounded-lg bg-white/10 border border-white/20 disabled:opacity-40" title={profileComplete ? 'Open your health QR' : 'Complete your profile to unlock QR'}>
                <QrCode className="w-6 h-6 text-white" />
              </button>
            </div>
          </div>

          {/* Quick Stats / Adherence */}
          <div className="glass-card rounded-2xl p-4 border border-slate-800 space-y-3">
            <h4 className="text-xs font-bold text-slate-300 uppercase tracking-wider">Patient Health Security</h4>
            <div className="flex items-center justify-between text-xs">
              <span className="text-slate-400">HIPAA Compliant Cloud</span>
              <span className="text-emerald-400 font-semibold">Active</span>
            </div>
            <div className="flex items-center justify-between text-xs">
              <span className="text-slate-400">Two-Factor Authentication</span>
              <span className="text-cyan-400 font-semibold">Google Auth</span>
            </div>
          </div>
        </div>

        {/* Right Column: Profile Form & Settings */}
        <div className="lg:col-span-2 space-y-8">
          <div className="text-xs text-slate-400">{section === 'profile' ? 'Manage your patient details and Medicare identity.' : 'Manage display and account preferences.'}</div>
          {/* Profile Details Form */}
          <div className={`glass-card rounded-3xl p-6 sm:p-8 border border-slate-800 ${section === 'profile' ? '' : 'hidden'}`}>
            <h3 className="text-lg font-bold text-white mb-1 flex items-center gap-2">
              <User className="w-5 h-5 text-cyan-400" />
              Personal & Clinical Information
            </h3>
            <p className="text-xs text-slate-400 mb-6">
              Enter your Date of Birth and Gender to automatically generate and maintain your official system Medicare ID.
            </p>

            <form onSubmit={handleSubmitProfile} className="space-y-5">
              <div className="grid sm:grid-cols-2 gap-4">
                <div>
                  <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                    Full Name
                  </label>
                  <input
                    type="text"
                    required
                    value={name}
                    onChange={(e) => setName(e.target.value)}
                    placeholder="Ayush Sharma"
                    className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2.5 px-3.5 text-sm text-white placeholder-slate-500 focus:outline-none focus:border-cyan-500"
                  />
                </div>

                <div>
                  <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                    Email Address (Linked with Auth)
                  </label>
                  <input
                    type="email"
                    disabled
                    value={email}
                    className="w-full bg-slate-950 border border-slate-800/60 rounded-xl py-2.5 px-3.5 text-sm text-slate-400 cursor-not-allowed"
                  />
                </div>
              </div>

              <div className="grid sm:grid-cols-3 gap-4">
                <div>
                  <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                    Date of Birth *
                  </label>
                  <input
                    type="date"
                    required
                    value={dob}
                    onChange={(e) => setDob(e.target.value)}
                    className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2.5 px-3.5 text-sm text-white focus:outline-none focus:border-cyan-500"
                  />
                </div>

                <div>
                  <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                    Gender *
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

                <div>
                  <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                    Blood Group
                  </label>
                  <select
                    value={bloodGroup}
                    onChange={(e) => setBloodGroup(e.target.value)}
                    className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2.5 px-3.5 text-sm text-white focus:outline-none focus:border-cyan-500"
                  >
                    <option value="O+">O positive (O+)</option>
                    <option value="O-">O negative (O-)</option>
                    <option value="A+">A positive (A+)</option>
                    <option value="A-">A negative (A-)</option>
                    <option value="B+">B positive (B+)</option>
                    <option value="B-">B negative (B-)</option>
                    <option value="AB+">AB positive (AB+)</option>
                    <option value="AB-">AB negative (AB-)</option>
                  </select>
                </div>
              </div>

              <div>
                <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                  Emergency Contact Phone Number
                </label>
                <input
                  type="tel"
                  value={phone}
                  onChange={(e) => setPhone(e.target.value)}
                  placeholder="+91 98765 43210"
                  className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2.5 px-3.5 text-sm text-white placeholder-slate-500 focus:outline-none focus:border-cyan-500"
                />
              </div>

              <div className="flex items-center justify-between pt-2">
                <button
                  type="submit"
                  disabled={saving}
                  className="px-6 py-3 rounded-xl bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-slate-950 font-bold text-sm flex items-center gap-2 transition-all shadow-md shadow-cyan-500/25 cursor-pointer disabled:opacity-50"
                >
                  <Save className="w-4 h-4" />
                  <span>{saving ? 'Generating Medicare ID...' : 'Submit & Save Profile'}</span>
                </button>

                {savedSuccess && (
                  <span className="text-xs text-emerald-400 font-semibold flex items-center gap-1.5 animate-pulse">
                    <CheckCircle className="w-4 h-4" />
                    Profile & Medicare ID Saved Successfully!
                  </span>
                )}
              </div>
            </form>
          </div>

          {/* Settings & Theme Section */}
          <div className={`glass-card rounded-3xl p-6 sm:p-8 border border-slate-800 space-y-6 ${section === 'settings' ? '' : 'hidden'}`}>
            <h3 className="text-lg font-bold text-white">App Settings & Customization</h3>

            {/* Theme Toggle */}
            <div className="flex items-center justify-between py-3 border-b border-slate-800">
              <div>
                <p className="text-sm font-semibold text-white">Interface Theme</p>
                <p className="text-xs text-slate-400">Choose between dark medical mode and high contrast</p>
              </div>
              <div className="flex items-center gap-2 p-1 bg-slate-900 border border-slate-800 rounded-xl">
                <button
                  onClick={() => setTheme('dark')}
                  className={`px-3 py-1.5 rounded-lg text-xs font-semibold flex items-center gap-1.5 transition-all cursor-pointer ${
                    theme === 'dark' ? 'bg-cyan-500 text-slate-950 font-bold' : 'text-slate-400 hover:text-white'
                  }`}
                >
                  <Moon className="w-3.5 h-3.5" />
                  <span>Dark</span>
                </button>
                <button
                  onClick={() => setTheme('light')}
                  className={`px-3 py-1.5 rounded-lg text-xs font-semibold flex items-center gap-1.5 transition-all cursor-pointer ${
                    theme === 'light' ? 'bg-cyan-500 text-slate-950 font-bold' : 'text-slate-400 hover:text-white'
                  }`}
                >
                  <Sun className="w-3.5 h-3.5" />
                  <span>Light</span>
                </button>
              </div>
            </div>

            {/* Account Management & Temporary 7-Day Delete */}
            <div className="pt-2">
              <h4 className="text-sm font-semibold text-rose-400 mb-1 flex items-center gap-2">
                <Trash2 className="w-4 h-4" />
                Account Deletion & Grace Period
              </h4>
              <p className="text-xs text-slate-400 mb-4">
                You can temporarily delete your account. MEDICARE+ holds your data safely for <strong>7 days</strong> in a grace period. If you do not reactivate within 7 days, your account and sensor logs are permanently deleted.
              </p>

              {!scheduledDeletion ? (
                <div>
                  {!showDeleteConfirm ? (
                    <button
                      onClick={() => setShowDeleteConfirm(true)}
                      className="px-4 py-2 rounded-xl bg-rose-500/10 hover:bg-rose-500/20 text-rose-400 border border-rose-500/30 text-xs font-semibold transition-colors cursor-pointer"
                    >
                      Schedule Account Deletion (7-Day Temporary Delete)
                    </button>
                  ) : (
                    <div className="p-4 rounded-xl bg-slate-900 border border-rose-500/40 space-y-3">
                      <p className="text-xs text-rose-300 font-semibold">
                        Are you sure you want to schedule deletion? You have 7 days to restore it before permanent deletion.
                      </p>
                      <div className="flex items-center gap-3">
                        <button
                          onClick={handleScheduleDelete}
                          className="px-4 py-2 rounded-lg bg-rose-600 hover:bg-rose-500 text-white text-xs font-bold cursor-pointer"
                        >
                          Confirm 7-Day Deletion
                        </button>
                        <button
                          onClick={() => setShowDeleteConfirm(false)}
                          className="px-4 py-2 rounded-lg bg-slate-800 text-slate-300 text-xs font-semibold cursor-pointer"
                        >
                          Cancel
                        </button>
                      </div>
                    </div>
                  )}
                </div>
              ) : (
                <div className="flex items-center gap-3">
                  <span className="text-xs font-mono text-amber-400 bg-amber-500/10 px-3 py-1.5 rounded-lg border border-amber-500/30">
                    Deletion Scheduled: {new Date(scheduledDeletion).toLocaleDateString()}
                  </span>
                  <button
                    onClick={handleCancelDelete}
                    className="px-4 py-1.5 rounded-lg bg-emerald-500/20 text-emerald-400 border border-emerald-500/30 text-xs font-semibold cursor-pointer hover:bg-emerald-500/30"
                  >
                    Cancel & Keep Account
                  </button>
                </div>
              )}
            </div>
          </div>
        </div>
      </div>

      {showQr && qrImage && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-slate-950/80 p-4 backdrop-blur-sm" onClick={() => setShowQr(false)}>
          <div className="w-full max-w-sm rounded-3xl bg-white p-6 text-center shadow-2xl" onClick={(e) => e.stopPropagation()}>
            <h3 className="text-lg font-black text-slate-900">MEDICARE+ Health QR</h3>
            <p className="text-xs text-slate-500 mt-1">Scan to view this patient's identity and latest vital readings.</p>
            <img src={qrImage} alt="Patient health QR" className="w-64 h-64 mx-auto my-5" />
            <button onClick={() => setShowQr(false)} className="w-full rounded-xl bg-slate-900 py-2.5 text-sm font-bold text-white">Close</button>
          </div>
        </div>
      )}
    </div>
  );
}

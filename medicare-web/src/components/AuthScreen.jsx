import React, { useState } from 'react';
import { 
  loginWithGoogle, 
  loginWithEmail, 
  registerWithEmail, 
  resetPassword,
  saveUserProfileToDb,
  generateMedicareId
} from '../firebase';
import { 
  Activity, 
  Heart, 
  ShieldCheck, 
  Cpu, 
  Pill, 
  FileText, 
  Mail, 
  Lock, 
  User, 
  ArrowRight, 
  Sparkles,
  CheckCircle2,
  AlertCircle,
  HelpCircle,
  X
} from 'lucide-react';

export default function AuthScreen({ onLoginSuccess }) {
  const [isSignUp, setIsSignUp] = useState(false);
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [name, setName] = useState('');
  const [loading, setLoading] = useState(false);
  const [errorMsg, setErrorMsg] = useState('');
  const [successMsg, setSuccessMsg] = useState('');
  const [showForgotModal, setShowForgotModal] = useState(false);
  const [forgotEmail, setForgotEmail] = useState('');
  const [forgotLoading, setForgotLoading] = useState(false);

  const handleEmailAuth = async (e) => {
    e.preventDefault();
    setErrorMsg('');
    setSuccessMsg('');
    setLoading(true);

    try {
      if (isSignUp) {
        if (!name.trim()) {
          throw new Error('Please enter your full name');
        }
        const userCred = await registerWithEmail(email, password, name);
        // Create initial profile in database
        const medicareId = generateMedicareId(userCred.user.uid);
        await saveUserProfileToDb(userCred.user.uid, {
          name: name,
          email: userCred.user.email,
          medicareId: medicareId,
          createdAt: new Date().toISOString()
        });
        setSuccessMsg('Account created successfully! Welcome to MEDICARE+');
      } else {
        await loginWithEmail(email, password);
        setSuccessMsg('Welcome back to MEDICARE+');
      }
      if (onLoginSuccess) onLoginSuccess();
    } catch (err) {
      console.error(err);
      let message = err.message || 'Authentication failed';
      if (err.code === 'auth/invalid-credential' || err.code === 'auth/wrong-password') {
        message = 'Invalid email or password.';
      } else if (err.code === 'auth/email-already-in-use') {
        message = 'This email is already registered. Please login.';
      } else if (err.code === 'auth/weak-password') {
        message = 'Password should be at least 6 characters.';
      }
      setErrorMsg(message);
    } finally {
      setLoading(false);
    }
  };

  const handleGoogleAuth = async () => {
    setErrorMsg('');
    setLoading(true);
    try {
      const res = await loginWithGoogle();
      // check/create profile in db
      const user = res.user;
      const medicareId = generateMedicareId(user.uid);
      await saveUserProfileToDb(user.uid, {
        name: user.displayName || 'Medicare User',
        email: user.email,
        photoURL: user.photoURL,
        medicareId: medicareId
      });
      if (onLoginSuccess) onLoginSuccess();
    } catch (err) {
      console.error(err);
      setErrorMsg(err.message || 'Google authentication failed.');
    } finally {
      setLoading(false);
    }
  };

  const handleForgotPassword = async (e) => {
    e.preventDefault();
    if (!forgotEmail) {
      setErrorMsg('Please enter your email to receive reset instructions.');
      return;
    }
    setForgotLoading(true);
    try {
      await resetPassword(forgotEmail);
      setSuccessMsg(`Password reset link sent to ${forgotEmail}. Check your inbox!`);
      setShowForgotModal(false);
      setForgotEmail('');
    } catch (err) {
      setErrorMsg(err.message || 'Failed to send password reset email.');
    } finally {
      setForgotLoading(false);
    }
  };

  return (
    <div className="h-[100dvh] w-full flex bg-slate-950 text-slate-100 relative overflow-hidden">
      {/* Background visual accents */}
      <div className="absolute top-0 -left-40 w-96 h-96 bg-cyan-500/10 rounded-full blur-3xl pointer-events-none" />
      <div className="absolute bottom-0 -right-40 w-96 h-96 bg-blue-600/10 rounded-full blur-3xl pointer-events-none" />

      {/* LEFT SIDE: About MEDICARE+ YOUR SMART MEDICAL KIT */}
      <div className="hidden lg:flex lg:w-[55%] flex-col justify-between p-7 xl:p-10 relative border-r border-slate-800/80 bg-gradient-to-br from-slate-900/90 via-slate-950 to-slate-900/40 overflow-hidden">
        <div className="absolute inset-0 bg-ecg-grid opacity-30 pointer-events-none" />

        {/* Top Header Badge */}
        <div className="relative z-10 flex items-center gap-3">
          <div className="w-12 h-12 rounded-2xl bg-gradient-to-tr from-cyan-500 to-blue-600 p-0.5 shadow-lg shadow-cyan-500/20">
            <div className="w-full h-full bg-slate-950 rounded-[14px] flex items-center justify-center">
              <Activity className="w-6 h-6 text-cyan-400" />
            </div>
          </div>
          <div>
            <h2 className="text-2xl font-black tracking-tight text-white flex items-center">
              MEDICARE<span className="text-cyan-400">+</span>
            </h2>
            <p className="text-[11px] font-semibold text-cyan-400 uppercase tracking-widest">
              YOUR SMART MEDICAL KIT
            </p>
          </div>
        </div>

        {/* Center Presentation: Smart Medical Kit Highlight */}
        <div className="relative z-10 my-4 xl:my-6 space-y-4 xl:space-y-5">
          <div className="inline-flex items-center gap-2 px-3 py-1.5 rounded-full bg-cyan-950/80 border border-cyan-500/30 text-cyan-300 text-xs font-semibold">
            <Sparkles className="w-3.5 h-3.5 text-cyan-400" />
            ESP32 IoT Cloud Connected Diagnostic Suite
          </div>

          <h1 className="text-3xl xl:text-5xl font-extrabold tracking-tight leading-tight text-white">
            Next-Gen Health <br />
            <span className="bg-gradient-to-r from-cyan-400 via-teal-300 to-blue-400 bg-clip-text text-transparent">
              Monitoring In Your Pocket
            </span>
          </h1>

          <p className="text-slate-400 text-xs xl:text-sm leading-relaxed max-w-lg">
            Connect your physical <strong className="text-cyan-300">MEDICARE+ Smart Pocket Kit</strong> with Firebase Realtime Cloud. Stream continuous pulse oximetry, ECG rhythm, and schedule smart medication alarms with physical confirmation buttons.
          </p>

          {/* Feature Highlights Grid */}
          <div className="grid grid-cols-2 gap-3 pt-2">
            <div className="glass-card rounded-2xl p-3 xl:p-4 border border-slate-800 hover:border-cyan-500/40 transition-all">
              <div className="w-8 h-8 rounded-xl bg-cyan-500/10 flex items-center justify-center mb-2">
                <Heart className="w-4 h-4 text-rose-400 fill-rose-400/30" />
              </div>
              <h4 className="text-sm font-bold text-white mb-1">SpO2 & Heart Rate</h4>
              <p className="text-xs text-slate-400">Continuous optical measurement via MAX30102 sensor.</p>
            </div>

            <div className="glass-card rounded-2xl p-3 xl:p-4 border border-slate-800 hover:border-cyan-500/40 transition-all">
              <div className="w-8 h-8 rounded-xl bg-blue-500/10 flex items-center justify-center mb-2">
                <Activity className="w-4 h-4 text-blue-400" />
              </div>
              <h4 className="text-sm font-bold text-white mb-1">ECG Rhythm Analysis</h4>
              <p className="text-xs text-slate-400">Live electrocardiogram waveform visualization & alerts.</p>
            </div>

            <div className="glass-card rounded-2xl p-3 xl:p-4 border border-slate-800 hover:border-cyan-500/40 transition-all">
              <div className="w-8 h-8 rounded-xl bg-emerald-500/10 flex items-center justify-center mb-2">
                <Pill className="w-4 h-4 text-emerald-400" />
              </div>
              <h4 className="text-sm font-bold text-white mb-1">Hardware Medicine Sync</h4>
              <p className="text-xs text-slate-400">Schedule doses here; device beeps & shows TAKE/SKIP buttons.</p>
            </div>

            <div className="glass-card rounded-2xl p-3 xl:p-4 border border-slate-800 hover:border-cyan-500/40 transition-all">
              <div className="w-8 h-8 rounded-xl bg-purple-500/10 flex items-center justify-center mb-2">
                <FileText className="w-4 h-4 text-purple-400" />
              </div>
              <h4 className="text-sm font-bold text-white mb-1">Instant PDF Reports</h4>
              <p className="text-xs text-slate-400">Generate formatted clinical PDF reports with single click.</p>
            </div>
          </div>
        </div>

        {/* Bottom Hardware Badge */}
        <div className="relative z-10 flex items-center justify-between text-xs text-slate-500 pt-6 border-t border-slate-800/80">
          <div className="flex items-center gap-2">
            <Cpu className="w-4 h-4 text-cyan-400" />
            <span>ESP32 Hardware Compatible (.ino Ready)</span>
          </div>
          <div className="flex items-center gap-2">
            <ShieldCheck className="w-4 h-4 text-emerald-400" />
            <span>Firebase Realtime Database 2.0</span>
          </div>
        </div>
      </div>

      {/* RIGHT SIDE: Authentication Form (Login / Signup) */}
      <div className="w-full lg:w-[45%] flex items-center justify-center p-4 sm:p-6 relative z-10">
        <div className="w-full max-w-md rounded-3xl bg-slate-900/55 border border-slate-700/70 shadow-2xl shadow-cyan-950/30 backdrop-blur-xl p-5 sm:p-7 lg:scale-[.9] xl:scale-100 origin-center">
          {/* Mobile Header */}
          <div className="flex lg:hidden items-center justify-center gap-2 mb-8">
            <Activity className="w-8 h-8 text-cyan-400" />
            <span className="text-2xl font-black text-white">MEDICARE+</span>
          </div>

          {/* Form Header */}
          <div className="mb-5">
            <h2 className="text-3xl font-extrabold text-white tracking-tight">
              {isSignUp ? 'Create your Account' : 'Welcome to MEDICARE+'}
            </h2>
            <p className="text-slate-400 text-sm mt-1">
              {isSignUp 
                ? 'Join to link your medical kit & access live vitals.' 
                : 'Sign in to monitor your health and manage medications.'}
            </p>
          </div>

          {/* Tab Switcher */}
          <div className="flex p-1 bg-slate-950/80 border border-slate-700 rounded-xl mb-4">
            <button
              type="button"
              onClick={() => { setIsSignUp(false); setErrorMsg(''); setSuccessMsg(''); }}
              className={`flex-1 py-2 text-sm font-semibold rounded-lg transition-all ${
                !isSignUp 
                  ? 'bg-cyan-500 text-slate-950 shadow-md font-bold' 
                  : 'text-slate-400 hover:text-white'
              }`}
            >
              Sign In
            </button>
            <button
              type="button"
              onClick={() => { setIsSignUp(true); setErrorMsg(''); setSuccessMsg(''); }}
              className={`flex-1 py-2 text-sm font-semibold rounded-lg transition-all ${
                isSignUp 
                  ? 'bg-cyan-500 text-slate-950 shadow-md font-bold' 
                  : 'text-slate-400 hover:text-white'
              }`}
            >
              Sign Up
            </button>
          </div>

          {/* Alert Messages */}
          {errorMsg && (
            <div className="mb-4 p-3.5 rounded-xl bg-rose-500/10 border border-rose-500/30 text-rose-300 text-xs flex items-start gap-2.5">
              <AlertCircle className="w-4 h-4 shrink-0 text-rose-400 mt-0.5" />
              <span>{errorMsg}</span>
            </div>
          )}

          {successMsg && (
            <div className="mb-4 p-3.5 rounded-xl bg-emerald-500/10 border border-emerald-500/30 text-emerald-300 text-xs flex items-start gap-2.5">
              <CheckCircle2 className="w-4 h-4 shrink-0 text-emerald-400 mt-0.5" />
              <span>{successMsg}</span>
            </div>
          )}

          {/* Google Auth Button */}
          <button
            type="button"
            onClick={handleGoogleAuth}
            disabled={loading}
            className="w-full flex items-center justify-center gap-3 py-3 px-4 rounded-xl bg-slate-950 hover:bg-slate-800 border border-slate-700/80 text-white font-semibold text-sm transition-all shadow-sm hover:border-cyan-500/50 mb-4 cursor-pointer"
          >
            <svg className="w-4 h-4" viewBox="0 0 24 24">
              <path
                fill="#4285F4"
                d="M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92c-.26 1.37-1.04 2.53-2.21 3.31v2.77h3.57c2.08-1.92 3.28-4.74 3.28-8.09z"
              />
              <path
                fill="#34A853"
                d="M12 23c2.97 0 5.46-.98 7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z"
              />
              <path
                fill="#FBBC05"
                d="M5.84 14.09c-.22-.66-.35-1.36-.35-2.09s.13-1.43.35-2.09V7.06H2.18C1.43 8.55 1 10.22 1 12s.43 3.45 1.18 4.94l2.85-2.22.81-.63z"
              />
              <path
                fill="#EA4335"
                d="M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 7.7 1 3.99 3.47 2.18 7.06l3.66 2.84c.87-2.6 3.3-4.52 6.16-4.52z"
              />
            </svg>
            <span>Continue with Google</span>
          </button>

          {/* Divider */}
          <div className="relative flex items-center justify-center gap-3 mb-4">
            <div className="border-t border-slate-800 flex-1" />
            <span className="shrink-0 whitespace-nowrap bg-slate-950 px-3 text-[10px] text-slate-500 uppercase tracking-[0.16em] font-mono">
              or continue with email
            </span>
            <div className="border-t border-slate-800 flex-1" />
          </div>

          {/* Email / Password Form */}
          <form onSubmit={handleEmailAuth} className="space-y-4">
            {isSignUp && (
              <div>
                <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                  Full Name
                </label>
                <div className="relative">
                  <User className="w-4 h-4 text-slate-500 absolute left-3.5 top-3.5" />
                  <input
                    type="text"
                    required
                    value={name}
                    onChange={(e) => setName(e.target.value)}
                    placeholder="e.g. Ayush Sharma"
                    className="w-full bg-slate-900/90 border border-slate-800 rounded-xl py-2.5 pl-10 pr-4 text-sm text-white placeholder-slate-500 focus:outline-none focus:border-cyan-500 focus:ring-1 focus:ring-cyan-500 transition-all"
                  />
                </div>
              </div>
            )}

            <div>
              <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                Email Address
              </label>
              <div className="relative">
                <Mail className="w-4 h-4 text-slate-500 absolute left-3.5 top-3.5" />
                <input
                  type="email"
                  required
                  value={email}
                  onChange={(e) => setEmail(e.target.value)}
                  placeholder="name@example.com"
                  className="w-full bg-slate-900/90 border border-slate-800 rounded-xl py-2.5 pl-10 pr-4 text-sm text-white placeholder-slate-500 focus:outline-none focus:border-cyan-500 focus:ring-1 focus:ring-cyan-500 transition-all"
                />
              </div>
            </div>

            <div>
              <div className="flex items-center justify-between mb-1.5">
                <label className="block text-xs font-semibold text-slate-300">
                  Password
                </label>
                {!isSignUp && (
                  <button
                    type="button"
                    onClick={() => { setShowForgotModal(true); setForgotEmail(email); }}
                    className="text-xs text-cyan-400 hover:text-cyan-300 transition-colors cursor-pointer"
                  >
                    Forgot Password?
                  </button>
                )}
              </div>
              <div className="relative">
                <Lock className="w-4 h-4 text-slate-500 absolute left-3.5 top-3.5" />
                <input
                  type="password"
                  required
                  value={password}
                  onChange={(e) => setPassword(e.target.value)}
                  placeholder="••••••••"
                  className="w-full bg-slate-900/90 border border-slate-800 rounded-xl py-2.5 pl-10 pr-4 text-sm text-white placeholder-slate-500 focus:outline-none focus:border-cyan-500 focus:ring-1 focus:ring-cyan-500 transition-all"
                />
              </div>
            </div>

            <button
              type="submit"
              disabled={loading}
              className="w-full mt-2 py-3 px-4 rounded-xl bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-slate-950 font-bold text-sm flex items-center justify-center gap-2 transition-all shadow-lg shadow-cyan-500/25 cursor-pointer disabled:opacity-50"
            >
              {loading ? (
                <div className="w-5 h-5 border-2 border-slate-950 border-t-transparent rounded-full animate-spin" />
              ) : (
                <>
                  <span>{isSignUp ? 'Create Medicare Account' : 'Sign In to Portal'}</span>
                  <ArrowRight className="w-4 h-4" />
                </>
              )}
            </button>
          </form>

          {/* Footer note */}
          <p className="mt-5 text-center text-[11px] text-slate-500">
            By signing in, you agree to MEDICARE+ Health Privacy Policy & HIPAA-compliant Cloud Storage.
          </p>
        </div>
      </div>

      {/* Forgot Password Modal */}
      {showForgotModal && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/70 backdrop-blur-sm p-4">
          <div className="bg-slate-900 border border-slate-800 rounded-2xl p-6 max-w-sm w-full shadow-2xl relative">
            <button
              onClick={() => setShowForgotModal(false)}
              className="absolute top-4 right-4 text-slate-400 hover:text-white"
            >
              <X className="w-5 h-5" />
            </button>

            <div className="w-10 h-10 rounded-xl bg-cyan-500/10 flex items-center justify-center mb-4 text-cyan-400">
              <HelpCircle className="w-6 h-6" />
            </div>

            <h3 className="text-lg font-bold text-white mb-1">Reset Password</h3>
            <p className="text-xs text-slate-400 mb-4">
              Enter your registered email address. We'll send you a secure link to reset your MEDICARE+ password.
            </p>

            <form onSubmit={handleForgotPassword} className="space-y-4">
              <input
                type="email"
                required
                value={forgotEmail}
                onChange={(e) => setForgotEmail(e.target.value)}
                placeholder="your.email@example.com"
                className="w-full bg-slate-950 border border-slate-800 rounded-xl py-2.5 px-3.5 text-sm text-white placeholder-slate-500 focus:outline-none focus:border-cyan-500"
              />
              <button
                type="submit"
                disabled={forgotLoading}
                className="w-full py-2.5 rounded-xl bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-bold text-sm transition-all shadow-md shadow-cyan-500/20"
              >
                {forgotLoading ? 'Sending Link...' : 'Send Reset Link'}
              </button>
            </form>
          </div>
        </div>
      )}
    </div>
  );
}

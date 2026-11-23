import React, { useState } from 'react';
import { loginWithGoogle, loginWithEmail, registerWithEmail, resetPassword, saveUserProfileToDb, generateMedicareId } from '../firebase';
import { Activity, Heart, ShieldCheck, Cpu, Pill, FileText, Mail, Lock, User, ArrowRight, AlertCircle, CheckCircle2, X, HelpCircle } from 'lucide-react';

export default function AuthScreen({ onLoginSuccess, theme = 'light' }) {
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
    setErrorMsg(''); setSuccessMsg(''); setLoading(true);
    try {
      if (isSignUp) {
        if (!name.trim()) throw new Error('Please enter your full name');
        const userCred = await registerWithEmail(email, password, name);
        await saveUserProfileToDb(userCred.user.uid, {
          name, email: userCred.user.email, medicareId: generateMedicareId(userCred.user.uid), createdAt: new Date().toISOString()
        });
        setSuccessMsg('Account created successfully! Welcome to MEDICARE+');
      } else {
        await loginWithEmail(email, password);
        setSuccessMsg('Welcome back to MEDICARE+');
      }
      if (onLoginSuccess) onLoginSuccess();
    } catch (err) {
      let message = err.message || 'Authentication failed';
      if (err.code === 'auth/invalid-credential' || err.code === 'auth/wrong-password') message = 'Invalid email or password.';
      else if (err.code === 'auth/email-already-in-use') message = 'This email is already registered. Please login.';
      else if (err.code === 'auth/weak-password') message = 'Password should be at least 6 characters.';
      setErrorMsg(message);
    } finally { setLoading(false); }
  };

  const handleGoogleAuth = async () => {
    setErrorMsg(''); setLoading(true);
    try {
      const res = await loginWithGoogle();
      if (!res) return;
      if (onLoginSuccess) onLoginSuccess();
    } catch (err) { setErrorMsg(err.message || 'Google authentication failed.'); }
    finally { setLoading(false); }
  };

  const handleForgotPassword = async (e) => {
    e.preventDefault();
    if (!forgotEmail) { setErrorMsg('Please enter your email to receive reset instructions.'); return; }
    setForgotLoading(true);
    try {
      await resetPassword(forgotEmail);
      setSuccessMsg(`Password reset link sent to ${forgotEmail}.`);
      setShowForgotModal(false); setForgotEmail('');
    } catch (err) { setErrorMsg(err.message || 'Failed to send password reset email.'); }
    finally { setForgotLoading(false); }
  };

  return (
    <main className={`auth-page ${theme === 'dark' ? 'auth-dark' : ''}`}>
      <section className="auth-showcase">
        <div className="auth-brand"><span className="brand-mark"><Heart /></span><div><strong>MEDICARE<span>+</span></strong><small>Smart health companion</small></div></div>
        <div className="auth-showcase-copy">
          <span className="eyebrow"><Activity /> Connected care</span>
          <h1>One secure place for your everyday health.</h1>
          <p>Monitor live vitals, manage medications and keep a clear health record with your connected MEDICARE+ kit.</p>
          <div className="auth-features">
            <div><span><Activity /></span><strong>Live vitals</strong><small>ECG and oxygen monitoring</small></div>
            <div><span><Pill /></span><strong>Smart reminders</strong><small>Synced with your device</small></div>
            <div><span><FileText /></span><strong>Health reports</strong><small>Clean clinical summaries</small></div>
            <div><span><ShieldCheck /></span><strong>Secure profile</strong><small>Private cloud account</small></div>
          </div>
        </div>
        <div className="auth-footer"><span><Cpu /> ESP32 connected</span><span><ShieldCheck /> Cloud protected</span></div>
      </section>

      <section className="auth-panel">
        <div className="auth-card">
          <div className="auth-mobile-brand"><span className="brand-mark"><Heart /></span><strong>MEDICARE<span>+</span></strong></div>
          <header>
            <span className="eyebrow">Patient account</span>
            <h2>{isSignUp ? 'Create your account' : 'Welcome back'}</h2>
            <p>{isSignUp ? 'Set up your profile and connected medical kit.' : 'Sign in to open your personal health dashboard.'}</p>
          </header>

          <div className="auth-tabs">
            <button type="button" className={!isSignUp ? 'active' : ''} onClick={() => { setIsSignUp(false); setErrorMsg(''); setSuccessMsg(''); }}>Sign in</button>
            <button type="button" className={isSignUp ? 'active' : ''} onClick={() => { setIsSignUp(true); setErrorMsg(''); setSuccessMsg(''); }}>Create account</button>
          </div>

          {errorMsg && <div className="auth-alert error"><AlertCircle /><span>{errorMsg}</span></div>}
          {successMsg && <div className="auth-alert success"><CheckCircle2 /><span>{successMsg}</span></div>}

          <button type="button" className="google-button" onClick={handleGoogleAuth} disabled={loading}>
            <svg viewBox="0 0 24 24"><path fill="#4285F4" d="M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92c-.26 1.37-1.04 2.53-2.21 3.31v2.77h3.57c2.08-1.92 3.28-4.74 3.28-8.09z"/><path fill="#34A853" d="M12 23c2.97 0 5.46-.98 7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z"/><path fill="#FBBC05" d="M5.84 14.09c-.22-.66-.35-1.36-.35-2.09s.13-1.43.35-2.09V7.06H2.18C1.43 8.55 1 10.22 1 12s.43 3.45 1.18 4.94l2.85-2.22.81-.63z"/><path fill="#EA4335" d="M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 7.7 1 3.99 3.47 2.18 7.06l3.66 2.84c.87-2.6 3.3-4.52 6.16-4.52z"/></svg>
            Continue with Google
          </button>
          <div className="auth-divider"><span>or use email</span></div>

          <form onSubmit={handleEmailAuth} className="auth-form">
            {isSignUp && <label><span>Full name</span><div><User /><input required value={name} onChange={(e) => setName(e.target.value)} placeholder="Your full name" /></div></label>}
            <label><span>Email address</span><div><Mail /><input type="email" required value={email} onChange={(e) => setEmail(e.target.value)} placeholder="name@example.com" /></div></label>
            <label>
              <span className="password-label"><span>Password</span>{!isSignUp && <button type="button" onClick={() => { setShowForgotModal(true); setForgotEmail(email); }}>Forgot password?</button>}</span>
              <div><Lock /><input type="password" required value={password} onChange={(e) => setPassword(e.target.value)} placeholder="Enter password" /></div>
            </label>
            <button type="submit" className="primary-auth-button" disabled={loading}>
              {loading ? <span className="auth-loader-ring small" /> : <>{isSignUp ? 'Create account' : 'Sign in'}<ArrowRight /></>}
            </button>
          </form>
          <p className="auth-privacy">Protected access to your personal health information.</p>
        </div>
      </section>

      {showForgotModal && (
        <div className="modal-backdrop">
          <div className="modal-card">
            <button className="modal-close" onClick={() => setShowForgotModal(false)}><X /></button>
            <span className="modal-icon"><HelpCircle /></span>
            <h3>Reset password</h3><p>Enter your registered email to receive a secure reset link.</p>
            <form onSubmit={handleForgotPassword}>
              <input type="email" required value={forgotEmail} onChange={(e) => setForgotEmail(e.target.value)} placeholder="name@example.com" />
              <button type="submit" disabled={forgotLoading}>{forgotLoading ? 'Sending...' : 'Send reset link'}</button>
            </form>
          </div>
        </div>
      )}
    </main>
  );
}

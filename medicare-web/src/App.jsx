import React, { lazy, Suspense, useState, useEffect } from 'react';
import { onAuthStateChanged } from 'firebase/auth';
import {
  auth,
  logoutUser,
  subscribeToLiveVitals,
  subscribeToMedicines,
  getUserProfileFromDb,
  saveUserProfileToDb,
  generateMedicareId,
  onPermissionWarning
} from './firebase';

import SplashScreen from './components/SplashScreen';
import AuthScreen from './components/AuthScreen';
import Sidebar from './components/Sidebar';
import { getEcgConnectionState } from './sensorStatus';

import HomePage from './pages/HomePage';
const TestPage = lazy(() => import('./pages/TestPage'));
const ReportPage = lazy(() => import('./pages/ReportPage'));
const MedicinePage = lazy(() => import('./pages/MedicinePage'));
const HelpPage = lazy(() => import('./pages/HelpPage'));
const ProfilePage = lazy(() => import('./pages/ProfilePage'));

import {
  Activity, FileText, Pill, HelpCircle, Heart, User,
  AlertTriangle, Copy, Check, X, ChevronDown, LogOut, Settings, Menu
} from 'lucide-react';

export default function App() {
  const [showSplash, setShowSplash] = useState(true);
  const [user, setUser] = useState(null);
  const [userProfile, setUserProfile] = useState(null);
  const [authLoading, setAuthLoading] = useState(true);
  const [activeTab, setActiveTab] = useState('HOME');
  const [isSidebarCollapsed, setIsSidebarCollapsed] = useState(false);
  const [mobileProfileOpen, setMobileProfileOpen] = useState(false);
  const [mobileMenuOpen, setMobileMenuOpen] = useState(false);
  const [theme, setTheme] = useState(() => {
    try {
      return localStorage.getItem('medicare-theme') === 'dark' ? 'dark' : 'light';
    } catch {
      return 'light';
    }
  });
  const [hasPermissionWarning, setHasPermissionWarning] = useState(false);
  const [rulesCopied, setRulesCopied] = useState(false);
  const [dismissBanner, setDismissBanner] = useState(false);
  const [liveVitals, setLiveVitals] = useState(null);
  const [medicines, setMedicines] = useState([]);
  const [sensorStatusClock, setSensorStatusClock] = useState(Date.now());
  const ecgConnected = getEcgConnectionState(liveVitals?.ecg, sensorStatusClock) === 'connected';

  useEffect(() => {
    const timer = setInterval(() => setSensorStatusClock(Date.now()), 5000);
    return () => clearInterval(timer);
  }, []);

  useEffect(() => {
    try {
      localStorage.setItem('medicare-theme', theme);
    } catch {
      // Theme persistence is optional if browser storage is unavailable.
    }
  }, [theme]);

  const navItems = [
    { id: 'HOME', label: 'Overview', icon: Heart },
    { id: 'TEST', label: 'Live Tests', icon: Activity },
    { id: 'REPORT', label: 'Reports', icon: FileText },
    { id: 'MEDICINE', label: 'Medicines', icon: Pill },
    { id: 'HELP', label: 'Help', icon: HelpCircle }
  ];

  useEffect(() => {
    const unsub = onPermissionWarning((warn) => setHasPermissionWarning(warn));
    return () => unsub();
  }, []);

  useEffect(() => {
    const unsubscribe = onAuthStateChanged(auth, async (currentUser) => {
      setUser(currentUser);
      if (currentUser) {
        try {
          let profile = await getUserProfileFromDb(currentUser.uid);
          if (!profile || !profile.medicareId) {
            const firstProfile = {
              ...(profile || {}),
              name: profile?.name || currentUser.displayName || currentUser.email?.split('@')[0] || 'Medicare User',
              email: currentUser.email || profile?.email || '',
              photoURL: profile?.photoURL || currentUser.photoURL || null,
              medicareId: profile?.medicareId || generateMedicareId(currentUser.uid),
              createdAt: profile?.createdAt || new Date().toISOString()
            };
            await saveUserProfileToDb(currentUser.uid, firstProfile);
            profile = firstProfile;
          }
          setUserProfile({
            ...(profile || {}),
            photoURL: profile?.photoURL || currentUser.photoURL || null
          });
        } catch (err) {
          console.warn('Failed to load user profile from cloud, using local:', err);
        }
      } else {
        setUserProfile(null);
      }
      setAuthLoading(false);
    });
    return () => unsubscribe();
  }, []);

  useEffect(() => {
    if (!user) return;
    const unsubVitals = subscribeToLiveVitals(setLiveVitals);
    const unsubMeds = subscribeToMedicines(setMedicines);
    return () => {
      if (unsubVitals) unsubVitals();
      if (unsubMeds) unsubMeds();
    };
  }, [user]);

  const handleLogout = async () => {
    await logoutUser();
    setUser(null);
    setUserProfile(null);
    setActiveTab('HOME');
    setMobileProfileOpen(false);
  };

  const copyFirebaseRules = () => {
    navigator.clipboard.writeText(JSON.stringify({ rules: { '.read': true, '.write': true } }, null, 2));
    setRulesCopied(true);
    setTimeout(() => setRulesCopied(false), 3000);
  };

  if (showSplash) return <SplashScreen onComplete={() => setShowSplash(false)} theme={theme} />;
  if (authLoading) return <div className="auth-loader"><div className="auth-loader-ring" /></div>;
  if (!user) return <AuthScreen onLoginSuccess={() => {}} theme={theme} />;

  const displayName = userProfile?.name || user?.displayName || user?.email?.split('@')[0] || 'Patient';
  const photoURL = userProfile?.photoURL || user?.photoURL;
  const medicareId = userProfile?.medicareId || 'MED-PENDING';
  const pageLabel = navItems.find((item) => item.id === activeTab)?.label || activeTab;

  return (
    <div className={`app-shell ${theme === 'light' ? 'theme-light' : 'theme-dark'}`}>
      <Sidebar
        activeTab={activeTab}
        setActiveTab={(tab) => { setActiveTab(tab); setMobileMenuOpen(false); }}
        user={user}
        userProfile={userProfile}
        onLogout={handleLogout}
        isCollapsed={isSidebarCollapsed}
        setIsCollapsed={setIsSidebarCollapsed}
        theme={theme}
        setTheme={setTheme}
        mobileOpen={mobileMenuOpen}
        setMobileOpen={setMobileMenuOpen}
      />

      <div className={`portal-main ${isSidebarCollapsed ? 'sidebar-collapsed' : ''}`}>
        <header className="portal-header">
          <div className="header-brand-mobile">
            <button className="icon-button menu-button" onClick={() => setMobileMenuOpen(true)} aria-label="Open menu">
              <Menu />
            </button>
            <button className="mobile-brand" onClick={() => setActiveTab('HOME')}>
              <span className="brand-mark"><Heart /></span>
              <span>MEDICARE<span>+</span></span>
            </button>
          </div>

          <div className="header-title">
            <span>Patient portal</span>
            <strong>{pageLabel}</strong>
          </div>

          <div className="header-actions">
            <div className={`device-status ${ecgConnected ? 'connected' : ''}`}>
              <span />
              <div>
                <small>Device link</small>
                <strong>{ecgConnected ? 'Connected' : 'Disconnected'}</strong>
              </div>
            </div>
            <div className="profile-menu-wrap">
              <button className="profile-trigger" onClick={() => setMobileProfileOpen(!mobileProfileOpen)}>
                {photoURL ? (
                  <img src={photoURL} alt={displayName} referrerPolicy="no-referrer" />
                ) : (
                  <span className="profile-avatar">{displayName.charAt(0).toUpperCase()}</span>
                )}
                <span className="profile-copy">
                  <strong>{displayName}</strong>
                  <small>{medicareId}</small>
                </span>
                <ChevronDown className={mobileProfileOpen ? 'rotated' : ''} />
              </button>
              {mobileProfileOpen && (
                <div className="profile-menu">
                  <button onClick={() => { setActiveTab('PROFILE'); setMobileProfileOpen(false); }}>
                    <User /> View profile
                  </button>
                  <button onClick={() => { setActiveTab('PROFILE'); setMobileProfileOpen(false); }}>
                    <Settings /> Settings
                  </button>
                  <button className="logout-option" onClick={handleLogout}>
                    <LogOut /> Sign out
                  </button>
                </div>
              )}
            </div>
          </div>
        </header>

        {hasPermissionWarning && !dismissBanner && (
          <div className="permission-banner">
            <div><AlertTriangle /><span><strong>Cloud permissions need attention.</strong> Firebase Realtime Database rules are locked.</span></div>
            <div className="permission-actions">
              <button onClick={copyFirebaseRules}>{rulesCopied ? <Check /> : <Copy />}{rulesCopied ? 'Copied' : 'Copy rules'}</button>
              <button className="dismiss" onClick={() => setDismissBanner(true)} aria-label="Dismiss"><X /></button>
            </div>
          </div>
        )}

        <main className="portal-content">
          <Suspense fallback={<div className="page-loader"><div className="auth-loader-ring" /></div>}>
            {activeTab === 'HOME' && <HomePage onNavigate={setActiveTab} liveVitals={liveVitals} medicines={medicines} user={user} userProfile={userProfile} />}
            {activeTab === 'TEST' && <TestPage liveVitals={liveVitals} />}
            {activeTab === 'REPORT' && <ReportPage user={user} userProfile={userProfile} />}
            {activeTab === 'MEDICINE' && <MedicinePage user={user} userProfile={userProfile} medicines={medicines} />}
            {activeTab === 'HELP' && <HelpPage />}
            {activeTab === 'PROFILE' && (
              <ProfilePage user={user} userProfile={userProfile} onProfileUpdated={setUserProfile} theme={theme} setTheme={setTheme} onLogout={handleLogout} liveVitals={liveVitals} />
            )}
          </Suspense>
        </main>
      </div>

      <nav className="mobile-bottom-nav" aria-label="Primary navigation">
        {navItems.map((item) => {
          const Icon = item.icon;
          return (
            <button key={item.id} onClick={() => setActiveTab(item.id)} className={activeTab === item.id ? 'active' : ''}>
              <Icon /><span>{item.label}</span>
            </button>
          );
        })}
      </nav>
    </div>
  );
}

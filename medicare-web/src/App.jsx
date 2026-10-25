import React, { useState, useEffect } from 'react';
import { onAuthStateChanged } from 'firebase/auth';
import { 
  auth, 
  logoutUser, 
  subscribeToLiveVitals, 
  subscribeToMedicines, 
  getUserProfileFromDb,
  onPermissionWarning 
} from './firebase';

import SplashScreen from './components/SplashScreen';
import AuthScreen from './components/AuthScreen';
import Sidebar from './components/Sidebar';

import HomePage from './pages/HomePage';
import TestPage from './pages/TestPage';
import ReportPage from './pages/ReportPage';
import MedicinePage from './pages/MedicinePage';
import HelpPage from './pages/HelpPage';
import ProfilePage from './pages/ProfilePage';

import { 
  Home,
  Activity, 
  FileText,
  Pill,
  HelpCircle,
  Heart, 
  Wifi, 
  Moon, 
  Sun, 
  Menu, 
  Bell, 
  ShieldCheck,
  IdCard,
  User,
  AlertTriangle,
  Copy,
  Check,
  X,
  ChevronDown,
  LogOut,
  Settings
} from 'lucide-react';

export default function App() {
  // Application phase states
  const [showSplash, setShowSplash] = useState(true);
  const [user, setUser] = useState(null);
  const [userProfile, setUserProfile] = useState(null);
  const [authLoading, setAuthLoading] = useState(true);

  // Navigation and Layout states
  const [activeTab, setActiveTab] = useState('HOME'); // 'HOME' | 'TEST' | 'REPORT' | 'MEDICINE' | 'HELP' | 'PROFILE'
  const [isSidebarCollapsed, setIsSidebarCollapsed] = useState(false);
  const [mobileProfileOpen, setMobileProfileOpen] = useState(false);
  const [theme, setTheme] = useState('dark');

  // Firebase Permission Warning Banner state
  const [hasPermissionWarning, setHasPermissionWarning] = useState(false);
  const [rulesCopied, setRulesCopied] = useState(false);
  const [dismissBanner, setDismissBanner] = useState(false);

  // Real-time Cloud Data
  const [liveVitals, setLiveVitals] = useState(null);
  const [medicines, setMedicines] = useState([]);

  // Mobile Bottom Navigation items
  const navItems = [
    { id: 'HOME', label: 'Home', icon: Home },
    { id: 'TEST', label: 'Test', icon: Activity },
    { id: 'REPORT', label: 'Report', icon: FileText },
    { id: 'MEDICINE', label: 'Meds', icon: Pill },
    { id: 'HELP', label: 'Help', icon: HelpCircle },
  ];

  // Listen for permission warnings from Firebase
  useEffect(() => {
    const unsub = onPermissionWarning((warn) => {
      setHasPermissionWarning(warn);
    });
    return () => unsub();
  }, []);

  // Auth Listener
  useEffect(() => {
    const unsubscribe = onAuthStateChanged(auth, async (currentUser) => {
      setUser(currentUser);
      if (currentUser) {
        try {
          const profile = await getUserProfileFromDb(currentUser.uid);
          if (profile) setUserProfile(profile);
        } catch (err) {
          console.warn("Failed to load user profile from cloud, using local:", err);
        }
      } else {
        setUserProfile(null);
      }
      setAuthLoading(false);
    });

    return () => unsubscribe();
  }, []);

  // Real-time Firebase Sync for Vitals and Medicines
  useEffect(() => {
    if (!user) return;

    const unsubVitals = subscribeToLiveVitals((data) => {
      setLiveVitals(data);
    });

    const unsubMeds = subscribeToMedicines((list) => {
      setMedicines(list);
    });

    return () => {
      if (unsubVitals) unsubVitals();
      if (unsubMeds) unsubMeds();
    };
  }, [user]);

  // Handle Logout
  const handleLogout = async () => {
    await logoutUser();
    setUser(null);
    setUserProfile(null);
    setActiveTab('HOME');
    setMobileProfileOpen(false);
  };

  const copyFirebaseRules = () => {
    const rulesText = JSON.stringify({
      "rules": {
        ".read": true,
        ".write": true
      }
    }, null, 2);

    navigator.clipboard.writeText(rulesText);
    setRulesCopied(true);
    setTimeout(() => setRulesCopied(false), 3000);
  };

  // 1. Show 5-second Splash Screen first
  if (showSplash) {
    return <SplashScreen onComplete={() => setShowSplash(false)} />;
  }

  // 2. If Auth is still initializing
  if (authLoading) {
    return (
      <div className="min-h-screen bg-slate-950 flex items-center justify-center text-cyan-400">
        <div className="w-10 h-10 border-4 border-cyan-500 border-t-transparent rounded-full animate-spin" />
      </div>
    );
  }

  // 3. If User is not logged in, show AuthScreen (Login / Signup)
  if (!user) {
    return (
      <AuthScreen 
        onLoginSuccess={() => {
          // Handled by onAuthStateChanged
        }} 
      />
    );
  }

  // 4. Main Authenticated Application Dashboard
  const displayName = userProfile?.name || user?.displayName || user?.email?.split('@')[0] || 'Patient';
  const photoURL = userProfile?.photoURL || user?.photoURL;
  const medicareId = userProfile?.medicareId || 'MED-PENDING';

  return (
    <div className={`min-h-screen ${theme === 'dark' ? 'bg-slate-950 text-slate-100' : 'bg-slate-100 text-slate-900'} flex transition-colors duration-200 overflow-x-hidden`}>
      {/* Desktop Collapsible Sidebar (Hidden on Mobile) */}
      <div className="hidden md:block">
        <Sidebar
          activeTab={activeTab}
          setActiveTab={setActiveTab}
          user={user}
          userProfile={userProfile}
          onLogout={handleLogout}
          isCollapsed={isSidebarCollapsed}
          setIsCollapsed={setIsSidebarCollapsed}
          theme={theme}
          setTheme={setTheme}
        />
      </div>

      {/* Main Content Area: Pl-0 on mobile, Pl-20/Pl-64 on desktop */}
      <div 
        className={`flex-1 flex flex-col min-w-0 transition-all duration-300 w-full ${
          isSidebarCollapsed ? 'md:pl-20' : 'md:pl-64'
        }`}
      >
        {/* Top Navbar */}
        <header className="sticky top-0 z-30 h-16 sm:h-18 backdrop-blur-xl bg-slate-950/90 border-b border-slate-800/80 px-4 sm:px-8 flex items-center justify-between">
          {/* Left Side: Logo with Beating Heart on Mobile; Tab title on Desktop */}
          <div className="flex items-center gap-3">
            {/* Mobile Brand Logo with Animated Cardiac Beep */}
            <div 
              onClick={() => setActiveTab('HOME')}
              className="flex md:hidden items-center gap-2.5 cursor-pointer select-none"
            >
              <div className="relative flex items-center justify-center w-8 h-8 rounded-xl bg-cyan-500/15 border border-cyan-500/30 shadow-md shadow-cyan-500/10">
                <Activity className="w-4 h-4 text-cyan-400" />
                <Heart className="w-2.5 h-2.5 text-rose-500 fill-rose-500 absolute -top-1 -right-1 animate-heartbeat drop-shadow-[0_0_8px_rgba(244,63,94,0.9)]" />
              </div>
              <span className="font-black text-lg text-white tracking-tight">
                MEDICARE<span className="text-cyan-400">+</span>
              </span>
            </div>

            {/* Desktop Section Header */}
            <div className="hidden md:flex items-center gap-3">
              <div className="relative flex items-center justify-center w-8 h-8 rounded-xl bg-cyan-500/15 border border-cyan-500/30">
                <Activity className="w-4 h-4 text-cyan-400" />
                <Heart className="w-2.5 h-2.5 text-rose-500 fill-rose-500 absolute -top-1 -right-1 animate-heartbeat drop-shadow-[0_0_8px_rgba(244,63,94,0.9)]" />
              </div>
              <h2 className="text-xl font-black text-white tracking-tight flex items-center gap-2">
                <span>{activeTab}</span>
                <span className="text-xs font-normal text-slate-500">
                  &bull; MEDICARE+ Clinical Portal
                </span>
              </h2>
            </div>
          </div>

          {/* Right Header: Status Pill & Profile with Dropdown on Both Mobile & Desktop */}
          <div className="flex items-center gap-3 sm:gap-4 relative">
            {/* Live ESP32 Link Status */}
            <div className="hidden sm:flex items-center gap-2 px-3 py-1.5 rounded-full bg-slate-900 border border-slate-800 text-xs">
              <span className="relative flex h-2 w-2">
                <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"></span>
                <span className="relative inline-flex rounded-full h-2 w-2 bg-emerald-500"></span>
              </span>
              <span className="text-slate-300 font-mono text-[11px]">ESP32 Link: Connected</span>
            </div>

            {/* Profile Dropdown Trigger */}
            <div className="relative">
              <button
                onClick={() => setMobileProfileOpen(!mobileProfileOpen)}
                className="flex items-center gap-2 p-1 sm:p-1.5 sm:pr-3 rounded-full bg-slate-900 hover:bg-slate-800 border border-slate-800 transition-all cursor-pointer"
              >
                {photoURL ? (
                  <img
                    src={photoURL}
                    alt={displayName}
                    className="w-7 h-7 sm:w-8 sm:h-8 rounded-full object-cover ring-2 ring-cyan-500/40"
                  />
                ) : (
                  <div className="w-7 h-7 sm:w-8 sm:h-8 rounded-full bg-gradient-to-tr from-cyan-600 to-blue-600 flex items-center justify-center text-xs font-bold text-white ring-2 ring-cyan-500/40">
                    {displayName.charAt(0).toUpperCase()}
                  </div>
                )}
                <span className="text-xs font-semibold text-slate-200 hidden sm:inline truncate max-w-[110px]">
                  {displayName}
                </span>
                <ChevronDown className={`w-3.5 h-3.5 text-slate-400 transition-transform ${mobileProfileOpen ? 'rotate-180' : ''}`} />
              </button>

              {/* Profile Dropdown Menu */}
              {mobileProfileOpen && (
                <div className="absolute right-0 mt-2 w-56 rounded-2xl bg-slate-900 border border-slate-800 shadow-2xl z-50 p-2 backdrop-blur-2xl">
                  <div className="px-3 py-2 border-b border-slate-800">
                    <p className="text-xs font-bold text-white truncate">{displayName}</p>
                    <p className="text-[10px] font-mono text-cyan-400 truncate">{medicareId}</p>
                  </div>

                  <button
                    onClick={() => {
                      setActiveTab('PROFILE');
                      setMobileProfileOpen(false);
                    }}
                    className="w-full flex items-center gap-2.5 px-3 py-2.5 text-xs text-slate-300 hover:text-white hover:bg-slate-800 rounded-xl transition-colors cursor-pointer mt-1"
                  >
                    <User className="w-4 h-4 text-cyan-400" />
                    <span>View Profile & ID</span>
                  </button>

                  <button
                    onClick={() => {
                      setActiveTab('PROFILE');
                      setMobileProfileOpen(false);
                    }}
                    className="w-full flex items-center gap-2.5 px-3 py-2.5 text-xs text-slate-300 hover:text-white hover:bg-slate-800 rounded-xl transition-colors cursor-pointer"
                  >
                    <Settings className="w-4 h-4 text-slate-400" />
                    <span>Settings & Theme</span>
                  </button>

                  <div className="border-t border-slate-800 my-1" />

                  <button
                    onClick={handleLogout}
                    className="w-full flex items-center gap-2.5 px-3 py-2.5 text-xs text-rose-400 hover:bg-rose-500/10 rounded-xl transition-colors cursor-pointer"
                  >
                    <LogOut className="w-4 h-4 text-rose-400" />
                    <span>Sign Out</span>
                  </button>
                </div>
              )}
            </div>
          </div>
        </header>

        {/* FIREBASE REALTIME DATABASE RULES HELPER BANNER */}
        {hasPermissionWarning && !dismissBanner && (
          <div className="bg-gradient-to-r from-amber-950/80 via-slate-900 to-amber-950/80 border-b border-amber-500/30 p-3 sm:p-4 text-xs">
            <div className="max-w-6xl mx-auto flex flex-col md:flex-row items-start md:items-center justify-between gap-3">
              <div className="flex items-start gap-2.5">
                <AlertTriangle className="w-4 h-4 text-amber-400 shrink-0 mt-0.5" />
                <div>
                  <span className="font-bold text-amber-200">
                    Firebase Realtime Database Rules are currently locked:
                  </span>
                  <p className="text-slate-300 text-[11px] mt-0.5">
                    Open <strong>Firebase Console &rarr; Realtime Database &rarr; Rules</strong> and paste <code>{`{ "rules": { ".read": true, ".write": true } }`}</code> to allow ESP32 & web sync.
                  </p>
                </div>
              </div>

              <div className="flex items-center gap-2 shrink-0 self-end md:self-auto">
                <button
                  onClick={copyFirebaseRules}
                  className="px-3 py-1.5 rounded-lg bg-amber-500 hover:bg-amber-400 text-slate-950 font-bold text-[11px] flex items-center gap-1.5 transition-all cursor-pointer shadow-sm"
                >
                  {rulesCopied ? <Check className="w-3.5 h-3.5" /> : <Copy className="w-3.5 h-3.5" />}
                  <span>{rulesCopied ? 'Rules Copied!' : 'Copy Rules JSON'}</span>
                </button>
                <button
                  onClick={() => setDismissBanner(true)}
                  className="p-1.5 rounded-lg text-slate-400 hover:text-white transition-colors cursor-pointer"
                  title="Dismiss warning"
                >
                  <X className="w-4 h-4" />
                </button>
              </div>
            </div>
          </div>
        )}

        {/* Dynamic Page Views: Added pb-28 on mobile so content never overlaps with floating bottom bar */}
        <main className="flex-1 p-4 sm:p-6 lg:p-8 max-w-7xl w-full mx-auto pb-28 md:pb-12">
          {activeTab === 'HOME' && (
            <HomePage 
              onNavigate={(tab) => setActiveTab(tab)} 
              liveVitals={liveVitals} 
              medicines={medicines} 
            />
          )}

          {activeTab === 'TEST' && (
            <TestPage 
              liveVitals={liveVitals} 
            />
          )}

          {activeTab === 'REPORT' && (
            <ReportPage 
              user={user} 
              userProfile={userProfile} 
            />
          )}

          {activeTab === 'MEDICINE' && (
            <MedicinePage 
              user={user} 
              userProfile={userProfile} 
              medicines={medicines} 
            />
          )}

          {activeTab === 'HELP' && (
            <HelpPage />
          )}

          {activeTab === 'PROFILE' && (
            <ProfilePage 
              user={user} 
              userProfile={userProfile} 
              onProfileUpdated={(updated) => setUserProfile(updated)}
              theme={theme}
              setTheme={setTheme}
              onLogout={handleLogout}
            />
          )}
        </main>
      </div>

      {/* MOBILE FLOATING ELEVATED BOTTOM NAVIGATION BAR (Icons Only, Modern App Style) */}
      <nav className="md:hidden fixed bottom-4 left-4 right-4 z-40 rounded-2xl bg-slate-900/90 border border-slate-700/80 backdrop-blur-2xl shadow-2xl py-2 px-3 flex justify-around items-center">
        {navItems.map((item) => {
          const IconComponent = item.icon;
          const isActive = activeTab === item.id;
          return (
            <button
              key={item.id}
              onClick={() => setActiveTab(item.id)}
              className={`p-2.5 rounded-xl transition-all relative flex flex-col items-center justify-center cursor-pointer ${
                isActive
                  ? 'bg-gradient-to-tr from-cyan-500/20 to-blue-500/20 text-cyan-400 border border-cyan-500/40 shadow-lg shadow-cyan-500/20 scale-105'
                  : 'text-slate-400 hover:text-white'
              }`}
              title={item.label}
            >
              <IconComponent className={`w-5 h-5 ${isActive ? 'text-cyan-400 animate-pulse' : 'text-slate-400'}`} />
              {isActive && (
                <span className="w-1 h-1 rounded-full bg-cyan-400 absolute -bottom-1 shadow-sm shadow-cyan-400" />
              )}
            </button>
          );
        })}
      </nav>
    </div>
  );
}

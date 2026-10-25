import React, { useState } from 'react';
import { 
  Home, 
  Activity, 
  FileText, 
  Pill, 
  HelpCircle, 
  ChevronLeft, 
  ChevronRight, 
  User, 
  LogOut, 
  Settings, 
  ChevronUp, 
  Shield, 
  Wifi, 
  HeartPulse,
  Heart,
  X
} from 'lucide-react';

export default function Sidebar({ 
  activeTab, 
  setActiveTab, 
  user, 
  userProfile, 
  onLogout, 
  isCollapsed, 
  setIsCollapsed,
  theme,
  setTheme,
  mobileOpen,
  setMobileOpen
}) {
  const [profileDropdownOpen, setProfileDropdownOpen] = useState(false);

  const navItems = [
    { id: 'HOME', label: 'HOME', icon: Home, desc: 'Overview & Device' },
    { id: 'TEST', label: 'TEST', icon: Activity, desc: 'SpO2 & ECG Live' },
    { id: 'REPORT', label: 'REPORT', icon: FileText, desc: 'PDF Downloads' },
    { id: 'MEDICINE', label: 'MEDICINE', icon: Pill, desc: 'Hardware Schedule' },
    { id: 'HELP', label: 'HELP', icon: HelpCircle, desc: 'Interactive Guide' },
  ];

  const displayName = userProfile?.name || user?.displayName || user?.email?.split('@')[0] || 'Patient User';
  const displayEmail = user?.email || userProfile?.email || 'patient@medicare.io';
  const photoURL = userProfile?.photoURL || user?.photoURL;
  const medicareId = userProfile?.medicareId || 'MED-PENDING';

  const handleNavClick = (tabId) => {
    setActiveTab(tabId);
    if (setMobileOpen) {
      setMobileOpen(false);
    }
  };

  return (
    <>
      {/* Mobile Backdrop Overlay */}
      {mobileOpen && (
        <div 
          onClick={() => setMobileOpen(false)}
          className="fixed inset-0 bg-black/60 backdrop-blur-sm z-40 md:hidden transition-opacity"
        />
      )}

      {/* Sidebar Drawer */}
      <aside 
        className={`fixed top-0 bottom-0 left-0 z-50 bg-slate-900/95 border-r border-slate-800 backdrop-blur-2xl transition-all duration-300 flex flex-col justify-between ${
          mobileOpen ? 'translate-x-0 w-64' : '-translate-x-full md:translate-x-0'
        } ${
          isCollapsed ? 'md:w-20' : 'md:w-64'
        }`}
      >
        {/* Top Header: Logo + Collapse/Close Buttons */}
        <div>
          {isCollapsed && !mobileOpen ? (
            /* COLLAPSED HEADER: Centered Logo + Centered Expand Button (Never cut off!) */
            <div className="py-4 flex flex-col items-center justify-center border-b border-slate-800/80 gap-2">
              <div 
                className="w-11 h-11 rounded-2xl bg-gradient-to-tr from-cyan-500 to-blue-600 p-0.5 shadow-lg shadow-cyan-500/20 cursor-pointer transition-transform hover:scale-105 relative flex items-center justify-center"
                onClick={() => handleNavClick('HOME')}
                title="MEDICARE+ (Click for Home)"
              >
                <div className="w-full h-full bg-slate-950 rounded-[14px] flex items-center justify-center relative">
                  <Activity className="w-5 h-5 text-cyan-400" />
                  <Heart className="w-2.5 h-2.5 text-rose-500 fill-rose-500 absolute -top-1 -right-1 animate-heartbeat" />
                </div>
              </div>
              <button
                onClick={() => setIsCollapsed(false)}
                className="p-1 rounded-lg text-slate-400 hover:text-white hover:bg-slate-800 transition-colors cursor-pointer"
                title="Expand Sidebar"
              >
                <ChevronRight className="w-4 h-4" />
              </button>
            </div>
          ) : (
            /* EXPANDED HEADER */
            <div className="h-18 flex items-center justify-between px-4 border-b border-slate-800/80">
              <div 
                className="flex items-center gap-3 overflow-hidden cursor-pointer" 
                onClick={() => handleNavClick('HOME')}
              >
                <div className="w-10 h-10 rounded-xl bg-gradient-to-tr from-cyan-500 to-blue-600 p-0.5 shrink-0 shadow-lg shadow-cyan-500/20 flex items-center justify-center relative">
                  <div className="w-full h-full bg-slate-950 rounded-[10px] flex items-center justify-center relative">
                    <Activity className="w-5 h-5 text-cyan-400" />
                    <Heart className="w-2.5 h-2.5 text-rose-500 fill-rose-500 absolute -top-1 -right-1 animate-heartbeat" />
                  </div>
                </div>
                <div className="transition-opacity duration-300">
                  <span className="text-lg font-black tracking-tight text-white flex items-center">
                    MEDICARE<span className="text-cyan-400">+</span>
                  </span>
                  <span className="text-[9px] uppercase tracking-wider text-cyan-400/90 font-bold block">
                    Smart Medical Kit
                  </span>
                </div>
              </div>

              {/* Desktop collapse button */}
              <button
                onClick={() => setIsCollapsed(true)}
                className="hidden md:block p-1.5 rounded-lg text-slate-400 hover:text-white hover:bg-slate-800 transition-colors cursor-pointer"
                title="Collapse Sidebar"
              >
                <ChevronLeft className="w-5 h-5" />
              </button>

              {/* Mobile close button */}
              <button
                onClick={() => setMobileOpen(false)}
                className="md:hidden p-1.5 rounded-lg text-slate-400 hover:text-white hover:bg-slate-800 transition-colors cursor-pointer"
                title="Close Menu"
              >
                <X className="w-5 h-5" />
              </button>
            </div>
          )}

          {/* Live ESP32 Status Pill */}
          {(!isCollapsed || mobileOpen) ? (
            <div className="mx-3 mt-4 mb-2 p-2.5 rounded-xl bg-cyan-950/40 border border-cyan-800/40 flex items-center justify-between text-xs">
              <div className="flex items-center gap-2">
                <span className="relative flex h-2 w-2">
                  <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"></span>
                  <span className="relative inline-flex rounded-full h-2 w-2 bg-emerald-500"></span>
                </span>
                <span className="font-semibold text-slate-200">ESP32 Cloud Link</span>
              </div>
              <span className="text-[10px] font-mono px-1.5 py-0.5 rounded bg-emerald-500/20 text-emerald-400 font-bold">
                ACTIVE
              </span>
            </div>
          ) : (
            <div className="flex justify-center my-3" title="ESP32 Cloud Active">
              <span className="relative flex h-2.5 w-2.5">
                <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"></span>
                <span className="relative inline-flex rounded-full h-2.5 w-2.5 bg-emerald-500"></span>
              </span>
            </div>
          )}

          {/* Navigation Links */}
          <nav className="mt-3 px-2 space-y-1">
            {navItems.map((item) => {
              const IconComponent = item.icon;
              const isActive = activeTab === item.id;
              return (
                <button
                  key={item.id}
                  onClick={() => handleNavClick(item.id)}
                  title={isCollapsed && !mobileOpen ? item.label : undefined}
                  className={`w-full flex items-center justify-center md:justify-start gap-3.5 px-3 py-3 rounded-xl text-sm font-semibold transition-all group cursor-pointer ${
                    isActive
                      ? 'bg-gradient-to-r from-cyan-500/20 to-blue-500/10 text-cyan-400 border border-cyan-500/30 shadow-md shadow-cyan-500/10'
                      : 'text-slate-400 hover:text-slate-100 hover:bg-slate-800/60'
                  }`}
                >
                  <IconComponent className={`w-5 h-5 shrink-0 transition-transform group-hover:scale-110 ${isActive ? 'text-cyan-400' : 'text-slate-400'}`} />
                  {(!isCollapsed || mobileOpen) && (
                    <div className="text-left flex-1">
                      <span className="block leading-none">{item.label}</span>
                      <span className="text-[10px] text-slate-500 block mt-0.5 font-normal">{item.desc}</span>
                    </div>
                  )}
                  {(!isCollapsed || mobileOpen) && isActive && (
                    <span className="w-1.5 h-1.5 rounded-full bg-cyan-400 shadow-sm shadow-cyan-400" />
                  )}
                </button>
              );
            })}
          </nav>
        </div>

        {/* Bottom Profile Section with Dropdown */}
        <div className="p-3 border-t border-slate-800/80 relative flex justify-center">
          {/* Dropdown Menu */}
          {profileDropdownOpen && (
            <div className={`absolute bottom-full mb-2 bg-slate-900 border border-slate-700/80 rounded-2xl p-2 shadow-2xl z-50 backdrop-blur-2xl ${
              isCollapsed && !mobileOpen ? 'left-2 w-56' : 'left-3 right-3'
            }`}>
              <div className="px-3 py-2 border-b border-slate-800">
                <p className="text-xs font-bold text-white truncate">{displayName}</p>
                <p className="text-[10px] font-mono text-cyan-400 truncate">{medicareId}</p>
              </div>

              <button
                onClick={() => {
                  handleNavClick('PROFILE');
                  setProfileDropdownOpen(false);
                }}
                className="w-full flex items-center gap-2.5 px-3 py-2 text-xs text-slate-300 hover:text-white hover:bg-slate-800 rounded-lg transition-colors cursor-pointer mt-1"
              >
                <User className="w-4 h-4 text-cyan-400" />
                <span>Go to Profile Page</span>
              </button>

              <button
                onClick={() => {
                  handleNavClick('PROFILE');
                  setProfileDropdownOpen(false);
                }}
                className="w-full flex items-center gap-2.5 px-3 py-2 text-xs text-slate-300 hover:text-white hover:bg-slate-800 rounded-lg transition-colors cursor-pointer"
              >
                <Settings className="w-4 h-4 text-slate-400" />
                <span>Settings & Theme</span>
              </button>

              <div className="border-t border-slate-800 my-1" />

              <button
                onClick={() => {
                  setProfileDropdownOpen(false);
                  if (setMobileOpen) setMobileOpen(false);
                  onLogout();
                }}
                className="w-full flex items-center gap-2.5 px-3 py-2 text-xs text-rose-400 hover:bg-rose-500/10 rounded-lg transition-colors cursor-pointer"
              >
                <LogOut className="w-4 h-4 text-rose-400" />
                <span>Sign Out</span>
              </button>
            </div>
          )}

          {/* Profile Card Trigger */}
          <div
            onClick={() => setProfileDropdownOpen(!profileDropdownOpen)}
            className={`flex items-center justify-center gap-3 p-2 rounded-xl hover:bg-slate-800/80 border border-transparent hover:border-slate-700 cursor-pointer transition-all w-full ${
              profileDropdownOpen ? 'bg-slate-800/90 border-slate-700' : ''
            }`}
            title={isCollapsed && !mobileOpen ? `${displayName} (Click for menu)` : undefined}
          >
            {photoURL ? (
              <img
                src={photoURL}
                alt={displayName}
                className="w-9 h-9 rounded-full object-cover ring-2 ring-cyan-500/40 shrink-0"
              />
            ) : (
              <div className="w-9 h-9 rounded-full bg-gradient-to-tr from-cyan-600 to-blue-600 flex items-center justify-center text-white font-bold text-xs ring-2 ring-cyan-500/40 shrink-0">
                {displayName.charAt(0).toUpperCase()}
              </div>
            )}

            {(!isCollapsed || mobileOpen) && (
              <div className="flex-1 min-w-0">
                <p className="text-xs font-bold text-white truncate">{displayName}</p>
                <p className="text-[10px] text-slate-400 truncate">{displayEmail}</p>
              </div>
            )}

            {(!isCollapsed || mobileOpen) && (
              <ChevronUp className={`w-4 h-4 text-slate-400 transition-transform ${profileDropdownOpen ? 'rotate-180' : ''}`} />
            )}
          </div>
        </div>
      </aside>
    </>
  );
}

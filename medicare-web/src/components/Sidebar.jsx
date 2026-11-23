import React, { useState } from 'react';
import { Home, Activity, FileText, Pill, HelpCircle, User, LogOut, Settings, Heart, ChevronLeft, ChevronRight, X } from 'lucide-react';

export default function Sidebar({ activeTab, setActiveTab, user, userProfile, onLogout, isCollapsed, setIsCollapsed, mobileOpen, setMobileOpen }) {
  const [profileOpen, setProfileOpen] = useState(false);
  const displayName = userProfile?.name || user?.displayName || user?.email?.split('@')[0] || 'Patient';
  const photoURL = userProfile?.photoURL || user?.photoURL;
  const navItems = [
    { id: 'HOME', label: 'Overview', icon: Home },
    { id: 'TEST', label: 'Live testing', icon: Activity },
    { id: 'REPORT', label: 'Reports', icon: FileText },
    { id: 'MEDICINE', label: 'Medicines', icon: Pill },
    { id: 'HELP', label: 'Help center', icon: HelpCircle }
  ];

  const navigate = (tab) => {
    setActiveTab(tab);
    setProfileOpen(false);
  };

  return (
    <>
      {mobileOpen && <button className="sidebar-backdrop" onClick={() => setMobileOpen(false)} aria-label="Close menu" />}
      <aside className={`portal-sidebar ${isCollapsed ? 'collapsed' : ''} ${mobileOpen ? 'mobile-open' : ''}`}>
        <div className="sidebar-top">
          <button className="sidebar-brand" onClick={() => navigate('HOME')}>
            <span className="brand-mark"><Heart /></span>
            {!isCollapsed && <span className="brand-name">MEDICARE<span>+</span></span>}
          </button>
          <button className="sidebar-collapse" onClick={() => setIsCollapsed(!isCollapsed)} aria-label="Toggle sidebar">
            {isCollapsed ? <ChevronRight /> : <ChevronLeft />}
          </button>
          <button className="sidebar-close" onClick={() => setMobileOpen(false)} aria-label="Close menu"><X /></button>
        </div>

        <nav className="sidebar-nav">
          <span className="nav-caption">{isCollapsed ? '' : 'Workspace'}</span>
          {navItems.map((item) => {
            const Icon = item.icon;
            return (
              <button key={item.id} className={activeTab === item.id ? 'active' : ''} onClick={() => navigate(item.id)} title={item.label}>
                <Icon />
                {!isCollapsed && <span>{item.label}</span>}
              </button>
            );
          })}
        </nav>

        <div className="sidebar-profile">
          {profileOpen && (
            <div className="sidebar-profile-menu">
              <button onClick={() => navigate('PROFILE')}><User /> Profile</button>
              <button onClick={() => navigate('PROFILE')}><Settings /> Settings</button>
              <button className="logout" onClick={onLogout}><LogOut /> Sign out</button>
            </div>
          )}
          <button className="sidebar-profile-trigger" onClick={() => setProfileOpen(!profileOpen)}>
            {photoURL ? <img src={photoURL} alt={displayName} referrerPolicy="no-referrer" /> : <span className="profile-avatar">{displayName.charAt(0).toUpperCase()}</span>}
            {!isCollapsed && <span><strong>{displayName}</strong><small>Patient account</small></span>}
          </button>
        </div>
      </aside>
    </>
  );
}

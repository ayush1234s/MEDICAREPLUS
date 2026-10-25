import { initializeApp, getApps, getApp } from "firebase/app";
import { 
  getAuth, 
  GoogleAuthProvider, 
  signInWithPopup, 
  signInWithRedirect,
  signInWithEmailAndPassword, 
  createUserWithEmailAndPassword, 
  sendPasswordResetEmail,
  signOut,
  updateProfile,
  onAuthStateChanged 
} from "firebase/auth";
import { 
  getDatabase, 
  ref, 
  set, 
  push, 
  onValue, 
  update, 
  remove, 
  get, 
  child,
  query,
  orderByChild
} from "firebase/database";

const firebaseConfig = {
  apiKey: import.meta.env.VITE_FIREBASE_API_KEY,
  authDomain: import.meta.env.VITE_FIREBASE_AUTH_DOMAIN,
  databaseURL: import.meta.env.VITE_FIREBASE_DATABASE_URL,
  projectId: import.meta.env.VITE_FIREBASE_PROJECT_ID,
  storageBucket: import.meta.env.VITE_FIREBASE_STORAGE_BUCKET,
  messagingSenderId: import.meta.env.VITE_FIREBASE_MESSAGING_SENDER_ID,
  appId: import.meta.env.VITE_FIREBASE_APP_ID,
  measurementId: import.meta.env.VITE_FIREBASE_MEASUREMENT_ID
};

// Initialize Firebase
const app = getApps().length === 0 ? initializeApp(firebaseConfig) : getApp();
export const auth = getAuth(app);
export const googleProvider = new GoogleAuthProvider();
googleProvider.setCustomParameters({ prompt: 'select_account' });

export const db = getDatabase(app);

// Event emitter / callback for permission warning banner in UI
let permissionWarningListeners = [];
export const onPermissionWarning = (callback) => {
  permissionWarningListeners.push(callback);
  return () => {
    permissionWarningListeners = permissionWarningListeners.filter(cb => cb !== callback);
  };
};

const notifyPermissionWarning = () => {
  permissionWarningListeners.forEach(cb => {
    try { cb(true); } catch (e) {}
  });
};

// Authentication Helpers
export const loginWithGoogle = async () => {
  try {
    return await signInWithPopup(auth, googleProvider);
  } catch (err) {
    console.warn("Popup blocked or failed, trying redirect fallback:", err);
    if (err.code === 'auth/popup-blocked' || err.code === 'auth/popup-closed-by-user') {
      return await signInWithRedirect(auth, googleProvider);
    }
    throw err;
  }
};

export const loginWithEmail = async (email, password) => {
  return await signInWithEmailAndPassword(auth, email, password);
};

export const registerWithEmail = async (email, password, displayName) => {
  const userCredential = await createUserWithEmailAndPassword(auth, email, password);
  if (displayName) {
    await updateProfile(userCredential.user, { displayName });
  }
  return userCredential;
};

export const resetPassword = async (email) => {
  return await sendPasswordResetEmail(auth, email);
};

export const logoutUser = async () => {
  return await signOut(auth);
};

// User Profile & System Medicare ID Generator
export const generateMedicareId = (uid) => {
  const randomSuffix = Math.floor(1000 + Math.random() * 9000);
  const prefix = (uid || "USER").substring(0, 3).toUpperCase();
  return `MED-${prefix}-${randomSuffix}`;
};

export const saveUserProfileToDb = async (uid, profileData) => {
  try {
    const existing = JSON.parse(localStorage.getItem(`medicare_profile_${uid}`) || '{}');
    const merged = { ...existing, ...profileData, updatedAt: new Date().toISOString() };
    localStorage.setItem(`medicare_profile_${uid}`, JSON.stringify(merged));
  } catch (e) {}

  try {
    const userRef = ref(db, `users/${uid}`);
    await update(userRef, {
      ...profileData,
      updatedAt: new Date().toISOString()
    });
  } catch (err) {
    if (err.message?.includes('permission_denied') || err.code === 'PERMISSION_DENIED') {
      notifyPermissionWarning();
      console.warn("[Firebase] Realtime Database permission denied at /users. Cached locally.");
      return;
    }
    throw err;
  }
};

export const getUserProfileFromDb = async (uid) => {
  let localData = null;
  try {
    const cached = localStorage.getItem(`medicare_profile_${uid}`);
    if (cached) localData = JSON.parse(cached);
  } catch (e) {}

  try {
    const snapshot = await get(ref(db, `users/${uid}`));
    if (snapshot.exists()) {
      return snapshot.val();
    }
  } catch (err) {
    if (err.message?.includes('permission_denied') || err.code === 'PERMISSION_DENIED') {
      notifyPermissionWarning();
    }
  }

  return localData;
};

// Real-time Medicine Sync (Two-way link with ESP32 Device)
export const subscribeToMedicines = (callback) => {
  const getLocalMedicines = () => {
    try {
      const cached = localStorage.getItem('medicare_medicines');
      if (cached) return JSON.parse(cached);
    } catch (e) {}
    return [];
  };

  const medicinesRef = ref(db, 'medicines');
  return onValue(medicinesRef, (snapshot) => {
    const data = snapshot.val();
    if (!data) {
      callback(getLocalMedicines());
      return;
    }
    const list = Object.keys(data).map(key => ({
      id: key,
      ...data[key]
    })).sort((a, b) => (b.createdAt || 0) - (a.createdAt || 0));
    
    try {
      localStorage.setItem('medicare_medicines', JSON.stringify(list));
    } catch (e) {}

    callback(list);
  }, (err) => {
    if (err.message?.includes('permission_denied') || err.code === 'PERMISSION_DENIED') {
      notifyPermissionWarning();
      console.warn("[Firebase] Medicines permission denied. Using local cache.");
    }
    callback(getLocalMedicines());
  });
};

export const addMedicineToDb = async (medicineData) => {
  const tempId = 'med_' + Date.now();
  const payload = {
    ...medicineData,
    id: tempId,
    status: medicineData.status || 'Pending',
    createdAt: Date.now()
  };

  // 1. Save locally first
  try {
    const existing = JSON.parse(localStorage.getItem('medicare_medicines') || '[]');
    existing.unshift(payload);
    localStorage.setItem('medicare_medicines', JSON.stringify(existing));
  } catch (e) {}

  // 2. Attempt writing to Firebase
  try {
    const medicinesRef = ref(db, 'medicines');
    const newMedRef = push(medicinesRef);
    payload.id = newMedRef.key;
    await set(newMedRef, payload);
    
    // Push active reminder with unquoted values and clean format for ESP32
    await set(ref(db, 'active_reminder'), {
      active: true,
      medicineId: newMedRef.key,
      patientName: medicineData.fullName || 'Patient',
      medicine: medicineData.medicine,
      dosage: medicineData.dose || '1 Dose',
      scheduledDate: medicineData.date,
      scheduledTime: medicineData.time,
      timestamp: Date.now(),
      status: 'Pending'
    });

    return newMedRef.key;
  } catch (err) {
    if (err.message?.includes('permission_denied') || err.code === 'PERMISSION_DENIED') {
      notifyPermissionWarning();
      console.warn("[Firebase] Stored medicine locally due to permission_denied.");
      return tempId;
    }
    throw err;
  }
};

export const updateMedicineStatusInDb = async (id, status) => {
  try {
    const existing = JSON.parse(localStorage.getItem('medicare_medicines') || '[]');
    const updated = existing.map(m => m.id === id ? { ...m, status, lastActionAt: Date.now() } : m);
    localStorage.setItem('medicare_medicines', JSON.stringify(updated));
  } catch (e) {}

  try {
    const medRef = ref(db, `medicines/${id}`);
    await update(medRef, { 
      status,
      lastActionAt: Date.now()
    });

    const activeReminderSnap = await get(ref(db, 'active_reminder'));
    if (activeReminderSnap.exists() && activeReminderSnap.val()?.medicineId === id) {
      await update(ref(db, 'active_reminder'), { 
        status,
        active: status === 'Pending' 
      });
    }
  } catch (err) {
    if (err.message?.includes('permission_denied') || err.code === 'PERMISSION_DENIED') {
      notifyPermissionWarning();
      return;
    }
    console.error("Error updating medicine status in DB:", err);
  }
};

export const deleteMedicineFromDb = async (id) => {
  try {
    const existing = JSON.parse(localStorage.getItem('medicare_medicines') || '[]');
    const filtered = existing.filter(m => m.id !== id);
    localStorage.setItem('medicare_medicines', JSON.stringify(filtered));
  } catch (e) {}

  try {
    await remove(ref(db, `medicines/${id}`));
  } catch (err) {
    if (err.message?.includes('permission_denied') || err.code === 'PERMISSION_DENIED') {
      notifyPermissionWarning();
      return;
    }
    console.error("Error deleting medicine from DB:", err);
  }
};

// Sensor Data Real-time Sync (SpO2, Heart Rate, ECG)
export const subscribeToLiveVitals = (callback) => {
  const liveRef = ref(db, 'live');
  return onValue(liveRef, (snapshot) => {
    if (snapshot.exists()) {
      callback(snapshot.val());
    } else {
      callback(null);
    }
  }, (err) => {
    if (err.message?.includes('permission_denied') || err.code === 'PERMISSION_DENIED') {
      notifyPermissionWarning();
    }
    callback(null);
  });
};

export const pushSensorReadingToDb = async (type, reading) => {
  const normType = type.toLowerCase();
  const localKey = 'readings_' + normType;
  const tempItem = {
    id: 'rd_' + Date.now(),
    type: type.toUpperCase(),
    ...reading,
    timestamp: Date.now()
  };

  try {
    const existing = JSON.parse(localStorage.getItem(localKey) || '[]');
    existing.unshift(tempItem);
    localStorage.setItem(localKey, JSON.stringify(existing));
  } catch (e) {}

  try {
    const readingsRef = ref(db, `readings/${normType}`);
    const newRef = push(readingsRef);
    const data = {
      id: newRef.key,
      ...reading,
      timestamp: Date.now()
    };
    await set(newRef, data);
    await update(ref(db, `live/${normType}`), {
      ...data,
      updatedAt: Date.now()
    });
    return newRef.key;
  } catch (err) {
    if (err.message?.includes('permission_denied') || err.code === 'PERMISSION_DENIED') {
      notifyPermissionWarning();
      console.warn(`[Firebase] Saved ${type} reading locally due to permission_denied.`);
      return tempItem.id;
    }
    throw err;
  }
};

export const deleteReadingFromDb = async (type, id) => {
  const normType = type.toLowerCase();
  const localKey = 'readings_' + normType;
  try {
    const existing = JSON.parse(localStorage.getItem(localKey) || '[]');
    const filtered = existing.filter(r => r.id !== id);
    localStorage.setItem(localKey, JSON.stringify(filtered));
  } catch (e) {}

  try {
    const readingRef = ref(db, `readings/${normType}/${id}`);
    await remove(readingRef);
  } catch (err) {
    if (err.message?.includes('permission_denied') || err.code === 'PERMISSION_DENIED') {
      notifyPermissionWarning();
      return;
    }
    console.error("Error deleting reading from DB:", err);
  }
};

export const fetchReadingsFromDb = async (type, fromDate, toDate) => {
  let results = [];
  const normType = type.toLowerCase();

  try {
    const path = type === 'BOTH' ? 'readings' : `readings/${normType}`;
    const snap = await get(ref(db, path));
    if (snap.exists()) {
      const val = snap.val();
      if (type === 'BOTH') {
        const spo2Obj = val.spo2 || {};
        const ecgObj = val.ecg || {};
        const spo2List = Object.keys(spo2Obj).map(k => ({ id: k, type: 'SPO2', ...spo2Obj[k] }));
        const ecgList = Object.keys(ecgObj).map(k => ({ id: k, type: 'ECG', ...ecgObj[k] }));
        results = [...spo2List, ...ecgList];
      } else {
        results = Object.keys(val).map(k => ({
          id: k,
          type: type.toUpperCase(),
          ...val[k]
        }));
      }
    }
  } catch (err) {
    if (err.message?.includes('permission_denied') || err.code === 'PERMISSION_DENIED') {
      notifyPermissionWarning();
    }
  }

  // Only fallback to local cache if Firebase returned no records (e.g. offline)
  if (results.length === 0) {
    try {
      if (type === 'SPO2' || type === 'BOTH') {
        const localSpo2 = JSON.parse(localStorage.getItem('readings_spo2') || '[]');
        results.push(...localSpo2);
      }
      if (type === 'ECG' || type === 'BOTH') {
        const localEcg = JSON.parse(localStorage.getItem('readings_ecg') || '[]');
        results.push(...localEcg);
      }
    } catch (e) {}
  }

  // 100% Timezone-Proof Date Range Filter
  if (fromDate || toDate) {
    results = results.filter(item => {
      let itemDateStr = '';
      if (item.date && /^\d{4}-\d{2}-\d{2}$/.test(item.date)) {
        itemDateStr = item.date;
      } else if (item.timestamp) {
        const d = new Date(item.timestamp);
        const y = d.getFullYear();
        const m = String(d.getMonth() + 1).padStart(2, '0');
        const day = String(d.getDate()).padStart(2, '0');
        itemDateStr = `${y}-${m}-${day}`;
      } else {
        itemDateStr = new Date().toISOString().split('T')[0];
      }

      if (fromDate && itemDateStr < fromDate) return false;
      if (toDate && itemDateStr > toDate) return false;
      return true;
    });
  }

  return results.sort((a, b) => (b.timestamp || 0) - (a.timestamp || 0));
};

export default app;

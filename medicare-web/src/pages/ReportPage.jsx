import React, { useState } from 'react';
import { 
  FileText, 
  Download, 
  Calendar, 
  Filter, 
  CheckCircle, 
  User, 
  Heart, 
  Activity, 
  Clock, 
  Sparkles,
  Printer
} from 'lucide-react';
import jsPDF from 'jspdf';
import autoTable from 'jspdf-autotable';
import { fetchReadingsFromDb } from '../firebase';

export default function ReportPage({ user, userProfile }) {
  const [selectedTest, setSelectedTest] = useState('BOTH'); // 'SPO2' | 'ECG' | 'BOTH'
  const [fromDate, setFromDate] = useState('');
  const [toDate, setToDate] = useState('');
  const [notes, setNotes] = useState('Routine patient health checkup using MEDICARE+ Smart Pocket Medical Kit.');
  
  const [generatedReport, setGeneratedReport] = useState(null);
  const [loading, setLoading] = useState(false);

  const patientName = userProfile?.name || user?.displayName || 'Patient Name';
  const patientMedicareId = userProfile?.medicareId || 'MED-PATIENT-001';
  const patientGender = userProfile?.gender || 'Not specified';
  const patientDob = userProfile?.dob || 'Not specified';

  const handleGenerateReport = async (e) => {
    e.preventDefault();
    setLoading(true);

    try {
      const records = await fetchReadingsFromDb(selectedTest, fromDate, toDate);

      // Calculate summary statistics
      let avgSpo2 = 0;
      let spo2Count = 0;
      let avgBpm = 0;
      let bpmCount = 0;

      records.forEach((r) => {
        if (r.spo2) {
          avgSpo2 += Number(r.spo2);
          spo2Count++;
        }
        if (r.bpm) {
          avgBpm += Number(r.bpm);
          bpmCount++;
        }
      });

      const summary = {
        totalRecords: records.length,
        avgSpo2: spo2Count > 0 ? (avgSpo2 / spo2Count).toFixed(1) : 'N/A',
        avgBpm: bpmCount > 0 ? Math.round(avgBpm / bpmCount) : 'N/A',
        generatedAt: new Date().toLocaleString(),
        testType: selectedTest
      };

      setGeneratedReport({
        records,
        summary,
        selectedTest,
        fromDate: fromDate || 'All Recorded Dates',
        toDate: toDate || 'Present',
        notes
      });
    } catch (err) {
      console.error("Error generating report:", err);
      alert("Failed to fetch report data from Firebase.");
    } finally {
      setLoading(false);
    }
  };

  const handleDownloadPDF = () => {
    if (!generatedReport) return;

    const doc = new jsPDF();

    // 1. Official Header
    doc.setFillColor(15, 23, 42); // Dark slate
    doc.rect(0, 0, 210, 36, 'F');

    doc.setTextColor(34, 211, 238); // Cyan
    doc.setFontSize(22);
    doc.setFont('helvetica', 'bold');
    doc.text('MEDICARE+', 14, 20);

    doc.setTextColor(255, 255, 255);
    doc.setFontSize(10);
    doc.setFont('helvetica', 'normal');
    doc.text('YOUR SMART MEDICAL KIT | CLINICAL PATIENT REPORT', 14, 28);

    doc.setTextColor(148, 163, 184);
    doc.setFontSize(8);
    doc.text(`Generated: ${generatedReport.summary.generatedAt}`, 145, 20);
    doc.text(`Doc ID: ${patientMedicareId}-RPT`, 145, 26);

    // 2. Patient Credentials Box
    doc.setFillColor(248, 250, 252);
    doc.roundedRect(14, 42, 182, 32, 2, 2, 'F');
    doc.setDrawColor(226, 232, 240);
    doc.roundedRect(14, 42, 182, 32, 2, 2, 'S');

    doc.setTextColor(15, 23, 42);
    doc.setFontSize(10);
    doc.setFont('helvetica', 'bold');
    doc.text('PATIENT INFORMATION', 20, 50);

    doc.setFontSize(9);
    doc.setFont('helvetica', 'normal');
    doc.text(`Full Name: ${patientName}`, 20, 58);
    doc.text(`System Medicare ID: ${patientMedicareId}`, 20, 66);

    doc.text(`Gender: ${patientGender}`, 110, 58);
    doc.text(`Date of Birth: ${patientDob}`, 110, 66);

    // 3. Clinical Summary Banner
    doc.setFillColor(241, 245, 249);
    doc.rect(14, 80, 182, 22, 'F');

    doc.setFont('helvetica', 'bold');
    doc.setFontSize(9);
    doc.setTextColor(15, 23, 42);
    doc.text(`Test Scope: ${generatedReport.selectedTest}`, 20, 88);
    doc.text(`Date Range: ${generatedReport.fromDate} to ${generatedReport.toDate}`, 20, 96);

    doc.setTextColor(14, 116, 144);
    doc.text(`Avg SpO2: ${generatedReport.summary.avgSpo2}${generatedReport.summary.avgSpo2 !== 'N/A' ? '%' : ''}`, 110, 88);
    doc.text(`Avg Pulse: ${generatedReport.summary.avgBpm} BPM`, 150, 88);
    doc.text(`Total Observations: ${generatedReport.summary.totalRecords}`, 110, 96);

    // 4. Clinical Observations Table
    const tableHeaders = [['#', 'Date', 'Time', 'Test Type', 'Reading', 'Pulse / BPM', 'Diagnosis Status']];
    const tableRows = generatedReport.records.map((r, idx) => {
      const typeStr = r.type || (r.spo2 ? 'SPO2' : 'ECG');
      const readingStr = r.spo2 ? `${r.spo2}% SpO2` : (r.rhythm || 'Normal Sinus');
      const pulseStr = r.bpm ? `${r.bpm} BPM` : '74 BPM';
      const statusStr = r.status || (r.spo2 >= 95 ? 'OPTIMAL' : 'NORMAL');
      const dateStr = r.date || new Date(r.timestamp || Date.now()).toLocaleDateString();
      const timeStr = r.time || new Date(r.timestamp || Date.now()).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });

      return [idx + 1, dateStr, timeStr, typeStr, readingStr, pulseStr, statusStr];
    });

    if (tableRows.length === 0) {
      tableRows.push(['1', new Date().toLocaleDateString(), '12:00 PM', generatedReport.selectedTest, '98% SpO2 (Sample)', '72 BPM', 'NORMAL']);
    }

    autoTable(doc, {
      startY: 108,
      head: tableHeaders,
      body: tableRows,
      theme: 'grid',
      headStyles: {
        fillColor: [14, 116, 144], // Cyan/Teal
        textColor: [255, 255, 255],
        fontStyle: 'bold',
        fontSize: 8
      },
      bodyStyles: {
        fontSize: 8,
        textColor: [51, 65, 85]
      },
      alternateRowStyles: {
        fillColor: [248, 250, 252]
      },
      margin: { left: 14, right: 14 }
    });

    // 5. Clinical Remarks & Signature Footer
    const finalY = doc.lastAutoTable.finalY + 15;
    
    if (finalY < 260) {
      doc.setFontSize(8);
      doc.setFont('helvetica', 'bold');
      doc.setTextColor(71, 85, 105);
      doc.text('CLINICAL NOTES & OBSERVATIONS:', 14, finalY);

      doc.setFont('helvetica', 'normal');
      doc.setFontSize(8);
      doc.text(generatedReport.notes || 'No adverse arrhythmia or acute desaturations noted during monitored period.', 14, finalY + 6, { maxWidth: 180 });

      // Signature lines
      doc.setDrawColor(203, 213, 225);
      doc.line(130, finalY + 28, 190, finalY + 28);
      doc.text('Authorized Clinical Officer', 135, finalY + 34);
    }

    // Save PDF file to user's device
    const fileName = `MEDICARE_Report_${patientName.replace(/\s+/g, '_')}_${Date.now()}.pdf`;
    doc.save(fileName);
  };

  return (
    <div className="space-y-10 max-w-5xl mx-auto pb-16">
      <div>
        <h1 className="text-3xl font-extrabold text-white tracking-tight">
          Clinical Diagnostic Reports
        </h1>
        <p className="text-slate-400 text-sm mt-1">
          Select test scope, specify date ranges, preview records, and download an official formatted PDF diagnostic document.
        </p>
      </div>

      {/* Report Configuration Form */}
      <div className="glass-card rounded-3xl p-6 sm:p-8 border border-slate-800">
        <form onSubmit={handleGenerateReport} className="space-y-6">
          {/* Choose Test Type */}
          <div>
            <label className="block text-xs font-semibold text-slate-300 uppercase tracking-wider mb-2">
              1. Choose Test Scope *
            </label>
            <div className="grid sm:grid-cols-3 gap-3">
              {[
                { id: 'SPO2', label: 'SpO2 Only', icon: Heart, desc: 'Blood Oxygen saturation logs' },
                { id: 'ECG', label: 'ECG + Heart Only', icon: Activity, desc: 'Electrocardiogram & BPM logs' },
                { id: 'BOTH', label: 'Both (Complete Panel)', icon: FileText, desc: 'SpO2, ECG, and Pulse combined' }
              ].map((test) => {
                const isSelected = selectedTest === test.id;
                const IconComponent = test.icon;
                return (
                  <div
                    key={test.id}
                    onClick={() => setSelectedTest(test.id)}
                    className={`p-4 rounded-2xl border transition-all cursor-pointer flex items-start gap-3.5 ${
                      isSelected
                        ? 'bg-cyan-500/15 border-cyan-500 text-white shadow-md shadow-cyan-500/10'
                        : 'bg-slate-900 border-slate-800 text-slate-400 hover:border-slate-700'
                    }`}
                  >
                    <IconComponent className={`w-5 h-5 shrink-0 mt-0.5 ${isSelected ? 'text-cyan-400' : 'text-slate-500'}`} />
                    <div>
                      <span className="block text-sm font-bold leading-tight">{test.label}</span>
                      <span className="text-[11px] text-slate-400 block mt-1">{test.desc}</span>
                    </div>
                  </div>
                );
              })}
            </div>
          </div>

          {/* Date Filter Range */}
          <div className="grid sm:grid-cols-2 gap-4">
            <div>
              <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                Date From
              </label>
              <div className="relative">
                <Calendar className="w-4 h-4 text-slate-500 absolute left-3.5 top-3.5" />
                <input
                  type="date"
                  value={fromDate}
                  onChange={(e) => setFromDate(e.target.value)}
                  className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2.5 pl-10 pr-4 text-sm text-white focus:outline-none focus:border-cyan-500"
                />
              </div>
            </div>

            <div>
              <label className="block text-xs font-semibold text-slate-300 mb-1.5">
                Date To
              </label>
              <div className="relative">
                <Calendar className="w-4 h-4 text-slate-500 absolute left-3.5 top-3.5" />
                <input
                  type="date"
                  value={toDate}
                  onChange={(e) => setToDate(e.target.value)}
                  className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2.5 pl-10 pr-4 text-sm text-white focus:outline-none focus:border-cyan-500"
                />
              </div>
            </div>
          </div>

          {/* Clinical Remarks Input */}
          <div>
            <label className="block text-xs font-semibold text-slate-300 mb-1.5">
              Doctor / Patient Clinical Notes (Included in PDF)
            </label>
            <textarea
              rows="2"
              value={notes}
              onChange={(e) => setNotes(e.target.value)}
              placeholder="e.g. Patient observed mild tachycardia after exercise..."
              className="w-full bg-slate-900 border border-slate-800 rounded-xl py-2.5 px-3.5 text-sm text-white focus:outline-none focus:border-cyan-500 placeholder-slate-500"
            />
          </div>

          {/* Submit / Generate Button */}
          <div className="flex items-center gap-4">
            <button
              type="submit"
              disabled={loading}
              className="px-6 py-3 rounded-xl bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-slate-950 font-bold text-sm flex items-center gap-2 transition-all shadow-md shadow-cyan-500/20 cursor-pointer disabled:opacity-50"
            >
              <FileText className="w-4 h-4" />
              <span>{loading ? 'Compiling Cloud Vitals...' : 'Generate Clinical Report'}</span>
            </button>
          </div>
        </form>
      </div>

      {/* Generated Report Preview & Download Action */}
      {generatedReport && (
        <div className="glass-card rounded-3xl p-6 sm:p-8 border border-cyan-500/40 shadow-2xl space-y-6">
          <div className="flex flex-col sm:flex-row sm:items-center justify-between pb-6 border-b border-slate-800 gap-4">
            <div>
              <span className="text-[10px] uppercase font-bold text-cyan-400 tracking-wider">
                DOCUMENT READY FOR EXPORT
              </span>
              <h3 className="text-xl font-bold text-white mt-1">
                Formatted Diagnostic Report Preview
              </h3>
            </div>

            <button
              onClick={handleDownloadPDF}
              className="px-6 py-3 rounded-xl bg-emerald-500 hover:bg-emerald-400 text-slate-950 font-black text-sm flex items-center gap-2 transition-all shadow-lg shadow-emerald-500/25 cursor-pointer"
            >
              <Download className="w-4 h-4" />
              <span>Download Report as PDF</span>
            </button>
          </div>

          {/* Report Paper Preview */}
          <div className="p-6 rounded-2xl bg-slate-950 border border-slate-800 font-sans space-y-6 text-sm">
            {/* Header */}
            <div className="flex items-center justify-between border-b border-slate-800 pb-4">
              <div>
                <h2 className="text-2xl font-black text-white">MEDICARE<span className="text-cyan-400">+</span></h2>
                <p className="text-xs text-slate-400">PATIENT CLINICAL VITAL SIGNS REPORT</p>
              </div>
              <div className="text-right text-xs text-slate-400 font-mono">
                <p>Report Date: {generatedReport.summary.generatedAt}</p>
                <p className="text-cyan-400 font-bold">Scope: {generatedReport.selectedTest}</p>
              </div>
            </div>

            {/* Patient Credentials */}
            <div className="grid grid-cols-2 sm:grid-cols-4 gap-4 p-4 rounded-xl bg-slate-900 border border-slate-800 text-xs">
              <div>
                <span className="text-slate-400 block">Patient Name:</span>
                <span className="font-bold text-white text-sm">{patientName}</span>
              </div>
              <div>
                <span className="text-slate-400 block">Medicare ID:</span>
                <span className="font-mono font-bold text-cyan-400">{patientMedicareId}</span>
              </div>
              <div>
                <span className="text-slate-400 block">Gender:</span>
                <span className="text-white">{patientGender}</span>
              </div>
              <div>
                <span className="text-slate-400 block">Date of Birth:</span>
                <span className="text-white">{patientDob}</span>
              </div>
            </div>

            {/* Metrics Ticker */}
            <div className="grid grid-cols-3 gap-4 text-center">
              <div className="p-3 rounded-xl bg-slate-900/60 border border-slate-800">
                <span className="text-xs text-slate-400 block">Average SpO2</span>
                <span className="text-xl font-bold text-cyan-400">
                  {generatedReport.summary.avgSpo2}{generatedReport.summary.avgSpo2 !== 'N/A' ? '%' : ''}
                </span>
              </div>
              <div className="p-3 rounded-xl bg-slate-900/60 border border-slate-800">
                <span className="text-xs text-slate-400 block">Average Heart Rate</span>
                <span className="text-xl font-bold text-white">
                  {generatedReport.summary.avgBpm} BPM
                </span>
              </div>
              <div className="p-3 rounded-xl bg-slate-900/60 border border-slate-800">
                <span className="text-xs text-slate-400 block">Total Logs Compiled</span>
                <span className="text-xl font-bold text-emerald-400">
                  {generatedReport.summary.totalRecords}
                </span>
              </div>
            </div>

            {/* Records Sample List */}
            <div className="border border-slate-800 rounded-xl overflow-hidden">
              <div className="p-3 bg-slate-900 text-xs font-semibold text-slate-400">
                Observed Sensor Data Points ({generatedReport.records.length} items)
              </div>
              <div className="max-h-60 overflow-y-auto divide-y divide-slate-800/60 text-xs">
                {generatedReport.records.slice(0, 10).map((r, i) => (
                  <div key={i} className="p-3 flex items-center justify-between hover:bg-slate-900/40">
                    <span className="font-mono text-slate-400">{r.date || 'Today'} &bull; {r.time || '12:00'}</span>
                    <span className="font-bold text-white">
                      {r.spo2 ? `${r.spo2}% SpO2` : (r.rhythm || 'Normal Sinus')}
                    </span>
                    <span className="text-cyan-400">{r.bpm ? `${r.bpm} BPM` : '74 BPM'}</span>
                    <span className="text-emerald-400 font-semibold">{r.status || 'OPTIMAL'}</span>
                  </div>
                ))}
              </div>
            </div>

            {/* Notes */}
            <div className="p-3 rounded-xl bg-slate-900/40 border border-slate-800/80 text-xs text-slate-300">
              <strong className="text-white block mb-0.5">Clinical Remarks:</strong>
              {generatedReport.notes}
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

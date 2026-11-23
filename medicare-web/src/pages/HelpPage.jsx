import React, { useState } from 'react';
import { ChevronDown, Cpu, Wifi, Activity, HeartPulse, Pill, Globe2, FileText, ShieldAlert, Languages, CircleHelp } from 'lucide-react';

const faqText = {
  en: [
    {
      category: 'Hardware setup', icon: Cpu,
      q: 'How do I turn on the kit and connect it to Wi-Fi?',
      a: 'Power the kit from its supported USB supply. It will try its saved Wi-Fi network. If setup mode appears, connect your phone to the MediCare+ setup network shown by the kit, open the setup page shown on its screen (usually 192.168.4.1), add your home Wi-Fi details, save, and let the kit reconnect. Keep it on a trusted network with internet access.'
    },
    {
      category: 'Hardware setup', icon: Wifi,
      q: 'What does Connected / Disconnected in the top bar mean?',
      a: 'Connected means the ESP32 has recently sent a heartbeat to the web dashboard through Firebase. Disconnected appears when the kit is switched off, loses Wi-Fi/internet, or cannot reach Firebase. It can take about 18 seconds for an old heartbeat to expire. This shows the kit-to-cloud link, not whether ECG or SpO2 sensors are attached.'
    },
    {
      category: 'Hardware buttons', icon: Cpu,
      q: 'What do the three physical buttons do?',
      a: 'BACK returns to the previous screen or dashboard; NEXT moves through dashboard tiles and skips an active medicine reminder; ENTER opens a tile or starts a measurement, and confirms a medicine reminder as Taken. Follow the labels on your particular kit.'
    },
    {
      category: 'SpO2', icon: Activity,
      q: 'How do I take an SpO2 reading?',
      a: 'Open the SpO2 tile with ENTER, then place a relaxed finger on the MAX30102 sensor as directed by the kit. Keep your hand still while it reads. Wait for the screen to finish. If it reports the sensor or finger is missing, reposition your finger and try again.'
    },
    {
      category: 'ECG + Heart', icon: HeartPulse,
      q: 'How do I use ECG + Heart?',
      a: 'Open the ECG + Heart tile. Connect the AD8232 module and its three electrodes using the hardware labels and your kit’s wiring/electrode guide. Attach the electrodes as instructed, sit still, and wait for stabilization and the measurement to finish. If the display says disconnected or leads off, stop and check the module and electrode connections.'
    },
    {
      category: 'Medicines', icon: Pill,
      q: 'How do I schedule a medicine reminder?',
      a: 'Open Medicines in the web portal, enter the medicine, dose, patient details, date, time, and duration, then save. The kit must be powered, online, and connected to Firebase to receive the reminder. Always check the prescription and details yourself; the reminder does not choose or validate a dose.'
    },
    {
      category: 'Medicines', icon: Pill,
      q: 'What do the buttons do when a medicine alert appears?',
      a: 'ENTER marks the current alert Taken, NEXT marks it Skip, and BACK dismisses it. Check the medicine and dose against the prescription before confirming. A button action is recorded in the connected web account when the cloud connection is available.'
    },
    {
      category: 'Web dashboard', icon: Globe2,
      q: 'Where do I find readings and schedules on the website?',
      a: 'Overview summarizes available readings and reminders. Live Tests is for test pages, Reports lets you review and export available records, Medicines is for schedules and their status, Help explains the kit, and Profile contains your account details and preferences. The top-bar device indicator reports the kit’s recent cloud heartbeat.'
    },
    {
      category: 'Web dashboard', icon: FileText,
      q: 'How do I make a report?',
      a: 'Open Reports, select the available test type and date range, review the records shown, then use the report download action. Reports reflect the data currently available to this account; check the dates and values before sharing them.'
    },
    {
      category: 'Safety', icon: ShieldAlert,
      q: 'Can I use the kit to diagnose or manage a serious condition?',
      a: 'No. This kit and dashboard are for general, everyday informational use only. They are not a substitute for professional medical advice, diagnosis, treatment, or emergency care. Do not use a reading to make a serious health decision or delay care. If you have concerning symptoms or a serious problem, stop using the kit and contact a qualified doctor or local emergency service.'
    },
    {
      category: 'Troubleshooting', icon: CircleHelp,
      q: 'The dashboard says Disconnected. What should I check?',
      a: 'Make sure the ESP32 is powered, has joined Wi-Fi, and can reach the internet. Wait briefly for its heartbeat. If it remains disconnected, restart the kit and check its Wi-Fi setup. A green device status does not guarantee that an individual sensor is connected or producing valid data.'
    }
  ],
  hi: [
    {
      category: 'हार्डवेयर सेटअप', icon: Cpu,
      q: 'किट चालू करके Wi-Fi से कैसे जोड़ें?',
      a: 'किट को उसके समर्थित USB पावर से चालू करें। यह पहले से सेव Wi-Fi से जुड़ने की कोशिश करेगी। अगर सेटअप मोड खुले, तो फोन को किट पर दिखाए गए MediCare+ नेटवर्क से जोड़ें और स्क्रीन पर दिया सेटअप पेज खोलें (आमतौर पर 192.168.4.1)। घर का Wi-Fi भरकर सेव करें और किट के दोबारा जुड़ने का इंतज़ार करें। भरोसेमंद इंटरनेट नेटवर्क इस्तेमाल करें।'
    },
    {
      category: 'हार्डवेयर सेटअप', icon: Wifi,
      q: 'ऊपर Connected / Disconnected का क्या मतलब है?',
      a: 'Connected का मतलब है कि ESP32 ने हाल में Firebase के ज़रिए वेब डैशबोर्ड को heartbeat भेजी है। किट बंद होने, Wi-Fi/इंटरनेट टूटने या Firebase तक न पहुँचने पर Disconnected दिखेगा। पुरानी heartbeat हटने में लगभग 18 सेकंड लग सकते हैं। यह किट और क्लाउड का कनेक्शन बताता है, ECG या SpO2 सेंसर का नहीं।'
    },
    {
      category: 'बटन', icon: Cpu,
      q: 'तीनों बटन कैसे काम करते हैं?',
      a: 'BACK पिछली स्क्रीन या डैशबोर्ड पर लौटाता है। NEXT टाइल बदलता है और दवा अलर्ट को Skip करता है। ENTER टाइल खोलता है, माप शुरू करता है या दवा अलर्ट को Taken दर्ज करता है। अपने किट पर लिखे बटन लेबल देखें।'
    },
    {
      category: 'SpO2', icon: Activity,
      q: 'SpO2 रीडिंग कैसे लें?',
      a: 'ENTER से SpO2 टाइल खोलें और किट के निर्देश के अनुसार MAX30102 सेंसर पर आराम से उंगली रखें। रीडिंग के दौरान हाथ स्थिर रखें और स्क्रीन पर प्रक्रिया पूरी होने दें। सेंसर या उंगली न मिलने का संदेश आए तो उंगली की स्थिति ठीक करके फिर कोशिश करें।'
    },
    {
      category: 'ECG + Heart', icon: HeartPulse,
      q: 'ECG + Heart कैसे इस्तेमाल करें?',
      a: 'ECG + Heart टाइल खोलें। AD8232 मॉड्यूल और तीनों इलेक्ट्रोड को हार्डवेयर लेबल तथा किट की वायरिंग/इलेक्ट्रोड गाइड के अनुसार जोड़ें। निर्देश के अनुसार इलेक्ट्रोड लगाकर स्थिर बैठें और स्थिर होने तथा माप पूरा होने दें। Disconnected या Leads Off दिखे तो मॉड्यूल और इलेक्ट्रोड कनेक्शन जाँचें।'
    },
    {
      category: 'दवाइयाँ', icon: Pill,
      q: 'दवा का रिमाइंडर कैसे सेट करें?',
      a: 'वेब पोर्टल में Medicines खोलें। दवा, खुराक, मरीज की जानकारी, तारीख, समय और अवधि भरकर सेव करें। रिमाइंडर पाने के लिए किट चालू, ऑनलाइन और Firebase से जुड़ी होनी चाहिए। खुराक खुद न तय करें; विवरण डॉक्टर के पर्चे से मिलाएँ।'
    },
    {
      category: 'दवाइयाँ', icon: Pill,
      q: 'दवा अलर्ट आने पर बटन क्या करते हैं?',
      a: 'ENTER वर्तमान अलर्ट को Taken, NEXT को Skip और BACK को Dismiss करता है। पुष्टि करने से पहले दवा और खुराक पर्चे से मिलाएँ। क्लाउड कनेक्शन उपलब्ध होने पर बटन का निर्णय वेब खाते में दर्ज होगा।'
    },
    {
      category: 'वेब डैशबोर्ड', icon: Globe2,
      q: 'रीडिंग और दवा शेड्यूल कहाँ मिलेंगे?',
      a: 'Overview में उपलब्ध रीडिंग और रिमाइंडर का सार है। Live Tests में टेस्ट पेज, Reports में रिकॉर्ड देखना और डाउनलोड करना, Medicines में शेड्यूल और स्थिति, Help में निर्देश और Profile में खाते की जानकारी व सेटिंग मिलती हैं। ऊपर का डिवाइस संकेत हाल की क्लाउड heartbeat बताता है।'
    },
    {
      category: 'वेब डैशबोर्ड', icon: FileText,
      q: 'रिपोर्ट कैसे बनाएँ?',
      a: 'Reports खोलें, उपलब्ध टेस्ट और तारीख की सीमा चुनें, दिखाए गए रिकॉर्ड जाँचें और डाउनलोड विकल्प चुनें। रिपोर्ट इसी खाते में उपलब्ध डेटा पर आधारित होती है; साझा करने से पहले तारीख और मान जाँच लें।'
    },
    {
      category: 'सुरक्षा', icon: ShieldAlert,
      q: 'क्या यह किट गंभीर बीमारी का निदान या प्रबंधन कर सकती है?',
      a: 'नहीं। यह किट और डैशबोर्ड केवल सामान्य रोज़मर्रा की जानकारी के लिए हैं। ये डॉक्टर की सलाह, निदान, इलाज या आपातकालीन सेवा का विकल्प नहीं हैं। गंभीर स्वास्थ्य निर्णय के लिए रीडिंग पर निर्भर न हों और इलाज लेने में देरी न करें। लक्षण चिंताजनक हों या समस्या गंभीर हो तो किट का उपयोग रोकें और योग्य डॉक्टर या स्थानीय आपातकालीन सेवा से संपर्क करें।'
    },
    {
      category: 'समस्या समाधान', icon: CircleHelp,
      q: 'डैशबोर्ड Disconnected दिखाए तो क्या जाँचें?',
      a: 'देखें कि ESP32 चालू है, Wi-Fi से जुड़ी है और इंटरनेट चल रहा है। heartbeat आने के लिए थोड़ा इंतज़ार करें। फिर भी Disconnected हो तो किट रीस्टार्ट करके Wi-Fi सेटअप जाँचें। हरा डिवाइस स्टेटस यह साबित नहीं करता कि हर सेंसर जुड़ा है या सही डेटा दे रहा है।'
    }
  ]
};

export default function HelpPage() {
  const [language, setLanguage] = useState('en');
  const [openFaq, setOpenFaq] = useState(0);
  const faqs = faqText[language];
  const overviewCards = language === 'en'
    ? [['Hardware', 'Wi-Fi, buttons and sensors'], ['Web dashboard', 'Readings, reports and medicines'], ['Safety', 'Everyday information only']]
    : [['हार्डवेयर', 'Wi-Fi, बटन और सेंसर'], ['वेब डैशबोर्ड', 'रीडिंग, रिपोर्ट और दवाइयाँ'], ['सुरक्षा', 'केवल सामान्य जानकारी']];

  return (
    <div className="mx-auto max-w-5xl space-y-7 pb-8">
      <header className="flex flex-wrap items-end justify-between gap-4">
        <div>
          <div className="mb-3 inline-flex items-center gap-2 rounded-full border border-cyan-800/40 bg-cyan-950/60 px-3.5 py-1 text-xs font-semibold uppercase tracking-wider text-cyan-300">
            <Cpu className="h-4 w-4" /> {language === 'en' ? 'MEDICARE+ user guide' : 'MEDICARE+ उपयोगकर्ता गाइड'}
          </div>
          <h1 className="text-3xl font-extrabold tracking-tight text-white">
            {language === 'en' ? 'Hardware & Web Dashboard Help' : 'हार्डवेयर और वेब डैशबोर्ड सहायता'}
          </h1>
          <p className="mt-1 max-w-3xl text-sm text-slate-400">
            {language === 'en'
              ? 'Step-by-step answers for setup, buttons, sensors, medicine reminders, and the web portal.'
              : 'सेटअप, बटन, सेंसर, दवा रिमाइंडर और वेब पोर्टल के आसान निर्देश।'}
          </p>
        </div>
        <div className="inline-flex rounded-xl border border-slate-700 bg-slate-900 p-1" aria-label="Choose language">
          <button onClick={() => { setLanguage('en'); setOpenFaq(0); }} className={`rounded-lg px-3 py-2 text-xs font-bold ${language === 'en' ? 'bg-cyan-500 text-slate-950' : 'text-slate-300'}`}><Languages className="mr-1 inline h-4 w-4" />English</button>
          <button onClick={() => { setLanguage('hi'); setOpenFaq(0); }} className={`rounded-lg px-3 py-2 text-xs font-bold ${language === 'hi' ? 'bg-cyan-500 text-slate-950' : 'text-slate-300'}`}>हिन्दी</button>
        </div>
      </header>

      <div className="grid gap-3 sm:grid-cols-3">
        {overviewCards.map(([title, detail]) => (
          <div key={title} className="rounded-2xl border border-slate-800 bg-slate-950/70 p-4">
            <p className="text-sm font-bold text-cyan-300">{title}</p>
            <p className="mt-1 text-xs text-slate-400">{detail}</p>
          </div>
        ))}
      </div>

      <section className="space-y-3" aria-label={language === 'en' ? 'Frequently asked questions' : 'अक्सर पूछे जाने वाले सवाल'}>
        {faqs.map((faq, index) => {
          const Icon = faq.icon;
          const expanded = openFaq === index;
          return (
            <article key={`${language}-${faq.q}`} className="overflow-hidden rounded-2xl border border-slate-800 bg-slate-950/65">
              <button className="flex w-full items-center gap-3 p-4 text-left sm:p-5" onClick={() => setOpenFaq(expanded ? -1 : index)} aria-expanded={expanded}>
                <span className="grid h-9 w-9 shrink-0 place-items-center rounded-xl bg-cyan-950 text-cyan-300"><Icon className="h-4 w-4" /></span>
                <span className="min-w-0 flex-1">
                  <span className="block text-[10px] font-bold uppercase tracking-wider text-cyan-400">{faq.category}</span>
                  <span className="mt-1 block text-sm font-bold leading-snug text-white">{faq.q}</span>
                </span>
                <ChevronDown className={`h-4 w-4 shrink-0 text-slate-400 transition-transform ${expanded ? 'rotate-180' : ''}`} />
              </button>
              {expanded && <p className="border-t border-slate-800 px-4 pb-5 pt-4 text-sm leading-relaxed text-slate-300 sm:px-5 sm:pl-[4.5rem]">{faq.a}</p>}
            </article>
          );
        })}
      </section>
    </div>
  );
}

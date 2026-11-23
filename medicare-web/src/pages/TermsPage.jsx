import React, { useState } from 'react';
import { Languages, ShieldAlert, HeartPulse, Cpu, Wifi, Pill, Database, FileText, RefreshCw, Scale } from 'lucide-react';

const terms = {
  en: {
    badge: 'Service terms', title: 'Terms & Conditions',
    intro: 'Please read these terms before using the MEDICARE+ web dashboard or Smart Pocket Medical Kit.',
    sections: [
      { icon: FileText, title: '1. What this service is', body: 'MEDICARE+ provides a web dashboard and companion ESP32-based hardware interface for viewing general wellness readings, keeping records, and managing user-entered medicine reminders. Features may change as the project is maintained.' },
      { icon: HeartPulse, title: '2. General information only — not for serious conditions', body: 'The kit and dashboard are intended for general, everyday informational use only. They are not a substitute for professional medical advice, examination, diagnosis, treatment, or emergency care. Do not use this device to diagnose, monitor, or manage a serious condition, and do not make urgent health decisions from its readings. If you have a serious problem, worrying symptoms, or think you may need urgent care, stop relying on the device and contact a qualified doctor or local emergency service immediately.' },
      { icon: ShieldAlert, title: '3. Readings have limitations', body: 'Sensor contact, movement, fit, power, wiring, environment, network delays, software, and other factors can affect whether a reading is available or accurate. A missing, unusual, normal-looking, or delayed value does not prove that you are healthy or unwell. The dashboard may show old data when the device or internet is disconnected. Always consult a qualified healthcare professional for interpretation and decisions.' },
      { icon: Cpu, title: '4. Safe hardware use', body: 'Use the kit only as described in its setup and wiring instructions. Inspect cables, sensor modules, electrodes, and the power supply before use. Do not use damaged parts, modify wiring while powered, expose the electronics to water, or use the kit in a way that could cause injury. Keep small parts and cables away from children. Stop using the hardware if it becomes hot, damaged, or behaves unexpectedly.' },
      { icon: Pill, title: '5. Medicine reminders are user-entered', body: 'Medicine names, doses, schedules, and patient details are entered by users. The service does not prescribe, verify, recommend, or adjust medication. Reminders can be delayed or missed if the kit is off, offline, muted, or unable to reach the cloud. Confirm every medicine and dose against the prescription and advice of a qualified healthcare professional. Never start, stop, or change a medicine based only on this service.' },
      { icon: Wifi, title: '6. Connectivity and availability', body: 'Device status means that the ESP32 has recently communicated with the web service; it does not confirm that a sensor is attached or working. Internet, Wi-Fi, power, Firebase, browser, or maintenance issues may interrupt synchronization, notifications, reports, or access. Do not depend on the service for emergency communication or time-critical care.' },
      { icon: Database, title: '7. Accounts and information', body: 'You are responsible for keeping your sign-in credentials private and for using an account you are authorized to access. The application may store or synchronize profile information, readings, and medicine schedules through Firebase and may keep limited cached data in your browser. Availability and retention depend on the project’s cloud configuration and your browser. Enter only information you are authorized to provide, and avoid sharing unnecessary sensitive details.' },
      { icon: FileText, title: '8. Your responsibilities', body: 'Use the dashboard lawfully and for its intended general-information purpose. Keep your account details current, review the values and dates before relying on or sharing a report, and ensure that any person whose information you enter has given appropriate permission. You are responsible for maintaining a safe Wi-Fi network and compatible hardware.' },
      { icon: Scale, title: '9. Service and third-party components', body: 'The dashboard depends on third-party services, including Firebase, and on compatible browsers and network providers. Those services may have their own terms and privacy practices. MEDICARE+ may be unavailable or change without notice. These terms do not remove rights that cannot be excluded under applicable law.' },
      { icon: RefreshCw, title: '10. Updates to these terms', body: 'These terms may be revised when the service changes. The latest version will appear on this page with its updated date. Continued use after an update means you have had the opportunity to review the revised terms, subject to rights under applicable law.' },
      { icon: Scale, title: '11. No warranty; liability', body: 'The service is provided on an “as available” basis. To the extent permitted by applicable law, no guarantee is made that readings, reminders, connectivity, reports, or service availability will be uninterrupted or error-free. Nothing here excludes liability or consumer rights that applicable law does not allow to be excluded.' },
      { icon: FileText, title: '12. Contact and questions', body: 'For product or account questions, use the Help section of the dashboard or contact the project administrator through the support channel provided with your installation. Do not send emergency or urgent medical requests through the website.' }
    ],
    footer: 'Last updated: 23 November 2026. This page provides general service terms and does not replace any legally required notice.'
  },
  hi: {
    badge: 'सेवा की शर्तें', title: 'नियम और शर्तें',
    intro: 'MEDICARE+ वेब डैशबोर्ड या Smart Pocket Medical Kit इस्तेमाल करने से पहले ये शर्तें पढ़ें।',
    sections: [
      { icon: FileText, title: '1. यह सेवा क्या है', body: 'MEDICARE+ सामान्य स्वास्थ्य-संबंधी रीडिंग देखने, रिकॉर्ड रखने और उपयोगकर्ता द्वारा दर्ज दवा रिमाइंडर सँभालने के लिए वेब डैशबोर्ड और ESP32 हार्डवेयर इंटरफ़ेस देता है। प्रोजेक्ट के रखरखाव के दौरान सुविधाएँ बदल सकती हैं।' },
      { icon: HeartPulse, title: '2. केवल सामान्य जानकारी — गंभीर समस्या के लिए नहीं', body: 'किट और डैशबोर्ड केवल रोज़मर्रा की सामान्य जानकारी के लिए हैं। ये डॉक्टर की सलाह, जाँच, निदान, इलाज या आपातकालीन सेवा का विकल्प नहीं हैं। गंभीर बीमारी का निदान, निगरानी या प्रबंधन करने के लिए इस डिवाइस का इस्तेमाल न करें और इसकी रीडिंग से तुरंत स्वास्थ्य-निर्णय न लें। गंभीर समस्या, चिंताजनक लक्षण या तत्काल इलाज की ज़रूरत लगे तो डिवाइस पर निर्भर न रहें और तुरंत योग्य डॉक्टर या स्थानीय आपातकालीन सेवा से संपर्क करें।' },
      { icon: ShieldAlert, title: '3. रीडिंग की सीमाएँ', body: 'सेंसर का संपर्क, शरीर की हलचल, फिट, पावर, वायरिंग, वातावरण, नेटवर्क देरी और सॉफ्टवेयर रीडिंग की उपलब्धता या सटीकता को प्रभावित कर सकते हैं। कोई मान न मिलना, असामान्य लगना, सामान्य दिखना या देर से आना यह साबित नहीं करता कि आप स्वस्थ हैं या बीमार। डिवाइस या इंटरनेट बंद होने पर डैशबोर्ड पुराना डेटा दिखा सकता है। अर्थ समझने और निर्णय लेने के लिए योग्य स्वास्थ्य पेशेवर से सलाह लें।' },
      { icon: Cpu, title: '4. हार्डवेयर सुरक्षित तरीके से इस्तेमाल करें', body: 'किट को उसके सेटअप और वायरिंग निर्देशों के अनुसार ही इस्तेमाल करें। उपयोग से पहले केबल, सेंसर मॉड्यूल, इलेक्ट्रोड और पावर सप्लाई जाँचें। खराब हिस्से न इस्तेमाल करें, चालू हालत में वायरिंग न बदलें, इलेक्ट्रॉनिक्स को पानी से दूर रखें और चोट पहुँचाने वाले तरीके से इस्तेमाल न करें। छोटे हिस्से और तार बच्चों से दूर रखें। हार्डवेयर गर्म हो, खराब हो या अनपेक्षित व्यवहार करे तो इस्तेमाल रोक दें।' },
      { icon: Pill, title: '5. दवा रिमाइंडर उपयोगकर्ता द्वारा भरे जाते हैं', body: 'दवा का नाम, खुराक, समय-सारणी और मरीज की जानकारी उपयोगकर्ता दर्ज करता है। सेवा दवा लिखती, जाँचती, सुझाती या बदलती नहीं है। किट बंद, ऑफलाइन, म्यूट या क्लाउड से कनेक्ट न हो तो रिमाइंडर देर से आ सकता है या छूट सकता है। हर दवा और खुराक पर्चे तथा योग्य स्वास्थ्य पेशेवर की सलाह से मिलाएँ। केवल इस सेवा के आधार पर दवा शुरू, बंद या बदलें नहीं।' },
      { icon: Wifi, title: '6. कनेक्टिविटी और उपलब्धता', body: 'डिवाइस स्टेटस बताता है कि ESP32 ने हाल में वेब सेवा से संपर्क किया है; यह नहीं बताता कि सेंसर लगा है या काम कर रहा है। इंटरनेट, Wi-Fi, पावर, Firebase, ब्राउज़र या रखरखाव की समस्या से सिंक, नोटिफिकेशन, रिपोर्ट या वेबसाइट बाधित हो सकते हैं। आपातकालीन संपर्क या समय-संवेदनशील इलाज के लिए इस सेवा पर निर्भर न रहें।' },
      { icon: Database, title: '7. खाता और जानकारी', body: 'अपने लॉगिन विवरण सुरक्षित रखना और केवल अधिकृत खाते का उपयोग करना आपकी ज़िम्मेदारी है। ऐप Firebase के माध्यम से प्रोफ़ाइल, रीडिंग और दवा शेड्यूल सहेज या सिंक कर सकता है और ब्राउज़र में कुछ डेटा कैश कर सकता है। उपलब्धता और डेटा रहने की अवधि प्रोजेक्ट की क्लाउड सेटिंग तथा ब्राउज़र पर निर्भर है। केवल वही जानकारी भरें जिसे देने की अनुमति है; अनावश्यक संवेदनशील जानकारी साझा न करें।' },
      { icon: FileText, title: '8. आपकी ज़िम्मेदारियाँ', body: 'डैशबोर्ड का उपयोग कानून के अनुसार और सामान्य जानकारी के उद्देश्य से करें। खाता जानकारी सही रखें, रिपोर्ट पर भरोसा करने या साझा करने से पहले तारीख और मान जाँचें, और किसी दूसरे व्यक्ति की जानकारी दर्ज करने से पहले उचित अनुमति लें। सुरक्षित Wi-Fi नेटवर्क और संगत हार्डवेयर बनाए रखना आपकी ज़िम्मेदारी है।' },
      { icon: Scale, title: '9. सेवा और तीसरे पक्ष की सुविधाएँ', body: 'डैशबोर्ड Firebase जैसी तीसरे पक्ष की सेवाओं, संगत ब्राउज़र और नेटवर्क प्रदाताओं पर निर्भर है। उन सेवाओं की अपनी शर्तें और गोपनीयता प्रक्रियाएँ हो सकती हैं। MEDICARE+ बिना सूचना के अनुपलब्ध हो सकता है या बदल सकता है। लागू कानून के तहत जिन अधिकारों को हटाया नहीं जा सकता, ये शर्तें उन्हें नहीं हटातीं।' },
      { icon: RefreshCw, title: '10. शर्तों में बदलाव', body: 'सेवा बदलने पर इन शर्तों को संशोधित किया जा सकता है। नया संस्करण इसी पेज पर तारीख के साथ दिखेगा। बदलाव के बाद सेवा का उपयोग जारी रखने पर आपको संशोधित शर्तें देखने का अवसर माना जाएगा, लागू कानून के अधिकारों के अधीन।' },
      { icon: Scale, title: '11. वारंटी और दायित्व', body: 'सेवा “उपलब्धता के अनुसार” दी जाती है। लागू कानून की सीमा के भीतर रीडिंग, रिमाइंडर, कनेक्टिविटी, रिपोर्ट या सेवा बिना रुकावट या त्रुटि के चलने की गारंटी नहीं है। लागू कानून के तहत जिन दायित्वों या उपभोक्ता अधिकारों को हटाया नहीं जा सकता, उन्हें यहाँ हटाया नहीं गया है।' },
      { icon: FileText, title: '12. संपर्क और सवाल', body: 'उत्पाद या खाते से जुड़े सवालों के लिए डैशबोर्ड का Help भाग देखें या इंस्टॉलेशन के साथ दिए गए सहायता माध्यम से प्रोजेक्ट प्रशासक से संपर्क करें। आपातकालीन या तत्काल चिकित्सा सहायता के अनुरोध वेबसाइट से न भेजें।' }
    ],
    footer: 'अंतिम अपडेट: 23 नवंबर 2026। यह पेज सेवा की सामान्य शर्तें बताता है और किसी कानूनी रूप से आवश्यक सूचना का विकल्प नहीं है।'
  }
};

export default function TermsPage() {
  const [language, setLanguage] = useState('en');
  const copy = terms[language];

  return (
    <div className="mx-auto max-w-5xl space-y-7 pb-8">
      <header className="flex flex-wrap items-end justify-between gap-4">
        <div>
          <span className="mb-3 inline-flex items-center gap-2 rounded-full border border-cyan-800/40 bg-cyan-950/60 px-3.5 py-1 text-xs font-bold uppercase tracking-wider text-cyan-300">
            <Scale className="h-4 w-4" /> {copy.badge}
          </span>
          <h1 className="text-3xl font-extrabold tracking-tight text-white">{copy.title}</h1>
          <p className="mt-2 max-w-3xl text-sm leading-relaxed text-slate-400">{copy.intro}</p>
        </div>
        <div className="inline-flex rounded-xl border border-slate-700 bg-slate-900 p-1" aria-label="Choose language">
          <button onClick={() => setLanguage('en')} className={`rounded-lg px-3 py-2 text-xs font-bold ${language === 'en' ? 'bg-cyan-500 text-slate-950' : 'text-slate-300'}`}><Languages className="mr-1 inline h-4 w-4" />English</button>
          <button onClick={() => setLanguage('hi')} className={`rounded-lg px-3 py-2 text-xs font-bold ${language === 'hi' ? 'bg-cyan-500 text-slate-950' : 'text-slate-300'}`}>हिन्दी</button>
        </div>
      </header>

      <div className="rounded-2xl border border-amber-500/30 bg-amber-500/10 p-4 text-sm leading-relaxed text-amber-100">
        <strong className="mb-1 flex items-center gap-2 text-amber-300"><ShieldAlert className="h-4 w-4" />
          {language === 'en' ? 'Health and emergency notice' : 'स्वास्थ्य और आपातकालीन सूचना'}
        </strong>
        {language === 'en'
          ? 'For everyday informational use only. Do not use this kit for a serious condition or as a replacement for professional care. Seek a qualified doctor for health concerns; use local emergency services for urgent problems.'
          : 'केवल रोज़मर्रा की सामान्य जानकारी के लिए। गंभीर समस्या में इस किट का उपयोग या पेशेवर देखभाल के विकल्प के रूप में न करें। स्वास्थ्य चिंता के लिए योग्य डॉक्टर और तत्काल समस्या में स्थानीय आपातकालीन सेवा से संपर्क करें।'}
      </div>

      <section className="space-y-3">
        {copy.sections.map((section) => {
          const Icon = section.icon;
          return (
            <article key={section.title} className="rounded-2xl border border-slate-800 bg-slate-950/65 p-5 sm:p-6">
              <h2 className="flex items-center gap-3 text-base font-bold text-white">
                <span className="grid h-9 w-9 shrink-0 place-items-center rounded-xl bg-cyan-950 text-cyan-300"><Icon className="h-4 w-4" /></span>
                {section.title}
              </h2>
              <p className="mt-3 text-sm leading-relaxed text-slate-300">{section.body}</p>
            </article>
          );
        })}
      </section>

      <p className="border-t border-slate-800 pt-4 text-xs leading-relaxed text-slate-500">{copy.footer}</p>
    </div>
  );
}

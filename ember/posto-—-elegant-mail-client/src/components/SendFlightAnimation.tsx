import React, { useState, useEffect } from 'react';
import { Send, CheckCircle2, Sparkles, User, Mail, ShieldCheck, ArrowRight } from 'lucide-react';

interface SendFlightAnimationProps {
  senderName: string;
  senderEmail: string;
  recipientEmail: string;
  subject: string;
  onComplete: () => void;
}

export const SendFlightAnimation: React.FC<SendFlightAnimationProps> = ({
  senderName,
  senderEmail,
  recipientEmail,
  subject,
  onComplete,
}) => {
  const [phase, setPhase] = useState<number>(0);

  useEffect(() => {
    // Phase 0: Takeoff / Stamping (0 - 800ms)
    // Phase 1: In-Flight Arc across to receiver (800 - 2000ms)
    // Phase 2: Landing in recipient's mailbox (2000 - 2800ms)
    // Phase 3: Delivered celebration & auto-close (2800ms+)
    const t1 = setTimeout(() => setPhase(1), 800);
    const t2 = setTimeout(() => setPhase(2), 2000);
    const t3 = setTimeout(() => {
      onComplete();
    }, 3200);

    return () => {
      clearTimeout(t1);
      clearTimeout(t2);
      clearTimeout(t3);
    };
  }, [onComplete]);

  return (
    <div className="fixed inset-0 z-50 bg-[#140A05]/90 backdrop-blur-md flex items-center justify-center p-4 select-none animate-in fade-in duration-150">
      
      {/* Flight Card */}
      <div className="relative w-full max-w-xl bg-[#FAF5EC] border-2 border-[#D8C4AA] rounded-3xl shadow-2xl overflow-hidden p-8 flex flex-col items-center">
        
        {/* Airmail Ribbon Border on top */}
        <div className="absolute top-0 left-0 right-0 h-2 bg-[repeating-linear-gradient(45deg,#C8622A,#C8622A_14px,#FAF5EC_14px,#FAF5EC_28px,#422616_28px,#422616_42px,#FAF5EC_42px,#FAF5EC_56px)]" />

        <div className="text-[10px] font-mono tracking-widest uppercase font-bold text-[#A86438] bg-[#F2E5D4] px-3.5 py-1 rounded-full border border-[#D8C4AA] mb-3 flex items-center gap-1.5 mt-2">
          <Sparkles className="w-3 h-3 text-[#C8622A]" />
          <span>AIRMAIL TRANSIT DISPATCH</span>
        </div>

        <h3 className="text-2xl font-serif font-bold text-[#2A180E] text-center">
          {phase < 2 ? 'Dispatching Mail in Transit...' : 'Mail Successfully Delivered!'}
        </h3>
        <p className="text-xs text-[#7A5B44] text-center mt-1 truncate max-w-sm">
          "{subject || 'No Subject'}"
        </p>

        {/* Flight Trajectory Canvas */}
        <div className="relative w-full h-48 my-6 bg-[#F4EDE0] rounded-2xl border-2 border-[#D8C5AB] overflow-hidden flex items-center justify-between px-6 sm:px-10 shadow-inner">
          
          {/* Background Flight Dashed Path */}
          <svg className="absolute inset-0 w-full h-full pointer-events-none">
            <path
              d="M 60 120 Q 180 30 300 120"
              stroke="#D5C1A6"
              strokeWidth="2.5"
              strokeDasharray="6 6"
              fill="none"
              className="opacity-70"
            />
          </svg>

          {/* SENDER NODE (You) */}
          <div className="flex flex-col items-center gap-2 relative z-10">
            <div className={`w-14 h-14 rounded-2xl bg-[#3D2517] text-[#FAF5EC] flex items-center justify-center font-bold text-lg shadow-md border-2 border-[#8C5835] transition-transform duration-300 ${phase === 0 ? 'scale-110 ring-4 ring-[#C8622A]/30' : 'scale-100'}`}>
              {senderName.charAt(0)}
            </div>
            <div className="text-center">
              <span className="text-xs font-bold text-[#2A180E] block leading-tight">You</span>
              <span className="text-[10px] font-mono text-[#7A5B44] block truncate max-w-[90px]">
                {senderEmail.split('@')[0]}
              </span>
            </div>
          </div>

          {/* FLYING ENVELOPE (Transitions from Sender to Receiver) */}
          <div
            className={`absolute z-20 w-24 h-16 bg-[#FFFDF9] rounded-xl border-2 border-[#C5AE95] shadow-xl flex flex-col justify-between p-2 transition-all duration-1000 ease-in-out ${
              phase === 0
                ? 'left-14 top-16 scale-90 rotate-[-12deg]'
                : phase === 1
                ? 'left-[45%] -translate-x-1/2 top-6 scale-110 rotate-6 shadow-2xl'
                : 'left-[78%] -translate-x-1/2 top-16 scale-95 rotate-0 shadow-md'
            }`}
          >
            <div className="flex justify-between items-center border-b border-[#EADCC8] pb-0.5">
              <span className="text-[7px] font-mono font-bold text-[#A86438]">AIRMAIL</span>
              <span className="text-[7px] font-bold text-[#2A180E]">POSTO</span>
            </div>
            
            {/* Wax Seal */}
            <div className="flex items-center justify-center">
              <div className="w-5 h-5 rounded-full bg-gradient-to-br from-[#D9483B] to-[#8C2A12] text-white flex items-center justify-center shadow-xs text-[8px] font-serif font-black">
                P
              </div>
            </div>

            <div className="text-[6px] font-mono text-[#7A5B44] text-center">
              EXPRESS DISPATCH
            </div>

            {/* Jet Sparkle Trail */}
            {phase === 1 && (
              <div className="absolute -left-4 top-1/2 -translate-y-1/2">
                <Sparkles className="w-5 h-5 text-[#C8622A] animate-pulse" />
              </div>
            )}
          </div>

          {/* RECIPIENT NODE (Receiver) */}
          <div className="flex flex-col items-center gap-2 relative z-10">
            <div className={`w-14 h-14 rounded-2xl bg-[#5A3824] text-[#FAF5EC] flex items-center justify-center font-bold text-lg shadow-md border-2 border-[#A86438] transition-transform duration-300 ${phase >= 2 ? 'scale-110 ring-4 ring-[#3D8C40]/40 bg-[#3D8C40]' : 'scale-100'}`}>
              {phase >= 2 ? <CheckCircle2 className="w-7 h-7 text-white" /> : recipientEmail.charAt(0).toUpperCase()}
            </div>
            <div className="text-center">
              <span className="text-xs font-bold text-[#2A180E] block leading-tight">Receiver</span>
              <span className="text-[10px] font-mono text-[#7A5B44] block truncate max-w-[100px]">
                {recipientEmail.split('@')[0]}
              </span>
            </div>
          </div>

        </div>

        {/* Narrative Status Bar */}
        <div className="w-full max-w-sm flex items-center justify-between text-xs font-mono text-[#7A5B44] mb-4">
          <span className="font-semibold">
            {phase === 0 && 'Stamping & taking flight...'}
            {phase === 1 && 'Gliding across airmail route...'}
            {phase >= 2 && `Delivered into ${recipientEmail}!`}
          </span>
          <span className="font-bold text-[#A75427]">
            {phase === 0 ? '30%' : phase === 1 ? '70%' : '100%'}
          </span>
        </div>

        {/* Action Button */}
        <button
          onClick={onComplete}
          className="px-6 py-2.5 rounded-xl bg-[#3D2517] hover:bg-[#2A180E] text-[#FAF5EC] text-xs font-bold transition-all shadow-md flex items-center gap-2"
        >
          <span>{phase >= 2 ? 'View in Sent Mail' : 'Complete Dispatch'}</span>
          <ArrowRight className="w-3.5 h-3.5" />
        </button>

      </div>

    </div>
  );
};

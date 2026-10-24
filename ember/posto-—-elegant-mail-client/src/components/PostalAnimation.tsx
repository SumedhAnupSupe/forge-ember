import React, { useState, useEffect } from 'react';
import { ArrowRight, Sparkles, CheckCircle2, Shield, Stamp } from 'lucide-react';
import { Logo } from './Logo';

interface PostalAnimationProps {
  onComplete: () => void;
}

export const PostalAnimation: React.FC<PostalAnimationProps> = ({ onComplete }) => {
  const [stage, setStage] = useState<number>(0);

  useEffect(() => {
    // Stage 0: Flight / Approach (0 - 1000ms)
    // Stage 1: Slot Drop (1000 - 2200ms)
    // Stage 2: Hot Wax Stamping with Sparks (2200 - 3600ms)
    // Stage 3: Ready!
    const t1 = setTimeout(() => setStage(1), 1000);
    const t2 = setTimeout(() => setStage(2), 2200);
    const t3 = setTimeout(() => setStage(3), 3600);

    return () => {
      clearTimeout(t1);
      clearTimeout(t2);
      clearTimeout(t3);
    };
  }, []);

  return (
    <div className="fixed inset-0 z-50 bg-[#140A05]/95 backdrop-blur-lg flex items-center justify-center p-4 select-none">
      
      {/* Decorative Postal Dispatch Container */}
      <div className="relative w-full max-w-xl bg-[#FAF5EC] border-2 border-[#D8C4AA] rounded-3xl shadow-2xl overflow-hidden flex flex-col items-center animate-in zoom-in-95 duration-200">
        
        {/* Dynamic Airmail Ribbon Accent */}
        <div className="h-2.5 w-full bg-[repeating-linear-gradient(45deg,#C8622A,#C8622A_14px,#FAF5EC_14px,#FAF5EC_28px,#422616_28px,#422616_42px,#FAF5EC_42px,#FAF5EC_56px)]" />

        <div className="p-8 md:p-10 flex flex-col items-center text-center w-full">
          
          <Logo size="lg" className="mb-4" />

          <div className="text-[10px] font-mono tracking-widest uppercase font-bold text-[#A86438] bg-[#F2E5D4] px-3.5 py-1 rounded-full border border-[#D8C4AA] mb-2 flex items-center gap-1.5">
            <Stamp className="w-3 h-3 text-[#C8622A]" />
            <span>POSTO · PARCEL & MAIL DISPATCH</span>
          </div>

          <h2 className="text-2xl md:text-3xl font-serif font-bold text-[#2A180E] tracking-tight">
            Incoming Airmail
          </h2>
          <p className="text-xs text-[#7A5B44] mt-1 max-w-xs font-sans">
            Authenticating recipient and sealing mail for private dispatch.
          </p>

          {/* Interactive Postal Animation Stage */}
          <div className="relative w-full max-w-sm h-56 my-6 bg-gradient-to-b from-[#F2E8DA] to-[#EADBCA] rounded-2xl border-2 border-[#D8C5AB] overflow-hidden flex flex-col items-center justify-center shadow-inner">
            
            {/* Ambient Background Postal Grid */}
            <div className="absolute inset-0 bg-[radial-gradient(#C8733E_1px,transparent_1px)] [background-size:16px_16px] opacity-20 pointer-events-none" />

            {/* Brass Drop Slot */}
            <div className="relative z-10 w-64 h-12 rounded-xl bg-[#2A160D] border-2 border-[#C88A58] shadow-lg flex items-center justify-center px-4">
              <div className="w-full h-2.5 rounded-full bg-[#120803] border border-[#52331F]" />
              <div className="absolute -top-2.5 px-3 py-0.5 rounded bg-[#C88A58] text-[#2A160D] text-[9px] font-mono font-bold uppercase tracking-wider">
                MAIL ONLY
              </div>
            </div>

            {/* The Animated Envelope */}
            <div
              className={`absolute z-20 w-48 h-32 bg-[#FFFDF9] rounded-xl border-2 border-[#C5AE95] shadow-2xl flex flex-col justify-between p-3.5 transition-all duration-700 ease-out ${
                stage === 0
                  ? 'translate-y-20 scale-90 opacity-60 rotate-6'
                  : stage === 1
                  ? 'translate-y-[-12px] scale-100 opacity-100 rotate-0'
                  : stage === 2
                  ? 'translate-y-[-28px] scale-95 opacity-90 rotate-[-2deg]'
                  : 'translate-y-[-64px] scale-75 opacity-0'
              }`}
            >
              {/* Envelope Airmail border on top */}
              <div className="flex justify-between items-start border-b border-[#EADCC8] pb-1.5">
                <div className="flex items-center gap-1.5">
                  <span className="w-2 h-2 rounded-full bg-[#C8622A]" />
                  <span className="text-[9px] font-mono tracking-wider font-bold text-[#5A3824]">AIRMAIL PRIORITY</span>
                </div>
                {/* Stamp token */}
                <div className="w-7 h-8 border border-dashed border-[#C8622A] bg-[#FAF5EC] flex flex-col items-center justify-center p-0.5 shadow-2xs">
                  <span className="text-[7px] font-mono text-[#C8622A] font-bold">AIR</span>
                  <span className="text-[8px] font-bold text-[#2D1C13]">2026</span>
                </div>
              </div>

              {/* Envelope Center Monogram Seal (Appears on stage >= 2) */}
              <div className="flex items-center justify-center my-1 relative">
                <div
                  className={`w-9 h-9 rounded-full bg-gradient-to-br from-[#D9483B] to-[#8C2A12] text-white flex items-center justify-center shadow-md border-2 border-[#F4A261] transition-transform duration-300 ${
                    stage >= 2 ? 'scale-100 rotate-0' : 'scale-0 rotate-45'
                  }`}
                >
                  <span className="text-xs font-serif font-black">P</span>
                </div>

                {/* Stamping Sparks Effect */}
                {stage === 2 && (
                  <div className="absolute inset-0 flex items-center justify-center pointer-events-none">
                    <Sparkles className="w-12 h-12 text-[#F4A261] animate-ping" />
                  </div>
                )}
              </div>

              <div className="flex items-center justify-between text-[8px] font-mono text-[#7A5B44]">
                <span>CONFIDENTIAL</span>
                <span>SEAL NO. 9841</span>
              </div>
            </div>

            {/* Verification Wave Ring on Stamping */}
            {stage >= 2 && (
              <div className="absolute inset-0 flex items-center justify-center pointer-events-none animate-in fade-in duration-300">
                <div className="w-44 h-44 rounded-full border-2 border-[#C8622A]/40 animate-ping" />
              </div>
            )}

          </div>

          {/* Stepper Status Indicators */}
          <div className="w-full max-w-xs flex flex-col gap-2 mb-6">
            <div className="flex items-center justify-between text-xs font-mono text-[#7A5B44]">
              <span className="font-semibold">
                {stage === 0 && '1. Airmail incoming...'}
                {stage === 1 && '2. Sliding mail into brass slot...'}
                {stage === 2 && '3. Stamping wax seal...'}
                {stage === 3 && '4. Mail sealed & ready!'}
              </span>
              <span className="font-bold text-[#A75427]">
                {((stage + 1) * 25)}%
              </span>
            </div>

            <div className="w-full h-1.5 bg-[#E4D5C2] rounded-full overflow-hidden">
              <div
                className="h-full bg-gradient-to-r from-[#A75427] to-[#C8622A] transition-all duration-500 rounded-full"
                style={{ width: `${((stage + 1) / 4) * 100}%` }}
              />
            </div>
          </div>

          {/* Action to Advance */}
          <button
            onClick={onComplete}
            className="w-full max-w-xs py-3 rounded-2xl bg-gradient-to-r from-[#3D2517] to-[#5A3824] hover:from-[#2B180E] hover:to-[#462918] text-[#FAF5EC] text-xs font-bold transition-all shadow-md hover:shadow-lg flex items-center justify-center gap-2"
          >
            <span>Proceed to Sign In</span>
            <ArrowRight className="w-4 h-4" />
          </button>

        </div>

      </div>

    </div>
  );
};

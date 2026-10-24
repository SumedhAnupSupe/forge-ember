import React from 'react';

interface LogoProps {
  className?: string;
  size?: 'sm' | 'md' | 'lg' | 'xl';
  showSubtitle?: boolean;
}

export const Logo: React.FC<LogoProps> = ({
  className = '',
  size = 'md',
  showSubtitle = false,
}) => {
  const iconDimensions = {
    sm: 'w-8 h-8',
    md: 'w-10 h-10',
    lg: 'w-14 h-14',
    xl: 'w-20 h-20',
  }[size];

  const titleSizes = {
    sm: 'text-base',
    md: 'text-xl',
    lg: 'text-2xl',
    xl: 'text-4xl',
  }[size];

  return (
    <div className={`flex items-center gap-3 select-none ${className}`}>
      {/* Sleek Falcon Courier & Wax-Sealed Letter Shield */}
      <div
        className={`relative flex items-center justify-center shrink-0 rounded-2xl bg-gradient-to-br from-[#2D1A10] via-[#1F1008] to-[#120804] p-2 shadow-lg border border-[#A75427]/40 overflow-hidden ${iconDimensions}`}
      >
        {/* Subtle radial warm glow */}
        <div className="absolute inset-0 bg-radial from-[#F4A261]/25 via-transparent to-transparent pointer-events-none" />

        <svg
          viewBox="0 0 64 64"
          fill="none"
          xmlns="http://www.w3.org/2000/svg"
          className="w-full h-full relative z-10"
        >
          {/* Geometric Diamond Postal Crest */}
          <path
            d="M32 4L58 32L32 60L6 32L32 4Z"
            stroke="url(#crestGold)"
            strokeWidth="2"
            fill="#26140B"
          />

          {/* Falcon Wing Left */}
          <path
            d="M32 18L14 28L24 38L32 30V18Z"
            fill="url(#wingGradientLeft)"
            opacity="0.9"
          />

          {/* Falcon Wing Right */}
          <path
            d="M32 18L50 28L40 38L32 30V18Z"
            fill="url(#wingGradientRight)"
            opacity="0.9"
          />

          {/* Floating Courier Envelope */}
          <rect
            x="20"
            y="26"
            width="24"
            height="16"
            rx="2.5"
            fill="#FFFDF9"
            stroke="#C8622A"
            strokeWidth="1.6"
          />
          {/* Flap fold */}
          <path
            d="M21 27L32 35L43 27"
            stroke="#C8622A"
            strokeWidth="1.6"
            strokeLinecap="round"
          />

          {/* Radiant Monogram Wax Seal */}
          <circle cx="32" cy="34" r="4.2" fill="url(#sealGlow)" stroke="#FFFDF9" strokeWidth="1" />
          <path
            d="M30.5 32H32.5C33.3 32 33.8 32.4 33.8 33.2C33.8 34 33.3 34.4 32.5 34.4H31.5V36H30.5V32ZM31.5 33.6H32.4C32.8 33.6 33 33.4 33 33.2C33 33 32.8 32.8 32.4 32.8H31.5V33.6Z"
            fill="#FFFDF9"
          />

          {/* Spark of speed */}
          <circle cx="32" cy="12" r="1.5" fill="#F4A261" />

          <defs>
            <linearGradient id="crestGold" x1="6" y1="4" x2="58" y2="60" gradientUnits="userSpaceOnUse">
              <stop offset="0%" stopColor="#F4A261" />
              <stop offset="50%" stopColor="#C8733E" />
              <stop offset="100%" stopColor="#7A3D18" />
            </linearGradient>
            <linearGradient id="wingGradientLeft" x1="14" y1="18" x2="32" y2="38" gradientUnits="userSpaceOnUse">
              <stop offset="0%" stopColor="#E59866" />
              <stop offset="100%" stopColor="#8C4E26" />
            </linearGradient>
            <linearGradient id="wingGradientRight" x1="50" y1="18" x2="32" y2="38" gradientUnits="userSpaceOnUse">
              <stop offset="0%" stopColor="#F4A261" />
              <stop offset="100%" stopColor="#A85B28" />
            </linearGradient>
            <linearGradient id="sealGlow" x1="32" y1="30" x2="32" y2="38" gradientUnits="userSpaceOnUse">
              <stop offset="0%" stopColor="#E76F51" />
              <stop offset="100%" stopColor="#B23A1E" />
            </linearGradient>
          </defs>
        </svg>
      </div>

      {/* Styled Brand Wordmark */}
      <div className="flex flex-col">
        <div className="flex items-center gap-2">
          <span className={`font-black tracking-tight text-[#2A160D] font-sans ${titleSizes}`}>
            POSTO
          </span>
          <span className="text-[10px] font-mono tracking-widest px-2 py-0.5 rounded-md bg-[#EDE1D0] text-[#6E4226] border border-[#D5BEA4] font-bold uppercase shadow-2xs">
            AIRMAIL
          </span>
        </div>
        {showSubtitle && (
          <span className="text-[11px] font-medium tracking-wide text-[#7C5A42]">
            Curated Correspondence
          </span>
        )}
      </div>
    </div>
  );
};

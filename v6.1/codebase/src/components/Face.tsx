import React, { useEffect, useState } from 'react';
import { motion } from 'framer-motion';

export type FaceState = 'idle' | 'happy' | 'talking' | 'menu' | 'dizzy' | 'crying' | 'naughty' | 'wink' | 'breath' | 'sleep_wake' | 'look_left' | 'look_right' | 'yawn' | 'lost' | 'celebrate' | 'excited' | 'angry' | 'curious' | 'thinking' | 'surprised' | 'deep_sleep' | 'light_rest' | 'alert';

interface FaceProps {
  state: FaceState;
  theme: 'tech' | 'child' | 'dev';
}

export const Face: React.FC<FaceProps> = ({ state, theme }) => {
  const [blink, setBlink] = useState(false);

  useEffect(() => {
    const noBlinkStates = ['menu', 'dizzy', 'crying', 'happy', 'wink', 'naughty', 'sleep_wake', 'celebrate', 'excited', 'yawn', 'surprised', 'deep_sleep'];
    if (noBlinkStates.includes(state)) return;
    
    const blinkInterval = setInterval(() => {
      setBlink(true);
      setTimeout(() => setBlink(false), 150);
    }, 3000 + Math.random() * 2000);

    return () => clearInterval(blinkInterval);
  }, [state]);

  const scale = 1.6;

  // Helper for pupil micro movements
  const noiseX = [0, 2, -1, 3, 0, -2, 1, 0].map(v => v * scale);
  const noiseY = [0, -1, 2, 0, -2, 1, -1, 0].map(v => v * scale);

  const leftEyeVariants = {
    idle: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: '16px', x: noiseX, y: noiseY, rotate: 0, scale: [0.8, 0.85, 0.9, 0.85, 0.8], opacity: 1 },
    breath: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: '16px', x: noiseX, y: noiseY, rotate: 0, scale: [0.8, 0.85, 0.9, 0.85, 0.8], opacity: 1 },
    happy: { height: 12 * scale, width: 36 * scale, borderRadius: '16px 16px 0 0', x: 0, y: [-10, -16, -10], rotate: 0, scale: 1, opacity: 1 },
    talking: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: '16px', x: 0, y: 0, rotate: 0, scale: 1, opacity: 1 },
    menu: { height: 20 * scale, width: 20 * scale, borderRadius: '10px', x: 0, y: -20, opacity: 0.3, rotate: 0, scale: 1 },
    dizzy: { height: 32 * scale, width: 32 * scale, borderRadius: '8px', x: 0, y: 0, rotate: 360, scale: 1, opacity: 1 },
    crying: { height: [8 * scale, 10 * scale, 8 * scale], width: 32 * scale, borderRadius: '4px', x: 0, y: [5, 8, 5], rotate: [15, 18, 15], scale: 1, opacity: 1 },
    naughty: { height: [8 * scale, 16 * scale, 8 * scale], width: [32 * scale, 30 * scale, 32 * scale], borderRadius: '16px', x: 0, y: [0, -5, 0], rotate: [0, 10, 0], scale: 1, opacity: 1 },
    wink: { height: [40 * scale, 2 * scale, 4 * scale, 40 * scale], width: [32 * scale, 36 * scale, 34 * scale, 32 * scale], borderRadius: '16px', x: 0, y: [0, 10, 8, 0], rotate: [0, -18, -15, 0], scale: 1, opacity: 1 },
    
    deep_sleep: { height: 10 * scale, width: 32 * scale, borderRadius: '16px', x: 0, y: 10, rotate: 0, scale: [0.6, 0.65, 0.6], opacity: [0.2, 0.4, 0.2] },
    light_rest: { height: 30 * scale, width: 32 * scale, borderRadius: '16px', x: 0, y: 5, rotate: 0, scale: [0.75, 0.8, 0.75], opacity: [0.3, 0.6, 0.3] },
    alert: { height: blink ? 4 : 42 * scale, width: 34 * scale, borderRadius: '16px', x: [0, 3, -3, 0], y: [0, 2, -2, 0], rotate: 0, scale: [0.85, 0.95, 0.85], opacity: [0.6, 1, 0.6] },

    sleep_wake: { height: [2, 4 * scale, 10 * scale, 20 * scale, 40 * scale], width: 32 * scale, borderRadius: '16px', x: 0, y: 0, rotate: 0, scale: 1, opacity: 1 },
    look_left: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: '16px', x: -20, y: 0, rotate: 0, scale: 1, opacity: 1 },
    look_right: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: '16px', x: 20, y: 0, rotate: 0, scale: 1, opacity: 1 },
    yawn: { height: [40 * scale, 10 * scale, 8 * scale, 10 * scale, 40 * scale], width: [32 * scale, 36 * scale, 36 * scale, 36 * scale, 32 * scale], borderRadius: '16px', x: 0, y: [0, 5, 5, 5, 0], rotate: 0, scale: 1, opacity: 1 },
    lost: { height: 20 * scale, width: 32 * scale, borderRadius: '16px 16px 4px 4px', x: 0, y: 10, rotate: -15, scale: 1, opacity: 1 },
    celebrate: { height: 40 * scale, width: 32 * scale, borderRadius: '16px', x: 0, y: 0, rotate: 0, scale: [1, 1.3, 0.9, 1.2, 1], opacity: 1 },
    excited: { height: 45 * scale, width: 36 * scale, borderRadius: '16px', x: [0, 5, -5, 3, -3, 0], y: [0, 3, -3, 2, -2, 0], rotate: 0, scale: [0.9, 1.0, 0.9], opacity: 1 },
    angry: { height: 16 * scale, width: 32 * scale, borderRadius: '4px 4px 16px 16px', x: 0, y: 15, rotate: 20, scale: 1, opacity: 1 },
    curious: { height: 36 * scale, width: 32 * scale, borderRadius: '16px', x: -10, y: -10, rotate: 10, scale: 1, opacity: 1 },
    thinking: { height: 32 * scale, width: 28 * scale, borderRadius: '16px', x: -15, y: -5, rotate: 0, scale: 1, opacity: 1 },
    surprised: { height: 45 * scale, width: 40 * scale, borderRadius: '20px', x: 0, y: -10, rotate: 0, scale: 1, opacity: 1 },
  };

  const rightEyeVariants = {
    ...leftEyeVariants,
    happy: { height: 12 * scale, width: 36 * scale, borderRadius: '16px 16px 0 0', x: 0, y: [-10, -16, -10], rotate: 0, scale: 1, opacity: 1 },
    crying: { height: [8 * scale, 10 * scale, 8 * scale], width: 32 * scale, borderRadius: '4px', x: 0, y: [5, 8, 5], rotate: [-15, -18, -15], scale: 1, opacity: 1 },
    naughty: { height: [32 * scale, 40 * scale, 32 * scale], width: [36 * scale, 32 * scale, 36 * scale], borderRadius: '16px', x: 0, y: [0, -2, 0], rotate: [0, -5, 0], scale: 1, opacity: 1 },
    wink: { height: [40 * scale, 50 * scale, 48 * scale, 40 * scale], width: [32 * scale, 38 * scale, 36 * scale, 32 * scale], borderRadius: '16px', x: 0, y: [0, -10, -8, 0], rotate: [0, 10, 8, 0], scale: 1, opacity: 1 },
    lost: { height: 20 * scale, width: 32 * scale, borderRadius: '16px 16px 4px 4px', x: 0, y: 10, rotate: 15, scale: 1, opacity: 1 },
    angry: { height: 16 * scale, width: 32 * scale, borderRadius: '4px 4px 16px 16px', x: 0, y: 15, rotate: -20, scale: 1, opacity: 1 },
    curious: { height: 40 * scale, width: 36 * scale, borderRadius: '16px', x: -10, y: -15, rotate: 10, scale: 1, opacity: 1 },
    thinking: { height: 24 * scale, width: 24 * scale, borderRadius: '16px', x: -15, y: 0, rotate: 0, scale: 1, opacity: 1 },
  };

  const mouthVariants = {
    idle: { width: 24 * scale, height: 4 * scale, borderRadius: '2px', x: 0, y: 0, rotate: 0, scale: [0.8, 0.85, 0.9, 0.85, 0.8], opacity: 1 },
    breath: { width: 24 * scale, height: 4 * scale, borderRadius: '2px', x: 0, y: 0, rotate: 0, scale: [0.8, 0.85, 0.9, 0.85, 0.8], opacity: 1 },
    happy: { width: [40 * scale, 46 * scale, 40 * scale], height: [20 * scale, 26 * scale, 20 * scale], borderRadius: '0 0 20px 20px', x: 0, y: [-5, -2, -5], rotate: 0, scale: 1, opacity: 1 },
    talking: { width: 20 * scale, height: 24 * scale, borderRadius: '12px', x: 0, y: 0, rotate: 0, scale: 1, opacity: 1 },
    menu: { width: 10 * scale, height: 4 * scale, borderRadius: '2px', x: 0, y: -20, opacity: 0.3, rotate: 0, scale: 1 },
    dizzy: { width: 16 * scale, height: 16 * scale, borderRadius: '8px', x: 0, y: 10, rotate: 0, scale: 1, opacity: 1 },
    crying: { width: [30 * scale, 34 * scale, 30 * scale], height: [12 * scale, 16 * scale, 12 * scale], borderRadius: '12px 12px 0 0', x: 0, y: [15, 18, 15], rotate: 0, scale: 1, opacity: 1 },
    naughty: { width: [36 * scale, 42 * scale, 36 * scale], height: [16 * scale, 24 * scale, 16 * scale], borderRadius: '0 0 16px 16px', x: 0, y: [-5, -2, -5], rotate: [-10, -5, -10], scale: 1, opacity: 1 },
    wink: { width: [24 * scale, 48 * scale, 46 * scale, 24 * scale], height: [4 * scale, 26 * scale, 24 * scale, 4 * scale], borderRadius: '0 0 24px 24px', x: 0, y: [0, -10, -8, 0], rotate: [0, 15, 12, 0], scale: 1, opacity: 1 },
    
    deep_sleep: { width: 16 * scale, height: 2 * scale, borderRadius: '2px', x: 0, y: 0, rotate: 0, scale: [0.6, 0.65, 0.6], opacity: [0.2, 0.4, 0.2] },
    light_rest: { width: 20 * scale, height: 3 * scale, borderRadius: '2px', x: 0, y: 0, rotate: 0, scale: [0.75, 0.8, 0.75], opacity: [0.3, 0.6, 0.3] },
    alert: { width: 24 * scale, height: 4 * scale, borderRadius: '2px', x: 0, y: 0, rotate: 0, scale: [0.85, 0.95, 0.85], opacity: [0.6, 1, 0.6] },

    sleep_wake: { width: 16 * scale, height: 2 * scale, borderRadius: '2px', x: 0, y: 0, rotate: 0, scale: 1, opacity: [0, 1, 1, 1, 1] },
    look_left: { width: 16 * scale, height: 4 * scale, borderRadius: '4px', x: -10, y: 0, rotate: 0, scale: 1, opacity: 1 },
    look_right: { width: 16 * scale, height: 4 * scale, borderRadius: '4px', x: 10, y: 0, rotate: 0, scale: 1, opacity: 1 },
    yawn: { width: [24 * scale, 32 * scale, 40 * scale, 32 * scale, 24 * scale], height: [4 * scale, 24 * scale, 36 * scale, 24 * scale, 4 * scale], borderRadius: '16px', x: 0, y: [0, 5, 8, 5, 0], rotate: 0, scale: 1, opacity: 1 },
    lost: { width: 16 * scale, height: 6 * scale, borderRadius: '8px 8px 0 0', x: 0, y: 15, rotate: 0, scale: 1, opacity: 1 },
    celebrate: { width: 46 * scale, height: 26 * scale, borderRadius: '0 0 20px 20px', x: 0, y: [-5, -10, -5, -10, -5], rotate: 0, scale: [1, 1.1, 1, 1.1, 1], opacity: 1 },
    excited: { width: 36 * scale, height: 24 * scale, borderRadius: '0 0 16px 16px', x: 0, y: -5, rotate: 0, scale: [0.9, 1.0, 0.9], opacity: 1 },
    angry: { width: 20 * scale, height: 8 * scale, borderRadius: '8px 8px 2px 2px', x: 0, y: 20, rotate: 0, scale: 1, opacity: 1 },
    curious: { width: 16 * scale, height: 16 * scale, borderRadius: '16px', x: -5, y: -5, rotate: 0, scale: 1, opacity: 1 },
    thinking: { width: 12 * scale, height: 12 * scale, borderRadius: '16px', x: -10, y: 5, rotate: -10, scale: 1, opacity: 1 },
    surprised: { width: 16 * scale, height: 20 * scale, borderRadius: '10px', x: 0, y: 15, rotate: 0, scale: 1, opacity: 1 },
  };

  const getThemeClasses = () => {
    if (theme === 'tech') return "bg-[#33C5FF] shadow-[0_0_20px_rgba(51,197,255,0.7)]";
    if (theme === 'dev') return "bg-[#22C55E] shadow-[0_0_15px_rgba(34,197,94,0.6)]";
    return "bg-[#FF9E7D] shadow-[0_0_15px_rgba(255,158,125,0.6)]";
  };
  const faceColorClass = getThemeClasses();

  const tearColorClass = theme === 'tech'
    ? "bg-[#33C5FF]/80 shadow-[0_0_12px_rgba(51,197,255,0.6)]"
    : "bg-blue-400/80 shadow-[0_0_12px_rgba(96,165,250,0.6)]";

  const getEyeTransition = () => {
    if (state === 'idle' || state === 'breath') return { x: { repeat: Infinity, duration: 8, ease: "linear" }, y: { repeat: Infinity, duration: 8, ease: "linear" }, scale: { repeat: Infinity, duration: 4, ease: [0.42, 0, 0.58, 1] } };
    if (state === 'deep_sleep') return { scale: { repeat: Infinity, duration: 8, ease: "easeInOut" }, opacity: { repeat: Infinity, duration: 8, ease: "easeInOut" } };
    if (state === 'light_rest') return { scale: { repeat: Infinity, duration: 6, ease: "easeInOut" }, opacity: { repeat: Infinity, duration: 6, ease: "easeInOut" } };
    if (state === 'alert') return { x: { repeat: Infinity, duration: 2.5 }, y: { repeat: Infinity, duration: 2.5 }, scale: { repeat: Infinity, duration: 2.5, ease: "easeInOut" }, opacity: { repeat: Infinity, duration: 2.5, ease: "easeInOut" } };
    if (state === 'excited') return { x: { repeat: Infinity, duration: 1.5 }, y: { repeat: Infinity, duration: 1.5 }, scale: { repeat: Infinity, duration: 1.5, ease: "easeInOut" } };

    if (state === 'dizzy') return { rotate: { repeat: Infinity, duration: 1, ease: "linear" }, type: 'spring', stiffness: 300, damping: 20 };
    if (state === 'happy') return { y: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, type: 'spring', stiffness: 300, damping: 20 };
    if (state === 'wink') return { duration: 1.5, repeat: 0, ease: "easeInOut" };
    if (state === 'crying') return { y: { repeat: Infinity, duration: 1.2, ease: "easeInOut" }, height: { repeat: Infinity, duration: 1.2, ease: "easeInOut" }, rotate: { repeat: Infinity, duration: 1.2, ease: "easeInOut" }, type: 'spring', stiffness: 300, damping: 20 };
    if (state === 'naughty') return { y: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, height: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, width: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, rotate: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, type: 'spring', stiffness: 300, damping: 20 };
    
    if (state === 'sleep_wake') return { duration: 3, times: [0, 0.2, 0.4, 0.7, 1], ease: "easeInOut" };
    if (state === 'look_around') return { x: { repeat: Infinity, duration: 4, times: [0, 0.1, 0.4, 0.5, 0.8, 1], ease: "easeInOut" }, type: 'spring', stiffness: 300, damping: 20 };
    if (state === 'yawn') return { repeat: Infinity, duration: 4, times: [0, 0.2, 0.5, 0.8, 1], ease: "easeInOut" };
    if (state === 'celebrate') return { scale: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, type: 'spring', stiffness: 400, damping: 15 };
    
    return { type: 'spring', stiffness: 300, damping: 20 };
  };

  const getMouthTransition = () => {
    if (state === 'idle' || state === 'breath') return { scale: { repeat: Infinity, duration: 4, ease: [0.42, 0, 0.58, 1] } };
    if (state === 'deep_sleep') return { scale: { repeat: Infinity, duration: 8, ease: "easeInOut" }, opacity: { repeat: Infinity, duration: 8, ease: "easeInOut" } };
    if (state === 'light_rest') return { scale: { repeat: Infinity, duration: 6, ease: "easeInOut" }, opacity: { repeat: Infinity, duration: 6, ease: "easeInOut" } };
    if (state === 'alert') return { scale: { repeat: Infinity, duration: 2.5, ease: "easeInOut" }, opacity: { repeat: Infinity, duration: 2.5, ease: "easeInOut" } };
    if (state === 'excited') return { scale: { repeat: Infinity, duration: 1.5, ease: "easeInOut" } };

    if (state === 'talking') return { height: { repeat: Infinity, repeatType: 'reverse', duration: 0.2 }, type: 'spring', stiffness: 300, damping: 20 };
    if (state === 'happy') return { width: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, height: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, y: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, type: 'spring', stiffness: 300, damping: 20 };
    if (state === 'wink') return { duration: 1.5, repeat: 0, ease: "easeInOut" };
    if (state === 'crying') return { width: { repeat: Infinity, duration: 1.2, ease: "easeInOut" }, height: { repeat: Infinity, duration: 1.2, ease: "easeInOut" }, y: { repeat: Infinity, duration: 1.2, ease: "easeInOut" }, type: 'spring', stiffness: 300, damping: 20 };
    if (state === 'naughty') return { width: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, height: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, y: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, rotate: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, type: 'spring', stiffness: 300, damping: 20 };
    
    if (state === 'sleep_wake') return { duration: 3, times: [0, 0.2, 0.4, 0.7, 1], ease: "easeInOut" };
    if (state === 'look_around') return { x: { repeat: Infinity, duration: 4, times: [0, 0.1, 0.4, 0.5, 0.8, 1], ease: "easeInOut" }, type: 'spring', stiffness: 300, damping: 20 };
    if (state === 'yawn') return { repeat: Infinity, duration: 4, times: [0, 0.2, 0.5, 0.8, 1], ease: "easeInOut" };
    if (state === 'celebrate') return { y: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, scale: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, type: 'spring', stiffness: 400, damping: 15 };

    return { type: 'spring', stiffness: 300, damping: 20 };
  };

  return (
    <div className="flex-1 flex flex-col items-center justify-center relative w-full">
      <div className="flex gap-14 mb-8">
        <motion.div
          className={faceColorClass}
          animate={leftEyeVariants[state as keyof typeof leftEyeVariants] || leftEyeVariants.idle}
          transition={getEyeTransition()}
        />
        <motion.div
          className={faceColorClass}
          animate={rightEyeVariants[state as keyof typeof rightEyeVariants] || rightEyeVariants.idle}
          transition={getEyeTransition()}
        />
      </div>
      
      <motion.div
        className={faceColorClass}
        animate={mouthVariants[state as keyof typeof mouthVariants] || mouthVariants.idle}
        transition={getMouthTransition()}
      />

      {state === 'crying' && (
        <div className="absolute top-1/2 left-0 w-full flex justify-center gap-20 pointer-events-none">
          <motion.div 
            className={`w-3 h-6 rounded-full ${tearColorClass}`}
            initial={{ y: -15, opacity: 0, scale: 0.8 }}
            animate={{ y: 35, opacity: [0, 1, 1, 0], scale: [0.8, 1.2, 1, 0.8] }}
            transition={{ repeat: Infinity, duration: 1.2, ease: "easeIn" }}
          />
          <motion.div 
            className={`w-3 h-6 rounded-full ${tearColorClass}`}
            initial={{ y: -15, opacity: 0, scale: 0.8 }}
            animate={{ y: 35, opacity: [0, 1, 1, 0], scale: [0.8, 1.2, 1, 0.8] }}
            transition={{ repeat: Infinity, duration: 1.2, ease: "easeIn", delay: 0.6 }}
          />
        </div>
      )}
    </div>
  );
};
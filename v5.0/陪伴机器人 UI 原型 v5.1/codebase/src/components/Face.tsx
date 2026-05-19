import React, { useEffect, useState } from 'react';
import { motion } from 'framer-motion';

export type FaceState = 'idle' | 'happy' | 'talking' | 'menu' | 'dizzy' | 'crying' | 'naughty' | 'wink' | 'breath' | 'morning' | 'look_around' | 'yawn' | 'sad' | 'celebrate' | 'excited' | 'angry' | 'curious' | 'deep_sleep' | 'light_rest' | 'alert';
export type Theme = 'tech' | 'child' | 'dev';

interface FaceProps {
  state: FaceState;
  theme: Theme;
}

export const Face: React.FC<FaceProps> = ({ state, theme }) => {
  const [blink, setBlink] = useState(false);

  useEffect(() => {
    // Only natural blinking in calm/idle states
    if (['menu', 'dizzy', 'crying', 'happy', 'wink', 'naughty', 'excited', 'yawn', 'celebrate'].includes(state)) return;
    
    const blinkInterval = setInterval(() => {
      setBlink(true);
      setTimeout(() => setBlink(false), 150);
    }, 3000 + Math.random() * 2000);

    return () => clearInterval(blinkInterval);
  }, [state]);

  const scale = 1.6;
  const radius = theme === 'dev' ? '4px' : '16px';

  const leftEyeVariants = {
    idle: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 0, rotate: 0, scale: [1, 1.1, 1], opacity: [0.5, 0.8, 0.5] },
    deep_sleep: { height: 16 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 5, rotate: 0, scale: [0.9, 0.95, 0.9], opacity: [0.2, 0.4, 0.2] },
    light_rest: { height: blink ? 4 : 32 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 0, rotate: 0, scale: [0.95, 1.05, 0.95], opacity: [0.4, 0.6, 0.4] },
    alert: { height: blink ? 4 : 44 * scale, width: 34 * scale, borderRadius: radius, x: 0, y: -2, rotate: 0, scale: [1.05, 1.15, 1.05], opacity: [0.7, 0.9, 0.7] },
    happy: { height: 12 * scale, width: 36 * scale, borderRadius: theme==='dev'?'4px':'16px 16px 0 0', x: 0, y: [-10, -16, -10], rotate: 0, scale: 1, opacity: 1 },
    talking: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 0, rotate: 0, scale: 1, opacity: 1 },
    menu: { height: 20 * scale, width: 20 * scale, borderRadius: '10px', x: 0, y: -20, opacity: 0.3, rotate: 0, scale: 1 },
    dizzy: { height: 32 * scale, width: 32 * scale, borderRadius: '8px', x: 0, y: 0, rotate: 360, scale: 1, opacity: 1 },
    crying: { height: [8 * scale, 10 * scale, 8 * scale], width: 32 * scale, borderRadius: '4px', x: 0, y: [5, 8, 5], rotate: [15, 18, 15], scale: 1, opacity: 1 },
    naughty: { height: [8 * scale, 16 * scale, 8 * scale], width: [32 * scale, 30 * scale, 32 * scale], borderRadius: radius, x: 0, y: [0, -5, 0], rotate: [0, 10, 0], scale: 1, opacity: 1 },
    wink: { height: [40 * scale, 2 * scale, 4 * scale, 40 * scale], width: [32 * scale, 36 * scale, 34 * scale, 32 * scale], borderRadius: radius, x: 0, y: [0, 10, 8, 0], rotate: [0, -18, -15, 0], scale: 1, opacity: 1 },
    breath: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 0, rotate: 0, scale: [0.95, 1.05, 0.95], opacity: [0.5, 1, 0.5] },
    morning: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 0, rotate: 0, scale: 1, opacity: 1 },
    look_around: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: radius, x: [-15 * scale, 15 * scale, -15 * scale], y: 0, rotate: 0, scale: 1, opacity: 1 },
    yawn: { height: 20 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 5 * scale, rotate: -5, scale: 1, opacity: 1 },
    sad: { height: 16 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 8 * scale, rotate: 15, scale: 1, opacity: 1 },
    celebrate: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: [-10, 0, -10], scale: [1, 1.2, 1], rotate: 0, opacity: 1 },
    excited: { height: blink ? 4 : 45 * scale, width: 36 * scale, borderRadius: radius, x: 0, y: 0, rotate: [0, 2, -2, 0], scale: [1.1, 1.25, 1.1], opacity: [0.8, 1, 0.8] },
    angry: { height: 20 * scale, width: 30 * scale, borderRadius: '8px', x: 0, y: 5 * scale, rotate: -20, scale: 1, opacity: 1 },
    curious: { height: 36 * scale, width: 32 * scale, borderRadius: radius, x: 20, y: -10, rotate: 15, scale: 1, opacity: 1 }
  };

  const rightEyeVariants = {
    idle: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 0, rotate: 0, scale: [1, 1.1, 1], opacity: [0.5, 0.8, 0.5] },
    deep_sleep: { height: 16 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 5, rotate: 0, scale: [0.9, 0.95, 0.9], opacity: [0.2, 0.4, 0.2] },
    light_rest: { height: blink ? 4 : 32 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 0, rotate: 0, scale: [0.95, 1.05, 0.95], opacity: [0.4, 0.6, 0.4] },
    alert: { height: blink ? 4 : 44 * scale, width: 34 * scale, borderRadius: radius, x: 0, y: -2, rotate: 0, scale: [1.05, 1.15, 1.05], opacity: [0.7, 0.9, 0.7] },
    happy: { height: 12 * scale, width: 36 * scale, borderRadius: theme==='dev'?'4px':'16px 16px 0 0', x: 0, y: [-10, -16, -10], rotate: 0, scale: 1, opacity: 1 },
    talking: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 0, rotate: 0, scale: 1, opacity: 1 },
    menu: { height: 20 * scale, width: 20 * scale, borderRadius: '10px', x: 0, y: -20, opacity: 0.3, rotate: 0, scale: 1 },
    dizzy: { height: 32 * scale, width: 32 * scale, borderRadius: '8px', x: 0, y: 0, rotate: 360, scale: 1, opacity: 1 },
    crying: { height: [8 * scale, 10 * scale, 8 * scale], width: 32 * scale, borderRadius: '4px', x: 0, y: [5, 8, 5], rotate: [-15, -18, -15], scale: 1, opacity: 1 },
    naughty: { height: [32 * scale, 40 * scale, 32 * scale], width: [36 * scale, 32 * scale, 36 * scale], borderRadius: radius, x: 0, y: [0, -2, 0], rotate: [0, -5, 0], scale: 1, opacity: 1 },
    wink: { height: [40 * scale, 50 * scale, 48 * scale, 40 * scale], width: [32 * scale, 38 * scale, 36 * scale, 32 * scale], borderRadius: radius, x: 0, y: [0, -10, -8, 0], rotate: [0, 10, 8, 0], scale: 1, opacity: 1 },
    breath: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 0, rotate: 0, scale: [0.95, 1.05, 0.95], opacity: [0.5, 1, 0.5] },
    morning: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 0, rotate: 0, scale: 1, opacity: 1 },
    look_around: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: radius, x: [-15 * scale, 15 * scale, -15 * scale], y: 0, rotate: 0, scale: 1, opacity: 1 },
    yawn: { height: 20 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 5 * scale, rotate: 5, scale: 1, opacity: 1 },
    sad: { height: 16 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: 8 * scale, rotate: -15, scale: 1, opacity: 1 },
    celebrate: { height: blink ? 4 : 40 * scale, width: 32 * scale, borderRadius: radius, x: 0, y: [-10, 0, -10], scale: [1, 1.2, 1], rotate: 0, opacity: 1 },
    excited: { height: blink ? 4 : 45 * scale, width: 36 * scale, borderRadius: radius, x: 0, y: 0, rotate: [0, -2, 2, 0], scale: [1.1, 1.25, 1.1], opacity: [0.8, 1, 0.8] },
    angry: { height: 20 * scale, width: 30 * scale, borderRadius: '8px', x: 0, y: 5 * scale, rotate: 20, scale: 1, opacity: 1 },
    curious: { height: 28 * scale, width: 32 * scale, borderRadius: radius, x: 20, y: -10, rotate: 15, scale: 1, opacity: 1 }
  };

  const mouthVariants = {
    idle: { width: 24 * scale, height: 4 * scale, borderRadius: '2px', x: 0, y: 0, rotate: 0, scale: [1, 1.1, 1], opacity: [0.5, 0.8, 0.5] },
    deep_sleep: { width: 16 * scale, height: 2 * scale, borderRadius: '1px', x: 0, y: 5, rotate: 0, scale: [0.9, 0.95, 0.9], opacity: [0.2, 0.4, 0.2] },
    light_rest: { width: 20 * scale, height: 4 * scale, borderRadius: '2px', x: 0, y: 0, rotate: 0, scale: [0.95, 1.05, 0.95], opacity: [0.4, 0.6, 0.4] },
    alert: { width: 20 * scale, height: 6 * scale, borderRadius: '3px', x: 0, y: 0, rotate: 0, scale: [1.05, 1.15, 1.05], opacity: [0.7, 0.9, 0.7] },
    happy: { width: [40 * scale, 46 * scale, 40 * scale], height: [20 * scale, 26 * scale, 20 * scale], borderRadius: theme==='dev'?'4px':'0 0 20px 20px', x: 0, y: [-5, -2, -5], rotate: 0, scale: 1, opacity: 1 },
    talking: { width: 20 * scale, height: 24 * scale, borderRadius: '12px', x: 0, y: 0, rotate: 0, scale: 1, opacity: 1 },
    menu: { width: 10 * scale, height: 4 * scale, borderRadius: '2px', x: 0, y: -20, opacity: 0.3, rotate: 0, scale: 1 },
    dizzy: { width: 16 * scale, height: 16 * scale, borderRadius: '8px', x: 0, y: 10, rotate: 0, scale: 1, opacity: 1 },
    crying: { width: [30 * scale, 34 * scale, 30 * scale], height: [12 * scale, 16 * scale, 12 * scale], borderRadius: '12px 12px 0 0', x: 0, y: [15, 18, 15], rotate: 0, scale: 1, opacity: 1 },
    naughty: { width: [36 * scale, 42 * scale, 36 * scale], height: [16 * scale, 24 * scale, 16 * scale], borderRadius: theme==='dev'?'4px':'0 0 16px 16px', x: 0, y: [-5, -2, -5], rotate: [-10, -5, -10], scale: 1, opacity: 1 },
    wink: { width: [24 * scale, 48 * scale, 46 * scale, 24 * scale], height: [4 * scale, 26 * scale, 24 * scale, 4 * scale], borderRadius: theme==='dev'?'4px':'0 0 24px 24px', x: 0, y: [0, -10, -8, 0], rotate: [0, 15, 12, 0], scale: 1, opacity: 1 },
    breath: { width: 24 * scale, height: 4 * scale, borderRadius: '2px', x: 0, y: 0, rotate: 0, scale: [0.95, 1.05, 0.95], opacity: [0.5, 1, 0.5] },
    morning: { width: 24 * scale, height: 4 * scale, borderRadius: '2px', x: 0, y: 0, rotate: 0, scale: 1, opacity: 1 },
    look_around: { width: 20 * scale, height: 4 * scale, borderRadius: '2px', x: [-5 * scale, 5 * scale, -5 * scale], y: 0, rotate: 0, scale: 1, opacity: 1 },
    yawn: { width: 24 * scale, height: 24 * scale, borderRadius: '12px', x: 0, y: 10 * scale, rotate: 0, scale: 1, opacity: 1 },
    sad: { width: 20 * scale, height: 6 * scale, borderRadius: '10px 10px 0 0', x: 0, y: 15 * scale, rotate: 0, scale: 1, opacity: 1 },
    celebrate: { width: 40 * scale, height: 20 * scale, borderRadius: theme==='dev'?'4px':'0 0 20px 20px', x: 0, y: [-5, 0, -5], scale: [1, 1.2, 1], rotate: 0, opacity: 1 },
    excited: { width: 40 * scale, height: 24 * scale, borderRadius: theme==='dev'?'4px':'0 0 20px 20px', x: 0, y: -5 * scale, rotate: 0, scale: [1.1, 1.25, 1.1], opacity: [0.8, 1, 0.8] },
    angry: { width: 16 * scale, height: 4 * scale, borderRadius: '2px', x: 0, y: 15 * scale, rotate: 0, scale: 1, opacity: 1 },
    curious: { width: 12 * scale, height: 12 * scale, borderRadius: '6px', x: 15, y: -5, rotate: 0, scale: 1, opacity: 1 }
  };

  const getTransition = (type: 'eye' | 'mouth') => {
    if (state === 'dizzy') return { rotate: { repeat: Infinity, duration: 1, ease: "linear" }, type: 'spring', stiffness: 300, damping: 20 };
    if (state === 'happy') return { y: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, width: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, height: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, type: 'spring', stiffness: 300, damping: 20 };
    if (state === 'wink') return { duration: 1.5, repeat: 0, ease: "easeInOut" };
    if (state === 'crying') return { y: { repeat: Infinity, duration: 1.2, ease: "easeInOut" }, height: { repeat: Infinity, duration: 1.2, ease: "easeInOut" }, width: { repeat: Infinity, duration: 1.2, ease: "easeInOut" }, rotate: { repeat: Infinity, duration: 1.2, ease: "easeInOut" }, type: 'spring', stiffness: 300, damping: 20 };
    if (state === 'naughty') return { y: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, height: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, width: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, rotate: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, type: 'spring', stiffness: 300, damping: 20 };
    if (state === 'breath') return { scale: { repeat: Infinity, duration: 4, ease: "easeInOut" }, opacity: { repeat: Infinity, duration: 4, ease: "easeInOut" }, type: 'spring', stiffness: 300, damping: 20 };
    if (state === 'look_around') return { x: { repeat: Infinity, duration: 3, ease: "easeInOut" }, type: 'spring', stiffness: 300, damping: 20 };
    if (state === 'celebrate') return { scale: { repeat: Infinity, duration: 1, ease: "easeInOut" }, y: { repeat: Infinity, duration: 1, ease: "easeInOut" }, type: 'spring', stiffness: 300, damping: 20 };
    
    // Dynamic breathing intervals mapped from state
    if (state === 'deep_sleep') return { scale: { repeat: Infinity, duration: 8, ease: [0.42, 0, 0.58, 1] }, opacity: { repeat: Infinity, duration: 8, ease: "easeInOut" } };
    if (state === 'light_rest') return { scale: { repeat: Infinity, duration: 6, ease: [0.42, 0, 0.58, 1] }, opacity: { repeat: Infinity, duration: 6, ease: "easeInOut" } };
    if (state === 'idle') return { scale: { repeat: Infinity, duration: 4, ease: [0.42, 0, 0.58, 1] }, opacity: { repeat: Infinity, duration: 4, ease: "easeInOut" } };
    if (state === 'alert') return { scale: { repeat: Infinity, duration: 2.5, ease: [0.42, 0, 0.58, 1] }, opacity: { repeat: Infinity, duration: 2.5, ease: "easeInOut" } };
    if (state === 'excited') return { scale: { repeat: Infinity, duration: 1.5, ease: [0.42, 0, 0.58, 1] }, opacity: { repeat: Infinity, duration: 1.5, ease: "easeInOut" }, rotate: { repeat: Infinity, duration: 0.3 } };
    if (state === 'morning') return { height: { duration: 3, ease: "easeOut" }, type: 'tween' };
    if (state === 'talking' && type === 'mouth') return { height: { repeat: Infinity, repeatType: 'reverse', duration: 0.2 }, type: 'spring', stiffness: 300, damping: 20 };
    
    return { type: 'spring', stiffness: 300, damping: 20 };
  };

  let baseColor = "bg-cyan-400 shadow-[0_0_20px_rgba(34,211,238,0.7)]";
  if (theme === 'child') baseColor = "bg-[#FF7F50] shadow-[0_0_15px_rgba(255,127,80,0.4)]";
  if (theme === 'dev') baseColor = "bg-green-500 shadow-[0_0_15px_rgba(34,197,94,0.6)]";

  if (state === 'sad') {
    baseColor = "bg-yellow-400 shadow-[0_0_25px_rgba(250,204,21,0.8)]";
  } else if (state === 'angry') {
    baseColor = "bg-red-500 shadow-[0_0_25px_rgba(239,68,68,0.8)]";
  } else if (state === 'excited') {
    baseColor = theme === 'tech' ? "bg-cyan-300 shadow-[0_0_30px_rgba(34,211,238,1)]" : 
                theme === 'dev' ? "bg-green-300 shadow-[0_0_30px_rgba(34,197,94,1)]" : 
                "bg-orange-300 shadow-[0_0_30px_rgba(255,127,80,1)]";
  }

  const faceColorClass = baseColor;
  
  const tearColorClass = theme === 'tech'
    ? "bg-cyan-400/80 shadow-[0_0_12px_rgba(34,211,238,0.6)]"
    : theme === 'dev'
    ? "bg-green-400/80 shadow-[0_0_12px_rgba(34,197,94,0.6)]"
    : "bg-blue-400/80 shadow-[0_0_12px_rgba(96,165,250,0.6)]";

  const pupilVariants: Record<string, any> = {
    deep_sleep: { x: 0, y: 0, scale: 0.6, transition: { scale: { duration: 4 } } },
    light_rest: { x: [0, 1, -1, 0], y: [0, -1, 1, 0], scale: 0.75, transition: { x: { duration: 10, repeat: Infinity }, y: { duration: 10, repeat: Infinity } } },
    idle: { x: [0, 5, -3, 4, -5, 2, -1, 0], y: [0, -3, 2, 0, -2, 3, -1, 0], scale: [0.8, 0.85, 0.9, 0.85, 0.8], transition: { x: { duration: 8, repeat: Infinity, ease: "linear" }, y: { duration: 8, repeat: Infinity, ease: "linear" }, scale: { duration: 4, repeat: Infinity, ease: [0.42, 0, 0.58, 1] } } },
    alert: { x: [0, 4, -4, 2, -2, 0], y: [0, 2, -2, 1, -1, 0], scale: 0.85, transition: { x: { duration: 2, repeat: Infinity }, y: { duration: 2, repeat: Infinity } } },
    excited: { x: [0, 1, -1, 2, -2, 1, -1, 0], y: [0, -1, 1, -2, 2, -1, 1, 0], scale: 0.9, transition: { x: { duration: 0.2, repeat: Infinity }, y: { duration: 0.2, repeat: Infinity } } },
    curious: { x: [0, 3, 3, 0], y: [0, -2, -2, 0], scale: 0.85, transition: { duration: 2, repeat: Infinity } },
    yawn: { x: 0, y: 2, scale: 0.7 },
    look_around: { x: [0, 6, -6, 0], y: 0, scale: 0.85, transition: { x: { duration: 3, repeat: Infinity, ease: "easeInOut" } } },
    dizzy: { x: [0, 2, 0, -2, 0], y: [2, 0, -2, 0, 2], scale: 0.8, transition: { x: { duration: 0.5, repeat: Infinity }, y: { duration: 0.5, repeat: Infinity } } }
  };
  const getPupilVariant = (s: string) => pupilVariants[s] || { x: 0, y: 0, scale: 1 };

  return (
    <div className="flex-1 flex flex-col items-center justify-center relative w-full">
      <div className="flex gap-14 mb-8">
        <motion.div
          animate={leftEyeVariants[state]}
          transition={getTransition('eye')}
          className="relative"
        >
          <motion.div className={`w-full h-full ${faceColorClass}`} animate={getPupilVariant(state)} style={{ borderRadius: 'inherit' }} />
        </motion.div>
        <motion.div
          animate={rightEyeVariants[state]}
          transition={getTransition('eye')}
          className="relative"
        >
          <motion.div className={`w-full h-full ${faceColorClass}`} animate={getPupilVariant(state)} style={{ borderRadius: 'inherit' }} />
        </motion.div>
      </div>
      
      <motion.div
        className={faceColorClass}
        animate={mouthVariants[state]}
        transition={getTransition('mouth')}
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
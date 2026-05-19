import React, { useState, useEffect } from 'react';
import { useSearchParams, useNavigate } from 'react-router-dom';
import { Face, FaceState } from './components/Face';
import { BottomBar } from './components/BottomBar';
import { MenuOverlay } from './components/MenuOverlay';
import { WifiSetup } from './components/WifiSetup';
import { BootAnimation } from './components/BootAnimation';
import { RadialMenu } from './components/RadialMenu';
import { EnvSimulator, EnvState } from './components/EnvSimulator';
import { AnimatePresence, motion } from 'framer-motion';
import { Sunrise } from 'lucide-react';
import { MotionController, MotionData } from './components/MotionController';
import { playHapticFeedback } from './lib/hapticFeedback';
import { playAudioFeedback } from './lib/audioFeedback';
import { useMemory } from './hooks/useMemory';

const Particles = () => {
  return (
    <div className="absolute inset-0 pointer-events-none z-10 overflow-hidden">
      {[...Array(30)].map((_, i) => (
        <motion.div
          key={i}
          className="absolute w-2 h-2 rounded-full"
          style={{
            background: ['#FFD700', '#FF6347', '#00FFFF', '#32CD32', '#FF69B4'][Math.floor(Math.random() * 5)],
            left: '50%',
            top: '50%',
          }}
          initial={{ x: 0, y: 0, opacity: 1, scale: 0 }}
          animate={{
            x: (Math.random() - 0.5) * 400,
            y: (Math.random() - 0.5) * 400,
            opacity: [1, 1, 0],
            scale: [0, Math.random() + 0.8, 0],
          }}
          transition={{ duration: 1.5 + Math.random(), repeat: Infinity, ease: "easeOut" }}
        />
      ))}
    </div>
  );
};

const MorningOverlay = () => {
  return (
    <motion.div initial={{opacity:0, y:-10}} animate={{opacity:1, y:0}} className="absolute top-2 w-full flex justify-center items-center gap-2 z-10">
      <Sunrise className="text-yellow-400" size={18} />
      <span className="text-white font-mono text-sm tracking-widest drop-shadow-md">08:00</span>
      <span className="text-white/80 text-xs ml-1 font-medium drop-shadow-md">24°C</span>
    </motion.div>
  );
};

const DialogBubble = ({ type, theme, onClose }: { type: string, theme: string, onClose: () => void }) => {
  const content = {
    morning: { text: "早上好呀！今天也是充满能量的一天。", showBtns: false },
    suggest: { text: "要不要试试调皮表情？", showBtns: true },
    sleep: { text: "已经很晚了，该休息了哦~", showBtns: false }
  }[type as 'morning' | 'suggest' | 'sleep'];

  if (!content) return null;

  const btnClass = theme === 'tech' ? 'bg-cyan-500 text-black' : theme === 'dev' ? 'bg-green-500 text-black' : 'bg-orange-500 text-white';

  return (
    <motion.div 
      initial={{ opacity: 0, y: 20, scale: 0.9 }}
      animate={{ opacity: 1, y: 0, scale: 1 }}
      exit={{ opacity: 0, y: 20, scale: 0.9 }}
      className={`absolute bottom-[20px] left-4 right-4 p-3 rounded-2xl z-20 shadow-xl border ${theme === 'tech' ? 'bg-cyan-950/90 border-cyan-800 text-cyan-100' : theme === 'dev' ? 'bg-green-950/90 border-green-800 text-green-400' : 'bg-white/95 border-orange-200 text-orange-600'}`}
    >
      <p className="text-[11px] font-medium leading-relaxed">{content.text}</p>
      {content.showBtns && (
        <div className="flex gap-2 mt-2 justify-end">
          <button onClick={onClose} className="px-3 py-1 rounded-full text-[10px] bg-black/10 hover:bg-black/20 transition-colors dark:bg-white/10 dark:hover:bg-white/20">不了</button>
          <button onClick={onClose} className={`px-3 py-1 rounded-full text-[10px] font-bold ${btnClass} opacity-90 hover:opacity-100 transition-opacity`}>试试看</button>
        </div>
      )}
    </motion.div>
  );
}

export const RobotUI: React.FC = () => {
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();
  const stateParam = searchParams.get('state') || 'boot';
  const barParam = searchParams.get('bar');
  const themeParam = (searchParams.get('theme') as 'tech' | 'child' | 'dev') || 'tech';
  const dialogParam = searchParams.get('dialog');
  const envParam = searchParams.get('env_panel') === '1';
  const menuParam = searchParams.get('menu');
  
  const currentState = stateParam;
  const isBottomBarVisible = barParam === '1';
  const isEnvPanelVisible = envParam;
  const isRadialMenuVisible = menuParam === 'radial';
  const theme = themeParam;

  const [randomFace, setRandomFace] = useState<FaceState>('idle');
  const [env, setEnv] = useState<EnvState>({ lux: 300, db: 40, hour: 14 });
  const [motionData, setMotionData] = useState<MotionData | null>(null);
  const { memory, recordInteraction } = useMemory();
  const [interactionTimes, setInteractionTimes] = useState<number[]>([]);

  const handleInteraction = () => {
    const now = Date.now();
    const newTimes = [...interactionTimes.filter(t => now - t < 5000), now];
    setInteractionTimes(newTimes);
    recordInteraction('other');

    if (newTimes.length >= 5 && currentState !== 'excited') {
      const params = new URLSearchParams(searchParams);
      params.set('state', 'excited');
      navigate(`/?${params.toString()}`);
      setInteractionTimes([]);
    }
  };

  const checkMorningGreeting = () => {
    const now = new Date();
    const hour = now.getHours();
    const todayStr = now.toDateString();
    
    if (hour >= 6 && hour <= 10) {
      if (memory.lastActiveDate !== todayStr) {
        const params = new URLSearchParams(searchParams);
        params.set('state', 'morning');
        params.set('dialog', 'morning');
        navigate(`/?${params.toString()}`);
        recordInteraction('other');
      }
    }
  };

  const handleMotionAction = (action: 'curious' | 'yawn' | 'look_around' | 'dizzy' | 'wink') => {
    const params = new URLSearchParams(searchParams);
    params.set('state', action);
    navigate(`/?${params.toString()}`);
    handleInteraction();
    checkMorningGreeting();
  };

  useEffect(() => {
    // Initial check for morning greeting when system becomes idle
    if (currentState === 'idle') {
      const now = new Date();
      const hour = now.getHours();
      const todayStr = now.toDateString();
      if (hour >= 6 && hour <= 10 && memory.lastActiveDate !== todayStr && !dialogParam) {
        checkMorningGreeting();
      }
    }

    if (currentState === 'random') {
      const faces: FaceState[] = ['idle', 'happy', 'talking', 'dizzy', 'crying', 'naughty', 'wink', 'breath', 'look_around', 'yawn', 'excited', 'curious'];
      setRandomFace(faces[Math.floor(Math.random() * faces.length)]);
      
      const interval = setInterval(() => {
        setRandomFace(prev => {
          const available = faces.filter(f => f !== prev);
          return available[Math.floor(Math.random() * available.length)];
        });
      }, 3000);
      return () => clearInterval(interval);
    }
  }, [currentState]);

  const handleDoubleClick = () => {
    if (currentState === 'boot') return;
    playHapticFeedback('doubleClick');
    playAudioFeedback('doubleClick');
    handleInteraction();
    checkMorningGreeting();
    
    const newParams = new URLSearchParams(searchParams);
    if (isRadialMenuVisible) {
      newParams.delete('menu');
    } else {
      newParams.set('menu', 'radial');
    }
    navigate(`/?${newParams.toString()}`);
  };

  const handleRadialMenuSelect = (action: string) => {
    playHapticFeedback('menuSelect');
    playAudioFeedback('menuSelect');
    handleInteraction();
    
    const newParams = new URLSearchParams(searchParams);
    newParams.delete('menu');
    
    if (action === 'expressions') {
      newParams.set('state', 'happy');
    } else if (action === 'dialogue') {
      newParams.set('state', 'talking');
    } else if (action === 'settings') {
      newParams.set('state', 'menu');
    } else if (action === 'theme') {
      newParams.set('theme', theme === 'tech' ? 'child' : theme === 'child' ? 'dev' : 'tech');
    } else if (action === 'extensions') {
      newParams.set('bar', '1');
    } else if (action === 'random') {
      newParams.set('state', 'random');
    }
    navigate(`/?${newParams.toString()}`);
  };

  const dismissDialog = () => {
    const newParams = new URLSearchParams(searchParams);
    newParams.delete('dialog');
    navigate(`/?${newParams.toString()}`);
  };

  const isWifiState = currentState.startsWith('wifi_');
  
  let faceState: FaceState = 'idle';
  if (isWifiState || currentState === 'menu') {
    faceState = 'menu';
  } else if (currentState === 'random') {
    faceState = randomFace;
  } else if (currentState !== 'boot') {
    faceState = currentState as FaceState;
  }

  // Calculate environmental breathing overrides when idle
  if (faceState === 'idle') {
    if (env.lux < 10 || env.hour < 6 || env.hour > 22) {
      faceState = 'deep_sleep';
    } else if ((env.hour >= 18 && env.hour <= 22) && env.lux < 50) {
      faceState = 'light_rest';
    } else if (env.db > 60 || env.lux > 800) {
      faceState = 'alert';
    }
  }

  const containerBg = theme === 'child' ? 'bg-[#FFF9E6]' : 'bg-black';

  return (
    <div 
      className={`w-[320px] h-[240px] flex flex-col overflow-hidden relative select-none transition-colors duration-500 ${containerBg}`}
      onDoubleClick={handleDoubleClick}
    >
      <AnimatePresence mode="wait">
        {currentState === 'boot' ? (
          <motion.div key="boot" className="absolute inset-0 z-50">
            <BootAnimation theme={theme} />
          </motion.div>
        ) : (
          <motion.div 
            key="ui"
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            transition={{ duration: 0.5 }}
            className="flex-1 relative flex flex-col"
          >
            {currentState === 'morning' && <MorningOverlay />}
            {currentState === 'celebrate' && <Particles />}

            <MotionController onMotionAction={handleMotionAction} onDataUpdate={setMotionData} />

            <Face state={faceState} theme={theme} />
            
            <AnimatePresence>
              {dialogParam && (
                <DialogBubble type={dialogParam} theme={theme} onClose={dismissDialog} />
              )}
            </AnimatePresence>

            <AnimatePresence>
              {isRadialMenuVisible && (
                <RadialMenu 
                  theme={theme} 
                  onSelect={handleRadialMenuSelect} 
                  onClose={() => {
                    const params = new URLSearchParams(searchParams);
                    params.delete('menu');
                    navigate(`/?${params.toString()}`);
                  }} 
                />
              )}
              {isEnvPanelVisible && (
                <EnvSimulator 
                  env={env} 
                  onEnvChange={setEnv} 
                  theme={theme}
                  motionData={motionData}
                  onClose={() => {
                    const params = new URLSearchParams(searchParams);
                    params.delete('env_panel');
                    navigate(`/?${params.toString()}`);
                  }}
                />
              )}
              {currentState === 'menu' && <MenuOverlay theme={theme} />}
              {isWifiState && <WifiSetup state={currentState} theme={theme} />}
            </AnimatePresence>

            <AnimatePresence>
              {isBottomBarVisible && (
                <motion.div
                  initial={{ y: '100%' }}
                  animate={{ y: 0 }}
                  exit={{ y: '100%' }}
                  transition={{ type: 'spring', damping: 25, stiffness: 200 }}
                  className="absolute bottom-0 left-0 w-full z-30"
                >
                  <BottomBar currentState={currentState} theme={theme} />
                </motion.div>
              )}
            </AnimatePresence>
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
};
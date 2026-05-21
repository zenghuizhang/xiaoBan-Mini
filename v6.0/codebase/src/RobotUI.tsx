import React, { useState, useEffect } from 'react';
import { useSearchParams, useNavigate } from 'react-router-dom';
import { Face, FaceState } from './components/Face';
import { MenuOverlay } from './components/MenuOverlay';
import { ScenarioOverlay } from './components/ScenarioOverlay';
import { WifiSetup } from './components/WifiSetup';
import { BootAnimation } from './components/BootAnimation';
import { DialogBubble } from './components/DialogBubble';
import { AnimatePresence, motion } from 'framer-motion';
import { playTone, vibrate } from './lib/feedback';

export const RobotUI: React.FC = () => {
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();
  const stateParam = searchParams.get('state') || 'boot';
  const themeParam = (searchParams.get('theme') as 'tech' | 'child' | 'dev') || 'tech';
  
  const currentState = stateParam;
  const theme = themeParam;

  const [randomFace, setRandomFace] = useState<FaceState>('idle');

  useEffect(() => {
    if (currentState === 'random') {
      const faces: FaceState[] = ['idle', 'happy', 'talking', 'dizzy', 'crying', 'naughty', 'wink', 'breath', 'thinking', 'surprised'];
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
    playTone(800, 100, 0.1);
    vibrate([50, 30, 50]);
    if (currentState === 'menu') {
      navigate(`/?state=idle&theme=${theme}`);
    } else {
      navigate(`/?state=menu&theme=${theme}`);
    }
  };

  const isWifiState = currentState.startsWith('wifi_');
  
  let faceState: FaceState = 'idle';
  let bubbleConfig = null;
  let customRgbStrip = null;

  if (isWifiState) {
    faceState = 'menu';
  } else if (currentState === 'random') {
    faceState = randomFace;
  } else if (currentState === 'scenario_sim') {
    faceState = 'menu';
  } else if (currentState === 'morning') {
    faceState = 'sleep_wake';
    bubbleConfig = { text: "早上好呀！今天也是充满能量的一天。", type: 'text' as const };
  } else if (currentState === 'lonely_3') {
    faceState = 'lost';
    customRgbStrip = { color: 'bg-yellow-400 shadow-[0_0_20px_rgba(250,204,21,0.8)]', duration: 4, opacity: [0.2, 0.8, 0.2] };
    bubbleConfig = { text: "好无聊哦，陪我玩一会吧...", type: 'text' as const };
  } else if (currentState === 'reward') {
    faceState = 'celebrate';
    customRgbStrip = { color: theme === 'tech' ? 'bg-[#33C5FF] shadow-[0_0_20px_rgba(51,197,255,0.8)]' : theme === 'dev' ? 'bg-[#22C55E] shadow-[0_0_20px_rgba(34,197,94,0.8)]' : 'bg-[#FF9E7D] shadow-[0_0_20px_rgba(255,158,125,0.8)]', duration: 0.5, opacity: [0.5, 1, 0.5] };
  } else if (currentState === 'angry') {
    faceState = 'angry';
    customRgbStrip = { color: 'bg-red-500 shadow-[0_0_30px_rgba(239,68,68,0.9)]', duration: 1.2, opacity: [0.3, 1, 0.3] };
  } else if (currentState === 'suggest') {
    faceState = 'idle';
    bubbleConfig = { 
      text: "要不要试试调皮表情？", 
      type: 'suggest' as const,
      onAction: (action: string) => {
        if (action === 'yes' || action === 'timeout') {
          navigate(`/?state=naughty&theme=${theme}`);
        }
      }
    };
  } else if (currentState !== 'boot') {
    faceState = currentState as FaceState;
  }

  // Modified Sine Curve logic for RGB strip
  const baseColor = theme === 'tech' ? 'bg-[#33C5FF]' : theme === 'dev' ? 'bg-[#22C55E]' : 'bg-[#FF9E7D]';
  const shadowColor = theme === 'tech' ? 'rgba(51,197,255,0.6)' : theme === 'dev' ? 'rgba(34,197,94,0.6)' : 'rgba(255,158,125,0.6)';
  const defaultRgbClass = `${baseColor} shadow-[0_0_20px_${shadowColor}]`;

  let rgbAnimDuration = 4;
  let rgbOpacity = [0.3, 0.6, 0.3];
  
  if (faceState === 'deep_sleep') {
    rgbAnimDuration = 8;
    rgbOpacity = [0.1, 0.2, 0.1];
  } else if (faceState === 'light_rest') {
    rgbAnimDuration = 6;
    rgbOpacity = [0.2, 0.35, 0.2];
  } else if (faceState === 'alert') {
    rgbAnimDuration = 2.5;
    rgbOpacity = [0.5, 0.8, 0.5];
  } else if (faceState === 'excited') {
    rgbAnimDuration = 1.5;
    rgbOpacity = [0.6, 0.9, 0.6];
  }

  const activeRgbColor = customRgbStrip ? customRgbStrip.color : defaultRgbClass;
  const activeRgbDuration = customRgbStrip ? customRgbStrip.duration : rgbAnimDuration;
  const activeRgbOpacity = customRgbStrip ? customRgbStrip.opacity : rgbOpacity;

  const containerBg = theme === 'tech' || theme === 'dev' ? 'bg-black' : 'bg-[#FFF9E6]';

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
            <Face state={faceState} theme={theme} />
            
            {bubbleConfig && (
              <DialogBubble 
                key={`bubble-${currentState}`}
                text={bubbleConfig.text} 
                type={bubbleConfig.type} 
                theme={theme}
                onAction={bubbleConfig.type === 'suggest' ? bubbleConfig.onAction : undefined}
              />
            )}

            <motion.div
              key="rgb-strip"
              animate={{ opacity: activeRgbOpacity }}
              transition={{ 
                repeat: Infinity, 
                duration: activeRgbDuration, 
                times: [0, 0.5, 1], // Simplified modified sine wave easing
                ease: ["easeIn", "easeOut"] 
              }}
              className={`absolute bottom-0 left-0 w-full h-1.5 z-10 ${activeRgbColor}`}
            />
            
            <AnimatePresence>
              {currentState === 'menu' && <MenuOverlay theme={theme} />}
              {currentState === 'scenario_sim' && <ScenarioOverlay theme={theme} />}
              {isWifiState && <WifiSetup state={currentState} theme={theme} />}
            </AnimatePresence>
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
};
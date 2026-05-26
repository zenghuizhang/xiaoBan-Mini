// v6.3.0 round2 — applied 9 fixes per PM review
import React, { useState, useEffect } from 'react';
import { useSearchParams, useNavigate } from 'react-router-dom';
import { Face, FaceState } from './components/Face';
import { MenuOverlay } from './components/MenuOverlay';
import { SettingsOverlay } from './components/SettingsOverlay';
import { WifiSetup } from './components/WifiSetup';
import { ThemePicker } from './components/ThemePicker';
import { Skills } from './components/Skills';
import { BootAnimation } from './components/BootAnimation';
import { DialogBubble } from './components/DialogBubble';
import { StatusBar, StatusBarMode } from './components/StatusBar';
import { ModelPicker } from './components/ModelPicker';
import { PersonaGrid } from './components/PersonaGrid';
import { MemoryBrowser } from './components/MemoryBrowser';
import { Console } from './components/Console';
import { AnimatePresence, motion } from 'framer-motion';
import { playTone, vibrate } from './lib/feedback';
import { TOKENS_TABLE, ThemeName } from './theme/tokens';

export const RobotUI: React.FC = () => {
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();
  const stateParam = searchParams.get('state') || 'boot';
  const themeParam = (searchParams.get('theme') as ThemeName) || 'tech';
  
  const currentState = stateParam;
  const theme = themeParam;
  const t = TOKENS_TABLE[theme] || TOKENS_TABLE.tech;

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
  const isSettingsState = currentState === 'settings';
  
  const subPages = [
    'model', 'persona', 'memory', 'memory_purge', 'settings', 'theme_picker',
    'console_system', 'console_ai', 'console_display', 'console_service', 'skills'
  ];
  const isSubPage = subPages.includes(currentState) || isWifiState;

  let sbMode: StatusBarMode = 'idle';
  if (isSubPage) sbMode = 'always';

  let faceState: FaceState = 'idle';
  let bubbleConfig: any = null;
  let customRgbStrip: any = null;

  if (isSubPage) {
    faceState = 'menu';
  } else if (currentState === 'random') {
    faceState = randomFace;
  } else if (currentState === 'morning') {
    faceState = 'sleep_wake';
    bubbleConfig = { text: "早上好呀！今天也是充满能量的一天。", type: 'text' as const };
  } else if (currentState === 'lonely_3') {
    faceState = 'lost';
    customRgbStrip = { color: '#FACC15', shadow: 'rgba(250,204,21,0.8)', duration: 4, opacity: [0.2, 0.8, 0.2] };
    bubbleConfig = { text: "好无聊哦，陪我玩一会吧...", type: 'text' as const };
  } else if (currentState === 'reward') {
    faceState = 'celebrate';
    customRgbStrip = { color: t.accent_hi, shadow: `${t.accent_hi}CC`, duration: 0.5, opacity: [0.5, 1, 0.5] };
  } else if (currentState === 'angry') {
    faceState = 'angry';
    customRgbStrip = { color: t.danger, shadow: `${t.danger}E6`, duration: 1.2, opacity: [0.3, 1, 0.3] };
  } else if (currentState === 'ota') {
    faceState = 'deep_sleep';
    bubbleConfig = { text: "OTA 系统更新中...", type: 'text' as const };
    customRgbStrip = { color: '#A855F7', shadow: 'rgba(168,85,247,0.8)', duration: 1.5, opacity: [0.4, 0.8, 0.4] };
  } else if (currentState === 'error') {
    faceState = 'dizzy';
    bubbleConfig = { text: "系统发生异常错误！", type: 'text' as const };
    customRgbStrip = { color: t.danger, shadow: `${t.danger}E6`, duration: 0.5, opacity: [0.5, 1, 0.5] };
  } else if (currentState === 'voice_wake') {
    faceState = 'excited';
    bubbleConfig = { text: "我在听...", type: 'text' as const };
    customRgbStrip = { color: t.accent_hi, shadow: `${t.accent_hi}CC`, duration: 0.5, opacity: [0.8, 1, 0.8] };
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

  const activeRgbColorHex = customRgbStrip ? customRgbStrip.color : t.accent_hi;
  const activeRgbShadow = customRgbStrip ? customRgbStrip.shadow : `${t.accent_hi}99`;
  const activeRgbDuration = customRgbStrip ? customRgbStrip.duration : rgbAnimDuration;
  const activeRgbOpacity = customRgbStrip ? customRgbStrip.opacity : rgbOpacity;

  return (
    <div 
      className="w-[320px] h-[240px] flex flex-col overflow-hidden relative select-none transition-colors duration-500"
      style={{ backgroundColor: t.bg }}
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
            <StatusBar mode={sbMode} theme={theme} />

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
                times: [0, 0.5, 1],
                ease: ["easeIn", "easeOut"] 
              }}
              className="absolute bottom-0 left-0 w-full h-1.5 z-10"
              style={{ backgroundColor: activeRgbColorHex, boxShadow: `0 0 20px ${activeRgbShadow}` }}
            />
            
            <AnimatePresence>
              {currentState === 'menu' && <MenuOverlay theme={theme} />}
              {isSettingsState && <SettingsOverlay theme={theme} />}
              {isWifiState && <WifiSetup state={currentState} theme={theme} />}
              {currentState === 'model' && <ModelPicker theme={theme} />}
              {currentState === 'persona' && <PersonaGrid theme={theme} />}
              {(currentState === 'memory' || currentState === 'memory_purge') && <MemoryBrowser theme={theme} purgeModal={currentState === 'memory_purge'} />}
              {currentState === 'theme_picker' && <ThemePicker theme={theme} />}
              {currentState === 'skills' && <Skills theme={theme} />}
              {currentState.startsWith('console_') && <Console theme={theme} activeTab={currentState.replace('console_', '') as any} />}
            </AnimatePresence>
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
};
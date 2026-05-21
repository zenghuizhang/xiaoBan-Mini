import React, { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { Smile, MessageCircle, Settings, Palette, Puzzle, Shuffle } from 'lucide-react';
import { useNavigate } from 'react-router-dom';
import { playTone, vibrate } from '../lib/feedback';

interface MenuOverlayProps {
  theme: 'tech' | 'child' | 'dev';
}

export const MenuOverlay: React.FC<MenuOverlayProps> = ({ theme }) => {
  const navigate = useNavigate();
  const [hoveredItem, setHoveredItem] = useState<string | null>(null);

  useEffect(() => {
    playTone(600, 80, 0.1);
  }, []);

  const navigateTo = (newState: string, newTheme: string = theme) => {
    playTone(1200, 80, 0.2);
    vibrate([30]);
    navigate(`/?state=${newState}&theme=${newTheme}`);
  };

  const sectors = [
    { id: 'expressions', label: '表情', icon: Smile, angle: -90, action: () => navigateTo('random') },
    { id: 'dialogue', label: '对话', icon: MessageCircle, angle: -30, action: () => navigateTo('talking') },
    { id: 'settings', label: '设置', icon: Settings, angle: 30, action: () => navigateTo('wifi_ap') },
    { id: 'theme', label: '主题', icon: Palette, angle: 90, action: () => {
      const themes = ['tech', 'child', 'dev'];
      const nextTheme = themes[(themes.indexOf(theme) + 1) % themes.length];
      navigateTo('menu', nextTheme);
    }},
    { id: 'extensions', label: '扩展', icon: Puzzle, angle: 150, action: () => navigateTo('scenario_sim') },
    { id: 'random', label: '随机', icon: Shuffle, angle: 210, action: () => navigateTo('random') }
  ];

  const overlayClass = theme === 'tech' ? "bg-black/80" : theme === 'dev' ? "bg-black/80" : "bg-[#FFF9E6]/90";
  const btnClass = theme === 'tech'
    ? "bg-cyan-950/80 text-cyan-400 border-cyan-800/50 shadow-[0_0_15px_rgba(34,211,238,0.2)]"
    : theme === 'dev'
    ? "bg-green-950/80 text-green-400 border-green-800/50 shadow-[0_0_15px_rgba(34,197,94,0.2)]"
    : "bg-white/95 text-orange-500 border-orange-200 shadow-[0_5px_15px_rgba(255,127,80,0.15)]";

  const activeBtnClass = theme === 'tech'
    ? "bg-cyan-900 text-cyan-200 border-cyan-400 shadow-[0_0_20px_rgba(34,211,238,0.6)]"
    : theme === 'dev'
    ? "bg-green-900 text-green-200 border-green-400 shadow-[0_0_20px_rgba(34,197,94,0.6)]"
    : "bg-orange-100 text-orange-600 border-orange-400 shadow-[0_5px_20px_rgba(255,127,80,0.4)]";

  const textLabelClass = theme === 'tech' 
    ? 'text-cyan-300 drop-shadow-[0_0_8px_rgba(34,211,238,0.8)]' 
    : theme === 'dev'
    ? 'text-green-300 drop-shadow-[0_0_8px_rgba(34,197,94,0.8)]'
    : 'text-orange-500 drop-shadow-[0_0_8px_rgba(255,127,80,0.8)]';

  const radius = 70;

  return (
    <motion.div 
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      exit={{ opacity: 0 }}
      transition={{ duration: 0.2 }}
      className={`absolute inset-0 backdrop-blur-sm z-20 overflow-hidden ${overlayClass}`}
      onClick={() => navigateTo('idle')}
    >
      <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-full h-full pointer-events-none">
        {sectors.map((item, index) => {
          const Icon = item.icon;
          const rad = (item.angle * Math.PI) / 180;
          const x = Math.cos(rad) * radius;
          const y = Math.sin(rad) * radius;
          const isHovered = hoveredItem === item.id;

          return (
            <motion.button
              key={item.id}
              onClick={(e) => {
                e.stopPropagation();
                item.action();
              }}
              onMouseEnter={() => {
                setHoveredItem(item.id);
                playTone(400 + index * 50, 50, 0.05);
              }}
              onMouseLeave={() => setHoveredItem(null)}
              initial={{ scale: 0, x: 0, y: 0, opacity: 0 }}
              animate={{ scale: isHovered ? 1.1 : 1, x, y, opacity: 1 }}
              exit={{ scale: 0, x: 0, y: 0, opacity: 0 }}
              transition={{ 
                type: 'spring', 
                damping: 14, 
                stiffness: 150, 
                delay: index * 0.05 
              }}
              className={`absolute top-1/2 left-1/2 -ml-[23px] -mt-[23px] w-[46px] h-[46px] rounded-full flex flex-col items-center justify-center border pointer-events-auto transition-colors duration-200 ${isHovered ? activeBtnClass : btnClass}`}
            >
              <Icon size={20} />
            </motion.button>
          );
        })}

        <AnimatePresence>
          {hoveredItem && (
            <motion.div
              initial={{ opacity: 0, scale: 0.8 }}
              animate={{ opacity: 1, scale: 1 }}
              exit={{ opacity: 0, scale: 0.8 }}
              className={`absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 font-bold tracking-widest pointer-events-none ${textLabelClass}`}
            >
              {sectors.find(s => s.id === hoveredItem)?.label}
            </motion.div>
          )}
        </AnimatePresence>
      </div>
    </motion.div>
  );
};
import React, { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { MessageSquare, Settings, Sparkles, Brain, Palette, UserSquare } from 'lucide-react';
import { useNavigate } from 'react-router-dom';
import { playTone, vibrate } from '../lib/feedback';
import { TOKENS_TABLE, ThemeName } from '../theme/tokens';

interface MenuOverlayProps {
  theme: string;
}

export const MenuOverlay: React.FC<MenuOverlayProps> = ({ theme }) => {
  const navigate = useNavigate();
  const [hoveredItem, setHoveredItem] = useState<string | null>(null);
  const t = TOKENS_TABLE[theme as ThemeName] || TOKENS_TABLE.tech;

  useEffect(() => {
    playTone(600, 80, 0.1);
  }, []);

  const navigateTo = (newState: string, newTheme: string = theme) => {
    playTone(1200, 80, 0.2);
    vibrate([30]);
    navigate(`/?state=${newState}&theme=${newTheme}`);
  };

  const sectors = [
    { id: 'chat', label: '对话', icon: MessageSquare, angle: -90, action: () => navigateTo('talking') },
    { id: 'model', label: '模型', icon: Sparkles, angle: -30, action: () => navigateTo('model') },
    { id: 'theme', label: '主题', icon: Palette, angle: 30, action: () => navigateTo('theme_picker') },
    { id: 'settings', label: '设置', icon: Settings, angle: 90, action: () => navigateTo('settings') },
    { id: 'persona', label: '人格', icon: UserSquare, angle: 150, action: () => navigateTo('persona') },
    { id: 'memory', label: '记忆', icon: Brain, angle: 210, action: () => navigateTo('memory') }
  ];

  const radius = 78;

  return (
    <motion.div 
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      exit={{ opacity: 0 }}
      transition={{ duration: 0.2 }}
      className="absolute inset-0 backdrop-blur-sm z-20 overflow-hidden"
      style={{ backgroundColor: `${t.bg}CC` }}
      onClick={() => navigateTo('idle')}
    >
      <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-full h-full pointer-events-none">
        <motion.button
          onClick={(e) => {
            e.stopPropagation();
            navigateTo('idle');
          }}
          className="absolute top-1/2 left-1/2 -ml-[20px] -mt-[20px] w-[40px] h-[40px] rounded-full flex items-center justify-center pointer-events-auto z-10 border transition-colors duration-200 hover:brightness-110"
          style={{ color: t.accent, backgroundColor: `${t.panel}80`, borderColor: `${t.border}B3` }}
        >
          <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.4" strokeLinecap="round" strokeLinejoin="round"><path d="M18 6 6 18"/><path d="m6 6 12 12"/></svg>
        </motion.button>
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
              className="absolute top-1/2 left-1/2 -ml-[28px] -mt-[28px] w-[56px] h-[56px] rounded-full flex flex-col items-center justify-center border pointer-events-auto transition-colors duration-200"
              style={{
                backgroundColor: isHovered ? t.panel : `${t.panel}CC`,
                borderColor: isHovered ? t.accent : `${t.border}80`,
                color: isHovered ? t.accent_hi : t.accent,
                boxShadow: isHovered ? `0 0 20px ${t.accent}99` : `0 0 15px ${t.accent}33`
              }}
            >
              <Icon size={24} strokeWidth={2.4} />
            </motion.button>
          );
        })}

        <AnimatePresence>
          {hoveredItem && (
            <motion.div
              initial={{ opacity: 0, scale: 0.8 }}
              animate={{ opacity: 1, scale: 1 }}
              exit={{ opacity: 0, scale: 0.8 }}
              className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 font-bold tracking-widest pointer-events-none"
              style={{ color: t.text, textShadow: `0 0 8px ${t.accent}CC` }}
            >
              {sectors.find(s => s.id === hoveredItem)?.label}
            </motion.div>
          )}
        </AnimatePresence>
      </div>
    </motion.div>
  );
};
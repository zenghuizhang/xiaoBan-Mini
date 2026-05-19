import React, { useState, useEffect } from 'react';
import { motion } from 'framer-motion';
import { Smile, MessageSquare, Settings, Palette, Puzzle, Shuffle } from 'lucide-react';
import { playAudioFeedback } from '../lib/audioFeedback';

interface RadialMenuProps {
  theme: 'tech' | 'child' | 'dev';
  onSelect: (action: string) => void;
  onClose: () => void;
}

export const RadialMenu: React.FC<RadialMenuProps> = ({ theme, onSelect, onClose }) => {
  const [hovered, setHovered] = useState<string | null>(null);

  const radius = 70;
  
  // 6 Sectors
  const items = [
    { id: 'expressions', label: '表情', icon: Smile, angle: -90, action: 'expressions' },
    { id: 'dialogue', label: '对话', icon: MessageSquare, angle: -30, action: 'dialogue' },
    { id: 'settings', label: '设置', icon: Settings, angle: 30, action: 'settings' },
    { id: 'theme', label: '主题', icon: Palette, angle: 90, action: 'theme' },
    { id: 'extensions', label: '扩展', icon: Puzzle, angle: 150, action: 'extensions' },
    { id: 'random', label: '随机', icon: Shuffle, angle: 210, action: 'random' }
  ];

  const getThemeColors = () => {
    switch (theme) {
      case 'child':
        return {
          bg: 'bg-white/90',
          border: 'border-orange-200',
          text: 'text-orange-500',
          activeBg: 'bg-orange-100',
          glow: 'shadow-[0_0_15px_rgba(255,127,80,0.4)]',
          line: 'bg-orange-300'
        };
      case 'dev':
        return {
          bg: 'bg-black/90',
          border: 'border-green-800/50',
          text: 'text-green-400',
          activeBg: 'bg-green-900/50',
          glow: 'shadow-[0_0_15px_rgba(34,197,94,0.4)]',
          line: 'bg-green-700'
        };
      default: // tech
        return {
          bg: 'bg-black/80',
          border: 'border-cyan-800/50',
          text: 'text-cyan-400',
          activeBg: 'bg-cyan-900/50',
          glow: 'shadow-[0_0_15px_rgba(34,211,238,0.4)]',
          line: 'bg-cyan-700'
        };
    }
  };

  const colors = getThemeColors();

  useEffect(() => {
    playAudioFeedback('menuOpen');
  }, []);

  return (
    <motion.div
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      exit={{ opacity: 0, transition: { duration: 0.2 } }}
      className="absolute inset-0 z-40 backdrop-blur-sm flex items-center justify-center"
      onClick={onClose}
    >
      <div className="relative w-full h-full flex items-center justify-center">
        {/* Center eye representation / Cancel zone */}
        <motion.div
          initial={{ scale: 0 }}
          animate={{ scale: 1 }}
          exit={{ scale: 0 }}
          className={`absolute w-12 h-12 rounded-full flex items-center justify-center border ${colors.bg} ${colors.border} ${colors.text} ${colors.glow}`}
          onClick={(e) => { e.stopPropagation(); onClose(); }}
        >
          <div className="text-[10px] font-bold">返回</div>
        </motion.div>

        {items.map((item, index) => {
          const rad = (item.angle * Math.PI) / 180;
          const x = Math.cos(rad) * radius;
          const y = Math.sin(rad) * radius;
          const isHovered = hovered === item.id;

          return (
            <motion.div
              key={item.id}
              initial={{ x: 0, y: 0, scale: 0, opacity: 0 }}
              animate={{ x, y, scale: 1, opacity: 1 }}
              exit={{ x: 0, y: 0, scale: 0, opacity: 0, transition: { type: 'spring', damping: 20, stiffness: 300, delay: 0 } }}
              transition={{
                type: 'spring',
                damping: 14,
                stiffness: 150,
                delay: index * 0.05
              }}
              className="absolute"
            >
              {/* Connecting line */}
              <motion.div 
                className={`absolute left-1/2 top-1/2 h-[1px] origin-left -z-10 ${colors.line} opacity-50`}
                style={{ 
                  width: radius - 24, 
                  rotate: `${item.angle}deg`,
                  x: -x, 
                  y: -y,
                }}
              />

              <button
                className={`relative flex flex-col items-center justify-center w-[46px] h-[46px] rounded-full border transition-colors ${colors.bg} ${colors.border} ${colors.text} ${isHovered ? colors.activeBg + ' ' + colors.glow : ''}`}
                onMouseEnter={() => setHovered(item.id)}
                onMouseLeave={() => setHovered(null)}
                onClick={(e) => {
                  e.stopPropagation();
                  onSelect(item.action);
                }}
                style={{
                  transform: isHovered ? 'scale(1.1)' : 'scale(1)'
                }}
              >
                <item.icon size={18} className="mb-0.5" />
                <span className="text-[8px] font-medium tracking-wider">{item.label}</span>
              </button>
            </motion.div>
          );
        })}
      </div>
    </motion.div>
  );
};
import React, { useEffect, useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { Check, X } from 'lucide-react';
import { TOKENS_TABLE, ThemeName } from '../theme/tokens';

export type DialogType = 'text' | 'suggest';

interface DialogBubbleProps {
  text: string;
  type?: DialogType;
  theme: string;
  onAction?: (action: 'yes' | 'no' | 'timeout') => void;
  autoCloseDelay?: number;
}

export const DialogBubble: React.FC<DialogBubbleProps> = ({ 
  text, 
  type = 'text', 
  theme,
  onAction,
  autoCloseDelay = 5000
}) => {
  const [visible, setVisible] = useState(true);
  const t = TOKENS_TABLE[theme as ThemeName] || TOKENS_TABLE.tech;

  useEffect(() => {
    if (type === 'text') {
      const timer = setTimeout(() => {
        setVisible(false);
      }, autoCloseDelay);
      return () => clearTimeout(timer);
    } else if (type === 'suggest') {
      const timer = setTimeout(() => {
        setVisible(false);
        onAction?.('timeout');
      }, 3000);
      return () => clearTimeout(timer);
    }
  }, [type, autoCloseDelay, onAction]);

  const handleAction = (act: 'yes' | 'no') => {
    setVisible(false);
    onAction?.(act);
  };

  return (
    <AnimatePresence>
      {visible && (
        <motion.div
          initial={{ opacity: 0, y: -10, scale: 0.95 }}
          animate={{ opacity: 1, y: 0, scale: 1 }}
          exit={{ opacity: 0, y: -10, scale: 0.95 }}
          transition={{ type: "spring", damping: 20, stiffness: 300 }}
          className="absolute top-4 left-1/2 -translate-x-1/2 w-[85%] max-w-[280px] p-2.5 rounded-2xl border backdrop-blur-md z-40 flex flex-col items-center gap-1.5"
          style={{ backgroundColor: `${t.panel}CC`, borderColor: `${t.border}80`, color: t.text, boxShadow: `0 0 15px ${t.accent}33` }}
        >
          <p className="text-[11px] font-medium text-center leading-relaxed tracking-wide z-10 relative">
            {text}
          </p>
          
          {type === 'suggest' && (
            <div className="flex gap-3 mt-1 z-10 relative w-full justify-center">
              <button 
                onClick={() => handleAction('no')}
                className="p-1.5 rounded-full border transition-colors hover:brightness-110"
                style={{ backgroundColor: `${t.panel}80`, borderColor: t.border, color: `${t.text}B3` }}
              >
                <X size={12} />
              </button>
              <button 
                onClick={() => handleAction('yes')}
                className="p-1.5 rounded-full border transition-colors hover:brightness-110"
                style={{ backgroundColor: `${t.accent}33`, borderColor: `${t.accent}80`, color: t.accent_hi }}
              >
                <Check size={12} />
              </button>
            </div>
          )}
        </motion.div>
      )}
    </AnimatePresence>
  );
};
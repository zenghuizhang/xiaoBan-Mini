import React, { useEffect, useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { Check, X } from 'lucide-react';

export type DialogType = 'text' | 'suggest';

interface DialogBubbleProps {
  text: string;
  type?: DialogType;
  theme: 'tech' | 'child' | 'dev';
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

  const bubbleClass = theme === 'tech'
    ? "bg-cyan-950/80 border-cyan-800/50 text-cyan-100 shadow-[0_0_15px_rgba(34,211,238,0.2)]"
    : theme === 'dev'
    ? "bg-green-950/80 border-green-800/50 text-green-100 shadow-[0_0_15px_rgba(34,197,94,0.2)]"
    : "bg-white/95 border-orange-200/80 text-orange-800 shadow-[0_5px_15px_rgba(255,127,80,0.15)]";

  const btnYesClass = theme === 'tech'
    ? "bg-cyan-900/50 hover:bg-cyan-800 text-cyan-300 border-cyan-700/50"
    : theme === 'dev'
    ? "bg-green-900/50 hover:bg-green-800 text-green-300 border-green-700/50"
    : "bg-orange-100 hover:bg-orange-200 text-orange-600 border-orange-300/50";

  const btnNoClass = theme === 'tech'
    ? "bg-black/50 hover:bg-cyan-950 text-cyan-600 border-cyan-900/50"
    : theme === 'dev'
    ? "bg-black/50 hover:bg-green-950 text-green-600 border-green-900/50"
    : "bg-gray-50 hover:bg-gray-100 text-gray-500 border-gray-200";

  return (
    <AnimatePresence>
      {visible && (
        <motion.div
          initial={{ opacity: 0, y: -10, scale: 0.95 }}
          animate={{ opacity: 1, y: 0, scale: 1 }}
          exit={{ opacity: 0, y: -10, scale: 0.95 }}
          transition={{ type: "spring", damping: 20, stiffness: 300 }}
          className={`absolute top-4 left-1/2 -translate-x-1/2 w-[85%] max-w-[280px] p-2.5 rounded-2xl border backdrop-blur-md z-40 flex flex-col items-center gap-1.5 ${bubbleClass}`}
        >
          <p className="text-[11px] font-medium text-center leading-relaxed tracking-wide z-10 relative">
            {text}
          </p>
          
          {type === 'suggest' && (
            <div className="flex gap-3 mt-1 z-10 relative w-full justify-center">
              <button 
                onClick={() => handleAction('no')}
                className={`p-1.5 rounded-full border transition-colors ${btnNoClass}`}
              >
                <X size={12} />
              </button>
              <button 
                onClick={() => handleAction('yes')}
                className={`p-1.5 rounded-full border transition-colors ${btnYesClass}`}
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
import React, { useEffect, useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { BatteryFull, BatteryWarning, Wifi, Plug, Circle } from 'lucide-react';
import { TOKENS_TABLE, ThemeName } from '../theme/tokens';

export type StatusBarMode = 'idle' | 'transient' | 'critical' | 'always';

interface StatusBarProps {
  mode: StatusBarMode;
  batteryLevel?: number;
  isCharging?: boolean;
  isWifiConnected?: boolean;
  theme: string;
}

export const StatusBar: React.FC<StatusBarProps> = ({ 
  mode, 
  batteryLevel = 88, 
  isCharging = false, 
  isWifiConnected = true,
  theme
}) => {
  const [visible, setVisible] = useState(false);
  const t = TOKENS_TABLE[theme as ThemeName] || TOKENS_TABLE.tech;

  useEffect(() => {
    if (mode === 'always' || mode === 'critical') {
      setVisible(true);
    } else if (mode === 'transient') {
      setVisible(true);
      const timer = setTimeout(() => setVisible(false), 2000);
      return () => clearTimeout(timer);
    } else {
      setVisible(false);
    }
  }, [mode]);

  const isLowBat = batteryLevel <= 15;
  const isOffline = !isWifiConnected;

  return (
    <AnimatePresence>
      {visible && (
        <motion.div
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          exit={{ opacity: 0 }}
          transition={{ duration: 0.2 }}
          className="absolute top-0 left-0 w-full h-[22px] px-2 flex items-center justify-end gap-2.5 z-50 backdrop-blur-sm"
          style={{ backgroundColor: `${t.bg}80`, color: t.text }}
        >
          {isCharging && <Plug size={16} strokeWidth={2.4} style={{ color: t.accent }} />}
          
          <Circle size={16} className="opacity-80" fill="currentColor" strokeWidth={2.4} />
          
          {isOffline ? (
            <div className="relative">
              <Wifi size={16} className="opacity-40" strokeWidth={2.4} />
              <div className="absolute top-[8px] left-[-2px] w-[20px] h-[2px] rotate-[135deg]" style={{ backgroundColor: t.danger }} />
            </div>
          ) : (
             <Wifi size={16} className="opacity-80" strokeWidth={2.4} />
          )}

          <div className="flex items-center gap-1">
            <span className="text-[11px] font-mono leading-none" style={{ color: isLowBat ? t.danger : 'inherit', opacity: isLowBat ? 1 : 0.9 }}>
              {batteryLevel}%
            </span>
            {isLowBat ? (
              <BatteryWarning size={16} strokeWidth={2.4} style={{ color: t.danger }} />
            ) : (
              <BatteryFull size={16} className="opacity-80" strokeWidth={2.4} />
            )}
          </div>
        </motion.div>
      )}
    </AnimatePresence>
  );
};
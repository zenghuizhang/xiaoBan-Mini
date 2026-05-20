import React from 'react';
import { motion } from 'framer-motion';
import { Sun, Volume2, Clock, X } from 'lucide-react';
import { useSearchParams } from 'react-router-dom';
import { MotionData } from './MotionController';

export interface EnvState {
  lux: number;
  db: number;
  hour: number;
}

interface EnvSimulatorProps {
  env: EnvState;
  onEnvChange: (env: EnvState) => void;
  onClose: () => void;
  theme: 'tech' | 'child' | 'dev';
  motionData?: MotionData | null;
}

export const EnvSimulator: React.FC<EnvSimulatorProps> = ({ env, onEnvChange, onClose, theme, motionData }) => {
  const getThemeColors = () => {
    if (theme === 'child') return { bg: 'bg-white/95', border: 'border-orange-200', text: 'text-orange-600', accent: 'accent-orange-400' };
    if (theme === 'dev') return { bg: 'bg-black/95', border: 'border-green-800', text: 'text-green-400', accent: 'accent-green-500' };
    return { bg: 'bg-black/95', border: 'border-cyan-800', text: 'text-cyan-400', accent: 'accent-cyan-400' };
  };

  const colors = getThemeColors();

  return (
    <motion.div
      initial={{ opacity: 0, x: 100 }}
      animate={{ opacity: 1, x: 0 }}
      exit={{ opacity: 0, x: 100 }}
      className={`absolute right-2 top-2 w-[140px] p-3 rounded-xl border z-50 backdrop-blur-md shadow-2xl ${colors.bg} ${colors.border}`}
    >
      <div className={`flex justify-between items-center mb-3 ${colors.text}`}>
        <span className="text-[10px] font-bold tracking-widest">环境感知模拟</span>
        <button onClick={onClose} className="p-0.5 rounded-full bg-white/10 hover:bg-white/20 transition-colors">
          <X size={12} />
        </button>
      </div>

      <div className="space-y-4">
        {/* Lux Slider */}
        <div className="space-y-1">
          <div className={`flex justify-between text-[8px] ${colors.text}`}>
            <span className="flex items-center gap-1"><Sun size={10} />光照 (Lux)</span>
            <span>{env.lux}</span>
          </div>
          <input 
            type="range" min="0" max="1000" 
            value={env.lux} 
            onChange={(e) => onEnvChange({ ...env, lux: Number(e.target.value) })}
            className={`w-full h-1 bg-white/20 rounded-lg appearance-none cursor-pointer ${colors.accent}`} 
          />
        </div>

        {/* Noise Slider */}
        <div className="space-y-1">
          <div className={`flex justify-between text-[8px] ${colors.text}`}>
            <span className="flex items-center gap-1"><Volume2 size={10} />噪音 (dB)</span>
            <span>{env.db}</span>
          </div>
          <input 
            type="range" min="0" max="100" 
            value={env.db} 
            onChange={(e) => onEnvChange({ ...env, db: Number(e.target.value) })}
            className={`w-full h-1 bg-white/20 rounded-lg appearance-none cursor-pointer ${colors.accent}`} 
          />
        </div>

        {/* Time Slider */}
        <div className="space-y-1">
          <div className={`flex justify-between text-[8px] ${colors.text}`}>
            <span className="flex items-center gap-1"><Clock size={10} />时间 (时)</span>
            <span>{env.hour}:00</span>
          </div>
          <input 
            type="range" min="0" max="23" 
            value={env.hour} 
            onChange={(e) => onEnvChange({ ...env, hour: Number(e.target.value) })}
            className={`w-full h-1 bg-white/20 rounded-lg appearance-none cursor-pointer ${colors.accent}`} 
          />
        </div>

        {motionData && (
          <div className={`pt-3 border-t ${colors.border} space-y-2`}>
            <span className={`text-[9px] font-bold tracking-widest ${colors.text}`}>体感数据流</span>
            <div className={`grid grid-cols-2 gap-1 text-[8px] ${colors.text} font-mono bg-white/5 p-1.5 rounded`}>
              <div>P: {motionData.pitch}°</div>
              <div>R: {motionData.roll}°</div>
              <div>A: {motionData.accel}g</div>
              <div>Z: {motionData.zAccel}g</div>
            </div>
          </div>
        )}
      </div>
    </motion.div>
  );
};
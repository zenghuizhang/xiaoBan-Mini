// copied from refProto1:src/components/SettingsOverlay.tsx
import React, { useState } from 'react';
import { motion } from 'framer-motion';
import { ChevronLeft, Volume2, Sun, Mic, Wifi, Clock, User, RefreshCw, AlertTriangle, ChevronRight } from 'lucide-react';
import { useNavigate } from 'react-router-dom';
import { playTone, vibrate } from '../lib/feedback';

interface SettingsOverlayProps {
  theme: 'tech' | 'child' | 'dev';
}

export const SettingsOverlay: React.FC<SettingsOverlayProps> = ({ theme }) => {
  const navigate = useNavigate();
  const [volume, setVolume] = useState(70);
  const [brightness, setBrightness] = useState(80);

  const navigateTo = (newState: string) => {
    playTone(1000, 50, 0.1);
    vibrate([20]);
    navigate(`/?state=${newState}&theme=${theme}`);
  };

  const overlayClass = theme === 'tech' || theme === 'dev' ? "bg-black/90" : "bg-[#FFF9E6]/95";
  const textClass = theme === 'tech' ? 'text-cyan-500' : theme === 'dev' ? 'text-green-500' : 'text-orange-600';
  const textSubClass = theme === 'tech' ? 'text-cyan-600/70' : theme === 'dev' ? 'text-green-600/70' : 'text-orange-400/70';
  const closeBtnClass = theme === 'tech'
    ? "text-cyan-600/70 hover:text-cyan-400 hover:bg-cyan-950/50 border-cyan-900/50"
    : theme === 'dev'
    ? "text-green-600/70 hover:text-green-400 hover:bg-green-950/50 border-green-900/50"
    : "text-orange-400 hover:text-orange-600 hover:bg-orange-100 border-orange-200";
  
  const itemClass = theme === 'tech'
    ? "bg-cyan-950/30 border-cyan-900/40 text-cyan-400"
    : theme === 'dev'
    ? "bg-green-950/30 border-green-900/40 text-green-400"
    : "bg-white/70 border-orange-200 text-orange-600";

  const sliderAccent = theme === 'tech' ? 'bg-cyan-400' : theme === 'dev' ? 'bg-green-400' : 'bg-orange-400';
  const sliderTrack = theme === 'tech' ? 'bg-cyan-950' : theme === 'dev' ? 'bg-green-950' : 'bg-orange-100';

  const CustomSlider = ({ value, onChange, icon: Icon }: any) => (
    <div className={`flex items-center gap-2 p-2 rounded border ${itemClass}`}>
      <Icon size={14} className={textSubClass} />
      <div className="flex-1 relative h-1.5 rounded-full overflow-hidden flex items-center">
        <div className={`absolute left-0 top-0 bottom-0 w-full ${sliderTrack}`}></div>
        <div className={`absolute left-0 top-0 bottom-0 ${sliderAccent}`} style={{ width: `${value}%` }}></div>
        <input 
          type="range" 
          min="0" max="100" 
          value={value} 
          onChange={(e) => onChange(Number(e.target.value))}
          className="absolute inset-0 w-full opacity-0 cursor-pointer"
        />
      </div>
      <span className={`text-[10px] w-6 text-right font-mono ${textSubClass}`}>{value}%</span>
    </div>
  );

  const ActionItem = ({ icon: Icon, label, value, onClick, isAlert = false }: any) => (
    <button 
      onClick={onClick}
      className={`w-full flex items-center justify-between p-2.5 rounded border transition-colors ${itemClass} ${isAlert ? (theme === 'tech' || theme === 'dev' ? 'hover:border-rose-500/50 hover:text-rose-400 text-rose-500/80 border-rose-900/30 bg-rose-950/20' : 'hover:border-rose-400 hover:text-rose-500 text-rose-500 border-rose-200 bg-rose-50') : ''}`}
    >
      <div className="flex items-center gap-2">
        <Icon size={14} className={isAlert ? 'text-rose-500/70' : textSubClass} />
        <span className="text-[11px] tracking-wide">{label}</span>
      </div>
      <div className="flex items-center gap-1">
        {value && <span className={`text-[10px] ${isAlert ? 'text-rose-500/50' : textSubClass}`}>{value}</span>}
        <ChevronRight size={12} className={isAlert ? 'text-rose-500/50' : textSubClass} />
      </div>
    </button>
  );

  return (
    <motion.div 
      initial={{ opacity: 0, scale: 0.95 }}
      animate={{ opacity: 1, scale: 1 }}
      exit={{ opacity: 0, scale: 0.95 }}
      transition={{ type: 'spring', damping: 25, stiffness: 200 }}
      className={`absolute inset-0 z-20 flex flex-col backdrop-blur-md ${overlayClass}`}
    >
      <div className="flex items-center p-3 relative shadow-sm z-10">
        <button 
          onClick={() => navigateTo('menu')}
          className={`p-1.5 rounded-full transition-all border ${closeBtnClass}`}
        >
          <ChevronLeft size={16} />
        </button>
        <h2 className={`absolute left-1/2 -translate-x-1/2 text-xs font-bold tracking-widest ${textClass}`}>
          系统设置
        </h2>
      </div>

      <div className="flex-1 overflow-y-auto scrollbar-hide px-3 pb-4 space-y-2.5" style={{ scrollbarWidth: 'none', msOverflowStyle: 'none' }}>
        <style>{`.scrollbar-hide::-webkit-scrollbar { display: none; }`}</style>
        
        {/* Controls */}
        <div className="space-y-2 mt-1">
          <CustomSlider value={volume} onChange={setVolume} icon={Volume2} />
          <CustomSlider value={brightness} onChange={setBrightness} icon={Sun} />
        </div>

        {/* Configurations */}
        <div className="space-y-1.5 pt-2">
          <ActionItem icon={Wifi} label="Wi-Fi网络" value="未连接" onClick={() => navigateTo('wifi_ap')} />
          <ActionItem icon={Mic} label="语音设定" value="默认" onClick={() => {}} />
          <ActionItem icon={Clock} label="睡眠定时" value="30分钟" onClick={() => {}} />
        </div>

        {/* System */}
        <div className="space-y-1.5 pt-2">
          <ActionItem icon={User} label="账号绑定" value="已绑定 User01" onClick={() => {}} />
          <ActionItem icon={RefreshCw} label="检查更新" value="当前 v6.0" onClick={() => {}} />
          <ActionItem icon={AlertTriangle} label="出厂重置" value="" onClick={() => {}} isAlert />
        </div>
      </div>
    </motion.div>
  );
};
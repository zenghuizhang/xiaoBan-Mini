import React, { useState } from 'react';
import { motion } from 'framer-motion';
import { ChevronLeft, Volume2, Sun, Wifi, Clock, User, Info, AlertTriangle, ChevronRight, Terminal, Globe, DownloadCloud, Package } from 'lucide-react';
import { useNavigate } from 'react-router-dom';
import { playTone, vibrate } from '../lib/feedback';
import { TOKENS_TABLE, ThemeName } from '../theme/tokens';

interface SettingsOverlayProps {
  theme: string;
}

export const SettingsOverlay: React.FC<SettingsOverlayProps> = ({ theme }) => {
  const navigate = useNavigate();
  const [volume, setVolume] = useState(70);
  const [brightness, setBrightness] = useState(80);
  const t = TOKENS_TABLE[theme as ThemeName] || TOKENS_TABLE.tech;

  const navigateTo = (newState: string) => {
    playTone(1000, 50, 0.1);
    vibrate([20]);
    navigate(`/?state=${newState}&theme=${theme}`);
  };

  const CustomSlider = ({ value, onChange, icon: Icon }: any) => (
    <div className="w-full flex items-center px-3 py-2 border-b last:border-0" style={{ borderColor: t.border }}>
      <Icon size={16} strokeWidth={2.4} style={{ color: `${t.text}B3` }} />
      <div className="flex-1 mx-3 relative h-1.5 rounded-full flex items-center">
        <div className="absolute left-0 top-0 bottom-0 w-full rounded-full opacity-20" style={{ backgroundColor: t.text }}></div>
        <div className="absolute left-0 top-0 bottom-0 rounded-full" style={{ backgroundColor: t.accent, width: `${value}%` }}></div>
        <input 
          type="range" 
          min="0" max="100" 
          value={value} 
          onChange={(e) => onChange(Number(e.target.value))}
          className="absolute inset-0 w-full opacity-0 cursor-pointer"
        />
      </div>
    </div>
  );

  const ActionItem = ({ icon: Icon, label, value, onClick, isAlert = false, rightElement }: any) => (
    <button 
      onClick={onClick}
      className="w-full flex items-center justify-between px-3 py-2 border-b last:border-0 transition-colors hover:brightness-110"
      style={{ borderColor: t.border, backgroundColor: 'transparent' }}
    >
      <div className="flex items-center gap-2">
        <Icon size={16} strokeWidth={2.4} style={{ color: isAlert ? t.danger : `${t.text}B3` }} />
        <span className="text-[14px]" style={{ color: isAlert ? t.danger : t.text }}>{label}</span>
      </div>
      <div className="flex items-center gap-2">
        {rightElement ? rightElement : (
          <>
            {value && <span className="text-[12px]" style={{ color: isAlert ? `${t.danger}99` : `${t.text}80` }}>{value}</span>}
            <ChevronRight size={16} strokeWidth={2.4} style={{ color: isAlert ? `${t.danger}99` : `${t.text}80` }} />
          </>
        )}
      </div>
    </button>
  );

  const Group = ({ title, children, badge }: any) => (
    <div className="mb-3">
      <div className="flex items-center mb-1 px-1">
        <h3 className="text-[12px] font-bold" style={{ color: `${t.text}B3` }}>{title}</h3>
        {badge && (
          <span className="ml-2 text-[9px] font-bold px-1.5 py-0.5 rounded-sm leading-none" style={{ backgroundColor: t.accent, color: '#000' }}>
            {badge}
          </span>
        )}
      </div>
      <div className="rounded-[4px] border overflow-hidden" style={{ backgroundColor: theme === 'tech' ? `${t.panel}4D` : t.panel, borderColor: t.border }}>
        {children}
      </div>
    </div>
  );

  return (
    <motion.div 
      initial={{ opacity: 0, scale: 0.95 }}
      animate={{ opacity: 1, scale: 1 }}
      exit={{ opacity: 0, scale: 0.95 }}
      transition={{ type: 'spring', damping: 25, stiffness: 200 }}
      className="absolute inset-0 z-20 flex flex-col backdrop-blur-md"
      style={{ backgroundColor: `${t.bg}E6` }}
    >
      <div className="flex items-center p-3 relative shadow-sm z-10">
        <button 
          onClick={() => navigateTo('menu')}
          className="p-1.5 rounded-full transition-all border hover:brightness-110"
          style={{ color: `${t.text}B3`, borderColor: 'transparent', backgroundColor: `${t.panel}80` }}
        >
          <ChevronLeft size={16} />
        </button>
        <h2 className="absolute left-1/2 -translate-x-1/2 text-xs font-bold tracking-widest" style={{ color: t.text }}>
          系统设置
        </h2>
      </div>

      <div className="flex-1 overflow-y-auto scrollbar-hide px-2 py-2" style={{ scrollbarWidth: 'none', msOverflowStyle: 'none' }}>
        <style>{`.scrollbar-hide::-webkit-scrollbar { display: none; }`}</style>
        
        <Group title="通用">
          <ActionItem icon={User} label="账号绑定" value="User01" onClick={() => {}} />
          <ActionItem icon={Globe} label="语言设置" value="简体中文" onClick={() => {}} />
          <ActionItem icon={Clock} label="睡眠定时" value="30分钟" onClick={() => {}} />
        </Group>

        <Group title="显示与声音">
          <CustomSlider value={brightness} onChange={setBrightness} icon={Sun} />
          <CustomSlider value={volume} onChange={setVolume} icon={Volume2} />
        </Group>

        <Group title="拓展功能">
          <ActionItem icon={Wifi} label="Wi-Fi网络" value="未连接" onClick={() => navigateTo('wifi_ap')} />
          <ActionItem icon={Package} label="技能插件" value="3 个已安装" onClick={() => navigateTo('skills')} />
        </Group>

        <Group title="系统">
          <ActionItem icon={DownloadCloud} label="系统更新" value="有新版本" onClick={() => navigateTo('ota')} isAlert={true} />
          <ActionItem icon={AlertTriangle} label="清除偏好数据" value="" onClick={() => {}} isAlert />
          <ActionItem icon={Info} label="关于系统" value="v7.2" onClick={() => {}} />
        </Group>

        <Group title="开发者选项" badge="测试">
          <ActionItem icon={Terminal} label="控制台" value="" onClick={() => navigateTo('console_ai')} />
          <ActionItem 
            icon={AlertTriangle} 
            label="详细日志" 
            onClick={() => {}} 
            rightElement={
              <div className="w-8 h-4 rounded-full p-[2px]" style={{ backgroundColor: t.accent }}>
                <div className="w-3 h-3 rounded-full ml-auto" style={{ backgroundColor: t.bg }}></div>
              </div>
            }
          />
        </Group>
      </div>
    </motion.div>
  );
};
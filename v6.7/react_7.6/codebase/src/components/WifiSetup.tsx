import React, { useEffect } from 'react';
import { motion } from 'framer-motion';
import { ChevronLeft, Loader2, CheckCircle2, XCircle, QrCode } from 'lucide-react';
import { useNavigate, useSearchParams } from 'react-router-dom';
import { TOKENS_TABLE, ThemeName } from '../theme/tokens';

interface WifiSetupProps {
  state: string;
  theme: string;
}

export const WifiSetup: React.FC<WifiSetupProps> = ({ state, theme }) => {
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();
  const barQuery = searchParams.get('bar') === '1' ? '&bar=1' : '';
  const t = TOKENS_TABLE[theme as ThemeName] || TOKENS_TABLE.tech;

  const navigateTo = (newState: string) => {
    navigate(`/?state=${newState}&theme=${theme}${barQuery}`);
  };

  useEffect(() => {
    if (state === 'wifi_connecting') {
      const timer = setTimeout(() => {
        navigateTo('wifi_success');
      }, 2000);
      return () => clearTimeout(timer);
    }
  }, [state, navigate, theme, barQuery]);
  
  const renderContent = () => {
    switch (state) {
      case 'wifi_ap':
        return (
          <div className="flex-1 flex flex-col items-center justify-center p-4 gap-3">
            <div className="p-2 rounded-lg" style={{ backgroundColor: '#ffffff', boxShadow: `0 0 20px ${t.accent}33` }}>
              <QrCode size={80} className="text-black" />
            </div>
            <div className="text-center space-y-1">
              <p className="text-xs font-bold tracking-wider" style={{ color: t.accent }}>扫码配网</p>
              <p className="text-[10px]" style={{ color: `${t.text}B3` }}>热点: Robot_AP_1234</p>
            </div>
            <button 
              onClick={() => navigateTo('wifi_connecting')}
              className="mt-2 px-6 py-2 rounded-full text-xs font-medium border transition-all tracking-wider hover:brightness-110"
              style={{ backgroundColor: `${t.panel}80`, borderColor: `${t.border}80`, color: t.accent }}
            >
              模拟接收配置
            </button>
            <button 
              onClick={() => navigateTo('wifi_error')}
              className="opacity-0 absolute bottom-0 right-0 w-8 h-8"
              aria-label="Simulate Error"
            />
          </div>
        );

      case 'wifi_connecting':
        return (
          <div className="flex-1 flex flex-col items-center justify-center gap-4">
            <Loader2 size={36} className="animate-spin" style={{ color: t.accent, filter: `drop-shadow(0 0 12px ${t.accent}CC)` }} />
            <span className="text-xs font-medium animate-pulse tracking-widest" style={{ color: t.accent }}>正在连接...</span>
          </div>
        );

      case 'wifi_success':
        return (
          <div className="flex-1 flex flex-col items-center justify-center gap-4">
            <motion.div initial={{ scale: 0 }} animate={{ scale: 1 }} transition={{ type: "spring", bounce: 0.5 }}>
              <CheckCircle2 size={48} style={{ color: t.accent, filter: `drop-shadow(0 0 15px ${t.accent}CC)` }} />
            </motion.div>
            <span className="text-xs font-medium tracking-widest" style={{ color: t.accent }}>连接成功</span>
            <button 
              onClick={() => navigateTo('idle')}
              className="mt-4 px-8 py-2 rounded-full text-xs font-medium border transition-all tracking-wider hover:brightness-110"
              style={{ backgroundColor: `${t.panel}80`, borderColor: `${t.border}80`, color: t.accent }}
            >
              完成
            </button>
          </div>
        );

      case 'wifi_error':
        return (
          <div className="flex-1 flex flex-col items-center justify-center gap-4">
            <motion.div initial={{ scale: 0 }} animate={{ scale: 1 }} transition={{ type: "spring", bounce: 0.5 }}>
              <XCircle size={48} style={{ color: t.danger, filter: `drop-shadow(0 0 15px ${t.danger}99)` }} />
            </motion.div>
            <span className="text-xs font-medium tracking-widest" style={{ color: t.danger }}>连接失败</span>
            <div className="flex gap-4 mt-4">
              <button 
                onClick={() => navigateTo('wifi_ap')}
                className="px-6 py-2 rounded-full text-xs font-medium border transition-all tracking-wider hover:brightness-110"
                style={{ backgroundColor: `${t.bg}80`, borderColor: `${t.border}80`, color: `${t.text}B3` }}
              >
                返回
              </button>
              <button 
                onClick={() => navigateTo('wifi_connecting')}
                className="px-6 py-2 rounded-full text-xs font-medium border transition-all tracking-wider hover:brightness-110"
                style={{ backgroundColor: `${t.danger}1A`, borderColor: `${t.danger}33`, color: t.danger }}
              >
                重试
              </button>
            </div>
          </div>
        );

      default:
        return null;
    }
  };

  return (
    <motion.div 
      initial={{ opacity: 0, scale: 0.95 }}
      animate={{ opacity: 1, scale: 1 }}
      exit={{ opacity: 0, scale: 0.95 }}
      transition={{ type: 'spring', damping: 25, stiffness: 200 }}
      className="absolute inset-0 z-20 flex flex-col backdrop-blur-md"
      style={{ backgroundColor: `${t.bg}E6` }}
    >
      <button 
        onClick={() => navigateTo('settings')}
        className="absolute top-4 left-4 p-1.5 rounded-full transition-all border z-30 hover:brightness-110"
        style={{ color: `${t.text}B3`, borderColor: 'transparent', backgroundColor: `${t.panel}80` }}
      >
        <ChevronLeft size={20} />
      </button>
      
      {renderContent()}
    </motion.div>
  );
};
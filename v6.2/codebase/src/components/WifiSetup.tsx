import React, { useEffect } from 'react';
import { motion } from 'framer-motion';
import { ChevronLeft, Loader2, CheckCircle2, XCircle, QrCode } from 'lucide-react';
import { useNavigate, useSearchParams } from 'react-router-dom';

interface WifiSetupProps {
  state: string;
  theme: 'tech' | 'child' | 'dev';
}

export const WifiSetup: React.FC<WifiSetupProps> = ({ state, theme }) => {
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();
  const barQuery = searchParams.get('bar') === '1' ? '&bar=1' : '';

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

  const overlayClass = theme === 'tech' || theme === 'dev' ? "bg-black/90" : "bg-white/95";
  const backBtnClass = theme === 'tech' 
    ? "text-cyan-600/70 hover:text-cyan-400 hover:bg-cyan-950/50 border-transparent hover:border-cyan-900/50" 
    : theme === 'dev'
    ? "text-green-600/70 hover:text-green-400 hover:bg-green-950/50 border-transparent hover:border-green-900/50"
    : "text-orange-400 hover:text-orange-600 hover:bg-orange-100 border-transparent hover:border-orange-200";
    
  const qrBoxClass = theme === 'tech' ? "bg-white/90 shadow-[0_0_20px_rgba(34,211,238,0.2)]" : theme === 'dev' ? "bg-white/90 shadow-[0_0_20px_rgba(34,197,94,0.2)]" : "bg-white shadow-[0_0_20px_rgba(255,127,80,0.15)] border border-orange-100";
  const textTitleClass = theme === 'tech' ? "text-cyan-400" : theme === 'dev' ? "text-green-400" : "text-orange-500";
  const textSubClass = theme === 'tech' ? "text-cyan-600" : theme === 'dev' ? "text-green-600" : "text-orange-400/70";
  const btnClass = theme === 'tech'
    ? "bg-cyan-950/50 text-cyan-400 border-cyan-800/50 hover:bg-cyan-900/80 hover:shadow-[0_0_15px_rgba(34,211,238,0.3)] hover:border-cyan-500/50"
    : theme === 'dev'
    ? "bg-green-950/50 text-green-400 border-green-800/50 hover:bg-green-900/80 hover:shadow-[0_0_15px_rgba(34,197,94,0.3)] hover:border-green-500/50"
    : "bg-orange-50 text-orange-600 border-orange-200 hover:bg-orange-100 hover:shadow-[0_0_15px_rgba(255,127,80,0.2)] hover:border-orange-400";
  const spinnerClass = theme === 'tech' ? "text-cyan-400 drop-shadow-[0_0_12px_rgba(34,211,238,0.8)]" : theme === 'dev' ? "text-green-400 drop-shadow-[0_0_12px_rgba(34,197,94,0.8)]" : "text-orange-500 drop-shadow-[0_0_12px_rgba(255,127,80,0.5)]";
  const checkClass = theme === 'tech' ? "text-cyan-400 drop-shadow-[0_0_15px_rgba(34,211,238,0.8)]" : theme === 'dev' ? "text-green-400 drop-shadow-[0_0_15px_rgba(34,197,94,0.8)]" : "text-orange-500 drop-shadow-[0_0_15px_rgba(255,127,80,0.6)]";
  
  const renderContent = () => {
    switch (state) {
      case 'wifi_ap':
        return (
          <div className="flex-1 flex flex-col items-center justify-center p-4 gap-3">
            <div className={`p-2 rounded-lg ${qrBoxClass}`}>
              <QrCode size={80} className="text-black" />
            </div>
            <div className="text-center space-y-1">
              <p className={`text-xs font-bold tracking-wider ${textTitleClass}`}>扫码配网</p>
              <p className={`text-[10px] ${textSubClass}`}>热点: Robot_AP_1234</p>
            </div>
            <button 
              onClick={() => navigateTo('wifi_connecting')}
              className={`mt-2 px-6 py-2 rounded-full text-xs font-medium border transition-all tracking-wider ${btnClass}`}
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
            <Loader2 size={36} className={`animate-spin ${spinnerClass}`} />
            <span className={`text-xs font-medium animate-pulse tracking-widest ${textTitleClass}`}>正在连接...</span>
          </div>
        );

      case 'wifi_success':
        return (
          <div className="flex-1 flex flex-col items-center justify-center gap-4">
            <motion.div initial={{ scale: 0 }} animate={{ scale: 1 }} transition={{ type: "spring", bounce: 0.5 }}>
              <CheckCircle2 size={48} className={checkClass} />
            </motion.div>
            <span className={`text-xs font-medium tracking-widest ${textTitleClass}`}>连接成功</span>
            <button 
              onClick={() => navigateTo('idle')}
              className={`mt-4 px-8 py-2 rounded-full text-xs font-medium border transition-all tracking-wider ${btnClass}`}
            >
              完成
            </button>
          </div>
        );

      case 'wifi_error':
        return (
          <div className="flex-1 flex flex-col items-center justify-center gap-4">
            <motion.div initial={{ scale: 0 }} animate={{ scale: 1 }} transition={{ type: "spring", bounce: 0.5 }}>
              <XCircle size={48} className="text-rose-500 drop-shadow-[0_0_15px_rgba(244,63,94,0.6)]" />
            </motion.div>
            <span className="text-xs font-medium text-rose-500 tracking-widest">连接失败</span>
            <div className="flex gap-4 mt-4">
              <button 
                onClick={() => navigateTo('wifi_ap')}
                className={`px-6 py-2 rounded-full text-xs font-medium border transition-all tracking-wider ${theme === 'tech' ? 'bg-black/50 text-cyan-600 border-cyan-900/50 hover:bg-cyan-950/50' : theme === 'dev' ? 'bg-black/50 text-green-600 border-green-900/50 hover:bg-green-950/50' : 'bg-white/50 text-orange-500 border-orange-200 hover:bg-orange-50'}`}
              >
                返回
              </button>
              <button 
                onClick={() => navigateTo('wifi_connecting')}
                className="px-6 py-2 rounded-full text-xs font-medium bg-rose-50 text-rose-500 border border-rose-200 hover:bg-rose-100 hover:shadow-[0_0_15px_rgba(244,63,94,0.2)] transition-all tracking-wider"
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
      className={`absolute inset-0 z-20 flex flex-col backdrop-blur-md ${overlayClass}`}
    >
      <button 
        onClick={() => navigateTo('settings')}
        className={`absolute top-4 left-4 p-1.5 rounded-full transition-all border z-30 ${backBtnClass}`}
      >
        <ChevronLeft size={20} />
      </button>
      
      {renderContent()}
    </motion.div>
  );
};
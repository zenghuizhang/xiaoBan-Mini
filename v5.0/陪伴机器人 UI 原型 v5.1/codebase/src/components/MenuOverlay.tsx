import React from 'react';
import { motion } from 'framer-motion';
import { Settings, Power, X, Wifi, Palette, Wind } from 'lucide-react';
import { useNavigate, useSearchParams } from 'react-router-dom';

interface MenuOverlayProps {
  theme: 'tech' | 'child' | 'dev';
}

export const MenuOverlay: React.FC<MenuOverlayProps> = ({ theme }) => {
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();
  const barQuery = searchParams.get('bar') === '1' ? '&bar=1' : '';

  const navigateTo = (newState: string, newTheme: string = theme) => {
    navigate(`/?state=${newState}&theme=${newTheme}${barQuery}`);
  };

  const menuItems = [
    { id: 'wifi', icon: Wifi, label: '网络设置', action: () => navigateTo('wifi_ap') },
    { id: 'theme', icon: Palette, label: '切换主题', action: () => navigateTo('menu', theme === 'tech' ? 'child' : theme === 'child' ? 'dev' : 'tech') },
    { id: 'breath', icon: Wind, label: '呼吸模式', action: () => navigateTo('breath') },
    { id: 'boot', icon: Power, label: '重启系统', action: () => navigateTo('boot') },
    { id: 'settings', icon: Settings, label: '系统设置', action: () => {} },
  ];

  let overlayClass = "bg-black/80";
  let closeBtnClass = "text-cyan-600/70 hover:text-cyan-400 hover:bg-cyan-950/50 border-transparent hover:border-cyan-900/50";
  let itemClass = "bg-cyan-950/20 text-cyan-500 active:bg-cyan-900/40 border-cyan-900/30 hover:border-cyan-500/50 hover:text-cyan-300 hover:shadow-[0_0_15px_rgba(34,211,238,0.2)]";
  let iconClass = "drop-shadow-[0_0_8px_rgba(34,211,238,0.5)]";

  if (theme === 'child') {
    overlayClass = "bg-[#FFF9E6]/90";
    closeBtnClass = "text-orange-400 hover:text-orange-600 hover:bg-orange-100 border-transparent hover:border-orange-200";
    itemClass = "bg-white/60 text-orange-600 active:bg-orange-100 border-orange-200 hover:border-orange-400 hover:text-orange-600 hover:shadow-[0_0_15px_rgba(255,127,80,0.3)]";
    iconClass = "drop-shadow-[0_0_8px_rgba(255,127,80,0.4)]";
  } else if (theme === 'dev') {
    overlayClass = "bg-black/90";
    closeBtnClass = "text-green-600/70 hover:text-green-400 hover:bg-green-950/50 border-transparent hover:border-green-900/50";
    itemClass = "bg-green-950/20 text-green-500 active:bg-green-900/40 border-green-900/30 hover:border-green-500/50 hover:text-green-300 hover:shadow-[0_0_15px_rgba(34,197,94,0.2)]";
    iconClass = "drop-shadow-[0_0_8px_rgba(34,197,94,0.5)]";
  }

  return (
    <motion.div 
      initial={{ opacity: 0, scale: 0.95 }}
      animate={{ opacity: 1, scale: 1 }}
      exit={{ opacity: 0, scale: 0.95 }}
      transition={{ type: 'spring', damping: 25, stiffness: 200 }}
      className={`absolute inset-0 backdrop-blur-md flex flex-col p-4 z-10 ${overlayClass}`}
    >
      <button 
        onClick={() => navigateTo('idle')}
        className={`absolute top-4 right-4 p-1.5 rounded-full transition-all border ${closeBtnClass}`}
      >
        <X size={18} />
      </button>

      <div className="flex-1 flex items-center justify-center pt-4">
        <div className="grid grid-cols-2 gap-3 w-full max-w-[280px]">
          {menuItems.map((item) => {
            const Icon = item.icon;
            return (
              <button 
                key={item.id}
                onClick={item.action}
                className={`flex items-center gap-2 p-2.5 rounded-lg transition-all border ${itemClass} ${item.id === 'settings' ? 'col-span-2 justify-center' : ''}`}
              >
                <Icon size={16} className={`shrink-0 ${iconClass}`} />
                <span className="text-xs font-medium tracking-widest">{item.label}</span>
              </button>
            );
          })}
        </div>
      </div>
    </motion.div>
  );
};
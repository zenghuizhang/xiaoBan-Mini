import React, { useState } from 'react';
import { motion } from 'framer-motion';
import { X, Sunrise, Frown, Compass, Wind, Zap, RotateCcw, MoveHorizontal, MoveVertical, Brain, SlidersHorizontal, Award } from 'lucide-react';
import { useNavigate } from 'react-router-dom';
import { playTone, vibrate } from '../lib/feedback';

interface ScenarioOverlayProps {
  theme: 'tech' | 'child' | 'dev';
}

export const ScenarioOverlay: React.FC<ScenarioOverlayProps> = ({ theme }) => {
  const navigate = useNavigate();
  const [activeTab, setActiveTab] = useState<'imu' | 'scenarios' | 'memory'>('imu');

  const navigateTo = (newState: string) => {
    playTone(1000, 50, 0.1);
    vibrate([20]);
    navigate(`/?state=${newState}&theme=${theme}`);
  };

  const overlayClass = theme === 'tech' ? "bg-black/90" : theme === 'dev' ? "bg-black/90" : "bg-[#FFF9E6]/95";
  const textClass = theme === 'tech' ? 'text-cyan-500' : theme === 'dev' ? 'text-green-500' : 'text-orange-500';
  const closeBtnClass = theme === 'tech'
    ? "text-cyan-600/70 hover:text-cyan-400 hover:bg-cyan-950/50 border-cyan-900/50"
    : theme === 'dev'
    ? "text-green-600/70 hover:text-green-400 hover:bg-green-950/50 border-green-900/50"
    : "text-orange-400 hover:text-orange-600 hover:bg-orange-100 border-orange-200";
  const itemClass = theme === 'tech'
    ? "bg-cyan-950/30 text-cyan-400 border-cyan-900/40 hover:border-cyan-500/60 hover:text-cyan-300"
    : theme === 'dev'
    ? "bg-green-950/30 text-green-400 border-green-900/40 hover:border-green-500/60 hover:text-green-300"
    : "bg-white/70 text-orange-600 border-orange-200 hover:border-orange-400 hover:text-orange-600";
  const activeTabClass = theme === 'tech' ? "bg-cyan-900/50 text-cyan-300" : theme === 'dev' ? "bg-green-900/50 text-green-300" : "bg-orange-200 text-orange-700";

  return (
    <motion.div 
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      exit={{ opacity: 0, y: 20 }}
      transition={{ type: 'spring', damping: 25, stiffness: 200 }}
      className={`absolute inset-0 backdrop-blur-md flex flex-col p-3 z-30 ${overlayClass}`}
    >
      <div className="flex justify-between items-center mb-2 px-1">
        <h2 className={`text-xs font-bold tracking-widest ${textClass}`}>
          开发者工具台
        </h2>
        <button 
          onClick={() => navigateTo('menu')}
          className={`p-1.5 rounded-full transition-all border ${closeBtnClass}`}
        >
          <X size={14} />
        </button>
      </div>

      <div className="flex gap-2 mb-3 px-1">
        {[
          { id: 'imu', label: '体感测试', icon: Compass },
          { id: 'scenarios', label: '场景模拟', icon: SlidersHorizontal },
          { id: 'memory', label: '记忆数据', icon: Brain }
        ].map(t => (
          <button
            key={t.id}
            onClick={() => setActiveTab(t.id as any)}
            className={`flex-1 flex items-center justify-center gap-1 py-1.5 rounded text-[10px] border border-transparent transition-colors ${activeTab === t.id ? activeTabClass : itemClass}`}
          >
            <t.icon size={10} />
            {t.label}
          </button>
        ))}
      </div>

      <div className="flex-1 overflow-y-auto scrollbar-hide pb-4 px-1" style={{ scrollbarWidth: 'none', msOverflowStyle: 'none' }}>
        <style>{`.scrollbar-hide::-webkit-scrollbar { display: none; }`}</style>
        
        {activeTab === 'imu' && (
          <div className="space-y-3">
            <div className="grid grid-cols-2 gap-1.5">
              <button onClick={() => navigateTo('curious')} className={`flex items-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><MoveVertical size={12} /> <span className="text-[9px]">前倾 (Pitch &gt; 15)</span></button>
              <button onClick={() => navigateTo('yawn')} className={`flex items-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><MoveVertical size={12} /> <span className="text-[9px]">后倾 (Pitch &lt; -15)</span></button>
              <button onClick={() => navigateTo('look_right')} className={`flex items-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><MoveHorizontal size={12} /> <span className="text-[9px]">左倾 (Roll &gt; 15)</span></button>
              <button onClick={() => navigateTo('look_left')} className={`flex items-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><MoveHorizontal size={12} /> <span className="text-[9px]">右倾 (Roll &lt; -15)</span></button>
              <button onClick={() => navigateTo('dizzy')} className={`flex items-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><Zap size={12} /> <span className="text-[9px]">摇晃 (Shake)</span></button>
              <button onClick={() => navigateTo('naughty')} className={`flex items-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><RotateCcw size={12} /> <span className="text-[9px]">倒立 (Gyro Z)</span></button>
              <button onClick={() => navigateTo('wink')} className={`col-span-2 flex items-center justify-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><Zap size={12} /> <span className="text-[9px]">轻敲 (Tap Z &gt; 1.5g) - Wink</span></button>
            </div>
          </div>
        )}

        {activeTab === 'scenarios' && (
          <div className="space-y-3">
            <div>
              <h3 className={`text-[9px] mb-1.5 opacity-70 ${textClass}`}>呼吸模式</h3>
              <div className="grid grid-cols-2 gap-1.5">
                <button onClick={() => navigateTo('deep_sleep')} className={`flex items-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><Wind size={12} /> <span className="text-[9px]">深睡 (8s)</span></button>
                <button onClick={() => navigateTo('light_rest')} className={`flex items-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><Wind size={12} /> <span className="text-[9px]">浅休 (6s)</span></button>
                <button onClick={() => navigateTo('breath')} className={`flex items-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><Wind size={12} /> <span className="text-[9px]">待机 (4s)</span></button>
                <button onClick={() => navigateTo('alert')} className={`flex items-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><Zap size={12} /> <span className="text-[9px]">警觉 (2.5s)</span></button>
              </div>
            </div>
            <div>
              <h3 className={`text-[9px] mb-1.5 opacity-70 ${textClass}`}>特定事件</h3>
              <div className="grid grid-cols-2 gap-1.5">
                <button onClick={() => navigateTo('morning')} className={`flex items-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><Sunrise size={12} /> <span className="text-[9px]">早安问候</span></button>
                <button onClick={() => navigateTo('reward')} className={`flex items-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><Award size={12} /> <span className="text-[9px]">互动奖励</span></button>
                <button onClick={() => navigateTo('lonely_3')} className={`flex items-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><Frown size={12} /> <span className="text-[9px]">寂寞超时</span></button>
                <button onClick={() => navigateTo('angry')} className={`flex items-center gap-1.5 px-2 py-2 rounded-md transition-all border ${itemClass}`}><Frown size={12} /> <span className="text-[9px]">粗暴对待</span></button>
              </div>
            </div>
          </div>
        )}

        {activeTab === 'memory' && (
          <div className={`p-2 rounded border text-[9px] font-mono leading-relaxed ${itemClass}`}>
            <div className="mb-2"><strong className="opacity-70">Lv3 朋友</strong> (连续7天交互)</div>
            <div><span className="opacity-50">活跃时间:</span> [08:00, 20:00, 21:00]</div>
            <div><span className="opacity-50">最爱表情:</span> happy(15), curious(8)</div>
            <div><span className="opacity-50">偏好主题:</span> {theme}</div>
            <div><span className="opacity-50">交互总数:</span> 120 次 (450分钟)</div>
            <div className="mt-2"><span className="opacity-50">最近情绪记录:</span></div>
            <ul className="pl-2 border-l border-current/20 ml-1 space-y-1">
              <li>10:30 - curious (前倾触发)</li>
              <li>08:05 - happy (早安问候)</li>
            </ul>
          </div>
        )}
      </div>
    </motion.div>
  );
};
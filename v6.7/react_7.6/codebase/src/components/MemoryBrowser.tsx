import React from 'react';
import { ChevronLeft, MessageSquare, Settings, Briefcase, Activity, Globe, Trash2 } from 'lucide-react';
import { useNavigate } from 'react-router-dom';
import { playTone, vibrate } from '../lib/feedback';
import { TOKENS_TABLE, ThemeName } from '../theme/tokens';

interface MemoryBrowserProps {
  purgeModal?: boolean;
  theme: string;
}

export const MemoryBrowser: React.FC<MemoryBrowserProps> = ({ purgeModal, theme }) => {
  const navigate = useNavigate();
  const t = TOKENS_TABLE[theme as ThemeName] || TOKENS_TABLE.tech;

  const memories = [
    { icon: MessageSquare, text: '用户喜欢喝美式咖啡', time: '2 小时前', type: 'chat' },
    { icon: Settings, text: '主题=tech', time: '3 天前', type: 'settings' },
    { icon: Briefcase, text: 'pomodoro 已安装', time: '1 周前', type: 'skills' },
    { icon: Activity, text: '用户偏好早晨活跃', time: '2 周前', type: 'usage' },
    { icon: Globe, text: '家庭 Wi-Fi 自动连接', time: '1 个月前', type: 'net' },
    { icon: MessageSquare, text: '不爱吃香菜', time: '1 个月前', type: 'chat' },
  ];

  const navigateTo = (state: string) => {
    playTone(1000, 50, 0.1);
    vibrate([20]);
    navigate(`/?state=${state}&theme=${theme}`);
  };

  return (
    <div className="absolute inset-0 z-30 flex flex-col" style={{ backgroundColor: t.bg }}>
      <div className="h-[28px] flex items-center justify-between px-2 border-b shrink-0 mt-[22px]" style={{ borderColor: t.border }}>
        <div className="flex items-center">
          <button onClick={() => navigateTo('menu')} className="flex items-center" style={{ color: t.accent }}>
            <ChevronLeft size={20} strokeWidth={2.4} />
          </button>
          <span className="text-[16px] font-bold ml-1" style={{ color: t.text }}>长程记忆</span>
        </div>
        <button 
          onClick={() => navigateTo('memory_purge')}
          className="text-[12px] px-2 py-1 border rounded-[4px]"
          style={{ color: t.text, backgroundColor: t.panel, borderColor: t.border }}
        >
          清空
        </button>
      </div>
      <div className="flex-1 overflow-y-auto scrollbar-hide" style={{ scrollbarWidth: 'none', msOverflowStyle: 'none' }}>
        <style>{`.scrollbar-hide::-webkit-scrollbar { display: none; }`}</style>
        {memories.map((m, i) => (
          <div key={i} className="flex items-center px-2 h-[56px] border-b" style={{ borderColor: t.border }}>
            <div className="w-[32px] shrink-0 flex items-center justify-center">
              <m.icon size={20} strokeWidth={2.4} style={{ color: t.accent }} />
            </div>
            <div className="flex-1 min-w-0 px-1 flex flex-col justify-center">
              <span className="text-[14px] leading-tight truncate" style={{ color: t.text }}>{m.type} · 「{m.text}」</span>
              <span className="text-[12px] mt-[2px]" style={{ color: `${t.text}80` }}>{m.time}</span>
            </div>
            <button className="w-[32px] shrink-0 flex items-center justify-center hover:opacity-80" style={{ color: t.danger }}>
              <Trash2 size={16} strokeWidth={2.4} />
            </button>
          </div>
        ))}
      </div>

      {purgeModal && (
        <div className="absolute inset-0 bg-black/60 z-40 flex items-end">
          <div className="w-full rounded-t-[8px] p-4 border-t" style={{ backgroundColor: t.panel, borderColor: t.border }}>
            <h3 className="text-[16px] font-bold mb-1" style={{ color: t.text }}>清空所有长程记忆?</h3>
            <p className="text-[12px] leading-snug mb-4" style={{ color: `${t.text}80` }}>
              这只清除记忆里的对话偏好/学习数据，不影响设置和 Wi-Fi 状态
            </p>
            <div className="flex gap-2">
              <button 
                onClick={() => navigateTo('memory')}
                className="flex-1 h-[28px] rounded-[4px] border text-[14px] font-bold flex items-center justify-center transition-opacity hover:opacity-80"
                style={{ color: t.accent, borderColor: t.accent }}
              >
                取消
              </button>
              <button 
                onClick={() => navigateTo('memory')}
                className="flex-1 h-[28px] rounded-[4px] text-white text-[14px] font-bold flex items-center justify-center transition-opacity hover:opacity-80"
                style={{ backgroundColor: t.danger }}
              >
                全部清空
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
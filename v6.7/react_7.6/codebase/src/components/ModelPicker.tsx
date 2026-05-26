import React from 'react';
import { ChevronLeft } from 'lucide-react';
import { useNavigate } from 'react-router-dom';
import { playTone, vibrate } from '../lib/feedback';
import { TOKENS_TABLE, ThemeName } from '../theme/tokens';

export const ModelPicker: React.FC<{ theme: string }> = ({ theme }) => {
  const navigate = useNavigate();
  const t = TOKENS_TABLE[theme as ThemeName] || TOKENS_TABLE.tech;

  const models = [
    { id: 'auto', label: 'ⓘ Auto (智能路由)', sub: '', selected: true },
    { id: 'gpt4o', label: '💎 GPT-4o', sub: '云端 · 专业版', selected: false },
    { id: 'claude35', label: '🎭 Claude 3.5 Sonnet', sub: '云端 · 专业版', selected: false },
    { id: 'doubao', label: '🚀 Doubao Pro', sub: '云端 · 标准版', selected: false },
    { id: 'qwen', label: '🦾 Qwen-2.5 1.5B', sub: '本地 · 极速版', selected: false },
    { id: 'phi3', label: '📱 Phi-3 mini', sub: '本地 · 极速版', selected: false },
  ];

  const navigateTo = (state: string) => {
    playTone(1000, 50, 0.1);
    vibrate([20]);
    navigate(`/?state=${state}&theme=${theme}`);
  };

  return (
    <div className="absolute inset-0 z-30 flex flex-col" style={{ backgroundColor: t.bg }}>
      <div className="h-[28px] flex items-center px-2 border-b shrink-0 mt-[22px]" style={{ borderColor: t.border }}>
        <button onClick={() => navigateTo('menu')} className="flex items-center" style={{ color: t.accent }}>
          <ChevronLeft size={20} strokeWidth={2.4} />
        </button>
        <span className="text-[16px] font-bold ml-1" style={{ color: t.text }}>大模型选择</span>
      </div>
      <div className="flex-1 overflow-y-auto px-2 py-2 space-y-[2px] scrollbar-hide" style={{ scrollbarWidth: 'none', msOverflowStyle: 'none' }}>
        <style>{`.scrollbar-hide::-webkit-scrollbar { display: none; }`}</style>
        {models.map(m => (
          <div key={m.id} className="flex items-center h-[36px] px-2 rounded-[4px] border" style={{ backgroundColor: theme === 'tech' ? `${t.panel}4D` : t.panel, borderColor: t.border }}>
            {m.selected ? (
              <div className="w-3 h-3 rounded-full mr-2 shrink-0 flex items-center justify-center" style={{ backgroundColor: t.accent }}>
                <div className="w-1.5 h-1.5 rounded-full" style={{ backgroundColor: t.bg }}></div>
              </div>
            ) : (
              <div className="w-3 h-3 rounded-full border mr-2 shrink-0" style={{ borderColor: t.border }}></div>
            )}
            <div className="flex flex-col justify-center min-w-0 pb-[2px]">
              <span className="text-[14px] leading-none truncate" style={{ color: t.text }}>
                {m.label}
              </span>
              {m.sub && <span className="text-[11px] leading-none mt-[2px]" style={{ color: `${t.text}80` }}>{m.sub}</span>}
            </div>
            {m.selected && <span className="ml-auto text-[12px]" style={{ color: t.accent }}>✓</span>}
          </div>
        ))}
      </div>
      <div className="h-[28px] shrink-0 flex items-center justify-center border-t" style={{ backgroundColor: theme === 'tech' ? `${t.panel}4D` : t.panel, borderColor: t.border }}>
        <span className="text-[11px]" style={{ color: `${t.text}80` }}>切换会立即生效,正在进行的对话不打断</span>
      </div>
    </div>
  );
};
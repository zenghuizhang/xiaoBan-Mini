import React from 'react';
import { ChevronLeft } from 'lucide-react';
import { useNavigate } from 'react-router-dom';
import { playTone, vibrate } from '../lib/feedback';
import { TOKENS_TABLE, ThemeName } from '../theme/tokens';

export const PersonaGrid: React.FC<{ theme: string }> = ({ theme }) => {
  const navigate = useNavigate();
  const t = TOKENS_TABLE[theme as ThemeName] || TOKENS_TABLE.tech;

  const personas = [
    { emoji: '🎵', name: 'Lyra', desc: '温柔诗人', selected: true },
    { emoji: '🪞', name: 'Echo', desc: '话痨复读机', selected: false },
    { emoji: '🌟', name: 'Nova', desc: '极客科普', selected: false },
    { emoji: '🦉', name: 'Sage', desc: '冷静顾问', selected: false },
    { emoji: '🐣', name: 'Pico', desc: '童趣小鸡', selected: false },
    { emoji: '🩺', name: 'Doc', desc: '严谨医师', selected: false },
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
        <span className="text-[16px] font-bold ml-1" style={{ color: t.text }}>人格配置</span>
      </div>
      <div className="flex-1 overflow-y-auto px-[8px] py-2 scrollbar-hide flex justify-center" style={{ scrollbarWidth: 'none', msOverflowStyle: 'none' }}>
        <style>{`.scrollbar-hide::-webkit-scrollbar { display: none; }`}</style>
        <div className="grid grid-cols-3 gap-[8px] w-full max-w-[304px]">
          {personas.map((p, i) => (
            <div 
              key={i} 
              className="w-[96px] h-[84px] rounded-[4px] flex flex-col items-center justify-center gap-1 transition-all"
              style={{
                backgroundColor: theme === 'tech' ? `${t.panel}4D` : t.panel,
                borderColor: p.selected ? t.accent : t.border,
                borderWidth: p.selected ? '2px' : '1px',
                borderStyle: 'solid'
              }}
            >
              <span className="text-[24px] leading-none">{p.emoji}</span>
              <span className="text-[14px] font-bold" style={{ color: p.selected ? t.accent : t.text }}>{p.name}</span>
              <span className="text-[11px] leading-none" style={{ color: `${t.text}99` }}>「{p.desc}」</span>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};
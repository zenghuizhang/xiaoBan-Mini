import React from 'react';
import { ChevronLeft } from 'lucide-react';
import { useNavigate } from 'react-router-dom';
import { playTone, vibrate } from '../lib/feedback';
import { TOKENS_TABLE, ThemeName } from '../theme/tokens';

export const ThemePicker: React.FC<{ theme: string }> = ({ theme }) => {
  const navigate = useNavigate();
  const t = TOKENS_TABLE[theme as ThemeName] || TOKENS_TABLE.tech;

  const THEMES = [
    { id: 'tech',     name: '科技青', desc: 'Tech', dot: '#22D3EE' },
    { id: 'lavender', name: '柔紫',   desc: 'Lavender', dot: '#9333EA' },
    { id: 'child',    name: '温暖儿童', desc: 'Child', dot: '#FF7F50' },
    { id: 'cocoa',    name: '草莓可可', desc: 'Cocoa', dot: '#FB7185' },
  ];

  const navigateTo = (state: string, newTheme?: string) => {
    playTone(1000, 50, 0.1);
    vibrate([20]);
    navigate(`/?state=${state}&theme=${newTheme || theme}`);
  };

  return (
    <div className="absolute inset-0 z-30 flex flex-col" style={{ backgroundColor: t.bg }}>
      <div className="h-[28px] flex items-center px-2 border-b shrink-0 mt-[22px]" style={{ borderColor: t.border }}>
        <button onClick={() => navigateTo('menu')} className="flex items-center" style={{ color: t.accent }}>
          <ChevronLeft size={20} strokeWidth={2.4} />
        </button>
        <span className="text-[16px] font-bold ml-1" style={{ color: t.text }}>主题风格</span>
      </div>
      <div className="flex-1 overflow-y-auto px-[8px] py-2 scrollbar-hide flex flex-col items-center" style={{ scrollbarWidth: 'none', msOverflowStyle: 'none' }}>
        <style>{`.scrollbar-hide::-webkit-scrollbar { display: none; }`}</style>
        <div className="grid grid-cols-2 gap-[8px] w-full max-w-[304px]">
          {THEMES.map((th) => {
            const isSelected = theme === th.id;
            return (
              <button 
                key={th.id}
                onClick={() => navigateTo('theme_picker', th.id)}
                className="w-full h-[64px] rounded-[4px] flex flex-col items-center justify-center gap-1 transition-all"
                style={{
                  backgroundColor: theme === 'tech' ? `${t.panel}4D` : t.panel,
                  borderColor: isSelected ? t.accent : t.border,
                  borderWidth: isSelected ? '2px' : '1px',
                  borderStyle: 'solid'
                }}
              >
                <div className="flex items-center gap-2">
                  <div className="w-3 h-3 rounded-full shadow-sm" style={{ backgroundColor: th.dot }} />
                  <span className="text-[14px] font-bold" style={{ color: isSelected ? t.accent : t.text }}>{th.name}</span>
                </div>
                <span className="text-[11px] leading-none" style={{ color: `${t.text}99` }}>{th.desc}</span>
              </button>
            );
          })}
        </div>
      </div>
    </div>
  );
};
import React from 'react';
import { ChevronLeft, Package } from 'lucide-react';
import { useNavigate } from 'react-router-dom';
import { TOKENS_TABLE, ThemeName } from '../theme/tokens';

export const Skills: React.FC<{ theme: string }> = ({ theme }) => {
  const navigate = useNavigate();
  const t = TOKENS_TABLE[theme as ThemeName] || TOKENS_TABLE.tech;

  const navigateTo = (state: string) => {
    navigate(`/?state=${state}&theme=${theme}`);
  };

  return (
    <div className="absolute inset-0 z-30 flex flex-col" style={{ backgroundColor: t.bg }}>
      <div className="h-[28px] flex items-center px-2 border-b shrink-0 mt-[22px]" style={{ borderColor: t.border }}>
        <button onClick={() => navigateTo('settings')} className="flex items-center" style={{ color: t.accent }}>
          <ChevronLeft size={20} strokeWidth={2.4} />
        </button>
        <span className="text-[16px] font-bold ml-1" style={{ color: t.text }}>技能插件</span>
      </div>
      <div className="flex-1 flex flex-col items-center justify-center gap-2">
        <Package size={32} style={{ color: t.accent, opacity: 0.5 }} />
        <span className="text-[12px]" style={{ color: `${t.text}80` }}>暂无可用技能</span>
      </div>
    </div>
  );
};
import React from 'react';
import { Menu, Mic, Smile, Frown, Dna, Zap, Shuffle, Eye, Wind, Sunrise, Compass, Moon, HeartCrack, PartyPopper, Sparkles, Flame, HelpCircle, MessageSquarePlus } from 'lucide-react';
import { useNavigate, useSearchParams } from 'react-router-dom';
import { cn } from '../lib/utils';

interface BottomBarProps {
  currentState: string;
  theme: 'tech' | 'child' | 'dev';
}

export const BottomBar: React.FC<BottomBarProps> = ({ currentState, theme }) => {
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();
  const barQuery = searchParams.get('bar') === '1' ? '&bar=1' : '';

  const navigateState = (state: string, extraParams: string = '') => {
    navigate(`/?state=${state}&theme=${theme}${barQuery}${extraParams}`);
  };

  const buttons = [
    { id: 'env', icon: Wind, label: '环境', action: () => navigateState(currentState, '&env_panel=1') },
    { id: 'menu', icon: Menu, label: '菜单', action: () => navigateState('menu') },
    { id: 'morning', icon: Sunrise, label: '早安', action: () => navigateState('morning', '&dialog=morning') },
    { id: 'suggest', icon: MessageSquarePlus, label: '建议', action: () => navigateState('idle', '&dialog=suggest') },
    { id: 'sleep', icon: Moon, label: '休息', action: () => navigateState('yawn', '&dialog=sleep') },
    { id: 'look_around', icon: Compass, label: '寂寞', action: () => navigateState('look_around') },
    { id: 'yawn', icon: Moon, label: '哈欠', action: () => navigateState('yawn') },
    { id: 'sad', icon: HeartCrack, label: '失落', action: () => navigateState('sad') },
    { id: 'celebrate', icon: PartyPopper, label: '庆祝', action: () => navigateState('celebrate') },
    { id: 'excited', icon: Sparkles, label: '兴奋', action: () => navigateState('excited') },
    { id: 'angry', icon: Flame, label: '生气', action: () => navigateState('angry') },
    { id: 'curious', icon: HelpCircle, label: '好奇', action: () => navigateState('curious') },
    { id: 'breath', icon: Wind, label: '呼吸', action: () => navigateState('breath') },
    { id: 'random', icon: Shuffle, label: '随机', action: () => navigateState('random') },
    { id: 'talking', icon: Mic, label: '对话', action: () => navigateState('talking') },
    { id: 'happy', icon: Smile, label: '开心', action: () => navigateState('happy') },
    { id: 'wink', icon: Eye, label: '眨眼', action: () => navigateState('wink') },
    { id: 'naughty', icon: Dna, label: '调皮', action: () => navigateState('naughty') },
    { id: 'dizzy', icon: Zap, label: '眩晕', action: () => navigateState('dizzy') },
    { id: 'crying', icon: Frown, label: '哭泣', action: () => navigateState('crying') },
  ];

  let containerClass = "bg-black/80 border-cyan-900/30";
  if (theme === 'child') {
    containerClass = "bg-[#FFF9E6]/90 border-orange-200/50 shadow-[0_-5px_15px_rgba(255,127,80,0.05)]";
  } else if (theme === 'dev') {
    containerClass = "bg-black/90 border-green-900/30";
  }

  const getBtnClass = (isActive: boolean) => {
    if (theme === 'child') {
      return isActive
        ? "bg-orange-100 text-orange-600 shadow-[0_0_10px_rgba(255,127,80,0.15)] border border-orange-300"
        : "text-orange-400/80 hover:bg-orange-50 hover:text-orange-500 border border-transparent";
    } else if (theme === 'dev') {
      return isActive 
        ? "bg-green-900/50 text-green-300 shadow-[0_0_10px_rgba(34,197,94,0.3)] border border-green-500/30" 
        : "text-green-600/70 hover:bg-green-950/40 hover:text-green-400 border border-transparent";
    } else {
      return isActive 
        ? "bg-cyan-900/50 text-cyan-300 shadow-[0_0_10px_rgba(34,211,238,0.3)] border border-cyan-500/30" 
        : "text-cyan-600/70 hover:bg-cyan-950/40 hover:text-cyan-400 border border-transparent";
    }
  };

  return (
    <div className={`h-14 w-full flex items-center px-2 shrink-0 backdrop-blur-md border-t overflow-x-auto gap-2 scrollbar-hide ${containerClass}`} style={{ scrollbarWidth: 'none', msOverflowStyle: 'none' }}>
      <style>{`
        .scrollbar-hide::-webkit-scrollbar {
            display: none;
        }
      `}</style>
      {buttons.map((btn) => {
        const Icon = btn.icon;
        const isActive = currentState === btn.id || (btn.id === 'suggest' && searchParams.get('dialog') === 'suggest') || (btn.id === 'sleep' && searchParams.get('dialog') === 'sleep');
        
        return (
          <button
            key={btn.id}
            onClick={(e) => {
              e.stopPropagation();
              btn.action();
            }}
            className={cn(
              "flex flex-col items-center justify-center min-w-[54px] h-10 transition-all shrink-0 rounded-md border",
              getBtnClass(isActive)
            )}
          >
            <Icon size={16} className="mb-1" />
            <span className="text-[10px] font-medium">{btn.label}</span>
          </button>
        );
      })}
    </div>
  );
};
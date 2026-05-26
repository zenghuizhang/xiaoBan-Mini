import React from 'react';
import { ChevronLeft, Terminal, Sparkles, Brain, Palette, Smile, Shuffle, Bug, Briefcase, Send, Volume2, Download, Mic, DownloadCloud, AlertTriangle } from 'lucide-react';
import { useNavigate } from 'react-router-dom';
import { playTone, vibrate } from '../lib/feedback';
import { TOKENS_TABLE, ThemeName } from '../theme/tokens';

interface ConsoleProps {
  activeTab?: 'system' | 'ai' | 'display' | 'service';
  theme: string;
}

export const Console: React.FC<ConsoleProps> = ({ activeTab = 'ai', theme }) => {
  const navigate = useNavigate();
  const tabs = [
    { id: 'system', label: '系统状态' },
    { id: 'ai', label: 'AI引擎' },
    { id: 'display', label: '显示调试' },
    { id: 'service', label: '服务模拟' }
  ];

  const t = TOKENS_TABLE[theme as ThemeName] || TOKENS_TABLE.tech;

  const navigateTo = (state: string) => {
    playTone(1000, 50, 0.1);
    vibrate([20]);
    navigate(`/?state=${state}&theme=${theme}`);
  };

  const renderContent = () => {
    if (activeTab === 'ai') {
      const cards = [
        { icon: Terminal, title: '运行日志', desc: '查看最近运行日志' },
        { icon: Sparkles, title: '模型路由', desc: '强制指定大模型' },
        { icon: Brain, title: '记忆管理', desc: '列表 / 删除 / 清空' },
        { icon: Briefcase, title: '技能调试', desc: '调用及测试插件' }
      ];
      return (
        <div className="grid grid-cols-2 gap-2 p-2 w-[304px] mx-auto">
          {cards.map((c, i) => (
            <div key={i} className="w-[144px] h-[56px] rounded-[4px] border p-2 flex flex-col justify-center" style={{ backgroundColor: theme === 'tech' ? `${t.panel}4D` : t.panel, borderColor: t.border }}>
              <div className="flex items-center gap-1.5 mb-0.5">
                <c.icon size={14} strokeWidth={2.4} style={{ color: t.accent }} />
                <span className="text-[12px] font-bold truncate" style={{ color: t.text }}>{c.title}</span>
              </div>
              <span className="text-[10px] leading-none truncate" style={{ color: `${t.text}80` }}>{c.desc}</span>
            </div>
          ))}
        </div>
      );
    }

    if (activeTab === 'display') {
      const cards = [
        { icon: Palette, title: '主题配置', desc: '切换主题模式' },
        { icon: Smile, title: '静态表情', desc: '选择静态表情' },
        { icon: Shuffle, title: '动态表情', desc: '轮播表情模式' },
        { icon: Bug, title: '表情调试', desc: '自动遍历 26 帧' }
      ];
      return (
        <div className="grid grid-cols-2 gap-2 p-2 w-[304px] mx-auto">
          {cards.map((c, i) => (
            <div key={i} className="w-[144px] h-[56px] rounded-[4px] border p-2 flex flex-col justify-center" style={{ backgroundColor: theme === 'tech' ? `${t.panel}4D` : t.panel, borderColor: t.border }}>
              <div className="flex items-center gap-1.5 mb-0.5">
                <c.icon size={14} strokeWidth={2.4} style={{ color: t.accent }} />
                <span className="text-[12px] font-bold truncate" style={{ color: t.text }}>{c.title}</span>
              </div>
              <span className="text-[10px] leading-none truncate" style={{ color: `${t.text}80` }}>{c.desc}</span>
            </div>
          ))}
        </div>
      );
    }

    if (activeTab === 'system') {
      const fireSensorTest = (id: string) => {
        if (id === 'tilt_forward') navigateTo('curious');
        if (id === 'tilt_backward') navigateTo('yawn');
        if (id === 'tilt_left') navigateTo('look_left');
        if (id === 'tilt_right') navigateTo('look_right');
        if (id === 'shake') navigateTo('dizzy');
        if (id === 'rotate') navigateTo('thinking');
      };

      return (
        <div className="p-2 w-[304px] mx-auto pb-4">
          <div className="mb-2 p-2 rounded border" style={{ backgroundColor: theme === 'tech' ? `${t.panel}4D` : t.panel, borderColor: t.border }}>
             <h3 className="text-[11px] font-bold" style={{ color: t.text }}>传感器状态</h3>
             <div className="text-[10px] mt-1" style={{ color: `${t.text}80` }}>IMU: 正常<br/>光照: 420 lx</div>
          </div>
          
          <div className="text-[11px] font-bold mb-1 mt-2" style={{ color: t.text }}>体感测试</div>
          <div className="grid grid-cols-3 gap-2">
            {[
              { id: 'tilt_forward', label: '前倾' },
              { id: 'tilt_backward', label: '后仰' },
              { id: 'tilt_left', label: '左倾' },
              { id: 'tilt_right', label: '右倾' },
              { id: 'shake', label: '摇晃' },
              { id: 'rotate', label: '旋转' },
            ].map(b => (
              <button key={b.id}
                onClick={() => fireSensorTest(b.id)}
                className="h-[28px] rounded-[4px] text-[11px] font-medium transition-transform active:scale-95 border"
                style={{ backgroundColor: t.panel, borderColor: t.border, color: t.text }}>
                {b.label}
              </button>
            ))}
          </div>
        </div>
      );
    }
    
    if (activeTab === 'service') {
      const post = (evt: string, val: any) => console.log(evt, val);
      const sleep = (ms: number) => new Promise(r => setTimeout(r, ms));

      const SCENARIOS = [
        { id: 'voice_wake', label: '🎤 语音唤醒', fire: () => navigateTo('voice_wake') },
        { id: 'ota',     label: '🤖 OTA模拟', fire: async () => { navigateTo('ota'); for (const v of [10,30,60,90,100]) { post('OTA_PROGRESS', v); await sleep(200); } } },
        { id: 'error',   label: '📵 报错状态', fire: () => navigateTo('error') },
        { id: 'call',    label: '📱 语音通话', fire: () => { post('MCP_ACTIVE', 1); post('FACE_REQUEST', 5); navigateTo('talking'); } },
        { id: 'lowbat',  label: '🔋 低电量',   fire: () => { post('BATTERY', 10); post('FACE_REQUEST', 3); navigateTo('lost'); } },
        { id: 'hot',     label: '🌡 过热',     fire: () => { post('LOG_LINE', '[therm] CPU 72°C'); post('FACE_REQUEST', 7); navigateTo('dizzy'); } },
      ];

      return (
        <div className="p-2 space-y-2 w-[304px] mx-auto pb-4">
          <div className="text-[11px] font-bold mb-1" style={{ color: t.text }}>场景模拟</div>
          <div className="grid grid-cols-3 gap-2">
            {SCENARIOS.map(s => (
              <button key={s.id}
                onClick={s.fire}
                className="h-[32px] rounded-[4px] border text-[10px] font-medium transition-all flex items-center justify-center"
                style={{ backgroundColor: t.panel, borderColor: t.border, color: t.text }}
                onMouseDown={e => {
                  e.currentTarget.style.borderColor = t.accent;
                  e.currentTarget.style.color = t.accent;
                }}
                onMouseUp={e => {
                  e.currentTarget.style.borderColor = t.border;
                  e.currentTarget.style.color = t.text;
                }}
                onMouseLeave={e => {
                  e.currentTarget.style.borderColor = t.border;
                  e.currentTarget.style.color = t.text;
                }}
              >
                {s.label}
              </button>
            ))}
          </div>
        </div>
      );
    }
    
    return <div className="p-4 text-[12px] text-center" style={{ color: `${t.text}80` }}>标签内容 ({activeTab})</div>;
  };

  return (
    <div className="absolute inset-0 z-30 flex flex-col" style={{ backgroundColor: t.bg }}>
      <div className="h-[28px] flex items-center px-2 border-b shrink-0 mt-[22px]" style={{ borderColor: t.border }}>
        <button onClick={() => navigateTo('settings')} className="flex items-center" style={{ color: t.accent }}>
          <ChevronLeft size={20} strokeWidth={2.4} />
        </button>
        <span className="text-[16px] font-bold ml-1" style={{ color: t.text }}>开发者控制台</span>
      </div>
      
      <div className="flex border-b shrink-0 h-[32px]" style={{ borderColor: t.border }}>
        {tabs.map(tab => (
          <button 
            key={tab.id}
            onClick={() => navigateTo(`console_${tab.id}`)}
            className="flex-1 flex items-center justify-center text-[12px] font-bold border-b-2 transition-colors"
            style={{
              borderColor: activeTab === tab.id ? t.accent : 'transparent',
              color: activeTab === tab.id ? t.accent : `${t.text}80`
            }}
          >
            {tab.label}
          </button>
        ))}
      </div>

      <div className="flex-1 overflow-y-auto scrollbar-hide" style={{ scrollbarWidth: 'none', msOverflowStyle: 'none' }}>
        <style>{`.scrollbar-hide::-webkit-scrollbar { display: none; }`}</style>
        {renderContent()}
      </div>
    </div>
  );
};
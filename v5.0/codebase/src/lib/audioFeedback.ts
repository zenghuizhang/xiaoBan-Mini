export const playTone = (frequency: number, durationMs: number) => {
  try {
    const AudioContext = window.AudioContext || (window as any).webkitAudioContext;
    if (!AudioContext) return;
    const ctx = new AudioContext();
    const osc = ctx.createOscillator();
    const gainNode = ctx.createGain();
    
    osc.type = 'sine';
    osc.frequency.setValueAtTime(frequency, ctx.currentTime);
    
    // Smooth envelope to avoid clicks
    gainNode.gain.setValueAtTime(0.1, ctx.currentTime);
    gainNode.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + durationMs / 1000);
    
    osc.connect(gainNode);
    gainNode.connect(ctx.destination);
    
    osc.start();
    osc.stop(ctx.currentTime + durationMs / 1000);
  } catch (e) {
    console.error('Audio play failed', e);
  }
};

export const playAudioFeedback = (type: 'doubleClick' | 'menuOpen' | 'menuSelect' | 'motion') => {
  switch (type) {
    case 'doubleClick':
      playTone(800, 100);
      break;
    case 'menuOpen':
      playTone(600, 80);
      break;
    case 'menuSelect':
      playTone(1200, 80);
      break;
    case 'motion':
      playTone(500, 150);
      break;
  }
};
export const playHapticFeedback = (type: 'doubleClick' | 'menuSelect' | 'motion' | 'shake') => {
  if (!navigator.vibrate) return;
  try {
    switch (type) {
      case 'doubleClick':
        navigator.vibrate([50, 30, 50]);
        break;
      case 'menuSelect':
        navigator.vibrate([30]);
        break;
      case 'motion':
        navigator.vibrate([40, 20]);
        break;
      case 'shake':
        navigator.vibrate([100, 50, 100]);
        break;
    }
  } catch (e) {
    console.error('Haptic play failed', e);
  }
};
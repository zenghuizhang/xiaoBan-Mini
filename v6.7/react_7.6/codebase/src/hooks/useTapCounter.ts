import { useRef, useCallback } from 'react';

export function useTapCounter(targetCount: number, timeWindow: number, onUnlock: () => void) {
  const taps = useRef<number[]>([]);
  return useCallback(() => {
    const now = Date.now();
    taps.current.push(now);
    taps.current = taps.current.filter(t => now - t <= timeWindow);
    if (taps.current.length >= targetCount) {
      taps.current = [];
      onUnlock();
    }
  }, [targetCount, timeWindow, onUnlock]);
}
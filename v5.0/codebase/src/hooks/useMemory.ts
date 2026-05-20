import { useState, useEffect, useCallback } from 'react';

export interface MemoryData {
  activeHours: Record<number, number>;
  favoriteExpressions: Record<string, number>;
  consecutiveDays: number;
  totalInteractions: number;
  lastActiveDate: string | null;
}

export const useMemory = () => {
  const [memory, setMemory] = useState<MemoryData>(() => {
    const saved = localStorage.getItem('robot_memory');
    if (saved) {
      try {
        return JSON.parse(saved);
      } catch (e) {}
    }
    return {
      activeHours: {},
      favoriteExpressions: {},
      consecutiveDays: 0,
      totalInteractions: 0,
      lastActiveDate: null
    };
  });

  useEffect(() => {
    localStorage.setItem('robot_memory', JSON.stringify(memory));
  }, [memory]);

  const recordInteraction = useCallback((type: 'expression' | 'other', value?: string) => {
    setMemory(prev => {
      const now = new Date();
      const hour = now.getHours();
      const dateStr = now.toDateString();
      
      const newActiveHours = { ...prev.activeHours };
      newActiveHours[hour] = (newActiveHours[hour] || 0) + 1;
      
      const newFavs = { ...prev.favoriteExpressions };
      if (type === 'expression' && value) {
        newFavs[value] = (newFavs[value] || 0) + 1;
      }
      
      let newConsecutive = prev.consecutiveDays;
      if (prev.lastActiveDate !== dateStr) {
        const lastDate = prev.lastActiveDate ? new Date(prev.lastActiveDate) : null;
        if (lastDate) {
          const diff = now.getTime() - lastDate.getTime();
          if (diff <= 86400000 * 2 && diff > 86400000) {
            newConsecutive += 1;
          } else if (diff > 86400000 * 2) {
            newConsecutive = 1;
          }
        } else {
          newConsecutive = 1;
        }
      }

      return {
        ...prev,
        activeHours: newActiveHours,
        favoriteExpressions: newFavs,
        totalInteractions: prev.totalInteractions + 1,
        consecutiveDays: newConsecutive,
        lastActiveDate: dateStr
      };
    });
  }, []);

  const getFavoriteExpression = useCallback(() => {
    const entries = Object.entries(memory.favoriteExpressions);
    if (entries.length === 0) return null;
    return entries.reduce((a, b) => (a[1] > b[1] ? a : b))[0];
  }, [memory.favoriteExpressions]);

  return { memory, recordInteraction, getFavoriteExpression };
};
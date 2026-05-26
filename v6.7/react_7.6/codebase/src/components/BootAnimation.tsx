import React, { useEffect } from 'react';
import { motion } from 'framer-motion';
import { useNavigate, useSearchParams } from 'react-router-dom';
import { TOKENS_TABLE, ThemeName } from '../theme/tokens';

interface BootAnimationProps {
  theme: string;
}

export const BootAnimation: React.FC<BootAnimationProps> = ({ theme }) => {
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();
  const t = TOKENS_TABLE[theme as ThemeName] || TOKENS_TABLE.tech;

  useEffect(() => {
    const timer = setTimeout(() => {
      const newParams = new URLSearchParams(searchParams);
      newParams.set('state', 'idle');
      navigate(`/?${newParams.toString()}`);
    }, 3500);

    return () => {
      clearTimeout(timer);
    };
  }, [navigate, searchParams]);

  return (
    <motion.div 
      className="absolute inset-0 flex flex-col items-center justify-center overflow-hidden z-50"
      initial={{ backgroundColor: '#000000' }}
      animate={{ backgroundColor: t.bg }}
      transition={{ duration: 1.5, ease: 'easeOut' }}
    >
      <div className="relative w-full h-full flex items-center justify-center">
        {/* Left Eye */}
        <motion.div 
          className="absolute"
          style={{ backgroundColor: t.accent_hi, boxShadow: `0 0 20px ${t.accent_hi}B3`, width: 32, left: '80px', top: '90px', borderRadius: 16 }}
          initial={{ height: 2, y: 10 }}
          animate={{
            height: [2, 10, 2, 16, 40],
            y: [10, 5, 10, 5, -10],
          }}
          transition={{ duration: 2.5, times: [0, 0.2, 0.4, 0.6, 1], delay: 0.5, ease: "easeInOut" }}
        />
        {/* Right Eye */}
        <motion.div 
          className="absolute"
          style={{ backgroundColor: t.accent_hi, boxShadow: `0 0 20px ${t.accent_hi}B3`, width: 32, right: '80px', top: '90px', borderRadius: 16 }}
          initial={{ height: 2, y: 10 }}
          animate={{
            height: [2, 12, 2, 14, 40],
            y: [10, 5, 10, 5, -10],
          }}
          transition={{ duration: 2.5, times: [0, 0.25, 0.45, 0.6, 1], delay: 0.5, ease: "easeInOut" }}
        />

        {/* Mouth */}
        <motion.div 
          className="absolute"
          style={{ backgroundColor: t.accent_hi, boxShadow: `0 0 20px ${t.accent_hi}B3`, top: '150px' }}
          initial={{ width: 16, height: 2, borderRadius: '2px' }}
          animate={{
            width: [16, 16, 28, 24],
            height: [2, 4, 36, 10],
            borderRadius: ['2px', '4px', '16px', '0 0 12px 12px'],
            y: [0, 0, 10, 0]
          }}
          transition={{ duration: 2, times: [0, 0.3, 0.7, 1], delay: 1.0, ease: "easeInOut" }}
        />

      </div>
    </motion.div>
  );
};
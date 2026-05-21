import React, { useEffect } from 'react';
import { motion } from 'framer-motion';
import { useNavigate, useSearchParams } from 'react-router-dom';

interface BootAnimationProps {
  theme: 'tech' | 'child' | 'dev';
}

export const BootAnimation: React.FC<BootAnimationProps> = ({ theme }) => {
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();

  useEffect(() => {
    const timer = setTimeout(() => {
      const newParams = new URLSearchParams(searchParams);
      newParams.set('state', 'idle');
      navigate(`/?${newParams.toString()}`);
    }, 3500);
    return () => clearTimeout(timer);
  }, [navigate, searchParams]);

  if (theme === 'tech' || theme === 'dev') {
    const mainColor = theme === 'tech' ? 'cyan' : 'green';
    const hexColor = theme === 'tech' ? '34,211,238' : '34,197,94';
    return (
      <div className="absolute inset-0 bg-black flex flex-col items-center justify-center overflow-hidden z-50">
        <motion.div
          initial={{ width: 0, opacity: 0 }}
          animate={{ width: '80%', opacity: 1 }}
          transition={{ duration: 0.5, delay: 0.5, ease: "easeOut" }}
          className={`h-1 bg-${mainColor}-400 shadow-[0_0_15px_rgba(${hexColor},0.8)]`}
        />
        <motion.div
          initial={{ opacity: 0, y: 10 }}
          animate={{ opacity: [0, 1, 0.5, 1], y: 0 }}
          transition={{ duration: 1.5, delay: 1 }}
          className={`mt-4 text-${mainColor}-400 font-mono text-xs tracking-[0.2em]`}
        >
          SYSTEM BOOTING...
        </motion.div>
        
        <div className="flex gap-1 mt-6">
          {[0, 1, 2, 3, 4].map((i) => (
            <motion.div
              key={i}
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              transition={{ delay: 1.5 + i * 0.2 }}
              className={`w-4 h-2 bg-${mainColor}-500 shadow-[0_0_8px_rgba(${hexColor},0.5)]`}
            />
          ))}
        </div>
      </div>
    );
  }

  return (
    <div className="absolute inset-0 bg-[#FFF9E6] flex flex-col items-center justify-center overflow-hidden z-50">
       <motion.div
          initial={{ scale: 0, rotate: -180 }}
          animate={{ scale: 1, rotate: 0 }}
          transition={{ type: "spring", damping: 12, stiffness: 100, delay: 0.5 }}
          className="w-16 h-16 bg-[#FF7F50] rounded-full flex items-center justify-center shadow-lg"
       >
         <motion.div 
           animate={{ y: [0, -5, 0] }}
           transition={{ repeat: Infinity, duration: 1 }}
           className="text-white font-bold text-xl"
         >
           :)
         </motion.div>
       </motion.div>
       
       <motion.div
         initial={{ opacity: 0, y: 10 }}
         animate={{ opacity: 1, y: 0 }}
         transition={{ delay: 1.2, type: "spring" }}
         className="mt-6 text-[#FF7F50] font-black text-lg tracking-widest drop-shadow-[0_2px_4px_rgba(255,127,80,0.3)]"
       >
         HELLO!
       </motion.div>
    </div>
  );
};
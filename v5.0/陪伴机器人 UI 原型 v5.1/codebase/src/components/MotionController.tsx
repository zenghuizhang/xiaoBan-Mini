import React, { useEffect, useRef } from 'react';
import { playAudioFeedback } from '../lib/audioFeedback';
import { playHapticFeedback } from '../lib/hapticFeedback';

export interface MotionData {
  pitch: number;
  roll: number;
  accel: number;
  zAccel: number;
}

interface MotionControllerProps {
  onMotionAction: (action: 'curious' | 'yawn' | 'look_around' | 'dizzy' | 'wink') => void;
  onDataUpdate?: (data: MotionData) => void;
}

export const MotionController: React.FC<MotionControllerProps> = ({ onMotionAction, onDataUpdate }) => {
  const lastActionTime = useRef<number>(0);

  useEffect(() => {
    const handleMotion = (event: DeviceMotionEvent) => {
      const { accelerationIncludingGravity, acceleration } = event;
      if (!accelerationIncludingGravity) return;

      const x = accelerationIncludingGravity.x || 0;
      const y = accelerationIncludingGravity.y || 0;
      const z = accelerationIncludingGravity.z || 0;
      
      const pureX = acceleration?.x || 0;
      const pureY = acceleration?.y || 0;
      const pureZ = acceleration?.z || 0;

      // Calculate pitch and roll
      const pitch = Math.atan2(y, Math.sqrt(x * x + z * z)) * (180 / Math.PI);
      const roll = Math.atan2(-x, z) * (180 / Math.PI);
      
      const accelMagnitude = Math.sqrt(pureX * pureX + pureY * pureY + pureZ * pureZ) / 9.81;
      const zImpact = Math.abs(pureZ) / 9.81;

      if (onDataUpdate) {
        onDataUpdate({
          pitch: Math.round(pitch),
          roll: Math.round(roll),
          accel: Number(accelMagnitude.toFixed(2)),
          zAccel: Number(zImpact.toFixed(2))
        });
      }

      const now = Date.now();
      if (now - lastActionTime.current < 500) return; // 500ms 防抖动

      let actionTriggered = false;

      if (accelMagnitude > 2.5) {
        onMotionAction('dizzy');
        playHapticFeedback('shake');
        playAudioFeedback('motion');
        actionTriggered = true;
      } else if (zImpact > 1.5) {
        onMotionAction('wink');
        playHapticFeedback('motion');
        playAudioFeedback('motion');
        actionTriggered = true;
      } else if (pitch > 15) {
        onMotionAction('curious');
        playHapticFeedback('motion');
        playAudioFeedback('motion');
        actionTriggered = true;
      } else if (pitch < -15) {
        onMotionAction('yawn');
        playHapticFeedback('motion');
        playAudioFeedback('motion');
        actionTriggered = true;
      } else if (Math.abs(roll) > 15) {
        onMotionAction('look_around');
        playHapticFeedback('motion');
        playAudioFeedback('motion');
        actionTriggered = true;
      }

      if (actionTriggered) {
        lastActionTime.current = now;
      }
    };

    // Need user gesture to request permission on some devices, but for prototype we bind directly.
    window.addEventListener('devicemotion', handleMotion);
    return () => {
      window.removeEventListener('devicemotion', handleMotion);
    };
  }, [onMotionAction, onDataUpdate]);

  return null;
};
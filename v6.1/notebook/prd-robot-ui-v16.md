# M5Stack CoreS3 Companion Robot UI v16 (PRD v1.1 final)
- **Pupil Micro-movements**: Eyes now jitter naturally during idle, using X/Y keyframe offsets based on Perlin noise characteristics to mimic realistic gaze.
- **Modified Sine Curve Breathing**: Perfected 5-level breathing modes (`deep_sleep`, `light_rest`, `idle`, `alert`, `excited`) syncing pupil scale, eye scale, and the RGB status strip with `easeInOut` curve approximations for realism.
- **Dev Theme**: Added a third theme (`dev`) using green `#22C55E` for the "Professional Assistant" persona.
- **Somatosensory Matrix**: Mocked physical tilt, roll, and shake triggers (mapping to curious, yawn, thinking, surprised, dizzy, naughty, and wink) in the Developer Simulator Overlay.
- **Feedback Integration**: Built `feedback.ts` to trigger synth tones and vibrations upon interactions (menu open, select, double click).
- **Memory System**: Visual preview inside the Simulator indicating the logic for "Morning Greeting" based on user active hours.
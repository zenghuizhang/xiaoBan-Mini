# M5Stack CoreS3 Companion Robot UI v15 (Interactive Update v1.1)
- **Breathing System Refactoring**: Replaced simple breathing with a 5-level dynamic breathing system (`deep_sleep`, `light_rest`, `idle`, `alert`, `excited`).
  - Utilizes framer-motion keyframes to approximate "Modified Sine" wave.
  - Dynamically responds to environment parameters (Lux, dB, Time) to alter breathing frequency, amplitude, and pupil states.
- **Radial Menu**: Implemented a 6-sector physics-based radial menu triggered by double-click. Includes Spring animations for spreading and selection.
  - Sectors: Expressions (表情), Dialogue (对话), Settings (设置), Theme (主题), Extensions (扩展 - opens Test BottomBar), Random (随机).
- **Environment Simulation**: Added an interactive overlay panel to simulate environmental factors (Light intensity, Noise level, Time of day) and observe the robot's state changes in real-time.
- **Text Layout Protocol**: Enforced strict text rendering zones (Top 20px for status/weather, Bottom 60px for dialog bubbles), avoiding the 80-240px X and 60-180px Y "Face Box" restriction area.
- **Resolution & UI**: 320x240, Chinese UI Language, 3 Themes (Tech, Child, Dev) maintained.